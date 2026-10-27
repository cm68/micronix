# Z180 CPU board CPLDs

Verilog-2001 RTL for the two **ATF1508AS-10**, 84-pin PLCC CPLDs on the
Z180 MPZ80-compatible S-100 board.  The board contract is
`../z180-cpu.md`; this directory is the first-cut logic, not a fitted
JEDEC/POF image.

## Files

| file | device | contents |
|---|---|---|
| `u1.v` | U1 | sequencer + translation: task register and 8-M1 delay, trap window, trap-address shift, mask/status, register/local decode, permission check, map RAM and flash control |
| `u2.v` | U2 | S-100 bus cycle, wait states, buffer enables, interrupts, DMA grant, S-100 clock divider, switch status |
| `pins_u1.csv` | U1 | 60 logical user-I/O assignments |
| `pins_u2.csv` | U2 | 51 logical user-I/O assignments |
| `Makefile` | - | optional iverilog syntax check |

## Pin budget

| device | user I/O | 84-pin PLCC spare |
|---|---:|---:|
| U1 | 60 | 4 |
| U2 | 51 | 13 |

The package pin numbers are deliberately blank in the CSVs.  The
84-pin PLCC exposes about 64 user I/O; VCC/GND, JTAG (TDI/TMS/TCK/TDO),
the dedicated inputs and the power-down pins are fixed by the part.
Assign the can oscillator to a GCLK pin, and confirm the JTAG/GCLK/VCC
pin numbers against the ATF1508AS datasheet and the fitter's pin list
before layout.

The HDL adds three signals that are not in the original prose pin
table: `INT_REQ` (U2 -> U1), `INT_TO_CPU` (U1 -> U2) and Z180
`/BUSRQ` (U2 output).  `z180-cpu.md` has been updated to the 60/51
budget.

## Toolchain

The ATF1508AS is a 5 V, 128-macrocell CPLD.  Practical Verilog paths:

- **ProChip Designer** with Mentor Precision RTL (Verilog/VHDL) and the
  bundled FIT15xx fitter.  Precision RTL is licensed; this is the
  supported Verilog route.
- Synthesize the Verilog elsewhere to **EDIF 2.0** and run the ATF15xx
  fitter directly, for example `FTT1502 -i file.edf -ifmt EDIF
  -lib aprim.lib -dev p1508c84`.
- Quartus plus POF2JED is an alternate front end for the ATF1508AS.

WinCUPL is free but is CUPL-only; it does not consume these Verilog
files.  The fitter device mnemonic for the 84-pin PLCC is `p1508c84`.

`make` runs iverilog if it is installed.  There is no testbench and no
iverilog in the authoring environment; the RTL has not been simulated,
fitted or timing-checked.

## Implemented behaviour

U1:

- local register decode at 0x400-0x403 (trap address, switch/keyboard
  read, task register, mask/status);
- task low nibble and 8-M1 switch delay; the high nibble is latched but
  not decoded (reserved context);
- 15-read trap window with the external flash address counter;
- trap-address shift register and latched page pair;
- permission check from map RAM B, RI10/grow, deferred memory fault;
- local page/flash-region select, map RAM and A12-A23 driver control;
- user-halt opcode substitution and 0x00 trap-data mux;
- interrupt request gating with the mask.

U2:

- S-100 cycle state machine and wait states;
- buffer enables for address, DI/DO and control;
- pRDY timeout, pWAIT and Z180 /WAIT;
- S-100 control/status outputs, interrupt acknowledge;
- pHOLD/pHLDA to Z180 /BUSRQ//BUSAK;
- 4-pin can divider, /4 /5 /6 /8 strap, two clock-buffer outputs;
- INT PEND / TRAP RESET switch bits.

## Known gaps / bring-up items

- S-100 timing and polarity: `pRDY`, `pWAIT`, `pSYNC`, `pSTVAL`,
  `pDBIN`, `pWR` and the active-low `sMEMR`/`sMEMW`/`sINP`/`sOUT`/
  `sM1`/`sINTA` need scope verification against the actual memory and
  controller boards.  `RDY_ACTIVE_LOW` is a localparam in `u2.v`.
- The user-halt substitution and `/WAIT` stretch are modelled but need
  a real timing check on the S-100 read path.
- The external 74AHCT161/163 flash counter load value and the
  `flash_cnt_load_n/clk/clr` timing must match the counter circuit.
- DMA is treated as an immediate bus grant; mid-cycle grant and the
  Z180 /BUSAK handshake need board testing.
- The U1 `halt_n` input is currently unused; it is reserved for a halt
  fail-safe if the opcode substitution ever misses.
- The U1's SINT polarity follows the emulator/kernel: mask bit 5 high
  blocks supervisor interrupts.  The manual's wording is the reverse;
  `z180-cpu.md` now records the emulator-compatible sense.
- `EMU_FAULT_STATUS=1` reports all user memory faults as 0xf7 for the
  current kernel.  Set it to 0 for the manual's 0x3e/0x7e/0xbe/0xfe
  forms.
- The ATF1508AS fit, macrocell usage and timing are unverified.  U1 is
  the tight device at 60 pins and much of the trap logic.

## External parts referenced by the RTL

The RTL expects the external map RAMs (CY7C199-15PC), the 1M x 8 flash,
the 74AHCT138 local decode, the A23-A20 = F comparator, the flash
address mux/counter, the 74ABT/74AHCT buffers, the MAX232A ASCI
headers and the socketed 4-pin can oscillator described in
`../z180-cpu.md`.
