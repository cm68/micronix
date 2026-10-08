/*
 * Micronix driver for the CompuPro DISK 3 hard disk controller.
 *
 * sys/disk3.c
 *
 * The DISK 3 is an intelligent ST-506 controller (on board 8085 plus
 * firmware), so the driver has no real time work beyond the DMA rate: it
 * builds a sixteen byte IOPB in memory, writes the attention port, and the
 * board copies the IOPB, runs the command, transfers the data to or from
 * the address the IOPB names, and writes the status back.  The interface
 * is include/sys/disk3.h; this file is the same shape as sys/mw.c because
 * the two controllers are the same shape - an in-memory command block, an
 * attention port, and DMA - the DISK 3 just folds the channel-address port
 * into the IOPB's own LINK field.
 *
 * The strategy path is simpler than the HD-DMA's for the same reason the
 * controller is: seek and home are firmware commands, not step counts, and
 * a read or a write addresses an absolute sector and lets the board work
 * out where the head has to be.  So the driver sends GLOBAL once, SPECIFY
 * per drive, HOME per drive, and then R/W commands that carry nothing but
 * the absolute sector, the count and the DMA address.
 */
#include <types.h>
#include <sys/sys.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <sys/con.h>
#include <sys/disk3.h>
#include <sys/ovl.h>
#include <sys/dlabel.h>
#include <errno.h>

/*
 * Declared before anything calls them: an undeclared call is extern int by
 * default and the definition says static.
 */
static int d3error();
static void d3link();
static void d3specify();
static UINT32 d3addr();

#define NDRIVES 4               /* Number of drives */
#define RETRIES 10              /* no. of retries on r/w error */
#define D3DEADLINE 1            /* ticks a request may sit in flight */

/*
 * The drive's timing, which the label does not carry - the same gap the
 * HD-DMA driver closes with its own constants, for the same reason
 * (docs/DISKLABEL.md).  These are the DISK 3's SPECIFY table's step and
 * settle times, in 100 us units, and the two cylinders at which writing
 * changes, both zero so the compensation is always on.
 */
#define D3STPDEL 30             /* step pulse delay, 100 us */
#define D3SETTLE 0              /* head settle after seek, 100 us */
#define D3PRECOMP 0             /* cylinder where precompensation begins */
#define D3LOWCUR 0              /* cylinder where low write current begins */

/*
 * Drive configuration information, one per drive.
 *
 * The first seven fields are struct dlgeom (sys/dlabel.h), in its order
 * and of its types, because sys/dlabel.c decodes the label through a view
 * of them - disk3open passes this struct as one.  The bookkeeping below
 * them is this driver's own and may be moved freely; moving any of the
 * seven breaks the decode in a way the compiler will not report.
 */
struct info
{
    UINT tracks;                /* number of tracks per surface */
    UINT8 heads;                /* number of heads */
    UINT8 sectors;              /* number of sectors per track */
    UINT maxblk;                /* dllabel's bound for the first slice opened */
    UINT spc;                   /* number of sectors per cylinder */
    UINT roll;                  /* the drive's d_roll */
    UINT cylstart;              /* dllabel's offset for the first slice opened */
    UINT8 firstsec;             /* first sector number on a track */

    UINT curtrk;                /* not used: the board keeps this itself */
    UINT8 flags;                /* see below */

    /* the slice table off the label, cached at open */
    struct dslice d_slice[NSLICE];
} disk3s[NDRIVES] = 0;

/*
 * Info flags for above structure
 */
#define CALIB	1               /* drive is calibrated (specify + home done) */
#define OPEN	2               /* drive is open */
#define RECAL	4               /* drive is being re-calibrated */

/*
 * The one IOPB, and the SPECIFY table the board reads out of it.  Both are
 * data objects in this module, so both have to be handed to the board as
 * physical addresses - d3addr below, the same conversion the HD-DMA driver
 * makes for its command block.
 */
static struct disk3_iopb iopb;
static struct disk3_spec d3spec;

/*
 * Globals
 */
static struct info *d3info = 0; /* current drive */
static struct buf *d3buf = 0;   /* current io request */
static UINT8 retry = 0,         /* number of retries so far */
    curdrv = -1,                /* number of current drive */
    d3state = 0,                /* see below */
    d3dead = 0;                 /* ticks the request has been in flight */

/*
 * States for d3state (above)
 */
#define VIRGIN	0
#define STOPPED	1
#define ACTIVE	2
#define INTRPT	3
#define TOOLATE 5

