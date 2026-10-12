/*
 * Micronix driver for the CompuPro DISK 2 hard disk controller.
 *
 * sys/disk2.c
 *
 * The DISK 2 is the simple one: a finite state machine on the board finds
 * a sector header and moves its data, but everything else is the host's
 * job - the seek is done a step pulse at a time by reading the data port,
 * and each DMA transfer goes through the SELECTOR CHANNEL, which is
 * programmed with the buffer's address and direction before the command.
 * There is no in-memory command block, so no physical address to work out,
 * and no interrupt is used: the driver polls the status register.  The
 * interface is include/sys/disk2.h.
 *
 * Everything here is polled, so the strategy path runs the whole queue
 * top to bottom under the bus lock - no interrupt to hand off to, and the
 * board does not know how to do anything on its own between commands.
 */
#include <types.h>
#include <sys/sys.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <sys/con.h>
#include <sys/disk2.h>
#include <sys/ovl.h>
#include <sys/dlabel.h>
#include <errno.h>

/*
 * Declared before anything calls them: an undeclared call is extern int by
 * default and the definition says static.
 */
static void d2reset();
static void d2rw();

#define NDRIVES 4               /* Number of drives */
#define RETRIES 10              /* no. of retries on r/w error */
#define MAXTRK 256              /* the most cylinders one header byte holds */

/*
 * Drive configuration information, one per drive - the same shape the
 * other disk drivers use: the first nine fields are struct dlgeom
 * (sys/dlabel.h) so sys/dlabel.c decodes the label through a view of
 * them, then this driver's own bookkeeping, then the cached slice map.
 * The label's timing is read and dropped: a diskette controller has no
 * use for it.
 */
struct info
{
    UINT tracks;                /* number of tracks per surface */
    UINT8 heads;                /* number of heads */
    UINT8 sectors;              /* number of sectors per track */
    UINT maxblk;                /* dllabel's bound for the first slice opened */
    UINT spc;                   /* number of sectors per cylinder */
    UINT roll;                  /* the slice's d_roll */
    UINT cylstart;              /* dllabel's offset for the first slice opened */
    UINT8 firstsec;             /* first sector number on a track */
    UINT len;                   /* dllabel's slice length, in cylinders */

    UINT curtrk;                /* where the driver thinks the head is */
    UINT8 flags;                /* see below */

    struct slicemap sm[NSLICE];
} disk2s[NDRIVES] = 0;

/*
 * Info flags for above structure
 */
#define CALIB	1               /* drive is calibrated (home done) */
#define OPEN	2               /* drive is open */

/*
 * Globals
 */
static struct info *d2info = 0; /* current drive */
static struct buf *d2buf = 0;   /* the queue head */
static UINT8 curdrv = -1;       /* number of current drive */
static UINT8 d2state = 0;       /* see below */
static UINT8 d2dead = 0;        /* ticks a command has been in flight */

/*
 * States for d2state (above)
 */
#define VIRGIN	0
#define ACTIVE	1
#define STOPPED	2

/*
 * Device open.  Identical to the HD-DMA's: the drive is described by its
 * label and by nothing else, so the label is read once per drive and its
 * geometry and slice table cached.
 */
