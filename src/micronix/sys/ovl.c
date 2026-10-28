/*
 * Driver modules: the resident half.
 *
 * sys/ovl.h is the interface; this is the kernel's side of it.  biosw[]
 * and ciosw[] point at trampolines rather than at drivers, so the tables
 * the kernel links are the same tables whether a driver is resident, is
 * a module, or is not there at all - and the kernel names no driver.
 *
 * The tables live here and not in sys/consts.c beside the switch tables
 * they feed.  consts.c links into upage.o, the u page, which every
 * process has its own copy of and which fork's segcopy clones; these are
 * written when a module initializes and must be one table for the whole
 * system.
 */
#include <types.h>
#include <sys/sys.h>
#include <sys/buf.h>
#include <sys/con.h>
#include <sys/ioctl.h>
#include <sys/ovl.h>
#include <sys/proc.h>

extern UINT nbdev, ncdev;
extern UINT8 map0[];
extern unsigned bmajor(), cmajor();
extern int nodev();

#define	OVLMAJ	16			/* majors we keep a place for */

/*
 * The two namespaces, each with its own major-to-segment map.  There is
 * no single "major -> segment" table: char 2 and block 2 are unrelated
 * rows in unrelated tables, and each module page is registered only in
 * the space its header names.
 */
UINT bdevseg[NBDEV];			/* block major -> segment, 0 = none */
UINT cdevseg[NCDEV];			/* char major -> segment, 0 = none */
struct biovec ovlbvec[OVLMAJ];	/* entries its init() registered */
static struct ciovec ovlcvec[OVLMAJ];

/*
 * Drivers that want a tick, in the order they were placed.  The entry
 * carries its own segment, so the tick, like the interrupt line table,
 * is per module and does not consult a major.
 */
#define	NTICK	12
static struct tickrec {
    int (*fn) ();
    UINT seg;
} ovlticks[NTICK];
static UINT novltick;
static char ovltickarmed = 0;		/* the resident tick is running */

/*
 * The DMA bus lock's words (sys/bus.c): the module driving the bus, and
 * the module waiting for it.  busget and busgive are leaf code - two
 * driver modules call them and cannot call into each other - so they are
 * in the u page, but what they read and write is one bus for the whole
 * machine and so is here, for the reason the tables above are.  In the u
 * page every process had its own copy, and a transfer's busget to its
 * busgive is not inside one process: it spans a sleep, mw and dj both
 * sleep holding the bus (mw's mwstop waits for the controller, dj's
 * busplease sleeps for the heir to call).  A busgive run in another
 * process read its own heir, found 0, and dropped the heir the first
 * process had registered - a transfer that never started and never
 * completed, with nothing left to notice.
 */
int (*busmaster) ();		/* module driving the bus, 0 = none */
UINT busmasterseg = 0;		/* the segment busmaster lives in */
int (*busheir) ();		/* module waiting for it, 0 = none */
UINT busheirseg = 0;		/* the segment busheir lives in */

/*
 * Call something that lives in a module: put its page in place for the
 * call and put the interrupted map back afterwards.  This is the whole
 * of what an interrupt handler and a tick both need, which is why they
 * share it.  Segment 0 is a driver the kernel still holds - resident
 * code, always mapped, nothing to do - and no module is ever placed in
 * segment 0, which is the cpu board's own ram and prom below the bottom
 * of main memory.
 *
 * The map register is write-only; the current segment is tracked in u.drv.
 *
 * Named and not static because the bus lock calls through it too
 * (sys/bus.c): the heir it wakes is a function in another module, and
 * giving up the bus is one more of the doors into a driver - a driver
 * calling a driver - that the trampolines above do not cover.
 *
 * u.drv goes with the map register, and that is what makes a callee that
 * sleeps in its own module safe: the heir the bus lock calls is dj's
 * busready (sys/dj.c), which sleeps for the bus, and it is running in
 * whatever process called busgive - whose u.drv names the caller's module
 * and not this one.  Putting the callee's segment there is what ovlremap()
 * finds after the switch, so the process comes back to the page it is
 * really in.  The caller's goes back with its page, so a driver that
 * returns from here and sleeps later in its own code is still right.
 */
int
ovlcall(fn, seg)
    int (*fn) ();
    UINT seg;
{
    UINT8 drv;

    if (seg == 0) {
        (*fn) ();
        return (0);
    }

    drv = u.drv;

    /*
     * u.drv goes before the register, and the caller's comes back
     * before the register too.  A switch between the pair restores the
     * page from u.drv, so the order decides which page a callee's first
     * instruction runs in; the other order puts the page being left
     * back under it.
     */
    u.drv = (UINT8) seg;
    map0[2 * OVLSEG] = (UINT8) seg;
    (*fn) ();
    u.drv = drv;
    map0[2 * OVLSEG] = drv;
    return (0);
}

