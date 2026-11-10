// mpz80_seq.v - the Morrow MPZ80 sequencer, translator and trap engine.
//
// Exact MPZ80 behaviour:
//   - 64K task space, 16 tasks, 16 4K pages each;
//   - 512-byte map RAM at 0x600, write-only, 8-bit segment + attribute;
//   - A20-A23 from the task register's high nibble (bank), with the
//     upward-compatible PTE-bank extension (EXT_PTE_BANK);
//   - local page = task 0 and address < 0x1000;
//   - 15-read trap window from the monitor ROM at 0xBF0;
//   - 8-M1 task-switch delay;
//   - user halt substitution, I/O traps, permission faults, RI10.
//
// The attribute and register names are the common MPZ80 vocabulary used
// by the Z180 CPLD (hw/cpld/u1.v) and the emulator (hwsim/d1/mpz80.c).
// The permission bits are the manual's Table 4-9/4-10: bit 0 read,
// bit 1 execute, bit 2 RI10; execute is any nonzero attribute (Table
// 1-1 makes Read-Only executable), write is Full only.
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

    // Common MPZ80 contract definitions.  The same names and values are
    // in the Z180 CPLD (hw/cpld/u1.v); change one, change both.
    parameter EXT_PTE_BANK = 1;     // 1: A20-A23 from attr[7:4];
                                    // 0: exact MPZ80, taskreg[7:4]
    localparam ATTR_READ   = 0;     // map attribute byte, bit 0
    localparam ATTR_EXEC   = 1;     // bit 1
    localparam ATTR_RI10   = 2;     // bit 2
    localparam ATTR_UNUSED = 3;     // bit 3 (manual: ignored)

    localparam MASK_STOP = 0,
               MASK_AUX  = 1,
               MASK_TINT = 2,
               MASK_HALT = 4,
               MASK_SINT = 5,
               MASK_IO   = 6,
               MASK_ZIO  = 7;

    localparam ST_VOID = 8'h01,
               ST_IORQ = 8'h02,
               ST_HALT = 8'h04,
               ST_INT  = 8'h08,
               ST_STOP = 8'h10,
               ST_AUX  = 8'h20,
               ST_RI10 = 8'h40,
               ST_READ = 8'h80;

    localparam TS_RESET    = 8'hBF, // ST_RESET
               TS_HALT     = 8'hBB,
               TS_INT      = 8'hB7,
               TS_IN       = 8'h3C,
               TS_OUT      = 8'hBC,
               TS_FAULT    = 8'hF7,
               TS_FAULT_RD = 8'h3E,
               TS_FAULT_WR = 8'hBE,
               TS_GROW_RD  = 8'h7E,
               TS_GROW_WR  = 8'hFE,
               TS_STOP_RD  = 8'h2F,
               TS_STOP_WR  = 8'hAF,
               TS_AUX_RD   = 8'h1F,
               TS_AUX_WR   = 8'h9F;

    // ---------------------------------------------------------------
    // map RAM: 16 tasks x 16 pages, 2 bytes each
    // ---------------------------------------------------------------
    (* ramstyle = "logic" *) reg [7:0] map_seg  [0:255];
    (* ramstyle = "logic" *) reg [7:0] map_attr [0:255];

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
    reg       trap_win_r;       // 0 = idle, 1 = trap window running
    reg       rom_half;         // 0 = lower 1K, 1 = upper 1K
    reg       trap_arm;         // a trap is armed, window not started
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

    wire       supervisor = (task_eff[3:0] == 4'd0);
    wire [3:0] task_sel   = task_eff[3:0];
    wire       local      = supervisor && (a[15:12] == 4'h0);
    wire       local_mem  = local && mem;

    wire [7:0] map_raddr = {task_sel, a[15:12]};
    wire [7:0] seg       = map_seg[map_raddr];
    wire [7:0] attr      = map_attr[map_raddr];

    // A20-A23.  The exact MPZ80 banks with the task register's high
    // nibble; the PTE-bank extension takes it per page from the
    // attribute byte, which the manual says is ignored - upward
    // compatible, provided the map is initialized with the bank nibbles.
    wire [3:0] bank = EXT_PTE_BANK ? attr[7:4] : task_eff[7:4];

    wire       exec = m1;
    // Manual Tables 1-1 and 4-9/4-10: execute for any nonzero
    // attribute (Read-Only is executable), read from bit 0, write from
    // both bits (Full only).
    wire       perm_ok = exec ? (attr[ATTR_READ] | attr[ATTR_EXEC]) :
                         (memread ? attr[ATTR_READ] :
                         (attr[ATTR_READ] & attr[ATTR_EXEC]));

    // ---------------------------------------------------------------
    // trap conditions
    // ---------------------------------------------------------------
    wire perm_fault = !local_mem && mem && !supervisor && !perm_ok && !trap_win;
    wire grow       = !local_mem && mem && !supervisor &&  perm_ok &&
                      attr[ATTR_RI10] && !trap_win;
    wire io_trap    = io && !supervisor && maskreg[MASK_IO] && !trap_win;
    wire int_req    = (int_n == 1'b0);
    wire int_trap   = int_req && !supervisor && maskreg[MASK_TINT];

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
    wire start_window = trap_arm && m1_fetch_p && (delay == 4'd0) && (trap_win_r == 1'b0);
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
                     (supervisor || maskreg[MASK_HALT]) && !trap_win;

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
    assign trap_win   = (trap_win_r == 1'b1) || start_window;
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
            phys_addr = {8'h00, (maskreg[MASK_ZIO] ? a[15:8] : 8'h00), a[7:0]};
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
            t80_int_n = maskreg[MASK_SINT] ? 1'b1 : 1'b0;
        else
            t80_int_n = maskreg[MASK_TINT] ? 1'b1 : 1'b0;
    end

    // ---------------------------------------------------------------
    // one-shot pulses
    // ---------------------------------------------------------------
    wire halt_pulse = halt_cond && !halt_d;
    wire fault_pulse = (perm_fault || grow) && !fault_d;
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
            trapstat    <= TS_RESET;
            trapreg     <= 8'h00;
            fault_trapreg <= 8'h00;
            trapcount   <= 4'd15;
            trap_win_r  <= 1'b0;
            rom_half    <= 1'b0;
            trap_arm<= 1'b1;         // reset window starts on the first read
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
            fault_d   <= (perm_fault || grow);
            io_d      <= io_trap;
            int_d     <= int_trap && !trap_win;
            halt_d    <= halt_cond;

            if (m1_fetch_p)
                prefix <= (raw_di == 8'hED) || (raw_di == 8'hDD) ||
                          (raw_di == 8'hFD) || (raw_di == 8'hCB);

            // map write: 0x600 + (task << 5) + (page << 1) + byte
            if (local_mem && memwrite && (lsel == 3'd3)) begin
                if (a[0])
                    map_attr[{a[8:5], a[4:1]}] <= cpu_do;
                else
                    map_seg[{a[8:5], a[4:1]}] <= cpu_do;
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
            if ((delay == 4'd0) && (trap_win_r == 1'b0) && !trap_win) begin
                if (halt_pulse) begin
                    taskreg      <= 8'h00;
                    rom_half     <= 1'b1;
                    trap_arm <= 1'b1;
                    trapstat     <= TS_HALT;
                end
                if (fault_pulse) begin
                    fault_trapreg <= {a[15:12], trapreg[7:4]};
                    taskreg       <= 8'h00;
                    rom_half      <= 1'b1;
                    trap_arm  <= 1'b1;
                    trapstat      <= TS_FAULT;
                end
                if (io_pulse) begin
                    taskreg      <= 8'h00;
                    rom_half     <= 1'b1;
                    trap_arm <= 1'b1;
                    trapstat     <= ioread ? TS_IN : TS_OUT;
                end
                if (int_pulse) begin
                    taskreg      <= 8'h00;
                    rom_half     <= 1'b1;
                    trap_arm <= 1'b1;
                    trapstat     <= TS_INT;
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
                trap_win_r   <= 1'b1;
                trapcount    <= 4'd14;
                trap_arm <= 1'b0;
                if (trapstat == TS_FAULT)
                    trapreg <= fault_trapreg;
            end else if ((trap_win_r == 1'b1) && memread_p) begin
                if (trapcount == 4'd1) begin
                    trap_win_r <= 1'b0;
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
