# Interrupts

How a device's interrupt gets from a card's edge connector to a driver's
handler, and the three ways the interrupt controllers can be wired.  This
is the whole story from the board to the door `sys/stub_intr.c` mentions.

## The pieces

The S-100 (IEEE 696) bus carries eight **vectored interrupt** lines,
VI0..VI7.  A controller card drives one of them (which one is a jumper on
the card), and an 8259 programmable interrupt controller monitors the
lines, prioritizes them, and raises the single processor interrupt line
**PINT** (line 73).  When the CPU answers with an interrupt acknowledge,
**SINTA** (line 96), the 8259 puts a CALL vector on the data bus.

The machine has two boards that each carry an 8259:

| board | role in the base system | 8259 base | CAS pins (12/13/15) | ROM socket |
|---|---|---|---|---|
| **Wunderbus I/O** | the base system | `0x48` | no | no |
| **MultIO** | the add-on | `0x40` | yes | yes (disk boot ROMs) |

Each 8259 monitors **three** VI lines on its IR0..IR2 (jumper-selectable),
and uses IR3..IR7 for its own three serial ports, parallel port, and
real-time clock.

## The three topologies

The kernel is built for one of three cases, chosen with two macros in
`include/sys/ints.h` (`N8259`, the PIC count, and `N8259_POLLED`, how the
slave attaches):

1. **One board** — `N8259 == 1`.  A single Wunderbus *or* a single
   MultIO.  Three VI lines (VI0..VI2) are live; there is no slave and no
   cascade.

2. **Wunderbus + MultIO** — `N8259 == 2`, `N8259_POLLED == 1`.  The
   Wunderbus is the master, the MultIO the slave.  The Wunderbus has no
   CAS pins, so the slave cannot hand its vector over during the
   acknowledge; instead its interrupt is **polled**.  The slave's INT is
   wired to the master's cascade line (IR6), so the master owns SINTA and
   emits IR6's vector; the cascade handler (`ovlint6`, `sys/intrpt.s`)
   then sends the slave an OCW3 poll command and reads back the poll word,
   which names the slave's line and acknowledges it in the one read.  This
   is the only case that does the polling.

3. **Two MultIOs** — `N8259 == 2`, `N8259_POLLED == 0`.  Both boards have
   CAS pins, so the slave **CAS-cascades**: its INT is wired to the
   master's cascade line, the master drives the slave's ID on CAS0..CAS2
   during the acknowledge, and the slave emits its own vector.  No
   polling; the vector arrives directly.

The build picks a case with `-DN8259=… -DN8259_POLLED=…`.  `include/sys/ints.h`
defaults to `1` and `1` (one board), but the d1's kernel build
(`sys/GNUmakefile` and `sys/Makefile`) sets `-DN8259=2 -DN8259_POLLED=1`
to match the machine it models; a single-board kernel overrides both with
`-DN8259=1`.

## The line map

Under two PICs each controller gets its own line; under one PIC the
historical sharing applies.  This is the whole table, and it lives in one
place (`include/sys/ints.h`, and its mirror for the simulator in
`hwsim/d1/vimap.h`):

| controller | `N8259 == 2` | PIC | `N8259 == 1` |
|---|---|---|---|
| HD-DMA | VI0 | master IR0 | VI0 |
| DJ-DMA (floppy) | VI1 | master IR1 | VI1 |
| HDCA | VI2 | master IR2 | VI0 (shared) |
| NCR 5380 (SCSI) | VI3 | slave IR0 | VI2 (shared) |
| IDE | VI4 | slave IR1 | VI2 (shared) |
| (spare) | VI5 | slave IR2 | — |

## The driver contract

A driver never names a line, a PIC, or a topology.  It claims its
interrupt by a name and that is all:

```c
#define NCRINT INT_NCR        /* ncr.h - the name, not the wiring */
```

