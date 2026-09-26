/*
 * Micronix driver for HD-DMA
 * Gary Fitts, Morrow Designs
 * See the HD-DMA manual for more information.
 *
 * sys/mw.c 
 * Changed: <2021-12-23 15:22:37 curt>
 */

#include <types.h>
#include <sys/sys.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <sys/con.h>
#include <sys/mw.h>
#include <sys/ovl.h>
#include <sys/dlabel.h>
#include <errno.h>

/*
 * Declared before anything calls it: an undeclared call is extern int
 * by default and the definition says static.
 */
static int error();

#define NDRIVES 4               /* Number of drives. See mws[] below. */
#define SECSIZE 3               /* Sector size (512 bytes) */
#define RETRIES 10              /* no. of retries on r/w error */

/*
 * The row of specs[] that dev/devlist's hd<N>c nodes name, and so the
 * type field of the device number mwopen builds for a drive's whole-disk
 * slice.  It picks no row here: mws[] is keyed by drive and already
 * holds the row the caller's minor named, so this one never reaches the
 * controller, and the label's own sector is at cylinder 0 with no step
 * to take.  It is part of the *name* of that sector and nothing else -
 * and the label is read and written as the disk block it is, so a tool
 * opening hd0c and this driver reading drive 0's label must name it the
 * same way, or the cache will hold two copies of the one sector.
 */
#define HDCROW  2

/*
 * Drive specfications.
 * Copied into info structure (below) during open.
 */
struct spec
{
    UINT tracks;                /* number of tracks per surface */
    UINT8 heads;                /* number of heads */
    UINT8 sectors;              /* number of sectors per track */
    UINT8 stpdel;               /* delay between step pulses, 100 us */
    UINT precomp;               /* track where precomp begins */
    UINT lowcur;                /* track where low current begins */
} specs[] = {
    {153, 4, 17, 30, 128, 128}, /* Seagate 5 meg */
    {306, 4, 17, 2, 128, 128},  /* generic 10 meg */
    {306, 6, 17, 2, 128, 128},  /* CMI 16 meg */
    {640, 6, 17, 0, 256, 256},  /* CMI 32 meg */
    {733, 5, 17, 30, 300, 733}, /* Seagate 40 meg */
};

/*
 * Drive configuration information, one per drive.
 *
 * The first seven fields are struct dlgeom (sys/dlabel.h), in its order
 * and of its types, because sys/dlabel.c decodes the label through a
 * view of them - mwopen passes this struct as one.  The timings and the
 * bookkeeping below them are this driver's own and may be moved freely;
 * moving any of the seven breaks the decode in a way the compiler will
 * not report.
 */
struct info
{
    UINT tracks;                /* number of tracks per surface */
    UINT8 heads;                /* number of heads */
    UINT8 sectors;              /* number of sectors per track */
    UINT maxblk;                /* max legal block number in the slice */
    UINT spc;                   /* number of sectors per cylinder */
    UINT roll;                  /* what mwcyl adds to blk / spc */
    UINT cylstart;              /* first cylinder of the slice being read */

    UINT8 stpdel;               /* delay between step pulses, 100 us */
    UINT precomp;               /* track where precomp begins */
    UINT lowcur;                /* track where low current begins */
    UINT curtrk;                /* current track */
    UINT8 flags;                /* see below */
    UINT8 type;                 /* index into specs table above */
    UINT8 slice;                /* the slice this open is bound to */
} mws[NDRIVES] = 0;

/*
 * Info flags for above structure
 */
#define CALIB	1               /* drive is calibrated */
#define OPEN	2               /* drive is open */
#define RECAL	4               /* drive is being re-calibrated */

/*
 * Controller command structure
 * (with additions)
 * See HD-DMA manual
 */
struct
{
    UINT8 seksel;               /* out<<4 | drv */
    UINT steps;                 /* number of steps */
    UINT8 hedsel;               /* pcmp<<7 | hicur<<6 | (~head&7)<<2 | drv */
    UINT dma;                   /* dma address */
    UINT8 xdma;                 /* high byte of 24-bit address */

    union
    {
        UINT word;
        struct
        {
            UINT8 low;
            UINT8 high;
        }
        byte;
    }
    arg0;
    UINT8 arg2;
    UINT8 arg3;

