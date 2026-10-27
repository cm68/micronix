// u2.v - Z180 MPZ80-compatible S-100 bus interface CPLD
//
// ATF1508AS-10, 84-pin PLCC.  RTL only; the fitter assigns package pins.
// All S-100 outputs are buffer inputs; the 74ABT/74F parts drive the edge.
//
// The U2 owns the S-100 cycle, wait states, buffer enables, interrupt
// request/acknowledge, DMA acknowledgment, the S-100 clock divider and
// the switch-register status bits.  The trap engine and permission check
// live in U1.

`default_nettype none

module u2_bus (
    input  wire        clk,
    input  wire        reset_n,

    // S-100 clock divider strap: 00=/4, 01=/5, 10=/6, 11=/8
    input  wire [1:0]  clkdiv,

    // Z180 bus
    input  wire        mreq_n,
    input  wire        iorq_n,
    input  wire        rd_n,
    input  wire        wr_n,
    input  wire        m1_n,
    input  wire        busak_n,

    // S-100 inputs (after 74AHCT244 receivers)
    input  wire        s100_rdy,
    input  wire        s100_hold_n,
    input  wire        s100_reset_n,
    input  wire        s100_poc_n,
    input  wire        s100_nmi_n,
    input  wire        s100_pint_n,
    input  wire        s100_vi_or_n,

    // front panel
    input  wire        panel_stop,
    input  wire        panel_aux,

    // U1 inter-CPLD
    input  wire        local_i,
    input  wire        trap_i,
    input  wire        int_to_cpu,
    output wire        stop_o,
    output wire        aux_o,
    output wire        bus_grant,
    output wire        int_req,

    // S-100 control buffer inputs
    output wire        pSYNC,
    output wire        pSTVAL,
    output wire        pDBIN,
    output wire        pWR,
    output wire        sMEMR,
    output wire        sMEMW,
    output wire        sINP,
    output wire        sOUT,
    output wire        sM1,
    output wire        sINTA,
    output wire        pWAIT,

    // buffer enables (active low)
    output wire        a0_11_oe_n,
    output wire        do_oe_n,
    output wire        di_oe_n,
    output wire        ctrl_oe_n,

    // S-100 clock buffer
    output wire        s100_clk1,
    output wire        s100_clk2,

    // S-100 hold acknowledge buffer input
    output wire        phlda,

    // Z180 control
    output wire        z180_wait_n,
    output wire        z180_nmi_n,
    output wire        z180_int0_n,
    output wire        z180_reset_n,
    output wire        z180_busreq_n,

    // switch-register buffer
    output wire        int_pend,
    output wire        trap_reset
);

    // S-100 pRDY polarity.  The IEEE-696 signal is commonly active low;
    // verify against the memory board before changing this.
    localparam RDY_ACTIVE_LOW = 1'b0;

    wire mreq = ~mreq_n;
    wire iorq = ~iorq_n;
    wire rd   = ~rd_n;
    wire wr   = ~wr_n;
    wire m1   = ~m1_n;

    wire local = local_i | trap_i;
    wire grant = ~busak_n;

    wire rdy = RDY_ACTIVE_LOW ? ~s100_rdy : s100_rdy;

    // ------------------------------------------------------------------
    // S-100 bus state machine
    // ------------------------------------------------------------------
    localparam [2:0] IDLE  = 3'd0,
                     ADDR  = 3'd1,
                     READ  = 3'd2,
                     WRITE = 3'd3,
                     WAIT  = 3'd4,
                     DONE  = 3'd5;

    reg [2:0] state;
    reg       is_io;
    reg       is_write;
    reg       is_m1;
    reg [3:0] timeout;

    wire start = !local && !grant && (state == IDLE) &&
                 (mreq || iorq);

    wire wait_done = rdy || (timeout == 4'hf);

    always @(posedge clk or negedge reset_n) begin
        if (!reset_n) begin
            state    <= IDLE;
            is_io    <= 1'b0;
            is_write <= 1'b0;
            is_m1    <= 1'b0;
            timeout  <= 4'd0;
        end else if (local || grant) begin
            // A local/trap access or a DMA grant takes the S-100 bus away
            // immediately.  Returning to IDLE releases /WAIT so the Z180
            // can run the local trap or the DMA master can own the bus.
            state   <= IDLE;
            timeout <= 4'd0;
        end else begin
            case (state)
                IDLE: begin
                    timeout <= 4'd0;
                    if (start) begin
                        is_io    <= iorq;
                        is_write <= wr;
                        is_m1    <= m1;
                        state    <= ADDR;
                    end
                end

                ADDR: begin
                    state <= is_write ? WRITE : READ;
                end

                READ, WRITE: begin
                    if (rdy) begin
                        state <= DONE;
                    end else begin
                        timeout <= timeout + 4'd1;
                        state   <= WAIT;
                    end
                end

                WAIT: begin
                    if (wait_done) begin
                        state <= DONE;
                    end else begin
                        timeout <= timeout + 4'd1;
                    end
                end

                DONE: begin
                    state <= IDLE;
                end

                default: state <= IDLE;
            endcase
        end
    end

    wire cycle = (state != IDLE) && !local && !grant;
    wire read_cycle  = cycle && !is_write;
    wire write_cycle = cycle && is_write;

    // ------------------------------------------------------------------
    // S-100 control outputs (buffer inputs)
    // ------------------------------------------------------------------
    assign pSYNC  = (state == ADDR);
    assign pSTVAL = (state == ADDR);
    assign pDBIN  = read_cycle;
    assign pWR    = write_cycle;

    // S-100 status/control lines sMEMR, sMEMW/sWO, sINP, sOUT, sM1 and
    // sINTA are active low; pSYNC/pSTVAL/pDBIN/pWR are active high.
    assign sMEMR = ~(read_cycle  && !is_io);
    assign sMEMW = ~(write_cycle && !is_io);
    assign sINP  = ~(read_cycle  &&  is_io);
    assign sOUT  = ~(write_cycle &&  is_io);

    assign sM1   = ~m1;
    assign sINTA = ~(m1 && iorq);

    // S-100 pWAIT is active low; the Z180 /WAIT is held while the bus
    // cycle is in progress.
    assign pWAIT       = ~(state == WAIT);
    assign z180_wait_n = (state == IDLE) ? 1'b1 : 1'b0;

    // ------------------------------------------------------------------
    // Buffer enables.  All are forced off for a local/trap cycle, while
    // the bus is granted to a DMA master, and for a non-S-100 cycle.
    // ------------------------------------------------------------------
    assign a0_11_oe_n = ~cycle;
    assign do_oe_n    = ~(write_cycle && (state != ADDR));
    assign di_oe_n    = ~(read_cycle  && (state != ADDR));
    assign ctrl_oe_n  = ~(cycle);

    // ------------------------------------------------------------------
    // Interrupts, reset and DMA
    // ------------------------------------------------------------------
    assign stop_o = panel_stop;
    assign aux_o  = panel_aux;

    assign int_req = ~(s100_pint_n & s100_vi_or_n);
    assign int_pend = int_req;

    assign z180_int0_n = ~int_to_cpu;
    assign z180_nmi_n  = s100_nmi_n;

    assign z180_reset_n = reset_n & s100_reset_n & s100_poc_n;
    assign trap_reset   = ~z180_reset_n;

    assign z180_busreq_n = s100_hold_n;
    assign phlda         = ~busak_n;
    assign bus_grant     = ~busak_n;

    // ------------------------------------------------------------------
    // S-100 clock divider: 10 MHz /5 -> 2 MHz with the default strap.
    // The two outputs feed the clock buffer; clk2 is the complement.
    // ------------------------------------------------------------------
    reg [3:0] div_cnt;
    reg       s100_clk;

    wire [3:0] div_n = (clkdiv == 2'b00) ? 4'd4 :
                       (clkdiv == 2'b01) ? 4'd5 :
                       (clkdiv == 2'b10) ? 4'd6 : 4'd8;

    always @(posedge clk or negedge reset_n) begin
        if (!reset_n) begin
            div_cnt  <= 4'd0;
            s100_clk <= 1'b0;
        end else begin
            if (div_cnt >= (div_n - 4'd1))
                div_cnt <= 4'd0;
            else
                div_cnt <= div_cnt + 4'd1;

            s100_clk <= (div_cnt < (div_n >> 1));
        end
    end

    assign s100_clk1 = s100_clk;
    assign s100_clk2 = ~s100_clk;

endmodule

`default_nettype wire
