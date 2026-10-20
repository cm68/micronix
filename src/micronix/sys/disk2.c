/*
 * Micronix driver for the CompuPro DISK 2 hard disk controller.
 *
 * sys/disk2.c
 *
 * The DISK 2 is the simple one: a finite state machine on the board finds
 * a sector header and moves its data, but everything else is the host's
 * job - the seek is a count of step pulses the host sends by reading the
 * data port, and each DMA transfer goes through the SELECTOR CHANNEL,
 * which is programmed with the buffer's address and direction before the
 * command.  There is no in-memory command block, so there is no physical
 * address to work out and no init: the interface is include/sys/disk2.h.
 *
 * The driver is a top end and a bottom end, the shape sys/hdca.c argues
 * for: the strategy path queues a request and arms the phase that begins
 * it, and every phase after that runs from the card's completion
 * interrupt.  A phase issues what the board was asked for - a step pulse,
 * a NULL, a read - and returns; d2intr is the phase the next completion
 * resumes, and 0 when nothing is armed, which is what an interrupt left
 * over from the probe finds.  The step pulses are the one thing the host
 * has to do itself, because on this board they are the interface and not
 * a command; everything the board can be left to finish, it is.
 *
 * The board's interrupt is the manual's "done" flag: ATTN* falls when a
 * command finishes and the card pulls the VI line its J7..J14 jumper
 * selected.  So a request is issued and left, and the status check, the
 * retry and the next buffer all run in the handler.  Which line that is
 * is not this file's business: disk2probe tickles the card and the
 * resident attach (sys/probe.c, sys/INTR.md) discovers the line at boot.
 * A driver that named a line would be a driver compiled for one machine.
 *
 * The done flag is reset through the control register and not by reading
 * the status: writing ATTN* high makes the status register transparent
 * again.  So every command this driver issues is also the acknowledgement
 * of the one before it, and there is no separate end-of-interrupt to give
 * the card.
 *
 * The board has no seek command and no timer for one - the host pulses
 * the step line and the drive moves - so the wait for the arm to settle
 * is the board's NULL command: it has no function but to complete, which
 * it does when the drive reports SEEK COMPLETE, and its completion is the
 * interrupt that gets the driver moving again.  That is d2settle below,
 * and it is why a seek costs two commands and no spinning.
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
static void d2begin(), d2done(), d2error(), d2fin(), d2home(), d2load(),
    d2rw(), d2sc(), d2seek(), d2sel(), d2settle(), d2step();

#define NDRIVES 4               /* Number of drives */
#define RETRIES 10              /* no. of retries on r/w error */
#define MAXTRK 256              /* the most cylinders one header byte holds */
#define D2DEADLINE 1            /* ticks a request may sit in flight */

/*
 * Drive configuration information, one per drive - the same shape the
 * other disk drivers use: the first nine fields are struct dlgeom
 * (sys/dlabel.h) so sys/dlabel.c decodes the label through a view of
 * them, then this driver's own bookkeeping, then the cached slice map.
 * The label's timing is read and dropped: this board has no timers to
 * program - it steps when it is told to, and the drive's own step rate is
 * the drive's business.
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
 * Globals.  d2buf is the request in flight and the controller is free
 * exactly when it is zero; d2intr is the phase the card's next completion
 * resumes, and nothing is armed when it is zero.
 */
static struct info *d2info = 0; /* the drive of the request in flight */
static struct buf *d2buf = 0;   /* the request in flight */
static int (*d2intr) () = 0;    /* the phase the op-done interrupt resumes */
static UINT8 retry = 0;         /* number of retries so far */
static UINT8 curdrv = -1;       /* number of current drive */
static UINT8 d2dead = 0;        /* ticks the request has been in flight */

/*
 * Device open.  Identical to the HD-DMA's: the drive is described by its
 * label and by nothing else, so the label is read once per drive and its
 * geometry and slice table cached.
 */
