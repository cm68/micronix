/*
 * Micronix driver for an IDE hard disk behind the S100Computers MYIDE
 * board.  The cpu moves every byte of a transfer itself - programmed I/O,
 * PIO, with no DMA controller - and the drive raises an interrupt when it
 * has found the sector, so the driver sleeps through the seek and moves
 * the data once it wakes.
 *
 * sys/ide.c
 * Changed: <2026-09-24 curt>
 *
 * Terms used throughout, in the order they first matter:
 *
 *   IDE, ATA    integrated drive electronics, attached (formerly AT)
 *               attachment: the interface and the command set this drive
 *               speaks.  A drive is a disk with a controller built into
 *               it, and the controller is driven by writing its registers.
 *   task file   that set of registers: data, error (or features, when
 *               written), sector count, three holding an LBA, drive/head,
 *               and status (or command, when written).  "File" because
 *               each register is addressed like a word of memory.  This
 *               driver knows them by number, 0 through 7.
 *   LBA         logical block address: the whole disk as one run of
 *               512-byte sectors numbered from 0.  This is how the drive
 *               is addressed here, and the only geometry it is told.
 *   CHS         cylinder, head, sector: the older way of naming a sector
 *               by where it sits on the physical disk.  The filesystem
 *               still lays a volume out this way, so idelba() below
 *               converts, and that conversion is what makes a volume
 *               written for the HD-DMA readable here.
 *   8255        the programmable peripheral interface (PPI) chip on the
 *               board: three 8-bit ports called A, B and C, whose
 *               direction is set by writing a mode word to a fourth
 *               address.  Ports A and B are wired to the drive's 16 data
 *               lines, port C to its address and control pins.
 *   /CS0, /CS1  chip selects: which half of the drive's registers the
 *               board's decode is pointing at.  /CS0 reaches the task
 *               file; /CS1 reaches the two registers ATA sets apart from
 *               it, the device control register when written and the
 *               alternate status register when read.  /RD, /WR and /RST
 *               are the read, write and reset strobes.  A leading / marks
 *               a signal asserted by pulling it low, which is why the
 *               board inverts most of port C on its way to the drive.
 *   INTRQ       the drive's interrupt request.  It goes up when the drive
 *               has finished looking for a sector and wants to hand one
 *               over, and comes down again when the status register is
 *               read - so that read is both the test for which drive is
 *               asking and the acknowledgement that stops it asking.
 *   nIEN        the drive's interrupt enable: bit 1 of the device control
 *               register, active low, so clearing it leaves INTRQ
 *               connected to the drive's interrupt pin.
 *   unit        one drive of the two a board can carry.  Which one is
 *               meant is a latch on the board, not a task file register.
 *   VI, PIC     vectored interrupt and programmable interrupt controller:
 *               the S-100 bus's eight interrupt lines, and the 8259 on the
 *               Mult I/O board which ranks them and hands the cpu a vector
 *               when it acknowledges one.  This card drives VI2, which
 *               nothing else on this machine uses; see IDEINT below.
 *   strategy    the entry point the buffer cache calls to start a
 *               transfer (uio.c).  mw.c's is the model for this one.
 *   window      the single page of address space that bhold() maps a
 *               buffer's segment into for the length of an access, so
 *               that the kernel can reach a buffer in another segment.
 *
 * The board puts the 8255 between the bus and the drive's pins, and the
 * register-level detail below follows its sample source in extra/docs
 * (MYIDE.pdf, and the MYIDE.ASM beside it).  The task file is not mapped
 * into the bus at all: port C carries the register address and the
 * strobes, port A carries the byte, and port B is the high half of a
 * 16-bit transfer.
 *
 * The shape of the driver is sys/mw.c's, because the filesystem is the
 * same one.  Open reads the disk label mkfs writes into the boot sector,
 * the block mapping is the same one (a cylinder is blk/spc shifted by the
 * slice the minor names and rotated by d_roll, wrapped at tracks)
 * flattened the way lib/fslib.c flattens it, and the strategy routine is
 * the same entry point with the same contract.  A volume laid out for the
 * HD-DMA therefore reads back block for block here.
 *
 * What differs is the transport.  The data moves through the data
 * register a word at a time and the driver reaches buffer memory itself,
 * through the window bhold() opens, instead of handing the card a DMA
 * address.  So the transfer itself holds the cpu, and it has to run in the
 * caller's context rather than in the handler.  The interrupt buys back
 * the rest of the command, which is nearly all of it: a seek is ten to
 * thirty milliseconds and the rotation adds eight more on average, against
 * about a millisecond to move the sector.  A driver that spins through the
 * seek is idle for most of the request, and on a single-user machine that
 * is time nothing else can use either.
 *
 * That is why the request splits in two.  idestart() issues the command
 * and sleeps until the drive interrupts; idefinish() takes the window and
 * moves the sector.  The sleep is what forces the split rather than merely
 * suggesting it - sleep() panics if a buffer is held (see the window's
 * discipline in uio.c) - so the hold cannot be taken until after the drive
 * has answered, and idestrat()'s bhold() sits at the end of the request
 * instead of the top.
 *
 * The transfer runs with interrupts off, and must: the handler reads the
 * status register through the same port C latch the transfer is driving,
 * so an interrupt taken mid-word would pulse /RD twice and the sector
 * would come back shifted by one word.  That costs almost nothing, because
 * the cpu is doing programmed I/O for the whole of the transfer and has no
 * attention to spare anyway; and no other card is held off for longer than
 * the transfer, because the seek - not the transfer - is what used to
 * block.  mw.c takes the same kind of care for its own reason, stopping
 * the cpu while the HD-DMA has the bus.
 *
 * What the interrupt does not remove is serialisation.  There is still one
 * board, and one drive-select latch shared by its two drives, so a second
 * request waits: idestart() holds the board with a busy flag and sleeps on
 * it, waking the next waiter in idefinish().  mw.c's bsort() sorts a queue
 * to shorten seeks, and is not copied here - with one request in flight
 * there is no queue to sort.  What the two do share is that requests are
 * served in the order they arrive.
 *
 * A drive that never raises INTRQ is not waited out.  idereset() clears
 * nIEN, so a conforming drive interrupts; one that does not is wired or
 * strapped wrong, and the request waits there instead of the driver
 * spending a bounded time pretending otherwise.  mw.c's watchdog covers
 * the neighbouring case, a controller that stops answering part way
 * through a command, and is the piece to copy if this ever needs one.
 *
 * Register access is behind struct ideif: the protocol below never names
 * a port.  It speaks task-file register numbers and hands whole sectors
 * to xfer(), and the board turns each call into bus cycles.  A card that
 * maps the task file into the bus instead of strobing it is another row
 * in boards[], not another driver.
 */

