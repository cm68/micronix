# Interrupt wiring — probe and attach

How a driver's interrupt handler gets wired to the card it serves.
Nothing here is a compile-time constant in the driver: the wiring is
discovered at boot and done once, by the resident half.

## First principles

- **No sharing.**  One controller, one VI line.  The historical HDCA +
  HD-DMA both on VI0 is invalid; no known system carried both controllers,
  and forcing them onto distinct lines is more compatible, not less.
- **Drivers don't vary.**  A driver is compiled once, identical for every
  PIC configuration.  Its whole interrupt surface is two things: the
  handler it supplies, and a probe that tickles its card.  It never names
  a line, and it never calls `inton`.
- **Runtime probe.**  One resident 8259 driver discovers the board at
  boot and fills the resident line table (sys/pic8259.h).  It probes the
  SS1's pair, then the master at 0x48; a Wunderbus I/O answers its DIP
  switch at 0x49 while a MultIO reads 0xff, and the optional slave at
  0x40 decides three or five lines.  The master's type chooses polling
  or CAS for the slave: a Wunderbus master has no cascade hardware, so
  its slave is polled; a MultIO master CAS-vectors it.  One kernel boots
  every board; there is no per-board image or build flag.

## The model

The S-100 bus has eight vectored interrupt lines, VI0..VI7.  Which card
is on which line is a physical property — its slot, its jumpers — and
the kernel discovers it rather than being told.  Attach is the wiring:
put the driver's handler into the vector table at the index the card's
vector lands on.

## The ovlhdr

The module header (sys/ovl.h) drops its line number and gains the tickle.
`UINT line` becomes `int (*probe)()` in place, so the field offsets and
`devname` are untouched:

    struct ovlhdr {
        UINT major;
        int (*init) ();         /* hardware setup, once, 0 for none */
        int (*tick) ();         /* watchdog, once a second, 0 for none */
        char *data;
        struct biovec *bvec;
        struct ciovec *cvec;
        int (*probe) ();        /* the tickle: assert this card's interrupt */
        int (*intr) ();         /* the interrupt handler */
        char name[12];
        UINT8 ndrv;
    };

`probe()` makes the card raise its interrupt with no side effect — a
reset or sense that completes for a disk controller.  It runs with the
module mapped and nothing else in flight,
and it is the one thing a driver knows about interrupts that the rest of
the system does not.

## The PIC interface

The resident half links against one object that provides five entry
points and nothing else:

    void pic_init(void);        /* bring the PIC(s) up, everything masked */
    void inton(int line);       /* unmask a line */
    void intoff(int line);      /* mask a line */
    int  pic_irr(void);         /* which lines are pending — the probe's read */
    void pic_eoi(int line);     /* end of interrupt on a line */

`pic_irr()` is an OCW3 read of the IRR register(s), taken with every line
masked so the CPU never sees the tickled interrupt.  It returns a mask of
pending lines; with no sharing exactly one bit is set, and the probe
takes that line.  The three objects differ only behind these five names.

## The three boards

One init (sys/pic_init.c) programs and probes each in turn and fills the
line table from whichever answered:

- **SS1** — two CAS-cascaded 8259s on the CPU board at 0x70/0x72, all
  eight VI lines vectored.  `pic_irr()` reads both PICs; `pic_eoi()`
  acknowledges both.
- **Wunderbus I/O** — the single PIC at 0x48, three lines (VI0–VI2).
  Its DIP switch at group 0, register 2 (0x49) identifies the board;
  it has no cascade hardware.
- **Wunderbus I/O + MultIO** — the 0x48 master plus the add-on board's
  slave at 0x40, five lines.  The slave is polled: its pending lines are
  read by selecting it, and its poll *is* its acknowledge.
- **Two MultIOs** — the same addresses; the master at 0x48 has cascade
  hardware and the slave at 0x40 CAS-vectors through its own table.

The master's type is read once at boot (the DIP switch answers on a
Wunderbus; a MultIO reads open bus), which is what decides whether the
slave is programmed polled or CAS-vectored.

## The attach phase

Resident, in sys/ovl.c, run after `ovlplaceall` and before the first
open.  For each module in major order:

    intoff(all lines)           /* the tickle must not reach the CPU */
    probe()                     /* the card asserts its line */
    line = pic_irr()            /* which line went pending */
    ovlint[line].fn = intr      /* the wiring: vector -> table index */
    ovlint[line].seg = seg
    pic_eoi(line)               /* clear the tickled interrupt */
    inton(line)                 /* the line is live from here on */

With no sharing one tickle raises exactly one line, so there is nothing
to disambiguate, and `ovlint` holds one handler per line (`OVLSHARE` is
gone).

## The dispatch

Unchanged.  `ovlntr(line)` runs `ovlint[line].fn` with the module's
segment mapped (`ovlcall`); the handler never learns its line — it is
only ever called because its card asserted it.  The EOI the handler owes
is `pic_eoi(line)`, which the resident dispatch supplies; the driver does
not touch the PIC.

## Level-asserted lines and the idle mask

A card that releases its line only when the driver consumes data — the
net card holds VI5 low while its receive FIFO is non-empty — cannot be
served by the attach's single `inton(line)` alone.  The handler cannot
drain the card (the frame belongs to the reader), so a frame arriving
with nobody in `read()` would re-assert the line forever.

The pattern that driver uses, and the one to copy:

    handler:   pic_mask(line); wakeup(&chan);
    reader:    inton(line); look; sleep(&chan);     /* loop */

The line is closed whenever no reader waits; the reader opens it for
each look, and because the card still holds it low the interrupt is
immediate.  A sleep aborted by a signal leaves the line open with no
reader, which is safe — the next frame's handler closes it.

The handler still does not name its line.  `intrline(handler, seg)`
scans `ovlint[]` for the handler's own address and segment, which the
attach wrote; the segment is part of the match because every module's
functions are at OVLBASE-relative addresses and two modules can share
one.  A module that carries a second face — the ND module's network
face (`sys/net.c`) — claims it from the module's init, which
`ovlplaceall()` runs before `intr_attach()`:

    intrset(netprobe, netint, seg);

`pic_mask(line)` (sys/probe.c) is the resident counterpart of
`inton()`; `intoff()` is init-only.

## What went away

- `ints.h`'s `INT_*` line names and its `#if N8259` table — a driver
  names no line, so there is nothing to name.
- `OVLSHARE` and the shared-line machinery in `ovl.c` — one handler per
  line, and a second card that tickles a taken line is a panic.
- The `inton(HDCAINT)` idiom in every driver's `open` — the resident
  attach unmasks once, at boot.
- The single-PIC three-line build as a separate kernel — it is one of
  the four runtime arrangements now.

## State

One image, no `N8259` flag and no poled/vectored build split.  The
generic `pic_init()` probes the master board (0x49 DIP switch), the
slave at 0x40, and the SS1 pair, and programs polled or CAS from what it
finds.  The simulator names a machine the same way: the wiring is the boards -
`ss1`, `wb`, `mio`, `wb,mio` or `mio,mio` - followed by the cards'
`viN=card` assignments, one `sim.json` preset per topology.