disk2open(dev, mode)
    UINT dev, mode;
{
    struct info *info;
    UINT8 drive, sl;
    struct buf *b;

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
    info->firstsec = 1;
    info->roll = 0;
    info->curtrk = 0;

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
        info->firstsec = 1;
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
 * board has no light to turn off and the heads are not parked by this
 * controller.
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
    b->cyl = d2cyl(b->blk, info->spc, off, roll, len);
    di();
    if (d2buf == 0)
        d2buf = b;
    else
        bsort(d2buf, b);
    if (d2intr == 0 && d2buf == b)
        d2start();
    ei();
}

/*
 * Take the DMA bus for the first queued request.  A controller that asks
 * while another holds it is remembered as the heir (sys/bus.c) and
 * started from there; until then the request is queued and nothing on
 * this board is in flight, which is why the request in flight and the
 * queue are the same list.
 */
d2start()
{
    if (!busget(&d2start))
        return;
    d2begin();
}

/*
 * Begin the request at the head of the queue: select its drive, home it
 * if this is the first time it has been asked for anything, and seek.
 * Called with the bus held, and after that every phase runs from the
 * card's interrupt.
 */
static void
d2begin()
{
    curdrv = d2buf->dev & 3;
    d2info = &disk2s[curdrv];
    retry = 0;
    d2dead = 0;
    if (d2info->flags & CALIB)
        d2seek();
    else
        d2home();
}

/*
 * Move the head to the request's cylinder, one step pulse per cylinder,
 * and let the arm settle before the transfer.  Nothing is waited for
 * here: the settle is the NULL command, and its completion is what
 * resumes d2rw.
 */
static void
d2seek()
{
    static UINT track, cur;
    static int steps;
    static UINT8 dir;

    track = d2buf->cyl;
    cur = d2info->curtrk;
    if (track == cur) {
        d2rw();                 /* already there: the drive never moved */
        return;
    }
    steps = (int) track - (int) cur;
    dir = 0;
    if (steps < 0) {
        dir = D2_OP2;           /* step out, toward track 0 */
        steps = -steps;
    }
    d2step(dir, steps);
    d2info->curtrk = track;
    d2settle(d2rw);
}

/*
 * Home the head: step out until the drive says it is at track 0.  TRACK 0*
 * is the drive's own indication and is the only position feedback this
 * board has - the manual says most drives "require that the controller
 * software remember the current track", and a remembered track is a
 * guess after a reset or a failed seek, which is what this restores.  The
 * step count is bounded by what one header byte can address, so a drive
 * that never reports track 0 costs that many pulses and no more.
 */
static void
d2home()
{
    static UINT i;

    d2sel(curdrv, 0);
    out(D2_CTL, D2_ATTN | D2_OP2 | (curdrv & 3));    /* step out */
    for (i = 0; !(in(D2_CTL) & D2_TRK0); i++) {
        if (i >= MAXTRK)        /* a drive that never gets there */
            break;
        in(D2_DATA);            /* one step pulse */
    }
    d2info->curtrk = 0;
    d2info->flags |= CALIB;
    d2settle(d2seek);
}

/*
 * Step the head n cylinders in a direction.  The board has no seek
 * command: the host moves the head itself, one step pulse per cylinder,
 * by reading the data port, and OP2 in the control register drives the
 * direction line - which way is out is not something the manual states,
 * and it is a jumper on some drives; D2_OP2 here is the direction that
 * reaches track 0, which is what TRACK 0* below confirms.
 *
 * This is the one place the driver spins, and it has to: the pulses are
 * the interface, so they are issued back to back the way hdstep (sys/
 * hdca.c) pulses the HDCA's step line, and the drive's fast seek mode -
 * the manual's term for a drive that accepts and accumulates pulses at a
 * rate above its step rate and then seeks - is what makes that safe.  A
 * drive with no such mode wants the pulses paced to its step rate, which
 * is a number the manual defers to "your drive specifications"; the wait
 * for the arm to settle is the part that is offloaded, and that is the
 * NULL command in d2settle below.
 */
