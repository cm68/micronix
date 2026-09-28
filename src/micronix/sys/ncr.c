/*
 * Micronix driver for a SCSI host adapter built on the NCR 5380.
 *
 * The cpu moves every byte of a transfer itself - programmed I/O, PIO,
 * with no DMA controller - and the card raises an interrupt when the
 * target lets go of the bus.  Only two commands are ever issued, READ(10)
 * and WRITE(10); there is no INQUIRY, no READ CAPACITY, no MODE SENSE and
 * no REQUEST SENSE, and the geometry comes off the disk label the same
 * way it does for every other disk here.
 *
 * sys/ncr.c
 * Changed: <2026-09-26 curt>
 *
 * This driver is a module (sys/OVERLAY-DRIVERS.md), and that is what let
 * it be linked at all.  As resident kernel text its 3241 bytes did not
 * fit: the link failed with "objects out of address order (data)" 155
 * bytes short of the frame, and the buffer pool was the tighter of the
 * two budgets.  sys/TODO works both out and names the three ways it
 * could have been made to fit; sys/overlay.md is the one written for
 * this, and it is the one that was taken.  The build names it in MODS
 * (GNUmakefile) and appends its page to the kernel file, the loader
 * places it at boot (ovlplaceall, sys/main_init.c), and major 5
 * (sys/consts.c) reaches it through the same ovlopen/ovlclose/ovlstrat
 * trampolines the other three drivers do.
 *
 * The names it exports are the module's interface - ncropen, ncrclose,
 * ncrstrat and ncrint, handed over in ncrbvec and the header
 * (sys/ncrhdr.c) - and they are named for the card where the routines
 * below are named for the bus.  SCSI is the protocol: the handshake, the
 * phases and the command blocks are SCSI's on any adapter, and the four
 * routines above are this adapter's.  The device nodes stay scsi*
 * (dev/devlist) for the same reason - scsi0..scsi7 are b 5 0..7, the
 * target being the low three bits of the minor, and the whole-disk
 * slices scsi0c..scsi7c are b 5 64..71.
 *
 * The label decode is sys/dlabel.c's, shared with mw.c and ide.c, so
 * what a slice means is decided in one place - which matters more here
 * than the bytes it saves, since a disk read by one driver and written
 * by another is the failure the label exists to prevent.
 *
 * Terms used throughout, in the order they first matter:
 *
 *   SCSI        small computer system interface: a bus, not a drive.  Up
 *               to eight devices share it, each with a number of its own
 *               called its id, and any one of them can be the one talking
 *               (the initiator) or the one answering (the target).  Here
 *               the card is always the initiator and a disk is always the
 *               target; nothing else on this machine speaks SCSI.
 *   NCR 5380    the bus interface chip: eight registers the cpu writes
 *               and reads, and a set of bus transceivers.  It does not
 *               know what a disk is, and a command means nothing to it -
 *               it drives the wires, and the driver tells it when to.
 *   CDB         command descriptor block: the 6, 10 or 12 bytes that ARE
 *               the command.  A disk has no command register; you hand it
 *               a CDB and it does what the bytes say.  READ(10) and
 *               WRITE(10) below are the two this driver builds, and each
 *               names an LBA and a block count.
 *   LBA         logical block address: the whole disk as one run of
 *               512-byte blocks numbered from 0.  SCSI is addressed this
 *               way natively, and the mapping below converts to it for
 *               the same reason ide.c does - so that a volume laid out
 *               for the HD-DMA reads back block for block here.
 *   phase       what the bus is being used for at this instant.  The
 *               target moves the bus through them in order and the driver
 *               follows: BUS FREE (nobody is talking), SELECTION (the
 *               card is calling a target), COMMAND (the CDB goes out),
 *               DATA IN or DATA OUT (the blocks), STATUS (one byte: did
 *               it work), MESSAGE IN (one byte: the target's parting
 *               word), and back to BUS FREE.  Three wires say which -
 *               MSG, C/D and I/O - and the target owns them.
 *   REQ, ACK    the handshake, one byte at a time.  The target raises REQ
 *               when it has a byte or wants one; the initiator moves the
 *               byte and raises ACK; the target drops REQ; the initiator
 *               drops ACK.  That round trip per byte is the whole of the
 *               transfer, and it is what "no DMA" costs.
 *   SEL, BSY    select and busy, the two wires the two ends claim the bus
 *               with.  The card raises both to call a target and the
 *               target answers by raising BSY of its own; from then until
 *               the end of the command BSY stays up, and BSY going away
 *               is how the command ends.
 *   VI, PIC     vectored interrupt and programmable interrupt controller:
 *               the S-100 bus's eight interrupt lines, and the 8259 on the
 *               Mult I/O board which ranks them and hands the cpu a vector
 *               when it acknowledges one.  This card shares VI2 with the
 *               IDE card; see NCRINT, include/sys/ncr.h.
 *   strategy    the entry point the buffer cache calls to start a
 *               transfer (uio.c).  mw.c's and ide.c's are the models.
 *   window      the single page of address space that bhold() maps a
 *               buffer's segment into for the length of an access, so
 *               that the kernel can reach a buffer in another segment.
 *
 * THE INTERRUPT, AND WHAT IT IS FOR
 *
 * A 5380 in initiator mode with no DMA controller has exactly two things
 * to say asynchronously, and this driver is built on both:
 *
 *	the bus went free	an unexpected BSY that never was, which is how
 *				the chip reports that the target has let go.
 *				It is enabled by MONITOR BSY (mode register
 *				bit 2) and it is the end of the command.
 *	bad parity		the chip checks odd parity on every byte it
 *				hands over, when asked to.
 *
 * and one thing it CANNOT say: that a data byte is ready.  That is DRQ,
 * which is a DMA pin, and there is no DMA controller here to notice it.
 * So the byte handshake is polled, by the cpu, in the caller's context -
 * and that is not a shortcut, it is what the absence of DMA means.  What
 * the interrupt buys is that the driver never spins waiting for the
 * TARGET: the target's own work (finding the block, writing it, releasing
 * the bus) is waited out in sleep(), and the handler is what ends it.
 *
 * The one wait this does not cover is the target's first byte, which for
 * a READ arrives after the seek.  SCSI's own answer is for the target to
 * disconnect and be reselected - and a reselect is an interrupt - but
 * that is a state machine this driver does not have, so the wait is the
 * bounded poll in scsibyte() and its bound is why it is bounded.
 *
 * Sharing VI2 with the IDE card no longer asks anything of this file.
 * The line is level triggered and the 8259 is told the interrupt is over
 * once, when intrupt (mio.s) returns, so both cards on the line have to
 * be serviced in that one entry - and the resident dispatcher does it:
 * ovlntr (sys/ovl.c) holds two handlers per line and calls each in turn,
 * mapping each handler's own segment (ovlcall) because a handler runs at
 * a moment its page is not necessarily the one mapped.  Each handler
 * reads its own card's registers, which is what takes its own line down,
 * and each returns at once when its card has nothing in flight.  This
 * driver used to chain the two by hand (scsii2int); that routine is gone
 * with the module split, since ideint is in another module now and this
 * one cannot name it.
 *
 * The shape of the driver is ide.c's, because the filesystem is the same
 * one.  Open reads the disk label mkfs writes into the boot sector - the
 * decode is sys/dlabel.c's, shared with mw.c and ide.c so that what a
 * slice means is decided in one place - the block mapping is the same one
 * (a cylinder is blk/spc shifted by the slice the minor names and rotated
 * by d_roll, wrapped at tracks) flattened the way lib/fslib.c flattens it,
 * and the strategy routine is the same entry point with the same contract.
 * A volume laid out for the HD-DMA therefore reads back block for block
 * here.
 *
 * What differs is that ide.c's drive is addressed by somebody else's
 * idea of where a block is, and this one by SCSI's own.  So the transfer
 * is a command and not a register write: build the CDB, call the target,
 * hand the CDB over, move the blocks, take the status, and let go.
 */

