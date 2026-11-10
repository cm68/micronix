// mpz80_top.v - S100Computers FPGA Z80 SBC as a Morrow MPZ80 (bus master).
//
// The board pin names are the stock S100Computers names with hyphens
// replaced by _n.  The design targets the V1.6 board: EXT_A20-A23 are
// E6/F10/B14/E11.  The JEDEC socket is the monitor/boot-selector ROM
// (the 512K SRAM is not fitted).  UART1 drives the VGA/PS/2 console,
// UART2 the USB serial, and the uPD1990 is backed by the DS1305.

`default_nettype none

module mpz80_top (
    input  wire        CLK_50,
    input  wire        FPGA_IN_BOARD_RESET_n,

    // front-panel switches (DIP0..DIP7)
    input  wire [7:0]  DIP,

    // S-100 data
    input  wire [7:0]  FPGA_IN_DI,
    output wire [7:0]  FPGA_OUT_DO,
    output wire        FPGA_OUT_DO_OE_n,
    output wire        FPGA_OUT_DI_OE_n,

    // JEDEC socket ROM on the RAM bus
    inout  wire [7:0]  FPGA_BI_D,
    output wire        FPGA_OUT_RAM_A16,
    output wire        FPGA_OUT_RAM_A17,
    output wire        FPGA_OUT_RAM_A18,
    output wire        FPGA_OUT_RAM_OE_n,
    output wire        FPGA_OUT_RAM_WR_n,
    output wire        FPGA_OUT_RAM_CS_n,

    // Micronix MYIDE: 82C55 + CF at 0x30-0x34
    output wire        FPGA_OUT_8255_SEL_n,
    output wire        FPGA_OUT_IDE_WR_n,
    output wire        FPGA_OUT_IDE_RD_n,
    output wire        FPGA_OUT_5V_IDE_WR_n,
    output wire        FPGA_OUT_5V_IDE_RD_n,
    input  wire        IDE_INTRQ,

    // DS1305 RTC
    output wire        RTC_CS_n,
    output wire        RTC_SPI_CLK,
    output wire        RTC_SPI_SI,
    input  wire        RTC_SPI_SO,
    input  wire        RTC_INT,

    // S-100 address (A0-A19 + V1.6 EXT_A20-A23)
    output wire [19:0] FPGA_OUT_A,
    output wire [3:0]  FPGA_OUT_A_EXT,

    // S-100 control
    output wire        FPGA_OUT_pSYNC,
    output wire        FPGA_OUT_pSTVAL_n,
    output wire        FPGA_OUT_pDBIN,
    output wire        FPGA_OUT_pWR_n,
    output wire        FPGA_OUT_sMEMR,
    output wire        FPGA_OUT_sWO_n,
    output wire        FPGA_OUT_sINP,
    output wire        FPGA_OUT_sOUT,
    output wire        FPGA_OUT_sM1,
    output wire        FPGA_OUT_sINTA,
    output wire        FPGA_OUT_sHLTA,
    output wire        FPGA_OUT_MWRT,

    // S-100 status / DMA
    input  wire        FPGA_IN_XRDY,
    input  wire        FPGA_IN_RDY,
    input  wire        FPGA_IN_pHOLD_n,
    output wire        FPGA_OUT_pHLDA,
    input  wire        FPGA_IN_INT_n,
    input  wire        FPGA_IN_INT_A_n,
    input  wire        FPGA_IN_INT_B_n,
    input  wire        FPGA_IN_INT_C_n,
    input  wire        FPGA_IN_INT_D_n,

    // USB serial (UART2 backend)
    input  wire        USB_RX,
    output wire        USB_TX,

    // VGA console (UART1 backend)
    output wire        VGA_R,
    output wire        VGA_G,
    output wire        VGA_B,
    output wire        HSync,
    output wire        VSync,

    // PS/2 keyboard (UART1 receive)
    input  wire        FPGA_IN_PS2_DATA,
    input  wire        FPGA_IN_PS2_CLK,

    // clocks and buffer enables
    output wire        FPGA_OUT_PHI,
    output wire        FPGA_OUT_2mHz_CLOCK,
    output wire        FPGA_OUT_CPU_CLK,
    output wire        FPGA_ADD_OE_n,
    output wire        FPGA_OUT_CTL_OE_n,
    output wire        FPGA_OUT_CTL_DISABLE,
    output wire        FPGA_OUT_STATUS_OE_n,
    output wire        FPGA_OUT_STATUS_DISABLE,

    // debug LEDs
    output wire        LED_1,
    output wire        LED_2,
    output wire        LED_3
);

    // ---------------------------------------------------------------
    // clocks: 50 MHz -> 4.000 MHz CPU, 2.000 MHz S-100 CLOCK
    //   S-100 pin 24 (PHI) is the master timing signal for the bus: it
    //   is the CPU clock and must track the CPU.
    //   S-100 pin 49 (CLOCK) is a fixed 2 MHz timebase with no
    //   relationship to any other bus signal (IEEE-696 2.2.9.3).
    // ---------------------------------------------------------------
    wire clk4, clk2, c2, c3;
    PLL01_50 pll (
        .inclk0(CLK_50),
        .c0(clk4),
        .c1(clk2),
        .c2(c2),
        .c3(c3));

    // Hold the CPU in reset for a few 4 MHz clocks after board reset.
    reg [3:0] rstcnt;
    always @(posedge clk4) begin
        if (!FPGA_IN_BOARD_RESET_n)
            rstcnt <= 4'hF;
        else if (rstcnt != 4'h0)
            rstcnt <= rstcnt - 4'd1;
    end
    wire reset_n = (rstcnt == 4'h0);

    // ---------------------------------------------------------------
    // CPU and MPZ80 sequencer
    // ---------------------------------------------------------------
    wire [15:0] cpu_a;
    wire [7:0]  seq_di, cpu_di, cpu_do, seq_do;
    wire m1_n, mreq_n, iorq_n, rd_n, wr_n, rfsh_n, halt_n, busak_n;
    wire cpu_wait_n;
    wire t80_int_n, t80_nmi_n;

    wire [9:0]  ram_addr;
    wire        ram_we;
    wire [7:0]  ram_dout, ram_q;
    wire [10:0] rom_addr;
    wire        ext_sel;
    wire [18:0] jedec_addr;
    wire        dma = (busak_n == 1'b0);
    wire        mio_sel;
    wire [7:0]  mio_q;
    wire        int_n;

    wire [23:0] phys_addr;
    wire local_sel, trap_win;
    wire bus_enable, bus_read, bus_write, bus_iorq, bus_m1, bus_inta;
    wire [7:0]  bus_do;

    // The JEDEC part is the monitor ROM at the 2716 window (offset 0)
    // and the boot selector at 0xFF0000 (offset 0x800).  A local ROM
    // or trap read selects it; a mapped read in the selector's physical
    // page does too, and suppresses the S-100 cycle.
    wire local_rom = local_sel &&
                     ((cpu_a[11:9] == 3'd4) || (cpu_a[11:9] == 3'd5));
    wire jedec_sel = (local_rom || trap_win || ext_sel) && !dma;
    assign ext_sel = (phys_addr[23:16] == 8'hFF) && !local_sel && !trap_win;
    assign jedec_addr = ext_sel
        ? (19'h00800 + {3'b000, phys_addr[15:0]})
        : {8'h00, rom_addr[10:0]};

    t80_wrap cpu (
        .reset_n(reset_n),
        .clk(clk4),
        .wait_n(cpu_wait_n),
        .int_n(t80_int_n),
        .nmi_n(t80_nmi_n),
        .busrq_n(FPGA_IN_pHOLD_n),
        .m1_n(m1_n),
        .mreq_n(mreq_n),
        .iorq_n(iorq_n),
        .rd_n(rd_n),
        .wr_n(wr_n),
        .rfsh_n(rfsh_n),
        .halt_n(halt_n),
        .busak_n(busak_n),
        .a(cpu_a),
        .di(cpu_di),
        .do(cpu_do));

    mpz80_seq seq (
        .clk(clk4),
        .reset_n(reset_n),
        .a(cpu_a),
        .cpu_do(seq_do),
        .m1_n(m1_n),
        .mreq_n(mreq_n),
        .iorq_n(iorq_n),
        .rd_n(rd_n),
        .wr_n(wr_n),
        .rfsh_n(rfsh_n),
        .cpu_di(seq_di),
        .ram_q(ram_q),
        .ram_addr(ram_addr),
        .ram_we(ram_we),
        .ram_dout(ram_dout),
        .rom_q(FPGA_BI_D),
        .rom_addr(rom_addr),
        .ext_sel(ext_sel),
        .ext_q(FPGA_BI_D),
        .mio_sel(mio_sel),
        .mio_q(mio_q),
        .s100_di(FPGA_IN_DI),
        .phys_addr(phys_addr),
        .local_sel(local_sel),
        .trap_win(trap_win),
        .bus_enable(bus_enable),
        .bus_read(bus_read),
        .bus_write(bus_write),
        .bus_iorq(bus_iorq),
        .bus_m1(bus_m1),
        .bus_inta(bus_inta),
        .bus_do(bus_do),
        .int_n(int_n),        .switches(DIP),
        .t80_int_n(t80_int_n),
        .t80_nmi_n(t80_nmi_n));

    ram1k u_ram (
        .address(ram_addr),
        .clock(clk4),
        .data(ram_dout),
        .wren(ram_we),
        .q(ram_q));

    // JEDEC socket ROM on the shared on-board data bus.  The FPGA reads
    // the ROM and drives the same bus for the 82C55 writes.
    assign FPGA_OUT_RAM_A16  = jedec_addr[16];
    assign FPGA_OUT_RAM_A17  = jedec_addr[17];
    assign FPGA_OUT_RAM_A18  = jedec_addr[18];
    assign FPGA_OUT_RAM_OE_n = ~jedec_sel;
    assign FPGA_OUT_RAM_WR_n = 1'b1;
    assign FPGA_OUT_RAM_CS_n = ~jedec_sel;

    // ---------------------------------------------------------------
    // Wunderbus/MultIO: 8259 pair, UART1/2 and the uPD1990
    // ---------------------------------------------------------------
    wire [3:0] mio_vi = {~FPGA_IN_INT_D_n, ~FPGA_IN_INT_C_n,
                         ~FPGA_IN_INT_B_n, ~FPGA_IN_INT_A_n};
    // {VI4, VI3, VI1, VI0}: the second MultIO owns VI3/VI4, the first
    // owns VI0/VI1, and VI2 is the internal cascade, not a bus line.
    // A/B/C/D on the carrier are treated as VI0/VI1/VI3/VI4.
    wire mio_inta = (m1_n == 1'b0) && (iorq_n == 1'b0) && (rfsh_n == 1'b1);
    wire mio_int_req, mio_pic_sel;
    wire [7:0] mio_pic_dout, mio_inta_byte;
    wire [1:0] mio_group;

    wire uart1_cs = (iorq_n == 1'b0) && (rfsh_n == 1'b1) &&
                    (mio_group == 2'd1) && (cpu_a[7:3] == 5'b01001) &&
                    (cpu_a[2:0] != 3'd7);
    wire uart2_cs = (iorq_n == 1'b0) && (rfsh_n == 1'b1) &&
                    (mio_group == 2'd2) && (cpu_a[7:3] == 5'b01001) &&
                    (cpu_a[2:0] != 3'd7);
    wire upd_cs   = (iorq_n == 1'b0) && (rfsh_n == 1'b1) &&
                    (mio_group == 2'd0) && (cpu_a[7:0] == 8'h4A);
    // Group 0's second register: the Wunderbus puts its DIP switch at
    // 0x49, a MultIO does not decode it and reads open bus.  pic_init.c
    // reads it to decide whether the slave can CAS-vector.
    wire mio_sw_cs = (iorq_n == 1'b0) && (rfsh_n == 1'b1) &&
                     (mio_group == 2'd0) && (cpu_a[7:0] == 8'h49);

    wire [7:0] uart1_dout, uart2_dout, upd_dout;
    wire uart1_int, uart2_int, upd_int;
    wire [7:0] uart1_tx_data, uart1_rx_data;
    wire uart1_tx_stb, uart1_tx_busy, uart1_rx_valid, uart1_rx_ack;
    wire [7:0] uart2_tx_data, uart2_rx_data;
    wire uart2_tx_stb, uart2_tx_busy, uart2_rx_valid, uart2_rx_ack;

    i8250 u_uart1 (
        .clk(clk4), .reset_n(reset_n),
        .cs(uart1_cs), .a(cpu_a[2:0]), .rd_n(rd_n), .wr_n(wr_n),
        .din(cpu_do), .dout(uart1_dout),
        .tx_data(uart1_tx_data), .tx_stb(uart1_tx_stb), .tx_busy(uart1_tx_busy),
        .rx_data(uart1_rx_data), .rx_valid(uart1_rx_valid), .rx_ack(uart1_rx_ack),
        .int_req(uart1_int));

    vga_console u_console (
        .clk50(CLK_50), .clk25(c3), .reset_n(reset_n),
        .tx_data(uart1_tx_data), .tx_stb(uart1_tx_stb), .tx_busy(uart1_tx_busy),
        .rx_data(uart1_rx_data), .rx_valid(uart1_rx_valid), .rx_ack(uart1_rx_ack),
        .vga_r(VGA_R), .vga_g(VGA_G), .vga_b(VGA_B),
        .vga_hsync(HSync), .vga_vsync(VSync),
        .ps2_clk(FPGA_IN_PS2_CLK), .ps2_data(FPGA_IN_PS2_DATA));

    i8250 u_uart2 (
        .clk(clk4), .reset_n(reset_n),
        .cs(uart2_cs), .a(cpu_a[2:0]), .rd_n(rd_n), .wr_n(wr_n),
        .din(cpu_do), .dout(uart2_dout),
        .tx_data(uart2_tx_data), .tx_stb(uart2_tx_stb), .tx_busy(uart2_tx_busy),
        .rx_data(uart2_rx_data), .rx_valid(uart2_rx_valid), .rx_ack(uart2_rx_ack),
        .int_req(uart2_int));

    uart #(.CLOCK_DIVIDE(1302)) u_usb (
        .clk(CLK_50), .rst(~reset_n),
        .rx(USB_RX), .tx(USB_TX),
        .transmit(uart2_tx_stb), .tx_byte(uart2_tx_data),
        .received(), .rx_byte(uart2_rx_data),
        .is_receiving(), .is_transmitting(uart2_tx_busy),
        .recv_error(), .data_ready(uart2_rx_valid),
        .data_read(uart2_rx_ack));

    wire [7:0] cal_sec, cal_min, cal_hour, cal_day;
    wire [3:0] cal_month;
    wire [2:0] cal_wday;
    wire [7:0] wr_sec, wr_min, wr_hour, wr_day;
    wire [3:0] wr_month;
    wire [2:0] wr_wday;
    wire       cal_set;

    upd1990 u_clock (
        .clk(clk4), .reset_n(reset_n),
        .cs(upd_cs), .rd_n(rd_n), .wr_n(wr_n), .din(cpu_do), .dout(upd_dout),
        .int_req(upd_int),
        .cal_sec(cal_sec), .cal_min(cal_min), .cal_hour(cal_hour), .cal_day(cal_day),
        .cal_month(cal_month), .cal_wday(cal_wday),
        .cal_set(cal_set),
        .wr_sec(wr_sec), .wr_min(wr_min), .wr_hour(wr_hour), .wr_day(wr_day),
        .wr_month(wr_month), .wr_wday(wr_wday));

    wire spi_enable, spi_busy;
    wire [15:0] spi_tx, spi_rx;

    ds1305 u_rtc (
        .clk(clk4), .reset_n(reset_n),
        .spi_enable(spi_enable), .spi_tx(spi_tx),
        .spi_busy(spi_busy), .spi_rx(spi_rx),
        .cal_sec(cal_sec), .cal_min(cal_min), .cal_hour(cal_hour), .cal_day(cal_day),
        .cal_month(cal_month), .cal_wday(cal_wday),
        .wr_sec(wr_sec), .wr_min(wr_min), .wr_hour(wr_hour), .wr_day(wr_day),
        .wr_month(wr_month), .wr_wday(wr_wday), .wr_stb(cal_set));

    spi_ds1305 u_spi (
        .clk(clk4), .reset_n(reset_n),
        .enable(spi_enable), .tx_data(spi_tx), .miso(RTC_SPI_SO),
        .sclk(RTC_SPI_CLK), .cs_n(RTC_CS_n), .mosi(RTC_SPI_SI),
        .busy(spi_busy), .rx_data(spi_rx));

    wire [11:0] onboard = {5'b00000, IDE_INTRQ, 1'b0, 1'b0,
                           upd_int, 1'b0, uart2_int, uart1_int};

    wire        im0_read_sel, im0_write_sel, im0_extra;
    wire [7:0]  im0_read_byte, im0_write_byte;
    wire [7:0]  mio_vec_low, mio_vec_high;

    im0_call u_im0 (
        .clk(clk4), .reset_n(reset_n),
        .m1_n(m1_n), .mreq_n(mreq_n), .rd_n(rd_n), .wr_n(wr_n), .a(cpu_a),
        .inta_level(mio_inta), .inta_byte(mio_inta_byte),
        .vec_low(mio_vec_low), .vec_high(mio_vec_high),
        .read_sel(im0_read_sel), .read_byte(im0_read_byte),
        .write_sel(im0_write_sel), .write_byte(im0_write_byte),
        .inta_extra(im0_extra));

    assign cpu_di = im0_read_sel ? im0_read_byte : seq_di;
    assign seq_do = im0_write_sel ? im0_write_byte : cpu_do;

    mio_pic #(.CASCADE(1), .SLAVE_PRESENT(1)) u_pic (
        .clk(clk4), .reset_n(reset_n),
        .addr(cpu_a[7:0]), .iorq(iorq_n == 1'b0),
        .rd_n(rd_n), .wr_n(wr_n),
        .din(cpu_do), .dout(mio_pic_dout), .sel(mio_pic_sel),
        .group_o(mio_group),
        .vi(mio_vi), .onboard(onboard),
        .int_req(mio_int_req), .inta(mio_inta | im0_extra),
        .inta_byte(mio_inta_byte),
        .vec_low_o(mio_vec_low), .vec_high_o(mio_vec_high));

    // ---------------------------------------------------------------
    // Micronix MYIDE: 82C55 + CF at 0x30-0x34
    // ---------------------------------------------------------------
    wire ide_cs = (iorq_n == 1'b0) && (rfsh_n == 1'b1) &&
                  (cpu_a[7:3] == 5'b00110) && (cpu_a[2:0] <= 3'd4);
    wire ppi_cs = ide_cs && (cpu_a[2:0] <= 3'd3);
    wire ppi_wr = ppi_cs && (wr_n == 1'b0);
    wire ppi_rd = ppi_cs && (rd_n == 1'b0);
    reg  drvsel;

    always @(posedge clk4 or negedge reset_n) begin
        if (!reset_n)
            drvsel <= 1'b0;
        else if (ide_cs && (wr_n == 1'b0) && (cpu_a[2:0] == 3'd4))
            drvsel <= cpu_do[0];
    end

    wire [7:0] ide_dout = ppi_rd ? FPGA_BI_D :
                          (ide_cs && (cpu_a[2:0] == 3'd4)) ? {7'b0, drvsel} : 8'hFF;

    assign FPGA_BI_D             = ppi_wr ? cpu_do : 8'hzz;
    assign FPGA_OUT_8255_SEL_n   = ~ppi_cs;
    assign FPGA_OUT_IDE_WR_n     = ~ppi_wr;
    assign FPGA_OUT_IDE_RD_n     = ~ppi_rd;
    assign FPGA_OUT_5V_IDE_WR_n  = ~ppi_wr;
    assign FPGA_OUT_5V_IDE_RD_n  = ~ppi_rd;

    assign mio_sel = mio_pic_sel || uart1_cs || uart2_cs || upd_cs ||
                     mio_sw_cs || ide_cs || mio_inta;
    assign mio_q   = mio_inta    ? mio_inta_byte :
                     mio_pic_sel ? mio_pic_dout :
                     uart1_cs    ? uart1_dout :
                     uart2_cs    ? uart2_dout :
                     upd_cs      ? upd_dout :
                     mio_sw_cs   ? 8'hFF :
                     ide_cs      ? ide_dout : 8'h00;
    assign int_n   = mio_int_req ? 1'b0 : FPGA_IN_INT_n;

    // ---------------------------------------------------------------
    // S-100 bus cycle
    // ---------------------------------------------------------------
    wire ready = FPGA_IN_RDY | FPGA_IN_XRDY;
    reg [3:0] wcnt;
    always @(posedge clk4) begin
        if (!reset_n)
            wcnt <= 4'd0;
        else if (!bus_enable || ready)
            wcnt <= 4'd0;
        else if (wcnt != 4'hF)
            wcnt <= wcnt + 4'd1;
    end
    wire timeout = (wcnt == 4'hF);

    assign cpu_wait_n = (local_sel || trap_win || !bus_enable)
                        ? 1'b1 : (ready || timeout);

    // pSYNC is a one-clock pulse at the start of a bus cycle.
    reg psync_done;
    always @(posedge clk4) begin
        if (!reset_n)
            psync_done <= 1'b0;
        else if (!bus_enable)
            psync_done <= 1'b0;
        else if (!psync_done)
            psync_done <= 1'b1;
    end

    assign FPGA_OUT_pSYNC     = bus_enable && !psync_done && !dma;
    assign FPGA_OUT_pSTVAL_n  = !(bus_enable && !dma);
    assign FPGA_OUT_pDBIN     = bus_read && !dma;
    assign FPGA_OUT_pWR_n     = !(bus_write && !dma);
    assign FPGA_OUT_sMEMR     = bus_enable && !mreq_n && !rd_n && !bus_inta && !dma;
    assign FPGA_OUT_sWO_n     = !(bus_enable && !mreq_n && !wr_n && !dma);
    assign FPGA_OUT_sINP      = bus_enable && !iorq_n && !rd_n && !bus_inta && !dma;
    assign FPGA_OUT_sOUT      = bus_enable && !iorq_n && !wr_n && !dma;
    assign FPGA_OUT_sM1       = bus_m1 && !dma;
    assign FPGA_OUT_sINTA     = bus_inta && !dma;
    assign FPGA_OUT_sHLTA     = !halt_n;
    assign FPGA_OUT_MWRT      = 1'b0;

    assign FPGA_OUT_A         = jedec_sel ? {4'b0000, jedec_addr[15:0]}
                                           : phys_addr[19:0];
    assign FPGA_OUT_A_EXT     = jedec_sel ? 4'h0 : phys_addr[23:20];
    assign FPGA_OUT_DO        = bus_do;
    assign FPGA_OUT_DO_OE_n   = !(bus_write && !dma);
    assign FPGA_OUT_DI_OE_n   = !((bus_read || bus_inta) && !dma);

    assign FPGA_ADD_OE_n        = !(bus_enable && !dma);
    assign FPGA_OUT_CTL_OE_n    = !(bus_enable && !dma);
    assign FPGA_OUT_STATUS_OE_n = !(bus_enable && !dma);
    assign FPGA_OUT_CTL_DISABLE = 1'b0;
    assign FPGA_OUT_STATUS_DISABLE = 1'b0;

    assign FPGA_OUT_pHLDA      = !busak_n;

    // S-100 pin 24: master timing signal, tracks the CPU clock.
    assign FPGA_OUT_PHI        = clk4;
    // S-100 pin 49: constant 2 MHz, no relationship to the CPU clock.
    assign FPGA_OUT_2mHz_CLOCK = clk2;
    assign FPGA_OUT_CPU_CLK    = clk4;

    assign LED_1 = ~reset_n;
    assign LED_2 = local_sel;
    assign LED_3 = trap_win;

endmodule

`default_nettype wire
