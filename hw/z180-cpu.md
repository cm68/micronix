# The Z180 MPZ80-compatible S-100 CPU board

A modern CPU board for Micronix: a Z8S180, two socketed ATF1508AS
CPLDs, and an external 16-bit paging MMU/trap engine.  It is intended to
run the existing Micronix monitor, kernel and userland unchanged below
1 MB, while extending the MPZ80 page table from 12 bits (8 address + 4
permissions) to 16 bits (12 address + 4 permissions).  The four new
address bits are A20–A23, supplied by the PTE's high nibble.  The
Z180's own base-and-bounds MMU is left transparent and its upper
address outputs are not connected.

This is a design document, not a built board.  It follows the
contract-first style of `esp32-nic.md`: the software-visible interface
is fixed first, then the logic, memory and bus that implement it.  The
executable specifications are `src/hwsim/d1/mpz80.c` and
`src/hwsim/d1/mmu_test.c`; the kernel's view is
`src/micronix/sys/ARCHITECTURE.md`.

## 1. The contract

### 1.1 What the software sees

The board presents the MPZ80's 64K task address space.  When task 0's
current map entry has segment field 0, the low page is the CPU board's
own page and never reaches the S-100 bus:

| address | read | write |
|---|---|---|
| `0x000–0x3ff` | local RAM | local RAM |
| `0x400` | trap address register | front-panel segment |
| `0x401` | keyboard connector | front-panel column |
| `0x402` | switch register | **task register** |
| `0x403` | **trap status register** | **mask register** |
| `0x600–0x7ff` | open bus | **map RAM**, write-only |
| `0x800–0xbff` | monitor ROM (one 1K half) | ignored |
| `0xc00–0xfff` | optional FPU | optional FPU |
| `≥ 0x1000` | mapped memory | mapped memory |

Tasks 1–15 see the same numeric low addresses as ordinary mapped user
memory.  The local page exists only when task 0 is current and the map
entry for the current page has segment field 0.  In normal use that is
task 0, page 0.  The low address lines then select on-board RAM,
registers, map RAM or ROM.

The map RAM is write-only.  The monitor keeps a readable image at
`0x200`; the kernel keeps the master copy in `struct proc.mem[]` and
loads the hardware from it with `taskload()`.  Reading `0x600–0x7ff`
returns open-bus garbage and software must not rely on it.

### 1.2 The task register

`0x402` write is the task register:

```
bits 7-4   latched context; NOT an address field on this board
bits 3-0   task number; selects the 32-byte map table
```

The low nibble behaves as it does on the MPZ80.  The high nibble is
latched but does not drive A20–A23; those come from the PTE.  Section
4.5 analyses using it for future context bits and why that is not free.

Any write to `0x402` starts the task-switch delay (section 3.10) and
selects the upper 1K monitor-ROM half.

`0x402` read is the switch register, not the task register.  The upper
six bits are the front-panel switches, negated (switch on reads low);
bit 1 is INT PEND and bit 0 is TRAP RESET (low active).  The boot code
uses the switch field exactly as `mon500` does:

| bits | meaning |
|---|---|
| `0x04` | no monitor: boot the level-1 loader |
| `0xf8` | start-address mask |
| `0x00` | boot the local hard disk (HDCA under mon447, IDE under mon500) |
| `0x08` | boot HD-DMA |
| `0x10` | boot DJ-DMA |

### 1.3 Status and mask

`0x403` read is the trap status register.  Bits are active low except
RI10 (bit 6), which is high when the trap was caused by a segment with
the RI10 bit set:

| bit | name | meaning |
|---|---|---|
| 0 | VOID | illegal memory or I/O reference |
| 1 | IORQ | illegal I/O reference |
| 2 | HALT | halt attempted where halts are disabled |
| 3 | INT | interrupt occurred in a task that may not take interrupts |
| 4 | STOP | front-panel stop |
| 5 | AUX | auxiliary trap |
| 6 | RI10 | trap caused by a RI10 segment |
| 7 | RD STB | low for a read, high for a write |

`0x403` write is the mask register:

| bit | name | sense |
|---|---|---|
| 0 | STOP disable | low enables the stop trap |
| 1 | AUX enable | spare enable |
| 2 | TINT disable | high forces user interrupts to trap to task 0 |
| 3 | RUN enable | high allows a task to run; low single-steps |
| 4 | HALT disable | high makes a user halt trap |
| 5 | SINT enable | high allows supervisor interrupts |
| 6 | I/O disable | high makes user I/O trap |
| 7 | ZIO mode | high echoes the port address on A8–A15 |

The kernel's masks are `KERMASK = 0230` (0x98: ZIO, halt trap, run) and
`USRMASK = 0135` (0x5d: I/O trap, halt trap, run, interrupt trap, stop
trap).

## 2. Address translation

### 2.1 Pages and the physical address

A task is 64K, divided into sixteen 4K pages.  The virtual page number
is A15–A12; the offset is A11–A0.  The map entry supplies a 12-bit
physical page number; the physical address is

```
A[23:12] = {PTE[15:12], PTE[7:0]}
A[11:0]  = Z180 A[11:0]
```

The S-100 bus is a standard 24-bit IEEE-696 bus.  There are no A24+
lines in this revision.

### 2.2 The 16-bit PTE

Each page has two bytes in map RAM.  Even byte first, odd byte second:

| PTE bits | signal | meaning |
|---|---|---|
| 7–0 | A19–A12 | physical segment, unchanged from the MPZ80 |
| 8 | read enable | data reads |
| 9 | execute enable | instruction fetches |
| 10 | RI10 / GROW | access completes, then traps |
| 11 | unused | ECN-10's bit-3 A20 is not implemented |
| 15–12 | A23–A20 | the four new address bits |

In the monitor's terms the odd byte is the access byte: its low nibble
is permissions and its high nibble is the bank.  The MPZ80 ignored that
high nibble; this board uses it as A23–A20.

### 2.3 Task 0 and supervisor rules

- Task 0 with the current page's map entry segment field 0 is local.
  The low address lines select on-board RAM, registers, map RAM or ROM;
  the S-100 address and data drivers are disabled for the access.
- Task 0 with a nonzero segment uses task 0's map.  Task 0 bypasses
  permission checks.
- Tasks 1–15 use their own map for every page, including page 0.

The local page is selected by segment 0, so the monitor's identity map
must put segment 0 in task 0's local page entry.  In normal use that is
page 0; the low A11–A0 decode then picks the device.

### 2.4 Permissions and RI10

The manual's permission matrix is the default:

| PTE[9:8] | read | write | execute |
|---|---|---|---|
| 00 | no | no | no |
| 01 | yes | no | yes (read-only is executable) |
| 10 | no | no | yes |
| 11 | yes | yes | yes |

The kernel only uses `NONE 0`, `FULL 3` and `GROW 7`; the 01/10 rows
matter to the monitor and to CP/M-era software.  The emulator's
`mpz80.c` checks execute as `attr & 0x2`, which makes 01 non-executable;
the board defaults to the manual's matrix and provides a CPLD build
option for the emulator matrix if a regression ever requires it.

RI10 (PTE[10]) means "grow on reference":

- If the access is otherwise legal, it completes and a trap is raised
  after the instruction.
- If the access is illegal, it is blocked and a trap is raised.

The kernel's `GROW = 7` is `FULL(3) | RI10(4)`, so the first touch of a
grow page completes and then reaches `fault()`.

### 2.5 Map RAM layout and writes

