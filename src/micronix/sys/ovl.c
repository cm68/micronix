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
#include <sys/ovl.h>
#include <errno.h>

extern UINT nbdev, ncdev;
extern UINT8 image0[], map0[];
extern char devname[][11];
extern unsigned bmajor(), cmajor();
extern int nodev(), iodone();

#define	OVLMAJ	16			/* majors we keep a place for */

static UINT ovlseg[OVLMAJ];		/* each major's module segment, 0 = none */
static struct ovlhdr *ovlloc[OVLMAJ];	/* a resident driver's header, in the kernel */
static struct biovec ovlbvec[OVLMAJ];	/* entries its init() registered */
static struct ciovec ovlcvec[OVLMAJ];
static int (*ovlintc[NOVLINE]) ();	/* each line's handler ... */
static UINT ovlintseg[NOVLINE];		/* ... and the module it lives in */
static int (*ovltickc[OVLMAJ]) ();	/* each major's tick, 0 = none */
static char ovltickarmed = 0;		/* the resident tick is running */

/*
 * Call something that lives in a module: put its page in place for the
 * call and put the interrupted map back afterwards.  This is the whole
 * of what an interrupt handler and a tick both need, which is why they
 * share it.  Segment 0 is a driver the kernel still holds - resident
 * code, always mapped, nothing to do - and no module is ever placed in
 * segment 0, which is the cpu board's own ram and prom below the bottom
 * of main memory.
 *
 * The map is written the way newmap() writes it (sys/malloc.c): image0
 * is the kernel's readable copy of the map, the registers themselves
 * being write-only.
 */
static
ovlcall(fn, seg)
    int (*fn) ();
    UINT seg;
{
    UINT8 save;

    if (seg == 0) {
        (*fn) ();
        return (0);
    }

    save = image0[2 * OVLSEG];
    map0[2 * OVLSEG] = image0[2 * OVLSEG] = (UINT8) seg;
    (*fn) ();
    map0[2 * OVLSEG] = image0[2 * OVLSEG] = save;
    return (0);
}

/*
 * Map a major's module and return the header it starts with, or 0 if the
 * major has no driver at all.  A driver that is still linked into the
 * kernel is that first case with nothing to map: its header is a
 * resident object and its segment is 0, so the header in the kernel is
 * what is returned and no map register is touched.  The map is written
 * the way newmap() writes it (sys/malloc.c): image0 is the kernel's
 * readable copy of the map, the registers themselves being write-only.
 */
struct ovlhdr *
ovlmap(maj)
    UINT maj;
{
    if (maj >= OVLMAJ)
        return (0);
    if (ovlseg[maj] == 0)
        return (ovlloc[maj]);
    map0[2 * OVLSEG] = image0[2 * OVLSEG] = (UINT8) ovlseg[maj];
    return ((struct ovlhdr *) OVLBASE);
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

    hdr = ovlmap(maj);
    return (hdr ? hdr->data : (char *) 0);
}

/*
 * The block trampolines.  Each recovers its major from the device it was
 * handed, which every call site passes (bopen, bclose, strat in
 * sys/uio.c), and that is why three of them serve every major rather
 * than three for each.  A major with no driver behind it answers the way
 * the table's nodev row does, so an unloaded disk is indistinguishable
 * from one that was never there - except that a request against it
 * completes with an error rather than hanging, which is what a major
 * that may be loaded later has to do.
 *
 * The entry in the table is what says whether a driver is there, not the
 * segment: a driver still linked into the kernel has entries and no
 * segment, and needs none mapped.  ovlmap does whatever mapping there is
 * to do and its answer is not a test.
 */
int
ovlopen(dev, mode)
    UINT dev, mode;
{
    UINT maj;

    maj = bmajor(dev);
    ovlmap(maj);
    if (ovlbvec[maj].open == 0) {
        nodev();
        return (0);
    }
    return ((*ovlbvec[maj].open) (dev, mode));
}

int
ovlclose(dev, mode)
    UINT dev, mode;
{
    UINT maj;

    maj = bmajor(dev);
    ovlmap(maj);
    if (ovlbvec[maj].close == 0) {
        nodev();
        return (0);
    }
    return ((*ovlbvec[maj].close) (dev, mode));
}

int
ovlstrat(b)
    register struct buf *b;
{
    UINT maj;

    maj = bmajor(b->dev);
    ovlmap(maj);
    if (ovlbvec[maj].strat == 0) {
        b->flags |= BERROR;
        b->error = ENXIO;
        iodone(b);
        return (0);
    }
    return ((*ovlbvec[maj].strat) (b));
}

/*
 * The character trampolines, the same shape against ciosw[] (sys/cio.c).
 */
int
ovlcopen(dev, mode)
    UINT dev, mode;
{
    UINT maj;

    maj = cmajor(dev);
    ovlmap(maj);
    if (ovlcvec[maj].open == 0) {
        nodev();
        return (0);
    }
    return ((*ovlcvec[maj].open) (dev, mode));
}

int
ovlcclose(dev, mode)
    UINT dev, mode;
{
    UINT maj;

    maj = cmajor(dev);
    ovlmap(maj);
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
    UINT maj;

    maj = cmajor(dev);
    ovlmap(maj);
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
    UINT maj;

    maj = cmajor(dev);
    ovlmap(maj);
    if (ovlcvec[maj].write == 0) {
        nodev();
        return (0);
    }
    return ((*ovlcvec[maj].write) (dev));
}

