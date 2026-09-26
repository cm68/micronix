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
 * The frame is a hole in the kernel's own address space, between the text
 * and the data, and it costs nothing: it is placed where nothing else
 * can be.  Take N, the last address the kernel's text reaches, round it
 * up to a page boundary, and that is OVLBASE; the data segment begins a
 * page above it.  The segment between them has no text below it and no
 * data above it, so mapping that one segment to a module's page disturbs
 * nothing - no buffer window, no scratch window, no pool.  The pool
 * still runs from _ebss to BUFWIN as it always has.
 *
 * N is the end of the resident text and nothing else.  The init-only
 * code (highmem.o) is not text for this purpose: it is folded to data
 * and parked after bss, so it is already above the frame and is
 * reclaimed as buffer headers rather than counted as something the frame
 * must clear.
 *
 * Nothing needs to be written into the frame.  The linker is told where
 * the data begins (-Tdata), which is what opens the hole, and the module
 * is linked at OVLBASE and mapped there when it is reached.
 *
 * OVLSEG is therefore a property of the build, not a choice: it moves
 * when the kernel's text grows past its page.  The kernel link and every
 * module link come out of one make variable so they cannot disagree, and
 * CCFLAGS passes it in.  The value below is what resident text reaches
 * with the disk drivers out of the kernel, which is the build that
 * matters; while they are still linked the text is longer and the frame
 * is higher.  Measured, not chosen: the three drivers are 9032 bytes of
 * text, the resident kernel ends at 0xa6d0 with them and 0x8388 without,
 * so the frame is the page at 0x9000 and a page higher would cost the
 * pool four fifths of itself.
 */
/*
 * The default, and only the default: the build passes the real value in
 * (sys/GNUmakefile), and this is what a compile that was not told gets.
 * What follows is not under the guard - the whole interface is the same
 * whatever segment it is built for, and an #ifndef spanning it would
 * make passing the value in erase the header.
 */
#ifndef OVLSEG
#define OVLSEG      9
#endif
#define OVLBASE     (OVLSEG * 0x1000)

/*
 * Interrupt lines a module may claim.  The lines with a driver behind
 * them are the disk controllers; the ACE and clock lines are resident.
 */
#define NOVLINE     3

/*
 * A module's first byte.  Everything the kernel needs to run a driver is
 * in these words, which is the whole point of them: the kernel names no
 * driver symbol and the driver calls no kernel function.  A driver
 * declares itself here and that is all it does.
 *
 * bvec and cvec are the entries the kernel will call through - a block
 * driver fills in bvec, a character driver cvec, one that is both (dj:
 * the floppy, and its raw character mode) fills in both.  The kernel
 * copies them into its own tables when the driver is placed, because a
 * function that lives in a module is only a function while that module
 * is the one mapped; a module that puts entries here and no driver
 * behind them registers nothing, and its major answers as nodev does.
 *
 * line is the interrupt line intr is on, and intr is the handler.  A
 * handler runs at a moment its page is not necessarily the one mapped,
 * which is why it is named here rather than registered from inside
 * init(): the kernel has to know which segment to put in place before it
 * calls, and the driver is in no position to say so at an interrupt.
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
 * name is what the driver calls its major, which is what devname[] and
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
    UINT line;                  /* the line intr is on, if there is one */
    int (*intr) ();             /* the interrupt handler, 0 for none */
    char name[12];              /* what devname[] reports for the major */
};

/*
 * The resident half, in sys/ovl.c.  None of it is in the u page
 * (sys/consts.c): these tables are written when a module initializes and
 * are one table for the whole system, where the u page is per-process
 * and cloned by fork.
 */
extern struct ovlhdr *ovlmap(); /* ovlmap(major): map that module, return its header */
extern char *ovldata();         /* ovldata(major): the driver's data, or 0 */
extern int ovlattach();         /* ovlattach(seg): place a module, run its init */
extern int ovlstart();          /* place the driver stamped into the slot */
