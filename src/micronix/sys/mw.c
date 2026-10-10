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
#include <sys/ioctl.h>
#include <errno.h>

/*
 * Declared before anything calls it: an undeclared call is extern int
 * by default and the definition says static.
 */
static int error();
static int mwlink();

#define NDRIVES 4               /* Number of drives. See mws[] below. */
#define SECSIZE 3               /* Sector size (512 bytes) */
#define RETRIES 10              /* no. of retries on r/w error */
#define MWDEADLINE 1            /* ticks a request may sit in flight */

/*
 * Drive configuration information, one per drive.
 *
 * The first nine fields are struct dlgeom (sys/dlabel.h), in its order
 * and of its types, because sys/dlabel.c decodes the label through a
 * view of them - mwopen passes this struct as one.  The timing below
 * them is the label's too, but it is filled by dllabel through a
 * struct dltiming pointer, not through the geometry view.  Moving any
 * of the nine breaks the decode in a way the compiler will not report.
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

    UINT8 step;                 /* step pulse delay, 100 us */
    UINT8 seek;                 /* head settle after seek, 100 us */
    UINT precomp;               /* track where precomp begins */
    UINT lowcur;                /* track where low current begins */

    UINT curtrk;                /* current track */
    UINT8 flags;                /* see below */

    /*
     * The slice map off the label, cached at open.  A buffer's minor
     * names its slice; mwstrat looks the offset and length up here, so
     * every slice of a drive maps at once instead of one open's.
     */
    struct slicemap sm[NSLICE];
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
 * Controller commands.  Not a format: the opcode for that is OP_FORMAT in
 * include/sys/mw.h, and a raw command block carries it - what a format
 * means is the program's business now, not this driver's.
 */
#define READS		0
#define WRITES		1
#define READH		2
#define LOAD		4
#define STAT		5
#define HOME		6

/*
 * Controller constants
 */
#define HOMDEL	30              /* step pulse delay during home in 100 us */
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
    mwstate = 0,                /* see below */
    mwdead = 0;                 /* ticks the request has been in flight */

/*
 * States for mwstate (above)
 */
#define VIRGIN	0
#define STOPPED	1
#define ACTIVE	2
#define INTRPT	3
#define TOOLATE 5

/*
 * Device open
 *
 * The drive is described by the disk and by nothing else.  The minor
 * number says which drive (its low two bits) and which slice (the three
 * above them), and prints no geometry: what the drive *is* comes from the
 * label at physical cylinder 0, head 0, sector 0, which is the one
 * address that can be found knowing nothing at all.  A disk laid down by
 * something that wrote no label has no geometry, and then no block number
 * can be mapped - except the one the label goes in, and reaching that one
 * is what makes a label writable in the first place.
 *
 * The label itself, and the slice it names, are decoded in sys/dlabel.c,
 * which knows nothing about this controller; what is here is the handling
 * of a disk that has no label.
 */
