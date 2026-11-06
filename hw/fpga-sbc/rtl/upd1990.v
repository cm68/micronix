// upd1990.v - the Wunderbus/MultIO uPD1990 clock/calendar.
//
// The 40-bit BCD shift register at group 0, port 0x4A: DATA bit 0,
// DSTROBE bit 1, command bits 4-2, CSTROBE bit 5.  Commands: SHOLD,
// ENSR, SET, GET, 64/256/2048/32 Hz.  The 5th byte is month (0-11,
// binary) in the high nibble and weekday in the low nibble; there is no
// year.  The heartbeat is generated from the FPGA clock; the kernel
// programs 32 Hz and mio.s divides by four to 8 Hz.  The calendar is a
// local BCD register for now; the DS1305 becomes the cal_* source.

`default_nettype none

module upd1990 (
    input  wire       clk,
    input  wire       reset_n,

    input  wire       cs,
    input  wire       rd_n,
    input  wire       wr_n,
    input  wire [7:0] din,
    output reg  [7:0] dout,

    output reg        int_req,

    input  wire [7:0] cal_sec, cal_min, cal_hour, cal_day,
    input  wire [3:0] cal_month,
    input  wire [2:0] cal_wday,
    output reg        cal_set,
    output wire [7:0] wr_sec, wr_min, wr_hour, wr_day,
    output wire [3:0] wr_month,
    output wire [2:0] wr_wday
);

    reg [39:0] rtc;
    reg [39:0] wrclock;
    reg [5:0]  rtcptr, wrptr;
    reg        dirty;
    reg [7:0]  last;

    reg [15:0] rate;            // 32, 64, 256, 2048; 0 = off
    reg [31:0] hb_div;
    reg [31:0] hb_cnt;

    assign wr_sec   = wrclock[7:0];
    assign wr_min   = wrclock[15:8];
    assign wr_hour  = wrclock[23:16];
    assign wr_day   = wrclock[31:24];
    assign wr_month = wrclock[35:32];
    assign wr_wday  = wrclock[38:36];

    wire [2:0] cmd      = din[4:2];
    wire       setcmd   = din[5];
    wire       shift    = din[1];
    wire       cmd_fall = !setcmd && last[5] && (cmd == last[4:2]);
    wire       shift_fall = !shift && last[1] && (cmd == last[4:2]);

    always @* begin
        dout = {7'b0000000, rtc[rtcptr]};
    end

    always @(posedge clk or negedge reset_n) begin
        if (!reset_n) begin
            rtc <= {8'h76, 8'h25, 8'h12, 8'h34, 8'h25};
            wrclock <= 40'h0000000000;
            rtcptr <= 6'd0; wrptr <= 6'd0;
            dirty <= 1'b0; last <= 8'h00;
            rate <= 16'd0; hb_div <= 32'd0; hb_cnt <= 32'd0;
            int_req <= 1'b0;
            cal_set <= 1'b0;
        end else begin
            cal_set <= 1'b0;

            // heartbeat
            if (rate != 16'd0) begin
                if (hb_cnt >= hb_div - 32'd1) begin
                    hb_cnt <= 32'd0;
                    int_req <= 1'b1;
                end else begin
                    hb_cnt <= hb_cnt + 32'd1;
                end
            end

            if (cs && !rd_n)
                int_req <= 1'b0;

            if (cs && !wr_n) begin
                if (cmd_fall) begin
                    case (cmd)
                        3'd0: ;                       // SHOLD
                        3'd1: begin                   // ENSR
                            rtcptr <= 6'd0;
                            wrptr  <= 6'd0;
                            dirty  <= 1'b0;
                        end
                        3'd2: begin                   // SET
                            if (dirty) begin
                                cal_set <= 1'b1;
                                dirty   <= 1'b0;
                            end
                        end
                        3'd3: begin                   // GET
                            rtc <= {cal_month, 1'b0, cal_wday,
                                    cal_day, cal_hour, cal_min, cal_sec};
                        end
                        3'd4: begin rate <= 16'd64;   hb_div <= 32'd62500;  end
                        3'd5: begin rate <= 16'd256;  hb_div <= 32'd15625;  end
                        3'd6: begin rate <= 16'd2048; hb_div <= 32'd1953;   end
                        3'd7: begin rate <= 16'd32;   hb_div <= 32'd125000; end
                    endcase
                end

                if (shift_fall) begin
                    wrclock[wrptr] <= din[0];
                    dirty <= 1'b1;
                    if (wrptr == 6'd39) wrptr <= 6'd0; else wrptr <= wrptr + 6'd1;
                    if (rtcptr == 6'd39) rtcptr <= 6'd0; else rtcptr <= rtcptr + 6'd1;
                end

                last <= din;
            end
        end
    end

endmodule

`default_nettype wire
