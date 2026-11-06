# The S100Computers FPGA Z80 SBC as a Morrow MPZ80

A **bus-master replacement for the Morrow MPZ80 CPU board**, built on
the S100Computers FPGA Z80 SBC (WaveShare CoreEP4CE10, Cyclone IV
EP4CE10F17C8, soft T80 core).  The board plugs into a Morrow Decision
chassis and presents the MPZ80's software-visible hardware exactly:
the 16-task MMU, the map RAM, the trap engine, the local page, the
2 KB monitor ROM and the 24-bit S-100 bus.  The real S-100 cards supply
memory, console, the 8259 and the disk controllers; the FPGA replaces
only the CPU board.

This document is the contract.  It follows the style of
`hw/z180-cpu.md`, which is the *compatible extension*: the Z180 board
moves A20–A23 into the PTE and rewrites the selector boot.  This board
is the original MPZ80 first, with that extension available at compile
time and off by default.

The executable specifications are `src/hwsim/d1/mpz80.c`,
`src/hwsim/d1/mmu_test.c`, and the two consumers that must not change:
`src/micronix/stand/roms/mon500.s` (also mon447 and mon375) and the
kernel in `src/micronix/sys/`.  The original hardware is
`publicdocs/Morrow-MPZ80-CPU-Technical-Manual-Rev1-Apr82.pdf` and
`extra/docs/MPZ8010.ECN.txt`.  Where the manual, the emulator and the
kernel disagree, the emulator and the kernel win; the differences are
recorded in section 9.

## 1. Scope

### 1.1 What is emulated

The MPZ80 CPU board, not the rest of the machine:

- the Z80 and its 64K task address space;
- the 16-task paging MMU and its 512-byte map RAM;
- the trap engine (halt, I/O, memory protection, RI10, interrupt,
  stop, aux, reset);
- the local page: 1 KB RAM, four registers, the map RAM window, the
  2 KB monitor ROM and the optional FPU window;
- the front-panel switch register, trap-address register and keyboard
  input;
- the S-100 bus master: 24-bit address, data in/out, control strobes,
  wait states, interrupt acknowledge, DMA, reset and the 2 MHz bus
  clock.

The S-100 cards are not emulated.  The bus carries the memory cards
and up to four VI interrupt sources; more than four needs additional
VI wiring.  The FPGA is the MPZ80 and the Wunderbus/MultIO: the 8259
pair, the UARTs (VGA and USB), the uPD1990 clock, and the on-board
82C55/CF IDE.  The JEDEC socket holds the monitor/boot-selector ROM
(section 4.4); the 512 KB SRAM is not fitted.

### 1.2 Non-goals

- The board is the MPZ80 plus the Wunderbus/MultIO on-board functions
  (PIC, UARTs, clock, IDE).  The S-100 bus carries memory and up to
  four VI sources; more than four needs extra VI wiring.
- No Z180, no Z280, no internal MMU, no ASCI.
- No kernel change.  If the kernel must change, the emulation is
  wrong.
- No PTE high-nibble banking by default.  That is the extension of
  section 5.

### 1.3 The two consumers

Two programs are the authority for the contract, in this order:

1. **`src/hwsim/d1/mpz80.c`** — the emulator the kernel and the
   regression suite already pass on.  Its register layout, trap
   statuses, task-delay count and permission matrix are the reference.
2. **`src/micronix/stand/roms/mon500.s`** — the firmware.  It is the
   code that actually runs in the reset and trap windows, and its
   comments record the hardware behavior the original board required.

The manual is consulted for anything both leave ambiguous, and section
9 lists the points where this board deliberately follows the emulator
rather than the manual.

## 2. The board

### 2.1 What the S100Computers SBC provides

| item | detail |
|---|---|
| FPGA | Altera Cyclone IV **EP4CE10F17C8**, 10,320 LEs, 423,936 RAM bits, 2 PLLs |
| adaptor | WaveShare CoreEP4CE10; 50 MHz clock, configuration flash, JTAG |
| CPU core | OpenCores **T80s** (`Components/Z80/T80s.vhd`), cycle-accurate, all documented Z80 instructions |
| S-100 | IEEE-696 bus master; A0–A19 in V1.1, **A20–A23 added in V1.6** |
| level shifting | 74LVC245 to the 5 V S-100 edge; 74ABT/74F drivers where required |
| interrupts | on-board 74148/74373/74244 controller; **JP10 bypasses it** |
| power | +8 V S-100 or 8–10 V on P30/P31; 5 V and 3.3 V regulators |
| bus clock | FPGA output PHI; existing design 2 MHz |

The stock design occupies **3,891 / 10,320 LEs (38%)**, **196,608 /
423,936 RAM bits (46%)** and **155 / 180 pins (86%)**.  The translator
and trap engine are small compared with the CPU; the pin budget, not
the logic, is the constraint.  The MPZ80 build keeps the VGA and PS/2
console, the USB serial, the IDE/CF and the RTC, and frees only the SD
and printer pins.

### 2.2 T80 signals the MPZ80 needs

The T80s exposes every signal the trap engine requires.  The mapping is
direct:

| T80s | MPZ80 use |
|---|---|
| `a[15:0]` | virtual address; A11–A0 is the page offset, A15–A12 the page index |
| `di[7:0]` | data from S-100 DI, local RAM/ROM, trap window, or the 0xFF/0x00 substitution muxes |
| `do[7:0]` | data to S-100 DO or the local write devices |
| `m1_n` | page-index shift, halt substitution, task-switch countdown, execute permission |
| `mreq_n`, `rd_n` | memory read: trap-window count, permission check, S-100 `sMEMR` |
| `mreq_n`, `wr_n` | memory write: permission check, S-100 `sMEMW`/`sWO` |
| `iorq_n`, `rd_n` | I/O read: user I/O trap, S-100 `sINP` |
| `iorq_n`, `wr_n` | I/O write: user I/O trap, S-100 `sOUT` |
| `m1_n` + `iorq_n` | interrupt acknowledge: S-100 `sINTA`, 8259 vector on DI |
| `wait_n` | S-100 slave wait (pRDY/XRDY) and local-memory wait |
| `int_n` | gated by the mask (section 3.14) |
| `nmi_n` | S-100 NMI, if wired |
| `busrq_n` / `busak_n` | S-100 pHOLD / pHLDA |
| `rfsh_n` | exclude refresh from the trap-window and permission logic |
| `halt_n` | unused; the MPZ80 substitutes the halt before the CPU sees it |

The stock `Microcomputer.vhd` wrapper already wires these to the bus
core.  The MPZ80 build replaces the wrapper's local memory and I/O
decode; it does not change the CPU core.

### 2.3 Clock

