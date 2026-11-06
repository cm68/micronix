// i8250.v - compact 8250-compatible UART front end.
//
// Register behavior follows src/hwsim/d1/multio.c: RBR/THR+DLL, IER+DLM,
// IIR/FCR, LCR, MCR, LSR, MSR, SCR, the THRE/TXE-ack rules and the
// loopback the boot's console probe uses.  The byte stream is external:
// tx_data/tx_stb to the backend, rx_data/rx_valid/rx_ack from it.

`default_nettype none

module i8250 (
    input  wire       clk,
    input  wire       reset_n,

    input  wire       cs,
    input  wire [2:0] a,
    input  wire       rd_n,
    input  wire       wr_n,
    input  wire [7:0] din,
    output reg  [7:0] dout,

    output reg  [7:0] tx_data,
    output reg        tx_stb,
    input  wire       tx_busy,
    input  wire [7:0] rx_data,
    input  wire       rx_valid,
    output reg        rx_ack,

    output wire       int_req
);

    reg [7:0] ier, lcr, mcr, msr, dll, dlm, scr;
    reg [7:0] rx_hold;
    reg       rx_full, overrun;
    reg       txe_ack;
    reg       tx_pending;

    wire dlab = lcr[7];
    wire loop = mcr[4];
    wire thre = !tx_busy && !tx_pending;
    wire temt = !tx_busy && !tx_pending;

    wire [7:0] lsr_val = {1'b0, temt, thre, 3'b000, overrun, rx_full};
    wire [7:0] iir_val = (ier[0] && rx_full)                    ? 8'h04 :
                         (ier[1] && thre && !txe_ack)          ? 8'h02 : 8'h01;

    assign int_req = (iir_val != 8'h01);

    always @* begin
        dout = 8'h00;
        if (cs && !rd_n) begin
            case (a)
                3'd0: dout = dlab ? dll : (rx_full ? rx_hold : 8'h00);
                3'd1: dout = dlab ? dlm : ier;
                3'd2: dout = iir_val;
                3'd3: dout = lcr;
                3'd4: dout = mcr;
                3'd5: dout = lsr_val;
                3'd6: dout = msr;
                default: dout = scr;
            endcase
        end
    end

    always @(posedge clk or negedge reset_n) begin
        if (!reset_n) begin
            ier <= 8'h00; lcr <= 8'h00; mcr <= 8'h00;
            msr <= 8'hB0; dll <= 8'h00; dlm <= 8'h00; scr <= 8'h00;
            rx_hold <= 8'h00; rx_full <= 1'b0; overrun <= 1'b0;
            txe_ack <= 1'b0; tx_pending <= 1'b0;
            tx_data <= 8'h00; tx_stb <= 1'b0; rx_ack <= 1'b0;
        end else begin
            tx_stb <= 1'b0;
            rx_ack <= 1'b0;

            if (!tx_busy)
                tx_pending <= 1'b0;

            if (rx_valid && !rx_full) begin
                rx_hold <= rx_data;
                rx_full <= 1'b1;
                rx_ack  <= 1'b1;
            end else if (rx_valid) begin
                overrun <= 1'b1;
                rx_ack  <= 1'b1;
            end

            if (cs && !wr_n) begin
                case (a)
                    3'd0: begin
                        if (dlab) begin
                            dll <= din;
                        end else begin
                            tx_data   <= din;
                            tx_stb    <= 1'b1;
                            tx_pending <= 1'b1;
                            txe_ack   <= 1'b0;
                            if (loop) begin
                                rx_hold <= din;
                                rx_full <= 1'b1;
                            end
                        end
                    end
                    3'd1: begin
                        if (dlab) begin
                            dlm <= din;
                        end else begin
                            ier <= din;
                            if (din[1] && thre)
                                txe_ack <= 1'b0;
                        end
                    end
                    3'd2: ;                 // FCR write; no FIFO model
                    3'd3: lcr <= din;
                    3'd4: mcr <= din;
                    3'd5: ;                 // LSR is read-only in practice
                    3'd6: msr <= din;
                    default: scr <= din;
                endcase
            end

            if (cs && !rd_n) begin
                if (a == 3'd0 && !dlab) begin
                    rx_full <= 1'b0;
                    overrun <= 1'b0;
                end else if (a == 3'd2 && iir_val == 8'h02) begin
                    txe_ack <= 1'b1;
                end else if (a == 3'd5) begin
                    overrun <= 1'b0;
                end
            end
        end
    end

endmodule

`default_nettype wire