    UINT8 op;                   /* op code */
    UINT8 stat;                 /* completion status */
    UINT link;                  /* address of next command */
    UINT8 xlink;                /* high byte of address */

    UINT count;                 /* number of bytes to transfer */
    UINT blk;                   /* block no. */
} cmd = 0;

/*
 * Controller commands
 */
#define READS		0
#define WRITES		1
#define READH		2
#define FORM		3
#define LOAD		4
#define STAT		5
#define HOME		6

/*
 * Controller constants
 */
#define HOMDEL	30              /* step pulse delay during home in 100 us */
#define SETTLE	0               /* controller head-settle time */
#define INT	0x80            /* Interrupt enable bit with step delay */
#define RESET	0x54            /* Reset to controller */
#define ATTN	0x55            /* Attention to controller */
#define PRECOMP 0x80            /* Write precompensation */
#define HIGHCUR 0x40            /* Use high write current */
#define STEPOUT 0x10            /* step out toward track 0 */
#define LCONST	0x30            /* must be set for LOAD constants command */
#define NOTRDY	4               /* bit 2 of sense status */
#define BUSY	0               /* controller is busy */
#define OK	0xff            /* operation completed w/o error */
#define INTOFF	0               /* turn off completion interrupts */

/*
 * Decision interrupt controller.
 * See the Wunderbus I/O or Mult I/O manual.
 */
#define BASE	0x48            /* Base address of WB I/O */
#define PIC1	BASE+5
#define GRPSEL	BASE+7
#define PICMASK 050             /* enable PIC interrupts */
#define VI	1               /* Interrput 0 (PIC mask bit) */

/*
 * Globals
 */
static struct info *mwinfo = 0; /* current drive */
static struct buf *mwbuf = 0;   /* current io request */
static UINT8 retry = 0,         /* number of retries so far */
    curdrv = -1,                /* number of current drive */
    mwstate = 0;                /* see below */

/*
 * States for mwstate (above)
 */
#define VIRGIN	0
#define STOPPED	1
#define ACTIVE	2
#define INTRPT	3
#define LATE	4
#define TOOLATE 5

/*
 * Device open
 *
 * Two things can say what the drive is, and they are not equal.  The disk
 * can say: the label mkfs writes at physical cylinder 0, head 0, sector 0
 * is marked with a magic number and sits at the one address that can be
 * found knowing nothing at all, so if it is there it is believed and
 * used.  The minor number can say: it names a row of specs[] below, which
 * is what is fallen back on for a disk laid down by something that wrote
 * no label.  Neither is a reason to refuse on its own - a disk with a
 * label mounts from the label whatever the minor names, and a minor
 * naming no row mounts from the label alone.  Only a disk with neither is
 * refused, because then there is no geometry and no block number can be
 * mapped at all.
 *
 * The label itself, and the slice it names, are decoded in sys/dlabel.c,
 * which knows nothing about this controller - what is here is the table
 * that stands in for a label, and the handling of a disk that has none.
 */
