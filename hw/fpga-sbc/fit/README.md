# Two 8259s for the wb/mio wirings — fit probe

`pic_fit` instantiates `mio_pic` with `CASCADE=1, SLAVE_PRESENT=1` and
the four-VI + on-board mapping.  It has no pin assignments; the
fitter's own report is the answer.

    quartus_sh --flow compile pic_fit

## Measured

| resource | pic_fit | MPZ80 design | combined | free after |
|---|---:|---:|---:|---:|
| logic elements | 514 | 6,174 | 6,688 (65%) | 3,632 |
| registers | 167 | 3,241 | 3,408 | 6,912 |
| M9K bits | 0 | 8,192 | 8,192 (2%) | 415,744 |
| pins (probe) | 55 auto | 94 | — | 4 VI inputs on integration |

The first-cut integration with the on-board IR inputs tied off
measures **6,410 LEs (62%), 3,332 registers and 98 pins**; the 6,688
is the worst case with all sixteen IR inputs live.  The full MIO cut
with the two 8250s, the USB UART, the uPD1990, its DS1305 calendar
and the on-board MYIDE
is **7,649 LEs (74%)**, 3,901 registers, 111 pins.  The VGA/PS2
console (text and font RAM, the display core and the keyboard
translator) brings the complete image to **8,813 LEs (85%), 4,209
registers, 118 pins and 73,728 memory bits**, leaving 1,507 LEs;
timing closes at 6.767 ns (50 MHz), 11.755 ns (25 MHz pixel clock)
and 211.8 ns (4 MHz).

The 514 LEs are two complete `i8259` instances with **all sixteen IR
inputs live**, plus the 0x4C/0x4D and 0x44/0x45 decode, the 0x4F/0x47
selects, the slave's IR2 cascade fold and the 3-byte INTA mux.  The
integration's
internal I/O mux adds only a little more.

## The kernel's IR map

`sys/pic_init.c` (the kernel's built PIC init) is the authority; it
programs a CAS-cascaded pair with `CASCADE_IR 2` — "the slave yanks on
VI2, the master's IR2" (`src/hwsim/d1/vimap.h`):

| 8259 input | source |
|---|---|
| master IR0–IR1 | S-100 VI0–VI1 |
| master IR2 | the slave's INT (the notional VI2 cascade) |
| master IR3–IR5 | on-board UART1–3 |
| master IR6 | on-board / spare |
| master IR7 | on-board clock (uPD1990 heartbeat) |
| slave IR0–IR1 | S-100 VI3–VI4 |
| slave IR2 | on-board (the dropped VI5 / IDE position) |
| slave IR3–IR7 | second-board functions |

`pic_init.c` reads group 0's port 0x49: a MultIO master reads open bus
(0xFF) and gets the cascade path, a Wunderbus reads its DIP (0x00) and
the slave is polled on IR2.  With the cascade, the master is programmed
`ICW1=0x1d`, `ICW3=1<<2` and the slave gets a vector base and
CAS-vectors; `slave_icw2 != 0` selects that path in `mio_pic`.

## The four VI lines and the on-board positions

The SBC routes four S-100 VI lines: VI0 and VI1 into the first MultIO's
IR0–IR1, VI3 and VI4 into the second's IR0–IR1.  VI2 is not a bus
line — it is the cascade between the two cards — and VI5 is not routed:

| vector | 8259 input | typical use |
|---|---|---|
| `onboard[0]` | master IR3 | UART1 — VGA console |
| `onboard[1]` | master IR4 | UART2 — USB serial |
| `onboard[2]` | master IR5 | UART3 — tied off |
| `onboard[3]` | master IR7 | uPD1990 heartbeat |
| `onboard[4]` | master IR6 | spare |
| `onboard[5]` | — | spare (VI4 is wired) |
| `onboard[6]` | slave IR2 | dropped VI5 / on-board IDE |
| `onboard[11:7]` | slave IR3–IR7 | SD, RTC, spares |

## The UART/clock plan

- **UART1 (master IR3) = VGA console.**  The 8250's TX feeds the VGA
  text terminal; its RX comes from the PS/2 keyboard scanner.  The
  divisor/baud/modem registers are cosmetic.
- **UART2 (master IR4) = USB serial (FT231x).**  The 8250's TX/RX go
  through the board's USB UART; LSR/IIR can follow the real byte flow.
- **UART3 (master IR5) = tied off.**
- **uPD1990 (master IR7) = DS1305 calendar + FPGA heartbeat.**
  `upd1990_init.c` programs `CC_32HZ`; `mio.s` divides it by four to
  the kernel's 8 Hz tick.  The DS1305's 1 Hz output is **not** the
  32 Hz heartbeat; the FPGA's clock divider is.  The DS1305 backs the
  40-bit BCD calendar on `GET`/`SET`, and the FPGA converts its BCD
  month (1–12) to the uPD1990's binary month (0–11).  The uPD1990 has
  no year; the kernel borrows the superblock year, so the DS1305 year
  is only for the FPGA's own calendar.

The SBC's parallel interface is unused.  The JEDEC socket is the
monitor/boot-selector ROM; the 512K SRAM is not fitted.  The JEDEC
part is private to the FPGA and not visible to an S-100 temporary
master, so incoming DMA cannot reach it, but it is the ROM, not main
memory.  The local page and map are block RAM; main memory remains the
S-100 cards.

## Profiles

| profile | parameters | behaviour |
|---|---|---|
| `wb` | `SLAVE_PRESENT=0` | master only; VI0–2 -> IR0–2; on-board IR3–7 |
| `wb,mio` | `SLAVE_PRESENT=1, CASCADE=0` | slave INT -> master IR2; the IR2 stub polls the slave at 0x40 |
| `mio,mio` | `SLAVE_PRESENT=1, CASCADE=1` | the built profile: slave INT -> master IR2, CAS-vectored; VI0/1 on the master, VI3/4 on the slave |

## Status

First-cut model, fit-verified but not simulated or hardware-tested.
The cascade delegate is latched for the 3-byte INTA sequence; the
8259's priority resolver, ICW/OCW state machine and poll path follow
`src/hwsim/d1/pic.c`.  The 8250, uPD1990, DS1305 and VGA/PS2 console
models are integrated; the Wunderbus/MultIO image is complete.