/*
 * Device open.  Identical to the HD-DMA's: the drive is described by its
 * label and by nothing else, so the label is read once per drive and its
 * geometry and slice table cached.  What the drive *is* comes from
 * sys/dlabel.c through the first seven fields of struct info.
 */
disk3open(dev, mode)
    UINT dev, mode;
{
    static struct info *info;
    static UINT8 drive, sl;
    static struct buf *b;

    drive = dev & 3;
    sl = devslice(dev);
    if (drive >= NDRIVES) {
        u.error = ENXIO;
        return;
    }
    info = &disk3s[drive];
    if (info->flags & OPEN)
        return;

    info->tracks = 0;
    info->heads = 0;
    info->sectors = 0;
    info->roll = 0;

    switch (dllabel((dev & ~0377) | (DL_WHOLE << 5) | drive,
                    sl, (struct dlgeom *)info, info->d_slice)) {
    case -1:
        return;                 /* no such slice; dllabel set u.error */
    case 0:
        if (sl != DL_WHOLE) {
            u.error = ENXIO;
            return;
        }
        info->heads = 1;
        info->sectors = 1;
        info->spc = 1;
        info->maxblk = 0;
        info->flags |= OPEN;
        return;
    default:
        break;                  /* the label was read; the geometry is set */
    }

    if ((b = bread(1, dev)) != 0) {
        info->flags |= OPEN;
        brelse(b);
    }
}

/*
 * Device close.  The cache has been flushed; nothing else to do - the
 * board has no light to turn off and no heads to park.
 */
disk3close(dev)
    int dev;
{
    struct info *info;

    info = &disk3s[dev & 3];
    info->flags &= ~OPEN;
}

/*
 * Strategy
 */
disk3strat(b)
    register struct buf *b;
{
    register struct info *info;
    static UINT sl, off, roll, maxblk;

    info = &disk3s[b->dev & 3];
    sl = devslice(b->dev);
    if (dlslice((struct dlgeom *)info, info->d_slice, sl,
                &off, &roll, &maxblk) || b->blk > maxblk) {
        b->flags |= BERROR;
        b->error = ENXIO;
        iodone(b);
        return;
    }
    b->cyl = d3cyl(b->blk, info->spc, off, roll, info->tracks);
    di();
    if (d3buf == 0)
        d3buf = b;
    else
        bsort(d3buf, b);
    if (d3state < ACTIVE)
        d3start();
    ei();
}

/*
 * Goose the lower half.  The first request brings the board up and sends
 * GLOBAL; after that each request picks the drive and reads or writes.
 */
d3start()
{
    if (!busget(&d3start))
        return;
    if (d3state == VIRGIN) {
        reset();
        /* GLOBAL: absolute sector addressing, retries, drive count */
        iopb.command = D3_GLOBAL;
        iopb.drive = 0;
        iopb.arg[0] = D3_ABS;
        iopb.arg[1] = RETRIES;
        iopb.arg[2] = NDRIVES;
        d3wait();
    }
    newdrv();
}

/*
 * Stop the controller.  It halts on its own after a command without the
 * continue bit, so stopping is just the state and the bus.
 */
d3stop()
{
    d3state = STOPPED;
    busgive(0);
}

/*
 * Interrupt service begins here.
 */
disk3int()
{
    if (d3state == STOPPED || d3state == INTRPT || iopb.status == D3_BUSY)
        return;
    d3state = INTRPT;
    rwint();
}

/*
 * Service a read/write interrupt.  The board's status byte says how the
 * command went; 0xff is success, and anything else is an error - except
 * that a drive that dropped ready is a terminal error rather than a retry.
 */
rwint()
{
    if (iopb.status == D3_OK) {
        advque();
        return;
    }
    if (iopb.status != D3_NOTRDY) {
        if (retry-- == RETRIES / 2) {
            d3info->flags &= ~CALIB;
            d3info->flags |= RECAL;
            newdrv();
            return;
        } else if (retry) {
            issue();
            return;
        }
    }
    d3error();
    advque();
}

/*
 * Advance the queue to the next request
 */
advque()
{
    static struct buf *f;

    iodone(d3buf);
    f = d3buf->forw;
    d3buf->forw = 0;
    d3buf = f;
    if (d3buf == 0)
        d3stop();
    else if (curdrv != (d3buf->dev & 3))
        newdrv();
    else
        newrw();
}

/*
 * Select a new drive: SPECIFY its geometry, then HOME it if this is the
 * first time it has been selected.  Both are polled; the board steps and
 * seeks on its own.
 */