mwopen(dev, mode)
    UINT dev, mode;
{
    static struct info *info;
    static UINT8 drive, type;
    static UINT nspec, sl;
    static struct buf *b;

    drive = dev & 3;
    type = devtype(dev);
    sl = devslice(dev);
    if (drive >= NDRIVES) {
        u.error = ENXIO;
        return;
    }
    info = &mws[drive];
    if ((info->flags & CALIB) && (info->type != type)) {
        u.error = ENXIO;
        return;
    }
    /*
     * One mapping per drive, not one per open - mws[] is keyed by drive
     * and holds the slice's offset and roll - so a second open asking for
     * a different slice would be served the first one's mapping and would
     * read and write the wrong cylinders while reporting success.  That
     * is the one failure this format is meant to make impossible, so it
     * is refused.  The same slice, or the same minor, is the same mapping
     * and is let through as before.
     */
    if (info->flags & OPEN) {
        if (info->type != type || info->slice != sl)
            u.error = EBUSY;
        return;
    }
    info->type = type;
    info->slice = sl;

    /*
     * What the minor number says the drive is.  If it names a real row in
     * specs[], copy it, geometry and timing together; otherwise leave the
     * geometry zero and pick conservative controller tuning - the label
     * carries only tracks/heads/spt/roll and not the three drive-specific
     * timings.  The label, if there is one, replaces the geometry below
     * either way; the timing is never the label's to give.
     *
     * The fields are copied one by one rather than as a block: struct
     * info is struct dlgeom first (above) and struct spec is not, so the
     * two no longer start alike.
     */
    nspec = sizeof specs / sizeof specs[0];
    if (type < nspec) {
        info->tracks = specs[type].tracks;
        info->heads = specs[type].heads;
        info->sectors = specs[type].sectors;
        info->stpdel = specs[type].stpdel;
        info->precomp = specs[type].precomp;
        info->lowcur = specs[type].lowcur;
        info->roll = info->tracks >> 1;
    } else {
        info->tracks = 0;
        info->heads = 0;
        info->sectors = 0;
        info->stpdel = 30;      /* the boot loader's step delay */
        info->precomp = 0;
        info->lowcur = 0;
        info->roll = 0;
    }

    /*
     * The label, as block 0 of this drive's 'c'.  The device number is
     * built here rather than taken from dev, because the type field of a
     * 'c' number is not the caller's - it names the same sector devlist's
     * hd<N>c node names, so that a tool labelling this drive and this
     * open reading it hold one cache entry between them rather than two
     * copies of one sector.
     */
    switch (dllabel((dev & ~0377) | (DL_WHOLE << 5) | (HDCROW << 2) | drive,
                    sl, (struct dlgeom *)info)) {
    case -1:
        return;                 /* no such slice; dllabel set u.error */
    case 0:
        /*
         * No label on the disk.  A row of the table above stands for
         * one, and that is what a disk laid down by something that wrote
         * no label gets.  A disk with neither - no row and no label -
         * has no geometry at all, and then no block number can be
         * mapped.
         */
        if (type >= nspec) {
            u.error = ENXIO;
            return;
        }
        break;
    default:
        /*
         * Say so when the label and the table disagree.  The disk decides
         * how it is laid out and that is what is used, but the timing
         * above came from the row the minor named, and timing from the
         * wrong row is a seek that quietly misses.
         */
        if (type < nspec &&
            (info->tracks != specs[type].tracks ||
             info->heads != specs[type].heads ||
             info->sectors != specs[type].sectors))
            pr("mw%d: label %d/%d/%d disagrees with table %d/%d/%d, using label\n",
                drive, info->tracks, info->heads, info->sectors,
                specs[type].tracks, specs[type].heads, specs[type].sectors);
    }

    if ((b = bread(1, dev)) != 0) {
        info->flags |= OPEN;
        brelse(b);
    }
}

/*
 * Device close
 * Attempt to move the heads to the inside track.
 * Note that at this point, the cache has been flushed.
 */
mwclose(dev)
    int dev;
{
    struct info *info;
    static struct buf *b;

    info = &mws[dev & 3];
    b = bread(info->spc * info->roll, dev);
    if (b) {
        brelse(b);
        bzero(b);
    }
    info->flags &= ~OPEN;
}

/*
 * Reading the label used to be its own synchronous sequence here - reset,
 * load constants, home, one read, with interrupts off throughout - to get
 * the boot sector in before any block could be mapped.  It does not have
 * to be: the label sits at block 0 of the drive's 'c', which is a block
 * number like any other, and the strategy path below calibrates itself on
 * the first request it is given (mwstart, on VIRGIN).  So the label is
 * read with a bread() of that block and this driver has one path to a
 * sector instead of two.  sys/dlabel.c does the reading.
 */

/*
 * Strategy
 */
mwstrat(b)
    register struct buf *b;
{
    register struct info *info;

    info = &mws[b->dev & 3];
    if (b->blk > info->maxblk) {
        b->flags |= BERROR;
        b->error = ENXIO;
        iodone(b);
        return;
    }
    b->cyl = mwcyl(b->blk, info);
    di();
    if (mwbuf == 0)
        mwbuf = b;
    else
        bsort(mwbuf, b);
    if (mwstate < ACTIVE)
        mwstart();
    ei();
}

/*
 * Goose the lower half
 */
mwstart()
{
    if (!busget(&mwstart))
        return;
    if (mwstate == VIRGIN)
        reset();
    cmd.steps = 0;
    cmd.arg0.byte.high = INT;
    cmd.hedsel |= LCONST;
    cmd.op = LOAD;
    issue();
}

/*
 * Stop the controller
 */
