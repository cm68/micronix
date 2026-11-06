// mpz80_seq.v - the Morrow MPZ80 sequencer, translator and trap engine.
//
// Exact MPZ80 behaviour:
//   - 64K task space, 16 tasks, 16 4K pages each;
//   - 512-byte map RAM at 0x600, write-only, 8-bit segment + attribute;
//   - A20-A23 from the task register's high nibble (bank);
//   - local page = task 0 and address < 0x1000;
//   - 15-read trap window from the monitor ROM at 0xBF0;
//   - 8-M1 task-switch delay;
//   - user halt substitution, I/O traps, permission faults, RI10.
//
// The map is 256 entries of 8 bits for the segment and 256 of 8 bits
// for the attribute.  Both are addressed by {task, page}.  The map is
// forced to logic registers so the translation is combinatorial; a
// block-RAM read would be one clock late.

`default_nettype none

module mpz80_seq (
    input  wire        clk,
    input  wire        reset_n,

    // CPU side
    input  wire [15:0] a,
    input  wire [7:0]  cpu_do,
    input  wire        m1_n,
    input  wire        mreq_n,
    input  wire        iorq_n,
    input  wire        rd_n,
    input  wire        wr_n,
    input  wire        rfsh_n,
    output reg  [7:0]  cpu_di,
    input  wire        ext_sel,     // boot-selector physical region
    input  wire [7:0]  ext_q,
    input  wire        mio_sel,     // an on-FPGA 8259 port or INTA
    input  wire [7:0]  mio_q,

    // local devices
    input  wire [7:0]  ram_q,
    output wire [9:0]  ram_addr,
    output wire        ram_we,
    output wire [7:0]  ram_dout,
    input  wire [7:0]  rom_q,
    output wire [10:0] rom_addr,

    // bus side
    input  wire [7:0]  s100_di,
    output reg  [23:0] phys_addr,
    output wire        local_sel,
    output wire        trap_win,
    output wire        bus_enable,
    output wire        bus_read,
    output wire        bus_write,
    output wire        bus_iorq,
    output wire        bus_m1,
    output wire        bus_inta,
    output wire [7:0]  bus_do,

    // interrupt / front panel
    input  wire        int_n,
    input  wire [7:0]  switches,
    output reg         t80_int_n,
    output reg         t80_nmi_n
);

    // ---------------------------------------------------------------
    // map RAM: 16 tasks x 16 pages, 2 bytes each
    // ---------------------------------------------------------------
    (* ramstyle = "logic" *) reg [7:0] map_lo [0:255];
    (* ramstyle = "logic" *) reg [7:0] map_hi [0:255];

    // ---------------------------------------------------------------
    // registers
    // ---------------------------------------------------------------
    reg [7:0] taskreg;
    reg [7:0] next_taskreg;
    reg [3:0] delay;
    reg [7:0] maskreg;
    reg [7:0] trapstat;
    reg [7:0] trapreg;
    reg [7:0] fault_trapreg;
    reg [3:0] trapcount;
    reg       state;            // 0 = idle, 1 = trap window
    reg       rom_half;         // 0 = lower 1K, 1 = upper 1K
    reg       trap_pending;
    reg       prefix;
    reg       m1_d;
    reg       memread_d;
    reg       fault_d;
    reg       io_d;
    reg       int_d;
    reg       halt_d;

    // ---------------------------------------------------------------
    // CPU bus decode
    // ---------------------------------------------------------------
    wire m1         = (m1_n == 1'b0);
    wire mem        = (mreq_n == 1'b0) && (rfsh_n == 1'b1);
    wire memread    = mem && (rd_n == 1'b0);
    wire memwrite   = mem && (wr_n == 1'b0);
    wire io         = (iorq_n == 1'b0) && (rfsh_n == 1'b1);
    wire ioread     = io && (rd_n == 1'b0);
    wire iowrite    = io && (wr_n == 1'b0);
    wire m1_fetch   = memread && m1;
    wire m1_fetch_p = m1_fetch && !m1_d;
    wire memread_p  = memread && !memread_d;

    // The final M1 of the task-switch countdown is translated with the
    // new map.  This is the emulator's order: set_taskreg() runs before
    // the access.
    wire delay_final = (delay == 4'd1) && m1_fetch;
    wire [7:0] task_eff = delay_final ? next_taskreg : taskreg;

    wire       supervisor    = (task_eff[3:0] == 4'd0);
    wire [3:0] task_sel  = task_eff[3:0];
    wire [3:0] bank     = task_eff[7:4];
    wire       is_local = supervisor && (a[15:12] == 4'h0);
    wire       local_mem = is_local && mem;

    wire [7:0] map_raddr = {task_sel, a[15:12]};
    wire [7:0] seg       = map_lo[map_raddr];
    wire [7:0] attr      = map_hi[map_raddr];

    wire       exec      = m1;
    wire       perm_ok   = exec ? attr[1] : (memread ? attr[0] : (attr[0] & attr[1]));

    // ---------------------------------------------------------------
    // trap conditions
    // ---------------------------------------------------------------
    wire perm_fault = !local_mem && mem && !supervisor && !perm_ok && !trap_win;
    wire ri10       = !local_mem && mem && !supervisor &&  perm_ok && attr[2] && !trap_win;
    wire io_trap    = io && !supervisor && maskreg[6] && !trap_win;
    wire int_req    = (int_n == 1'b0);
    wire int_trap   = int_req && !supervisor && maskreg[2];

    // ---------------------------------------------------------------
    // local page decode and data
    // ---------------------------------------------------------------
    wire [2:0] lsel = a[11:9];

    reg [7:0] reg_q;
    always @* begin
        case (a[1:0])
            2'd0: reg_q = trapreg;
            2'd1: reg_q = 8'h00;        // no front-panel keyboard
            2'd2: reg_q = ~switches;    // switch register, negated
            default: reg_q = trapstat;
        endcase
    end

    wire [7:0] raw_local =
        (lsel <= 3'd1)                 ? ram_q :
        (lsel == 3'd2)                 ? reg_q :
        ((lsel == 3'd4) || (lsel == 3'd5)) ? rom_q :
                                         8'hFF;

    // ---------------------------------------------------------------
    // trap window address and start
    // ---------------------------------------------------------------
    wire start_window = trap_pending && m1_fetch_p && (delay == 4'd0) && (state == 1'b0);
    wire [3:0] wcount = start_window ? 4'd15 : trapcount;
    wire [9:0] woff   = 10'h3F0 + {6'b0, (4'hF - wcount)};
    wire [10:0] trap_rom_addr = {rom_half, woff};
    wire [10:0] local_rom_addr = {rom_half, a[9:0]};
    wire [10:0] rom_sel = (local_mem && ((lsel == 3'd4) || (lsel == 3'd5)))
                          ? local_rom_addr : 11'h000;

    assign rom_addr = trap_win ? trap_rom_addr : rom_sel;
    assign ram_addr = a[9:0];
    assign ram_we   = local_mem && memwrite && (lsel <= 3'd1);
    assign ram_dout = cpu_do;

    // ---------------------------------------------------------------
    // halt substitution
    // ---------------------------------------------------------------
    reg [7:0] raw_di;
    always @* begin
        if (trap_win)             raw_di = rom_q;
        else if (local_mem)       raw_di = raw_local;
        else if (ext_sel)         raw_di = ext_q;
        else if (mio_sel)         raw_di = mio_q;
        else if (memread)         raw_di = perm_fault ? 8'hFF : s100_di;
        else if (bus_inta)        raw_di = s100_di;
        else if (ioread)          raw_di = io_trap ? 8'hFF : s100_di;
        else                      raw_di = 8'hFF;
    end

    wire halt_cond = m1_fetch && !prefix && (raw_di == 8'h76) &&
                     (supervisor || maskreg[4]) && !trap_win;

    always @* begin
        if (halt_cond)      cpu_di = 8'h00;
        else if (trap_win)  cpu_di = rom_q;
        else if (local_mem) cpu_di = raw_local;
        else if (ext_sel)   cpu_di = ext_q;
        else if (mio_sel)   cpu_di = mio_q;
        else if (bus_inta)  cpu_di = s100_di;
        else if (memread)   cpu_di = perm_fault ? 8'hFF : s100_di;
        else if (ioread)    cpu_di = io_trap ? 8'hFF : s100_di;
        else                cpu_di = 8'hFF;
    end

    // ---------------------------------------------------------------
    // bus control
    // ---------------------------------------------------------------
    assign local_sel  = local_mem;
    assign trap_win   = (state == 1'b1) || start_window;
    assign bus_enable = (mem || io) && !local_mem && !trap_win &&
                        !perm_fault && !io_trap && !ext_sel && !mio_sel;

    assign bus_inta  = m1 && io && bus_enable;
    assign bus_read  = bus_enable && !rd_n && !bus_inta;
    assign bus_write = bus_enable && !wr_n && !bus_inta;
    assign bus_iorq  = io && bus_enable;
    assign bus_m1    = m1 && bus_enable;
    assign bus_do    = cpu_do;

    always @* begin
        if (io)
            phys_addr = {8'h00, (maskreg[7] ? a[15:8] : 8'h00), a[7:0]};
        else if (local_mem)
            phys_addr = {8'h00, a[15:0]};
        else
            phys_addr = {bank, seg, a[11:0]};
    end

    // ---------------------------------------------------------------
    // interrupt gating
    // ---------------------------------------------------------------
    always @* begin
        t80_nmi_n = 1'b1;
        if (!int_req)
            t80_int_n = 1'b1;
        else if (supervisor)
            t80_int_n = maskreg[5] ? 1'b1 : 1'b0;
        else
            t80_int_n = maskreg[2] ? 1'b1 : 1'b0;
    end

    // ---------------------------------------------------------------
    // one-shot pulses
    // ---------------------------------------------------------------
    wire halt_pulse = halt_cond && !halt_d;
    wire fault_pulse = (perm_fault || ri10) && !fault_d;
    wire io_pulse = io_trap && !io_d;
    wire int_pulse = int_trap && !int_d;

    // ---------------------------------------------------------------
    // sequential logic
    // ---------------------------------------------------------------
    always @(posedge clk or negedge reset_n) begin
        if (!reset_n) begin
            taskreg     <= 8'h00;
            next_taskreg<= 8'h00;
            delay       <= 4'd0;
            maskreg     <= 8'h00;
            trapstat    <= 8'hBF;
            trapreg     <= 8'h00;
            fault_trapreg <= 8'h00;
            trapcount   <= 4'd15;
            state       <= 1'b0;
            rom_half    <= 1'b0;
            trap_pending<= 1'b1;         // reset window starts on the first read
            prefix      <= 1'b0;
            m1_d        <= 1'b0;
            memread_d   <= 1'b0;
            fault_d     <= 1'b0;
            io_d        <= 1'b0;
            int_d       <= 1'b0;
            halt_d      <= 1'b0;
        end else begin
            m1_d      <= m1_fetch;
            memread_d <= memread;
            fault_d   <= (perm_fault || ri10);
            io_d      <= io_trap;
            int_d     <= int_trap && !trap_win;
            halt_d    <= halt_cond;

            if (m1_fetch_p)
                prefix <= (raw_di == 8'hED) || (raw_di == 8'hDD) ||
                          (raw_di == 8'hFD) || (raw_di == 8'hCB);

            // map write: 0x600 + (task << 5) + (page << 1) + byte
            if (local_mem && memwrite && (lsel == 3'd3)) begin
                if (a[0])
                    map_hi[{a[8:5], a[4:1]}] <= cpu_do;
                else
                    map_lo[{a[8:5], a[4:1]}] <= cpu_do;
            end

            // registers: 0x400-0x403
            if (local_mem && memwrite && (lsel == 3'd2)) begin
                case (a[1:0])
                    2'd2: begin
                        next_taskreg <= cpu_do;
                        delay        <= 4'd8;
                        rom_half     <= 1'b1;
                    end
                    2'd3: maskreg <= cpu_do;
                    default: ;
                endcase
            end

            // trap events are inhibited during the task-switch delay and
            // while a window is already running
            if ((delay == 4'd0) && (state == 1'b0) && !trap_win) begin
                if (halt_pulse) begin
                    taskreg      <= 8'h00;
                    rom_half     <= 1'b1;
                    trap_pending <= 1'b1;
                    trapstat     <= 8'hBB;
                end
                if (fault_pulse) begin
                    fault_trapreg <= {a[15:12], trapreg[7:4]};
                    taskreg       <= 8'h00;
                    rom_half      <= 1'b1;
                    trap_pending  <= 1'b1;
                    trapstat      <= 8'hF7;
                end
                if (io_pulse) begin
                    taskreg      <= 8'h00;
                    rom_half     <= 1'b1;
                    trap_pending <= 1'b1;
                    trapstat     <= ioread ? 8'h3C : 8'hBC;
                end
                if (int_pulse) begin
                    taskreg      <= 8'h00;
                    rom_half     <= 1'b1;
                    trap_pending <= 1'b1;
                    trapstat     <= 8'hB7;
                end
            end

            // 8-M1 task-switch countdown
            if (m1_fetch_p && (delay != 4'd0)) begin
                if (delay == 4'd1) begin
                    taskreg <= next_taskreg;
                    delay   <= 4'd0;
                end else begin
                    delay <= delay - 4'd1;
                end
            end

            // trap window: the first read is the M1 that starts it
            if (start_window) begin
                state        <= 1'b1;
                trapcount    <= 4'd14;
                trap_pending <= 1'b0;
                if (trapstat == 8'hF7)
                    trapreg <= fault_trapreg;
            end else if ((state == 1'b1) && memread_p) begin
                if (trapcount == 4'd1) begin
                    state     <= 1'b0;
                    trapcount <= 4'd0;
                end else begin
                    trapcount <= trapcount - 4'd1;
                end
            end

            // trap-address shift: current page then previous page;
            // frozen during the window
            if (!trap_win && mem && !local_mem)
                trapreg <= {a[15:12], trapreg[7:4]};
        end
    end

endmodule

`default_nettype wire
