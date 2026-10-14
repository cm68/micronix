/*
 * Driver modules.
 *
 * A driver can be built as a module instead of being linked into the
 * kernel: a page of code and data, linked at a fixed address (OVLBASE)
 * against the kernel's own symbol table, and copied at run time into one
 * 4K segment of the kernel's address space.  Linking against the kernel
 * is what makes the module work in one direction - it names anything in
 * the kernel and gets a real address - and what makes it impossible in
 * the other: the kernel does not contain the module, so at kernel link
 * time there is no djopen, mwstrat or djint to name.
 *
 * What crosses in that direction is this header.  It is the module's
 * first byte, at OVLBASE, and it is the whole of the interface.  The
 * kernel learns a driver's entry points only because the driver hands
 * them over in init(), below; nothing in the kernel holds the name of a
 * driver entry point, which is why a kernel can be linked with no block
 * device drivers at all.
 *
 * OVLSEG is one page, so one module is mapped at a time.  Every address
 * in a module is offset from OVLBASE, and stays valid exactly while that
 * module is the one mapped - which is why the resident tables hold entry
 * points rather than the kernel branching into a driver directly, and
 * why a driver must not expect its own page to be there across a sleep.
 *
 * The frame is one fixed page of the kernel's own address space, at
 * segment OVLSEG (0xc000).  The pages around it are fixed too: the
 * scratch window at 0xd000, the buffer window at 0xe000, the u page at
 * 0xf000.  The buffer pool runs from _ebss - the end of the resident
 * data and bss - up to OVLBASE, so the driver page sits just above the
 * pool and just below the scratch window; mapping that one segment to a
 * module's page disturbs nothing - no pool header, no scratch window,
 * no buffer window.
 *
 * The init-only code (highmem.o) is not text for this purpose: it is
 * folded to data and parked at _ebss, inside the region the pool grows
 * into, and is reclaimed as buffer headers once main() has returned -
 * which is why the pool's ceiling is OVLBASE, not BUFWIN.
 *
 * Nothing needs to be written into the frame.  The module is linked at
 * OVLBASE and mapped there when it is reached, and setdev stamps the
 * root driver's page onto it at install time.
 *
 * OVLSEG is a property of the build, not a choice: it moves only if the
 * fixed layout itself moves.  The kernel link, every module link and the
 * slot page (ovlslot.o) all take it from one make variable so they
 * cannot disagree, and CCFLAGS passes it in.  The value below is only
 * the default - what a compile that was not told gets - so the header
 * and the build describe the same layout.
 */
/*
 * The default, and only the default: the build passes the real value in
 * (sys/GNUmakefile), and this is what a compile that was not told gets.
 * What follows is not under the guard - the whole interface is the same
 * whatever segment it is built for, and an #ifndef spanning it would
 * make passing the value in erase the header.
 */
#ifndef OVLSEG
#define OVLSEG      12
#endif
#define OVLBASE     (OVLSEG * 0x1000)

/*
 * The interrupt topology (N8259, N8259_POLLED), the number of vectored
 * lines (NOVLINE) and the line names (INT_*) are in sys/ints.h, so a
 * driver claims an interrupt by name rather than by wiring.
 */
#include <sys/ints.h>