mwstop()
{
    mwstate = STOPPED;
    cmd.steps = 0;
    cmd.arg0.byte.high = INTOFF;
    cmd.hedsel |= LCONST | 3;   /* select drive 4 - turn off light */
    cmd.op = LOAD;
    mwwait();
    busgive(0);
}

/*
 * Interrupt service begins here.
 * Some of this relies on the interrupt controller's
 * ability to screen further controller interrupts
 * until the current service is done.
 */
mwint()
{
    if (mwstate == STOPPED || mwstate == INTRPT || cmd.stat == BUSY)
        return;
    else
        mwstate = INTRPT;

    if (cmd.op <= WRITES)       /* READS or WRITES */
        rwint();
    else
        newdrv();
}

/*
 * Service a read/write interrupt
 */
rwint()
{
    if (cmd.stat == OK) {
        if (cmd.count <= 512) {
            advque();
            return;
        } else {
            cmd.count -= 512;
            cmd.dma += 512;
            cmd.blk++;
            rwcmd();
            return;
        }
    }

    else if (cmd.stat != NOTRDY) {      /* try again */
        if (retry-- == RETRIES / 2) {
            mwinfo->flags &= ~CALIB;
            mwinfo->flags |= RECAL;
            newdrv();
            return;
        } else if (retry) {
            cmd.steps = 0;
            issue();
            return;
        }
    }

    error();
    advque();
}

/*
 * Advance the queue to the next request
 */
advque()
{
    static struct buf *f;

    iodone(mwbuf);
    f = mwbuf->forw;
    mwbuf->forw = 0;
    mwbuf = f;
    if (mwbuf == 0)
        mwstop();
    else if (curdrv != (mwbuf->dev & 3))
        newdrv();
    else
        newrw();
}

/*
 * Select a new drive
 */
newdrv()
{
    curdrv = mwbuf->dev & 3;
    mwinfo = &mws[curdrv];
    cmd.steps = 0;
    cmd.seksel = curdrv;
    cmd.hedsel = curdrv;
    cmd.arg2 = SETTLE;
    cmd.arg3 = SECSIZE;
    if (mwinfo->flags & CALIB) {
        cmd.arg0.byte.high = mwinfo->stpdel | INT;
        cmd.hedsel |= LCONST;
        cmd.op = LOAD;
        mwwait();
        newrw();
        return;
    }
    cmd.op = STAT;
    mwwait();
    if (cmd.stat & NOTRDY) {
        error();
        advque();
        return;
    }
    cmd.arg0.byte.high = HOMDEL | INT;
    cmd.hedsel |= LCONST;
    cmd.op = LOAD;
    mwwait();
    cmd.steps = -1;
    cmd.seksel |= STEPOUT;
    cmd.op = HOME;
    mwinfo->curtrk = 0;
    mwinfo->flags |= CALIB;
    issue();
}

/*
 * Set up a new read/write command
 */
newrw()
{
    /*
     * The buffer's block number is a filesystem block and stays one.  The
     * slice's cylinder offset is added in mwcyl, after the division that
     * turns a block into a cylinder, so nothing here has to hold a device
     * block number - which is what a large drive could not be counted in.
     */
    cmd.blk = mwbuf->blk;
    cmd.dma = mwbuf->data;
    cmd.xdma = mwbuf->xmem;
    cmd.count = mwbuf->count;
    cmd.op = (mwbuf->flags & BREAD) ? READS : WRITES;
    if (mwinfo->flags & RECAL)
        mwinfo->flags &= ~RECAL;
    else
        retry = RETRIES;
    rwcmd();
}

/*
 * Issue (or re-issue) a read/write command
 */
rwcmd()
{
    static UINT track, head, sector;
    static UINT csec, nsecs, curtrk;

    csec = cmd.blk % mwinfo->spc;
    nsecs = mwinfo->sectors;
    curtrk = mwinfo->curtrk;

    track = mwcyl(cmd.blk, mwinfo);
    head = csec / nsecs;
    sector = csec % nsecs;

    cmd.seksel = curdrv;
    if (track > curtrk)
        cmd.steps = track - curtrk;
    else {
        cmd.steps = curtrk - track;
        cmd.seksel |= STEPOUT;
    }
    mwinfo->curtrk = track;

    cmd.hedsel = curdrv | ((~head & 7) << 2) | HIGHCUR;
    if (track >= mwinfo->precomp)
        cmd.hedsel |= PRECOMP;
    if (track >= mwinfo->lowcur)
        cmd.hedsel &= ~HIGHCUR;
    cmd.arg0.word = track;
    cmd.arg2 = head;
    cmd.arg3 = sector;
    issue();
}