#include <types.h>
#include <sys/sys.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <sys/con.h>
#include <sys/dlabel.h>
#include <errno.h>

/*
 * Declared before anything calls them: an undeclared call is extern int
 * by default and the definitions say static.
 */
static int idebsy(), idewait(), idestart(), idefinish();
static int idereset();

#define NDRIVES 2               /* the board's two drives */

/*
 * The interrupt line the card drives.  The S-100 bus carries eight
 * vectored interrupt lines, VI0 to VI7, and a card asserts one of them;
 * which of those reach the Mult I/O board's 8259 is the board's jumper
 * area's business, and it brings in three - VI0, VI1 and VI2.  The other
 * five inputs are wired on the board itself to its own devices (three
 * serial ports, the parallel printer and the clock), so those five are
 * not bus lines at all and the three are the whole of what a card on the
 * bus can claim.  Two of the three are spoken for, and by what the
 * machine shipped with: the Decision 1 leaves the jumper area with VI0 on
 * the hard disk controller and VI1 on the floppy controller, which is the
 * jumpering Micronix is written for - mw.c claims VI0 as it comes up with
 * inton(MWINT) and cus.c claims VI1 with inton(DJINT), both of which are
 * numbers 0 and 1 because that is where the jumpers put them.
 *
 * VI2 is the one left, and it is left in the literal sense: no card on
 * the bus drives it, so nothing is behind the line.
 *
 * Past the jumpers the line is simply input 2 of the 8259, and the
 * kernel's numbering means 8259 inputs from here on.  ARMMASTER in
 * inits.s masks inputs 0, 1 and 6 and allows 2 through 7, so this one is
 * already unmasked and needs no inton() call to claim it.  sys/intrpt.s
 * routes input 2 to ideint() below.
 */
#define IDEINT  2

/*
 * Task file register numbers.  These are ATA's names and not ports: the
 * board turns one into whatever its bus needs.
 */
