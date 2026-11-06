// pic_fit_top.v - resource probe for two wb/mio 8259s.
//
// No pin assignments and no clock domain beyond the input clock; the
// fitter's own report is the answer.  This is deliberately separate
// from mpz80_top so the working MPZ80 bitstream is untouched.

`default_nettype none

module pic_fit_top (
    input  wire        clk,
    input  wire        reset_n,
    input  wire [7:0]  addr,
    input  wire        iorq,
    input  wire        rd_n,
    input  wire        wr_n,
    input  wire [7:0]  din,
    input  wire [3:0]  vi,
    input  wire [11:0] onboard,
    input  wire        inta,
    output wire [7:0]  dout,
    output wire        int_req,
    output wire [7:0]  inta_byte
);

    mio_pic #(.CASCADE(1), .SLAVE_PRESENT(1)) u (
        .clk(clk), .reset_n(reset_n),
        .addr(addr), .iorq(iorq), .rd_n(rd_n), .wr_n(wr_n),
        .din(din), .dout(dout),
        .vi(vi), .onboard(onboard),
        .int_req(int_req), .inta(inta), .inta_byte(inta_byte));

endmodule

`default_nettype wire
