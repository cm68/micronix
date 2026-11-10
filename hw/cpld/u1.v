// u1.v - Z180 MPZ80-compatible sequencer + translation CPLD
//
// ATF1508AS-10, 84-pin PLCC.  This is RTL, not a fitted design.
// Pin names match hw/cpld/pins_u1.csv; package pin numbers are assigned
// by the fitter/board.
//
// The external map RAMs (CY7C199-15PC) and flash are not modelled here;
// this module drives their address/control pins and reads only the four
// permission bits from map RAM B.
//
// Trap model: the 15-byte sequence at 0xbf0 is fetched from the on-board
// flash.  The CPU's address is ignored for the 15 forced reads; the
// external 74AHCT161/163 counter supplies the flash offset.  A user HALT
// is replaced with 0x00 on its own M1, and the 15-read window starts on
// the next read.

`default_nettype none

module u1_sequencer (
    input  wire        clk,
    input  wire        reset_n,

    // Z180 address (only the decoded bits enter this device)
    input  wire        a0,
    input  wire        a1,
    input  wire        a2,
    input  wire        a3,
    input  wire        a12,
    input  wire        a13,
    input  wire        a14,
    input  wire        a15,

    // local decode from the external 74AHCT138 (active low)
    input  wire        sel_ram_n,
    input  wire        sel_regs_n,
    input  wire        sel_map_n,
    input  wire        sel_rom_n,
    input  wire        sel_fpu_n,

    // Z180 data bus (registers and local device traffic)
    inout  wire [7:0]  d,

    // Z180 bus control
    input  wire        m1_n,
    input  wire        mreq_n,
    input  wire        iorq_n,
    input  wire        rd_n,
    input  wire        wr_n,
    input  wire        halt_n,

    // map RAM B attribute nibble: {unused, RI10, execute, read}
    input  wire [3:0]  pte_attr,

    // from U2
    input  wire        bus_grant,
    input  wire        stop_i,
    input  wire        aux_i,
    input  wire        int_req,

    // from the A23-A20 = F comparator
    input  wire        flash_phys,

    // task nibble to the external map address mux
    output wire [3:0]  task_o,

    // local/trap state to U2
    output wire        local_o,
    output wire        trap_o,

    // map RAM and address-driver control
    output wire [1:0]  mapram_oe_n,     // {B,A}
    output wire        map_we,
    output wire        addr12_23_oe_n,

    // flash trap-window counter and address mux
    output wire        flash_cnt_load_n,
    output wire        flash_cnt_clr_n,
    output wire        flash_cnt_clk,
    output wire        flash_half,
    output wire        flash_mux,       // 1 = trap counter selected

    // flash and local SRAM
    output wire        flash_ce_n,
    output wire        flash_oe_n,
    output wire        sram_oe_n,
    output wire        sram_we_n,

    // trap data buffer / 0x00 substitution
    output wire        trapbuf_oe_n,
    output wire        trapdata_sel,    // 1 = flash, 0 = 0x00

    // interrupt request allowed through to the Z180 /INT0
    output wire        int_to_cpu
);

    // Common MPZ80 contract definitions.  The same names and values are
    // in the FPGA sequencer (hw/fpga-sbc/rtl/mpz80_seq.v); change one,
    // change both.  Manual Tables 1-1, 4-9 and 4-10.
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

    localparam TS_RESET    = 8'hBF,
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

    // ------------------------------------------------------------------
    // Bus decode
    // ------------------------------------------------------------------
    wire m1      = ~m1_n;
    wire mreq    = ~mreq_n;
    wire iorq    = ~iorq_n;
    wire rd      = ~rd_n;
    wire wr      = ~wr_n;

    wire mem     = mreq & ~iorq;
    wire m1_fetch = m1 & mem & rd;
    wire memread  = mem & rd;
    wire memwrite = mem & wr;
    wire ioread   = iorq & rd;
    wire iowrite  = iorq & wr;

    wire [3:0] addr_lo = {a3, a2, a1, a0};
    wire [3:0] page    = {a15, a14, a13, a12};

    // The 74AHCT138 is enabled by SEG0 & TASK0; any asserted select means
    // the access is the on-board page.  The FPU select may instead be a
    // second 1K RAM bank.
    // The external 74AHCT138 must be enabled only for memory cycles; the
    // mem term here is the second line of defence against an I/O address
    // accidentally matching the local page.
    wire local     = mem & ~(sel_ram_n & sel_regs_n & sel_map_n &
                             sel_rom_n & sel_fpu_n);
    wire local_mem = local | (flash_phys & mem);

    // Trap-window state is declared early because the decode and every
    // control output below are qualified by it.
    reg  [3:0] trapcount;
    reg        trap_win;
    reg        trap_arm;
    reg        flash_half_r;

    wire local_reg  = mem & ~sel_regs_n;
    wire local_ram  = mem & ~sel_ram_n;
    wire local_rom  = mem & ~sel_rom_n;
    wire local_fpu  = mem & ~sel_fpu_n;

    wire reg_400 = local_reg & (addr_lo == 4'h0);
    wire reg_401 = local_reg & (addr_lo == 4'h1);
    wire reg_402 = local_reg & (addr_lo == 4'h2);
    wire reg_403 = local_reg & (addr_lo == 4'h3);

    wire rd_reg = local_reg & rd & ~trap_win;
    wire wr_reg = local_reg & wr & ~trap_win;

    wire task_write = wr_reg & reg_402;
    wire mask_write = wr_reg & reg_403;
    wire trap_addr_read = rd_reg & reg_400;
    wire keyb_read      = rd_reg & reg_401;
    wire status_read    = rd_reg & reg_403;

    wire map_write = mem & ~sel_map_n & wr & ~trap_win;

    // A normal S-100 memory cycle: not local, not the flash region, not
    // inside the forced trap window.
    wire mapped = mem & ~local_mem & ~trap_win;

    // ------------------------------------------------------------------
    // Task register and 8-M1 switch delay
    //
    // Low nibble is the task/map-table selector.  The high nibble is
    // latched context and is not an address field.
    // ------------------------------------------------------------------
    reg  [3:0] taskreg;
    reg  [3:0] next_taskreg;
    reg  [3:0] delay;
    reg  [3:0] task_hi;

    // On the final M1 the new task is already visible to the map mux and
    // permission check, exactly as mpz80.c applies set_taskreg() before
    // decoding the address.
    wire [3:0] task_eff = (m1_fetch && (delay == 4'd1))
                          ? next_taskreg : taskreg;
    wire       supervisor    = (task_eff == 4'd0);
    wire       user     = ~supervisor;

    assign task_o = task_eff;

    // ------------------------------------------------------------------
    // Mask and status
    // ------------------------------------------------------------------
    reg  [7:0] maskreg;
    reg  [7:0] trapstat;

    localparam [7:0] ST_RESET = TS_RESET;

    // EMU_FAULT_STATUS=1 reports every user memory fault as 0xf7, matching
    // mpz80.c and the kernel's fault().  Set to 0 for the manual's
    // 0x3e/0x7e/0xbe/0xfe forms.
    localparam        EMU_FAULT_STATUS = 1'b1;

    // SINT follows the emulator/kernel: mask bit 5 high blocks supervisor
    // interrupts; low allows them.  (The manual's wording is the reverse.)
    wire mask_stop_dis = maskreg[MASK_STOP];
    wire mask_aux_en   = maskreg[MASK_AUX];
    wire mask_tint_dis = maskreg[MASK_TINT];
    wire mask_halt_dis = maskreg[MASK_HALT];
    wire mask_sint_dis = maskreg[MASK_SINT];
    wire mask_io_dis   = maskreg[MASK_IO];

    // ------------------------------------------------------------------
    // Prefix tracking, halt substitution and trap sources
    // ------------------------------------------------------------------
    reg prefix;

    wire is_prefix = (d == 8'hcb) || (d == 8'hdd) ||
                     (d == 8'hed) || (d == 8'hfd);

    wire halt_event = m1_fetch & ~prefix & (d == 8'h76) &
                      mask_halt_dis & ~trap_win & ~trap_arm;

    wire io_event = iorq & ~m1 & user & mask_io_dis &
                    ~trap_win & ~trap_arm;

    wire int_event = int_req & user & mask_tint_dis &
                     ~trap_win & ~trap_arm;

    wire stop_event = stop_i & ~mask_stop_dis &
                      ~trap_win & ~trap_arm;

    wire aux_event = aux_i & mask_aux_en &
                     ~trap_win & ~trap_arm;

    // ------------------------------------------------------------------
    // Permission check and deferred memory fault
    // ------------------------------------------------------------------
    // Manual Table 1-1 and Tables 4-9/4-10: Read-Only (01) may
    // execute and read, Execute-Only (10) may execute, Full (11) may
    // do everything.  So execute is any nonzero attribute, read is
    // bit 0 and write needs both bits.
    wire perm_read  = pte_attr[ATTR_READ];
    wire perm_exec  = pte_attr[ATTR_READ] | pte_attr[ATTR_EXEC];
    wire perm_write = pte_attr[ATTR_READ] & pte_attr[ATTR_EXEC];

    wire perm_ok = m1_fetch ? perm_exec :
                   memwrite ? perm_write : perm_read;

    wire attr_grow = pte_attr[ATTR_RI10];

    wire perm_fault = mapped & user & ~perm_ok;
    wire grow       = mapped & user & perm_ok & attr_grow;

    wire mem_fault = perm_fault | grow;

    reg        fault_pending;
    reg        fault_is_write;
    reg        fault_grow;
    reg  [7:0] fault_trapreg;

    wire fault_event = fault_pending & m1_fetch &
                       ~trap_win & ~trap_arm;

    // ------------------------------------------------------------------
    // Trap address shift register: {current page, previous page}
    // ------------------------------------------------------------------
    reg  [7:0] trapreg;
    reg  [7:0] trapreg_hold;

    wire trap_shift = mapped & (memread | memwrite) & ~trap_win;

    // ------------------------------------------------------------------
    // Trap window, flash counter and data buffer
    // ------------------------------------------------------------------
    wire trap_read = memread;          // opcode and operand fetches count

    // The faulting M1 is the first forced read; int/stop/aux/reset arm the
    // window for the next read; the halt's own M1 is substituted with 0x00
    // and the window starts after it.
    wire trap_start_read = trap_read &
                           (fault_event | (trap_arm & ~halt_event));

    wire trap_event = halt_event | io_event | int_event | stop_event |
                      aux_event | fault_event;

    // Status encoding, highest-priority source first.
    reg [7:0] trapstat_next;
    always @* begin
        trapstat_next = ST_RESET;
        if (stop_event)
            trapstat_next = rd ? TS_STOP_RD : TS_STOP_WR;
        else if (aux_event)
            trapstat_next = rd ? TS_AUX_RD : TS_AUX_WR;
        else if (int_event)
            trapstat_next = TS_INT;
        else if (halt_event)
            trapstat_next = TS_HALT;
        else if (io_event)
            trapstat_next = ioread ? TS_IN : TS_OUT;
        else if (fault_event) begin
            if (EMU_FAULT_STATUS)
                trapstat_next = TS_FAULT;
            else if (fault_is_write)
                trapstat_next = fault_grow ? TS_GROW_WR : TS_FAULT_WR;
            else
                trapstat_next = fault_grow ? TS_GROW_RD : TS_FAULT_RD;
        end
    end

    // ------------------------------------------------------------------
    // Interrupt gating (U2 supplies the raw S-100 request)
    // ------------------------------------------------------------------
    assign int_to_cpu = int_req & ~trap_win & ~trap_arm &
                        ((user  & ~mask_tint_dis) |
                         (supervisor & ~mask_sint_dis));

    // ------------------------------------------------------------------
    // Data bus
    // ------------------------------------------------------------------
    reg  [7:0] d_out;
    reg        d_oe;
    wire [7:0] d_in = d;

    always @* begin
        d_oe  = 1'b0;
        d_out = 8'hff;
        if (trap_addr_read) begin
            d_oe  = 1'b1;
            d_out = trapreg_hold;
        end else if (keyb_read) begin
            d_oe  = 1'b1;
            d_out = 8'hff;
        end else if (status_read) begin
            d_oe  = 1'b1;
            d_out = trapstat;
        end
    end

    assign d = d_oe ? d_out : 8'hzz;

    // ------------------------------------------------------------------
    // Sequential state
    // ------------------------------------------------------------------
    always @(posedge clk or negedge reset_n) begin
        if (!reset_n) begin
            taskreg          <= 4'd0;
            next_taskreg         <= 4'd0;
            delay        <= 4'd0;
            task_hi          <= 4'd0;
            maskreg          <= 8'h00;
            trapstat        <= ST_RESET;
            prefix            <= 1'b0;
            trapreg     <= 8'h00;
            trapreg_hold   <= 8'h00;
            trapcount        <= 4'd0;
            trap_win       <= 1'b0;
            trap_arm        <= 1'b1;      // reset window starts at first read
            flash_half_r      <= 1'b0;      // lower 1K reset half
            fault_pending <= 1'b0;
            fault_is_write    <= 1'b0;
            fault_grow        <= 1'b0;
            fault_trapreg        <= 8'h00;
        end else begin
            // Prefix: one prefix byte, then the next M1 is the opcode.
            if (m1_fetch)
                prefix <= is_prefix;
            else if (memread)
                prefix <= 1'b0;

            // Task switch delay.  The new task is selected on the final
            // M1, and task_eff already reflects that for this fetch.
            if (m1_fetch && (delay != 4'd0)) begin
                if (delay == 4'd1) begin
                    taskreg   <= next_taskreg;
                    delay <= 4'd0;
                end else begin
                    delay <= delay - 4'd1;
                end
            end

            // Trap address shift, and latch on a faulting reference.
            if (trap_shift)
                trapreg <= {page, trapreg[7:4]};

            if (mem_fault) begin
                fault_pending <= 1'b1;
                fault_is_write    <= memwrite;
                fault_grow        <= attr_grow;
                fault_trapreg        <= {page, trapreg[7:4]};
            end

            if (trap_event) begin
                trapstat      <= trapstat_next;
                taskreg        <= 4'd0;
                next_taskreg       <= 4'd0;
                delay      <= 4'd0;
                trap_arm      <= 1'b1;
            end

            if (fault_event) begin
                trapreg_hold   <= fault_trapreg;
                fault_pending <= 1'b0;
            end else if (trap_event) begin
                trapreg_hold   <= trap_shift
                                     ? {page, trapreg[7:4]}
                                     : trapreg;
            end

            // Window counter and first-read start.
            if (trap_win) begin
                if (trap_read) begin
                    trapcount <= trapcount - 4'd1;
                    if (trapcount == 4'd1)
                        trap_win <= 1'b0;
                end
            end else if (trap_start_read) begin
                trap_win  <= 1'b1;
                trapcount   <= 4'd15;
                trap_arm   <= 1'b0;
            end

            // Register writes.
            if (task_write) begin
                next_taskreg  <= d_in[3:0];
                task_hi   <= d_in[7:4];
                delay <= 4'd8;
                flash_half_r <= 1'b1;         // upper monitor half
            end

            if (mask_write)
                maskreg <= d_in;

            // Any non-reset trap selects the upper monitor half.
            if (trap_event)
                flash_half_r <= 1'b1;
        end
    end

    // ------------------------------------------------------------------
    // Output control
    // ------------------------------------------------------------------
    assign local_o = local_mem;

    assign trap_o  = trap_win | halt_event | mem_fault |
                     fault_pending | trap_arm;

    assign mapram_oe_n = mapped ? 2'b00 : 2'b11;

    assign map_we = map_write;

    assign addr12_23_oe_n = ~(mapped & ~bus_grant);

    assign flash_cnt_load_n = ~(trap_start_read);
    assign flash_cnt_clr_n  = reset_n;
    assign flash_cnt_clk    = trap_read & trap_win;

    assign flash_half = flash_half_r;
    assign flash_mux  = trap_win | trap_start_read;

    assign flash_ce_n = ~(local_rom | flash_phys | trap_win |
                          trap_start_read | halt_event);
    assign flash_oe_n = ~((local_rom | flash_phys | trap_win |
                           trap_start_read | halt_event) & memread);

    assign sram_oe_n = ~(local_ram & memread);
    assign sram_we_n = ~(local_ram & memwrite);

    assign trapbuf_oe_n = ~(trap_win | trap_start_read | halt_event);
    assign trapdata_sel = trap_win | trap_start_read;

endmodule

`default_nettype wire