#define DATA    0               /* data register */
#define ERRST   1               /* error (read) / features (write) */
#define SCOUNT  2               /* sector count */
#define LBA0    3               /* sector number / LBA 0-7 */
#define LBA1    4               /* cylinder low  / LBA 8-15 */
#define LBA2    5               /* cylinder high / LBA 16-23 */
#define DRVHD   6               /* drive/head    / LBA 24-27 */
#define STAT    7               /* status (read) / command (write) */

/*
 * Status register bits
 */
#define SBSY    0x80            /* busy */
#define SDRDY   0x40            /* drive ready */
#define SDRQ    0x08            /* data request */
#define SERR    0x01            /* error */

/*
 * Commands.  Both transfer one sector and retry internally, which is the
 * whole of what this driver needs: the geometry comes off the label, so
 * there is no initialize-drive-parameters to issue, and the drive is
 * addressed by LBA throughout, so there is no CHS translation to set up.
 */
#define CREAD   0x20            /* read sector(s), with retry */
#define CWRITE  0x30            /* write sector(s), with retry */

/*
 * What goes in the drive/head register: LBA addressing, head 0.  The
 * register also carries a bit for choosing between the two drives one
 * cable can hold - ATA calls them master and slave - but this board does
 * not use it: it switches the drives with its own latch, so the bit stays
 * clear here and mysel() below does the choosing.  (The sample's older
 * single-drive listing writes 0xd0 here; the two-drive revision corrected
 * it to 0xe0, which is what ATA specifies.)
 */
#define DLBA    0xe0

/*
 * Status polls before a command is called failed.  A loop count, not a
 * calibrated interval: the drive has no timeout of its own, and mw.c's
 * watchdog is software too.  It is sized for a drive that has been spun
 * down coming back up, which is seconds, and deliberately not longer,
 * because kernel code is not preempted (see the header) - a drive that
 * never answers holds the machine for the whole of this bound instead of
 * failing one request.
 */
#define SPIN    200000L

/*
 * The board's ports.  A port is a Z80 I/O address: a number space of its
 * own, separate from memory, reached with in() and out() rather than by a
 * load or a store (inout.s).  The 8255 answers at 0x30-0x33, and 0x34 is
 * a latch that picks which of the board's two drives the strobes go to -
 * bit 0 clear selects the first, set selects the second.
 *
 * The range is free on this machine: 0x48-0x4f and 0x58-0x5f are the
 * Mult I/O console, 0x50-0x53 the obsolete HDCA, 0x54-0x55 the HD-DMA,
 * 0xd0-0xd1 the simulator's own ports, and 0xef the DJ-DMA.
 */
#define PA      0x30            /* 8255 port A: drive data, low byte */
#define PB      0x31            /* 8255 port B: drive data, high byte */
#define PC      0x32            /* 8255 port C: register address, strobes */
#define PCTL    0x33            /* 8255 mode word */
#define PDRV    0x34            /* drive select latch */

/*
 * 8255 mode words.  Port C is an output either way; what changes is
 * whether ports A and B are inputs (reading a register or a sector) or
 * outputs (writing one).
 */
#define PPI_RD  0x92            /* A and B in, C out */
#define PPI_WR  0x80            /* A, B and C all out */

/*
 * Port C bits.  A0-A2 reach the drive's address pins directly; the rest
 * go through inverting buffers, so a set bit here asserts the line.
 */
#define C_A0    0x01
#define C_A1    0x02
#define C_A2    0x04
#define C_CS0   0x08
#define C_CS1   0x10
#define C_WR    0x20
#define C_RD    0x40
#define C_RST   0x80

/*
 * A register is reached by putting its number on the drive's A0-A2 with
 * /CS0 asserted, so the number is the whole of the address.  Status and
 * command are one address on the drive's pins and the direction picks
 * which is meant, which is why nothing here has to know which of the two
 * it is being asked for.
 */
#define REGSEL(reg) (C_CS0 | (reg))

/*
 * The pair of registers /CS1 reaches instead.  ATA keeps these two off to
 * one side of the task file because neither is part of a command: the
 * device control register holds the drive's interrupt enable, and the
 * alternate status register is the same byte as the status register but
 * reading it leaves INTRQ alone.  A controller that acknowledges its
 * interrupt by reading status therefore has an alternate status register
 * to poll without swallowing the interrupt - this driver has no use for
 * it, because it wants the interrupt taken down, but the number is here
 * because the address would otherwise look like a mistake.
 */