- The **name** is one of `INT_HDDMA`, `INT_DJDMA`, `INT_HDCA`, `INT_NCR`,
  `INT_IDE` (in `include/sys/ints.h`), mapped there to a line number for
  the chosen topology.
- A driver unmasks with `inton(NAME)`, masks with `intoff(NAME)`, and
  names the line in its `struct ovlhdr`'s `line` field.  `inton`/`intoff`
  route the name to the right PIC (`0x48` for lines 0..2, `0x40` for
  lines 3..5) without the driver knowing which.
- The **dismiss** (end-of-interrupt) is `intrdismiss()` in
  `sys/multio.c`, called from the dispatcher (`intrupt` in `sys/mio.s`).
  It EOIs the master always, and the slave only when CAS-cascaded (a
  polled slave is acknowledged by the poll read itself, so there is
  nothing to dismiss there).

So a driver is two lines: `#define <LOCAL> INT_<device>` and
`inton(<LOCAL>)`.  Rebuilding for a different machine is `-DN8259`/`-D…
N8259_POLLED`, never an edit to a driver.

## The wiring

The 8259 sits at two register addresses, `base+4` and `base+5` (register 0
is the ICW/OCW port, register 1 the IMR/OCW1); the board's group select is
`base+7` and must be written to select the PIC group:

| board | base | 8259 regs | group select | PIC select value |
|---|---|---|---|---|
| master (Wunderbus) | `0x48` | `0x4c`/`0x4d` | `0x4f` | `PENABLE` (`0x28`) |
| slave (MultIO) | `0x40` | `0x44`/`0x45` | `0x47` | `SPICONLY` (`0x08`) |

The master always drives PINT and owns SINTA.  In both two-PIC cases the
slave's INT is wired to the master's **cascade line, IR6** (`CASCADE_IR`
in `include/sys/ints.h`) — IR6 is otherwise unused, its inputs 0..2 being
the three VI lines, 3..5 the serial ports and 7 the clock.  In the polled
case (2) the cascade handler sends the slave an OCW3 poll command; in the
CAS case (3) CAS0..CAS2 are cabled between the boards (the four-pin
connector at the MultIO's `11A`) and the slave drives its own vector
during the acknowledge.

IR6 was the master's parallel-printer line in the one-PIC build; that
printer has no device and no simulator model, so it is dropped, and IR6
is the cascade line in every build (the poll finds nothing when there is
no slave).

## Strapping the cards

Each controller card is jumpered to the VI line the table above names it
on.  The specifics are the card's own manual; the lines that matter here:

- **Wunderbus I/O** — jumper-selectable which three VI lines its IR0..IR2
  monitor; no cascade jumpers exist (the board has no CAS pins).
- **MultIO** — the same three-line jumper for IR0..IR2; the cascade
  connector `11A` carries `CAS2/CAS1/CAS0` plus the slave's `INTR` for
  case 3.
- **CompuPro DISK 3** — the completion interrupt is jumper `J9`
  (VI0..VI7); the manual's own instruction is position 1, VI1.  (Its
  `J7`/`J8` select EPROM size and `J11` the DMA priority — not interrupt
  lines.)
- **CompuPro DISK 2** — `J7`..`J14` select VI0..VI7, `J15` selects the
  generalized INT line.
- The Morrow cards (HD-DMA, DJ-DMA, HDCA, NCR, IDE) are each strapped to
  their line; `hwsim/d1/vimap.h` is the simulator's record of the same
  assignment, kept beside this document so the two cannot drift.

## Where the topology shows up

Only three places in the kernel actually branch on the topology, and none
of them are drivers:

- `include/sys/ints.h` — the names-to-lines table and `CASCADE_IR`.
- `sys/multio.c` — `inton`/`intoff` (which PIC) and `intrdismiss` (which
  PICs to EOI).
- `sys/intrpt.s` — `ovlint6`, the IR6 handler that polls the slave and
  dispatches its line; the slave PIC's bring-up is `slave_pic_init` in
  `sys/cus.c` (init-only, reclaimed after boot).
