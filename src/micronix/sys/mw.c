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
#include <sys/dlabel.h>
#include <errno.h>

/*
 * Declared before anything calls it: an undeclared call is extern int
 * by default and the definition says static.
 */
static int error();

#define NDRIVES 4               /* Number of drives. See mws[] below. */
#define SECSIZE 3               /* Sector size (512 bytes) */
#define CHKTIME	(1 * HERTZ)     /* timeout ticks between controller status
                                 * checks */
#define RETRIES 10              /* no. of retries on r/w error */

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
 * Drive configuration information
 */
struct info
{
    UINT tracks;                /* number of tracks per surface */
    UINT8 heads;                /* number of heads */
    UINT8 sectors;              /* number of sectors per track */
    UINT8 stpdel;               /* delay between step pulses, 100 us */
    UINT precomp;               /* track where precomp begins */
    UINT lowcur;                /* track where low current begins */

    UINT maxblk;                /* max legal block number */
    UINT spc;                   /* number of sectors per cylinder */
    UINT roll;                  /* what mwcyl adds to blk / spc */
    UINT curtrk;                /* current track */
    UINT8 flags;                /* see below */
    UINT8 type;                 /* index into specs table above */
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
#define MWINT	0

/*
 * Globals
 */
static struct info *mwinfo = 0; /* current drive */
static struct buf *mwbuf = 0;   /* current io request */
static UINT8 retry = 0,         /* number of retries so far */
    curdrv = -1,                /* number of current drive */
    mwstate = 0;                /* see below */

/*
 * Where the disk label lands when mwopen reads it.  One 512 byte sector,
 * held statically so it costs nothing in the kernel image beyond the bss
 * it already has room for.
 */
static char labelbuf[512] = 0;

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
 * The geometry comes off the disk, from the label mkfs writes into the
 * second half of the boot sector (physical cylinder 0, head 0, sector 0).
 * The compiled-in specs[] table is a starting point and a thing to compare
 * against, not the answer: a drive whose minor number names the wrong row
 * is detected here, and a drive with no table row at all is still mounted
 * from the label alone.
 */
mwopen(dev, mode)
    UINT dev, mode;
{
    static struct info *info;
    static struct dlabel *lp;
    static UINT8 drive, type;
    static UINT nspec;
    static struct buf *b;

    drive = dev & 3;
    type = minor(dev) >> 2;
    if (drive >= NDRIVES) {
        u.error = ENXIO;
        return;
    }
    info = &mws[drive];
    if ((info->flags & CALIB) && (info->type != type)) {
        u.error = ENXIO;
        return;
    }
    if (info->flags & OPEN)
        return;
    info->type = type;

    /*
     * A starting geometry.  If the minor names a real row in specs[],
     * copy it; otherwise leave the geometry zero and pick conservative
     * controller tuning - the label carries only tracks/heads/spt/roll
     * and not the three drive-specific timings.
     */
    nspec = sizeof specs / sizeof specs[0];
    if (type < nspec) {
        copy(&specs[type], info, sizeof(struct spec));
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
     * The label, read straight off cylinder 0 before any block is mapped
     * - the mapping needs the geometry the label holds, so this has to be
     * a raw read and not a bread().
     */
    if (mwreadlabel(drive, labelbuf)) {
        lp = (struct dlabel *)&labelbuf[DL_OFFSET];
        if (lp->d_magic[0] == DL_MAGIC[0] && lp->d_magic[1] == DL_MAGIC[1] &&
            lp->d_magic[2] == DL_MAGIC[2] && lp->d_magic[3] == DL_MAGIC[3] &&
            lp->d_tracks && lp->d_heads && lp->d_spt) {
            /*
             * Say so when the label and the table disagree: this is the
             * disagreement that used to mount the wrong geometry.
             */
            if (type < nspec &&
                (lp->d_tracks != specs[type].tracks ||
                 lp->d_heads != specs[type].heads ||
                 lp->d_spt != specs[type].sectors))
                pr("mw%d: label %d/%d/%d disagrees with table %d/%d/%d, using label\n",
                    drive, lp->d_tracks, lp->d_heads, lp->d_spt,
                    specs[type].tracks, specs[type].heads, specs[type].sectors);

            info->tracks = lp->d_tracks;
            info->heads = lp->d_heads;
            info->sectors = lp->d_spt;
            info->roll = lp->d_roll;
        } else if (type >= nspec) {
            /*
             * No table row and no label: there is no geometry at all,
             * so no block number can be mapped.
             */
            u.error = ENXIO;
            return;
        }
        /* else: no label but a table row - the copy above already stands */
    } else if (type >= nspec) {
        u.error = ENXIO;
        return;
    }

    info->maxblk = info->tracks * info->heads * info->sectors - 1;
    info->spc = info->heads * info->sectors;
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
 * Read the boot sector - physical cylinder 0, head 0, sector 0 - the one
 * sector reachable without knowing the geometry.  The block mapping in
 * rwcmd needs the geometry this sector carries, so this bypasses it: the
 * controller is reset, the drive homed, and one 512 byte sector read
 * straight into buf, all synchronous.  Interrupts stay off through the
 * spin so the completion interrupt cannot run into the not-yet-queued
 * mwbuf, and the drive is left calibrated and stopped for the normal
 * strategy path that follows.
 *
 * Returns 1 if the sector was read, 0 otherwise.
 */
mwreadlabel(drive, buf)
    UINT8 drive;
    char *buf;
{
    reset();
    curdrv = drive;
    mwinfo = &mws[drive];

    di();
    cmd.steps = 0;
    cmd.seksel = drive;
    cmd.hedsel = drive;
    cmd.arg2 = SETTLE;
    cmd.arg3 = SECSIZE;
    cmd.arg0.byte.high = HOMDEL | INT;
    cmd.hedsel |= LCONST;
    cmd.op = LOAD;
    mwwait();

    cmd.steps = -1;
    cmd.seksel = drive | STEPOUT;
    cmd.op = HOME;
    mwwait();
    mwinfo->curtrk = 0;

    cmd.steps = 0;
    cmd.seksel = drive;
    cmd.hedsel = drive | ((~0 & 7) << 2);
    cmd.arg0.word = 0;          /* cylinder 0 */
    cmd.arg2 = 0;               /* head 0 */
    cmd.arg3 = 0;               /* sector 0 */
    cmd.dma = (UINT)buf;
    cmd.xdma = KERNEL;
    cmd.count = 512;
    cmd.op = READS;
    mwwait();

    mwinfo->flags |= CALIB;
    mwstate = STOPPED;
    ei();

    mwcheck();                  /* the watchdog reset() starts normally */
    return (cmd.stat == OK);
}

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
    if (mwstate == VIRGIN) {
        reset();
        mwcheck();              /* start timeout "daemon" */
    }
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

mwcyl(blk, info)
    UINT blk;
    struct info *info;
{
    static UINT cyl;

    cyl = blk / info->spc;
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
 * Simulate missing timeout hardware.
 * Started by mwstart(). Self-continuing.
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
    timeout(&mwcheck, 0, CHKTIME);
    if (mwstate == TOOLATE)
        mwint();
}

/*
 * Synchronization for the HD-DMA and the DJ-DMA.
 * Since the HD must hog the bus,
 * it might close some of the DJ's transfer
 * windows if they were allowed to be active
 * simultaneously.
 * Note that this code does not know anything
 * about the DJ drivers.
 */

static int (*bus)() = 0;        /* bus master */
static int (*next)() = 0;       /* bus heir */

busget(func)
    int *(func) ();
{
    di();
    if (bus != 0 && bus != func) {   /* someone else has it */
        if (next == 0)       /* register the heir */
            next = func;
        ei();
        return 0;              /* didn't get it */
    } else {
        bus = func;             /* application accepted */
        ei();
        return 1;             /* got it */
    }
}

busgive(func)
    int *(func) ();
{
    di();
    if (next) {                 /* there is an heir */
        bus = next;             /* give bus to heir */
        next = func;            /* register next heir */
        ei();
        (*bus) ();              /* invoke new master */
        return 1;             /* bus was given away */
    } else {
        bus = 0;             /* no bus master */
        ei();
        return 0;              /* no one took bus */
    }
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