#define DCTL    6               /* device control, through /CS1 */
#define NIEN    0x02            /* ...and its interrupt enable, low = on */
#define ASTAT   7               /* alternate status, through /CS1 */

#define DREGSEL(reg) (C_CS1 | (reg))

/*
 * An interface: where the task file is and how a byte reaches it.  The
 * six calls are every way this driver touches the bus.  The members below
 * them are the board's own state, which lives here because there is one
 * board and one command in flight on it at a time.
 */
struct ideif
{
    int (*rd)();                /* rd(reg) -> byte */
    int (*wr)();                /* wr(reg, byte) */
    int (*xfer)();              /* xfer(p, nbytes, read) */
    int (*reset)();             /* reset() */
    int (*sel)();               /* sel(unit) */
    int (*intr)();              /* intr(on): connect or cut the drive's INTRQ */

    int busy;                   /* a command is out and not yet finished */
    int done;                   /* ... and the drive has interrupted for it */
    int status;                 /* what the handler's status read found */
};

/*
 * One register, read: the address onto port C, pull /RD, take the byte
 * off port A, let both go again.  This runs with the 8255 in read mode,
 * which myreset() establishes and mywr()/myxfer() put back.
 */
static int
myrd(reg)
    UINT8 reg;
{
    static UINT sel, val;

    sel = REGSEL(reg);
    out(PC, sel);
    out(PC, sel | C_RD);
    val = in(PA);
    out(PC, sel);
    out(PC, 0);
    return (val);
}

/*
 * One register, written.  The byte has to come out of port A, so the 8255
 * is turned round for the length of the write and turned back after.  It
 * is that round trip, four extra port writes per byte, that keeps the
 * sector data path below out of here.
 */
static int
mywr(reg, val)
    UINT8 reg;
    UINT8 val;
{
    static UINT sel;

    sel = REGSEL(reg);
    out(PCTL, PPI_WR);
    out(PA, val);
    out(PC, sel);
    out(PC, sel | C_WR);
    out(PC, sel);
    out(PC, 0);
    out(PCTL, PPI_RD);
    return (0);
}

/*
 * A sector, one 16-bit word at a time: port A is the low byte and port B
 * the high byte, both taken inside a single /RD.  The word is the
 * transfer unit and not an optimisation - two reads of port A under two
 * /RD pulses would advance the drive a word each and hand back the low
 * half twice.
 */
static int
myxfer(p, n, read)
    register char *p;
    UINT n;
    int read;
{
    static UINT sel, i;

    sel = REGSEL(DATA);

    if (read) {
        for (i = 0; i < n; i += 2) {
            out(PC, sel);
            out(PC, sel | C_RD);
            p[0] = in(PA);
            p[1] = in(PB);
            out(PC, sel);
            p += 2;
        }
        return (0);
    }

    /*
     * A write needs A and B to be outputs, so the mode is set for the
     * whole sector: it cannot be flipped per word.
     */
    out(PCTL, PPI_WR);
    for (i = 0; i < n; i += 2) {
        out(PA, p[0] & 0xff);
        out(PB, p[1] & 0xff);
        out(PC, sel);
        out(PC, sel | C_WR);
        out(PC, sel);
        p += 2;
    }
    out(PCTL, PPI_RD);
    return (0);
}

/*
 * Hard reset, on the board's own reset line: port C bit 7 into the
 * drive's /RESET pin, which is what the sample drives.  The pulse is held
 * for a few microseconds, and the read in the loop is the delay - it goes
 * out to the bus, so the compiler cannot drop the loop, and it is
 * harmless because nothing is selected while the drive is being held in
 * reset.
 *
 * The 8255's mode is set here rather than in open because this is the
 * first routine to touch the card, and the read mode it leaves behind is
 * what every read below assumes.
 */
static int
myreset()
{
    static UINT n;

    out(PCTL, PPI_RD);
    out(PC, C_RST);
    for (n = 0; n < 0x40; n++)
        in(PA);
    out(PC, 0);
    return (0);
}

/*
 * Point the board's strobes at one of its two drives.  The latch is
 * shared by both drives, so it is written per transfer: the drive that
 * was selected when the last transfer finished has nothing to do with
 * which one this transfer wants.
 */
static int
mysel(unit)
    UINT8 unit;
{
    out(PDRV, unit & 1);
    return (0);
}