#include <types.h>
#include <sys/sys.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <sys/con.h>
#include <sys/dlabel.h>
#include <sys/ioctl.h>
#include <sys/ncr.h>
#include <errno.h>

/*
 * Declared before anything calls them: an undeclared call is extern int
 * by default and the definitions say static.
 */
static int scsibsy(), scsistart(), scsifinish();
static int scsireset(), scsiselect();

/*
 * How many targets the bus can hold, and so how many rows scs[] below
 * carries.  Eight is the bus's own limit and not a choice: the select
 * mask is one bit per id in a single byte, and the target number is the
 * low three bits of the minor.
 */
#define NTARGET 8

/*
 * The adapter's own id.  Seven is the convention - it is the highest
 * number on the bus and the one every host adapter takes - and it matters
 * only in that it must differ from every target's.
 */
#define HOSTID  7

/*
 * The interrupt line, the card's ports, and the command register the
 * presence probe reads are in include/sys/ncr.h, because the resident half
 * of this driver needs them and resident code cannot name anything in a
 * module: sys/ncrhdr.c carries NCRINT to the interrupt dispatcher,
 * sys/ncrinit.c is the probe, and neither is linked into the page below.
 * They are numbers, so they travel; the driver's own tables do not, and
 * the probe is written to need none of them.
 */

/*
 * The register set, by number.  Five of the eight mean one thing read and
 * another written, which is why they have two names each; the number is
 * the address.  ICR is the one number not below - it is in <sys/ncr.h>
 * with the probe that reads it back.
 */
#define CSD     0               /* read: current data on the bus
                                   write: output data, what we drive */
/*                               ICR, 1 - the initiator command register:
                                 SEL, BSY, ACK, RST, ATN, DATA */
#define MR      2               /* mode */
#define TCR     3               /* target command: unused, we are initiator */
#define CSBS    4               /* read: current SCSI bus status
                                   write: select enable */
#define BASR    5               /* read: bus and status */
/*                               write 5, 6 and 7 start a DMA transfer, and
                                 there is no DMA controller here - writing
                                 them would arm a transfer nothing answers */
#define IDR     6               /* read: input data, the same byte as CSD */
#define RPI     7               /* read: clears the interrupt latches, which
                                   is the acknowledgement.  Reading it is
                                   not optional: the 5380 holds IRQ down
                                   until it is read, and the 8259 is level
                                   triggered, so the line would fire again
                                   the moment intrupt wrote its end-of-
                                   interrupt. */

/*
 * Initiator command register.  Bits 5 and 6 and bit 5's write side mean
 * other things on a 5380 used as a target; as an initiator only the six
 * below matter.
 */
#define ICR_RST     0x80        /* assert RST, which resets the bus */
#define ICR_ACK     0x10        /* assert ACK, our half of the handshake */
#define ICR_BSY     0x08        /* assert BSY, our claim on the bus */
#define ICR_SEL     0x04        /* assert SEL */
#define ICR_ATN     0x02        /* assert ATN: a message is coming */
#define ICR_DATA    0x01        /* our output data register drives the bus */