Map RAM is 16 tasks × 16 pages × 2 bytes = 512 bytes.  Page entry
addresses are

```
0x600 + (task << 5) + (page << 1) + byte
```

The even byte is A19–A12.  The odd byte is `{A23–A20, unused, RI10,
execute, read}`.

The map is two 8-bit RAMs.  RAM A holds the even byte and drives the
S-100 A12–A19 line drivers directly.  RAM B holds the odd byte; its high
nibble drives the A20–A23 line drivers directly, and its low nibble
(the permissions) is the only part that enters U1.  The address bits do
not pass through any CPLD.

CPU writes to `0x600–0x7ff` in task 0's local page drive both RAM data
inputs directly.  The external map address mux selects the CPU's
A8–A1, U1 supplies `MAP_WE`, and U1 controls the RAM `/OE`.  The map is
write-only to the CPU.

### 2.6 Worked examples

```
task 1, page 0, segment 0x10, FULL:
    PTE low 0x10, high 0x03  ->  PTE = 0x0310
    physical = 0x0010 << 12 = 0x001000

selector ROM, page 1, segment 0xF0, bank 0xF, FULL:
    PTE low 0xF0, high 0xF3  ->  PTE = 0xF3F0
    physical = 0xFF0 << 12 = 0xFF0000

top of the 16 MB space, page 0, segment 0xFF, bank 0xF:
    PTE low 0xFF, high 0xF3  ->  PTE = 0xF3FF
    physical base = 0xFFF000
```

### 2.7 The Z180's address lines

The Z180's A0–A11 are the page offset and go to the S-100 bus through a
buffer.  A12–A15 are the page index into the external map.  A16–A19
are not used by the external MMU and are left unconnected.  The Z180's
internal MMU must remain in its reset pass-through state; see section 5.

### 2.8 The on-board flash region

The top 1 MB of the physical space, **F00000–FFFFFF**, is the on-board
flash.  A 1M×8 5 V flash fills it.  The CPLD decodes A23–A20 = F after
translation and serves the access locally: the flash address is
A0–A19, the data returns on the Z180 data bus, the S-100 address and
data drivers are disabled, and no S-100 cycle occurs.  Any task can map
a PTE to a page in this region; the normal PTE permissions apply.

The kernel's 1 MB allocator (`MAXSEG 256`) never reaches this region.
The monitor can map it to read boot images, the selector, or the rest
of the flash.  The selector at physical `0xFF0000` is inside the
region, so the existing selector PTE `0xF3F0` still points at it.

The monitor's 2K alias at `0x800–0xbff` is the first (or DIP-selected)
2K image of the same flash; the reset and trap windows use that alias.
The region and the alias are two views of the same device.

## 3. Trap sequencing

### 3.1 Trap sources

| source | condition |
|---|---|
| reset | S-100 pRESET/pPOC or the on-board reset button |
| stop | front-panel stop input and mask bit 0 low |
| aux | auxiliary input and mask bit 1 high |
| halt | user M1 fetch of 0x76 and mask bit 4 high |
| I/O | user IN/OUT and mask bit 6 high |
| interrupt | S-100 interrupt in a user task and mask bit 2 high |
| protection | user reference to a segment whose permissions fail |
| RI10 | user reference to a segment with PTE[10] set |

Task 0 is the supervisor and is not subject to the user halt/I/O/
interrupt/memory traps; a task-0 halt goes to the monitor or the reboot
path.

### 3.2 The trap event

A trap does all of the following, in the order the hardware sees them:

1. Latch the cause in the trap status register.
2. Latch the trap address pair from the trap-address shift register.
3. Clear the task register to 0, selecting task 0's map.
4. Select the upper 1K monitor-ROM half.
5. Load the trap-window counter and the trap window base address.
6. Inhibit further traps and interrupts during the window.
7. Force the next memory reads in the window to come from flash.

The CPU's PC is not changed by the trap.  It continues to count from the
trapping instruction, and the forced bytes make it execute the firmware
sequence as if that sequence were at its own PC.  This is why the
`call trappd` at offset 12 pushes the trapping address plus 15, and
why `trappd` subtracts 15 to recover the address.

### 3.3 The 15-byte window

The upper 1K ROM half contains the trap entry at `0xbf0`:

| address | bytes | instruction |
|---|---|---|
| `0xbf0` | `00` | nop (must be a nop to void a halt) |
| `0xbf1` | `ed 73 0a 00` | `ld (u.sp),sp` |
| `0xbf5` | `31 14 00` | `ld sp,begsav` |
| `0xbf8` | `f5` | `push af` |
| `0xbf9` | `e5` | `push hl` |
| `0xbfa` | `d5` | `push de` |
| `0xbfb` | `00` | nop |
| `0xbfc` | `cd 21 08` | `call trappd` |
| `0xbff` | `76` | task-0 halt guard, not part of the 15-byte window |

The 15-byte sequence is `0xbf0–0xbfe`; `0xbff` is the guard.  The
window is 15 **memory reads**, not 15 M1 cycles: operand fetches count
too.  It is simply the next 15 reads after the trap.  Because the
sequence is fetched linearly from the trapping PC, the hardware counts
reads and ignores the CPU's address; it supplies flash bytes
`0xbf0 + count` and does not need an address comparator.  The fifteenth
read is the last operand of the `call`; the call's target is the
sixteenth read, outside the window, and normal ROM execution resumes.

The reset window uses the same mechanism with the **lower** 1K ROM half.
The reset code there is 15 bytes (the patched sequence of section 8.3);
the counter covers it exactly, and the `jp` target is the sixteenth
read.

### 3.4 Trap address rules

| cause | `trapaddr` |
|---|---|
| reset | 0 |
| user halt | halt address + 1 |
| interrupt | address of the next instruction (no +1) |
| memory fault | the instruction boundary reached after the faulting instruction (emulator semantics) |

For a halt, the `+1` accounts for the halt's own M1 fetch: the halt is
voided, and the window starts at the instruction after it.  For an
interrupt, the CPU is already between instructions, so adding one would
land in the middle of the next instruction.  For a deferred memory
fault, the emulator traps at the PC after the instruction; the kernel's
grow-on-reference model depends on that, so the hardware does the same.

### 3.5 Trap status values

The manual's Table 4-4, with the emulator's memory-fault convention
noted:

| event | status |
|---|---|
| reset | `0xbf` |
| halt | `0xbb` |
| IN | `0x3c` (RI10: `0x7c`) |
| OUT | `0xbc` (RI10: `0xfc`) |
| memory read, no access | `0x3e` (RI10: `0x7e`) |
| memory write, no access | `0xbe` (RI10: `0xfe`) |
| interrupt | `0xb7` (emulator; the manual also lists `0x37`); RI10 `0xf7`/`0x77` |
| stop, read | `0x2f`; write `0xaf` |
| aux, read | `0x1f`; write `0x9f` |
| user memory fault, emulator-compatible | `0xf7` |

The last row is deliberate.  `mpz80.c` and `mmu_test.c` report every
user memory protection fault as `0xf7` (RI10 set), so the kernel's
`trap()` dispatches it to `fault()`.  The manual's `0x3e`/`0x7e`/
`0xbe`/`0xfe` forms are available as a CPLD build option, but the
default matches the emulator and the running kernel.

### 3.6 The trap address register

`0x400` read returns `{current_page, previous_page}`.  The hardware
shifts `A15–A12` in on every mapped memory reference, read or write:

```
trapreg = (address[15:12] << 4) | (trapreg >> 4)
```

