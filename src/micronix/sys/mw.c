/*
 * mw.c 
 */

/*
 * Micronix drivers for HD-DMA
 * Gary Fitts, Morrow Designs
 * See the HD-DMA manual for more information.
 */

#include <sys.h>
#include <buf.h>
#include <proc.h>
#include <con.h>

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
    UCHAR heads;                /* number of heads */
    UCHAR sectors;              /* number of sectors per track */
    UCHAR stpdel;               /* delay between step pulses, 100 us */
    UINT precomp;               /* track where precomp begins */
    UINT lowcur;                /* track where low current begins */
} specs[] = {
    {153, 4, 17, 30, 128, 128}  /* Seagate 5 meg */
    {306, 4, 17, 2, 128, 128}   /* generic 10 meg */
    {306, 6, 17, 2, 128, 128}   /* CMI 16 meg */
    {640, 6, 17, 0, 256, 256}   /* CMI 32 meg */
    {733, 5, 17, 30, 300, 733}  /* Seagate 40 meg */
};

/*
 * Drive configuration information
 */
struct info
{
    UINT tracks;                /* number of tracks per surface */
    UCHAR heads;                /* number of heads */
    UCHAR sectors;              /* number of sectors per track */
    UCHAR stpdel;               /* delay between step pulses, 100 us */
    UINT precomp;               /* track where precomp begins */
    UINT lowcur;                /* track where low current begins */

    UINT maxblk;                /* max legal block number */
    UINT spc;                   /* number of sectors per cylinder */
    UINT curtrk;                /* current track */
    UCHAR flags;                /* see below */
    UCHAR type;                 /* index into specs table above */
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
    UCHAR seksel;               /* out<<4 | drv */
    UINT steps;                 /* number of steps */
    UCHAR hedsel;               /* pcmp<<7 | hicur<<6 | (~head&7)<<2 | drv */
    UINT dma;                   /* dma address */
    UCHAR xdma;                 /* high byte of 24-bit address */

    union
    {
        UINT word;
        struct
        {
            UCHAR low;
            UCHAR high;
        }
        byte;
    }
    arg0;
    UCHAR arg2;
    UCHAR arg3;

    UCHAR op;                   /* op code */
    UCHAR stat;                 /* completion status */
    UINT link;                  /* address of next command */
    UCHAR xlink;                /* high byte of address */

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
static UCHAR retry = 0,         /* number of retries so far */
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
 */
mwopen(dev, mode)
    UINT dev, mode;
{
    static struct info *info;
    static UCHAR drive, type;
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
    copy(&specs[type], info, sizeof(struct spec));
    info->maxblk = info->tracks * info->heads * info->sectors - 1;
    info->spc = info->heads * info->sectors;
    if ((b = bread(1, dev)) != NULL) {
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
    b = bread(info->spc * (info->tracks >> 1), dev);
    if (b) {
        brelse(b);
        bzero(b);
    }
    info->flags &= ~OPEN;
}

/*
 * Strategy
 */
mwstrat(b)
    fast struct buf *b;
{
    fast struct info *info;

    info = &mws[b->dev & 3];
    if (b->blk > info->maxblk) {
        b->flags |= BERROR;
        b->error = ENXIO;
        iodone(b);
        return;
    }
    b->cyl = mwcyl(b->blk, info);
    di();
    if (mwbuf == NULL)
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
    busgive(NULL);
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
    mwbuf->forw = NULL;
    mwbuf = f;
    if (mwbuf == NULL)
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
    static UINT cyl, roll;

    cyl = blk / info->spc;
    roll = info->tracks;
    cyl += roll >> 1;
    if (cyl >= roll)
        cyl -= roll;
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
    if (bus != NULL && bus != func) {   /* someone else has it */
        if (next == NULL)       /* register the heir */
            next = func;
        ei();
        return NO;              /* didn't get it */
    } else {
        bus = func;             /* application accepted */
        ei();
        return YES;             /* got it */
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
        return YES;             /* bus was given away */
    } else {
        bus = NULL;             /* no bus master */
        ei();
        return NO;              /* no one took bus */
    }
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