mwopen(dev, mode)
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
    info = &mws[drive];
    /*
     * The label is loaded once, on the first open of any slice of this
     * drive, and cached here - geometry in the fields above, the slice
     * table below.  A later open of another slice on the same drive just
     * lets the buffer's minor name which slice, and mwstrat resolves its
     * offset and length from the cached table, so every slice of a drive
     * stays mapped at once, each by its own number.
     */
    if (info->flags & OPEN)
        return;

    /*
     * Conservative timing, only ever used for a drive with no label - and
     * that drive maps block 0 and nothing else, so no step, seek or write
     * ever consults them.  A label overwrites all four in dllabel().
     */
    info->step = 30;
    info->seek = 0;
    info->precomp = 0;          /* precomp on every track */
    info->lowcur = 0;           /* low write current on every track */

    /*
     * Nothing is known yet, and the decode is handed that: a zero
     * geometry is how this driver says so, and a label, if there is one,
     * replaces it whole.  What is left in mws[] from a previous open is
     * not knowledge and must not be offered as any - the disk may have
     * been formatted since, and its label, which is what the geometry
     * comes from, went with it.
     */
    info->tracks = 0;
    info->heads = 0;
    info->sectors = 0;
    info->roll = 0;

    /*
     * The label, as block 0 of this drive's 'c'.  The device number is
     * built here rather than taken from dev, so that it names the same
     * sector devlist's hd<N>c node names - a tool labelling this drive
     * and this open reading it hold one cache entry between them rather
     * than two copies of one sector.
     */
    switch (dllabel((dev & ~0377) | (DL_WHOLE << 5) | drive,
                    sl, (struct dlgeom *)info, info->sm,
                    (struct dltiming *)&info->step)) {
    case -1:
        return;                 /* no such slice; dllabel set u.error */
    case 0:
        /*
         * No label, so no geometry, so no block number that can be
         * mapped - but one address is reachable without any of that, and
         * it is the address the label goes in.  This is the state a
         * labeler works from, and the 'c' slice is the only way to be in
         * it: block 0 of any other slice is somewhere inside a filesystem
         * that has not been made yet, and mapping it "at cylinder 0"
         * would read a filesystem block as a label.
         *
         * One head and one sector of one cylinder maps block 0 to
         * cylinder 0, head 0, sector 0 and refuses every other block
         * through maxblk 0.  dllabel leaves the geometry zero, and zero
         * cannot be mapped with - mwcyl divides by the sectors per
         * cylinder - so this is also the least that makes the arithmetic
         * safe.  dlabel.c takes the same view of an unknown drive for the
         * length of its own read.
         */
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
    static UINT sl, off, roll, len, maxblk;

    info = &mws[b->dev & 3];

    /*
     * The first read of a drive is its label, before any geometry is
     * known - the probe at signon reads block 0 of 'c'.  Install a
     * minimal geometry so the division maps that one block to cylinder 0;
     * open() replaces it with the label's real geometry.
     */
    if (info->sectors == 0) {
        info->heads = 1;
        info->sectors = 1;
        info->spc = 1;
    }

    /*
     * The buffer's minor names its slice; dlslice resolves its offset,
     * roll, length and bound from the cached table ('c' is the whole
     * drive).
     */
    sl = devslice(b->dev);
    if (dlslice((struct dlgeom *)info, info->sm, sl,
                &off, &roll, &len, &maxblk) || b->blk > maxblk) {
        b->flags |= BERROR;
        b->error = ENXIO;
        iodone(b);
        return;
    }
    b->cyl = mwcyl(b->blk, info->spc, off, roll, len);
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
 * Stop the controller.  The select byte is assigned rather than added to:
 * the block now holds whatever the last caller of the raw ioctl left in it,
 * and a stop command that carried the head, precompensation and write
 * current bits of somebody else's command would not be this driver's own.
 */
mwstop()
{
    mwstate = STOPPED;
    cmd.steps = 0;
    cmd.arg0.byte.high = INTOFF;
    cmd.hedsel = LCONST | 3;    /* select drive 4 - turn off light */
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
    cmd.arg2 = mwinfo->seek;    /* the drive's head settle, from the label */
    cmd.arg3 = SECSIZE;
    if (mwinfo->flags & CALIB) {
        cmd.arg0.byte.high = mwinfo->step | INT;
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

    track = mwbuf->cyl;         /* resolved from the buffer's slice in mwstrat */
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

    /*
     * The head, and the one line of the select byte that is not a head
     * line on every drive.  Three bits are the head field; the fourth is
     * the low-current line, which a drive with more than eight heads uses
     * as head select line 4.  So the fourth head bit goes in bit 6, where
     * that line is - bit 5 of the byte is spare and has no wire - and
     * heads 0 through 7 leave the line high, which is the same thing as
     * high write current.  One encoding, both drives: the current is
     * commanded only where the line is still a current line, which is
     * where the drive's head count says eight or fewer.
     */
    cmd.hedsel = curdrv | ((~head & 7) << 2);
    if (~head & 8)
        cmd.hedsel |= HIGHCUR;
    if (track >= mwinfo->precomp)
        cmd.hedsel |= PRECOMP;
    if (mwinfo->heads <= 8 && track >= mwinfo->lowcur)
        cmd.hedsel &= ~HIGHCUR;
    cmd.arg0.word = track;
    cmd.arg2 = head;
    cmd.arg3 = sector;
    issue();
}

/*
 * Which cylinder a filesystem block lands on.  The block's cylinder is
 * its number divided by the sectors per cylinder, rotated by the slice's
 * own roll within the slice, then shifted by the slice's offset.  Both
 * terms are cylinders and both are applied after the division, so
 * neither is ever a block number: a slice's position on a large drive
 * could not be one.  The slice terms are passed in because mwstrat
 * resolves them from the buffer's minor against the cached table.
 */
mwcyl(blk, spc, off, roll, len)
    UINT blk, spc, off, roll, len;
{
    static UINT cyl;

    cyl = blk / spc + roll;
    if (cyl >= len)
        cyl -= len;
    return cyl + off;
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
 * Bring the driver up.  Nothing here touches the controller - its reset
 * is at open, below, where it has always been - but the driver has to be
 * told which page the kernel put it in, and this is the only moment
 * anything knows: ovlattach maps the page and calls this with the
 * segment (sys/ovl.c), and 0 means the kernel already contains the driver
 * and there is no segment to learn.
 *
 * The page matters because the controller is handed the address of cmd,
 * and cmd is a data object in this module.  The address this driver's own
 * code computes for cmd is the window address OVLBASE + offset, which is
 * where cmd *appears* while the page is mapped - not where it is.  The
 * board has no map of its own and follows a physical address, so a placed
 * module has to convert: (page << 12) | offset.  Handed the window
 * address instead, the controller reads whatever the kernel keeps at
 * OVLBASE + offset - the slot page, which is another driver's code - and
 * halts on the first byte of it that is not a command, writing no status:
 * the interrupt mwwait() is waiting for never comes.  The field is 24
 * bits and an unsigned is 16, so the shift has to be done in a UINT32.
 *
 * sys/djinit.c makes the same conversion for the same reason.  The
 * difference is placement: dj's init is folded into the kernel's init-only
 * region and can name nothing inside its module, so it asks for the
 * command block through the header's data field.  This one runs from the
 * driver's own page and names cmd directly, which is why it is here and
 * not in a file of its own.
 *
 * The conversion itself is mwlink(), below, and it is done from reset() as
 * well as here - both the channel address reset() writes and the block's
 * own chain come from the one number it works out, and the chain has to be
 * set again after every raw command block, because the caller's bytes
 * arrive over it (mwioctl).
 */
static UINT mwseg = 0;          /* the page we were placed in, 0 = resident */
static UINT32 mwphys = 0;       /* where cmd really is: (seg << 12) | offset */

mwinit(seg)
    UINT seg;
{
    mwseg = seg;
    mwlink();
    return (0);
}

/*
 * Reset (initialize) the controller
 */
reset()
{
    extern char map0[], image0[];
    int *ichan;

    mwlink();                   /* where cmd is, and the block's own chain */

    ichan = 0x1050;
    di();
    map0[2] = 0;
    ichan[0] = (UINT) mwphys;           /* the channel command address, */
    ichan[1] = (UINT) (mwphys >> 16);   /* three bytes at physical 0x50 */
    map0[2] = image0[2];

    /*
     * out(GRPSEL, 0 | PICMASK); /* select PIC * out(PIC1, in(PIC1) & ~VI);
     * /* enable interrupts *
     */
    inton(MWINT);
    ei();
    out(RESET, 0);
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
    mwdead = 0;
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
 *
 * A request that is merely in flight when the tick happens to run is not
 * failed: mwdead counts the ticks since issue() armed the command, and
 * only a request that has sat through MWDEADLINE of them - a controller
 * that really has stopped answering - reaches TOOLATE.  The tick is once
 * a second on no particular phase, so without that count a command issued
 * a moment before the tick would be shot down after a moment in flight.
 */
mwcheck()
{
    di();
    switch (mwstate) {          /* cases STOPPED and INTRPT do not change */
    case ACTIVE:
        if (++mwdead <= MWDEADLINE)
            break;
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
 * floppy's, are in sys/bus.c: leaf code, resident in the u page, because
 * two modules cannot call into each other (sys/bus.c says why).  The
 * lock's own words are in sys/ovl.c - one bus for the machine, and the
 * u page is one page per process.
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
static
mwbusready()
{
    wakeup(mwbusready);
}

/*
 * Wait for the DMA bus and take it, the floppy's own dance (sys/dj.c):
 * busget registers this as the heir when the bus is held, and the sleep
 * ends when busgive calls it back.
 */
static
mwbusplease()
{
    di();
    if (!busget(mwbusready))
        sleep(mwbusready, PRIBIO);
    else
        ei();
}

/*
 * The block switch's fourth entry (include/sys/con.h): run one raw command
 * block against the controller.
 *
 * The block is the controller's own - sixteen bytes of it, include/sys/mw.h
 * - and the caller fills in everything that says what to do: the opcode,
 * the drive and the step count, the head select byte with its
 * precompensation and write current bits, the four argument bytes, and, if
 * the command has a data phase, a buffer for it.  None of that does this
 * driver read: it has no opinion what a format is, or a read, or a seek,
 * and nothing here consults a table or a geometry.
 *
 * What it does decide is what no caller can:
 *
 *	the address of the data phase, because a caller's buffer is a virtual
 *	address and the board follows a physical one.  The buffer is held
 *	(bhold) and its address written in the three bytes at r->hole, the
 *	place the caller said it belongs.
 *
 *	the chain.  Bytes 13..15 are the address the board fetches its next
 *	command from, so they are an address again and are set to this
 *	block's own - which is what every other command this driver issues
 *	does, and what reset() sets up once for a placed module.
 *
 *	the wait, which is polled rather than interrupt driven: a raw command
 *	is synchronous, so the ioctl returns when the drive is done and the
 *	status goes back to the caller in cmd[12], the byte the board writes
 *	its completion into.  That byte is the only part of the block a
 *	caller can read but not write, which is why the block is wider than
 *	the command.
 *
 * Everything else - which track, how many heads, the sector-header table a
 * format points at, and what a status other than OK means - belongs to the
 * program.  /bin/mwformat is where that lives.
 *
 * Two blocks are refused.  One that names a drive other than the one the
 * node names, because curtrk and CALIB above are keyed by the unit, and a
 * block that moved another drive would leave this driver's idea of where
 * the head is describing the wrong drive.  And one that arrives while a
 * transfer is queued: the read/write path owns the controller from mwstrat
 * until its queue drains.
 *
 * The bus is claimed for the whole command, because the data phase is the
 * caller's buffer and the controller DMAs out of it (sys/bus.c: one bus
 * for the machine, shared with the floppy).  A hold must not sleep (uio.c),
 * so the bus comes first and goes back last.
 */
#define MWLEN	12		/* what a caller fills: out through the opcode */

/*
 * Work out where the controller's command block really is, and point the
 * board at it.
 *
 * The board follows a physical address and the address this driver's own
 * code computes for cmd is the window one, so a placed module has to
 * convert: (page << 12) | offset.  That is argued where it belongs, at
 * mwseg above, and this is the one place that does it - reset() takes the
 * channel address it writes to the controller from here, and the chain
 * below is the same number.
 *
 * The chain has to be set again after every raw command block, because the
 * caller's bytes are copied over the block and the link is inside it.  The
 * board fetches its next command from wherever the link points, so a link
 * the caller filled in - a virtual address, or zero - is a command block
 * read from another driver's code, or from nothing.
 *
 * cp is a pointer variable rather than &cmd taken straight into the cast:
 * c1 has a rule for converting a pointer to an integer, but none for a
 * pointer-to-object, which is what &cmd is.
 */
static
mwlink()
{
    static char *cp;

    cp = (char *) &cmd;
    if (mwseg)
        mwphys = ((UINT32) mwseg << 12) | ((unsigned) cp & 0xfff);
    else
        mwphys = (unsigned) cp;
    cmd.link = (UINT) mwphys;
    cmd.xlink = (UINT8) (mwphys >> 16);
}

int
mwioctl(dev, com, r, b)
    UINT dev, com;
    register struct cdb *r;
    register struct buf *b;
{
    static struct info *info;
    static char *p;
    static UINT i;
    static int hole;

    /*
     * A block is twelve bytes or it is not a block: those are all the
     * bytes a caller has, and a shorter one would leave whatever the last
     * command left in the driver standing as part of this one.  A data
     * phase is one block at most, which is as much as the kernel will
     * hand over - a bigger one arrives with no buffer at all and this
     * driver has no page of its own to move it through.
     */
    if (r->len != MWLEN || r->count < 0 || r->count > 512) {
        u.error = EINVAL;
        return (-1);
    }
    if (r->count && (r->hole < 0 || r->hole + 3 > MWLEN)) {
        u.error = EINVAL;
        return (-1);
    }
    hole = r->hole;
    if ((r->cmd[0] & 3) != (dev & 3)) {
        u.error = EINVAL;
        return (-1);
    }
    info = &mws[dev & 3];
    if (!(info->flags & OPEN)) {
        u.error = ENXIO;
        return (-1);
    }
    di();
    if (mwbuf != 0) {
        ei();
        u.error = EBUSY;
        return (-1);
    }
    ei();
    mwbusplease();
    /*
     * And again with the bus held: mwbusplease() sleeps when the floppy
     * has it, and a request that arrived while it slept would be issued
     * into the block this one is about to build.
     */
    if (mwbuf != 0) {
        busgive(0);
        u.error = EBUSY;
        return (-1);
    }

    mwstate = STOPPED;          /* polled: no completion interrupt is wanted,
                                 * and the tick has nothing to time out */
    curdrv = dev & 3;
    mwinfo = info;

    for (i = 0; i < MWLEN; i++)
        ((char *) &cmd)[i] = r->cmd[i];
    mwlink();

    if (r->count) {
        /*
         * The address, three bytes, written the way newrw() writes it for
         * a read or a write: the low two are the window address the
         * buffer is reachable at, the third is the segment it lives in.
         * The board takes those two together as one address, which is why
         * this is not computed as one - a 24-bit address does not fit a
         * 16-bit unsigned, and the segment is the top eight bits.
         */
        p = bhold(b);
        ((char *) &cmd)[hole] = (unsigned) p;
        ((char *) &cmd)[hole + 1] = (unsigned) p >> 8;
        ((char *) &cmd)[hole + 2] = b->xmem;
    }

    mwwait();

    /*
     * The head is wherever the block left it, so this driver's own idea of
     * where it is (curtrk, trusted while CALIB is set) is a guess from
     * here on - and a seek made from a stale guess lands somewhere and
     * reports that it arrived.  rwint() drops the same flag after a read
     * error, for the same reason.  The status is read out before mwstop()
     * reuses the block for its own command.
     */
    info->flags &= ~CALIB;
    r->cmd[MWLEN] = cmd.stat;

    if (r->count)
        brel();
    mwstop();

    /*
     * And the command ran.  How it went is in the status byte, and that is
     * the caller's to read: a raw command block's status is not one thing -
     * a read answers with a completion code, a sense answers with the
     * drive's status bits, and the board answers some commands with a byte
     * that is neither - so the driver does not decide whether it was good,
     * and only a refusal above is an error here.  This is the same split
     * the command block itself makes: the driver moves the bytes, the
     * program knows what they mean.
     */
    return (0);
}

/*
 * The entries the driver's header hands the kernel (sys/mwhdr.c).  It is
 * a different object because a module's first bytes have to be the
 * header's, and an object's data is placed in the order the objects are
 * named - so the header is alone in the object named first, and this is
 * the driver's own.
 */
struct biovec mwbvec = { &mwopen, &mwclose, &mwstrat, &mwioctl };

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