/*
 * Connect or cut the drive's interrupt line: the device control register,
 * which /CS1 reaches and the task file does not.  Bit 1 of it is nIEN,
 * active low, so a zero in it leaves INTRQ wired to the drive's interrupt
 * pin.  The drive comes out of reset that way - ATA's power-on default is
 * interrupts enabled, which is why the board's own sample, a polled
 * driver, never writes this register at all - but the driver states what
 * it needs rather than trusting a default it cannot see.
 *
 * The board does route /CS1 (extra/docs/MYIDE-.ASM names it, as
 * IDEcs1line), so this reaches the drive.  The byte on port A is the whole
 * of what goes: the other bits of the register are the software reset,
 * which this driver does not use - the board's own /RST line does that -
 * and two interrupt-mode bits for a kind of interrupt the drive never
 * raises.
 */
static int
myintr(on)
    int on;
{
    static UINT sel;

    sel = DREGSEL(DCTL);
    out(PCTL, PPI_WR);
    out(PA, on ? 0 : NIEN);
    out(PC, sel);
    out(PC, sel | C_WR);
    out(PC, sel);
    out(PC, 0);
    out(PCTL, PPI_RD);
    return (0);
}

/*
 * Boards, indexed by devtype(dev) the way mw.c indexes specs[]: the field
 * picks one entry out of a table of the controller's own options, and
 * what those options are is the controller's business.  mw.c's three are
 * drive sizes; this driver's one is the single board, so the field must
 * be zero - which is what the bound check in ideopen() is for.  A second
 * MYIDE board would add an entry here, and a second bit of drive number
 * below it.
 */
static struct ideif boards[] = {
    {&myrd, &mywr, &myxfer, &myreset, &mysel, &myintr, 0, 0, 0},
};

/*
 * Drive configuration.  Unlike mw.c's info there is nothing in here that
 * the label does not carry.  mw.c keeps a table of per-model numbers,
 * because an HD-DMA controller has to be told all of it: how long to
 * pause between sectors while the heads settle (step delay), the track
 * from which on the write current is reduced, and the track from which on
 * the write signal is shaped to compensate for the way a disk's inner
 * tracks are packed more tightly (precompensation).  A drive with its own
 * controller is told none of it - it takes an LBA and finds the sector
 * itself - so that table, and the warning mw.c raises when the table
 * disagrees with the label, have nothing to do here.
 *
 * The first seven fields are struct dlgeom (sys/dlabel.h), in its order
 * and of its types, because sys/dlabel.c decodes the label through a
 * view of them - ideopen passes this struct as one.  The bookkeeping
 * below them is this driver's own; moving any of the seven breaks the
 * decode in a way the compiler will not report.
 */
struct info
{
    UINT tracks;                /* cylinders */
    UINT8 heads;
    UINT8 sectors;              /* sectors per track */

    UINT maxblk;                /* max legal block number in the slice */
    UINT spc;                   /* sectors per cylinder */
    UINT roll;                  /* what idecyl adds to blk / spc */
    UINT cylstart;              /* first cylinder of the slice being read */
    UINT8 flags;                /* see below */
    UINT8 type;                 /* index into boards[] */
    UINT8 slice;                /* the slice this open is bound to */
} ides[NDRIVES] = 0;

#define OPEN    2               /* drive is open */

/*
 * Wait for the drive to stop being busy - SBSY clear in the status
 * register - and return that status, or -1 if it stayed busy past the
 * spin bound and the driver is giving up on it.
 */
static int
idebsy(ip)
    register struct ideif *ip;
{
    static UINT32 n;
    static int st;

    for (n = 0; n < SPIN; n++) {
        st = (*ip->rd) (STAT);
        if (!(st & SBSY))
            return (st);
    }
    return (-1);
}

/*
 * Load the task file and issue a command for one sector at lba.  The
 * drive is in LBA mode, so the geometry never reaches it - it is only
 * used above this to work out which LBA a block is.
 */
static int
idecmd(ip, cmd, lba)
    register struct ideif *ip;
    UINT8 cmd;
    UINT32 lba;
{
    static int st;

    if ((st = idebsy(ip)) < 0)
        return (-1);
    if (!(st & SDRDY) || (st & SERR))
        return (-1);

    (*ip->wr) (DRVHD, DLBA | ((lba >> 24) & 0x0f));
    (*ip->wr) (ERRST, 0);
    (*ip->wr) (SCOUNT, 1);
    (*ip->wr) (LBA0, (UINT8)lba);
    (*ip->wr) (LBA1, (UINT8)(lba >> 8));
    (*ip->wr) (LBA2, (UINT8)(lba >> 16));
    (*ip->wr) (STAT, cmd);
    return (0);
}