The shift is frozen during the trap window.  On a memory fault the
current value is latched so the firmware sequence cannot destroy it.
The kernel's `fault()` reads the high nibble as the faulting page and
the low nibble as the previous page.

### 3.7 User halt substitution

A user M1 fetch of `0x76` must not halt the CPU.  The board watches the
data bus during the fetch, and if the byte is `0x76` in a user task
with mask bit 4 set, it disables the S-100 DI buffer and drives `0x00`
to the Z180 instead.  The CPU sees a nop; the trap window starts on the
next read, at the halt address plus one.  The cause is halt and the
status is `0xbb`.

The comparison and substitution need the S-100 read-to-Z180 path to be
wait-stated.  U2 stretches the M1 read with `/WAIT` until the data is
stable; U1 makes the halt decision and supplies the `0x00`.  This is the one place
where the S-100 memory access time is part of a decode decision rather
than just a bus wait.

### 3.8 Interrupts

The S-100 VI0–VI7 lines are open collector.  The board ORs them into
the Z180's `/INT0`.  The mask decides:

- user task, TINT high: do not pass the interrupt to the Z180; latch an
  interrupt trap instead.
- user task, TINT low: pass it to `/INT0`.
- task 0, SINT low: block it.
- task 0, SINT high: pass it to `/INT0`.

The kernel's 8259 is on an S-100 board.  During the Z180 interrupt
acknowledge U2 drives S-100 `sINTA`, and the 8259 supplies its
three-byte `call` vector.  The kernel's `trap()` does not dispatch an
interrupt cause; it enables interrupts and lets the 8259 vector through
the table at `0x200`.

### 3.9 I/O traps

A user `IN`/`OUT` with mask bit 6 high is trapped.  The board must not
perform the S-100 cycle:

- `IN` returns `0xff` to the CPU.
- `OUT` discards the byte.
- The status is `0x3c` or `0xbc`.

The Z180's internal I/O is the exception: a user access to the internal
block (relocated to `0x80–0xbf`) is executed inside the Z180 and never
reaches the CPLD, so it cannot be trapped.  Section 5.8 records this as
a known compatibility limit.

### 3.10 Task switch delay

Writing `0x402` starts an 8-M1-cycle countdown.  Traps and interrupts
are inhibited while it runs, and the old task's map remains selected
until the final M1, when the new task's map is used for that fetch.
This matches `mpz80.c`; the manual says "seven instruction fetches",
but its own return sequence includes an `ED`-prefixed instruction
(`ld sp,(0ah)`) that takes two M1 cycles, so the emulator counts eight.

The monitor's canonical sequence is

```
ld a,task
ld (402h),a
six nops
jp destination
```

The kernel's return to user mode has the same shape, with `ld sp,(0ah)`
between the write and the jump.

### 3.11 Deferred memory faults

A user memory protection violation blocks the read or write, latches the
faulting page pair, and traps at the instruction boundary.  A RI10 hit
completes the access and then traps.  The hardware does not attempt a
precise restartable fault; the kernel's grow-on-reference model and
`mmu_test.c` were written for the deferred behavior.

### 3.12 Timing summary

```
trap event
  -> status/trap address latched, task register = 0, upper ROM half
  -> first forced read at CPU's PC (or PC+1 for halt)
  -> 15 forced reads from flash 0xbf0 + offset
  -> CPU executes call trappd at 0xbfc, leaves the window
  -> trappd builds the 14-word frame and calls super (0x0000)
  -> super -> jp 0x1000 -> _trapvec
```

## 4. Task model

### 4.1 Sixteen hardware tasks

Task 0 is the supervisor and the only task that can see the on-board
page.  Tasks 1–15 are user tasks.  Each has a 32-byte map table in the
external map SRAM.

The kernel's `struct proc` has a `task` field (1–15, or 0 for none) and
`mem[17]`; the map RAM is a cache of `mem[0..15]`, loaded by
`taskload()`.  When all fifteen slots are in use, `taskalloc()` takes
one from the stalest process.  The kernel never switches the hardware
task register during a process switch; it writes the saved task/mask
word through the firmware's `gotask` on return to user mode.

### 4.2 Trap frame

The firmware pushes fourteen words before calling the kernel.  They land
at offsets 4–31 of `struct user`:

| offset | field |
|---|---|
| 4 | task |
| 5 | mask |
| 6 | pc |
| 8 | sp |
| 10 | af |
| 12 | bc |
| 14 | de |
| 16 | hl |
| 18 | I + flags |
| 20 | ix |
| 22 | iy |
| 24 | af' |
| 26 | bc' |
| 28 | de' |
| 30 | hl' |

The kernel sets `trapstack = &u.save` so future frames land in the same
place.  This layout is part of the hardware/firmware contract and must
not change.

### 4.3 Task register high nibble

The high nibble is latched but not an address field.  It is tempting to
use it as four context bits (an address-space tag, a process generation,
or a second task field).  The document recommends reserving it in rev A
for these reasons:

- any write to `0x402` starts the 8-M1 delay, so a context write is not
  free;
- a trap clears the whole task register, so context must be saved in a
  shadow or the trap frame;
- `0x402` reads the switch register, so the high nibble cannot be read
  back;
- the monitor's `ctask` byte and the kernel's `u.task` carry only the
  low nibble today;
- the selector boot currently uses `0xF0` as a bank and is being
  rewritten anyway.

If it is used later, the trap entry must save the old high nibble
before clearing the register, and `gotask` must restore it with the
task/mask word.

### 4.4 Selector boot rewrite

The old selector boot maps page 1 to segment `0xF0`, then banks the task
register to `0xF0` to reach physical `0xFF0000`.  Because writing the
task register also drops the boot ROM, the banking loop cannot live in
ROM: the monitor copies a loop into on-board RAM and reads the selector
through a 256-byte bounce buffer, 256 bytes at a time.

With A20–A23 in the PTE, this machinery is unnecessary.  The selector
ROM is reached by an ordinary map entry:

```
task 0, page 1:  PTE low 0xF0, high 0xF3  ->  physical 0xFF0000
```

The rewrite runs from ROM, writes that entry (and the map image at
`0x200`), and either jumps to `0x1000` if the selector can run in place
or copies 4K to a second mapped page and enters it there.  No task
register bank, no RAM trampoline, and no bounce buffer remain.  The
task register write that selected the upper ROM half is still available
for later traps; the selector path no longer depends on it.

## 5. Z180 integration

### 5.1 Part and clock

The recommended CPU is the **Z8S180** in the 64-pin DIP
(`Z8S18010PSG`) for through-hole assembly, with the 68-pin PLCC
(`Z8S18010VSC`) in a through-hole PLCC socket as the alternate.  The
DIP package omits A19, which this board does not use; the external MMU
supplies A19 from the PTE.  A 10.000 MHz oscillator is the clock; U2
divides it by five for the 2.000 MHz S-100 bus clock.  The Z180's
internal clock multiplier and refresh are not used.

### 5.2 Internal MMU transparent

The Z180's MMU is left in its reset pass-through state: logical
addresses 0x0000–0xffff map to physical 0x00000–0x0ffff, and A16–A19
are driven low.  The board does not connect A16–A19; the external MMU
uses A12–A15 as the page index and A0–A11 as the offset.  The monitor's
`z180init` does not reprogram CBAR/CBR/BBR; if it touches them at all,
it restores the pass-through values.

### 5.3 Internal I/O relocation