disk2open(dev, mode)
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
    info = &disk2s[drive];
    if (info->flags & OPEN)
        return;

    info->tracks = 0;
    info->heads = 0;
    info->sectors = 0;
    info->roll = 0;

    switch (dllabel((dev & ~0377) | (DL_WHOLE << 5) | drive,
                    sl, (struct dlgeom *)info, info->sm, 0)) {
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
 * Device close.  The cache has been flushed; nothing else to do.
 */
disk2close(dev)
    int dev;
{
    disk2s[dev & 3].flags &= ~OPEN;
}

/*
 * Strategy
 */
disk2strat(b)
    register struct buf *b;
{
    register struct info *info;
    static UINT sl, off, roll, len, maxblk;

    info = &disk2s[b->dev & 3];

    /*
     * The first read of a drive is its label, before any geometry is
     * known.  Install a minimal geometry so the division maps block 0 to
     * cylinder 0; open() replaces it with the label's real geometry.
     */
    if (info->sectors == 0) {
        info->heads = 1;
        info->sectors = 1;
        info->spc = 1;
    }

    sl = devslice(b->dev);
    if (dlslice((struct dlgeom *)info, info->sm, sl,
                &off, &roll, &len, &maxblk) || b->blk > maxblk) {
        b->flags |= BERROR;
        b->error = ENXIO;
        iodone(b);
        return;
    }
    if (!(b->flags & BREAD) && b->blk == 0 && sl == DL_WHOLE)
        dlreload(b, (struct dlgeom *)info, info->sm, 0);
    b->cyl = d2cyl(b->blk, info->spc, off, roll, len);
    di();
    if (d2buf == 0)
        d2buf = b;
    else
        bsort(d2buf, b);
    if (d2state != ACTIVE)
        d2start();
    ei();
}

/*
 * Run the queue, top to bottom, under the bus lock.  The whole thing is
 * polled, so each request is a synchronous seek-plus-transfer and the next
 * one starts only after the last has completed.
 */
d2start()
{
    static struct buf *f;

    if (!busget(&d2start))
        return;
    if (d2state == VIRGIN)
        d2reset();
    d2state = ACTIVE;
    while (d2buf) {
        curdrv = d2buf->dev & 3;
        d2info = &disk2s[curdrv];
        d2rw(d2buf);
        iodone(d2buf);
        f = d2buf->forw;
        d2buf->forw = 0;
        d2buf = f;
    }
    d2state = STOPPED;
    busgive(0);
}

/*
 * Reset the board: make the status register transparent, then home every
 * drive that will be selected later by stepping it out until track 0.
 * The head position is a guess from here, and the first seek trusts it.
 */
static void
d2reset()
{
    static UINT unit;
    static int i;

    out(D2_CTL, D2_ATTN);       /* clear the latched completion flag */
    for (unit = 0; unit < NDRIVES; unit++) {
        out(D2_CTL, D2_ATTN | D2_REG_DRIVE | (unit & 3));
        out(D2_DATA, 1 << (unit + 4));  /* select the drive, head 0 */
        out(D2_CTL, D2_ATTN | D2_OP2 | (unit & 3));  /* step out */
        for (i = 0; i < MAXTRK; i++) {
            in(D2_DATA);        /* one step pulse */
            if (!(in(D2_CTL) & D2_TRK0))
                break;          /* at track 0 */
        }
        disk2s[unit].curtrk = 0;
        disk2s[unit].flags |= CALIB;
    }
    curdrv = -1;
}

/*
 * Program the SELECTOR CHANNEL for the next transfer.  A read from its
 * port disables it, then the four bytes are the 24 bit address - extended,
 * high, low - and the mode.  A disk read writes the data into memory, so
 * W/R* is low; a disk write reads it back out.
 */
static void
d2sc(b, writemem)
    struct buf *b;
    int writemem;
{
    in(D2_SC);                  /* disable the channel */
    out(D2_SC, b->xmem);        /* A23-A16 */
    out(D2_SC, (UINT8) ((unsigned) b->data >> 8));  /* A15-A8 */
    out(D2_SC, (UINT8) (unsigned) b->data);         /* A7-A0 */
    out(D2_SC, (writemem ? D2_WRMEM : D2_RDMEM) | D2_SCID);
}

/*
 * Select a drive and head in the DRIVE register: the high nibble picks the
 * drive (bit 4 is drive 0) and the low nibble the head.
 */
static void
d2sel(drive, head)
    UINT drive, head;
{
    out(D2_CTL, D2_ATTN | D2_REG_DRIVE | (drive & 3));
    out(D2_DATA, (UINT8) ((1 << (drive + 4)) | (head & 0xf)));
}

/*
 * Step the head to a cylinder.  OP2 in the control register is the step
 * direction, and each read of the data port is one step pulse.
 */
static void
d2seek(track)
    UINT track;
{
    static UINT cur;
    static int steps;
    static UINT8 dir;

    cur = d2info->curtrk;
    steps = (int) track - (int) cur;
    dir = 0;
    if (steps < 0) {
        dir = D2_OP2;           /* step out, toward track 0 */
        steps = -steps;
    }
    out(D2_CTL, D2_ATTN | dir | (curdrv & 3));
    while (steps--)
        in(D2_DATA);
    d2info->curtrk = track;
}

/*
 * Load the three header-match registers with the sector to find.
 */
static void
d2load(cyl, head, sect)
    UINT cyl, head, sect;
{
    out(D2_CTL, D2_ATTN | D2_REG_CYL | (curdrv & 3));
    out(D2_DATA, (UINT8) cyl);
    out(D2_CTL, D2_ATTN | D2_REG_HEAD | (curdrv & 3));
    out(D2_DATA, (UINT8) head);
    out(D2_CTL, D2_ATTN | D2_REG_SECT | (curdrv & 3));
    out(D2_DATA, (UINT8) sect);
}

/*
 * Issue a read or a write and poll for completion, returning the latched
 * status byte.
 */
static UINT8
d2cmd(opcode)
    UINT opcode;
{
    out(D2_CTL, D2_ATTN | D2_RUN | opcode | (curdrv & 3));
    while (in(D2_CTL) & D2_SATTN)       /* wait for ATTN* to go low */
        ;
    return in(D2_CTL);
}

/*
 * One sector read or write: program the DMA, select the drive and head,
 * seek, load the header registers, and run the command with retries.  On
 * an error the command is retried; at the halfway point the drive is
 * re-homed in case a seek went astray.
 */
static void
d2rw(b)
    struct buf *b;
{
    static UINT spc, csec, head, sect;
    static int w, r;
    static UINT8 st;

    spc = d2info->spc;
    csec = b->blk % spc;
    head = csec / d2info->sectors;
    sect = csec % d2info->sectors;
    w = !(b->flags & BREAD);

    d2sc(b, !w);                /* a disk read writes memory */
    d2sel(curdrv, head);
    d2seek(b->cyl);
    d2load(b->cyl, head, sect);

    for (r = RETRIES; r; r--) {
        st = d2cmd(w ? D2_WRITE : D2_READ);
        if (!(st & D2_ERRMASK) && (w ? (st & D2_WFLT) : 1))
            return;
        if (r == RETRIES / 2) {
            d2info->flags &= ~CALIB;
            d2reset();
            d2sel(curdrv, head);
            d2seek(b->cyl);
            d2load(b->cyl, head, sect);
            d2info->flags |= CALIB;
        }
    }
    b->flags |= BERROR;
}

/*
 * Which cylinder a filesystem block lands on - the same arithmetic the
 * other disk drivers use, because the disk and the label are the same even
 * when the controller is not.
 */
d2cyl(blk, spc, off, roll, len)
    UINT blk, spc, off, roll, len;
{
    UINT cyl;

    cyl = blk / spc + roll;
    if (cyl >= len)
        cyl -= len;
    return cyl + off;
}

/*
 * The watchdog, run once a second by the resident tick.  A command in
 * flight is polled, so it never sits in ACTIVE across a tick - this exists
 * only to keep the header honest, and it always reports the board alive.
 */
disk2check()
{
    return (0);
}

/*
 * The entries the driver's header hands the kernel (sys/disk2hdr.c).  No
 * ioctl yet: the raw command the format program needs is a later piece.
 */
struct biovec disk2bvec = { &disk2open, &disk2close, &disk2strat, 0 };

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
