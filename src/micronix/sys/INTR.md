# Interrupt wiring — probe and attach

How a driver's interrupt handler gets wired to the card it serves.
Nothing here is a compile-time constant in the driver: the wiring is
discovered at boot and done once, by the resident half.  This is a plan;
nothing below is built yet.

## First principles

- **No sharing.**  One controller, one VI line.  The historical HDCA +
  HD-DMA both on VI0 is invalid; no known system carried both controllers,
  and forcing them onto distinct lines is more compatible, not less.
- **Drivers don't vary.**  A driver is compiled once, identical for every
  PIC configuration.  Its whole interrupt surface is two things: the
  handler it supplies, and a probe that tickles its card.  It never names
  a line, and it never calls `inton`.
- **Compile-time PIC.**  The PIC stack is fixed per machine, so it is a
  linked object (`ss1pic`, `multiopic` or `wbiopic`), not a runtime
  choice.  The drivers and the resident half call `inton`/`pic_irr`/
  `pic_eoi` by name; the link resolves them to the chosen object.

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
reset or sense that completes for a disk controller, an `ND_READ` for the
network disk.  It runs with the module mapped and nothing else in flight,
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

## The three objects

- **ss1pic** — the CompuPro SS1's two 8259s, CAS-cascaded on the CPU
  board, all eight VI lines vectored.  `pic_irr()` reads both PICs, the
  slave reached through the cascade; `pic_eoi()` acknowledges both.
- **multiopic** — the multio's master at 0x48 with the second board's slave
  at 0x40, six lines.  The slave is polled, never vectored: its pending
  lines are read by selecting it, and its poll *is* its acknowledge, so
  `pic_eoi()` only ever acknowledges the master.
- **wbiopic** — the wunderbus I/O's single PIC, three lines.  The
  smallest configuration; under no-sharing it carries at most three
  controllers.

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

## What goes away

- `ints.h`'s `INT_*` line names and its `#if N8259` table — a driver
  names no line, so there is nothing to name.
- `OVLSHARE` and the shared-line machinery in `ovl.c` — one handler per
  line.
- The `inton(HDCAINT)` idiom in every driver's `open` — the resident
  attach unmasks once, at boot.
- The single-PIC three-line build as a full-system config — six
  controllers need six lines.

## The order of work

1. The PIC objects first — move `inton`/`intoff` and the slave bring-up
   out of `multio.c`/`slavepic.c` into `ss1pic.c`, `multiopic.c` and
   `wbiopic.c`, the same behavior, linked by a `PIC` make variable.
2. Then the ovlhdr change (`line` -> `probe`) and the attach phase in
   `ovl.c`, reading `pic_irr()` and writing `ovlint`.
3. Then delete the now-dead sharing (`OVLSHARE`, `INT_*`, the
   `inton`-in-`open` idiom).
