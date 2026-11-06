// vga_console.v - UART1's console backend: an 80x40 VGA text display with
// a PS/2 keyboard.
//
// UART1's transmit stream is the console output.  The writer handles CR,
// LF, BS, HT and FF and scrolls the 80x40 page.  The PS/2 keyboard feeds
// UART1's receive side through the 8250's rx_valid/rx_ack handshake.
//
// The text buffer is a true dual-port altsyncram (25 MHz display read,
// 50 MHz console read/write); the font is the 4KB lat0-12 bitmap ROM.
// The display core is Javier Valcarce's vga80x40 (LGPL, see the header),
// clocked at 25 MHz from the board PLL.

`default_nettype none

module vga_console (
    input  wire        clk50,
    input  wire        clk25,
    input  wire        reset_n,

    input  wire [7:0]  tx_data,
    input  wire        tx_stb,
    output wire        tx_busy,

    output wire [7:0]  rx_data,
    output wire        rx_valid,
    input  wire        rx_ack,

    output wire        vga_r,
    output wire        vga_g,
    output wire        vga_b,
    output wire        vga_hsync,
    output wire        vga_vsync,

    input  wire        ps2_clk,
    input  wire        ps2_data
);

    // ------------------------------------------------------------------
    // text buffer
    // ------------------------------------------------------------------
    wire [11:0] text_a;
    wire [7:0]  text_d;

    reg  [11:0] ram_addr;
    reg  [7:0]  ram_wdata;
    reg         ram_we;
    wire [7:0]  ram_rdata;

    altsyncram #(
        .clock_enable_input_a("BYPASS"),
        .clock_enable_output_a("BYPASS"),
        .clock_enable_input_b("BYPASS"),
        .clock_enable_output_b("BYPASS"),
        .intended_device_family("Cyclone IV E"),
        .lpm_type("altsyncram"),
        .numwords_a(4096),
        .widthad_a(12),
        .width_a(8),
        .width_byteena_a(1),
        .numwords_b(4096),
        .widthad_b(12),
        .width_b(8),
        .width_byteena_b(1),
        .operation_mode("BIDIR_DUAL_PORT"),
        .outdata_reg_a("CLOCK0"),
        .outdata_aclr_a("NONE"),
        .outdata_reg_b("CLOCK1"),
        .outdata_aclr_b("NONE"),
        .read_during_write_mode_mixed_ports("DONT_CARE"),
        .power_up_uninitialized("FALSE"),
        .init_file("text_ram.mif")
    ) text_ram (
        .clock0(clk25),
        .address_a(text_a),
        .q_a(text_d),
        .clock1(clk50),
        .address_b(ram_addr),
        .data_b(ram_wdata),
        .wren_b(ram_we),
        .q_b(ram_rdata),
        .aclr0(1'b0), .aclr1(1'b0),
        .addressstall_a(1'b0), .addressstall_b(1'b0),
        .byteena_a(1'b1), .byteena_b(1'b1),
        .clocken0(1'b1), .clocken1(1'b1), .clocken2(1'b1), .clocken3(1'b1),
        .data_a(8'h00),
        .rden_a(1'b1), .rden_b(1'b1),
        .wren_a(1'b0),
        .eccstatus()
    );

    // ------------------------------------------------------------------
    // font ROM
    // ------------------------------------------------------------------
    wire [11:0] font_a;
    wire [7:0]  font_d;

    altsyncram #(
        .clock_enable_input_a("BYPASS"),
        .clock_enable_output_a("BYPASS"),
        .intended_device_family("Cyclone IV E"),
        .lpm_hint("ENABLE_RUNTIME_MOD=NO"),
        .lpm_type("altsyncram"),
        .numwords_a(4096),
        .widthad_a(12),
        .width_a(8),
        .width_byteena_a(1),
        .operation_mode("ROM"),
        .outdata_aclr_a("NONE"),
        .outdata_reg_a("UNREGISTERED"),
        .power_up_uninitialized("FALSE"),
        .init_file("lat0-12.hex")
    ) font_rom (
        .clock0(clk25),
        .address_a(font_a),
        .q_a(font_d),
        .aclr0(1'b0), .aclr1(1'b0),
        .addressstall_a(1'b0), .addressstall_b(1'b0),
        .address_b(1'b1),
        .byteena_a(1'b1), .byteena_b(1'b1),
        .clock1(1'b1),
        .clocken0(1'b1), .clocken1(1'b1), .clocken2(1'b1), .clocken3(1'b1),
        .data_a(8'h00), .data_b(1'b1),
        .q_b(),
        .rden_a(1'b1), .rden_b(1'b1),
        .wren_a(1'b0), .wren_b(1'b0),
        .eccstatus()
    );

    // ------------------------------------------------------------------
    // display core
    // ------------------------------------------------------------------
    wire [6:0] cx;
    wire [5:0] cy;

    vga80x40 display (
        .reset(~reset_n),
        .clk25MHz(clk25),
        .TEXT_A(text_a),
        .TEXT_D(text_d),
        .FONT_A(font_a),
        .FONT_D(font_d),
        .ocrx({1'b0, cx}),
        .ocry({2'b00, cy}),
        .octl(8'hF7),           // VGA on, cursor on, blink, underline
        .R(vga_r), .G(vga_g), .B(vga_b),
        .hsync(vga_hsync), .vsync(vga_vsync),
        .cursor_x(), .cursor_y()
    );

    // ------------------------------------------------------------------
    // console writer
    // ------------------------------------------------------------------
    localparam S_IDLE      = 3'd0,
               S_ADV       = 3'd1,
               S_SCROLL_RD = 3'd2,
               S_SCROLL_WR = 3'd3,
               S_SCROLL_WD = 3'd4,
               S_SCROLL_NX = 3'd5,
               S_SCROLL_CLEAR = 3'd6,
               S_CLEAR     = 3'd7;

    reg  [2:0]  state;
    reg  [11:0] idx;
    reg  [7:0]  ch;
    reg  [6:0]  curx;
    reg  [5:0]  cury;
    reg         busy;

    assign tx_busy = busy;
    assign cx = curx;
    assign cy = cury;

    wire [6:0] tabx = {curx[6:3] + 4'd1, 3'b000};

    always @(posedge clk50 or negedge reset_n) begin
        if (!reset_n) begin
            state     <= S_IDLE;
            idx       <= 12'd0;
            ch        <= 8'h00;
            curx      <= 7'd0;
            cury      <= 6'd0;
            busy      <= 1'b0;
            ram_addr  <= 12'd0;
            ram_wdata <= 8'h20;
            ram_we    <= 1'b0;
        end else begin
            ram_we <= 1'b0;

            case (state)
                S_IDLE: begin
                    if (tx_stb) begin
                        ch <= tx_data;
                        case (tx_data)
                            8'h0D: begin           // CR
                                curx <= 7'd0;
                            end
                            8'h0A: begin           // LF
                                curx <= 7'd0;
                                if (cury == 6'd39) begin
                                    idx   <= 12'd0;
                                    busy  <= 1'b1;
                                    state <= S_SCROLL_RD;
                                end else begin
                                    cury <= cury + 6'd1;
                                end
                            end
                            8'h08: begin           // BS
                                if (curx != 7'd0)
                                    curx <= curx - 7'd1;
                                else if (cury != 6'd0) begin
                                    cury <= cury - 6'd1;
                                    curx <= 7'd79;
                                end
                            end
                            8'h09: begin           // HT
                                if (tabx >= 7'd80) begin
                                    curx <= 7'd0;
                                    if (cury == 6'd39) begin
                                        idx   <= 12'd0;
                                        busy  <= 1'b1;
                                        state <= S_SCROLL_RD;
                                    end else begin
                                        cury <= cury + 6'd1;
                                    end
                                end else begin
                                    curx <= tabx;
                                end
                            end
                            8'h0C: begin           // FF: clear screen
                                idx   <= 12'd0;
                                busy  <= 1'b1;
                                state <= S_CLEAR;
                            end
                            default: begin
                                if (tx_data >= 8'h20 && tx_data != 8'h7F) begin
                                    ram_addr  <= cury * 6'd80 + curx;
                                    ram_wdata <= tx_data;
                                    ram_we    <= 1'b1;
                                    busy      <= 1'b1;
                                    state     <= S_ADV;
                                end
                            end
                        endcase
                    end
                end

                S_ADV: begin
                    if (curx == 7'd79) begin
                        curx <= 7'd0;
                        if (cury == 6'd39) begin
                            idx   <= 12'd0;
                            state <= S_SCROLL_RD;
                        end else begin
                            cury <= cury + 6'd1;
                            busy <= 1'b0;
                            state <= S_IDLE;
                        end
                    end else begin
                        curx <= curx + 7'd1;
                        busy <= 1'b0;
                        state <= S_IDLE;
                    end
                end

                // move rows 1..39 up one row, then blank the last row
                S_SCROLL_RD: begin
                    ram_addr <= idx + 12'd80;
                    state    <= S_SCROLL_WR;
                end
                S_SCROLL_WR: begin
                    state <= S_SCROLL_WD;
                end
                S_SCROLL_WD: begin
                    ram_wdata <= ram_rdata;
                    ram_addr  <= idx;
                    ram_we    <= 1'b1;
                    state     <= S_SCROLL_NX;
                end
                S_SCROLL_NX: begin
                    if (idx == 12'd3119) begin
                        idx   <= 12'd3120;
                        state <= S_SCROLL_CLEAR;
                    end else begin
                        idx   <= idx + 12'd1;
                        state <= S_SCROLL_RD;
                    end
                end
                S_SCROLL_CLEAR: begin
                    ram_addr  <= idx;
                    ram_wdata <= 8'h20;
                    ram_we    <= 1'b1;
                    if (idx == 12'd3199) begin
                        curx  <= 7'd0;
                        cury  <= 6'd39;
                        busy  <= 1'b0;
                        state <= S_IDLE;
                    end else begin
                        idx <= idx + 12'd1;
                    end
                end
                S_CLEAR: begin
                    ram_addr  <= idx;
                    ram_wdata <= 8'h20;
                    ram_we    <= 1'b1;
                    if (idx == 12'd3199) begin
                        curx  <= 7'd0;
                        cury  <= 6'd0;
                        busy  <= 1'b0;
                        state <= S_IDLE;
                    end else begin
                        idx <= idx + 12'd1;
                    end
                end
                default: state <= S_IDLE;
            endcase
        end
    end

    // The write port sees ram_addr/ram_wdata one cycle after the FSM sets
    // them; S_SCROLL_WD waits an extra cycle for q_b, so the copies land
    // in the right order.

    // ------------------------------------------------------------------
    // PS/2 keyboard -> UART1 receive
    // ------------------------------------------------------------------
    wire [6:0] ascii_code;
    wire       ascii_new;

    ps2_keyboard_to_ascii #(
        .clk_freq(50000000),
        .ps2_debounce_counter_size(8)
    ) ps2 (
        .clk(clk50),
        .ps2_clk(ps2_clk),
        .ps2_data(ps2_data),
        .ascii_new(ascii_new),
        .ascii_code(ascii_code)
    );

    reg        rx_pend;
    reg  [7:0] rx_code;

    always @(posedge clk50 or negedge reset_n) begin
        if (!reset_n) begin
            rx_pend <= 1'b0;
            rx_code <= 8'h00;
        end else begin
            if (ascii_new) begin
                rx_code <= {1'b0, ascii_code};
                rx_pend <= 1'b1;
            end
            if (rx_pend && rx_ack)
                rx_pend <= 1'b0;
        end
    end

    assign rx_data  = rx_code;
    assign rx_valid = rx_pend;

endmodule

`default_nettype wire