The Z180's internal I/O block resets at 0x00–0x3f.  That conflicts with
the monitor's console group ports (0x09–0x0b) and the IDE card
(0x30–0x34).  The reset window writes ICR (port 0x3f) with `0x80`,
moving the internal block to 0x80–0xbf.  Ports 0x09–0x0b and 0x30–0x34
then reach the S-100 bus.

The internal block at 0x80–0xbf is reserved.  No Micronix driver uses
it.  A user `IN`/`OUT` there is executed inside the Z180 and cannot be
trapped; section 5.8.

### 5.4 Reset and z180init

The reset window's 15 bytes perform the ICR relocation before the
monitor touches any I/O.  After the window, `z180init` (called from
`reset0`, before `uartst`) can set the Z180 wait-state control and
verify the MMU registers.  It does not enable any Z180 peripheral.

### 5.5 Wait states and bus timing

The Z180 runs at 10 MHz, but the S-100 bus is asynchronous.  The
bus-interface CPLD (U2) drives the S-100 cycle and holds the Z180 in
`/WAIT` until the slave is ready.  Local memory is fast enough for a
single 10 MHz cycle; the CPLD can insert a wait state if a slower part
is fitted.  The map lookup is pipelined (section 6.7).  U2 also drives
S-100 `pWAIT` while it is stretching a cycle.

### 5.6 DMA

The Z180's `/BUSRQ` is connected to S-100 `pHOLD`, and `/BUSAK` to
`pHLDA`.  When a temporary master owns the bus, U2 disables the S-100
address, data and control buffers and the Z180 tri-states its bus.  The
Z180's internal DMA is not used.

### 5.7 Interrupts and NMI

S-100 pINT and VI0–VI7 are ORed into `/INT0`; S-100 NMI goes to `/NMI`.
The mask logic of section 3.8 gates user and supervisor interrupts.
The 8259 vector path is unchanged.

### 5.8 Unused peripherals and the I/O trap leak

The Z180's on-board ASCI/UARTs, PRT, CSI/O, DMA and internal MMU are
ignored.  The console and serial lines are on the S-100 MultIO or SS1
boards; there is no on-board UART header or driver.  The
internal I/O block at 0x80–0xbf remains decoded inside the Z180.  A
user program that executes `IN`/`OUT` to that block bypasses the CPLD's
I/O trap.  This is a deliberate, documented limitation; the kernel's
userland does not use those ports.  If it matters, the monitor can
relocate the internal block to 0xc0–0xff and accept the conflict with
the disk controllers instead—neither choice is free.

## 6. CPLD partition and design

### 6.1 Two 84-pin devices

The board uses two **ATF1508AS-10** CPLDs in 84-pin PLCC packages, in
through-hole PLCC sockets:

- **U1** — sequencer **and** translation: trap/task/local, permission
  check, map RAM and address-driver control.
- **U2** — S-100 bus interface: wait states, buffer enables, DMA,
  interrupts, clock/reset, switch register.

The merge is possible because U1 and the old translation device shared
CLK, RESET and the map path: once the permission nibble and map `/OE`
move into U1, the old inter-CPLD translation bus disappears and the
merged device is 56 pins.

```
Z180 A12-A15 --------------------+
TASK[3:0] from U1 ----------------+--> 74AHCT157 map address mux
CPU A8-A1 (map write) ------------+          |
MAP_WE from U1 ------------------/           v
                                        map RAM A/B
                                             |
                     +-----------------------+--------------------+
                     |                       |                    |
              A12-A19 direct           A20-A23 direct      permission nibble
                     |                       |                    |
                     v                       v                    v
                 74ABT541                74ABT541                U1
                     |                       |                    |
                     +-----> S-100 A12-A23 --+                    |
                                                                  |
        +-----+------------------+                                |
        |  U1                    |<-------------------------------+
        | sequencer +            |
        | translation            |
        +-----+------------------+
              |
        +-----+------------------+
        |  U2                    |
        |  S-100 bus interface   |
        |  wait/control/DMA/IRQ  |
        +-----+------------------+
              |
              +--> 74ABT541/244 --> S-100 control/data
```

No CPLD drives the S-100 edge.  The ATF1508AS I/O does not meet the
S-100 drive requirement, so every S-100 output goes through a 74ABT or
74F high-drive buffer; the CPLDs drive only the buffer inputs and the
buffer output enables.  S-100 inputs come back through 74AHCT244
receivers.

The map RAM address mux is external: the page nibble A15–A12 comes
directly from the Z180, the task nibble from U1's task register, and
the CPU's A8–A1 during a map write.  U1 is not in the map address path.
The map RAM **data outputs do not enter any CPLD**: A12–A19 and
A20–A23 go straight to the address line drivers, and only the four
permission bits go to U1.

### 6.2 U1: sequencer + translation

U1 owns the CPU side, the local page, and the permission check:

- register decode for 0x400–0x403 and the local page;
- task register, 8-M1 switch delay, mask/status;
- 15-read trap window, user-halt data substitution, flash address
  counter control;
- on-board flash region: `FLASH_PHYS` from the A23–A20 = F comparator;
  selects the physical flash address mux, asserts LOCAL to U2 so the
  S-100 drivers stay off, and drives the flash `/CE` and `/OE`;
- trap-address shift register: this is the **only** use of A12–A15.
  The local decode uses SEG0/TASK0 and A11–A0, and the trap window is a
  read counter, not an address comparator;
- task nibble to the map address mux; MAP_WE to the mux and the
  external `/WE` gate;
- reads the four permission bits from map RAM B; permission and RI10
  checks; controls the map RAM `/OE` and the A12–A23 address-driver
  `/OE`;
- local SRAM/flash chip selects and the trap data buffer;
- inter-CPLD signals to U2.

### 6.3 U2: S-100 bus interface

U2 owns the bus side:

- S-100 memory/I/O cycle state machine, pSYNC/pSTVAL/pDBIN/pWR,
  pWAIT/pRDY;
- S-100 address/data/control buffer enables;
- interrupt gating, NMI, and sINTA;
- DMA tri-state (`pHOLD`/`pHILDA`/Z180 `/BUSRQ`/`/BUSAK`);
- clock divider and reset;
- the switch-register buffer control.

It drives only high-drive buffer inputs and enables.  The CPLDs never
drive the S-100 edge.

### 6.4 Inter-CPLD bus

| signal | direction | pins | meaning |
|---|---|---|---|
| LOCAL, TRAP | U1 → U2 | 2 | bus cycle is local/trapping |
| STOP, AUX | U2 → U1 | 2 | panel trap causes |
| BUS_GRANT | U2 → U1 | 1 | tri-state A12–A23 buffer |
| **total** | | **5** | |

U1 owns the trap-address shift register; the 8-bit page pair does not
cross between devices.  U1 also owns the permission check and the map
RAM controls; U2 is purely the S-100 bus interface.

### 6.5 Pin budget

The 84-pin PLCC exposes roughly 64 user I/O; JTAG pins are dedicated.
These are the functional counts; the exact pin numbers are assigned in
the fitter.  The budget uses external 74AHCT logic to keep the CPLD
counts down: a 74AHCT138 local decode (enabled by SEG0 and TASK0; U1
receives its select lines), a SEG0 zero-detect on RAM A's output, a
74AHCT157 flash-address mux, a 74AHCT157 map-address mux, a
74AHCT161/163 trap-window counter, a 74AHCT541 switch register, and a
74AHCT30/688 comparator that detects A23–A20 = F for the on-board flash
region.  The flash-address mux selects physical A0–A19 for
F00000–FFFFFF, the local alias address, or the trap counter.  The CPLDs
do not spend pins on the 19 flash address lines, the map RAM address
lines, or the 8 DIP switches.