/*
 * The interrupt handler.  sys/intrpt.s routes VI2 here, and intrupt
 * (mio.s) has already saved the machine state, taken interrupts off for
 * the duration and will write the end-of-interrupt to the 8259 when this
 * returns - so there is nothing to save here and nothing to acknowledge
 * beyond the drive itself.
 *
 * The 8259 is level triggered, so the line has to be down before that
 * end-of-interrupt is written or it fires again the moment it lands.
 * Reading the status register is what takes INTRQ down, so that read is
 * done first, and it is the test as well: the status says whether the
 * drive is holding a sector for us or has nothing to say.
 *
 * Having nothing to say is the ordinary case for the second interrupt of a
 * write.  The drive raises INTRQ when it is handed the command and again
 * when the sector is in, and the second one arrives either during the
 * transfer, which runs with interrupts off, or just after it, by which
 * time the request is nearly done and idefinish()'s poll is what finds it.
 * Either way nobody is sleeping on that one, so it must not wake anybody:
 * `done` going up with no waiter would leave the flag set for the next
 * request to find, and that request would return before its drive had even
 * been given the command.  So the flag is only set when a command really
 * is in flight and the status really does report the drive's answer.
 */
ideint()
{
    register struct ideif *ip;

    ip = &boards[0];

    /* the acknowledgement, and the report of what the drive wants */
    ip->status = (*ip->rd) (STAT);

    if (!ip->busy || !(ip->status & (SDRQ | SERR)))
        return;

    ip->done = 1;
    wakeup(&ip->done);
}

/*
 * Wait for the handler to say the drive has answered.  The shape is the
 * kernel's canonical one - uio.c's block() and bwait() are the same three
 * lines - and it is not decoration: the test and the sleep are inside one
 * di(), so a drive that answers between the two cannot be missed, and
 * sleep() returns with interrupts enabled, which is why the di() is redone
 * on every pass rather than held across the loop.
 *
 * The event is the address of the flag itself.  That is the smallest thing
 * that will do, and it is distinct from the board address, so a process
 * waiting here is not woken by the next process waiting for the board.
 */
static int
idewait(ip)
    register struct ideif *ip;
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
 * Start one sector at lba, and sleep until the drive answers for it.
 *
 * One request per board at a time.  The cpu's I/O is not the reason - the
 * driver could interleave two requests' register writes without the card
 * noticing - the drive-select latch is: it is shared by the board's two
 * drives, and a second request that reached the card before the first had
 * been served would be talking to whichever drive the latch happened to
 * name last.  So requests queue behind the busy flag and wait on the
 * board's own address, which is what idefinish() wakes.
 *
 * On success the drive is holding a sector and interrupts are on, and
 * idefinish() is the other half of the pair.  On failure the board is
 * already free and there is nothing to finish.
 */
static int
idestart(ip, unit, lba, read)
    register struct ideif *ip;
    UINT8 unit;
    UINT32 lba;
    int read;
{
    for (;;) {
        di();
        if (!ip->busy)
            break;
        sleep(ip, PRIBIO);
    }
    ip->busy = 1;
    ip->done = 0;
    ei();

    (*ip->sel) (unit);
    if (idecmd(ip, read ? CREAD : CWRITE, lba) < 0) {
        ip->busy = 0;
        wakeup(ip);
        return (-1);
    }

    idewait(ip);

    /*
     * What the drive decided is only visible in the status the handler
     * read, and a drive that took the command says so by asking for the
     * sector.  One that would not take it answers with the error bit
     * instead, or with neither.
     */
    if (!(ip->status & SDRQ) || (ip->status & SERR)) {
        ip->busy = 0;
        wakeup(ip);
        return (-1);
    }
    return (0);
}

