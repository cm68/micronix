# FPGA SBC MPZ80 build

The bitstream for the S100Computers FPGA Z80 SBC as an S-100 bus-master
Morrow MPZ80.  The contract is `../fpga-sbc.md`; this directory is the
first-cut RTL and the Quartus project.

## Build

The tools are already installed on `nook`:

    export QUARTUS_ROOTDIR=$HOME/intelFPGA_lite/22.1std/quartus
    export PATH=$QUARTUS_ROOTDIR/bin:$PATH
    make

`make` runs `quartus_sh --flow compile fpga-sbc` and leaves
`output_files/fpga-sbc.sof`.  The bitstream does not carry the monitor:
program the JEDEC socket part separately with `mon500.bin` at offset 0
and the boot selector at offset 0x800.

Program a board with:

    make program

That needs a USB Blaster and the udev rule for `09fb`.

## What is here

| path | contents |
|---|---|
| `rtl/mpz80_top.v` | top level: PLL, T80, sequencer, local RAM, JEDEC ROM, S-100 bus |
| `rtl/mpz80_seq.v` | MPZ80 translator, map RAM, trap engine, registers |
| `rtl/im0_call.v` | turns the 8259's 8080-mode CALL into a Z80 IM0 dispatch |
| `rtl/t80_wrap.vhd` | T80s with `Mode=1, T2Write=1, IOWait=0` |
| `rtl/t80/` | Daniel Wallner's T80 core (BSD-style, headers retained) |
| `rtl/ram1k.vhd` | 1 KB local page RAM (altsyncram) |
| `rtl/rom2k.vhd` | unused alternative 2 KB block ROM |
| `rtl/i8259.v` | 8259A-compatible interrupt controller |
| `rtl/mio_pic.v` | master/slave pair, 0x44/0x4C/0x4F decode, VI mapping |
| `rtl/i8250.v` | 8250-compatible UART front end |
| `rtl/upd1990.v` | uPD1990 clock/calendar and 32 Hz heartbeat |
| `rtl/ds1305.v` | DS1305 SPI calendar cache and write-back |
| `rtl/spi_ds1305.vhd` | fixed-generic SPI master wrapper |
| `rtl/spi_16bit_master.vhd` | vendor 16-bit SPI master |
| `rtl/vga_console.v` | UART1 console: text RAM, font ROM, cursor, PS/2 RX |
| `rtl/vga80x40.vhd` | Javier Valcarce 80x40 VGA text core (LGPL) |
| `rtl/ctrm.vhd`, `rtl/losr.vhd` | counters and shift register used by the core |
| `rtl/ps2_keyboard.vhd` | Digi-Key PS/2 keyboard decoder |
| `rtl/ps2_keyboard_to_ascii.vhd` | Digi-Key PS/2-to-ASCII translator |
| `rtl/debounce.vhd` | Digi-Key debounce used by the PS/2 decoder |
| `lat0-12.hex` | 4 KB 8x12 font ROM initializer |
| `text_ram.mif` | power-up contents of the text page (spaces) |
| `rtl/uart.v` | vendor USB UART backend (FT231x) |
| `rtl/pll_4m.v` | 50 MHz -> 4.000 MHz CPU, 2.000 MHz PHI |
| `tools/bin2mif.py` | unused; the monitor goes in the JEDEC part |

## Board assumptions

- **V1.6** board: EXT_A20-A23 on E6/F10/B14/E11.  A V1.1 board does
  not route those pins.
- `JP10` fitted to bypass the on-board interrupt controller; the raw
  S-100 `pINT` goes to `FPGA_IN_INT-`.
- The JEDEC socket is the monitor/boot-selector ROM; the 512 KB SRAM
  is not fitted.  The board is the MPZ80 + two cascaded MultIOs: the
  8259 pair, the UARTs, the uPD1990 clock and the on-board 82C55/CF
  IDE are on the FPGA.  The S-100 bus carries memory and four VI
  sources; more than four needs additional VI wiring.  VGA/PS-2 is
  the console backend (R/G/B = T6/T7/T8, HSync = T9, VSync = T10,
  PS/2 data/clock = B12/B13), and SD is unused.
- The two 8259s are on the FPGA and CAS-cascaded.  The four S-100 VI
  inputs are `FPGA_IN_INT_A-D-` (T3/T4/J13/L12), treated as
  VI0/VI1/VI3/VI4: VI0/VI1 are the first MultIO's IR0/IR1, VI3/VI4 the
  second's IR0/IR1, and VI2 is the internal cascade — not a bus line.
  The on-board UART/clock/IDE
  vectors are master IR3/IR4/IR7 and slave IR2.  The on-FPGA pair owns
  0x44/0x45/0x47/0x4C/0x4D/0x4F (0x47 selects the second board);
  a real MultIO's PIC is not used.  Group
  0's 0x49 probe reads open bus (0xFF), which selects the cascade in
  `pic_init.c`.
- UART1 (master IR3) is the VGA/PS/2 console: an 80x40 text page in a
  dual-port block RAM, the `vga80x40` core at the PLL's 25 MHz output,
  and the PS/2 translator feeding the 8250's receive handshake.  UART2
  (IR4) is the USB serial through the
  vendor UART at 9600 (`USB_RX` M8 / `USB_TX` K8).  UART3 (IR5) is
  tied off.  The uPD1990 (IR7) uses the DS1305 SPI calendar and the
  FPGA 32 Hz heartbeat.
- The on-board 82C55/CF is the Micronix MYIDE at 0x30-0x34.  The CF's
  INTRQ must be wired to `IDE_INTRQ` (pin C9, the repurposed printer
  ACK); it goes to the PIC's slave IR2 (`onboard[6]`), keeping the four
  S-100 VI lines free.  The 82C55/CF share `FPGA_BI_D` with the JEDEC
  ROM; the chip selects are exclusive.
- The front-panel switch register reads the DIPs negated
  (`~DIP`), matching the MPZ80's "switch on reads low" sense.  DIP[7:2]
  are S1-S6; mon500's software bits are 0x04 (multiuser after the
  single-user timeout), 0x08/0x10/0x20/0x40/0x80 (enable the floppy,
  hdcdma/disk3, ide, scsi and hdca/disk2 boots).  DIP[1:0] are S7/S8,
  outside the software field.  If the
  board's DIP wiring is active-high, remove the inversion in
  `mpz80_seq.v`.

## Status

First-cut bitstream, not hardware-verified.  The 8259 pair, two 8250
front ends, the uPD1990/DS1305 clock, the VGA/PS/2 console and the
on-board MYIDE are integrated; the Wunderbus/MultIO image is complete
at 8,813 LEs (85%), 4,209 registers and 118 pins, with 1,507 LEs free.
`rtl/im0_call.v` supplies the 8259's three-byte 8080-mode CALL to the
T80's IM0 entry (operand bytes and the corrected return address); its
phase logic was checked against the T80 in GHDL.  The
bitstream reads the monitor from the JEDEC socket ROM; program the
part with `mon500.bin` at offset 0 and the boot selector at offset
0x800.  The block ROM (`rtl/rom2k.vhd`) is no longer instantiated.
The exact S-100 wait states, `pWAIT`/`pRESET`/NMI routing and the
trap/task timing still need a scope; see `../fpga-sbc.md` section 9.
