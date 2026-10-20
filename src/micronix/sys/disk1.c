/*
 * Micronix driver for the CompuPro DISK 1 / DISK 1A floppy controller.
 *
 * sys/disk1.c
 *
 * The DISK 1 is an Intel 8272 (NEC uPD765) floppy disk controller behind
 * CompuPro's glue: the chip owns the head, the seek, the address marks
 * and the sector data, and the glue adds a 24 bit DMA address register,
 * a drive status register and a motor register.  So the driver's work is
 * small - put the unit, the head, the cylinder and the sector into a
 * nine byte command, hand the board the physical address of the buffer,
 * and wait for the chip to raise its interrupt.  The interface is
 * include/sys/disk1.h.
 *
 * The transfer is interrupt driven, like the HDCA's (sys/hdca.c) and
 * unlike the DJ-DMA's, which runs the board from a program in memory.
 * The chip has one interrupt and what it means depends on what is out:
 * after a RECALIBRATE or a SEEK it means the head has arrived, and is
 * answered with SENSE INTERRUPT STATUS - the only thing that both reads
 * that answer and clears the interrupt - while after a read or a write
 * it means the result bytes are waiting.  Only one of the two is ever
 * outstanding, so one state variable tells the handler which it is.
 *
 * The card carries both drive banks at one port block: four 8 inch
 * drives and four 5 1/4 inch ones, the bank chosen by the drive select
 * register's data rate bit.  The drive is the minor's low three bits,
 * 0-3 the 8 inch bank and 4-7 the 5 1/4, exactly as the DJ-DMA numbers
 * its drives.
 */
#include <types.h>
#include <sys/sys.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <sys/con.h>
#include <sys/disk1.h>
#include <sys/ovl.h>
#include <sys/dlabel.h>
#include <sys/ioctl.h>
#include <errno.h>

extern int in(), out();

/*
 * Declared before anything calls them: an undeclared call is extern int
 * by default and the definition says static.
 */
static void d1error(), d1seek(), d1cmd(), d1sel(), d1motor(), d1specify();
static UINT d1res();

#define NDRIVES 8               /* two banks of four */
#define RETRIES 10              /* no. of retries on r/w error */
#define D1DEADLINE 3            /* ticks a request may sit in flight */
#define D1SPIN  1024            /* polls of the status register before dead */
#define D1POLL  100000L         /* ... and of the interrupt bit, in the ioctl */

/*
 * Drive configuration information, one per drive.
 *
 * The first nine fields are struct dlgeom (sys/dlabel.h), in its order
 * and of its types, because sys/dlabel.c decodes the label through a
 * view of them - disk1open passes this struct as one.  Moving any of
 * them breaks the decode in a way the compiler will not report.
 *
 * The label's timing is read and dropped: an 8272 is told its step rate
 * and head times by SPECIFY, and those are the controller's numbers and
 * the board's clock, not the drive's.
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

    /* the slice map off the label, cached at open */
    struct slicemap sm[NSLICE];
} disk1s[NDRIVES] = 0;

/*
 * Info flags for above structure
 */
#define CALIB	1               /* drive is calibrated (home done) */
#define OPEN	2               /* drive is open */

/*
 * Globals
 */
static struct info *d1info = 0; /* current drive */
static struct buf *d1buf = 0;   /* the queue head */
/*
 * The motor lines, one per 8 inch drive.  A table rather than a shift of
 * a one bit into place, because c1 has no rule for shifting by a
 * variable.
 */
static UINT8 d1mot[4] = { D1_M0, D1_M1, D1_M2, D1_M3 };

static UINT8 retry = 0,         /* number of retries so far */
    curdrv = -1,                /* number of current drive */
    d1state = 0,                /* see below */
    d1dead = 0,                 /* ticks the request has been in flight */
    d1fault = 0,                /* the chip stopped answering mid-command */
    d1bank = -1;                /* the bank the chip's clock is set for */

/*
 * States for d1state (above).  The order matters: a request starts the
 * controller only from VIRGIN or STOPPED, which are the states below
 * ACTIVE, and every state a request can be in flight from is above it.
 */