static void
d2step(dir, n)
    UINT8 dir;
    UINT n;
{
    out(D2_CTL, D2_ATTN | dir | (curdrv & 3));
    while (n--)
        in(D2_DATA);            /* one step pulse */
}

/*
 * Wait for the drive's arm to settle, then resume cont.  The board's NULL
 * command is exactly that wait: the manual gives it no function but to
 * complete - it fails with a time out after two index pulses, or when the
 * drive reports SEEK COMPLETE - and completion is the interrupt.  So this
 * is a command that is issued and left, like any other, and the phase
 * that follows is the one it resumes.  NULL is the opcode with no bits
 * set, which is why the command byte is the control register's own flags.
 */
static void
d2settle(cont)
    int (*cont) ();
{
    d2intr = cont;
    out(D2_CTL, D2_ATTN | D2_RUN | (curdrv & 3));
}

/*
 * Read or write the sector.  One sector per command - a buffer is 512
 * bytes, which is what the drive's sector holds - so the count is implied
 * and the whole transfer finishes in one interrupt.
 *
 * The selector channel is programmed first: it holds the address and the
 * direction for the transfer the board is about to make.  Then the drive
 * and head, the three header-match registers the board searches for, and
 * the command.  Nothing is waited for - the card's completion is the
 * interrupt that resumes d2done.
 */
static void
d2rw()
{
    static UINT spc, csec, head, sect;

    spc = d2info->spc;
    csec = d2buf->blk % spc;
    head = csec / d2info->sectors;
    sect = csec % d2info->sectors + d2info->firstsec;

    d2sc(d2buf, d2buf->flags & BREAD);  /* a disk read writes memory */
    d2sel(curdrv, head);
    d2load(d2buf->cyl, head, sect);
    d2intr = d2done;
    out(D2_CTL, D2_ATTN | D2_RUN |
        ((d2buf->flags & BREAD) ? D2_READ : D2_WRITE) | (curdrv & 3));
}

/*
 * The transfer finished.  The status register says how it went: the error
 * bits all clear is success, and a write additionally wants no write
 * fault.  Anything else is re-issued, up to RETRIES times, with a re-home
 * at the halfway point in case the seek went astray; a retry that runs
 * out fails the buffer and the queue moves on.  retry counts up and is
 * tested before it is used, because it is a byte: a count that ran off
 * the bottom would come back as 255 and retry for ever.
 *
 * The status has to be read before anything else is written to the board,
 * because a control write resets the done flag this interrupt is the
 * other half of.
 */
static void
d2done()
{
    static UINT8 st;

    st = in(D2_CTL);
    if (!(st & D2_ERRMASK) && ((d2buf->flags & BREAD) || (st & D2_WFLT))) {
        d2fin();
        return;
    }
    if (++retry > RETRIES) {
        d2error();
        d2fin();
        return;
    }
    if (retry == RETRIES / 2) {
        d2info->flags &= ~CALIB;        /* the head position is a guess now */
        d2home();
        return;
    }
    d2rw();                     /* the registers are still loaded */
}

/*
 * Finish the request: hand the buffer back, and take the next one off the
 * queue.  The controller is free when the queue is, which is where the
 * bus is given back.
 */