**U1 — sequencer + translation (58 pins)**

| group | pins |
|---|---|
| Z180 A0–A3, A12–A15 | 8 |
| local decode selects (RAM/REGS/MAP/ROM/FPU) + task nibble to map address mux | 9 |
| Z180 D0–D7 | 8 |
| Z180 /M1, /MREQ, /IORQ, /RD, /WR, /HALT, CLK | 7 |
| global reset in | 1 |
| permission bits from map RAM B | 4 |
| BUS_GRANT from U2 | 1 |
| inter-CPLD U2 (LOCAL/TRAP out; STOP/AUX in) | 4 |
| map RAM `/OE` | 2 |
| MAP_WE to map mux and `/WE` gate | 1 |
| A12–A23 address-driver `/OE` | 1 |
| flash counter /LOAD, /CLR, COUNT | 3 |
| flash half select | 1 |
| FLASH_PHYS from A23–A20 = F comparator | 1 |
| FLASH_MUX / TRAP_WIN to flash address mux | 1 |
| flash /CE, /OE; local SRAM /OE, /WE | 4 |
| trap data buffer /OE and 0x00/flash mux | 2 |
| **total** | **58** |

The four A12–A15 pins are solely the trap-address shift register: the
local decode is SEG0/TASK0 plus A11–A0, and the trap window is a
15-read counter.  The task nibble goes directly to the map address mux.
The map RAM `/CE` is tied active; the `/WE` low/high are generated
externally from MAP_WE and CPU A0.  U1 has six spare pins.

**U2 — S-100 bus interface (46 pins)**

| group | pins |
|---|---|
| Z180 /MREQ, /IORQ, /RD, /WR, /M1, /BUSAK | 6 |
| CLK, /RESET | 2 |
| S-100 inputs via 74AHCT244: pRDY, pHOLD, pRESET, pPOC, NMI, pINT, VI_OR | 7 |
| panel STOP, AUX | 2 |
| inter-CPLD U1 (LOCAL, TRAP in; STOP, AUX out) | 4 |
| S-100 control buffer inputs (pSYNC, pSTVAL, pDBIN, pWR, sMEMR, sMEMW, sINP, sOUT, sM1, sINTA, pWAIT) | 11 |
| buffer enables: A0–A11 /OE, DO /OE, DI /OE, control /OE | 4 |
| BUS_GRANT to U1 | 1 |
| S-100 clock outputs | 2 |
| pHLDA buffer input | 1 |
| Z180 /WAIT, /NMI, /INT0, /RESET | 4 |
| INT PEND, TRAP RESET to switch buffer | 2 |
| **total** | **46** |

U2 has eighteen spare pins for the front-panel display/keyboard, a
debug header, or a future second VI input.  The switch register is an
external 74AHCT541 whose inputs are the DIP switches and U2's INT
PEND/TRAP RESET bits; U1 enables it on a 0x402 read.

Two 84-pin devices are sufficient: U1 58 and U2 46, with 6 and 18 spare.


### 6.6 Register decode

The local page is selected by task 0 **and** segment 0.  A zero-detect
comparator on RAM A's A19–A12 output produces SEG0; a 74AHCT138 enables
on TASK0 and SEG0 and decodes the low address lines to select the
on-board device:

```
0x000-0x3ff  -> local RAM
0x400-0x403  -> registers
0x600-0x7ff  -> map RAM write; map address mux selects CPU A8-A1
0x800-0xbff  -> flash monitor window
0xc00-0xfff  -> optional FPU, otherwise open bus
```

Within the register block:

```
0x400 read   -> trap address shift register
0x400 write  -> front-panel segment (ignored if no panel)
0x401 read   -> keyboard (0xff if no panel)
0x401 write  -> front-panel column (ignored)
0x402 read   -> switch register
0x402 write  -> task register; start 8-M1 delay; select upper ROM half
0x403 read   -> trap status
0x403 write  -> mask register
```

A local access is not an S-100 cycle.  The A0–A11 and A12–A23 address
drivers are disabled, the DO/DI data buffers are disabled, and the
control outputs are inactive.  `NULL BUS`/local mode behaves exactly as
on the MPZ80.

### 6.7 Map RAM interface

Two 5 V 32K×8 10 ns SRAMs (IS61C256AH-10 baseline):

- **RAM A (even byte):** output A19–A12 drives the 74ABT A12–A19 line
  driver directly.
- **RAM B (odd byte):** output high nibble A23–A20 drives the 74ABT
  A20–A23 line driver directly; output low nibble is the four
  permission bits and is the only map data that enters U1.

RAM A's output also feeds the SEG0 zero-detect; SEG0 and TASK0 select
the local page and enable the 74AHCT138 low-address decode.

The map RAM outputs are the S-100 address.  No address bit passes
through a CPLD, and the map lookup is one 10 ns RAM access plus the
74ABT driver.  At 10 MHz (a 100 ns cycle), 10 ns RAM plus 7.5 ns CPLD
is 17.5 ns, leaving ample margin; the S-100 cycle is wait-stated
anyway.

There is **no A12–A23 output mux**.  The 74ABT parts are line drivers
(buffers) that meet the S-100 drive requirement; U1 controls only their
`/OE`.  The only mux in this path is the RAM **address input** mux
described below, which selects between the translation address and the
CPU's map-write address because the RAM is single-port.

The RAM address is 8 bits, `{task[3:0], page[3:0]}`, from a 74AHCT157
mux.  During translation its high nibble is U1's task register and its
low nibble is A15–A12 directly from the Z180; during a map write the
mux selects the CPU's A8–A1.  CPU A0 selects RAM A or RAM B for the
write; U1 supplies `MAP_WE`, and a small gate fans `MAP_WE` and A0 out
to the two `/WE` pins.  U1 controls the RAM `/OE`.  The RAM data inputs
are the CPU's D0–D7 directly.  The map is write-only to the CPU; reads
return open bus.

U1 is not in the map address path.  The page nibble A12–A15 and the
task nibble condition the map RAM address lines directly through the
mux.  The permission check is a separate path: RAM B's low nibble goes
to U1, which compares it with the access type and raises FAULT/RI10.
The address has already reached the driver by then; a violating access
is blocked at the data bus, not at the address bus.

### 6.8 S-100 address path

Memory cycle, non-local:

```
S-100 A[11:0]  = Z180 A[11:0]
S-100 A[19:12] = RAM A output
S-100 A[23:20] = RAM B high nibble
```

Local cycle (task 0, segment 0): no S-100 cycle.  The A0–A11 and
A12–A23 address drivers are disabled, the DO/DI data buffers are
disabled, and the control outputs are inactive.  The low A11–A0 decode
selects the on-board RAM, registers, map RAM or ROM on the Z180 data
bus.

On-board flash region: if the physical address is F00000–FFFFFF, the
access is local.  The flash address mux selects A0–A19, the flash data
returns on the Z180 data bus, and the S-100 address/data drivers stay
off.  The region is never an S-100 cycle.

I/O cycle:

```
S-100 A[7:0]  = Z180 A[7:0]
S-100 A[15:8] = mask[7] ? Z180 A[15:8] : 0
S-100 A[23:16]= drivers disabled, pulled low
```

The ZIO bit reproduces the MPZ80's 8080-style dual port address.  S-100
cards that decode only A0–A7 ignore the upper bytes.

