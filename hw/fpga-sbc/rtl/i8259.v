// i8259.v - a compact, synthesizable 8259A interrupt controller.
//
// The register set and the priority resolver follow src/hwsim/d1/pic.c:
// ICW1/2/3/4, OCW1/2/3, IRR/ISR/IMR, rotating priority, poll and the
// 8080-mode three-byte CALL vector (0xCD, low, high).
//
// The request inputs are treated as edge-sensitive: a rising edge sets
// the IRR bit, a falling edge clears it, and an acknowledge clears it.
// That is the behaviour multio.c relies on: a device that holds its line
// high does not re-request until the line falls and rises again.
//
// This is a first-cut model.  It is deliberately small; the cascade and
// board port decode live in mio_pic.v.

`default_nettype none

module i8259 (
    input  wire        clk,
    input  wire        reset_n,

    // CPU interface
    input  wire        cs,
    input  wire        a0,          // 0 = command, 1 = data
    input  wire        rd_n,
    input  wire        wr_n,
    input  wire [7:0]  din,
    output reg  [7:0]  dout,

    // interrupt requests
    input  wire [7:0]  ir,

    // interrupt output and acknowledge
    output wire        int_req,
    input  wire        inta,        // one pulse per INTA byte
    input  wire        skip,        // cascade delegate: clear irr, no isr
    output reg  [7:0]  inta_byte,

    // cascade visibility
    output reg  [3:0]  service_level,   // 0-7, 4'hF = none
    output reg  [7:0]  icw2_o,
    output wire [1:0]  inta_state_o,
    output wire [7:0]  vec_low_o, vec_high_o
);

    reg [7:0] icw1, icw2, icw3, icw4;
    reg [7:0] ocw2, ocw3;
    reg [7:0] imr, irr, isr;
    reg [2:0] priority;
    reg       auto_eoi, rotate;
    reg       poll_r;
    reg [2:0] state;                // 0 undef, 1 icw2, 2 icw3, 3 icw4, 4 ready
    reg [1:0] inta_state;
    reg [7:0] vec_low_l, vec_high_l;
    reg [7:0] ir_d;
    reg       inta_d;

    wire inta_p = inta && !inta_d;

    // ---------------------------------------------------------------
    // priority resolver
    // ---------------------------------------------------------------
    integer i;
    reg [3:0] isr_best, isr_rank;
    reg [3:0] best, bestrank;

    function [3:0] rank;
        input [2:0] lvl;
        input [2:0] pri;
        begin
            rank = ({1'b0, lvl} - {1'b0, pri}) & 4'h7;
        end
    endfunction

    always @* begin
        isr_best = 4'hF;
        isr_rank = 4'd8;
        for (i = 0; i < 8; i = i + 1) begin
            if (isr[i] && (rank(i[2:0], priority) < isr_rank)) begin
                isr_best = i[3:0];
                isr_rank = rank(i[2:0], priority);
            end
        end

        best = 4'hF;
        bestrank = 4'd8;
        for (i = 0; i < 8; i = i + 1) begin
            if (irr[i] && !imr[i] &&
                (rank(i[2:0], priority) < isr_rank) &&
                (rank(i[2:0], priority) < bestrank)) begin
                best = i[3:0];
                bestrank = rank(i[2:0], priority);
            end
        end
        service_level = best;
    end

    wire [7:0] vec_step = icw1[2] ? {service_level[2:0], 2'b00}
                                  : {service_level[2:0], 3'b000};
    wire [7:0] vec_low = (icw1 & 8'hE0) + vec_step;

    assign int_req = (service_level != 4'hF);
    assign inta_state_o = inta_state;
    assign vec_low_o    = vec_low_l;
    assign vec_high_o   = vec_high_l;

    // ---------------------------------------------------------------
    // register reads
    // ---------------------------------------------------------------
    always @* begin
        dout = 8'h00;
        if (cs && !rd_n) begin
            if (a0)
                dout = imr;
            else if (poll_r)
                dout = (service_level == 4'hF) ? 8'h00
                                               : (8'h80 | {4'b0, service_level[2:0]});
            else if (ocw3[1])
                dout = ocw3[0] ? isr : irr;
        end
    end

    // ---------------------------------------------------------------
    // register writes and state
    // ---------------------------------------------------------------
    always @(posedge clk or negedge reset_n) begin
        if (!reset_n) begin
            icw1 <= 8'h00; icw2 <= 8'h00; icw3 <= 8'h00; icw4 <= 8'h00;
            ocw2 <= 8'h00; ocw3 <= 8'h00;
            imr <= 8'h00; irr <= 8'h00; isr <= 8'h00;
            priority <= 3'd0;
            auto_eoi <= 1'b0; rotate <= 1'b0; poll_r <= 1'b0;
            state <= 3'd0; inta_state <= 2'd0;
            vec_low_l <= 8'h00; vec_high_l <= 8'h00;
            inta_byte <= 8'hFF;
            ir_d <= 8'h00; inta_d <= 1'b0;
            icw2_o <= 8'h00;
        end else begin
            ir_d  <= ir;
            inta_d <= inta;
            icw2_o <= icw2;

            // edge-sensitive request latch
            irr <= (irr | (ir & ~ir_d)) & ~(~ir & ir_d);

            if (cs && !wr_n) begin
                if (!a0) begin
                    if (din[4]) begin                 // ICW1
                        icw1 <= din;
                        icw2_o <= icw2;
                        state <= 3'd1;
                        irr <= 8'h00;
                        isr <= 8'h00;
                        priority <= 3'd0;
                        poll_r <= 1'b0;
                        auto_eoi <= 1'b0;
                        rotate <= 1'b0;
                    end else if (din[3]) begin        // OCW3
                        ocw3 <= din;
                        if (din[2])
                            poll_r <= 1'b1;
                    end else begin                    // OCW2
                        ocw2 <= din;
                        case (din[7:5])
                            3'b000: begin             // rotate clear
                                auto_eoi <= 1'b0;
                                rotate <= 1'b0;
                            end
                            3'b001: begin             // non-specific EOI
                                if (isr_best != 4'hF)
                                    isr[isr_best] <= 1'b0;
                            end
                            3'b011: begin             // specific EOI
                                isr[din[2:0]] <= 1'b0;
                            end
                            3'b101: begin             // non-specific EOI + rotate
                                if (isr_best != 4'hF) begin
                                    isr[isr_best] <= 1'b0;
                                    priority <= (isr_best[2:0] + 3'd1) & 3'd7;
                                end
                            end
                            3'b111: begin             // specific EOI + rotate
                                isr[din[2:0]] <= 1'b0;
                                priority <= (din[2:0] + 3'd1) & 3'd7;
                            end
                            3'b100: begin             // rotate set
                                auto_eoi <= 1'b1;
                                rotate <= 1'b1;
                            end
                            3'b110: begin             // set priority
                                priority <= (din[2:0] + 3'd1) & 3'd7;
                            end
                            default: ;
                        endcase
                    end
                end else begin
                    case (state)
                        3'd1: begin                    // ICW2
                            icw2 <= din;
                            if (!icw1[1])
                                state <= 3'd2;
                            else if (icw1[0])
                                state <= 3'd3;
                            else
                                state <= 3'd4;
                        end
                        3'd2: begin                    // ICW3
                            icw3 <= din;
                            state <= icw1[0] ? 3'd3 : 3'd4;
                        end
                        3'd3: begin                    // ICW4
                            icw4 <= din;
                            state <= 3'd4;
                        end
                        default:                       // IMR
                            imr <= din;
                    endcase
                end
            end

            // OCW3 poll read: return the level and drop the request
            if (cs && !rd_n && !a0 && poll_r) begin
                if (service_level != 4'hF)
                    irr[service_level] <= 1'b0;
                poll_r <= 1'b0;
            end

            // INTA: three bytes, 0xCD low high
            if (inta_p) begin
                if (inta_state == 2'd0) begin
                    if (service_level != 4'hF) begin
                        if (!skip)
                            isr[service_level] <= 1'b1;
                        irr[service_level] <= 1'b0;
                        vec_low_l  <= vec_low;
                        vec_high_l <= icw2;
                        inta_byte  <= 8'hCD;
                    end else begin
                        inta_byte <= 8'hFF;
                    end
                    inta_state <= 2'd1;
                end else if (inta_state == 2'd1) begin
                    inta_byte <= vec_low_l;
                    inta_state <= 2'd2;
                end else begin
                    inta_byte <= vec_high_l;
                    inta_state <= 2'd0;
                end
            end
        end
    end

endmodule

`default_nettype wire