/*
 * Move the sector the drive is holding, and let the board go.
 *
 * p is where it goes.  The interrupt state is this routine's business and
 * it is not an optimisation: the transfer strobes the same port C latch
 * that the handler reads the status register through, so an interrupt
 * taken between the two port writes of a word would pulse /RD a second
 * time and the drive would hand over the next word for this one's high
 * half - every sector after the first would come back shifted.  Nothing is
 * lost by keeping them off, because the cpu is doing programmed I/O for
 * the whole of the transfer and has nothing else to do with the time; the
 * poll that follows is inside the same window because those status reads
 * are what take the drive's end-of-write interrupt down.
 *
 * Returns 0, or -1 if the drive reported an error or stopped answering.
 */
static int
idefinish(ip, p, read)
    register struct ideif *ip;
    register char *p;
    int read;
{
    static int st;

    di();
    (*ip->xfer) (p, 512, read);
    st = idebsy(ip);
    ip->busy = 0;
    wakeup(ip);
    ei();

    if (st < 0 || (st & SERR))
        return (-1);
    return (0);
}

/*
 * Reset the drive and wait for it to report itself ready.
 *
 * This is the stand-in for what mwopen() gets for free from the HD-DMA's
 * strategy path: mwstart() resets that controller and loads its constants
 * on the first request after an open, from a state the controller keeps
 * in its own registers.  A drive with its own controller holds its own
 * state, and reset is all that is left to ask of it - it is what makes a
 * drive left mid-command usable again, and so what lets open trust the
 * label read that follows.
 */