### 6.9 Trap state machine

States: `IDLE`, `WINDOW`, `DONE`.

```
IDLE:
  on trap:
    latch status, trap address
    taskreg <= 0
    rom_half <= upper
    window_count <= 15
    state <= WINDOW
WINDOW:
  on memory read:
    force flash address 0xbf0 + (15 - window_count)
    window_count--
    if window_count == 0:
      state <= DONE
DONE:
  normal execution; next trap restarts
```

The reset window uses the lower ROM half and the same 15-read counter.
There is no address comparison: the sequence is linear, and the 15th
read is the last byte before the `call`/`jp` target.

### 6.10 Bus-cycle state machine

U2 owns the S-100 cycle.  It drives the buffer inputs; the external
74ABT buffers drive the bus.

```
IDLE -> ADDR -> (READ | WRITE) -> WAIT -> DONE -> IDLE
```

- `ADDR`: drive S-100 address, pSYNC, pSTVAL.
- `READ`: assert pDBIN/sMEMR or sINP; sample pRDY.
- `WRITE`: drive DO, assert pWR/sMEMW or sOUT.
- `WAIT`: hold Z180 /WAIT and S-100 pWAIT until the slave is ready.
- `DONE`: release /WAIT and the strobes.

Interrupt acknowledge is a separate path: M1 + IORQ -> sINTA, and the
8259 drives the vector on DI.  DMA grant tri-states all of the above.

### 6.11 Interrupts, DMA, clock, reset

U2 owns these bus-side functions:

- pINT and VI0–VI7 are ORed externally; the mask selects trap vs
  `/INT0`.
- NMI passes to `/NMI`.
- `pHOLD` -> Z180 `/BUSRQ`; `/BUSAK` -> `pHLDA`; U2 disables all S-100
  buffer enables while the bus is granted.
- 10.000 MHz input; divide by five for the 2.000 MHz S-100 clock.
- `pRESET`/`pPOC` and the reset button drive the Z180 `/RESET` and the
  CPLD reset; reset clears the task register, mask, trap window, and
  ROM-half latch.

### 6.12 JTAG

The two CPLDs are daisy-chained on one 10-pin JTAG header.  The chain
is programmed before the Z180 is allowed to run; the reset circuit holds
the CPU in reset until the CPLDs are configured.

### 6.13 Key equations

Pseudo-Verilog for the translation device:

```verilog
wire [15:0] pte = {sram_hi, sram_lo};
wire [23:0] paddr = {pte[15:12], pte[7:0], a[11:0]};

wire exec = m1;
wire read = !wr;
wire perm_ok =
    exec ? (pte[9] | pte[8]) :
    read ? pte[8] :
           (pte[8] & pte[9]);

wire fault = !super && !local && !perm_ok;
wire ri10  = !super && !local && pte[10];
```

U1 owns the trap-address shift register:

```verilog
trapreg <= {a[15:12], trapreg[7:4]};   // on each mapped access
```

The sequencer's window and delay are counters with the state tables
above.  The exact equations will be written when the fitter partition
is fixed.

## 7. S-100 interface

### 7.1 Signals used

The board is a standard IEEE-696 bus master.  Address and data pin
numbers from the IEEE-696 connector:

| signal | pin | signal | pin |
|---|---|---|---|
| A0 | 79 | A12 | 33 |
| A1 | 80 | A13 | 85 |
| A2 | 81 | A14 | 86 |
| A3 | 31 | A15 | 32 |
| A4 | 30 | A16 | 16 |
| A5 | 29 | A17 | 17 |
| A6 | 82 | A18 | 15 |
| A7 | 83 | A19 | 59 |
| A8 | 84 | A20 | 61 |
| A9 | 34 | A21 | 62 |
| A10 | 37 | A22 | 63 |
| A11 | 87 | A23 | 64 |

Data:

| signal | pin | signal | pin |
|---|---|---|---|
| DI0 | 95 | DO0 | 36 |
| DI1 | 94 | DO1 | 35 |
| DI2 | 41 | DO2 | 88 |
| DI3 | 42 | DO3 | 89 |
| DI4 | 91 | DO4 | 38 |
| DI5 | 92 | DO5 | 39 |
| DI6 | 93 | DO6 | 40 |
| DI7 | 43 | DO7 | 90 |

The control group is named, not pin-numbered here, because the
IEEE-696 connector interleaves address and control pins and the MPZ80
schematic must be the final check:

| signal | direction | use |
|---|---|---|
| pSYNC | out | start of a bus cycle |
| pSTVAL | out | status valid |
| pDBIN | out | read data strobe |
| pWR | out | write strobe |
| sMEMR | out | memory read |
| sMEMW / sWO | out | memory write |
| sINP | out | I/O read |
| sOUT | out | I/O write |
| sM1 | out | instruction fetch |
| sINTA | out | interrupt acknowledge |
| pWAIT | out | master wait |
| pRDY | in | slave ready |
| pHOLD | in | temporary-master request |
| pHLDA | out | hold acknowledge |
| pRESET | in | reset |
| pPOC | in | power-on clear |
| NMI | in | non-maskable interrupt |
| pINT | in | interrupt request |
| VI0–VI7 | in | vectored interrupts |

Pin numbers for this group must be taken from the IEEE-696 connector
table and verified against the MPZ80 schematic before layout.

### 7.2 Drive requirement, buffers and data paths

The ATF1508AS cannot drive the S-100 edge: its I/O does not meet the
IEEE-696 sink/source and termination requirements.  Every S-100 output
goes through a **74ABT or 74F** high-drive buffer; the CPLDs drive only
the buffer inputs and the buffer output enables.  S-100 inputs come back
through 74AHCT244 receivers.

- A0–A11: Z180 -> 74ABT245/541 -> S-100, /OE from U2.
- A12–A19: map RAM A -> 74ABT541 -> S-100, /OE from U1.
- A20–A23: map RAM B high nibble -> 74ABT541 -> S-100, /OE from U1.
- DO0–DO7: Z180 -> 74ABT541 -> S-100, enabled on writes by U2.
- DI0–DI7: S-100 -> 74AHCT244 -> Z180, enabled on reads by U2.
- Control outputs: U2 -> 74ABT541 -> S-100, common /OE from U2.
- Control/status inputs: S-100 -> 74AHCT244 -> U2, /OE tied low.
- Trap/flash data: a separate 74AHCT245 path drives the Z180 data bus
  during the trap window and the user-halt nop substitution; the S-100
  DI buffer is disabled then.
- Local SRAM/flash data: the same Z180 data bus, selected by U1/U2.
  For a local access U2 and U1 disable the S-100 address and data
  buffers; only the on-board devices see the cycle.  This includes the
  F00000–FFFFFF flash region; its data never reaches the S-100 bus.
- Clocks: U2 -> 74ABT541/clock buffer -> S-100.

The 74ABT/74F parts are the current-drive elements.  The CPLD pins that
appear in section 6.5 are buffer inputs and enables, not bus pins.

### 7.3 Memory read/write timing

The Z180's bus cycle starts the S-100 cycle.  U2 drives the buffer
inputs for address and status, waits for the slave, and only then
releases `/WAIT`.  The S-100 `pRDY` input and U2's wait counter
determine the length.
The map lookup is complete before the S-100 address buffers are
enabled, so it is not part of the slave's setup time.

### 7.4 I/O cycles and ZIO