/*
 * Mode register.  Everything else stays clear: this is not a target, it
 * does not DMA, it does not arbitrate (see scsiselect), and it does not
 * ask the chip to check parity - the parity line is generated and checked
 * by the chip when both ends agree to, and a machine whose target or
 * cabling does not is a machine where turning the check on buys a fault
 * on every byte.  MONITOR BSY is the one bit that earns its place: it is
 * what makes the bus going free an interrupt.
 */
#define MR_MONBSY   0x04        /* interrupt when BSY goes away */

/*
 * Current SCSI bus status register, read: what the wires are doing, ours
 * and the target's both - a set bit is an asserted signal whoever drove
 * it.  The three phase wires are read together to tell which phase the
 * target has moved the bus to.
 */
#define CSBS_RST    0x80
#define CSBS_BSY    0x40
#define CSBS_REQ    0x20
#define CSBS_MSG    0x10
#define CSBS_CD     0x08
#define CSBS_IO     0x04
#define CSBS_SEL    0x02

/* the three phase wires, and the phases they name */
#define PHASE       (CSBS_MSG | CSBS_CD | CSBS_IO)
#define P_DATAOUT   0x00
#define P_DATAIN    CSBS_IO
#define P_COMMAND   CSBS_CD
#define P_STATUS    (CSBS_CD | CSBS_IO)
#define P_MSGOUT    (CSBS_CD | CSBS_IO | CSBS_MSG)
#define P_MSGIN     (CSBS_CD | CSBS_IO | CSBS_MSG)

/*
 * Bus and status register, read.
 */
#define BASR_EOP    0x80        /* end of DMA: never, there is no DMA */
#define BASR_DRQ    0x40        /* a byte is ready, DMA's request line.  The
                                   driver polls this one; it is the only
                                   thing that says a byte is there */
#define BASR_PERR   0x20        /* the chip saw bad parity */
#define BASR_IRQ    0x10        /* the chip is asking for attention */
#define BASR_PHASE  0x08        /* the bus phase matches the target command
                                   register.  Target mode's mechanism; a
                                   set bit here means nothing to an
                                   initiator */
#define BASR_BUSY   0x04        /* BSY went away unexpectedly: the target
                                   has finished, or has gone */
#define BASR_ATN    0x02
#define BASR_ACK    0x01

/*
 * Status byte, the one the target sends in the STATUS phase.  The only
 * bit this driver acts on is CHECK CONDITION, and it acts by giving up:
 * finding out why means REQUEST SENSE, and this driver does not send one.
 */
#define ST_CHECK    0x02        /* check condition: see the sense data the
                                   driver does not ask for */

/*
 * Message byte, the one the target sends in the MESSAGE IN phase.  One
 * value is worth noticing: a target that answers DISCONNECT is telling
 * the driver it is going away and will reselect, and this driver has no
 * reselect, so it says so rather than taking the next byte for a status.
 */
#define MSG_DISCONN 0x04        /* command complete, target disconnecting */
#define MSG_CMDCOMP 0x00        /* command complete */

/*
 * The READ(10) and WRITE(10) command descriptor blocks.  Ten bytes, and
 * the two differ in the first one:
 *
 *	0	operation code
 *	1	LUN in the top three bits, LBA bits 23-16 in the low five.
 *		The LUN is zero: one target, one logical unit, always.
 *	2,3	LBA bits 15-8, then 7-0
 *	4	LBA bits 31-24.  Zero above bit 23, because a block number
 *		here is a UINT and cannot reach that far - the field is
 *		written out anyway so the CDB is well formed.
 *	5	reserved
 *	6,7	how many blocks, high byte first
 *	8	zero
 *	9	control: zero, which means no linked commands and no
 *		vendor specific flags.
 *
 * The two are built by one routine with the opcode as an argument, so
 * the ten bytes are laid out once.
 */
#define CREAD10 0x28            /* read(10) */
#define CWRITE10 0x2a           /* write(10) */

#define CDBLEN  10              /* the length of either */

/*
 * Polls before a bus wait is called failed.  A loop count, not a
 * calibrated interval, and the same shape as ide.c's SPIN: kernel code is
 * not preempted, so a target that never answers holds the machine for the
 * whole of this bound instead of failing one request.  It is sized for
 * the longest thing a target legitimately does across a bare handshake -
 * a drive that has spun down and is finding the block - and it is why the
 * wait for the first byte of a READ is the one that costs.
 */
#define SPIN    200000L

/*
 * An interface: which ports the chip's registers answer at.  The four
 * calls are every way this driver touches the card, and a second adapter
 * design - one that maps the registers into memory instead of strobing
 * them, or holds the chip at another address - is another row in
 * boards[] rather than another driver.
 *
 * The members below the calls are the card's own state, which lives here
 * because there is one card and one command in flight on it at a time.
 * overlay.md's seam puts this struct and the phase engine that reads it
 * on the resident side of the overlay: the handler runs when the
 * overlaid top end is not mapped, so everything a resident handler
 * touches has to be resident too.
 */
struct scsiif
{
    int (*rd)();                /* rd(reg) -> byte */
    int (*wr)();                /* wr(reg, byte) */
    int (*reset)();             /* reset() */
    int (*intr)();              /* intr(on): connect or cut the chip's IRQ */

    int busy;                   /* a command is out and not yet finished */
    int done;                   /* ... and the card has interrupted for it */
    int status;                 /* what the handler's register read found */
    int error;                  /* ... and what it made of that */

    UINT8 cdb[CDBLEN];          /* the command in flight */
};

/*
 * One register, read: the register number is the port.
 */
static int
myrd(reg)
    UINT8 reg;
{
    return (in(SCSIBASE + reg));
}

/*
 * One register, written.
 */