The MPZ80's Z80 runs at **4.000 MHz** and drives a **2.000 MHz** S-100
PHI.  The stock board runs the core at 2 or 25 MHz from a 50 MHz PLL.
For this build configure the PLL for 4.000 MHz (50 × 2/25) for the
T80, and divide by two for PHI.  The S-100 bus is asynchronous and
wait-stated; the 4 MHz CPU clock is the original, so the monitor's
timing loops (disk step rates, the 8259 initialization delay, the
console) behave as they did.

Do not run the core at 25 MHz in a chassis.  The MPZ80's disk
drivers and the monitor's delay loops are calibrated for 4 MHz; a
faster CPU changes the step timing and the trap-window budget even
though the bus protocol still works.

## 3. The contract

### 3.1 Address space

A task is 64 KB, divided into sixteen 4 KB pages.  The virtual address
is `{page[3:0], offset[11:0]}`.  There are sixteen hardware tasks.

The physical address is 24 bits:

```
A[11:0]  = CPU A[11:0]
A[19:12] = map RAM segment byte for (task, page)
A[23:20] = task register bits 7-4          (exact MPZ80)
```

The task register's high nibble is the **bank** for every page of the
task, not a per-page field.  The manual is explicit: "The onboard task
register drives the upper four extended address lines, A20–A23."  The
emulator agrees:

```c
*paddrp = ((taskreg & 0xf0) << 16) | (maps[pte] << 12) + (addr & 0xfff);
```

The bank is why the original selector boot can reach physical
`0xFF0000` by writing `0xF0` to the task register.  A later build may
move the bank into the PTE instead (section 5); this build keeps the
original.

The physical space is therefore 16 MB.  The S-100 edge carries
A0–A23; A20–A23 are outputs of the task register, and V1.6 of the SBC
routes them to the FPGA (`EXT_A20` E6, `EXT_A21` F10, `EXT_A22` B14,
`EXT_A23` E11).  V1.1 does not, so V1.6 is the minimum board.

### 3.2 The map RAM

The map is 16 tasks × 16 pages × 2 bytes = **512 bytes**, visible in
task 0's local page at `0x600–0x7FF`, **write-only**.  A read returns
open bus; software must not rely on it.

Entry address:

```
0x600 + (task << 5) + (page << 1) + byte
```

| byte | bits | meaning |
|---|---|---|
| even | 7–0 | segment, A19–A12 |
| odd | 7–4 | unused on the exact MPZ80; the extension's bank (section 5) |
| odd | 2 | RI10 / grow |
| odd | 1 | execute enable |
| odd | 0 | read enable |

The permission matrix is the manual's Table 1-1:

| attr[1:0] | read | write | execute |
|---|---|---|---|
| `00` | no | no | no |
| `01` | yes | no | yes |
| `10` | no | no | yes |
| `11` | yes | yes | yes |

The emulator tests execute as `attr & 0x2` and a read as `attr & 0x1`;
a write requires `(attr & 0x3) == 3`.  So `01` is read-only but
executable, and `10` is execute-only.  The kernel uses `NONE 0`,
`FULL 3` and `GROW 7`; the other rows matter to the monitor and to
CP/M-era software.

Writes to the map are ordinary local writes to `0x600–0x7FF`; the
low nine address bits are the offset into the 512-byte map.  The
translator reads the two bytes for the current `{task, page}` on every
mapped access.  A dual-port block RAM is the natural implementation:
port A read for translation, port B write for the CPU.

### 3.3 The task register

`0x402` write:

```
bits 7-4   bank, A23-A20 for every page of the task
bits 3-0   task number, 0-15
```

`0x402` read is the **switch register**, not the task register.

A write to `0x402` does all of the following:

1. latches the new value as `next_taskreg`;
2. starts the **8-M1 task-switch countdown** (section 3.10);
3. selects the **upper** 1 KB monitor-ROM half.

The old task's map remains in force until the countdown ends; the
countdown's final M1 is translated with the new map.  The emulator
does the switch before translating that M1; this board does the same.

Task 0 is the supervisor.  Tasks 1–15 are users.  Task 0 is not
subject to the user halt/I/O/interrupt/memory traps.  A task-0 halt
goes to the monitor/reboot path.

### 3.4 The local page

The local page is **task 0 with A15–A12 = 0**, i.e. task 0 addressing
below `0x1000`.  It is always local, regardless of what task 0's page
0 segment byte says; the manual says the devices are "permanently
memory-mapped into task 0's memory space starting at 0 (4K bytes
total)".  A local access generates no S-100 cycle: the address, data
and control drivers are off.

Decode is by A11–A9, with A1–A0 selecting the register:

| address | read | write |
|---|---|---|
| `0x000–0x3FF` | 1 KB local RAM | local RAM |
| `0x400–0x5FF` | registers, aliased every 4 bytes | registers |
| `0x600–0x7FF` | open bus | map RAM |
| `0x800–0xBFF` | monitor ROM, selected 1 KB half | ignored |
| `0xC00–0xFFF` | optional FPU or second 1 KB RAM | same |

The register block:

| address | read | write |
|---|---|---|
| `0x400` | trap-address register | front-panel segment (ignored if no panel) |
| `0x401` | keyboard connector | front-panel column (ignored) |
| `0x402` | switch register | **task register** |
| `0x403` | trap status | mask register |

The emulator decodes only A1–A0 inside the register block, so
`0x400–0x403` alias every four bytes through `0x5FF`.  The monitor uses
the four canonical addresses; reproducing the aliases costs nothing
and is exact.

The FPU is a 9511/9512 data/status/command interface in the emulator
(`0xC00`/`0xC08`).  The stock FPGA SBC has no FPU.  Decode the window
as open bus (`0xFF`) and leave the optional second 1 KB RAM as a
compile-time alternative; the kernel does not use the FPU.

### 3.5 The monitor ROM

The monitor is a **2 KB image** made of two 1 KB halves, both assembled
for `0x800–0xBFF`:

| half | selected | contents |
|---|---|---|
| lower | after reset, until a task-register write | reset sequence at `0xBF0`, monitor entry at `0x800` |
| upper | after any `0x402` write | trap sequence at `0xBF0`, trap entry at `0x800`/`0x821` |

The reset half's 15-byte window at `0xBF0` in mon500 is:

```
3e 00        ld a,0
32 02 06     ld (0602h),a     ; task 0 page 1 segment 0
3c           inc a
31 00 02     ld sp,0200h
c3 00 08     jp 0800h
00 00 00     padding
```

The trap half's 15-byte window is:

```
00           nop             ; voids a halt
ed 73 0a 00  ld (000ah),sp   ; u.sp
31 14 00     ld sp,0014h     ; beghome
f5           push af
e5           push hl
d5           push de
00           nop
cd 21 08     call 0821h      ; trappd
76           halt            ; task-0 guard, outside the window
```