The S-100 I/O address is A0–A7.  With mask bit 7 high, the board passes
the Z180's A8–A15 through; with it low, A8–A15 are zero.  For 8080-style
`IN A,(n)` the Z180 puts the port byte on both halves, so the high half
then matches the low byte; for `IN r,(C)` it carries B.  A16–A23 are
zero.  User I/O traps and the Z180 internal block are described in
sections 3.9 and 5.8.

### 7.5 Wait states

U2 holds `/WAIT` and `pWAIT` until the S-100 slave answers or the
fixed timeout expires.  At 10 MHz the local map/SRAM path has ample
margin; the CPLD can still insert a wait state for a slower part.  The
Z180's own wait-state generator is reserved for the local devices.

### 7.6 Bus arbitration

`pHOLD` from a temporary master goes to the Z180 `/BUSRQ`; the Z180
`/BUSAK` goes to `pHLDA`.  U2 disables all S-100 buffer enables while
the bus is granted.  The DMA controller's address and data path is
straight to the bus; the CPU board is a slave during that time.

### 7.7 Interrupts and NMI

pINT and VI0–VI7 are wired-OR into `/INT0`; NMI is direct.  The 8259
on the MultIO/SS1 board drives its interrupt line; the CPU board does
not contain the PIC.  The mask and trap behavior are in section 3.8.

### 7.8 Clock and reset

U2 divides the 10.000 MHz oscillator to a 2.000 MHz S-100 clock and
drives the bus clock buffer.  Reset comes from S-100 pRESET/pPOC and
the on-board button; it clears the CPLDs and holds the Z180 in reset
until the CPLDs are configured.

### 7.9 Pin verification

The address/data pin numbers are the IEEE-696 assignments.  The
non-monotonic A18/A16/A17 order and the interleaving of A19–A23 with
SIXTN/TMA must be checked against the backplane and the MPZ80 schematic
before the edge connector is committed.

## 8. Memory and boot firmware

### 8.1 Local memory map

| device | size | address | use |
|---|---|---|---|
| local SRAM | 2K×8 (1K used) | 0x000–0x3ff | MPZ80 low page |
| map RAM A/B | 2× 32K×8 (512 B used) | 0x600–0x7ff | A12–A19 / A20–A23 + perms |
| monitor flash | 1M×8 | F00000–FFFFFF; 0x800–0xbff alias | flash region + monitor alias |
| optional FPU | — | 0xc00–0xfff | not fitted |

The local SRAM needs only 1 KB; a 2K×8 or 8K×8 5 V SRAM is fine.  The
map is two 10 ns 32K×8 parts; their address outputs drive the A12–A23
line drivers directly, and only the four permission bits go to U1
(section 6.7).  The flash is a 1M×8 5 V parallel part (SST39SF080 or
equivalent) that fills F00000–FFFFFF.

### 8.2 Flash layout

The 1 MB flash is both the top physical megabyte and the source of the
monitor alias:

```
flash offset        physical            use
0x00000-0x007FF     F00000-F007FF       monitor image (two 1K halves)
0x00800-0xEFFFF     F00800-FEFFFF       boot images, diagnostics, free
0xF0000-0xFFFFF     FF0000-FFFFFF       64K selector/boot image
```

Each monitor image is 2 KB: a lower 1 KB reset half and an upper 1 KB
trap half.  A 4-bit image-select DIP chooses the 2K image aliased at
`0x800–0xbff`; the region at F00000–FFFFFF exposes every image without
the alias.  The CPLD selects the half: the lower half is visible after
reset until a task-register write, then the upper half.  U1 controls
the half select; the trap window forces the flash address to
`0xbf0 + counter` in the selected half.  The selector at `0xFF0000`
is the top 64K of the same flash, so the selector PTE `0xF3F0` still
works.

### 8.3 Reset window patch

The reset half's 15 bytes at `0xbf0` become:

```
3e 80        ld a,80h
d3 3f        out (3fh),a      ; ICR = 0x80: internal I/O -> 0x80-0xbf
af           xor a
32 02 06     ld (0602h),a     ; task 0 page 1 segment = 0
3c           inc a
31 00 02     ld sp,0200h
c3 00 08     jp getsw
```

This is exactly 15 bytes.  The `out` is the last internal-I/O access at
0x3f; after it, ports 0x09–0x0b and 0x30–0x34 are external.  The
`ld (0602h)` writes the map SRAM; the `jp getsw` leaves the reset
window and enters the monitor.

### 8.4 z180init

`reset0` calls `z180init` before `uartst`.  It sets the Z180 wait-state
control for local memory and leaves the internal MMU in pass-through.
It ignores the Z180's on-board ASCI/UARTs, PRT, CSI/O and DMA.  The
internal block is
already at 0x80–0xbf from the reset window.

### 8.5 Selector boot rewrite

Old path: map page 1 to segment 0xF0; copy a banking loop into on-board
RAM; bank the task register to 0xF0; read 256 bytes through a bounce
buffer; bank back; write to physical 0x0F000; repeat 16 times; enter.

New path:

1. Map task 0 page 1 to `0xF3F0` (physical 0xFF0000), or map `0xF300`
   (physical F00000) and read the selector at flash offset `0xF0000`.
   Both are views of the same 1 MB flash region.
2. Update the monitor's map image at `0x200`.
3. Either jump to `0x1000` and run the selector in place, or map a
   second page to the destination and copy 4K there with a plain
   `ldir`.
4. Enter the selector.

The rewrite runs from ROM because no task-register write is needed.
The RAM trampoline (`ramcode`) and the bounce buffer are deleted.  If
the selector requires the old physical destination, use the copy
variant and jump to the mapped destination; the map entry is the same
12-bit value.  The map write itself is just:

```
ld a,0f0h
ld (0602h),a        ; task 0 page 1, A19-A12
ld (0202h),a        ; keep the monitor image in step
ld a,0f3h
ld (0603h),a        ; A23-A20 = F, FULL
ld (0203h),a
jp 1000h            ; or the selector's link address
```

### 8.6 Map image and map writes

The monitor's `rstmap`/`putmap` routines take an allocation byte and an
access byte.  The access byte now carries the bank in its high nibble:

```
bank  = A23-A20
access = (bank << 4) | (RI10 << 2) | (execute << 1) | read
```

The monitor image at `0x200` stores the same two bytes per page.  The
kernel's `taskload()` copies `p->mem[]` (8-bit segment + permission
byte) into the map; the kernel's high nibbles are zero, so it stays
below 1 MB.

### 8.7 Kernel compatibility

No kernel change is required for the current 1 MB kernel:

- `MAXSEG` is 256 (1 MB) and `struct mem.seg` is 8 bits;
- every kernel PTE high nibble is zero;
- the trap address, map layout, permissions and trap frame are
  unchanged;
- the monitor's selector boot is the only structural rewrite; the reset
  window also needs the 15-byte ICR patch of section 8.3.

A future kernel that wants more than 1 MB must widen `struct mem.seg`
(or add a bank byte) and write the PTE high nibble; the hardware already
supports it.

## 9. Power, BOM, physical layout, bring-up

### 9.1 Power

S-100 +8 V unregulated on pins 1 and 51 feeds a 5 V buck regulator (or
a low-drop linear with a heatsink) at about 500 mA.  The +16 V and
−16 V rails are not used.  Decouple each IC with 0.1 µF and add 10 µF
bulk near the regulator.  Keep the map SRAM and CPLDs on a ground plane.

### 9.2 BOM