static int
mywr(reg, val)
    UINT8 reg;
    UINT8 val;
{
    out(SCSIBASE + reg, val);
    return (0);
}

/*
 * Reset the chip and the bus it drives.
 *
 * RST is asserted on the bus, held, and let go.  The read in the loop is
 * the delay - it goes out to the bus, so the compiler cannot drop the
 * loop - and it is harmless because a chip in reset answers nothing.  The
 * mode register is written after it and not before, because a reset
 * clears it.
 *
 * This is what makes a card left mid-command usable again, and so what
 * lets open trust the label read that follows.
 */
static int
myreset()
{
    static UINT n;

    out(SCSIBASE + ICR, ICR_RST);
    for (n = 0; n < 0x40; n++)
        in(SCSIBASE + CSBS);
    out(SCSIBASE + ICR, 0);
    out(SCSIBASE + MR, MR_MONBSY);
    return (0);
}

/*
 * Connect or cut the chip's interrupt line.
 *
 * The 5380 has no interrupt enable of its own - MONITOR BSY arms the
 * condition, and the card's glue is what puts IRQ on the bus - so this
 * driver's version of ide.c's device-control register is to arm and
 * disarm the condition.  Cutting it across a reset is the point: a reset
 * is the one moment the chip is entitled to raise IRQ with nothing
 * behind it.
 */
static int
myintr(on)
    int on;
{
    out(SCSIBASE + MR, on ? MR_MONBSY : 0);
    return (0);
}

/*
 * Boards, indexed by the two type bits of the minor number the way
 * ide.c's boards[] is indexed by devtype(dev).  One card today, so the
 * field must be zero - which is what the bound check in ncropen() is
 * for.
 */
static struct scsiif boards[] = {
    {&myrd, &mywr, &myreset, &myintr, 0, 0, 0, 0, 0},
};

/*
 * Target configuration.  Unlike mw.c's info there is nothing in here that
 * a table holds: a SCSI disk is told an LBA and finds the block itself, so
 * there is no per-model row of step rates, write precompensation and the
 * rest, and mw.c's table - which stands in for a label the disk does not
 * have - has nothing to stand in for here.  What is here is the geometry
 * sys/dlabel.c decoded, and the mapping built from it.
 *
 * The first seven fields are struct dlgeom (sys/dlabel.h), in its order
 * and of its types, because sys/dlabel.c decodes the label through a view
 * of them - ncropen passes this struct as one.  The bookkeeping below
 * them is this driver's own; moving any of the seven breaks the decode in
 * a way the compiler will not report.
 */
struct info
{
    UINT tracks;                /* cylinders */
    UINT8 heads;
    UINT8 sectors;              /* sectors per track */

    UINT maxblk;                /* max legal block number in the slice */
    UINT spc;                   /* sectors per cylinder */
    UINT roll;                  /* what scsicyl adds to blk / spc */
    UINT cylstart;              /* first cylinder of the slice being read */
    UINT8 flags;                /* see below */
    UINT8 type;                 /* index into boards[] */
    UINT8 slice;                /* the slice this open is bound to */
} scs[NTARGET] = 0;

#define OPEN    2               /* target is open */

/*
 * Wait for the bus to be free - BSY and SEL both down - and return the
 * status that said so, or -1 if it stayed busy past the spin bound and
 * the driver is giving up on it.
 */
static int
scsibsy(ip)
    register struct scsiif *ip;
{
    static UINT32 n;
    static int st;

    for (n = 0; n < SPIN; n++) {
        st = (*ip->rd) (CSBS);
        if (!(st & (CSBS_BSY | CSBS_SEL)))
            return (st);
    }
    return (-1);
}

/*
 * One byte across the bus, one REQ/ACK round trip, in whichever direction
 * the phase says.
 *
 * This is the whole of the transfer machinery and it is deliberately
 * small, because it runs once per byte of every request.  The order of
 * the handshake is the bus's and not a choice: the target raises REQ when
 * it has a byte or wants one, the initiator moves the byte, the initiator
 * raises ACK, the target drops REQ, the initiator drops ACK.  Reading
 * before ACK on an input and driving before ACK on an output is what
 * keeps the byte on the wires while the target is looking at it.
 *
 * The poll is DRQ in the bus and status register - the chip's mirror of
 * REQ - and not REQ in the bus status register, because DRQ is the bit
 * the chip sets when it is ready for the cpu, which is what is being
 * asked.  Returns 0, or -1 if REQ never came.
 */
static int
scsibyte(ip, out, bp)
    register struct scsiif *ip;
    int out;
    register char *bp;
{
    static UINT32 n;
    static int st;

    for (n = 0; n < SPIN; n++) {
        st = (*ip->rd) (CSBS);
        if (st & CSBS_REQ)
            break;
    }
    if (!(st & CSBS_REQ))
        return (-1);

    if (out) {
        (*ip->wr) (CSD, *bp);
        (*ip->wr) (ICR, ICR_DATA);
        (*ip->wr) (ICR, ICR_DATA | ICR_ACK);
        (*ip->wr) (ICR, ICR_DATA);
    } else {
        *bp = (*ip->rd) (CSD);
        (*ip->wr) (ICR, ICR_ACK);
        (*ip->wr) (ICR, 0);
    }

    for (n = 0; n < SPIN; n++) {
        st = (*ip->rd) (CSBS);
        if (!(st & CSBS_REQ))
            return (0);
    }
    return (-1);
}

/*
 * Move a run of bytes, all of them in the phase the target has put the
 * bus in.  Returns 0, or -1 at the first byte that never came.
 */