/*
 * ovlcall with one argument, for the probe: the mode that clears the
 * card's interrupt or raises it (PROBE_CLEAR / PROBE_TICKLE in ovl.h).
 */
ovlcall1(fn, seg, arg)
    int (*fn) ();
    UINT seg;
    int arg;
{
    UINT8 drv;
    int r;

    if (seg == 0)
        return ((*fn) (arg));

    drv = u.drv;
    u.drv = (UINT8) seg;
    map0[2 * OVLSEG] = (UINT8) seg;
    r = (*fn) (arg);
    u.drv = drv;
    map0[2 * OVLSEG] = drv;
    return (r);
}

/*
 * Map a segment into the driver window.  The map register is write-only;
 * the current segment is u.drv.  Zero is resident code - nothing to map.
 */
static
ovlmapseg(seg)
    UINT seg;
{
    if (seg == 0)
        return (0);
    u.drv = (UINT8) seg;                /* before the register - ovlcall() */
    map0[2 * OVLSEG] = (UINT8) seg;
    return (1);
}

/*
 * Map a block major's module and return the header it starts with, or 0
 * if that block major has no module.  The char entry point is the same
 * against the other namespace; nothing here ever looks at a char major
 * for a block device, or the reverse.
 */
struct ovlhdr *
ovlbmap(maj)
    UINT maj;
{
    if (maj >= NBDEV || bdevseg[maj] == 0)
        return (0);
    ovlmapseg(bdevseg[maj]);
    return ((struct ovlhdr *) OVLBASE);
}

struct ovlhdr *
ovlcmap(maj)
    UINT maj;
{
    if (maj >= NCDEV || cdevseg[maj] == 0)
        return (0);
    ovlmapseg(cdevseg[maj]);
    return ((struct ovlhdr *) OVLBASE);
}

/*
 * Put this process's module back in the window.
 *
 * The window is one page for the whole system, but the driver a process
 * is inside is the process's own business: a driver's strategy routine
 * sleeps - ide's idestart, ncr's scsistart and dj's busplease all do,
 * waiting on their board - and a process parked in one is switched out
 * with its driver's page in the window, another process runs another
 * driver, and the page is gone when the first wakes.  It would resume
 * with another driver's bytes at OVLBASE and that driver's data under
 * them, which is the same runaway the bus lock hit from the other
 * direction (sys/bus.c).
 *
 * What makes it per-process is u.drv, the segment of the page the process
 * is running in, which ovlbmap(), ovlcmap() and ovlcall() record as they
 * map it, each writing u.drv before the map register so that a switch
 * taken between the two writes puts this page back and not the one being
 * left - so the door into a module the trampolines do not cover, the bus
 * lock's heir, is recorded too.  newmap() calls this at every switch with
 * the u page already the incoming process's, so u.drv is that process's
 * page and it is written back as it stands: what was recorded is the value
 * the register wants, and there is no major to look up.  A process that is
 * not inside a driver has nothing to put back and is left alone - the
 * next ovlbmap or ovlcmap at a module entry is what sets the window, and
 * leaving a stale page mapped costs nothing because no resident code reads
 * OVLBASE.
 */
ovlremap()
{
    if (u.drv)
        map0[2 * OVLSEG] = u.drv;
}

/*
 * A driver's own data, wherever it lives.  This is the hook a resident
 * init() calls: it maps the driver's segment on the way, so the pointer
 * it returns points at the driver's data in that segment, and the data
 * has to be in that segment and nowhere else.  An init() that wants a
 * second data object wants it inside this structure, not beside it -
 * there is no second way to name one.
 */
char *
ovldata(maj)
    UINT maj;
{
    struct ovlhdr *hdr;

    if ((hdr = ovlbmap(maj)) == 0)
        hdr = ovlcmap(maj);
    return (hdr ? hdr->data : (char *) 0);
}

/*
 * The character trampolines, the same shape against ciosw[] (sys/cio.c).
 */
int
ovlcopen(dev, mode)
    UINT dev;
    UINT8 mode;
{
    UINT8 maj;

    maj = cmajor(dev);
    ovlcmap(maj);
    if (ovlcvec[maj].open == 0) {
        nodev();
        return (0);
    }
    return ((*ovlcvec[maj].open) (dev, mode));
}

int
ovlcclose(dev, mode)
    UINT dev;
    UINT8 mode;
{
    UINT8 maj;

    maj = cmajor(dev);
    ovlcmap(maj);
    if (ovlcvec[maj].close == 0) {
        nodev();
        return (0);
    }
    return ((*ovlcvec[maj].close) (dev, mode));
}