/*
 * A module's first byte.  Everything the kernel needs to run a driver is
 * in these words, which is the whole point of them: the kernel names no
 * driver symbol and the driver calls no kernel function.  A driver
 * declares itself here and that is all it does.
 *
 * bvec and cvec are the entries the kernel will call through - a block
 * driver fills in bvec, a character driver cvec, and a driver can be
 * both.  The kernel
 * copies them into its own tables when the driver is placed, because a
 * function that lives in a module is only a function while that module
 * is the one mapped; a module that puts entries here and no driver
 * behind them registers nothing, and its major answers as nodev does.
 *
 * probe is the tickle that makes the card raise its interrupt, and intr
 * is the handler.  A driver names no line: the wiring is discovered at
 * boot by tickling each card and reading the PIC's pending register
 * (sys/probe.c).  The handler runs at a moment its page is not
 * necessarily the one mapped, which is why it is named here rather than
 * registered from inside init(): the kernel has to know which segment to
 * put in place before it calls, and the driver is in no position to say
 * so at an interrupt.
 *
 * init() is called once, with the page in place and nothing able to reach
 * the driver yet, and it is for the hardware alone: the controller's
 * timers, the DMA channel, the interrupt enable.  It is called before any
 * of the above is registered, and what it returns decides whether any of
 * it happens: non-zero is a driver whose hardware is not on this machine,
 * and a driver that says so is not registered at all.  That is what lets
 * a kernel carry a driver for a card that is not plugged in - a SCSI
 * driver that looks, finds nothing, and returns ENODEV - at the cost of
 * the page it occupies and nothing else.  Nothing has been taken when
 * init runs, so there is nothing to give back.
 *
 * tick() is the same problem as intr, arriving by a different door.  A
 * disk controller that has stopped talking raises nothing, and the
 * driver that wants to notice has to go and look on a clock of its own -
 * work that runs at a moment when its page is not necessarily the one
 * mapped, exactly as an interrupt handler's is.  So a driver that wants
 * one names it here and the resident side calls it with the page in
 * place (ovltick, sys/ovl.c).  One clock for the whole system, walked
 * across the modules that asked, rather than a timer per driver: the
 * kernel has five timeouts in total (time.c) and each of these used to
 * take one and re-arm it forever.
 *
 * Zero is the ordinary case for tick and intr - a driver with no
 * periodic work, or nothing to say at an interrupt.  Zero init is a
 * driver with nothing to switch on: it is registered as soon as it is
 * placed.
 *
 * data is where the driver's own data structure is, and it is the one
 * thing init() cannot work out for itself.  A resident init is kernel
 * code and cannot name anything in a module - not a function, and not a
 * data object either; knowing the segment a module was placed in gives
 * it a page, not an offset into one.  So the module says where its data
 * is here, at a link time constant the kernel can read, and init()
 * reaches it through that one pointer and no other (ovldata below).  The
 * structure is opaque to the kernel: it is an address, and what the
 * driver keeps in it is the driver's business.
 *
 * A driver the kernel still holds says the same thing the same way: its
 * header is a resident object, so its data pointer is a kernel address
 * and the hook below returns it without mapping.  Resident and module
 * differ in where the header is, not in what it says.
 *
 * name is what the driver calls its major, which is what devname and
 * so pcon() report.  It is written in a header as a list of characters
 * and not as a string, and that is not a style: ccc emits a copy of a
 * string literal ahead of the object's data, so "djdma" would put six
 * dead bytes in front of the header and it would no longer start at
 * OVLBASE.  A header object holds the struct and nothing else, for the
 * same reason - it is named first on the module link so that its data
 * lands at the front of the page, and anything else in that object lands
 * in front of it.
 */
struct ovlhdr {
    UINT major;                 /* the major this module serves */
    int (*init) ();             /* init(seg), once, before it is reachable */
    int (*tick) ();             /* tick(), once a second, 0 for none */
    char *data;                 /* the driver's data, in its own segment */
    struct biovec *bvec;        /* its block entries, 0 for none */
    struct ciovec *cvec;        /* its character entries, 0 for none */
    int (*probe) ();            /* the tickle: assert this card's interrupt */
    int (*intr) ();             /* the interrupt handler, 0 for none */
    char name[12];              /* what devname reports for the major */
    UINT8 ndrv;                 /* how many drives, enumerated at signon */
};

/*
 * The name of the module mapped now.  Every module's header starts at
 * OVLBASE, so the name field is always at this one fixed address - no
 * table and no variable.  The 0x10 is the name[] offset in struct
 * ovlhdr: eight two-byte fields precede it.  A major with nothing mapped
 * leaves whatever was there before, so callers that can mean "none"
 * check first.
 */
#define devname ((char *)(OVLBASE + 0x10))

/*
 * The resident half, in sys/ovl.c.  None of it is in the u page
 * (sys/consts.c): these tables are written when a module initializes and
 * are one table for the whole system, where the u page is per-process
 * and cloned by fork.
 */
extern struct ovlhdr *ovlmap(); /* ovlmap(major): map that module, return its header */
extern int ovlremap();          /* ovlremap(): put this process's driver back */
extern char *ovldata();         /* ovldata(major): the driver's data, or 0 */
extern int ovlcall();           /* ovlcall(fn, seg): run fn with its own page mapped */
extern int ovlattach();         /* ovlattach(seg): place a module, run its init */
extern int ovlstart();          /* place the driver stamped into the slot */
extern int ovlntr();            /* ovlntr(line): call every handler on a line */
extern int intrset();           /* intrset(probe, fn, seg): claim for the attach loop */
extern int intr_attach();       /* intr_attach(): tickle each card, wire ovlint */