The CPU's PC is not changed by a trap.  The window supplies the next
15 memory reads from the selected half at `0xBF0 + (15 - count)`,
so the linear sequence executes at the trapping PC.  The `call trappd`
at `0xBFC` pushes `trapaddr + 15`; `trappd` subtracts 15 to recover
the trapping address (plus one for a halt, section 3.9).

The board stores the 2 KB image in the JEDEC socket ROM (section
4.4) at offset 0 and selects the half by the `rom_half` latch.  The
same part holds the boot selector at offset `0x800`; the FPGA maps it
at physical `0xFF0000` and suppresses the S-100 cycle.  The part is
programmed externally (or by a future in-system flash loader), so the
bitstream no longer carries the monitor image.

### 3.6 Status and mask

`0x403` read is the trap status.  Bits are active low except RI10
(bit 6), which is high when the trap was caused by an RI10 segment;
bit 7 is low for a read and high for a write:

| bit | name | meaning |
|---|---|---|
| 0 | VOID | illegal memory or I/O reference |
| 1 | IORQ | illegal I/O reference |
| 2 | HALT | halt attempted where halts are disabled |
| 3 | INT | interrupt occurred in a task that may not take interrupts |
| 4 | STOP | front-panel stop |
| 5 | AUX | auxiliary trap |
| 6 | RI10 | trap caused by an RI10 segment |
| 7 | RD STB | low for a read, high for a write |

`0x403` write is the mask:

| bit | name | sense |
|---|---|---|
| 0 | STOP disable | low enables the stop trap |
| 1 | AUX enable | spare enable |
| 2 | TINT disable | high forces user interrupts to trap to task 0 |
| 3 | RUN enable | high allows a task to run; low single-steps |
| 4 | HALT disable | high makes a user halt trap |
| 5 | SINT disable | high blocks supervisor interrupts (emulator/kernel sense) |
| 6 | I/O disable | high makes user I/O trap |
| 7 | ZIO mode | high echoes the port address on A8–A15 |

The kernel's masks are `KERMASK = 0230` (`0x98`: ZIO, halt trap, run)
and `USRMASK = 0135` (`0x5D`: I/O trap, halt trap, run, interrupt
trap, stop trap).  The monitor uses `t1mask = t0mask = 0x2B` and
`ssmode = 0x36`.

The status values the firmware and kernel dispatch on:

| event | status |
|---|---|
| reset | `0xBF` |
| halt | `0xBB` |
| IN | `0x3C` (RI10 `0x7C`) |
| OUT | `0xBC` (RI10 `0xFC`) |
| memory read, no access | `0x3E` (RI10 `0x7E`) |
| memory write, no access | `0xBE` (RI10 `0xFE`) |
| interrupt | `0xB7` (RI10 `0xF7`) |
| stop, read / write | `0x2F` / `0xAF` |
| aux, read / write | `0x1F` / `0x9F` |
| user memory fault, emulator-compatible | `0xF7` |

The kernel's `trap()` distinguishes the cause by which bit is
**missing**: `0xBB` clears HALT, `0xB7` clears INT.  The last row is
deliberate: `mpz80.c` and `mmu_test.c` report every user memory
protection fault as `0xF7` (RI10 set) so the kernel dispatches it to
`fault()`.  The manual's `0x3E`/`0x7E`/`0xBE`/`0xFE` forms are a
compile-time alternative for a future kernel.

### 3.7 Trap sources

| source | condition |
|---|---|
| reset | S-100 pRESET/pPOC or the board reset button |
| stop | front-panel stop and mask bit 0 low |
| aux | auxiliary input and mask bit 1 high |
| halt | user M1 fetch of `0x76` and mask bit 4 high |
| I/O | user IN/OUT and mask bit 6 high |
| interrupt | S-100 interrupt in a user task and mask bit 2 high |
| protection | user reference to a segment whose permissions fail |
| RI10 | user reference to a segment with PTE[2] set |

A task-0 halt is a monitor/reboot trap, not a user halt.  A task-0
memory access bypasses the permission check.

### 3.8 The trap event

A trap does the following, in the order the hardware sees them:

1. latch the cause in the trap status register;
2. latch the trap-address pair for a memory fault (section 3.9);
3. clear the task register to 0, selecting task 0's map;
4. select the upper 1 KB monitor-ROM half (reset uses the lower);
5. load the trap window counter (15); the firmware recovers the
   trapping address from the return address the window's `call`
   pushes;
6. inhibit further traps and interrupts during the window;
7. force the next reads in the window from the ROM.

The CPU's PC is not changed.  The first forced read is the first
memory read after the trap (for a halt, the first read after the
substituted halt; for reset, the first read at address 0).  The window
supplies the 15 bytes from `0xBF0 + (15 - count)`.  The 16th read (the
call/jump target) is normal.

### 3.9 The 15-read window

The window is **15 memory reads**, not 15 M1 cycles: operand fetches
count too.  The hardware counts reads and ignores the CPU's address;
it supplies ROM bytes `0xBF0 + (15 - count)`.  The sequence is fetched
linearly from the trapping PC, so no address comparator is needed.
The emulator instead replaces a read when its address is in
`[trapaddr, trapaddr + 15]`; the two models differ only in the reset
case (section 3.15) and produce the same register state.

The window address is:

```
rom_address = 0xBF0 + (15 - count)
```

and the data mux feeds the ROM byte to the T80 while the S-100 bus
drivers stay off.

Trap-address rules:

| cause | `trapaddr` |
|---|---|
| reset | 0 (the window starts on the first read) |
| user halt | halt address + 1 |
| interrupt | address of the next instruction (no +1) |
| memory fault | the instruction boundary after the faulting instruction |

The halt's `+1` accounts for the halt's own M1 fetch: the halt is
voided (returns `0x00`), the window starts at the instruction after
it, and `trappd` subtracts 15 from the pushed return address to
recover the address after the halt.  An interrupt is already between
instructions, so adding one would land inside the next instruction.
A deferred memory fault traps at the PC after the instruction; the
kernel's grow-on-reference model depends on it.

The window is frozen against a second trap: `trapcount` is nonzero
across the 15 reads, and the trap/interrupt logic ignores new events
until it reaches zero.

### 3.10 The task-switch delay

Writing `0x402` starts an **8-M1 countdown**.  Traps and interrupts
are inhibited while it runs; the old task's map remains in force until
the countdown's final M1, which is translated with the new map.

The monitor and the manual say "seven instruction fetches"; the
emulator counts **eight M1 cycles** because `ld sp,(0ah)` is an
ED-prefixed instruction: one instruction fetch, two M1 cycles.  The
kernel's return to user runs that instruction between the write and
the jump, so seven instructions are eight M1 cycles.  Count M1s, not
instructions.