static int
idereset(ip, unit)
    register struct ideif *ip;
    UINT8 unit;
{
    static int st;
    static UINT32 n;

    (*ip->sel) (unit);

    /*
     * The drive's interrupts are cut across the reset and connected again
     * after it.  A reset is the one moment the drive is entitled to raise
     * INTRQ with nothing behind it, and an interrupt arriving here would
     * be answered by a handler with no command to report on.
     */
    (*ip->intr) (0);
    (*ip->reset) ();
    (*ip->intr) (1);

    if ((st = idebsy(ip)) < 0)   /* BSY rises on the reset and falls after */
        return (-1);
    for (n = 0; n < SPIN; n++) {
        st = (*ip->rd) (STAT);
        if (st & (SDRDY | SERR))
            break;
    }
    return ((st & SDRDY) ? 0 : -1);
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
 * division, which is why neither has to be a block number.  A disk laid
 * out as one whole-disk slice carries d_roll tracks >> 1 the way every
 * disk made before slices does, and mw.c applies exactly the same two;
 * lib/fslib.c inverts the same two on the host, which is what makes the
 * three agree.
 */
static UINT
idecyl(blk, info)
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
 * argument: handing the drive the block number as an LBA would be
 * simpler and would read a different disk.
 */
static UINT32
idelba(blk, info)
    UINT blk;
    register struct info *info;
{
    static UINT csec, cyl;
    static UINT32 lba;

    /*
     * A drive opened with no label has no geometry and permits block 0
     * and only block 0 (see ideopen) - and block 0 is LBA 0 whatever the
     * geometry turns out to be, so the answer is known without one.  The
     * division below would trap.
     */
    if (info->spc == 0)
        return (0);
    csec = blk % info->spc;
    cyl = idecyl(blk, info);
    lba = (UINT32)cyl * info->heads + csec / info->sectors;
    lba = lba * info->sectors + csec % info->sectors;
    return (lba);
}

/*
 * Device open.
 *
 * A device number packs two things, the way mw.c's does: the low bit
 * names which drive, and the rest pick an entry out of the board table,
 * which is why the type has to be shifted back out of minor(dev) below.
 * Drives 0 and 1 are this board's two, and the unit each one is comes out
 * of the same low bit - so the board's latch and the device number agree.
 *
 * The geometry comes off the disk, from the label mkfs writes into the
 * second half of the boot sector, and there is no table to disagree with
 * it.  LBA 0 is read before any block is mapped, because the mapping is
 * what the label is needed for; sys/dlabel.c does the reading, off block
 * 0 of the drive's own 'c'.
 */
ideopen(dev, mode)
    UINT dev, mode;
{
    static struct info *info;
    static struct ideif *ip;
    static UINT8 drive, type;
    static UINT sl;
    static struct buf *b;

    drive = dev & 1;
    type = devtype(dev);
    sl = devslice(dev);
    if (type >= sizeof boards / sizeof boards[0]) {
        u.error = ENXIO;
        return;
    }
    info = &ides[drive];
    /*
     * One mapping per drive, not one per open: ides[] is keyed by drive
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

    if (idereset(ip, drive) < 0) {
        u.error = ENXIO;
        return;
    }
    /*
     * The label, as block 0 of this drive's own 'c'.  The device number
     * needs no type field built into it the way mw.c's does - this card
     * has one board and boards[] is indexed by a type that must be zero -
     * so the drive's 'c' is the drive's number with the slice field set
     * and nothing else moved, which is the ide<N>c minor devlist holds.
     */
    switch (dllabel((dev & ~0377) | (DL_WHOLE << 5) | drive,
                    sl, (struct dlgeom *)info)) {
    case -1:
        return;                 /* no such slice; dllabel set u.error */
    case 0:
        /*
         * No label, and nothing to fall back on: boards[] below holds
         * the controller's protocol and not a drive's geometry - the
         * whole point of this card is that the geometry is the drive's
         * and not a table's.  mw.c has specs[] for a disk laid down
         * before labels existed; a drive that says nothing about itself
         * is a drive this driver cannot map, and it is refused rather
         * than guessed at.  (The drive can be asked directly, with ATA's
         * IDENTIFY DEVICE command, which this driver does not send; that
         * is what a fallback would be made of if one is ever wanted.)
         *
         * The exception is 'c', which needs no geometry to find block 0:
         * it starts at cylinder 0 and is never rolled, so its block 0 is
         * LBA 0 - where this failed to find a label and where one can be
         * written.  That is the whole of what the drive permits until it
         * has been: block 0 and nothing else, which is a geometry of no
         * cylinders and a maxblk of 0.  (idelba() answers LBA 0 for any
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
 * Device close.  mwclose parks the heads - moves them off the data area
 * onto a track they can safely sit on, which is what a disk wants before
 * the machine is moved.  A drive with its own controller does that for
 * itself, and is already idle once the request that opened it has
 * finished, so there is nothing to tell it and no geometry needed to
 * tell it with.
 */
ideclose(dev)
    int dev;
{
    ides[dev & 1].flags &= ~OPEN;
}

/*
 * Strategy.  The caller is bwrite()/bawrite() (uio.c), which after calling
 * this either waits for the buffer to be marked done or relies on this
 * routine to release it; doing the work before returning satisfies both,
 * and is why nothing has to be queued.
 *
 * The command goes out and is slept through, and only then is the window
 * taken and the sector moved.  That order is forced by the window's own
 * rule - a buffer must not be held across a sleep (uio.c) - and it happens
 * to be the right one anyway: the hold is as short as it can be, and no
 * other process is kept off the window while this one waits for a disk.
 */
idestrat(b)
    register struct buf *b;
{
    static struct info *info;
    static struct ideif *ip;
    static char *p;
    static UINT32 lba;

    info = &ides[b->dev & 1];
    ip = &boards[info->type];

    if (b->blk > info->maxblk) {
        b->flags |= BERROR;
        b->error = ENXIO;
        iodone(b);
        return;
    }

    /*
     * strat() fixes a request's count at 512 for everything it routes,
     * and that is the only shape reachable here: a buffer is one sector,
     * and its bytes run contiguously through the segment the window maps.
     * swapio() is the one caller that sets its own count - 4096, a whole
     * user page in a single request - and it reaches the strategy routine
     * directly rather than through strat().  But it is only ever called
     * for the swap device, and swapdev is nodev - no device at all - in
     * every volume this tree builds.  Refusing the shape beats moving one
     * sector of it and calling that success.
     */
    if (b->count != 512) {
        b->flags |= BERROR;
        b->error = EIO;
        iodone(b);
        return;
    }

    /*
     * b->blk is a filesystem block and stays one: the slice's cylinder
     * offset is added in idecyl, after the division that turns a block
     * into a cylinder, so nothing here has to hold a device block number -
     * which is what a large drive could not be counted in.
     */
    lba = idelba(b->blk, info);

    if (idestart(ip, b->dev & 1, lba, b->flags & BREAD) < 0) {
        b->flags |= BERROR;
        iodone(b);
        return;
    }

    /*
     * The buffer's segment goes into the BUFSEG window for the length of
     * the transfer.  Nothing between the hold and the release sleeps, and
     * nothing reaches a buffer header or maps the window for its own use
     * - the two rules the window's discipline is made of (uio.c).
     */
    p = bhold(b);
    if (idefinish(ip, p, b->flags & BREAD) < 0)
        b->flags |= BERROR;
    brel();

    iodone(b);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