/*
 * Which cylinder a filesystem block lands on.
 *
 * The block's cylinder is its number divided by the sectors per cylinder,
 * shifted by the slice it lives in and rotated by d_roll.  Both terms are
 * cylinders and both are added after the division, so neither is ever a
 * block number: a slice's position on a large drive could not be one.
 * On every disk made before slices the slice term is zero and the
 * rotation is tracks >> 1, which is byte for byte what this has always
 * computed.
 */
mwcyl(blk, info)
    UINT blk;
    struct info *info;
{
    static UINT cyl;

    cyl = blk / info->spc;
    cyl += info->cylstart;
    cyl += info->roll;
    if (cyl >= info->tracks)
        cyl -= info->tracks;
    return cyl;
}

/*
 * Flag an error
 */
static
error()
{
    mwbuf->flags |= BERROR;
}

/*
 * Reset (initialize) the controller
 */
reset()
{
    extern char map0[], image0[];
    int *ichan;

    ichan = 0x1050;
    di();
    map0[2] = 0;
    ichan[0] = &cmd;
    ichan[1] = KERNEL;
    map0[2] = image0[2];

    /*
     * out(GRPSEL, 0 | PICMASK); /* select PIC * out(PIC1, in(PIC1) & ~VI);
     * /* enable interrupts * 
     */
    inton(MWINT);
    ei();
    out(RESET, 0);
    cmd.link = &cmd;
    cmd.xlink = KERNEL;
}

/*
 * Kick the controller
 */
attn()
{
    cmd.stat = BUSY;
    out(ATTN, 0);
}

/*
 * Issue a command whose completion will be
 * serviced by an interrupt.
 */
issue()
{
    mwstate = ACTIVE;
    attn();
}

/*
 * Issue a command and wait for completion
 */
mwwait()
{
    attn();
    while (cmd.stat == BUSY);
}

/*
 * Simulate missing timeout hardware.  This is the driver's tick, run once
 * a second by the resident side (sys/ovl.c), and it is named as the tick
 * in this driver's header at the end of this file - not a timer of its
 * own, and that is deliberate: it re-arms itself in the sense that it
 * wants to keep being called, and a driver-owned timer that is never
 * stopped would hold one of the kernel's five timeout slots for the rest
 * of the boot, one per time the disk was opened.
 *
 * A controller that has stopped talking raises no interrupt, which is the
 * whole reason this exists.  VIRGIN, STOPPED and INTRPT fall through the
 * switch below and change nothing, so being called from bring-up onwards
 * costs a comparison until a command is actually issued.
 */
mwcheck()
{
    di();
    switch (mwstate) {          /* cases STOPPED and INTRPT do not change */
    case ACTIVE:
        mwstate = LATE;
        break;
    case LATE:
        mwstate = TOOLATE;
        reset();
        curdrv = -1;            /* force re-selection */
        cmd.stat = NOTRDY;
        break;
    case TOOLATE:
        panic("hddma hung");
    }
    ei();
    if (mwstate == TOOLATE)
        mwint();
}

/*
 * busget() and busgive(), the bus lock this driver shares with the
 * floppy's, are in sys/bus.c: they are resident in the u page now,
 * because two modules cannot call into each other (sys/bus.c says why).
 *
 * Bring the driver up.  Where the kernel used to name mwopen, mwclose and
 * mwstrat in its own switch table, and name mwint on interrupt line 0,
 * the driver names itself, and this is where.  Registering is what makes
 * it reachable, so it comes last; a driver that cannot find its hardware
 * returns without registering and its major answers as nodev does.
 *
 * seg is the page the driver was placed in, and 0 means the kernel
 * already contains it - see sys/ovl.c.  The controller's reset() and the
 * inton(MWINT) that unmasks line 0 still happen at open, as they always
 * have.
 */
/*
 * The entries the driver's header hands the kernel (sys/mwhdr.c).  It is
 * a different object because a module's first bytes have to be the
 * header's, and an object's data is placed in the order the objects are
 * named - so the header is alone in the object named first, and this is
 * the driver's own.
 */
struct biovec mwbvec = { &mwopen, &mwclose, &mwstrat };

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