int
ovlcmode(dev, flag)
    UINT dev, flag;
{
    UINT maj;

    maj = cmajor(dev);
    ovlmap(maj);
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
bvecset(maj, open, close, strat)
    UINT maj;
    int (*open) ();
    int (*close) ();
    int (*strat) ();
{
    if (maj == 0 || maj >= nbdev || maj >= OVLMAJ)
        return (-1);
    ovlbvec[maj].open = open;
    ovlbvec[maj].close = close;
    ovlbvec[maj].strat = strat;
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
 * Claim an interrupt line.  The segment comes with the handler because a
 * handler's address is only an address while its module is the one
 * mapped, and the dispatcher below has to do the mapping itself.
 */
static
intrset(line, fn, seg)
    UINT line, seg;
    int (*fn) ();
{
    if (line >= NOVLINE)
        return (-1);
    ovlintc[line] = fn;
    ovlintseg[line] = seg;
    return (0);
}

/*
 * The name pcon() prints (sys/main_init.c).  devname[] is in the u page
 * and 11 bytes wide, so a name that fills it has to lose its last
 * character to the terminator; the header's field is 12 to allow that.
 */
static
nameset(maj, nm)
    UINT maj;
    char *nm;
{
    register char *p;
    register UINT i;

    if (maj >= nbdev)
        return (-1);
    p = devname[maj];
    for (i = 0; i < 10 && nm[i]; i++)
        p[i] = nm[i];
    p[i] = 0;
    return (0);
}

/*
 * An interrupt does not arrive inside a call, the way a strat() does: it
 * can land at any moment, with any module mapped or none.  So the line
 * carries the segment its handler lives in, and the line - not the
 * interrupted process - decides what is mapped.  That is what lets the
 * interrupt path work with one overlay page.
 */
static
ovlntr(line)
    UINT line;
{
    if (ovlintc[line] == 0)
        return (0);
    return (ovlcall(ovlintc[line], ovlintseg[line]));
}

int
ovlint0()
{
    return (ovlntr(0));
}

int
ovlint1()
{
    return (ovlntr(1));
}

int
ovlint2()
{
    return (ovlntr(2));
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
    register UINT maj;

    for (maj = 1; maj < OVLMAJ; maj++)
        if (ovltickc[maj])
            ovlcall(ovltickc[maj], ovlseg[maj]);

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
 * Ask for a tick.  A module does this in its header; a driver the kernel
 * still holds has no header to say it in, so it says it here - the same
 * duplication nameset has with the header's name field, for the same
 * reason.  Zero withdraws the request.
 */
static
tickset(maj, fn)
    UINT maj;
    int (*fn) ();
{
    if (maj == 0 || maj >= OVLMAJ)
        return (-1);
    ovltickc[maj] = fn;
    if (fn)
        ovlarm();
    return (0);
}

/*
 * Place a driver.  The caller has the header either way: the loader has
 * copied a module's page into seg and its header is at OVLBASE by
 * definition, and a driver the kernel still holds has its header in the
 * kernel, where the resident table at the bottom of this file names it.
 *
 * Everything the header is worth is taken from it only after init()
 * succeeds, because init() is the driver's answer to whether it is here
 * at all: a driver whose hardware is not on this machine returns non-zero
 * and is not registered, and its major goes on answering as nodev does.
 * That is what a kernel that carries a driver for a card this machine
 * does not have looks like from the inside - the page is spent, nothing
 * else is.
 *
 * The placement itself has to happen first, because init() is what has to
 * find the driver's own data: ovldata() maps the major (ovlmap above),
 * and for a driver the kernel still holds that is this header.  Failing
 * to init unplaces it again, and nothing else was taken.
 */
static
ovlplace(hdr, seg)
    struct ovlhdr *hdr;
    UINT seg;
{
    UINT maj;

    maj = hdr->major;
    if (maj == 0 || maj >= OVLMAJ)
        return (-1);
    if (maj >= nbdev && maj >= ncdev)
        return (-1);

    ovlseg[maj] = seg;
    if (seg == 0)
        ovlloc[maj] = hdr;

    if (hdr->init && (*hdr->init) (seg) != 0) {
        ovlseg[maj] = 0;
        ovlloc[maj] = 0;
        return (-1);
    }

    tickset(maj, hdr->tick);
    nameset(maj, hdr->name);
    if (hdr->bvec)
        bvecset(maj, hdr->bvec->open, hdr->bvec->close, hdr->bvec->strat);
    if (hdr->cvec)
        cvecset(maj, hdr->cvec->open, hdr->cvec->close, hdr->cvec->read,
                hdr->cvec->write, hdr->cvec->mode);
    if (hdr->intr)
        intrset(hdr->line, hdr->intr, seg);

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
    map0[2 * OVLSEG] = image0[2 * OVLSEG] = (UINT8) seg;
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
 * identity map the kernel runs under already has it mapped, and ovlmap's
 * mapping of a module placed here is the save and restore of a register
 * that never changes value.  A driver loaded later from elsewhere - out
 * of the kernel's own file, which is what _kino is for - would be copied
 * into a segment of its own and placed with ovlattach() at that segment;
 * it is the same table either way, and the same header.
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