#define VIRGIN	0
#define STOPPED	1
#define ACTIVE	2
#define SEEKING	3
#define TOOLATE	4

/*
 * Device open.  The same as every other disk's: the drive is described
 * by its label and by nothing else, so the label is read once per drive
 * and its geometry and slice table cached.
 */
disk1open(dev, mode)
    UINT dev, mode;
{
    struct info *info;
    UINT8 drive, sl;
    struct buf *b;

    drive = dev & 7;
    sl = devslice(dev);
    info = &disk1s[drive];
    if (info->flags & OPEN)
        return;

    d1motor(drive);
    info->tracks = 0;
    info->heads = 0;
    info->sectors = 0;
    info->roll = 0;
    /*
     * The label read is block 0, and a soft sectored floppy numbers its
     * sectors from 1, so block 0 is sector 1 and not 0.  dllabel() reads
     * block 0 with the geometry it can see - one head of one sector - so
     * this field is what makes that read land on a sector that exists;
     * the label's own d_firstsec then overwrites it.
     */
    info->firstsec = 1;

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
 * Device close.  The cache has been flushed; the board keeps the motors
 * turning until its own timer stops them.
 */
disk1close(dev)
    int dev;
{
    disk1s[dev & 7].flags &= ~OPEN;
}

/*
 * Strategy
 */
disk1strat(b)
    register struct buf *b;
{
    register struct info *info;
    static UINT sl, off, roll, len, maxblk;

    info = &disk1s[b->dev & 7];

    /*
     * The first read of a drive is its label, before any geometry is
     * known.  Install a minimal geometry so the division maps block 0 to
     * cylinder 0 sector 1; open() replaces it with the label's real one.
     */
    if (info->sectors == 0) {
        info->heads = 1;
        info->sectors = 1;
        info->spc = 1;
        info->firstsec = 1;
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
    b->cyl = d1cyl(b->blk, info->spc, off, roll, len);
    /*
     * The 8272's cylinder register is one byte, so a drive with more than
     * 256 cylinders is not addressable at all - a small floppy never is,
     * but a label that claims otherwise is refused rather than truncated
     * into a seek to the wrong cylinder.
     */
    if (b->cyl > 0xff) {
        b->flags |= BERROR;
        b->error = ENXIO;
        iodone(b);
        return;
    }
    di();
    if (d1buf == 0)
        d1buf = b;
    else
        bsort(d1buf, b);
    if (d1state < ACTIVE)
        d1start();
    ei();
}

/*
 * Goose the lower half.  The first request brings the controller up;
 * after that each request picks a drive and reads or writes.
 */
d1start()
{
    if (!busget(&d1start))
        return;
    newdrv();
}

/*
 * Stop the controller.  The 8272 has nothing to halt and the motors run
 * on their own timer, so stopping is the state and the bus.
 */
d1stop()
{
    d1state = STOPPED;
    busgive(0);
}

/*
 * Select a new drive: put the bank into the drive select register, spin
 * the motor up, and home the head if this is the first time this drive
 * has been used.  Homing is a RECALIBRATE, which has no result phase -
 * its interrupt is the whole answer and disk1int follows it.
 */
newdrv()
{
    curdrv = d1buf->dev & 7;
    d1info = &disk1s[curdrv];
    d1motor(curdrv);
    d1sel();
    if (d1info->flags & CALIB) {
        newrw();
        return;
    }
    d1fault = 0;
    retry = RETRIES;
    di();
    d1state = SEEKING;          /* arm before the command that raises it */
    d1dead = 0;
    d1putb(FDC_RECAL);
    d1putb(curdrv & 3);
    ei();
}

/*
 * Set up a read or a write: seek first if the head is not on the block's
 * cylinder, then issue the command.  One sector per command, because a
 * buffer is 512 bytes and one sector is one block.
 */
newrw()
{
    static UINT spc, csec, head, sect;

    spc = d1info->spc;
    csec = d1buf->blk % spc;
    head = csec / d1info->sectors;
    sect = csec % d1info->sectors;
    if (d1info->curtrk != d1buf->cyl) {
        d1seek(d1buf->cyl, head);
        return;
    }
    d1cmd(head, sect + d1info->firstsec);
}

/*
 * Step the head to a cylinder.  A SEEK has no result phase either, so it
 * is followed in the handler by the same SENSE INTERRUPT STATUS a
 * RECALIBRATE is.
 */
static void
d1seek(cyl, head)
    UINT cyl, head;
{
    d1fault = 0;
    di();
    d1state = SEEKING;
    d1dead = 0;
    d1putb(FDC_SEEK);
    d1putb((head << 2) | (curdrv & 3));
    d1putb(cyl);
    ei();
}

/*
 * Issue the read or the write.  The board's DMA address register is
 * loaded first, three bytes most significant first, and then the nine
 * command bytes - but only after the chip says it wants each one, which
 * is what the main status register's RQM bit is for.
 *
 * The command names the sector absolutely within the track: the cylinder
 * and head the slice arithmetic resolved, the sector's own number, the
 * size code for 512 bytes, the last sector the transfer covers, and the
 * gap length for a 512 byte MFM sector.  The data length byte is only
 * read when the size code is zero, which it never is here, so it is
 * filled with 0xff.
 *
 * That "last sector" is the command's EOT, and the chip reads from the
 * sector asked for through EOT inclusive - a whole track's worth if EOT
 * is left at the end of the track.  One buffer is 512 bytes, so EOT is
 * the sector itself: one block, one sector, one command.
 */
static void
d1cmd(head, sect)
    UINT head, sect;
{
    d1fault = 0;
    d1dma();
    di();
    d1state = ACTIVE;           /* arm before the command that raises it */
    d1dead = 0;
    d1putb((d1buf->flags & BREAD) ? FDC_READ : FDC_WRITE);
    d1putb((head << 2) | (curdrv & 3));
    d1putb(d1buf->cyl);
    d1putb(head);
    d1putb(sect);
    d1putb(D1_N);
    d1putb(sect);
    d1putb(D1_GPL);
    d1putb(0xff);
    if (d1fault) {
        d1state = STOPPED;
        ei();
        d1error();
        advque();
        return;
    }
    ei();
    retry = RETRIES;
}

/*
 * Program the board's 24 bit DMA address register from the buffer.  The
 * board follows a physical address and the address this driver's own
 * code computes for b->data is the window one - the buffer is mapped at
 * BUFWIN and lives in the segment b->xmem names - so the two are put
 * back together here.  It is the same conversion every DMA driver here
 * makes (uio.c says why at bhold) and the same one the hwsim card
 * unpicks.
 */
d1dma()
{
    static UINT32 a;

    a = ((UINT32) d1buf->xmem << 12) | ((unsigned) d1buf->data & 0xfff);
    out(D1_DMA, (UINT8) (a >> 16));
    out(D1_DMA, (UINT8) (a >> 8));
    out(D1_DMA, (UINT8) a);
}

/*
 * Interrupt service begins here.  The chip has one interrupt and the
 * state says which of the two things it is.
 */
disk1int()
{
    static UINT8 st0, st1, st2, c, h, r, n;
    static int i;

    if (d1state == SEEKING) {
        /*
         * A RECALIBRATE or a SEEK has finished.  Neither has a result
         * phase, so the chip is asked what it did with SENSE INTERRUPT
         * STATUS - which is both how the answer is read and the only way
         * that interrupt is cleared.  The second byte is the cylinder the
         * head really landed on, and it is believed over the request.
         */
        d1fault = 0;
        d1putb(FDC_SENSEI);
        st0 = d1getb();
        c = d1getb();
        if (d1fault) {
            d1error();
            advque();
            return;
        }
        d1info->curtrk = c;
        d1info->flags |= CALIB;
        newrw();
        return;
    }
    if (d1state != ACTIVE)
        return;                 /* nothing in flight, or already failed */

    d1fault = 0;
    st0 = d1getb();
    st1 = d1getb();
    st2 = d1getb();
    c = d1getb();
    h = d1getb();
    r = d1getb();
    n = d1getb();
    if (d1fault || (st0 & D1_ERRMASK) || (st1 & ST1_ERR) || (st2 & ST2_ERR)) {
        if (retry == 0) {
            d1error();
            advque();
            return;
        }
        retry--;
        if (retry == RETRIES / 2) {
            /*
             * Halfway out of retries the head may be the problem, not the
             * medium: home the drive and start the request over, which is
             * what the other disk drivers do at the same point.
             */
            d1info->flags &= ~CALIB;
            newdrv();
            return;
        }
        newrw();                /* the same sector again */
        return;
    }
    advque();
}

/*
 * Advance the queue to the next request
 */
advque()
{
    struct buf *f;

    iodone(d1buf);
    f = d1buf->forw;
    d1buf->forw = 0;
    d1buf = f;
    if (d1buf == 0)
        d1stop();
    else if (curdrv != (d1buf->dev & 7))
        newdrv();
    else
        newrw();
}

/*
 * Which cylinder a filesystem block lands on - the same arithmetic the
 * other disk drivers use, because the disk and the label are the same
 * even when the controller is not.
 */
d1cyl(blk, spc, off, roll, len)
    UINT blk, spc, off, roll, len;
{
    UINT cyl;

    cyl = blk / spc + roll;
    if (cyl >= len)
        cyl -= len;
    return cyl + off;
}

/*
 * Flag an error
 */
static void
d1error()
{
    d1buf->flags |= BERROR;
    d1buf->error = EIO;
}

/*
 * The drive select register, and the SPECIFY that goes with it.  The
 * data rate bit picks the bank and halves the chip's clock with it - 8
 * MHz for the 8 inch drives, 4 MHz for the 5 1/4 - and the manual's rule
 * for a clock the chip has not been told about is to send SPECIFY again,
 * so the step and head times are re-sent whenever the bank moves.  The
 * two sided line is forced only for a minifloppy cable that has no side
 * select of its own, which is what the bit is for.
 */
static void
d1sel()
{
    static UINT bank, five;

    five = (curdrv >= 4);
    out(D1_DSEL, (five ? D1_FIVE : 0)
        | ((five && d1info->heads > 1) ? D1_TWOSIDE : 0));
    if (five != d1bank) {
        d1bank = five;
        d1specify();
    }
}

/*
 * Tell the chip its step rate and head load and unload times, in the
 * units the datasheet gives them: the step rate is 16 minus a 1..16 ms
 * count in the high nibble of the first byte, the head unload time is
 * the low nibble in 16 ms units, and the head load time is the second
 * byte's bits 7..1 in 2 ms units.  The 8 inch numbers are a 3 ms step, a
 * 240 ms unload and an 8 ms load; the 5 1/4 inch ones are the same real
 * times asked of a chip whose clock is half as fast, so the counts are
 * halved with it.
 */
static void
d1specify()
{
    d1putb(FDC_SPECIFY);
    if (curdrv >= 4) {
        d1putb(0xaf);           /* 6 ms step, 240 ms unload */
        d1putb(0x04);           /* 4 ms head load, DMA */
    } else {
        d1putb(0xdf);           /* 3 ms step, 240 ms unload */
        d1putb(0x10);           /* 8 ms head load, DMA */
    }
}

/*
 * Spin a drive's motor up.  Every 5 1/4 inch motor hangs off bit 7 along
 * with the fourth 8 inch drive's, and bit 0 is written 1 on every call:
 * a zero there disables the board's boot EPROM, and only a system reset
 * brings it back.  There is no spin up delay here - the board's own
 * timer stops the motors after about fifteen seconds and this is called
 * on every drive selection, so a motor that was stopped is started again
 * with the request that needs it already queued behind it.
 */
static void
d1motor(drive)
    UINT drive;
{
    if (drive >= 4)
        out(D1_MOTOR, D1_BOOTOFF | D1_M3);
    else
        out(D1_MOTOR, D1_BOOTOFF | d1mot[drive]);
}

/*
 * Wait for the chip to want the data register and give it a byte.  RQM
 * says it is ready and DIO says which way; a chip that stops answering
 * part way through a command is a fault rather than a wait, because the
 * alternative is a driver that never returns.  The loop is bounded and
 * short: the 8272 asks for a byte within microseconds of getting the
 * previous one.
 */
d1putb(c)
    UINT c;
{
    static int i;

    for (i = 0; i < D1SPIN; i++)
        if (in(D1_MSR) & D1_RQM)
            break;
    if (i == D1SPIN) {
        d1fault = 1;
        return;
    }
    out(D1_DATA, c);
}

/*
 * ... and take one back.  A byte is there to read only when RQM and DIO
 * are both set, which is the main status register's way of saying the
 * register holds a result rather than a command parameter.
 */
d1getb()
{
    static int i;

    for (i = 0; i < D1SPIN; i++)
        if ((in(D1_MSR) & (D1_RQM | D1_DIO)) == (D1_RQM | D1_DIO))
            break;
    if (i == D1SPIN) {
        d1fault = 1;
        return (0xff);
    }
    return (in(D1_DATA));
}

/*
 * How many result bytes a command leaves.  A command with no result
 * phase has raised its interrupt by the time its last parameter byte is
 * in, and reading a result that is not there reads the next command's
 * first byte instead, so the length is looked up and not assumed.
 */
static UINT
d1res(op)
    UINT op;
{
    switch (op & 0x1f) {
    case FDC_READTRK:
    case FDC_WRITE:
    case FDC_READ:
    case FDC_WRDEL:
    case FDC_READID:
    case FDC_READDEL:
    case FDC_FORMAT:
        return (7);
    case FDC_SENSEI:
        return (2);
    case FDC_SENSEDRV:
        return (1);
    }
    return (0);                 /* SPECIFY, RECALIBRATE, SEEK */
}

/*
 * Wait for the board's copy of the chip's interrupt pin to come up.  The
 * chip raises it the moment it is done with a command; a board that
 * never does is a fault, and the caller is told so rather than left
 * spinning.
 */
static int
d1wait()
{
    static UINT32 i;

    for (i = 0; i < D1POLL; i++)
        if (in(D1_DSTAT) & D1_INT)
            return (1);
    return (0);
}

static
d1ready()
{
    wakeup(d1ready);
}

/*
 * The bus.  The floppy and the hard disk share one DMA bus (sys/bus.c),
 * so a request waits for it rather than starting a transfer it cannot
 * finish - and the wait is a sleep, so the register is taken first.
 */
static
d1busplease()
{
    di();
    if (!busget(d1ready))
        sleep(d1ready, PRIBIO);
    else
        ei();
}

/*
 * A raw command block from above (include/sys/ioctl.h).
 *
 * The block is the chip's own: the caller writes the command bytes in
 * cmd[0..len-1], with a hole at r->hole if the command has a data phase,
 * and the driver writes the result bytes back at cmd[len] - so the
 * caller leaves room at the end of the block for them.  Not one opcode
 * is looked at beyond the length of its result, and that is the whole
 * point: a FORMAT TRACK with its four byte header table per sector is
 * what a formatter needs, and the driver that issues read and write
 * knows nothing about it.
 *
 * The command runs polled rather than interrupt driven, the way the
 * HD-DMA's raw blocks do: it is synchronous, the caller sleeps in it,
 * and the controller is marked STOPPED meanwhile so the handler has
 * nothing to resume.  What the driver decides and the caller cannot is
 * the one address: a caller's buffer is a virtual address and the board
 * follows a physical one.
 */
int
disk1ioctl(dev, com, r, b)
    UINT dev, com;
    register struct cdb *r;
    register struct buf *b;
{
    static UINT8 res[7];
    static char *p;
    static UINT32 a;
    static UINT i, n, op, hole;
    struct info *info;

    if (com != CDBCMD || r->len < 1 || r->len > CDBMAX) {
        u.error = EINVAL;
        return (-1);
    }
    if (r->count < 0 || r->count > D1_NBYTES) {
        u.error = EINVAL;
        return (-1);
    }
    hole = r->hole;
    if (r->count && (hole < 0 || hole + 3 > r->len)) {
        u.error = EINVAL;
        return (-1);
    }
    op = r->cmd[0] & 0x1f;
    if (r->len + d1res(op) > CDBMAX) {
        u.error = EINVAL;       /* no room in the block for the results */
        return (-1);
    }
    info = &disk1s[dev & 7];

    /*
     * The controller serves one request at a time and this is a request:
     * fail rather than queue, the way the HD-DMA's and the HDCA's do.
     * The bus is taken first and given back last, because the data phase
     * is the caller's buffer and it must stay mapped across the transfer.
     */
    di();
    if (d1buf != 0) {
        ei();
        u.error = EBUSY;
        return (-1);
    }
    ei();
    d1busplease();
    /*
     * And again with the bus held: d1busplease() sleeps when the other
     * controller has it, and a request that arrived while it slept would
     * be issued into the controller this one is about to take.
     */
    if (d1buf != 0) {
        busgive(0);
        u.error = EBUSY;
        return (-1);
    }

    d1state = STOPPED;          /* polled: no interrupt is wanted of it */
    curdrv = dev & 7;
    d1info = info;
    d1sel();

    if (r->count) {
        p = bhold(b);
        a = ((UINT32) b->xmem << 12) | ((unsigned) p & 0xfff);
        out(D1_DMA, (UINT8) (a >> 16));
        out(D1_DMA, (UINT8) (a >> 8));
        out(D1_DMA, (UINT8) a);
    }

    d1fault = 0;
    for (i = 0; i < r->len; i++)
        d1putb(r->cmd[i]);

    n = 0;
    if (op != FDC_SPECIFY && d1wait()) {
        if (d1res(op) == 0) {
            /*
             * RECALIBRATE or SEEK: the interrupt is the answer, and
             * SENSE INTERRUPT STATUS is the only way to read and clear it.
             */
            d1putb(FDC_SENSEI);
            res[0] = d1getb();
            res[1] = d1getb();
            n = 2;
        } else {
            n = d1res(op);
            for (i = 0; i < n; i++)
                res[i] = d1getb();
        }
    }

    /*
     * The head is wherever the command left it, so this driver's idea of
     * where it is - trusted while CALIB is set - is a guess from here on.
     */
    info->flags &= ~CALIB;

    if (r->count)
        brel();
    busgive(0);

    if (d1fault || (op != FDC_SPECIFY && n == 0)) {
        u.error = EIO;
        return (-1);
    }
    for (i = 0; i < n; i++)
        r->cmd[r->len + i] = res[i];
    return (0);
}

/*
 * Simulate missing timeout hardware, as the other drivers' checks do: the
 * resident tick walks the modules, and this is the one-second check that
 * turns a silent board into an error instead of a hang.  There is no
 * reset line from the host to this card - only a system reset reaches
 * the 8272 - so recovery is limited to failing the request that is stuck
 * and letting the next one re-SPECIFY the chip and recalibrate the
 * drive, which is as much of a reset as the board offers.
 */
disk1check()
{
    di();
    if ((d1state == ACTIVE || d1state == SEEKING) && ++d1dead > D1DEADLINE)
        d1state = TOOLATE;
    ei();
    if (d1state == TOOLATE) {
        d1error();
        d1bank = -1;            /* the chip's clock is no longer known */
        d1state = STOPPED;
        curdrv = -1;            /* force re-selection */
        advque();
    }
    return (0);
}

/*
 * The entries the driver's header hands the kernel (sys/disk1hdr.c).
 */
struct biovec disk1bvec = { &disk1open, &disk1close, &disk1strat,
    &disk1ioctl };

/*
 * The probe: make the card raise its interrupt so the attach loop
 * (sys/probe.c) can discover which line it is on.  A RECALIBRATE of the
 * first drive completes, has no data side effect, and raises the chip's
 * interrupt - which is what the board's status register reports and the
 * attach reads out of the PIC.  The pending interrupt is left standing;
 * the first request's SENSE INTERRUPT STATUS clears it.
 */
disk1probe()
{
    out(D1_DSEL, 0);            /* the 8 inch bank */
    d1fault = 0;
    d1putb(FDC_RECAL);
    d1putb(0);
    return (0);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