static void
d2fin()
{
    struct buf *f;

    iodone(d2buf);
    f = d2buf->forw;
    d2buf->forw = 0;
    d2buf = f;
    if (d2buf == 0) {
        d2intr = 0;
        busgive(0);
        return;
    }
    d2begin();
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
    UINT32 phys;

    /*
     * A buffer's data pointer is a window address and its segment is
     * b->xmem, so the address the channel has to hold is the segment
     * folded into the address, exactly as sys/dj.c does for the DJ-DMA.
     * The card is a bus master and sees physical memory; the window
     * address is the kernel's alone.
     */
    phys = ((UINT32) b->xmem << 12) | ((unsigned) b->data & 0xfff);

    in(D2_SC);                              /* disable the channel */
    out(D2_SC, (UINT8) (phys >> 16));       /* A23-A16, extended byte */
    out(D2_SC, (UINT8) (phys >> 8));        /* A15-A8 */
    out(D2_SC, (UINT8) phys);               /* A7-A0 */
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
 * Flag an error
 */
static void
d2error()
{
    d2buf->flags |= BERROR;
}

/*
 * The interrupt handler.  The board has one completion signal and the
 * driver one armed phase, so this is the call and no more: a phase issues
 * what it wants done next and arms the phase after that before it
 * returns, which is what leaves an interrupt with nothing to resume when
 * the card's done flag is one the attach already accounted for.
 */
disk2int()
{
    int (*f) ();

    f = d2intr;
    if (f == 0)
        return (0);
    d2intr = 0;
    (*f) ();
    return (0);
}

/*
 * The watchdog, the driver's tick (sys/disk2hdr.c), run once a second by
 * the resident tick.  A board that has stopped answering raises no
 * interrupt, which is the whole reason this exists: a request whose seek
 * or transfer never completes is failed here, the only way it ever ends.
 * Normal requests finish in milliseconds, so the counter only reaches the
 * deadline when the drive is wedged.  The armed phase is dropped first,
 * so a late interrupt - if the board ever does answer - finds nothing to
 * resume.
 */
disk2check()
{
    if (d2buf == 0)
        return (0);
    if (++d2dead <= D2DEADLINE)
        return (0);
    di();
    d2intr = 0;
    ei();
    d2info->flags &= ~CALIB;    /* the head position is a guess now */
    d2buf->flags |= BERROR;
    d2buf->error = ENXIO;
    d2fin();
    return (0);
}

/*
 * The entries the driver's header hands the kernel (sys/disk2hdr.c).
 */
struct biovec disk2bvec = { &disk2open, &disk2close, &disk2strat, 0 };

/*
 * The probe: make the card raise its interrupt so the attach loop
 * (sys/probe_init.c) can discover which line it is on.  Called with the
 * module mapped and every line masked, before anything has been read or
 * written.
 *
 * The tickle is the one command this board has that does nothing: NULL,
 * which waits for SEEK COMPLETE and gives up after two index pulses.  The
 * manual gives it no function but that - "this completion would interrupt
 * the CPU indicating that the drive was ready to accept a read or write
 * command" - so it searches for no sector, moves no data, and finishes
 * either way: a card with no drive on it still lights its done latch.
 * What the attach reads is the latch, so it has to be up when this
 * returns, which is what the bounded wait below is for: it is not the
 * interrupt, there is nothing wired to take it yet.
 *
 * A machine with no DISK 2 has nothing at port 0xc8 and reads the bus's
 * 0xff, which holds ATTN* - it never comes back, and this is the presence
 * test (sys/INTR.md): no answer has to return as a result and not as a
 * hang, so the poll is bounded.  The bound is generous: a real card
 * answers in two index pulses, some tens of milliseconds, and the loop
 * does nothing but read the one port.
 *
 * Nothing is armed when it returns, so the interrupt the attach leaves
 * behind - it clears the PIC's bit but not the card's latch - is one
 * disk2int finds nothing to resume for.
 */
#define PROBEWAIT 10000

disk2probe(mode)
	int mode;
{
    register int n;

    if (mode == PROBE_CLEAR) {
        out(D2_CTL, 0);                 /* a write with RUN clear releases the line */
        return (0);
    }

    d2sel(0, 0);                        /* drive 0, head 0 */
    out(D2_CTL, D2_ATTN | D2_RUN);      /* NULL: wait for seek complete */
    for (n = 0; n < PROBEWAIT && (in(D2_CTL) & D2_SATTN); n++)
        ;
    d2intr = 0;
    return (0);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
