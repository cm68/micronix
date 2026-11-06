// im0_call.v - supply an 8259 8080-mode CALL to a Z80 in interrupt mode 0.
//
// The kernel programs the 8259 for an 8080-mode three-byte CALL (0xCD,
// low, high) and puts a four-byte JP at each vector slot.  A Z80 in IM0
// executes the opcode read during INTA and then fetches the two address
// bytes from memory at its return PC; a real 8259 drives them on two
// more INTA pulses.  The simulator plays a similar game (src/include/
// z80.h, "special hack for i8259 in mode 0").
//
// This module reproduces the game on the CPU card:
//   - after an INTA whose first byte is 0xCD, the two operand reads
//     (MREQ+RD, M1 high) are answered from the vector the 8259 latched;
//   - the two stack writes (MREQ+WR, M1 high) are given the correct
//     return address (the T80 pushes its PC+2 because its CALL
//     increments PC while fetching the operands);
//   - the 8259 gets the two extra INTA strobes so its byte sequencer
//     returns to the idle state.
//
// The phase advances when a bus strobe is first observed, i.e. at the
// clock edge that samples it; the mux is combinational, so it has been
// presenting the byte for the whole strobe.

`default_nettype none

module im0_call (
    input  wire        clk,
    input  wire        reset_n,

    input  wire        m1_n,
    input  wire        mreq_n,
    input  wire        rd_n,
    input  wire        wr_n,
    input  wire [15:0] a,

    input  wire        inta_level,   // the normal M1+IORQ acknowledge
    input  wire [7:0]  inta_byte,    // what the 8259 drives on INTA
    input  wire [7:0]  vec_low,      // captured at the first INTA
    input  wire [7:0]  vec_high,

    output wire        read_sel,     // use read_byte instead of memory
    output wire [7:0]  read_byte,
    output wire        write_sel,    // use write_byte for the bus write
    output wire [7:0]  write_byte,
    output reg         inta_extra    // extra INTA strobe for the 8259
);

    localparam P_IDLE = 3'd0,
               P_LOW  = 3'd1,       // next operand read is the low byte
               P_HIGH = 3'd2,       // next operand read is the high byte
               P_WRHI = 3'd3,       // next stack write is the high byte
               P_WRLO = 3'd4;       // next stack write is the low byte

    reg [2:0]  phase;
    reg [15:0] ret_pc;
    reg        fetch_d, push_d;

    wire fetch = !mreq_n && !rd_n && m1_n;
    wire push  = !mreq_n && !wr_n && m1_n;

    assign read_sel  = (phase == P_LOW || phase == P_HIGH) && fetch;
    assign read_byte = (phase == P_HIGH) ? vec_high : vec_low;
    assign write_sel = (phase == P_WRHI || phase == P_WRLO) && push;
    assign write_byte = (phase == P_WRLO) ? ret_pc[7:0] : ret_pc[15:8];

    always @(posedge clk or negedge reset_n) begin
        if (!reset_n) begin
            phase      <= P_IDLE;
            ret_pc     <= 16'h0000;
            fetch_d    <= 1'b0;
            push_d     <= 1'b0;
            inta_extra <= 1'b0;
        end else begin
            fetch_d <= fetch;
            push_d  <= push;

            inta_extra <= (phase == P_LOW || phase == P_HIGH) && fetch;

            // A new 8080-mode interrupt starts the sequence.  The level
            // lasts several clocks; re-arming it is harmless because the
            // operand fetches follow the acknowledge.
            if (inta_level && inta_byte == 8'hCD) begin
                phase <= P_LOW;
            end else begin
                case (phase)
                    P_LOW: if (fetch && !fetch_d) begin
                        ret_pc <= a;        // the T80's operand address is PC
                        phase  <= P_HIGH;
                    end
                    P_HIGH: if (fetch && !fetch_d)
                        phase <= P_WRHI;
                    P_WRHI: if (push && !push_d)
                        phase <= P_WRLO;
                    P_WRLO: if (push && !push_d)
                        phase <= P_IDLE;
                    default: phase <= P_IDLE;
                endcase
            end
        end
    end

endmodule

`default_nettype wire