int
ovlcread(dev)
    UINT dev;
{
    UINT8 maj;

    maj = cmajor(dev);
    ovlcmap(maj);
    if (ovlcvec[maj].read == 0) {
        nodev();
        return (0);
    }
    return ((*ovlcvec[maj].read) (dev));
}

int
ovlcwrite(dev)
    UINT dev;
{
    UINT8 maj;

    maj = cmajor(dev);
    ovlcmap(maj);
    if (ovlcvec[maj].write == 0) {
        nodev();
        return (0);
    }
    return ((*ovlcvec[maj].write) (dev));
}

int
ovlcmode(dev, flag)
    UINT dev;
    UINT8 flag;
{
    UINT8 maj;

    maj = cmajor(dev);
    ovlcmap(maj);
    if (ovlcvec[maj].mode == 0) {
        nodev();
        return (0);
    }
    return ((*ovlcvec[maj].mode) (dev, flag));
}

/*
 * Registration.  The header is where a driver declares its entries
 * (sys/ovl.h); these take them, and nothing outside this file calls any
 * of them any more - a driver declares, it does not call.  Registering
 * is what makes a driver reachable, so a header that names no entries
 * leaves its major answering as nodev does.  Major 0 is nodev and is
 * never claimed, and a major past the tables is not a device this build
 * has.
 */
static
bvecset(maj, open, close, strat, ioctl)
    UINT maj;
    int (*open) ();
    int (*close) ();
    int (*strat) ();
    int (*ioctl) ();
{
    if (maj == 0 || maj >= nbdev || maj >= OVLMAJ)
        return (-1);
    ovlbvec[maj].open = open;
    ovlbvec[maj].close = close;
    ovlbvec[maj].strat = strat;
    ovlbvec[maj].ioctl = ioctl;
    return (0);
}

static
cvecset(maj, open, close, read, write, mode)
    UINT maj;
    int (*open) ();
    int (*close) ();
    int (*read) ();
    int (*write) ();
    int (*mode) ();
{
    if (maj == 0 || maj >= ncdev || maj >= OVLMAJ)
        return (-1);
    ovlcvec[maj].open = open;
    ovlcvec[maj].close = close;
    ovlcvec[maj].read = read;
    ovlcvec[maj].write = write;
    ovlcvec[maj].mode = mode;
    return (0);
}

/*
 * The tick, for the drivers that have work which is not a response to
 * anything: a controller that has stopped talking raises no interrupt,
 * and a driver that wants to notice has to go and look on a clock of its
 * own.  That look has the handler's problem - it runs at a moment its
 * page is not necessarily the one mapped - so it is served the same way,
 * by the resident side mapping the module and calling (ovlcall above).
 *
 * One clock for the whole system, walked across the modules that asked,
 * rather than a timer per driver.  tlist[] (time.c) holds five timeouts
 * for the entire kernel and a driver's watchdog is never stopped, so a
 * driver that armed its own took a slot for the rest of the boot, and a
 * driver opened twice took two.  This takes one, and only for as long as
 * something wants one.
 *
 * The step is TICKINT, fast enough for the shortest period any driver
 * wants - mw's controller check, once a second - and a driver that wants
 * a longer period counts its own turns.
 */
#define	TICKINT	HERTZ			/* resident tick, once a second */

static
ovltick()
{
    register UINT i;

    for (i = 0; i < novltick; i++)
        ovlcall(ovlticks[i].fn, ovlticks[i].seg);

    timeout(&ovltick, 0, TICKINT);
}

/*
 * The tick re-arms itself, so starting it is what has to happen once, and
 * only when a driver has asked for one.  A kernel with no tick-driven
 * driver running has no timer of its own in tlist[] at all.
 */
static
ovlarm()
{
    if (ovltickarmed)
        return (0);
    ovltickarmed = 1;
    timeout(&ovltick, 0, TICKINT);
    return (0);
}

/*
 * Ask for a tick.  The entry carries its own segment, like a line-table
 * entry: a tick belongs to a module, not to a major in either space.
 * Zero withdraws the request.
 */
static
tickset(fn, seg)
    int (*fn) ();
    UINT seg;
{
    if (fn == 0)
        return (0);
    if (novltick >= NTICK)
        return (-1);
    ovlticks[novltick].fn = fn;
    ovlticks[novltick].seg = seg;
    novltick++;
    ovlarm();
    return (0);
}