newdrv()
{
    curdrv = d3buf->dev & 3;
    d3info = &disk3s[curdrv];
    if (d3info->flags & CALIB) {
        newrw();
        return;
    }
    d3specify(curdrv);
    iopb.command = D3_HOME;
    iopb.drive = curdrv;
    d3wait();
    d3info->curtrk = 0;
    d3info->flags |= CALIB;
    newrw();
}

/*
 * Set up a new read/write command.  One sector per command - a buffer is
 * 512 bytes, which is exactly what the drive's sector holds - so the count
 * argument is always one and the whole transfer finishes in one interrupt.
 *
 * The sector is absolute: the physical cylinder (resolved into b->cyl by
 * d3strat, with the slice offset and the roll already in) times the
 * sectors per cylinder, plus the sector's place in that cylinder.  That
 * product is 24 bits on a large drive, so it is built in a UINT32 and
 * spread across ARG2-ARG5 low byte first.
 */
newrw()
{
    static UINT32 abs;
    static UINT spc;

    spc = d3info->spc;
    abs = ((UINT32) d3buf->cyl * spc) + (d3buf->blk % spc);
    iopb.command = D3_RW | D3_INT;
    iopb.drive = curdrv;
    iopb.arg[0] = (d3buf->flags & BREAD) ? D3_READ : D3_WRITE;
    iopb.arg[1] = (UINT8) abs;
    iopb.arg[2] = (UINT8) (abs >> 8);
    iopb.arg[3] = (UINT8) (abs >> 16);
    iopb.arg[4] = (UINT8) (abs >> 24);
    iopb.arg[5] = 1;            /* sector count, low */
    iopb.arg[6] = 0;            /* sector count, high */
    iopb.data = (UINT8) (unsigned) d3buf->data;
    iopb.xdata = (UINT8) ((unsigned) d3buf->data >> 8);
    iopb.sdata = d3buf->xmem;
    if (d3info->flags & RECAL)
        d3info->flags &= ~RECAL;
    else
        retry = RETRIES;
    issue();
}

/*
 * Which cylinder a filesystem block lands on - the same arithmetic the
 * HD-DMA driver uses, because the disk and the label are the same even
 * when the controller is not.  The cylinder is the block divided by the
 * sectors per cylinder, shifted by the slice's offset and rotated by the
 * drive's roll, both terms added as cylinders after the division.
 */
d3cyl(blk, spc, off, roll, tracks)
    UINT blk, spc, off, roll, tracks;
{
    UINT cyl;

    cyl = blk / spc + off + roll;
    if (cyl >= tracks)
        cyl -= tracks;
    return cyl;
}

/*
 * Flag an error
 */
static
d3error()
{
    d3buf->flags |= BERROR;
}

/*
 * Build a drive's SPECIFY table from its geometry and send it.  The table
 * is a data object in this module, so the board is handed its physical
 * address in the IOPB's DATA field - d3addr, below.
 */
static void
d3specify(drive)
    UINT drive;
{
    static UINT32 a;

    d3spec.steprate = D3STPDEL;
    d3spec.settle = D3SETTLE;
    d3spec.secsize = 512;
    d3spec.sectrk = d3info->sectors;
    d3spec.trkcyl = d3info->heads;
    d3spec.maxtrk = d3info->tracks;
    d3spec.precomp = D3PRECOMP;
    d3spec.reduce = D3LOWCUR;
    d3spec.reserved = 0;
    d3spec.restracks = 0;
    d3spec.position = 0;
    a = d3addr((char *) &d3spec);
    iopb.command = D3_SPECIFY;
    iopb.drive = drive;
    iopb.data = (UINT8) a;
    iopb.xdata = (UINT8) (a >> 8);
    iopb.sdata = (UINT8) (a >> 16);
    d3wait();
}

/*
 * Bring the driver up: remember which page the kernel placed it in, and
 * work out where the IOPB really is.  Nothing here touches the controller
 * - its reset is the first d3start, which is what mwinit's comment argues
 * for the HD-DMA and what applies here verbatim: the board follows a
 * physical address, and the address this driver's own code computes for
 * the IOPB is the window one, so a placed module has to convert.
 */
static UINT d3seg = 0;          /* the page we were placed in, 0 = resident */

disk3init(seg)
    UINT seg;
{
    d3seg = seg;
    d3link();
    return (0);
}

/*
 * The physical address of a data object in this module: (page << 12) |
 * offset.  The field is 24 bits and an unsigned is 16, so the shift is
 * done in a UINT32.
 */
static UINT32
d3addr(p)
    char *p;
{
    if (d3seg)
        return ((UINT32) d3seg << 12) | ((unsigned) p & 0xfff);
    return (unsigned) p;
}