The monitor's canonical sequence is:

```
ld a,task
ld (402h),a
six nops
jp destination
```

and `gotask` has the same shape with `pop hl`, `ex af,af'`, `exx`,
`ld sp,(u.sp)` (two M1s) and `jp user`.

### 3.11 User halt substitution

A user M1 fetch of `0x76` must not halt the CPU.  The board watches
the data during the fetch, disables the S-100 DI data, and drives
`0x00` to the T80.  The CPU sees a nop; the trap window starts on the
next read at `halt + 1`.  The cause is halt (`0xBB`).

The substitution is suppressed when the previous M1 fetched an `ED`,
`DD`, `FD` or `CB` prefix: those bytes are operands or prefix bytes,
not a halt instruction.  The emulator tracks this with its `prefix`
flag; the board latches the last M1 byte and does the same.  Without
the prefix check, `DD 76` (a `halt` disguised as `ld (ix+d),...`
prefix stream) would trap incorrectly.

A task-0 halt is trapped the same way (`0xBB`) and enters the monitor;
the `-H` emulator option exits instead, but the hardware has no
equivalent.

### 3.12 I/O traps

A user IN/OUT with mask bit 6 high is trapped and the S-100 cycle is
not performed:

- IN returns `0xFF` to the CPU;
- OUT discards the byte;
- the status is `0x3C` (IN) or `0xBC` (OUT);
- the trap is taken at the instruction boundary after the I/O
  instruction (`trapaddr` = the next instruction).

Mapped I/O for a supervisor is passed to the S-100 bus.  The MPZ80's
I/O address is A0–A7; with mask bit 7 (`ZIO`) high, A8–A15 carry the
Z80's upper port half, else zero.  A16–A23 are zero.  This reproduces
the 8080-style dual port address: `IN A,(n)` puts `n` on both halves,
`IN r,(C)` carries B.

### 3.13 Memory faults and RI10

A user mapped access checks the page's permission bits:

- execute (M1): require bit 1;
- read: require bit 0;
- write: require bits 0 and 1.

A violation blocks the access (read returns `0xFF`, write is
discarded), latches the faulting page pair in the trap-address
register, and traps at the instruction boundary after the faulting
instruction.  The status is `0xF7` in the emulator-compatible build
(RI10 set), which is what the kernel's `trap()` uses to reach
`fault()`.

RI10 (`PTE[2]`) means grow-on-reference: if the access is otherwise
legal, it completes and a trap is raised after the instruction.  If it
is illegal, it is blocked and the trap is immediate at the boundary.
The kernel's `GROW = 7` is `FULL(3) | RI10(4)`.

The board does not attempt a precise restartable fault.  The kernel's
grow-on-reference model and `mmu_test.c` were written for the deferred
behavior.

### 3.14 Interrupts and NMI

The S-100 pINT line (and VI0–VI7 on the card side) is ORed into the
trap logic.  The mask decides:

- user task, TINT high (`mask & 0x04`): do not pass the interrupt to
  the T80; latch an interrupt trap instead.  The trap is taken at the
  next instruction boundary, `trapaddr` = the current PC, status
  `0xB7`.
- user task, TINT low: pass it to `int_n`.
- task 0, SINT high (`mask & 0x20`): block it.
- task 0, SINT low: pass it to `int_n`.

When the interrupt is passed, the T80 performs the interrupt
acknowledge cycle (M1 + IORQ).  The bus core asserts S-100 `sINTA`,
and the 8259 on the MultIO/SS1 drives its 3-byte `call` vector on DI;
the T80 reads it.  The trap engine does not dispatch the interrupt;
the kernel's `trap()` enables interrupts and lets the 8259 vector
through the table at `0x200`.

NMI is passed straight through to `nmi_n`.  The stock SBC has no
S-100 NMI pin (section 4.7); add a jumper or leave NMI tied high until
the chassis needs it.

### 3.15 DMA, reset, clock, front panel

**DMA.**  S-100 `pHOLD` drives the T80's `busrq_n`; the T80's
`busak_n` drives `pHLDA`.  While the bus is granted, the FPGA
tri-states the address, data and control buffers.  The disk
controllers' DMA is the only user; the CPU board is a slave during
that time.

**Reset.**  S-100 `pRESET`/`pPOC` and the board reset button drive the
T80 reset and clear the translator.  Reset state:

```
taskreg      = 0
maskreg      = 0
delay        = 0
rom_half     = lower
trapstat     = 0xBF
trapreg      = 0
trapcount    = 15
trapaddr     = 0
```

The window starts on the first read after reset, at address 0.  The
mon500 reset half is twelve bytes of code followed by three zero
padding bytes; the `jp 0800` at `0xBF9` is followed by the target's
first three bytes, which the window replaces with those zeros.  The
monitor's `0x800` entry is `c3 03 08` (`jp 0803`), so executing three
nops and landing at `0x803` is exactly where the entry would have
gone.  The 16th read, at `0x803`, is normal.  mon447 and mon375 are
built the same way; each image's reset half is the authority.

The emulator instead starts the window at address 1 and skips a
zero local-RAM byte.  The board follows the hardware counter; the
register state is the same.

**Clock.**  4.000 MHz CPU, 2.000 MHz PHI, from the 50 MHz PLL.  The
S-100 bus is wait-stated; the CPU speed changes throughput, not the bus
protocol.  Keep 4 MHz to preserve the monitor's disk timing.

**Front panel.**  The switch register at `0x402` read is the board's
DIP switches, negated: a switch on reads low.  mon500 defines the
software bits (register values; S1 is bit 7, S6 bit 2):

| bits | meaning |
|---|---|
| `0x04` | multiuser after the single-user idle timeout (init reads it) |
| `0x08` | enable boot from floppy |
| `0x10` | enable boot from hdcdma/disk3 |
| `0x20` | enable boot from ide |
| `0x40` | enable boot from scsi |
| `0x80` | enable boot from hdca/disk2 |
| `0x02` | INT PEND (status) |
| `0x01` | TRAP RESET (status) |

Any `0x08`..`0x80` bit boots; with none set the monitor runs.  The
level 1 rom probes the enabled classes in policy order (floppy, mfm,
ide, scsi, hdca/disk2) and boots the first with a label.  S7 and S8
are not in the readable register; bits 1 and 0 are the two status
bits, not switches.  The current RTL returns `~DIP[7:0]`, so the
carrier's DIP[7:2] must be S1..S6 and DIP[1:0] (S7/S8) must be left
out of the software field.

`0x401` read is the keyboard/diagnostics connector.  With no front
panel, tie it to `0xFF` (no diagnostics) or to a DIP bit.  `0x400`
and `0x401` writes are the front-panel segment and column; ignore them
when no panel is fitted, as the emulator does.