static int
scsiio(ip, p, n, out)
    register struct scsiif *ip;
    register char *p;
    UINT n;
    int out;
{
    static UINT i;

    for (i = 0; i < n; i++)
        if (scsibyte(ip, out, p + i) < 0)
            return (-1);
    return (0);
}

/*
 * Call a target and wait for it to answer.
 *
 * No arbitration: the card takes the bus rather than competing for it,
 * which is legal on a bus with one initiator and is what every simple
 * host adapter does.  A machine with two initiators would need the
 * arbitrate bit in the mode register and a lost-arbitration path, and
 * this one has neither.
 *
 * The sequence is SCSI's and its order is the bus's, not a preference:
 * the id bits go on the data lines first, then SEL to say a selection is
 * happening, then BSY to claim the bus.  The target answers by raising
 * BSY of its own, and only then does the data bus come off - the target
 * has to be able to read the id bits it was just handed.  SEL goes last.
 *
 * The deskew delays the bus specification asks for between those steps
 * are not written out: each one is a couple of hundred nanoseconds and
 * each port write on a 4MHz Z80 is nearer a microsecond, so the
 * instruction stream is the delay.
 *
 * Returns 0, or -1 if the bus never went free or nobody answered.  The
 * caller has already taken the card, so the failure path here only has
 * to leave the bus alone.
 */
static int
scsiselect(ip, target)
    register struct scsiif *ip;
    UINT8 target;
{
    static UINT32 n;
    static int st;
    static UINT8 mask;

    if (scsibsy(ip) < 0)
        return (-1);

    mask = (1 << HOSTID) | (1 << target);

    (*ip->wr) (CSD, mask);
    (*ip->wr) (ICR, ICR_DATA);
    (*ip->wr) (ICR, ICR_DATA | ICR_SEL);
    (*ip->wr) (ICR, ICR_DATA | ICR_SEL | ICR_BSY);

    for (n = 0; n < SPIN; n++) {
        st = (*ip->rd) (CSBS);
        if (st & CSBS_BSY)
            break;
    }
    if (!(st & CSBS_BSY)) {         /* nobody home at that id */
        (*ip->wr) (ICR, 0);
        return (-1);
    }

    (*ip->wr) (ICR, ICR_SEL | ICR_BSY);         /* the id bits come off */
    (*ip->wr) (ICR, ICR_BSY);                   /* and SEL */
    (*ip->wr) (ICR, 0);                         /* our BSY; the target keeps its own */

    return (0);
}

/*
 * The command phase: hand the CDB over, one byte per handshake.
 */
static int
scsicmd(ip)
    register struct scsiif *ip;
{
    return (scsiio(ip, ip->cdb, CDBLEN, 1));
}

/*
 * Build a READ(10) or WRITE(10) for one block at lba and leave it in the
 * card's command block.  The ten bytes are laid out once, here, so the
 * two commands cannot drift apart.
 */
static void
scsibuild(ip, read, lba, nblk)
    register struct scsiif *ip;
    int read;
    UINT32 lba;
    UINT nblk;
{
    static UINT8 *c;

    c = ip->cdb;
    c[0] = read ? CREAD10 : CWRITE10;
    c[1] = (UINT8)((lba >> 16) & 0x1f);         /* LUN 0 in the top three bits */
    c[2] = (UINT8)(lba >> 8);
    c[3] = (UINT8)lba;
    c[4] = (UINT8)(lba >> 24);                  /* zero: a block number is 16 bits */
    c[5] = 0;
    c[6] = (UINT8)(nblk >> 8);
    c[7] = (UINT8)nblk;
    c[8] = 0;
    c[9] = 0;
}

/*
 * The interrupt handler, the card's half of the shared line.
 *
 * sys/intrpt.s routes VI2 to the resident dispatcher (ovlint2), which
 * holds this handler's segment in its line table and calls ncrint through
 * ovlcall with this module's page mapped - and intrupt (mio.s) has
 * already saved the machine state, taken interrupts off for the duration
 * and will write the end-of-interrupt to the 8259 when this returns, so
 * there is nothing to save here and nothing to acknowledge beyond the
 * chip itself.
 *
 * The acknowledgement is the read of RPI, and it comes first: the 5380
 * holds IRQ down until that register is read, the 8259 is level
 * triggered, and intrupt's end-of-interrupt would otherwise land on a
 * line that is still asking.  Reading it clears the parity, IRQ and
 * busy-error latches together, so the bus and status register has to be
 * read first - it is the only place the reason survives.
 *
 * What is left to decide is whether the interrupt means anything.  A
 * card with nothing in flight has nothing to report, and a busy error
 * with no command out is exactly what a bus reset from another device
 * looks like; setting `done` for it would leave a flag for the next
 * request to find, and that request would return before its target had
 * even been called.  So the flag is only set when a command really is in
 * flight.
 */
ncrint()
{
    register struct scsiif *ip;

    ip = &boards[0];

    ip->status = (*ip->rd) (BASR);
    (*ip->rd) (RPI);                    /* the acknowledgement */

    if (!ip->busy)
        return;

    if (ip->status & BASR_PERR)
        ip->error = EIO;

    if (ip->status & (BASR_PERR | BASR_BUSY)) {
        ip->done = 1;
        wakeup(&ip->done);
    }
}

/*
 * Wait for the handler to say the card has answered.  The shape is the
 * kernel's canonical one - uio.c's block() and bwait() are the same three
 * lines - and it is not decoration: the test and the sleep are inside one
 * di(), so a target that lets go between the two cannot be missed, and
 * sleep() returns with interrupts enabled, which is why the di() is redone
 * on every pass rather than held across the loop.
 *
 * The event is the address of the flag itself.  That is the smallest thing
 * that will do, and it is distinct from the card address, so a process
 * waiting here is not woken by the next process waiting for the card.
 */
