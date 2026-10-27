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

    // map RAM B permission nibble: {unused, RI10, execute, read}
    input  wire [3:0]  perm,

    // from U2
    input  wire        bus_grant,
    input  wire        stop_i,
    input  wire        aux_i,
    input  wire        int_req,

    // from the A23-A20 = F comparator
    input  wire        flash_phys,

    // task nibble to the external map address mux
    output wire [3:0]  task,

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

    // ------------------------------------------------------------------
    // Bus decode
    // ------------------------------------------------------------------
    wire m1      = ~m1_n;
    wire mreq    = ~mreq_n;
    wire iorq    = ~iorq_n;
    wire rd      = ~rd_n;
    wire wr      = ~wr_n;

    wire mem     = mreq & ~iorq;
    wire m1_read = m1 & mem & rd;
    wire mem_read  = mem & rd;
    wire mem_write = mem & wr;
    wire io_read   = iorq & rd;
    wire io_write  = iorq & wr;

    wire [3:0] addr_lo = {a3, a2, a1, a0};
    wire [3:0] page    = {a15, a14, a13, a12};

    // The 74AHCT138 is enabled by SEG0 & TASK0; any asserted select means
    // the access is the on-board page.  The FPU select may instead be a
    // second 1K RAM bank.
    // The external 74AHCT138 must be enabled only for memory cycles; the
    // mem term here is the second line of defence against an I/O address
    // accidentally matching the local page.
    wire local_page = mem & ~(sel_ram_n & sel_regs_n & sel_map_n &
                              sel_rom_n & sel_fpu_n);
    wire local      = local_page | (flash_phys & mem);

    // Trap-window state is declared early because the decode and every
    // control output below are qualified by it.
    reg  [3:0] trap_count;
    reg        trap_active;
    reg        trap_armed;
    reg        flash_half_r;

    wire local_reg  = mem & ~sel_regs_n;
    wire local_ram  = mem & ~sel_ram_n;
    wire local_rom  = mem & ~sel_rom_n;
    wire local_fpu  = mem & ~sel_fpu_n;

    wire reg_400 = local_reg & (addr_lo == 4'h0);
    wire reg_401 = local_reg & (addr_lo == 4'h1);
    wire reg_402 = local_reg & (addr_lo == 4'h2);
    wire reg_403 = local_reg & (addr_lo == 4'h3);

    wire rd_reg = local_reg & rd & ~trap_active;
    wire wr_reg = local_reg & wr & ~trap_active;

    wire task_write = wr_reg & reg_402;
    wire mask_write = wr_reg & reg_403;
    wire trap_addr_read = rd_reg & reg_400;
    wire keyb_read      = rd_reg & reg_401;
    wire status_read    = rd_reg & reg_403;

    wire map_write = mem & ~sel_map_n & wr & ~trap_active;

    // A normal S-100 memory cycle: not local, not the flash region, not
    // inside the forced trap window.
    wire mapped = mem & ~local & ~trap_active;

    // ------------------------------------------------------------------
    // Task register and 8-M1 switch delay
    //
    // Low nibble is the task/map-table selector.  The high nibble is
    // latched context and is not an address field.
    // ------------------------------------------------------------------
    reg  [3:0] task_cur;
    reg  [3:0] task_next;
    reg  [3:0] task_delay;
    reg  [3:0] task_ctx;

    // On the final M1 the new task is already visible to the map mux and
    // permission check, exactly as mpz80.c applies set_taskreg() before
    // decoding the address.
    wire [3:0] eff_task = (m1_read && (task_delay == 4'd1))
                          ? task_next : task_cur;
    wire       super    = (eff_task == 4'd0);
    wire       user     = ~super;

    assign task = eff_task;

    // ------------------------------------------------------------------
    // Mask and status
    // ------------------------------------------------------------------
    reg  [7:0] mask_reg;
    reg  [7:0] status_reg;

    localparam [7:0] ST_VOID = 8'h01,
                     ST_IORQ = 8'h02,
                     ST_HALT = 8'h04,
                     ST_INT  = 8'h08,
                     ST_STOP = 8'h10,
                     ST_AUX  = 8'h20,
                     ST_RI10 = 8'h40,
                     ST_READ = 8'h80;

    localparam [7:0] ST_RESET = ST_VOID | ST_IORQ | ST_HALT | ST_INT |
                                ST_STOP | ST_AUX | ST_READ;

    // EMU_FAULT_STATUS=1 reports every user memory fault as 0xf7, matching
    // mpz80.c and the kernel's fault().  Set to 0 for the manual's
    // 0x3e/0x7e/0xbe/0xfe forms.
    localparam        EMU_FAULT_STATUS = 1'b1;

    // SINT follows the emulator/kernel: mask bit 5 high blocks supervisor
    // interrupts; low allows them.  (The manual's wording is the reverse.)
    wire mask_stop_dis = mask_reg[0];
    wire mask_aux_en   = mask_reg[1];
    wire mask_tint_dis = mask_reg[2];
    wire mask_halt_dis = mask_reg[4];
    wire mask_sint_dis = mask_reg[5];
    wire mask_io_dis   = mask_reg[6];

    // ------------------------------------------------------------------
    // Prefix tracking, halt substitution and trap sources
    // ------------------------------------------------------------------
    reg prefix;

    wire is_prefix = (d == 8'hcb) || (d == 8'hdd) ||
                     (d == 8'hed) || (d == 8'hfd);

    wire halt_event = m1_read & ~prefix & (d == 8'h76) &
                      mask_halt_dis & ~trap_active & ~trap_armed;

    wire io_event = iorq & ~m1 & user & mask_io_dis &
                    ~trap_active & ~trap_armed;

    wire int_event = int_req & user & mask_tint_dis &
                     ~trap_active & ~trap_armed;

    wire stop_event = stop_i & ~mask_stop_dis &
                      ~trap_active & ~trap_armed;

    wire aux_event = aux_i & mask_aux_en &
                     ~trap_active & ~trap_armed;

    // ------------------------------------------------------------------
    // Permission check and deferred memory fault
    // ------------------------------------------------------------------
    wire perm_read_ok  = perm[0];
    wire perm_exec_ok  = perm[0] | perm[1];   // manual: r/o is executable
    wire perm_write_ok = perm[0] & perm[1];

    wire access_ok = m1_read ? perm_exec_ok :
                     mem_write ? perm_write_ok : perm_read_ok;

    wire ri10 = perm[2];

    wire prot_fault = mapped & user & ~access_ok;
    wire grow_fault = mapped & user & access_ok & ri10;

    wire mem_fault_now = prot_fault | grow_fault;

    reg        mem_fault_pending;
    reg        fault_is_write;
    reg        fault_ri10;
    reg  [7:0] fault_page;

    wire fault_event = mem_fault_pending & m1_read &
                       ~trap_active & ~trap_armed;

    // ------------------------------------------------------------------
    // Trap address shift register: {current page, previous page}
    // ------------------------------------------------------------------
    reg  [7:0] trap_addr_reg;
    reg  [7:0] trap_addr_latch;

    wire trap_shift = mapped & (mem_read | mem_write) & ~trap_active;

    // ------------------------------------------------------------------
    // Trap window, flash counter and data buffer
    // ------------------------------------------------------------------
    wire trap_read = mem_read;          // opcode and operand fetches count

    // The faulting M1 is the first forced read; int/stop/aux/reset arm the
    // window for the next read; the halt's own M1 is substituted with 0x00
    // and the window starts after it.
    wire trap_start_read = trap_read &
                           (fault_event | (trap_armed & ~halt_event));

    wire trap_event = halt_event | io_event | int_event | stop_event |
                      aux_event | fault_event;

    // Status encoding, highest-priority source first.
    reg [7:0] trap_status;
    always @* begin
        trap_status = ST_RESET;
        if (stop_event)
            trap_status = rd ? 8'h2f : 8'haf;
        else if (aux_event)
            trap_status = rd ? 8'h1f : 8'h9f;
        else if (int_event)
            trap_status = 8'hb7;
        else if (halt_event)
            trap_status = 8'hbb;
        else if (io_event)
            trap_status = io_read ? 8'h3c : 8'hbc;
        else if (fault_event) begin
            if (EMU_FAULT_STATUS)
                trap_status = 8'hf7;
            else if (fault_is_write)
                trap_status = fault_ri10 ? 8'hfe : 8'hbe;
            else
                trap_status = fault_ri10 ? 8'h7e : 8'h3e;
        end
    end

    // ------------------------------------------------------------------
    // Interrupt gating (U2 supplies the raw S-100 request)
    // ------------------------------------------------------------------
    assign int_to_cpu = int_req & ~trap_active & ~trap_armed &
                        ((user  & ~mask_tint_dis) |
                         (super & ~mask_sint_dis));

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
            d_out = trap_addr_latch;
        end else if (keyb_read) begin
            d_oe  = 1'b1;
            d_out = 8'hff;
        end else if (status_read) begin
            d_oe  = 1'b1;
            d_out = status_reg;
        end
    end

    assign d = d_oe ? d_out : 8'hzz;

    // ------------------------------------------------------------------
    // Sequential state
    // ------------------------------------------------------------------
    always @(posedge clk or negedge reset_n) begin
        if (!reset_n) begin
            task_cur          <= 4'd0;
            task_next         <= 4'd0;
            task_delay        <= 4'd0;
            task_ctx          <= 4'd0;
            mask_reg          <= 8'h00;
            status_reg        <= ST_RESET;
            prefix            <= 1'b0;
            trap_addr_reg     <= 8'h00;
            trap_addr_latch   <= 8'h00;
            trap_count        <= 4'd0;
            trap_active       <= 1'b0;
            trap_armed        <= 1'b1;      // reset window starts at first read
            flash_half_r      <= 1'b0;      // lower 1K reset half
            mem_fault_pending <= 1'b0;
            fault_is_write    <= 1'b0;
            fault_ri10        <= 1'b0;
            fault_page        <= 8'h00;
        end else begin
            // Prefix: one prefix byte, then the next M1 is the opcode.
            if (m1_read)
                prefix <= is_prefix;
            else if (mem_read)
                prefix <= 1'b0;

            // Task switch delay.  The new task is selected on the final
            // M1, and eff_task already reflects that for this fetch.
            if (m1_read && (task_delay != 4'd0)) begin
                if (task_delay == 4'd1) begin
                    task_cur   <= task_next;
                    task_delay <= 4'd0;
                end else begin
                    task_delay <= task_delay - 4'd1;
                end
            end

            // Trap address shift, and latch on a faulting reference.
            if (trap_shift)
                trap_addr_reg <= {page, trap_addr_reg[7:4]};

            if (mem_fault_now) begin
                mem_fault_pending <= 1'b1;
                fault_is_write    <= mem_write;
                fault_ri10        <= ri10;
                fault_page        <= {page, trap_addr_reg[7:4]};
            end

            if (trap_event) begin
                status_reg      <= trap_status;
                task_cur        <= 4'd0;
                task_next       <= 4'd0;
                task_delay      <= 4'd0;
                trap_armed      <= 1'b1;
            end

            if (fault_event) begin
                trap_addr_latch   <= fault_page;
                mem_fault_pending <= 1'b0;
            end else if (trap_event) begin
                trap_addr_latch   <= trap_shift
                                     ? {page, trap_addr_reg[7:4]}
                                     : trap_addr_reg;
            end

            // Window counter and first-read start.
            if (trap_active) begin
                if (trap_read) begin
                    trap_count <= trap_count - 4'd1;
                    if (trap_count == 4'd1)
                        trap_active <= 1'b0;
                end
            end else if (trap_start_read) begin
                trap_active  <= 1'b1;
                trap_count   <= 4'd15;
                trap_armed   <= 1'b0;
            end

            // Register writes.
            if (task_write) begin
                task_next  <= d_in[3:0];
                task_ctx   <= d_in[7:4];
                task_delay <= 4'd8;
                flash_half_r <= 1'b1;         // upper monitor half
            end

            if (mask_write)
                mask_reg <= d_in;

            // Any non-reset trap selects the upper monitor half.
            if (trap_event)
                flash_half_r <= 1'b1;
        end
    end

    // ------------------------------------------------------------------
    // Output control
    // ------------------------------------------------------------------
    assign local_o = local_page | (flash_phys & mem);

    assign trap_o  = trap_active | halt_event | mem_fault_now |
                     mem_fault_pending | trap_armed;

    assign mapram_oe_n = mapped ? 2'b00 : 2'b11;

    assign map_we = map_write;

    assign addr12_23_oe_n = ~(mapped & ~bus_grant);

    assign flash_cnt_load_n = ~(trap_start_read);
    assign flash_cnt_clr_n  = reset_n;
    assign flash_cnt_clk    = trap_read & trap_active;

    assign flash_half = flash_half_r;
    assign flash_mux  = trap_active | trap_start_read;

    assign flash_ce_n = ~(local_rom | flash_phys | trap_active |
                          trap_start_read | halt_event);
    assign flash_oe_n = ~((local_rom | flash_phys | trap_active |
                           trap_start_read | halt_event) & mem_read);

    assign sram_oe_n = ~(local_ram & mem_read);
    assign sram_we_n = ~(local_ram & mem_write);

    assign trapbuf_oe_n = ~(trap_active | trap_start_read | halt_event);
    assign trapdata_sel = trap_active | trap_start_read;

endmodule

`default_nettype wire