## 4. The FPGA implementation

### 4.1 Partition

```
              +---------------------------------------------+
  T80s  <---->|  mpz80_seq  (sequencer + translator)        |
  a,di,do     |   - local decode                            |
  m1,mreq,    |   - map RAM (dual-port BRAM)                |
  iorq,rd,wr  |   - permission check / RI10                 |
  wait,int,   |   - trap window + ROM half                  |
  nmi,busrq   |   - trap address shift                      |
              |   - task register + 8-M1 delay              |
              |   - status/mask/switch                      |
              +----------------------+----------------------+
                                     |
                     +---------------+---------------+
                     |                               |
              +------+------+                 +------+------+
              | local page  |                 | S-100 core  |
              | 1K RAM      |                 | 24-bit addr |
              | 2K ROM      |                 | DI/DO       |
              | 512B map    |                 | control     |
              +-------------+                 | INTA/DMA    |
                                              +-------------+
```

The stock `Z80_FPGA.bdf` already contains the T80s wrapper and the
S-100 bus core.  Add the `mpz80_seq` module (VHDL-93, with a Verilog
twin if the repo keeps both) and replace the wrapper's memory/ROM
decode with it.  Keep the BDF as the top level only if editing it is
unavoidable; the preferred path is a new top-level HDL file that
instantiates the T80s, `mpz80_seq`, the bus core and the clock PLL,
so the design is text and reviewable.  Quartus can mix HDL and the
existing `.bsf` symbols either way.

### 4.2 The translator

On every CPU bus cycle the translator produces:

```
virtual  = { T80.a[15:12], T80.a[11:0] }
task     = taskreg[3:0]
page_idx = virtual[15:12]
pte_addr = (task << 5) | (page_idx << 1)
segment  = map[pte_addr]
attr     = map[pte_addr + 1]
local    = (taskreg[3:0] == 0) && (virtual < 0x1000)
physical = { taskreg[7:4], segment, virtual[11:0] }   // exact MPZ80
```

`physical` drives the S-100 A0–A23 when the cycle is not local and
not trapped.  `local` selects the on-board RAM/registers/map/ROM and
suppresses the bus.  The trap window overrides both: the data mux
supplies the ROM byte and the bus stays off.

The map lookup must complete before the S-100 address buffers are
enabled.  A dual-port BRAM read is one 4 MHz cycle; the existing bus
core's address setup is ample.  If the board is run faster than
4 MHz later, pipeline the lookup or add a wait state.

### 4.3 The trap sequencer

State: `IDLE`, `WINDOW`, `DONE`.

```
IDLE:
  on trap:
    trapstat  <= cause
    taskreg   <= 0
    rom_half  <= upper          (reset: lower)
    trapcount <= 15
    state     <= WINDOW

WINDOW:
  on a memory read:
    force ROM address 0xBF0 + (15 - trapcount)
    trapcount--
    if trapcount == 0: state <= DONE

DONE:
  normal execution; the next trap restarts
```

The window is entered on a halt's next read, an interrupt's next
instruction, a reset's first read, and the instruction boundary after
an I/O or memory fault.  The `trapcount`/`state` pair also inhibits
new traps and interrupts while it is nonzero.

The forced data path: the T80's `di` mux selects the ROM byte when
`state == WINDOW` and the read is in range.  The S-100 DI buffer and
the control buffers stay disabled.  The `0x00` halt substitution is a
separate one-cycle mux on the halt's M1.

### 4.4 Local page and ROM

| device | size | implementation |
|---|---|---|
| local RAM | 1 KB | M9K BRAM |
| map RAM | 512 B | M9K dual-port BRAM |
| monitor ROM | 2 KB at 0 | JEDEC socket ROM; boot selector at 0x800 |
| register file | 8 B | logic |

The 1 KB local RAM is the MPZ80's on-board static RAM: the monitor's
register save areas, the `0x200` temp stack, the `u` save frame and
the task-switch stub live there.  It must be writable at full CPU
speed; no S-100 cycle.

The monitor image is no longer a `.mif` compiled into block RAM.  The
JEDEC socket part holds the 2 KB monitor image (two 1 KB halves) at
offset 0; `rom_half` selects `0x000–0x3FF` or `0x400–0x7FF`.  The
same part holds the boot selector at offset `0x800`; the FPGA decodes
physical `0xFF0000–0xFFFFFF` and maps it there, so the monitor's
selector boot reads it locally and the S-100 cycle is suppressed.

The FPGA drives the part's A0–A15 on the shared `FPGA_OUT_A*` pins,
A16–A18 on `FPGA_OUT_RAM_A16–A18`, data on `FPGA_BI_D`, and the
`FPGA_OUT_RAM_OE-/WR-/CS-` strobes.  The S-100 address buffers are
disabled for a local ROM or selector read, so the shared low address
is private to the fetch.  `FPGA_OUT_RAM_WR_n` is held high in this
build; the part is programmed externally.  A future in-system flash
loader can drive the same write pin.

### 4.5 The S-100 bus master

The existing bus core already drives the IEEE-696 signals the
S100Computers board routes.  The MPZ80 build keeps the bus cycle and
changes the address source and the local/trap gating:

| signal | MPZ80 use |
|---|---|
| A0–A19 | translated physical A0–A19 (not the T80's raw address) |
| A20–A23 | task register bits 7–4 (V1.6 pins) |
| DO0–DO7 | T80 `do` on writes |
| DI0–DI7 | S-100 DI to T80 `di` on reads, and the 8259 vector during INTA |
| pSYNC, pSTVAL | start of cycle |
| pDBIN, pWR | read/write strobes |
| sMEMR, sMEMW/sWO | memory read/write |
| sINP, sOUT | I/O read/write |
| sM1, sINTA | instruction fetch, interrupt acknowledge |
| pWAIT | master wait, if the board routes it |
| pRDY/XRDY | slave ready; hold `wait_n` and pWAIT |
| pHOLD/pHLDA | DMA |
| pRESET/pPOC | reset |

The state machine is the existing `IDLE → ADDR → READ/WRITE → WAIT →
DONE` sequence.  The MPZ80 adds one rule: a local or trap-window cycle
never enters `ADDR`.  The address/control/data output enables are
deasserted, and the cycle completes from the on-board devices.

The I/O address rules of section 3.12 are applied at the address
drivers: A0–A7 = port; A8–A15 = `mask[7] ? port_high : 0`; A16–A23 =
0.  The 8259 vector path is unchanged: M1 + IORQ asserts `sINTA`, and
the DI buffer stays enabled so the vector reaches the T80.

### 4.6 Clock and reset

The 50 MHz input feeds a PLL with a 4.000 MHz output.  The T80 runs on
that; a divide-by-two produces the 2.000 MHz PHI.  Reset is the wired
OR of S-100 pRESET/pPOC and the board button; the FPGA holds the T80
in reset for a few clocks after configuration.

The stock board's `FPGA_OUT_CPU_CLK`, `FPGA_OUT_PHI` and
`FPGA_OUT_2mHz_CLOCK` pins are reused: CPU_CLK = 4 MHz, PHI = 2 MHz,
2mHz_CLOCK = PHI (or the second clock buffer the backplane wants).
Verify the polarity and the 74LVC245 direction against the V1.6
schematic before layout.

### 4.7 Pin budget

The MPZ80 build adds the 4 upper address pins and a handful of
switches, and drops the peripherals it does not use.

| group | stock | MPZ80 | notes |
|---|---:|---:|---|
| clock/reset | 5 | 5 | CLK_50, reset, PHI, 2mHz, CPU_CLK |
| S-100 address | 20 | **24** | A0–A19 + EXT_A20–A23 |
| S-100 data | 16 | 16 | DO0–7, DI0–7 |
| S-100 control | 20 | 20 | pSYNC/pSTVAL/pDBIN/pWR, sMEMR/sWO/sINP/sOUT/sM1/sINTA/sHLTA, MWRT, enables |
| S-100 status/DMA | 8 | 8 | XRDY, RDY, pHOLD, pHLDA, INT, CDSB, SDSB, BOARD_RESET |
| DIP switches | 8 | 8 | front-panel switch register |
| LEDs/debug | 4 | 4 | optional |
| on-board I/O | 0 | **21** | VGA 5, PS/2 2, USB 2, IDE/CF 7, RTC 5 |

The exact split depends on the V1.6 pin list.  VGA and PS/2 are the
V1.6 board's own connectors (R=T6, G=T7, B=T8, HSync=T9, VSync=T10,
PS/2 data/clock=B12/B13); the IDE, RTC and USB pins are the board's
stock assignments.  If the board is a V1.1, EXT_A20–A23 are not
routed; either use V1.6 or add a four-wire rework to the FPGA.

The S-100 interrupt controller on the SBC (74148/74373/74244) is not
the MPZ80's 8259.  Fit **JP10** to bypass it and bring the raw pINT to
the FPGA's `FPGA_IN_INT-` pin.  The 8259 stays on the MultIO/SS1 and
supplies the vector during `sINTA`; the SBC must not answer `sINTA`
itself.

### 4.8 Resource budget

The stock design uses 3,891 LEs and 196,608 RAM bits.  The JEDEC-ROM
MPZ80 build measures **6,174 LEs and 8,192 RAM bits**; adding the
8259 pair with the four VI inputs and the on-board vector tied off
gives **6,410 LEs (62%), 3,332 registers and 98 pins**.  The worst
case with all sixteen IR inputs live is about 6,688 LEs (65%).  With
the two 8250 front ends, the USB UART, the uPD1990 and its DS1305
calendar, the image is **7,649 LEs (74%), 3,901 registers and 111
pins**.  The VGA/PS2 console adds the 80×40 text and font memories
and the keyboard translator: with it the complete image is **8,813 LEs
(85%), 4,209 registers, 118 pins and 73,728 RAM bits (17%)**, leaving
1,507 LEs free.  The on-board MYIDE adds the 82C55/CF decode and the
0x34 drive-select latch.  Timing closes with 6.767 ns of setup slack
at 50 MHz, 11.755 ns at the 25 MHz pixel clock and 211.8 ns at 4 MHz;
the only
cross-domain paths are the UART handshake and the display memory,
which the SDC and the dual-clock RAM handle.  The
translator is small — a 512-byte map, a 1 KB local RAM, the T80 and
the bus logic account for the rest.  The JEDEC read path and selector
decode are small, the 2 KB block ROM is removed, and the 512K SRAM is
not fitted.

## 5. Optional extensions

The exact MPZ80 is the default.  Two compile-time options are worth
keeping in the RTL, both off:

### 5.1 PTE bank (`EXT_PTE_BANK`)

Move A20–A23 from the task register's high nibble into the map entry's
odd-byte high nibble:

```
A[23:12] = {PTE[15:12], PTE[7:0]}
```

This is the Z180 board's 16-bit PTE (`hw/z180-cpu.md`, section 2.2).
The task register's high nibble becomes latched context, not an
address.  The selector boot must then use a direct PTE map instead of
the task-register bank, exactly as `hw/z180-cpu.md` section 4.4 and
8.5 describe.  With the option off, the odd byte's high nibble is
stored but unused.

The option changes the monitor, not the kernel: the kernel's PTE high
nibbles are zero and it stays below 1 MB.

### 5.2 Second 1 KB local RAM / FPU

The `0xC00–0xFFF` window can be the optional FPU (9511/9512) or a
second 1 KB local RAM.  FPU and RAM are mutually exclusive.  The
kernel does not use the FPU; decode the window as open bus in the
default build.

### 5.3 The Wunderbus/MultIO on the FPGA

The board implements the Wunderbus/MultIO on-board functions rather
than relying on a real card: the 8259 pair, the two 8250s (VGA and
USB), the uPD1990 clock and the 82C55/CF IDE.  The S-100 bus carries
the memory and up to four VI sources; more than four needs additional
VI wiring.  The VGA/PS2 console and the DS1305-backed clock are
implemented in the same image.

The interrupt-controller half is measured: two 8259A instances plus the
0x4C/0x4D and 0x44/0x45 decode, the 0x4F/0x47 selects, the IR2 cascade
fold and the 3-byte INTA mux fit in **514 LEs and 167 registers** with
all sixteen IR inputs live (`fit/README.md`), so the worst-case combined
image is about 6,688 LEs (65%) with 3,632 LEs free.  The first-cut
integration — four VI inputs, on-board vector tied off — measures
**6,410 LEs (62%), 3,332 registers and 98 pins**.  The wirings are
parameters: `SLAVE_PRESENT=0` for `wb`; `SLAVE_PRESENT=1, CASCADE=0` for
the polled `wb,mio`; and `SLAVE_PRESENT=1, CASCADE=1` for the built
`mio,mio` two-MultIO machine.

`sys/pic_init.c` is the authority for the IR map (the built kernel does
not use `miopic_init.c`).  It programs two CAS-cascaded 8259s with
`CASCADE_IR 2`: master IR0–IR1 are VI0–VI1, master IR2 is the slave's
INT (the notional VI2 cascade), master IR3–IR5 are UART1–3, master IR6
is spare, and master IR7 is the clock.  The slave's IR0–IR1 are VI3–
VI4, its IR2 is the dropped VI5/IDE position, and its IR3–IR7 are
second-board sources.  The four S-100 VI inputs are therefore VI0, VI1,
VI3 and VI4; VI2 is internal to the FPGA, not a bus line, and VI5 is
not routed.  `pic_init.c` probes group 0's 0x49 to choose the cascade:
a MultIO reads open bus (0xFF) and the master is programmed with
`ICW1=0x1d, ICW3=1<<2`, while a Wunderbus DIP (0x00) falls back to the
master-single/slave-polled path.  This is the "pretend to be MIO
functions" mapping in `rtl/mio_pic.v`: the `onboard` vector names the
FPGA's console UART, clock, IDE/CF and the rest.