static int
sciwait(ip)
    register struct scsiif *ip;
{
    for (;;) {
        di();
        if (ip->done)
            break;
        sleep(&ip->done, PRIBIO);
    }
    ip->done = 0;
    ei();
    return (0);
}

/*
 * Start a command: take the card, call the target, hand over the CDB.
 *
 * One command per card at a time.  The chip could be interleaved between
 * two requests in principle, but there is one bus and one target selected
 * on it, and a second request that reached the card before the first had
 * finished would be talking to whatever target the wires happened to name
 * last.  So requests queue behind the busy flag and wait on the card's
 * own address, which is what scsifinish() wakes.
 *
 * Nothing is held across the sleep that waits for the card: the window
 * discipline in uio.c forbids sleeping with a buffer held, which is why
 * bhold() sits in ncrstrat() and not here.
 */
static int
scsistart(ip, target, read, lba, nblk)
    register struct scsiif *ip;
    UINT8 target;
    int read;
    UINT32 lba;
    UINT nblk;
{
    for (;;) {
        di();
        if (!ip->busy)
            break;
        sleep(ip, PRIBIO);
    }
    ip->busy = 1;
    ip->done = 0;
    ip->error = 0;
    ei();

    scsibuild(ip, read, lba, nblk);

    if (scsiselect(ip, target) < 0 || scsicmd(ip) < 0) {
        ip->busy = 0;
        wakeup(ip);
        return (-1);
    }
    return (0);
}

/*
 * Finish a command: the caller has moved the blocks and this is the
 * status, the message and the end.
 *
 * The blocks are already across - scsiio() did that under the caller's
 * window - so what is left is the target's two parting bytes and the bus
 * going free.  Both are read with the same handshake as the data, and
 * then the wait for BSY to drop is the one place this driver sleeps on
 * the card's interrupt rather than polling, which is what the interrupt
 * is for: the target's own work between the last byte and letting go is
 * the target's business and there is nothing to poll while it happens.
 *
 * A target that answers CHECK CONDITION is reported as a failed request
 * and not diagnosed.  Finding out why means REQUEST SENSE, which this
 * driver does not send, so the request fails with EIO and the reason
 * stays in the target's sense data where nothing here reads it.
 *
 * Returns 0, or -1 if the target reported a problem or stopped answering.
 */
static int
scsifinish(ip, target)
    register struct scsiif *ip;
    UINT8 target;
{
    static int st, msg, bad;

    bad = 0;

    st = 0;
    if (scsibyte(ip, 0, (char *)&st) < 0)
        bad = 1;
    else if (st & ST_CHECK) {
        ip->error = EIO;
        bad = 1;
    }

    msg = 0;
    if (scsibyte(ip, 0, (char *)&msg) < 0)
        bad = 1;
    else if (msg == MSG_DISCONN)
        /*
         * The target is going away and means to be called back.  Nothing
         * here can call it back, so the command is over and failed rather
         * than half finished with a stale status taken for a good one.
         */
        bad = 1;

    /*
     * The end.  Between the last message byte and BSY dropping there is
     * nothing to poll, so this is where the driver waits on the card
     * instead of on the bus - see the header.
     */
    if (!bad) {
        ip->done = 0;
        di();
        st = (*ip->rd) (CSBS);
        if (st & CSBS_BSY)
            sleep(&ip->done, PRIBIO);
        ip->done = 0;
        ei();
        if (ip->error)
            bad = 1;
    }

    if (bad || scsibsy(ip) < 0)
        ip->error = EIO;

    ip->busy = 0;
    wakeup(ip);

    return (ip->error ? -1 : 0);
}

/*
 * Reset the card and the bus, and wait for both to be quiet.
 *
 * Interrupts are cut across the reset and connected again after it: a
 * reset is the one moment the chip is entitled to raise IRQ with nothing
 * behind it, and an interrupt arriving here would be answered by a
 * handler with no command to report on.
 */
static int
scsireset(ip)
    register struct scsiif *ip;
{
    (*ip->intr) (0);
    (*ip->reset) ();
    (*ip->intr) (1);
    return (scsibsy(ip) < 0 ? -1 : 0);
}

/*
 * Which cylinder a filesystem block lands on.
 *
 * A block's cylinder is its number divided by the sectors per cylinder,
 * shifted by the slice it lives in and rotated by d_roll.  The rotation
 * is what keeps block 0 - the boot sector, the label, the superblock -
 * from always landing on the disk's outermost track; the slice shift is
 * what lets a filesystem live anywhere on a drive far larger than a block
 * number can count.  Both are cylinders and both are added after the
 * division, which is why neither has to be a block number.  ide.c and
 * mw.c apply exactly the same two, and lib/fslib.c inverts the same two
 * on the host, which is what makes them agree.
 */
static UINT
scsicyl(blk, info)
    UINT blk;
    register struct info *info;
{
    static UINT cyl;

    cyl = blk / info->spc;
    cyl += info->cylstart;
    cyl += info->roll;
    if (cyl >= info->tracks)
        cyl -= info->tracks;
    return (cyl);
}

/*
 * The block's LBA.  The block is turned into the cylinder, head and
 * sector the HD-DMA would have put it at, and that triple is then
 * flattened - lib/fslib.c's ((cyl * heads + head) * spt + sec) * 512,
 * divided back out.  Doing it in that order is the entire compatibility
 * argument: handing the target the block number as an LBA would be
 * simpler and would read a different disk.
 *
 * It is also why the LUN field of the CDB is always zero and why the
 * slice's offset is a cylinder: this driver's idea of where a block is
 * belongs to the machine and not to SCSI, and SCSI is only told the
 * answer.
 */
