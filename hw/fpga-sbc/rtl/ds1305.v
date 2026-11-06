// ds1305.v - DS1305 SPI calendar for the uPD1990.
//
// Reads the seven time registers once a second into a cache, writes the
// six time registers when the uPD1990 issues SET, and converts between
// the DS1305's BCD month/day (1-12, 1-7) and the uPD1990's binary,
// zero-based month (0-11) and weekday (0-6).  The year is not part of
// the uPD1990; the kernel borrows it from the superblock.

`default_nettype none

module ds1305 (
    input  wire        clk,
    input  wire        reset_n,

    output reg         spi_enable,
    output reg  [15:0] spi_tx,
    input  wire        spi_busy,
    input  wire [15:0] spi_rx,

    output reg  [7:0]  cal_sec, cal_min, cal_hour, cal_day,
    output reg  [3:0]  cal_month,
    output reg  [2:0]  cal_wday,

    input  wire [7:0]  wr_sec, wr_min, wr_hour, wr_day,
    input  wire [3:0]  wr_month,
    input  wire [2:0]  wr_wday,
    input  wire        wr_stb
);

    localparam S_INIT = 2'd0, S_READ = 2'd1, S_WRITE = 2'd2, S_IDLE = 2'd3;

    reg [1:0]  state;
    reg [3:0]  idx;
    reg [2:0]  ph;              // 0 start, 1 wait busy, 2 wait done
    reg [7:0]  rd [0:6];
    reg [31:0] hzcnt;
    reg        read_tick, read_pending, pending_wr;

    function [7:0] bcd2bin;
        input [7:0] b;
        begin
            bcd2bin = ((b >> 4) * 10) + (b & 8'h0F);
        end
    endfunction

    function [7:0] bin2bcd;
        input [7:0] v;
        begin
            bin2bcd = (((v / 10) % 10) << 4) | (v % 10);
        end
    endfunction

    function [7:0] wrdata;
        input [3:0] i;
        begin
            case (i)
                4'd0: wrdata = wr_sec;
                4'd1: wrdata = wr_min;
                4'd2: wrdata = wr_hour;
                4'd3: wrdata = bin2bcd({5'b00000, wr_wday} + 8'd1);
                4'd4: wrdata = wr_day;
                default: wrdata = bin2bcd({4'b0000, wr_month} + 8'd1);
            endcase
        end
    endfunction

    // 1 Hz read trigger
    always @(posedge clk or negedge reset_n) begin
        if (!reset_n) begin
            hzcnt <= 32'd0;
            read_tick <= 1'b0;
        end else if (hzcnt == 32'd3999999) begin
            hzcnt <= 32'd0;
            read_tick <= 1'b1;
        end else begin
            hzcnt <= hzcnt + 32'd1;
            read_tick <= 1'b0;
        end
    end

    always @(posedge clk or negedge reset_n) begin
        if (!reset_n) begin
            state <= S_INIT;
            idx <= 4'd0;
            ph <= 3'd0;
            spi_enable <= 1'b0;
            spi_tx <= 16'h0000;
            read_pending <= 1'b0;
            pending_wr <= 1'b0;
            cal_sec <= 8'h00; cal_min <= 8'h00; cal_hour <= 8'h00;
            cal_day <= 8'h01; cal_month <= 4'd0; cal_wday <= 3'd0;
        end else begin
            spi_enable <= 1'b0;

            if (read_tick)
                read_pending <= 1'b1;
            if (wr_stb)
                pending_wr <= 1'b1;

            if (ph == 3'd0) begin
                case (state)
                    S_INIT: begin
                        spi_tx <= {8'h07, 8'h00};   // CONTROL: EOSC on, WP off
                        spi_enable <= 1'b1;
                        ph <= 3'd1;
                    end
                    S_READ: begin
                        spi_tx <= {1'b1, 1'b0, 2'b00, idx, 8'h00};
                        spi_enable <= 1'b1;
                        ph <= 3'd1;
                    end
                    S_WRITE: begin
                        spi_tx <= {1'b0, 1'b0, 2'b00, idx, wrdata(idx)};
                        spi_enable <= 1'b1;
                        ph <= 3'd1;
                    end
                    default: begin
                        if (pending_wr) begin
                            pending_wr <= 1'b0;
                            state <= S_WRITE;
                            idx <= 4'd0;
                        end else if (read_pending) begin
                            read_pending <= 1'b0;
                            state <= S_READ;
                            idx <= 4'd0;
                        end
                    end
                endcase
            end else if (ph == 3'd1) begin
                if (spi_busy)
                    ph <= 3'd2;
            end else begin
                if (!spi_busy) begin
                    ph <= 3'd0;
                    case (state)
                        S_INIT: begin
                            state <= S_READ;
                            idx <= 4'd0;
                        end
                        S_READ: begin
                            rd[idx] <= spi_rx[7:0];
                            if (idx == 4'd6) begin
                                cal_sec   <= rd[0] & 8'h7F;
                                cal_min   <= rd[1];
                                cal_hour  <= rd[2];
                                cal_day   <= rd[4];
                                cal_month <= bcd2bin(rd[5]) - 8'd1;
                                cal_wday  <= bcd2bin(rd[3]) - 8'd1;
                                state <= S_IDLE;
                                idx <= 4'd0;
                            end else begin
                                idx <= idx + 4'd1;
                            end
                        end
                        S_WRITE: begin
                            if (idx == 4'd5) begin
                                cal_sec   <= wr_sec;
                                cal_min   <= wr_min;
                                cal_hour  <= wr_hour;
                                cal_day   <= wr_day;
                                cal_month <= wr_month;
                                cal_wday  <= wr_wday;
                                state <= S_IDLE;
                                idx <= 4'd0;
                            end else begin
                                idx <= idx + 4'd1;
                            end
                        end
                        default: ;
                    endcase
                end
            end
        end
    end

endmodule

`default_nettype wire