The UART/clock plan is the MIO's own layout: UART1 (master IR3) is the
VGA console, UART2 (IR4) is the USB serial, and UART3 (IR5) is tied
off.  The uPD1990 clock/calendar (master IR7) is backed by the board's
DS1305 for the 40-bit BCD calendar, but its heartbeat is the FPGA
clock: the kernel programs `CC_32HZ` and `mio.s` divides by four to
the 8 Hz tick, so the DS1305's 1 Hz output cannot be the tick.  The
uPD1990's month is binary 0–11 while the DS1305's is BCD 1–12, and the
uPD1990 has no year; the DS1305 year is only for the FPGA's own
calendar.  The SBC's parallel interface is unused.

The JEDEC socket is the monitor/boot-selector ROM (section 4.4); the
512K SRAM is not fitted.  The JEDEC part is private to the FPGA and
not visible to an S-100 temporary master, so incoming DMA cannot
reach it, but it is the ROM, not main memory.  The local page and map
are block RAM; main memory stays on the S-100 cards.

The RTL instantiates the pair in `mpz80_top` with `CASCADE=1`,
`SLAVE_PRESENT=1`, and the four VI inputs on `FPGA_IN_INT_A-D-` as
VI0/VI1/VI3/VI4.  The master's 0x4C/0x4D (group 0, selected by 0x4F), the
slave's 0x44/0x45 (selected by 0x47) and group 0's open-bus 0x49
probe are decoded
locally, the S-100 cycle is suppressed for them, and the 3-byte INTA
vector is muxed into the CPU data path.  On a cascaded acknowledge
`mio_pic` returns the slave's vector; `im0_call` uses that same
effective vector.  The UART/clock IR inputs (IR3–IR5/IR7) are driven by
the front ends below.

The kernel programs the 8259 for an 8080-mode three-byte CALL
(`ICW4` bit 0 = 0, `ICW1` bit 2 = 1) and puts a four-byte JP entry at
each slot.  A Z80 in IM0 executes the INTA opcode but fetches the two
operand bytes as ordinary memory reads, so `rtl/im0_call.v` plays the
same game the simulator does (`src/include/z80.h`, "special hack for
i8259 in mode 0"): after an acknowledge whose byte is 0xCD it answers
the two operand reads from the vector the 8259 captured, gives the
8259 the two extra acknowledge strobes, and replaces the two stack
pushes with the return address the T80 should have pushed (its CALL
increments PC while reading the operands, so it would otherwise push
PC+2).  The phase logic was checked against the T80 core in GHDL; the
simulation shows the CALL reaching the vector slot and the `ret`
returning to the interrupted instruction.

The same cut adds two `i8250` front ends and the `uPD1990`: UART1
(IR3) is the VGA console, UART2 (IR4) is the USB FT231x through the
vendor UART at 9600, UART3 (IR5) is tied off, and the uPD1990 (IR7)
uses the DS1305 SPI calendar and the FPGA 32 Hz heartbeat.  UART1's
backend is `rtl/vga_console.v`: an 80×40 text page in a true
dual-port block RAM (25 MHz display read, 50 MHz console read/write),
Javier Valcarce's `vga80x40` display core at 25 MHz, the `lat0-12`
4 KB font ROM, and the Digi-Key PS/2 keyboard translator feeding the
8250's receive handshake.  The writer handles CR, LF, BS, HT and FF
and scrolls the page; the cursor is the display core's underline
cursor.  The 4 MHz CPU and 50 MHz USB
UART handshake across a clock boundary, so the SDC marks those paths
asynchronous; the 4 MHz logic itself closes at 26.2 MHz Fmax.  The
The on-board 82C55/CF is the Micronix MYIDE.  The local decode covers
0x30 PA, 0x31 PB, 0x32 PC, 0x33 PCTL and 0x34 PDRV; the physical
82C55 drives the CF, the PDRV latch is in the FPGA, and the CF's
INTRQ goes to the PIC's slave IR2 (`onboard[6]`, the dropped VI5/IDE
position), so the four S-100 VI lines stay available.  The CF INTRQ
must be wired to `IDE_INTRQ` (pin C9, the repurposed printer ACK).
The mon500 `ideboot` and the kernel's `sys/ide.c` see the same 82C55
protocol; the on-board data bus is shared with the JEDEC ROM and the
chip selects are exclusive.

The resulting image is **8,813 LEs (85%)**, leaving 1,507 LEs free.

The trade-off is that only VI0, VI1, VI3 and VI4 are real S-100 lines;
the VI2 position is the internal cascade between the two emulated
MultIOs, and VI5 is the dropped IDE/on-board position.  The kernel's
driver probe discovers the line rather than assuming its number.  An
on-FPGA PIC also owns 0x44/0x45/0x47/0x4C/0x4D/0x4F, so it is a standalone
profile, not something to enable beside a real MultIO.

## 6. Firmware and boot

### 6.1 Monitor images

| image | boot device | notes |
|---|---|---|
| `mon500.bin` | MYIDE / IDE at `0x30–0x34` | the in-tree default; `ideboot` |
| `mon447.bin` | HDCA | original HDCA boot |
| `mon375.bin` | older monitor | for the earliest machines |

Each is a 2 KB image (two 1 KB halves) assembled for `0x800–0xBFF`.
The FPGA ROM stores the selected images; a DIP field selects which
one is aliased at `0x800`.  The reset half's `0xBF0` is the reset
window; the upper half's is the trap window.  The `mon500.s` comments
and `stand/roms/GNUmakefile` are the build authority.

### 6.2 Boot

1. Reset: task 0, mask 0, lower ROM, 15-read reset window at `0xBF0`.
2. The reset sequence writes task 0 page 1 segment 0 and jumps to
   `0x800`.
3. The monitor runs from the lower half, initializes the PIC/UART on
   the S-100 cards, reads the switch register at `0x402`, and boots the
   selected device.
4. The level-1 loader reads the kernel; the first task-register write
   selects the upper ROM half and starts the 8-M1 delay.
5. The kernel runs unchanged; the FPGA is transparent except for the
   MMU and the trap engine.

## 7. Verification

### 7.1 Unit testbench

Build `mpz80_seq` with a small S-100 memory model and the T80s, and
run the sequences that `mpz80.c` and `mmu_test.c` already cover:

1. reset window: the 15 reads at `0xBF0`, the `jp 0800` target outside;
2. map a task, read a known segment, check A12–A23;
3. task switch: 8 M1s, the final M1 translated with the new map;
4. user halt: `0x00` substitution, window at `halt+1`, status `0xBB`;
5. user interrupt: no `int_n`, window at PC, status `0xB7`;
6. supervisor interrupt: `int_n`, `sINTA`, 8259 vector on DI;
7. I/O trap: IN returns `0xFF`, OUT discarded, status `0x3C`/`0xBC`;
8. permission fault and RI10: blocked/completed, status `0xF7`;
9. trap-address shift and the latched fault pair;
10. prefix suppression: `DD 76` does not trap as a halt;
11. DMA: pHOLD/pHLDA and bus tri-state.

The testbench compares the translator's physical address, status and
data with the emulator's trace for the same sequence.  GHDL for the
VHDL twin, iverilog for the Verilog twin, as `hw/cpld/Makefile` does.

### 7.2 Whole-machine test

Boot `mon500.bin` + the level-1 loader + `sys/unix` in the chassis
with the real cards.  The existing `TESTING.md` matrix and `regress`
are the host-side checks; the board bring-up adds:

1. local RAM read/write at `0x000–0x3FF`;
2. map RAM write/read-back at `0x600–0x7FF` (write-only to the CPU);
3. known PTE to a known S-100 memory location, check A12–A23;
4. trap window: scope the 15 reads at `0xBF0` and the 16th target;
5. S-100 memory and I/O cycles, wait states, pRDY/XRDY;
6. interrupts: VI line to the 8259, vector, mask/trap behavior;
7. DMA: pHOLD/pHLDA, bus tri-state;
8. full boot: monitor, loader, kernel, userland.

## 8. Bring-up

1. Power: 5 V and 3.3 V, no hot parts.
2. Clock and reset: 4 MHz CPU, 2 MHz PHI, T80 reset released.
3. JTAG: the Cyclone IV is configured; the T80 runs.
4. Local page: RAM, registers, map RAM.
5. Translation: a PTE, a known S-100 address, A12–A23 at the edge.
6. Trap window: the reset sequence, then a user halt and an interrupt.
7. S-100 bus: memory read/write, I/O read/write, wait states.
8. Interrupts: the 8259's vector through `sINTA`.
9. DMA: pHOLD/pHLDA.
10. Boot: monitor, level-1 loader, kernel, userland.

## 9. Open decisions and known differences

| item | MPZ80 / manual | this board |
|---|---|---|
| task register high nibble | A20–A23 bank | same (exact); the extension moves it to the PTE |
| local page select | task 0, address < 0x1000 | same |
| map entry odd byte high nibble | unused | stored, unused (extension uses it) |
| task-switch delay | 7 instruction fetches | 8 M1 cycles (emulator/kernel) |
| halt substitution | M1 fetch of 0x76 | same, with ED/DD/FD/CB prefix suppression |
| memory fault status | manual 0x3E/0x7E/0xBE/0xFE | emulator-compatible 0xF7 by default |
| SINT sense | manual wording is the reverse | high blocks (emulator/KERMASK) |
| NMI | S-100 NMI | pass-through; the stock SBC has no NMI pin, add a jumper |
| pWAIT | master wait | the existing core waits on pRDY/XRDY; add pWAIT if the V1.6 route provides it |
| pRESET/pPOC | S-100 reset | verify the V1.6 reset circuit wires them to the FPGA |
| FPU | 9511/9512 at 0xC00 | open bus; optional second 1 KB RAM |
| JEDEC socket | n/a | monitor/boot-selector ROM; the 512K SRAM is not fitted |
| VGA/PS2/USB/IDE/RTC | n/a | VGA console, PS/2 keyboard, USB serial, IDE/CF and DS1305 used; SD and printer unused |
| DPLL/clock | 4 MHz CPU, 2 MHz PHI | same |
| board revision | n/a | **V1.6** for EXT_A20–A23; V1.1 needs a four-wire rework |

Open items before layout:

- Confirm the V1.6 pin numbers for EXT_A20–A23 and the pWAIT,
  pRESET/pPOC and NMI routes against the schematic.
- Confirm the JP10 bypass polarity and that the raw pINT reaches
  `FPGA_IN_INT-`.
- Decide whether the top level is a new HDL file or the existing
  `Z80_FPGA.bdf` with an HDL block (the former is preferred).
- Confirm the T80s `m1_n` and `rfsh_n` behavior for the prefix latch
  and the trap-window count against the core's VHDL, not just its
  documentation.
- Confirm the JEDEC part type/size and the socket wiring: the monitor
  needs 2 KB at offset 0, the boot selector 64 KB at offset 0x800, and
  the part needs at least 128 KB; the address A16–A18 and CS/OE/WE
  must follow the stock RAM bus.
- Confirm the selector's physical address and size.  The contract uses
  `0xFF0000–0xFFFFFF` and a 64 KB image; a different selector moves
  the decode.
- Decide whether the part is a flash the FPGA can program in-system
  (drive `FPGA_OUT_RAM_WR_n`) or an EPROM programmed externally.
- Confirm the 50 MHz PLL can produce exactly 4.000 MHz on this device
  (M=2, N=25, C=1); otherwise use a 4.000 MHz can or a fractional
  divider and re-check the monitor timing.

## 10. References

- `src/hwsim/d1/mpz80.c` — MMU, trap, task delay, status/mask model.
- `src/hwsim/d1/mmu_test.c` — PTE, RI10 and trap-register tests.
- `src/micronix/stand/roms/mon500.s`, `mon447.s`, `mon375.s` — reset,
  trap, selector boot and map routines.
- `src/micronix/sys/trap.c`, `malloc.c`, `ARCHITECTURE.md`,
  `include/sys/proc.h`, `include/sys/sys.h`, `sys/uhdr.s` — kernel
  contract.
- `hw/z180-cpu.md` — the compatible extension, the 16-bit PTE and
  the Z180 board.
- `publicdocs/Morrow-MPZ80-CPU-Technical-Manual-Rev1-Apr82.pdf` and
  `extra/docs/MPZ8010.ECN.txt` — original hardware and ECN-10.
- S100Computers FPGA Z80 SBC page and its Quartus project
  (`Z80_FPGA.bdf`, `Components/Z80/T80*.vhd`); V1.6 adds EXT_A20–A23.
- IEEE-696 S-100 bus standard.
- `hw/esp32-nic.md` — document style and table conventions.

## 11. Change log

- 2026-11-04: first contract.  Bus-master build, exact MPZ80 default,
  PTE-bank extension reserved, T80s + S-100 core retained, monitor
  images and verification defined.