static UINT32
scsilba(blk, info)
    UINT blk;
    register struct info *info;
{
    static UINT csec, cyl;
    static UINT32 lba;

    /*
     * A target opened with no label has no geometry and permits block 0
     * and only block 0 (see ncropen) - and block 0 is LBA 0 whatever the
     * geometry turns out to be, so the answer is known without one.  The
     * division below would trap.
     */
    if (info->spc == 0)
        return (0);
    csec = blk % info->spc;
    cyl = scsicyl(blk, info);
    lba = (UINT32)cyl * info->heads + csec / info->sectors;
    lba = lba * info->sectors + csec % info->sectors;
    return (lba);
}

/*
 * Device open.
 *
 * A device number packs two things, the way every disk driver here does
 * it: the low three bits name the target's SCSI id, and the two bits above
 * them pick an entry out of the board table - so the type has to be
 * shifted back out of minor(dev).  Eight targets and four board rows fill
 * the five bits below the slice, which sits at bit 5 and is the same
 * devslice() every other disk driver reads.
 *
 * The geometry comes off the disk, from the label mkfs writes into the
 * second half of the boot sector, and there is no table to disagree with
 * it.  Block 0 is read before any block is mapped, because the mapping is
 * what the label is needed for; sys/dlabel.c does the reading, off block
 * 0 of the drive's own 'c'.
 */
ncropen(dev, mode)
    UINT dev, mode;
{
    static struct info *info;
    static struct scsiif *ip;
    static UINT8 target, type;
    static UINT sl;
    static struct buf *b;

    target = minor(dev) & 7;
    type = (minor(dev) >> 3) & 3;
    sl = devslice(dev);
    if (type >= sizeof boards / sizeof boards[0]) {
        u.error = ENXIO;
        return;
    }
    info = &scs[target];
    /*
     * One mapping per target, not one per open: scs[] is keyed by target
     * and holds the slice's offset and roll, so a second open asking for
     * a different slice would be served the first one's mapping and would
     * read and write the wrong blocks while reporting success.  The same
     * slice, or the same minor, is the same mapping and is let through as
     * before.
     */
    if (info->flags & OPEN) {
        if (info->type != type || info->slice != sl)
            u.error = EBUSY;
        return;
    }
    info->type = type;
    info->slice = sl;
    ip = &boards[type];

    if (scsireset(ip) < 0) {
        u.error = ENXIO;
        return;
    }
    /*
     * The label, as block 0 of this target's own 'c'.  The device number
     * needs no type field built into it the way mw.c's does - this card
     * has one board and boards[] is indexed by a type that must be zero -
     * so the target's 'c' is the target's number with the slice field set
     * and nothing else moved: minor 64 + N, the scsi<N>c node dev/devlist
     * would carry beside ide0c's 64 and for the same reason.
     */
    switch (dllabel((dev & ~0377) | (DL_WHOLE << 5) | target,
                    sl, (struct dlgeom *)info)) {
    case -1:
        return;                 /* no such slice; dllabel set u.error */
    case 0:
        /*
         * No label, and nothing to fall back on: boards[] below holds
         * the adapter's protocol and not a disk's geometry - the whole
         * point of this card is that the geometry is the target's and not
         * a table's.  mw.c has specs[] for a disk laid down before labels
         * existed; a target that says nothing about itself is a target
         * this driver cannot map, and it is refused rather than guessed
         * at.  (A target can be asked directly, with INQUIRY and READ
         * CAPACITY, which this driver does not send; that is what a
         * fallback would be made of if one is ever wanted, and it is the
         * one thing that would make an unlabelled disk usable.)
         *
         * The exception is 'c', which needs no geometry to find block 0:
         * it starts at cylinder 0 and is never rolled, so its block 0 is
         * LBA 0 - where this failed to find a label and where one can be
         * written.  That is the whole of what the target permits until it
         * has been: block 0 and nothing else, which is a geometry of no
         * cylinders and a maxblk of 0.  (scsilba() answers LBA 0 for any
         * block when the geometry is empty, and this is the only case
         * that reaches it.)
         */
        if (sl != DL_WHOLE) {
            u.error = ENXIO;
            return;
        }
        info->tracks = 0;
        info->heads = 0;
        info->sectors = 0;
        info->spc = 0;
        info->roll = 0;
        info->cylstart = 0;
        info->maxblk = 0;
        info->flags |= OPEN;
        return;
    }

    if ((b = bread(1, dev)) != 0) {
        info->flags |= OPEN;
        brelse(b);
    }
}

/*
 * Device close.  There is nothing to park and nothing to tell the target:
 * a SCSI disk spins itself down on its own schedule, and a command that
 * has been finished has already left the bus free.
 */
ncrclose(dev)
    int dev;
{
    scs[dev & 7].flags &= ~OPEN;
}

/*
 * Strategy.  The caller is bwrite()/bawrite() (uio.c), which after calling
 * this either waits for the buffer to be marked done or relies on this
 * routine to release it; doing the work before returning satisfies both,
 * and is why nothing has to be queued.
 *
 * The command goes out first, and only then is the window taken and the
 * blocks moved.  That order is forced by the window's own rule - a buffer
 * must not be held across a sleep (uio.c) - and the sleep here is real:
 * the wait for the card in scsistart() blocks, and so does the wait for
 * the end in scsifinish().  Between them, scsiio() holds the window for
 * the length of the transfer and nothing sleeps, which is the same
 * bargain ide.c strikes and for the same reason.
 *
 * The card's interrupt stays connected through the transfer, which is
 * where this differs from ide.c: that driver has to take interrupts off
 * because its handler reads the status register through the same port C
 * latch the transfer is driving, and a second /RD pulse mid-word shifts
 * the sector.  A 5380's data register is a register of its own and its
 * handler never touches it, so an interrupt taken mid-byte costs nothing
 * but the interrupt.
 */