| qty | part | use |
|---|---|---|
| 1 | Z8S18010PSG (DIP-64) or Z8S18010VSC (PLCC-68 socket) | CPU, 10 MHz |
| 2 | ATF1508AS-10JI84 in PLCC-84 socket | sequencer+translation, bus |
| 2 | IS61C256AH-10 or AS7C256-10 (32K×8, 5 V, 10 ns) | map RAM A/B |
| 1 | AS6C6264-55 or IS61C256AH-10 | local SRAM |
| 1 | SST39SF080 (1M×8, 5 V) or equivalent | monitor flash |
| 1 | 10.000 MHz 5 V oscillator | CPU/bus clock |
| 1 | 5 V regulator | +8 V -> +5 V |
| — | 74ABT245/541/244 | S-100 high-drive output buffers |
| — | 74AHCT244/245/541/138/157/161/163, 74AHCT30/688 | S-100 inputs, local decode/counter, flash-region compare |
| — | PLCC-84 sockets, DIP-64 socket, JTAG header, reset button, 8-way DIP | assembly |
| — | 0.1 µF, 10 µF, pull-ups, series resistors | decoupling/termination |

There are no on-board UART/ASCI parts in the BOM.  The console and
serial lines are on the S-100 MultIO or SS1 board; the Z180's on-board
ASCIs are ignored.

Part availability and exact speed grades must be checked before the
board is ordered.

### 9.3 Physical layout

Standard 254 × 127 mm S-100 outline, 100-pin edge tongue along the
bottom.  Place the Z180 and the two CPLDs in the center, the two map
RAMs immediately beside the A12–A23 address drivers, the flash and
local SRAM beside U1, and the bus buffers along the edge.  Put the JTAG header, reset button,
image-select DIP and stop/aux/NMI header at the top.  There is no
on-board UART/ASCI header; the console is an S-100 MultIO or SS1 card.  Use through-hole PLCC sockets for the CPLDs and a DIP socket
for the Z180 if the DIP part is used.

The front-panel display/keyboard, the STOP and AUX inputs, and their
inter-CPLD signals are optional.  If pin pruning is needed later,
dropping them frees four U2 pins (PANEL_STOP, PANEL_AUX, STOP_U1,
AUX_U1) and two U1 pins (STOP, AUX), and removes the STOP/AUX trap
sources.  Reset, halt, I/O, interrupt and protection/RI10 remain.  The
0x402 boot switch register and the 0x400 trap-address read are not
optional — 0x400 read is the trap-address register, not front-panel
display.

### 9.4 Bring-up

1. Power rails: +5 V, current, no hot parts.
2. Clock and reset: Z180 clock, CPLD configuration, reset released.
3. JTAG: both CPLDs in the chain, IDs read.
4. Local memory: read/write the 1 KB page, the map SRAM bytes.
5. Map/translation: write a PTE, read a known S-100 memory location,
   check A12–A23.
6. Trap window: force a reset, scope the 15 reads from flash 0xbf0.
7. S-100 bus: memory read/write, I/O read/write, wait states, pRDY.
8. Interrupts: VI line to /INT0, 8259 vector, mask/trap behavior.
9. DMA: pHOLD/pHLDA, bus tri-state.
10. Boot: monitor from flash, level-1 loader, kernel, userland.

### 9.5 Test points

- Z180 clock, /RESET, /M1, /MREQ, /IORQ, /WAIT.
- CPLD JTAG TCK/TMS/TDI/TDO.
- Map RAM address, permission nibble, and the A12–A23 driver inputs.
- S-100 pSYNC, pSTVAL, pDBIN, pWR, pRDY, pWAIT.
- Trap window flash address counter.
- A20–A23 at the S-100 edge for a known PTE.

## 10. Compatibility notes, open decisions, verification

### 10.1 Exact and deliberate differences

| item | MPZ80 | this board |
|---|---|---|
| PTE | 8 address + 4 perms | 12 address + 4 perms |
| A20–A23 source | task register high nibble | PTE high nibble |
| task register high nibble | address bank | latched context, not address |
| selector boot | task-register bank + RAM trampoline | direct PTE map, no trampoline |
| Z180 MMU | n/a | transparent, A16–A19 unconnected |
| Z180 internal I/O | n/a | relocated to 0x80–0xbf |
| FPU/front panel | fitted | optional/omitted |
| memory fault status | manual values | emulator-compatible 0xf7 by default |
| task switch delay | 7 instruction fetches (manual) | 8 M1 cycles (emulator/kernel) |

Everything else—the register addresses, map layout, trap window, trap
frame, permissions, mask/status bits and boot sequence—is intended to be
identical.

### 10.2 Open decisions

- Task register high nibble: reserve or use as context bits (and the
  trap-clear/no-readback/delay effects).
- Z180 internal-I/O trap leak: accept and document, or relocate the
  internal block again.
- Manual-accurate vs emulator-accurate permission/status details.
- Two 84-pin CPLDs: U1 is 58 and U2 is 46, with 6 and 18 spare.  The
  merge of the old sequencer and translation devices is what makes the
  two-device build possible.
- Flash size is fixed at 1M×8; the image-select DIP chooses the 2K alias
  image, while the F00000–FFFFFF region exposes all images.
- Optional FPU and front-panel display/keyboard; the Z180 on-board
  UARTs are ignored and have no header or driver.
- DIP-64 vs PLCC-68 Z180 (through-hole preference).
- 10 ns 5 V map SRAM vs 8 ns 3.3 V with level shifting.

### 10.3 Verification checklist

1. Compute the PTE address for kernel segments below 1 MB, the selector
   ROM at 0xFF0000, and the top of the 16 MB space.
2. Walk reset, user halt, interrupt, GROW fault, I/O trap and task
   switch; compare the sequence and status/mask bytes to `mpz80.c`,
   `mmu_test.c` and `trap.c`.
3. Assemble the 15-byte reset patch and confirm it fits and relocates
   the Z180 internal I/O.
4. Confirm the selector rewrite maps 0xFF0000 through the PTE and needs
   neither the task register bank nor the RAM trampoline.
5. Map a PTE to F00000 and read the flash region; confirm the same bytes
   appear through the 0x800 alias and that no S-100 cycle is generated.
6. Check the per-device 84-pin pin budget and the registered inter-CPLD
   timing.
6. Check the BOM against available 5 V parts.
7. Future: change hwsim `getpte()` to source A20–A23 from the PTE high
   nibble, apply the selector rewrite, run the boot regression; then a
   CPLD testbench and prototype bring-up.

### 10.4 References

- `src/hwsim/d1/mpz80.c` — MMU, trap, task delay, status/mask model.
- `src/hwsim/d1/mmu_test.c` — PTE, RI10 and trap-register test.
- `src/micronix/stand/roms/mon500.s` and `mon500.lst` — reset and trap
  sequences, selector boot, map routines.
- `src/micronix/sys/trap.c`, `malloc.c`, `ARCHITECTURE.md`,
  `include/sys/proc.h`, `include/sys/sys.h`, `sys/uhdr.s` — kernel
  contract.
- `publicdocs/Morrow-MPZ80-CPU-Technical-Manual-Rev1-Apr82.pdf` and
  `extra/docs/MPZ8010.ECN.txt` — original hardware and ECN-10.
- Zilog Z8S180 user manual/datasheet (external).
- Atmel/Microchip ATF1508AS datasheet (external).
- ISSI IS61C256AH, Alliance AS7C256, IDT71256SA datasheets (external).
- IEEE-696 S-100 bus standard.
- `hw/esp32-nic.md` — document style and table conventions.