/*
 * Place a driver.  The header is at OVLBASE, mapped by the caller.  Its
 * entries are read into locals *before* init() is called: init is entered
 * through the gate, which restores the caller's window on return, so the
 * page is no longer necessarily mapped when it finishes.  Reading the
 * entries afterwards was the race that an interrupt during a module-local
 * init could turn into a crash.
 *
 * A module registers in either namespace or both, by its own major in
 * each.  The segment is the same page either way; the two rows are
 * independent bindings, and a failed init withdraws both.
 *
 * Registration happens before init, because init() is what finds the
 * driver's own data through ovldata(), and that looks the major up here.
 * A non-zero init is the driver saying the hardware is not on this
 * machine: its rows go back and its major answers as nodev does, at the
 * cost of the page and nothing else.
 */
static
ovlplace(hdr, seg)
    struct ovlhdr *hdr;
    UINT seg;
{
    UINT8 bmaj, cmaj;
    int (*init) (), (*tick) (), (*probe) (), (*intr) ();
    struct biovec bv;
    struct ciovec cv;

    bmaj = hdr->major;
    cmaj = hdr->cmajor;
    init = hdr->init;
    tick = hdr->tick;
    probe = hdr->probe;
    intr = hdr->intr;

    if ((bmaj == 0) && (cmaj == 0))
        return (-1);
    if (bmaj >= NBDEV || cmaj >= NCDEV)
        return (-1);
    if ((bmaj && bdevseg[bmaj]) || (cmaj && cdevseg[cmaj]))
        return (-1);
    if ((bmaj && hdr->bvec == 0) || (cmaj && hdr->cvec == 0))
        return (-1);

    if (hdr->bvec) {
        bv.open = hdr->bvec->open;
        bv.close = hdr->bvec->close;
        bv.strat = hdr->bvec->strat;
        bv.ioctl = hdr->bvec->ioctl;
    }
    if (hdr->cvec) {
        cv.open = hdr->cvec->open;
        cv.close = hdr->cvec->close;
        cv.read = hdr->cvec->read;
        cv.write = hdr->cvec->write;
        cv.mode = hdr->cvec->mode;
    }

    if (bmaj)
        bdevseg[bmaj] = seg;
    if (cmaj)
        cdevseg[cmaj] = seg;

    if (init && ovlcall1(init, seg, seg) != 0) {
        if (bmaj)
            bdevseg[bmaj] = 0;
        if (cmaj)
            cdevseg[cmaj] = 0;
        return (-1);
    }

    if (bmaj)
        bvecset(bmaj, bv.open, bv.close, bv.strat, bv.ioctl);
    if (cmaj)
        cvecset(cmaj, cv.open, cv.close, cv.read, cv.write, cv.mode);
    if (tick)
        tickset(tick, seg);
    if (intr)
        intrset(probe, intr, seg);

    return (0);
}

/*
 * Place a module.  The segment is the whole of the argument: a header is
 * at OVLBASE by definition, so a pointer to one says only which page is
 * mapped at the moment, which is not necessarily the page being placed.
 */
int
ovlattach(seg)
    UINT seg;
{
    ovlmapseg(seg);             /* u.drv too: an interrupt may cut in */
    return (ovlplace((struct ovlhdr *) OVLBASE, seg));
}

/*
 * The driver the kernel carries in its own text: the one setdev stamped
 * into the slot (cmd/setdev/setdev.c), which is the driver for the
 * device it was booted from and no other.  It is placed from here rather
 * than from a module the loader reads, because there is nothing to read:
 * the stamped page is kernel text like any other, loaded by the loader
 * that already runs, at OVLBASE where it is linked.  All that is missing
 * at this point is that the kernel has not been told the header is
 * there, and that is this call.
 *
 * The segment is OVLSEG because that is where the page is: the slot is
 * the page of the kernel's address space the frame occupies, so the
 * identity map the kernel runs under already has it mapped, and
 * ovlbmap's mapping of a module placed here is the save and restore of a
 * register that never changes value.  A driver loaded later from
 * elsewhere - out of the kernel's own file, which is what _kino is for -
 * would be copied into a segment of its own and placed with ovlattach()
 * at that segment; it is the same table either way, and the same header.
 *
 * A kernel that was linked but never stamped has a page of zeros here,
 * whose major is 0 - not a major anything serves - so ovlplace refuses
 * it and the kernel comes up with no disk drivers at all.  That is the
 * honest answer for an image with no driver in it, and it is loud: the
 * mount of the root device is what fails.
 *
 * Called from cus() (sys/cus.c), inside the di()/ei() window the board's
 * bring-up already holds, so the driver's hardware setup happens exactly
 * where it used to.
 */
int
ovlstart()
{
    return (ovlattach(OVLSEG));
}