ncrstrat(b)
    register struct buf *b;
{
    static struct info *info;
    static struct scsiif *ip;
    static char *p;
    static UINT32 lba;

    info = &scs[b->dev & 7];
    ip = &boards[info->type];

    if (b->blk > info->maxblk) {
        b->flags |= BERROR;
        b->error = ENXIO;
        iodone(b);
        return;
    }

    /*
     * strat() fixes a request's count at 512 for everything it routes, and
     * that is the only shape reachable here: a buffer is one block, and
     * its bytes run contiguously through the segment the window maps.
     * swapio() is the one caller that sets its own count - 4096, a whole
     * user page in a single request - and it reaches the strategy routine
     * directly rather than through strat().  But it is only ever called
     * for the swap device, and swapdev is nodev in every volume this tree
     * builds.  Refusing the shape beats moving one block of it and calling
     * that success.
     */
    if (b->count != 512) {
        b->flags |= BERROR;
        b->error = EIO;
        iodone(b);
        return;
    }

    lba = scsilba(b->blk, info);

    if (scsistart(ip, b->dev & 7, b->flags & BREAD, lba, 1) < 0) {
        b->flags |= BERROR;
        b->error = ip->error ? ip->error : EIO;
        iodone(b);
        return;
    }

    /*
     * The buffer's segment goes into the BUFSEG window for the length of
     * the block.  Nothing between the hold and the release sleeps, and
     * nothing reaches a buffer header or maps the window for its own use
     * - the two rules the window's discipline is made of (uio.c).
     */
    p = bhold(b);
    if (scsiio(ip, p, 512, !(b->flags & BREAD)) < 0)
        ip->error = EIO;
    brel();

    if (scsifinish(ip, b->dev & 7) < 0)
        b->flags |= BERROR;

    if (ip->error)
        b->error = ip->error;

    iodone(b);
}

/*
 * A command block from above (include/sys/ioctl.h).
 *
 * The block is a SCSI CDB, and its length is the caller's: the ten bytes
 * this driver builds for itself are one shape of CDB among several, and
 * the six-byte commands - INQUIRY, REQUEST SENSE, MODE SENSE, TEST UNIT
 * READY - are the ones a driver without a sense path has the most use
 * for.  So the bytes go to the target straight from the caller's
 * structure and never through ip->cdb, which stays the read/write path's
 * own.  scsiio() moves them, which is to say one byte per REQ/ACK
 * handshake, the same handshake the data phase uses; a CDB is not a
 * special case of the bus, only a phase of it.
 *
 * Everything else is scsistart() and scsifinish() with the middle left to
 * the caller: the card is taken before anything is put on the bus, the
 * data phase - when the block has one - is the buffer bioctl() passed in,
 * and the hold is taken after the select and released before the finish,
 * because the window's rule is that nothing sleeps with it held (uio.c).
 *
 * A block with no data phase is the ordinary case here rather than the
 * exception, and it needs nothing special: a target that has nothing to
 * transfer goes straight to the status phase, and scsifinish() is already
 * waiting on the bus for it rather than on the card's interrupt - so a
 * TEST UNIT READY costs a select, six bytes and two.
 *
 * The sense data a CHECK CONDITION leaves behind is still not read.
 * scsifinish() reports the command as failed and that is all; a caller
 * that wants the reason can have it by sending REQUEST SENSE itself, as
 * the six-byte block this call exists to make possible.
 */
int
ncrioctl(dev, cmd, r, b)
    UINT dev, cmd;
    register struct cdb *r;
    register struct buf *b;
{
    static struct info *info;
    static struct scsiif *ip;
    static char *p;

    info = &scs[dev & 7];
    ip = &boards[info->type];

    for (;;) {
        di();
        if (!ip->busy)
            break;
        sleep(ip, PRIBIO);
    }
    ip->busy = 1;
    ip->done = 0;
    ip->error = 0;
    ei();

    if (scsiselect(ip, dev & 7) < 0 || scsiio(ip, r->cmd, r->len, 1) < 0) {
        ip->busy = 0;
        wakeup(ip);
        u.error = ip->error ? ip->error : EIO;
        return (-1);
    }

    if (r->count) {
        p = bhold(b);
        if (scsiio(ip, p, r->count, !(r->flags & CDB_IN)) < 0)
            ip->error = EIO;
        brel();
    }

    /*
     * The status and the message are read whatever the data phase did.
     * A target left holding the bus mid-command is worse than a failed
     * request - it takes the next command's select with it - so the
     * finish is not conditional on the transfer having gone well, which
     * is ncrstrat()'s order too.
     */
    if (scsifinish(ip, dev & 7) < 0) {
        u.error = ip->error ? ip->error : EIO;
        return (-1);
    }
    return (0);
}

/*
 * The entries the driver's header hands the kernel (sys/ncrhdr.c).  It is
 * a different object because a module's first bytes have to be the
 * header's, and an object's data is placed in the order the objects are
 * named - so the header is alone in the object named first, and this is
 * the driver's own.
 */
struct biovec ncrbvec = { &ncropen, &ncrclose, &ncrstrat, &ncrioctl };

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