/*
 * Point the board at the IOPB.  The board fetches its next IOPB from the
 * LINK field of the previous one, so the field is set to the IOPB's own
 * physical address and left there - a single-IOPB driver.
 */
static void
d3link()
{
    static UINT32 a;

    a = d3addr((char *) &iopb);
    iopb.link = (UINT8) a;
    iopb.xlink = (UINT8) (a >> 8);
    iopb.slink = (UINT8) (a >> 16);
}

/*
 * Reset (initialize) the controller.  Writing 0xff holds the 8085 in
 * reset; while it is held, the LINK of the initial IOPB is written at
 * physical 0x5d; the first attention fetches that initial IOPB and keeps
 * only its LINK, and the second fetches and runs the real IOPB.  That
 * double attention is only needed once after a reset.
 */
reset()
{
    extern char map0[], image0[];
    static UINT32 a;
    int *ichan;

    d3link();
    a = d3addr((char *) &iopb);
    ichan = 0x105d;             /* the initial IOPB's LINK, physical 0x5d */
    di();
    map0[2] = 0;
    ichan[0] = (UINT) a;        /* the low two bytes */
    ichan[1] = (UINT) (a >> 16);/* and the high byte */
    map0[2] = image0[2];
    inton(DISK3INT);
    ei();
    out(D3_ATTN, D3_RESET);     /* hold the 8085 in reset */
    out(D3_ATTN, D3_GO);        /* first attention: fetch the initial IOPB */
    out(D3_ATTN, D3_GO);        /* second attention: fetch and run the IOPB */
}

/*
 * Kick the controller: mark the IOPB busy and write the attention port.
 */
attn()
{
    iopb.status = D3_BUSY;
    out(D3_ATTN, D3_GO);
}

/*
 * Issue a command whose completion will be serviced by an interrupt.
 */
issue()
{
    d3state = ACTIVE;
    d3dead = 0;
    attn();
}

/*
 * Issue a command and wait for completion.
 */
d3wait()
{
    attn();
    while (iopb.status == D3_BUSY);
}

/*
 * Simulate missing timeout hardware, as the HD-DMA's mwcheck does: the
 * resident tick walks the modules, and this is the one-second check that
 * turns a silent board into NOTRDY instead of a hang.  VIRGIN, STOPPED and
 * INTRPT change nothing; a request in flight counts its ticks and only a
 * board that has really stopped answering reaches TOOLATE.
 */
disk3check()
{
    di();
    switch (d3state) {
    case ACTIVE:
        if (++d3dead <= D3DEADLINE)
            break;
        d3state = TOOLATE;
        reset();
        curdrv = -1;            /* force re-selection */
        iopb.status = D3_NOTRDY;
        break;
    case TOOLATE:
        panic("disk3 hung");
    }
    ei();
    if (d3state == TOOLATE)
        disk3int();
}

/*
 * Enumerate the drives at signon: read each unit's label and print the
 * manufacturer, model and capacity of every drive that answers.  The major
 * is 6, from the header (sys/disk3hdr.c), and the 'c' slice names block 0.
 * A unit with nothing behind it does not answer, and the tick's watchdog
 * turns that silence into NOTRDY in a couple of seconds.
 */
disk3probe()
{
    struct info *info;
    struct buf *b;
    struct dlabel *lp;
    char *p;
    UINT dev, unit;
    UINT mb;

    for (unit = 0; unit < NDRIVES; unit++) {
        info = &disk3s[unit];
        dev = (6 << 8) | (DL_WHOLE << 5) | unit;
        if (dllabel(dev, DL_WHOLE, (struct dlgeom *)info, info->d_slice) != 1)
            continue;
        b = bread(0, dev);
        if (b == 0)
            continue;
        p = bhold(b);
        lp = (struct dlabel *) &p[DL_OFFSET];
        mb = (UINT) ((UINT32) info->tracks * info->heads * info->sectors >> 11);
        pr("disk3%d: %s %s, %d MB\n", unit, lp->d_mfg, lp->d_model, mb);
        brel();
        brelse(b);
    }
    return (0);
}

/*
 * The entries the driver's header hands the kernel (sys/disk3hdr.c).  It
 * is a different object because a module's first bytes have to be the
 * header's, and an object's data is placed in the order the objects are
 * named - so the header is alone in the object named first, and this is
 * the driver's own.  No ioctl yet: the raw-IOPB command the format program
 * needs is a later piece.
 */
struct biovec disk3bvec = { &disk3open, &disk3close, &disk3strat, 0 };

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
