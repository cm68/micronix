/*
 * the djdma is an intelligent controller with an onboard z80 that
 * processes requests placed into s100 memory when tickled by an
 * out to DJDMA_PORT.
 * although it is possible to do this with a child process and get
 * real concurrency, this is way overkill.  let's just do this with
 * everything being done synchronously.
 *
 * this will be complicated in that a channel command may set an interrupt
 * line.  this just changes the sense of what the output to DJDMA_PORT
 * does to an intack, which causes the next channel command in series
 * to be fetched.
 * so, all work in this driver happens in response to, and in series with, 
 * an output instruction, which may be in an interrupt handler
 */

#include "sim.h"
#include "z80glue.h"
#include "hwsim.h"
#include "vimap.h"
#include "imd.h"
#include "util.h"
#include "disz80.h"
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#ifndef NODEBUG
int trace_djdma;
extern int trace_bio;
#endif

extern char *printable(char);       // multio.c

extern int terminal_fd_in;
extern int terminal_fd_out;

static int djdma_vi_line = -1;  /* the VI line, from the -w wiring; -1 = not wired */

#define	DJDMA_PORT	0xef	// djdma command start port
#define	DEF_CCA		0x50	// djdma default channel command address
#define SERDATA     0x3e    // djdma serial input location
#define SERFLAG     0x3f    // djdma serial input ack location

typedef unsigned char byte;
typedef unsigned short word;

/*
 * this is the data structure contained in DJDMA memory
 * let's maintain our state here, too.  just like DJDMA-25 does
 */
#define DPARAM      0x1340

struct dparam {
    byte tracks;    // cylinders plus 1
    byte current;   // 0xff - unknown
    byte pattern;   // has drive select bit, starting with 0x80
    byte logical;   // logical drive number 0 - 7
    word step;      // step time
    word headload;  // head load time
    word motor;     // motor on delay
    word settle;    // ste p settle time
    byte firstsec;  // first sector's number
    byte spt;       // user spec - spt
    byte dcb;       // drive config byte (SB1_DD = double density)
    byte precomp;   // track for write precomp
} dparams[8];

/*
 * command codes for the djdma.   these are found at the CCA
 * and are variable length
 */

static unsigned char setdma(), readsec(), writesec(), sense(),
    setintr(), setretry(), setdrive(), settiming(),
    readtrk(), writetrk(), serin(), serout(),
    djhalt(), branch(), setchannel(), settrk(),
    setmp(), read_djmem(), write_djmem(), djexec(), djunknown();
static int secsize_for(int five, int density, int secs);

static struct djcmd {
    unsigned char code;             // command byte
    unsigned char increment;        // how much to adjust channel
    unsigned char status;           // location to write status
    unsigned char (*handler)();     // handler
    char *name;
} djcmd[] = {
    { 0x23, 4, 0, setdma, "set dma address" },
    { 0x20, 5, 4, readsec, "read sector" },
    { 0x21, 5, 4, writesec, "write sector" },
    { 0x22, 6, 5, sense, "sense drive status" },
    { 0x24, 0, 1, setintr, "set interrupt request" },
    { 0x28, 2, 0, setretry, "set error retry count" },
    { 0x2e, 3, 2, setdrive, "set logical drive" },
    { 0x2f, 2, 0, settiming, "set drive timing" },
    { 0x29, 8, 7, readtrk, "read track" },
    { 0x2a, 8, 7, writetrk, "write track" },
    { 0x2b, 3, 2, serout, "serial output" },
    { 0x2c, 2, 0, serin, "serial input enable" },
    { 0x25, 2, 1, djhalt, "controller halt" },
    { 0x26, 0, 0, branch, "branch in channel" },
    { 0x27, 4, 0, setchannel, "set channel address" },
    { 0x2d, 4, 3, settrk, "set track size" },
    { 0x31, 7, 6, setmp, "set media parameters" },
    { 0xa0, 8, 0, read_djmem, "read controller memory" },
    { 0xa1, 8, 0, write_djmem, "write controller memory" },
    { 0xa2, 4, 3, djexec, "execute controller" }
};
struct djcmd unknown = { 0, 0, 0, djunknown, "unknown command" };

/*
 * command status and error codes
 */
#define S_NORMAL    0x40
#define S_ILLREQ    0x80
#define S_ILLDRV    0x81
#define S_UNREADY   0x82
#define S_ILLTRK    0x83
#define S_NOREAD    0x84
#define S_NOSYNC    0x85
#define S_HDRCRC    0x86
#define S_BADSEEK   0x87
#define S_NOHDR0     0x88
#define S_NOHDR1    0x89
#define S_NOHDR2    0x8a
#define S_NOHDR3    0x8b
#define S_NOHDR4    0x8c
#define S_NOHDR5    0x8d
#define S_DATACRC   0x8e
#define S_ILLSEC    0x8f
#define S_PROT      0x90
#define S_DMAERR    0x91
#define S_CHANERR   0x92

static char *djdma_err[] = {
    "illegal request", "illegal drive select", "drive not ready", 
    "illegal track number", "unreadable media", "no sync byte", "header crc",
    "seek error", "sector header miscompare 0", "sector header miscompare 1",
    "sector header miscompare 2", "sector header miscompare 3", 
    "sector header miscompare 4", "sector header miscompare 5",
    "data crc", "illegal sector number", "write protected", "lost data",
    "lost command"
};

/*
 * get drive sense bytes
 */
/* drive characteristic byte */
#define SB1_HARD    0x02    // hard sectored
#define SB1_FIVE    0x04    // 5.25 inch, else 8 inch
#define SB1_MTRCON  0x08    // has motor control
#define SB1_DD      0x10    // double density
#define SB1_NORDY   0x20    // no drive ready signal
#define SB1_NOHDLD  0x40    // no head load signal
#define SB1_HDLD    0x80    // head is loaded

char *sb1_bits[] = {
    "", "hard", "5.25", "motor control",
    "dd", "no ready", "no head load", "head loaded"
};

/* sector length byte */
#define SB2_128     0       // 128 bytes
#define SB2_256     1       // 256 bytes
#define SB2_512     2       // 512 bytes
#define SB2_1024    3       // 1024 bytes

/* drive status byte */
#define SB3_SERIN   0x02    // serial input data bit
#define SB3_DSDD8   0x04    // double sided, double density 8"
#define SB3_INDEX   0x10    // index hole
#define SB3_TRK0    0x20    // track 0
#define SB3_WPROT   0x40    // write protect
#define SB3_RDY     0x80    // drive ready line

char *sb3_bits[] = {
    "", "serin", "dsdd", "", "index", "trk0", "wprot", "rdy"
};

static paddr resetchannel = DEF_CCA;    // the 24 bit channel command pointer
static paddr channel = DEF_CCA;         // the 24 bit channel command pointer
static paddr dmaaddr;                   // the 24 bit dma address
static int retrylimit = 10;
static int djdma_running = 0;
static char secbuf[2048];

/*
 * A routine downloaded to run on the controller's z80, and what a run of
 * the format routine is about to do.  The board cannot run the downloaded
 * z80, so DJEXEC emulates its effect instead: when it is asked to run the
 * routine at DJCODE - which is where the firmware's stack is, and where
 * FORMATDJ puts its formatter - the track the routine would have laid down
 * is filled with FMTFILL, which is all a format does to a medium whose
 * layout is already fixed by the image.  fmt_cyl is the track, advanced by
 * the second routine the host runs after each format (the one that
 * increments the formatter's track byte); fmt_drive is the drive.
 */
#define DJCODE   0x1030   /* where a downloaded routine runs: the firmware stack */
#define DJCODESZ 0x310    /* room up to the drive parameter tables at 0x1340 */
#define FMTFILL  0xe5     /* the data byte a format lays down (FORMATDJ) */
static char djcode[DJCODESZ];
static int fmt_cyl;             /* track the format routine will write next */
static int fmt_drive;           /* the drive it writes */
static int fmt_advance;         /* last exec was a format; this one advances */
/*
 * Physical drives: 0-3 are the 8 inch port, 4-7 the 5 1/4 inch port.
 * That split is the controller's, and Micronix inherits it - djdma.4
 * gives minor 0-3 as 8 inch, 4-7 as 5 1/4 inch, and 8-15 the same again
 * with alternate sectoring, which is the driver's skew and not ours.
 */
static void *imdp[8];
#define FIVE    4               // first physical 5 1/4 inch drive

/*
 * Which bank answers to logical 0-3.  DJ/DMA manual 2.6.7: the 8 inch
 * drives are logically 0-3 and the 5 1/4 inch 4-7 by default, and SET
 * LOGICAL DRIVE with bit 2 set swaps the two.  Since the banks are four
 * apart and four wide, the whole mapping is one bit.
 */
static int five_first;

static int
physdrive(int logical)
{
    return five_first ? (logical ^ FIVE) : logical;
}
static int need_intack;
static int int_posted;      // the interrupt line is actually asserted
static int int_pending;     // asked for, not raised yet - see setintr
#ifndef NODEBUG
int trace_djint;            // command/interrupt handshake
#endif

/*
 * The controller has finished; put the line up.  This runs from a
 * timeout, which is counted in cycles, so it lands the same number of
 * instructions after the command every run.
 */
static void
post_djdma_int(int a)
{
    trace(trace_djint, "djdma RAISE: interrupt asserted%s\n",
        need_intack ? "" : " - NOBODY IS WAITING FOR IT");
    int_posted = 1;
    if (djdma_vi_line >= 0)
        set_vi(djdma_vi_line, 0, 1);
}


static unsigned char
djunknown()
{
    djdma_running = 0;
    return 0;
}

/*
 * start the channel
 */
static void
pulse_djdma(portaddr p, byte v)
{
    int i;
    struct djcmd *cmd;
    unsigned char code;

    if (need_intack) {
        /*
         * if the last command was setintr, we are still "running" that command
         * until we get this pulse.  just advance the channel, since we already posted the status.
         */
        {
            char sbuf[16];

            trace(trace_djint, "djdma INTACK: from %s%s\n",
                dis_space(z80_get_reg16(pc_reg), sbuf, sizeof(sbuf)),
                int_posted ? "" : " - WHICH WAS NEVER RAISED");
        }
        channel += 2;
        need_intack = 0;
        int_posted = 0;
        int_pending = 0;
        if (djdma_vi_line >= 0)
            set_vi(djdma_vi_line, 0, 0);
    } else {
        {
            char sbuf[16];

            /*
             * Who started this.  The kernel reaches the controller from
             * one place - djstart, at the bottom of the strategy path -
             * so naming the caller says which request this is and lets a
             * repeated command be told from a repeated retry.
             */
            trace(trace_djint, "djdma START: channel pulse from %s%s\n",
                dis_space(z80_get_reg16(pc_reg), sbuf, sizeof(sbuf)),
                int_posted ? " - WITH AN INTERRUPT STILL ASSERTED" : "");
        }
        /*
         * fetch from the reset channel address
         */
        channel = resetchannel;
    }
    djdma_running = 1;    
    /*
     * run channel commands until we are told to stop
     */
    while (djdma_running) {
        trace(trace_djdma, "djdma: 0x%x ", channel);
        code = physread(channel);
        cmd = &unknown;
        for (i = 0; i < (sizeof(djcmd) / sizeof(djcmd[0])); i++) {
            if (djcmd[i].code == code) {
                cmd = &djcmd[i];
                break;
            }
        }
        tracec(trace_djdma, "%d %x %s ", code, code, cmd->name);
        i = (*cmd->handler)();
        tracec(trace_djdma, " = %x\n", i);
        if (cmd->status) {
            physwrite(channel + cmd->status, i);
        }
        channel += cmd->increment;
    }
}

/*
 * set the dma address from the channel, little-endian, no status
 */
static unsigned char
setdma()
{
    /* The boot loaders pass a flat 16-bit address (high byte == 0); the
     * kernel passes a 4K segment + 12-bit offset (high byte == segment),
     * matching the MMU (mpz80.c getpte): physical = seg<<12 | off. */
    if (physread(channel + 3) == 0)
        dmaaddr = physread(channel + 1) + (physread(channel + 2) << 8);
    else
        dmaaddr = (physread(channel + 3) << 12) |
            ((physread(channel + 1) + (physread(channel + 2) << 8)) & 0xfff);
    tracec(trace_djdma, "%x", dmaaddr);
    return 0;
}

/*
 * set the default channel address from the channel, little-endian, no status
 */
static unsigned char
setchannel()
{
    resetchannel = physread(channel + 1) + 
        (physread(channel + 2) << 8) +
        (physread(channel + 3) << 16);
    tracec(trace_djdma, "%x", resetchannel);
    return 0;
}

/*
 * end a channel command stream
 */
static unsigned char
djhalt()
{
    djdma_running = 0;
    return S_NORMAL;
}

/*
 * branch in channel - change the fetch address for the channel program
 */
static unsigned char
branch()
{
    channel = physread(channel + 1) + 
        (physread(channel + 2) << 8) +
        (physread(channel + 3) << 16);
    tracec(trace_djdma, "%x", channel);
    return 0;
}

/*
 * read a sector specifiec in 3 address bytes in channel.  1 status byte
 */
static unsigned char
readsec()
{
    unsigned char cyl;
    unsigned char sec;
    unsigned char drive;
    unsigned char head;
    unsigned char status;
    int bytes;

    cyl = physread(channel + 1);
    sec = physread(channel + 2);
    head = sec & 0x80 ? 1 : 0;
    sec &= 0x7f;
    drive = physdrive(physread(channel + 3));

    tracec(trace_bio|trace_djdma, "drive:%d cylinder:%d sec:%d head:%d",
            drive, cyl, sec, head);
    dparams[drive].current = cyl;
    /* read drive, getfdprmtrk, sec, head into dmaaddr */
    if (imdp[drive]) {
        bytes = imd_read(imdp[drive], cyl, head, sec, secbuf);
        if (bytes > 0) {
            tracec(trace_djdma, " dma %x bytes %d magic %.4s", dmaaddr, bytes,
                bytes > 260 ? secbuf + 256 : "....");
            // hexdump(secbuf, bytes);
            copyout(secbuf, dmaaddr, bytes);
            status = S_NORMAL;
        } else {
            status = S_NOREAD;
        }
    } else {
        status = S_ILLDRV;
    }
    return status;
}

/*
 * How long the controller takes to do what it was asked, in simulated
 * microseconds.  A five inch disk turns at 300 rpm, so a revolution is
 * 200ms and an average sector read is a good part of a rotation once
 * latency is counted.  The number does not have to be right; it has to
 * be long enough that the host cannot get back to the controller before
 * the controller has finished, which is the property the driver is
 * written against.
 */
#define DJDMA_INT_DELAY (30 * 1000)

static void post_djdma_int(int a);

/*
 * Ask for an interrupt.
 *
 * The line goes up when the controller finishes, and the controller has
 * its own z80 - the firmware is in resources/DJDMA25 - so finishing
 * takes milliseconds while the host gets on with something else.  Here
 * the whole channel program runs inside the guest's OUT instruction, so
 * without a delay the interrupt is delivered before the instruction that
 * caused it has finished.  Raise the line here and the interrupt is
 * delivered to a machine that has not finished the instruction that
 * caused it; the kernel then pulses the port to acknowledge an interrupt
 * it has not been given yet, and the two get one out of step - which is
 * how a completion goes missing and a process sleeps on a buffer for
 * ever.
 *
 * A timer was tried first and made it worse.  It moved the raise far
 * enough away that the acknowledgement reliably arrived first, so the
 * pending interrupt had to be cancelled to stop the line sticking, and
 * cancelling it lost every completion instead of some.
 *
 * Setting a bit and letting the next poll assert it costs nothing, has
 * no constant to tune, and puts the interrupt strictly after the
 * instruction - which is where real hardware puts it, because a real
 * controller has its own processor and is still busy.  The clock does
 * the same thing with clock_happened, and so does the hdca.
 */
static unsigned char
setintr()
{
    need_intack = 1;
    djdma_running = 0;
    /*
     * One controller, one completion outstanding.  Asking again before
     * the last one landed replaces it rather than queueing behind it -
     * there is only one interrupt line and only one z80 on the board.
     * Without this the timeout table fills during the loader, which
     * polls rather than waiting for interrupts, and the simulator exits
     * with "timeout overflow".
     */
    cancel_time_out(post_djdma_int, 0);
    trace(trace_djint, "djdma REQ:   interrupt requested, firing in %d us\n",
        DJDMA_INT_DELAY);
    time_out("djdma_setintr", DJDMA_INT_DELAY, post_djdma_int, 0);
    return S_NORMAL;
}

/*
 * write a sector using data at the dma address to (cyl, sec, drive)
 */
static unsigned char
writesec()
{
    unsigned char cyl;
    unsigned char sec;
    unsigned char drive;
    unsigned char head;
    unsigned char status;
    int bytes;
    int nsec;

    cyl = physread(channel + 1);
    sec = physread(channel + 2);
    head = sec & 0x80 ? 1 : 0;
    sec &= 0x7f;
    drive = physdrive(physread(channel + 3));

    tracec(trace_bio|trace_djdma, "drive:%d cyl:%d sec:%d head:%d",
        drive, cyl, sec, head);
    dparams[drive].current = cyl;
    if (imdp[drive]) {
        imd_trkinfo(imdp[drive], cyl, head, &nsec, &bytes);
        if (nsec == 0) {
            /*
             * A soft-sectored write to a track with no format is a format:
             * it lays the ID headers down along with the data.  The geometry
             * is the drive's own - SETMP named firstsec and sectors-per-track
             * - and the sector size is the one the count names, the way the
             * firmware's drive tables do (26 of 128, else the double-density
             * size).  This is fdj's path; djformat uses the whole-track write
             * and never reaches here.
             */
            int firstsec = dparams[drive].firstsec;
            int spt = dparams[drive].spt;
            int density = (spt == 26) ? 0 : 1;

            bytes = secsize_for(drive >= FIVE, density, spt);
            imd_setfmt(imdp[drive], cyl, head, firstsec, spt, bytes,
                (density ? 3 : 0) + (drive >= FIVE ? 2 : 0), FMTFILL);
        }
        copyin(secbuf, dmaaddr, bytes);
        bytes = imd_write(imdp[drive], cyl, head, sec, secbuf);
        if (bytes > 0) {
            status = S_NORMAL;
        } else {
            status = S_PROT;
        }
    } else {
        status = S_ILLDRV;
    }
    return status;
}

/*
 * sense drive status
 */
static unsigned char
sense()
{
    unsigned char drive;
    int secsize;
    int nsecs;
    int secsize2;
    int nsecs2;
    byte dcb;       // drive characteristics byte
    byte slc;       // sector length code
    byte dsb;       // drive status byte

    drive = physdrive(physread(channel + 1));

    /*
     * The drive a format will write is the one the host is talking to;
     * sense is the call it makes before it starts, so remember it here.
     */
    fmt_drive = drive;

    /*
     * An empty drive is not an error to ask about - software senses all
     * of them to find out what is there, and the answer for a drive with
     * no disk in it is that it is not ready.  This used to walk into
     * imd_trkinfo with a null and die, which no one noticed while every
     * drive slot was always filled.
     */
    if (!imdp[drive]) {
        trace(trace_djdma, "drive %d: empty", drive);
        return S_UNREADY;
    }

    // look at track 1 to determine density and sidedness - a hack
    imd_trkinfo(imdp[drive], 1, 0, &nsecs, &secsize);

    imd_trkinfo(imdp[drive], 1, 1, &nsecs2, &secsize2);

    // SB1_DS SB1_HDLD SB1_HARD SB1_FIVE SB1_MTRCON SB1_NORDY SB1_NOHDLD
    // SB2_128 SB2_256 SB2_512 SB2_1024
    // SB3_INDEX SB3_RDY SB3_SERIN SB3_TRK0 SB3_WPROT SB3_DSDD8

    /*
     * The density is told from the sector size: 128 byte sectors are the
     * one single density (FM) size, the rest are double.  The IMD mode byte
     * is not used here - every diskette in the tree carries 0, including
     * its double density tracks, so it says nothing.
     */
    switch (secsize) {
    case 128:
        dcb = SB1_HDLD;
        slc = SB2_128;
        dsb = SB3_INDEX | SB3_RDY;
        break;
    case 256:
        dcb = SB1_DD | SB1_HDLD;
        slc = SB2_256;
        dsb = SB3_INDEX | SB3_RDY;
        break;
    case 512:
        dcb = SB1_DD | SB1_HDLD;
        slc = SB2_512;
        dsb = SB3_INDEX | SB3_RDY;
        break;
    case 1024:
        dcb = SB1_DD | SB1_HDLD;
        slc = SB2_1024;
        dsb = SB3_INDEX | SB3_RDY;
        break;
    default:
#ifndef NODEBUG
        printf("fung wha secsize %d nsecs %d\n", secsize, nsecs);
#endif
    }

    /*
     * Which port a drive hangs off is a property of the drive, not of
     * whatever is in it, so it comes from the drive number and not from
     * the media.  Physical 4-7 are the 5 1/4 inch port.
     */
    if (drive >= FIVE) {
        dcb |= SB1_FIVE;
    }

    /*
     * Hard or soft sectored, which is not a detail - it decides how the
     * driver finds out what the disk is.  Micronix reads byte 5c of
     * sector 0 for a configuration byte and looks it up, but only on a
     * hard sectored disk; told the disk is soft sectored it never looks,
     * falls through to the IBM formats, and gets a track offset of zero
     * where every Morrow format has two.  Everything it reads after that
     * is two cylinders low, which looks like a corrupt filesystem rather
     * than like this.
     *
     * We know the answer already: sector numbering is what distinguishes
     * them, hard sectored from 0 and soft sectored from 1, and the track
     * says which it is.
     */
    if (imd_firstsec(imdp[drive], 1, 0) == 0) {
        dcb |= SB1_HARD;
    }
    if (secsize2 > 0) {
        dsb |= SB3_DSDD8;
    }
    if (dparams[drive].current == 0) {
        dsb |= SB3_TRK0;
    }
    physwrite(channel + 2, dcb);
    physwrite(channel + 3, slc);
    physwrite(channel + 4, dsb);

    tracec(trace_djdma, "drive:%d dcb:%x (%s) slc:%x dsb:%x (%s)", 
        drive, dcb, bitdef(dcb, sb1_bits), slc, dsb, bitdef(dsb, sb3_bits));
    return S_NORMAL;
}

/*
 * set retry limit
 */
static unsigned char
setretry()
{
    unsigned char drive;

    retrylimit = physread(channel + 1);
    tracec(trace_djdma, "%d", retrylimit);
    return 0;
}

/*
 * set logical drive
 */
static unsigned char
setdrive()
{
    unsigned char param;
    unsigned char was;

    param = physread(channel + 1);

    /*
     * "bit-2 in the parameter field is the only part of the byte
     * examined by the command" - 4 puts the 5 1/4 inch drives at 0-3,
     * 0 puts them back at 4-7.
     *
     * The status is the logical value the first physical 8 inch drive
     * had BEFORE this command: 40 if it was 0, 44 if it was 4.
     */
    was = five_first ? FIVE : 0;
    five_first = (param & FIVE) ? 1 : 0;

    tracec(trace_djdma, "%02x %s inch drives are now 0-3",
        param, five_first ? "5 1/4" : "8");

    return S_NORMAL | was;
}

/*
 * read track
 */
static unsigned char
readtrk()
{
    unsigned char drive;
    unsigned char head;
    unsigned char cyl;
    paddr sectab;
    int secs;
    int secsize;
    int firstsec;
    int i;
    int bytes;

    cyl = physread(channel + 1);
    head = physread(channel + 2);
    drive = physdrive(physread(channel + 3));
    sectab =
        physread(channel + 4) +
        (physread(channel + 5) << 8) +
        (physread(channel + 6) << 16);
    if (!imdp[drive]) {
        return S_UNREADY;
    }
    imd_trkinfo(imdp[drive], cyl, head, &secs, &secsize);

    dparams[drive].current = cyl;

    tracec(trace_djdma, "drive:%d cyl:%d head:%x sectab:%x secs:%d",
        drive, cyl, head, sectab, secs);

    /*
     * A read track walks the sectors by their number - firstsec through
     * firstsec+secs-1, the DJ49 firmware's counter - and the DMA address
     * follows that counter, one sector length per step.  The sector table
     * entry 0xff means the loader does not want that sector, so the read
     * is skipped but the counter still advances, exactly as DJ49's TTLOOP
     * does.  The sector number is the logical one; the IMD secmap says
     * which physical slot it lives in.
     */
    firstsec = imd_firstsec(imdp[drive], cyl, head);
    for (i = 0; i < secs; i++) {
        if (physread(sectab + i) == 0xff) {
            continue;
        }
        bytes = imd_read(imdp[drive], cyl, head, firstsec + i, secbuf);
        if (bytes > 0) {
            copyout(secbuf, dmaaddr + secsize * i, bytes);
            physwrite(sectab + i, S_NORMAL);
        } else {
            physwrite(sectab + i, S_NOREAD);
        }
    }
    return S_NORMAL;
}

/*
 * The sector size a track holds at a given density, the way the DJ49
 * firmware's drive parameter tables name it.  A 5 1/4 inch drive at single
 * density is 256 byte sectors (North Star), at double 512 (North Star) or
 * 1024 (MicroDecision); an 8 inch drive is 128 at single, and 256 (Series
 * 34), 512 or 1024 at double.
 */
static int
secsize_for(int five, int density, int secs)
{
    if (five) {
        if (density)
            return (secs <= 5) ? 1024 : 512;
        return 256;
    }
    if (density) {
        if (secs <= 8) return 1024;
        if (secs <= 15) return 512;
        return 256;
    }
    return 128;
}

/*
 * write track
 *
 * A format lays down a whole track: the sector headers the geometry names,
 * and the fill byte in each data field.  The layout is the drive's own:
 * firstsec and sectors-per-track, plus the density in the drive config byte
 * and the sector size that density and count together name.  When the host
 * has not named a density - the old djformat only sends SETMP, which carries
 * no density - the one track an 8 inch diskette holds at each density is
 * inferred (26 of 128, 15 of 512, 8 of 1024).  This is where the image
 * learns its per-track metadata instead of already carrying it.
 */
static unsigned char
writetrk()
{
    unsigned char drive;
    unsigned char head;
    unsigned char cyl;
    paddr sectab;
    int secs;
    int secsize;
    int firstsec;
    int density;
    int mode;

    cyl = physread(channel + 1);
    head = physread(channel + 2);
    drive = physdrive(physread(channel + 3));
    sectab =
        physread(channel + 4) +
        (physread(channel + 5) << 8) +
        (physread(channel + 6) << 16);
    if (!imdp[drive]) {
        return S_UNREADY;
    }

    secs = dparams[drive].spt;
    firstsec = dparams[drive].firstsec;
    if (dparams[drive].dcb & (SB1_DD | SB1_HARD | SB1_FIVE))
        density = (dparams[drive].dcb & SB1_DD) ? 1 : 0;
    else
        density = (secs == 26) ? 0 : 1;
    secsize = secsize_for(drive >= FIVE, density, secs);

    /*
     * The IMD mode byte is density plus data rate: FM is 0 on an 8 inch
     * drive and 2 on a 5 1/4, MFM is 3 or 5.  The originals carry these
     * exact bytes.
     */
    mode = (density ? 3 : 0) + (drive >= FIVE ? 2 : 0);

    dparams[drive].current = cyl;

    tracec(trace_djdma, "drive:%d cyl:%d head:%x sectab:%x secs:%d size:%d",
        drive, cyl, head, sectab, secs, secsize);
    imd_setfmt(imdp[drive], cyl, head, firstsec, secs, secsize, mode,
        FMTFILL);
    return S_NORMAL;
}

/*
 * set track count
 */
static unsigned char
settrk()
{
    byte drive;
    byte tracks;

    drive = physdrive(physread(channel + 1));
    tracks = physread(channel + 2);

    tracec(trace_djdma, "drive:%d tracks:%d", drive, tracks);
    return S_NORMAL;
}

/*
 * set media parameters (DJ49's SETMP)
 *
 * The five bytes after the opcode are drive, write-precomp start, highest
 * track, first sector's number, and sectors per track.  Only the last two
 * matter to the medium's layout: the first sector is masked to bit 0 (so
 * it is 0 or 1), and the count is the number djformat named for this
 * density.  The write-track command that follows lays the track down from
 * these two.
 */
static unsigned char
setmp()
{
    byte drive;
    byte firstsec;
    byte spt;

    drive = physdrive(physread(channel + 1));
    firstsec = physread(channel + 4) & 1;
    spt = physread(channel + 5);

    dparams[drive].firstsec = firstsec;
    dparams[drive].spt = spt;
    return S_NORMAL;
}

/*
 * set drive timing
 */
static unsigned char
settiming()
{
    byte timing;

    timing = physread(channel + 1);

    tracec(trace_djdma, "%d", timing);
    return S_NORMAL;
}

/*
 * The bit banged serial port on the DJ/DMA.
 *
 * The controller has a console of its own, and it is not the machine's
 * console: it is two channel commands, 2b to send a byte and 2c to arm
 * a poll that drops a received byte at 3e with a flag at 3f.  A Decision
 * with a MULTIO has no business using it - the MULTIO is the console -
 * and a configuration that does is one somebody built for a machine that
 * had no serial board at all.
 *
 * It is here because the standalone loaders use it, and it stays.  What
 * does not stay is it being invisible: both directions used to share
 * terminal_fd_in and terminal_fd_out with uart 0, so a character out of
 * the controller and a character out of the MULTIO arrived in the same
 * window looking identical, and a keystroke could be swallowed by
 * whichever of the two asked first.  Every use now says so.
 */
int serial_poll = 0;
int djserial_out;               // bytes the controller has sent
int djserial_in;                // and taken

/*
 * The port is bit banged, so a character takes a character time to
 * arrive and the next one cannot be there before that.  The delay is not
 * decoration.  A contyp 4 cbios reads it like this:
 *
 *	ci2:	cmp	m	; wait for 40 at 3f
 *		jz	ci2
 *		mov	m,a	; clear the flag FIRST
 *		dcx	h
 *		ana	m	; and only then fetch 3e
 *
 * - the flag is cleared before the byte is fetched, so anything able to
 * deposit a second character inside those three instructions destroys
 * the first one.  A separate Z80 shifting bits at 9600 baud cannot get
 * near it.  A poll with no rate limit drains the whole input pipe in one
 * go and loses exactly one character per burst, which is what this did:
 * "abc" arrived as "bc".
 */
#define DJSER_BAUD  9600
#define DJSER_CHAR  ((10 * 1000000) / DJSER_BAUD)   // microseconds per character

static unsigned long long serial_next;  // no character before this

static int
djdma_poll_func()
{
    int bytes;
    char conschar;


    /*
     * Only the device that owns the console may take a character out of
     * it: the multio reads the same descriptor, and whichever of us got
     * there first would leave the other waiting forever for a keystroke
     * that had already been delivered somewhere else.
     */
    if (console_owner != CONS_DJDMA) {
        return 0;
    }

    // and no faster than the port can shift one in
    if (simnow64() < serial_next) {
        return 0;
    }

    // if serial polling and there is space
    if (serial_poll && (physread(SERFLAG) != S_NORMAL)) {
        ioctl(terminal_fd_in, FIONREAD, &bytes);
        if (bytes) {
            if (read(terminal_fd_in, &conschar, 1) != 1) {
#ifndef NODEBUG
                printf("djdma_poll_func: read problem\n");
#endif
                return 1;
            }
            djserial_in++;
            serial_next = simnow64() + DJSER_CHAR;
            tracec(trace_djdma, "serial in %02x %s ", conschar & 0xff,
                printable(conschar));
            physwrite(SERDATA, conschar);
            physwrite(SERFLAG, S_NORMAL);
        }
    }
    return 0;
}

/*
 * serial in - this function enables a poll of any serial data into fixed 0x00003e
 * if 0x00003f is not 0x40.  that's the handshake for the serial data.
 */
static unsigned char
serin()
{
    switch (physread(channel + 1)) {
    case 0:
        serial_poll = 0;
        break;
    case 1:
        /*
         * Arming a receive is a claim on the console: a bios that did
         * not have this port for a console would have no reason to ask
         * it for a character.  Claim here as well as on output, because
         * a bios may well read before it writes.
         */
        console_claim(CONS_DJDMA);
        serial_poll = 1;
        break;
    }
    return S_NORMAL;
}

/*
 * serial out
 */
static unsigned char
serout()
{
    byte outch;

    outch = physread(channel + 1);
    console_claim(CONS_DJDMA);
    djserial_out++;
    tracec(trace_djdma, "%02x %s ", outch & 0xff, printable(outch));
    write(terminal_fd_out, &outch, 1);
    return S_NORMAL;
}

/*
 * set djdma memory
 * this is used by the bios to set drive speed, settling time, track count
 * this is a shitty API
 */
static unsigned char
write_djmem()
{
    paddr source;
    vaddr dest;
    vaddr count;
    int i;

    /*
     * The address is the same three-byte form SETDMA takes: byte 3 is the
     * 4K segment and bytes 1-2 the offset, except that a zero segment is a
     * flat 16-bit kernel address (the boot's step/settle table write passes
     * KERNEL, 0, in byte 3).  write_djmem must read it the same way setdma
     * does or a buffer-backed MEMWRITE lands in the wrong segment.
     */
    if (physread(channel + 3) == 0)
        source = physread(channel + 1) + (physread(channel + 2) << 8);
    else
        source = (physread(channel + 3) << 12) |
            ((physread(channel + 1) + (physread(channel + 2) << 8)) & 0xfff);
    count = physread(channel + 4) + physread(channel + 5);
    dest = physread(channel + 6) + (physread(channel + 7) << 8);

    /*
     * A write into the code area is a routine being downloaded - FORMATDJ's
     * formatter, loaded at DJCODE.  Keep the bytes, and treat a download
     * that starts at DJCODE as a fresh format: the first track it will be
     * asked to write is 0.
     */
    if (dest >= DJCODE && dest + count <= DJCODE + DJCODESZ) {
        for (i = 0; i < count; i++)
            djcode[dest - DJCODE + i] = physread(source + i);
        tracec(trace_djdma, "djmem download %x from %x for %d", dest, source, count);
        return S_NORMAL;
    }

    if (dest < DPARAM || (dest + count - DPARAM) > sizeof(dparams)) {
#ifndef NODEBUG
        printf("\twrite_djmem 0x%x outside of DPARAM\n", dest);
#endif
        return S_NORMAL;
    }
    dest -= DPARAM;
    for (i = 0; i < count; i++) {
        ((char *)dparams)[dest + i] = physread(source + i);
    }
    tracec(trace_djdma, "write mem %x from %x for %d ", dest, source, count);
    tracec(trace_djdma, "drive %d set %s", 
        dest / 16, (dest % 16) == 0 ? "tracks" : "timing");
    return S_NORMAL;
}

/*
 * read djdma memory
 */
static unsigned char
read_djmem()
{
    paddr source;
    vaddr dest;
    vaddr count;

    if (physread(channel + 3) == 0)
        source = physread(channel + 1) + (physread(channel + 2) << 8);
    else
        source = (physread(channel + 3) << 12) |
            ((physread(channel + 1) + (physread(channel + 2) << 8)) & 0xfff);
    count = physread(channel + 4) + physread(channel + 5);
    dest = physread(channel + 6) + (physread(channel + 7) << 8);

    tracec(trace_djdma, "read mem %x from %x for %d", dest, source, count);
    return S_NORMAL;
}

/*
 * Write a format's fill to one track: every sector on every side the image
 * has.  The medium's layout is fixed, so a format can only lay down the
 * data byte, FMTFILL, and this is that.  This is FORMATDJ's path: the blob
 * it downloads carries no geometry, only the fill.
 */
static void
fmt_track(drive, cyl)
    int drive, cyl;
{
    int head, secs, secsize, firstsec, i;

    if (!imdp[drive])
        return;
    for (head = 0; head < 2; head++) {
        imd_trkinfo(imdp[drive], cyl, head, &secs, &secsize);
        if (secs == 0)
            continue;
        firstsec = imd_firstsec(imdp[drive], cyl, head);
        memset(secbuf, FMTFILL, secsize);
        for (i = 0; i < secs; i++)
            imd_write(imdp[drive], cyl, head, firstsec + i, secbuf);
    }
}

/*
 * The fdj blob: its first sixteen bytes are the geometry the host patched in
 * and MEMWRITEd to DJCODE, ahead of the format routine that scatters them
 * into its code.  The offsets are the byte-map against FORMATDJ.ASM (dlast,
 * dlcode, verval); the head at DJCODE is djcode[].
 */
#define DJFMT   0x1040      /* fdj's format entry: JP into the format routine */
#define DJADV   0x1043      /* fdj's advance entry: JP into the advance routine */
#define DJFMA   0x1049      /* fdj's combined entry: format then advance */
#define DJ_H_LCODE  4       /* sector length code 0..3 -> 128/256/512/1024 */
#define DJ_H_DLAST  6       /* sectors per track + 1 */
#define DJ_H_FILL   7       /* the fill byte (0xe5) */

/*
 * Lay down one track the way fdj's downloaded blob does.  The geometry is the
 * blob head's own - the sector length code and the sectors-per-track - and
 * the sector size and count follow the firmware's tables: code 0 is the one
 * single density size (128), the rest are double.  Whether sectors number
 * from 0 (hard sectored) or 1 (soft) is the medium's own, kept from the
 * track already there.  This is fdj's path, where the medium's layout is
 * not yet fixed and the blob says what it becomes.
 */
static void
djfmt(drive, cyl)
    int drive, cyl;
{
    int head, nsec, secsize, firstsec, fill, density, mode;

    if (!imdp[drive])
        return;
    nsec = djcode[DJ_H_DLAST] - 1;
    secsize = 128 << djcode[DJ_H_LCODE];
    /*
     * Whether sectors are numbered from 0 or 1 is the medium's own - hard
     * sectored from 0, soft from 1 - and the blob does not say it, so it
     * comes from the track the drive already has, the way sense() reads it.
     */
    firstsec = imd_firstsec(imdp[drive], cyl, 0);
    fill = djcode[DJ_H_FILL];
    density = (djcode[DJ_H_LCODE] == 0) ? 0 : 1;
    mode = (density ? 3 : 0) + (drive >= FIVE ? 2 : 0);
    trace(trace_djdma, "djfmt drive %d cyl %d nsec %d secsize %d mode %d\n",
        drive, cyl, nsec, secsize, mode);
    for (head = 0; head < 2; head++)
        imd_setfmt(imdp[drive], cyl, head, firstsec, nsec, secsize, mode, fill);
}

/*
 * Execute controller code.
 *
 * The board's z80 is not emulated, so a routine the host downloaded is not
 * run byte for byte; its effect is.  Two hosts download one:
 *
 *   FORMATDJ's formatter runs at DJCODE: it writes one track, and the host
 *   follows it by running a second, shorter routine that advances the
 *   formatter's track byte.  So an exec at DJCODE fills the track fmt_cyl
 *   names, and the exec right after it - the advance - moves fmt_cyl on.
 *
 *   fdj's blob has three entries: DJFMT formats a track from its head's
 *   geometry, DJADV advances to the next track, and DJFMA does both - the
 *   steady-state format loop runs DJFMA once per track.
 *
 * Anything else is a routine with no effect on the medium and passes as
 * success.
 */
static unsigned char
djexec()
{
    vaddr target;

    target = physread(channel + 1) + (physread(channel + 2) << 8);
    tracec(trace_djdma, " target %x", target);
    if (target == DJCODE) {
        fmt_track(fmt_drive, fmt_cyl);
        fmt_advance = 1;
    } else if (target == DJFMT) {
        djfmt(fmt_drive, fmt_cyl);
        fmt_advance = 1;
    } else if (target == DJADV) {
        fmt_cyl++;
        fmt_advance = 0;
    } else if (target == DJFMA) {
        djfmt(fmt_drive, fmt_cyl);
        fmt_cyl++;
        fmt_advance = 0;
    } else if (fmt_advance) {
        fmt_cyl++;
        fmt_advance = 0;
    }
    return S_NORMAL;
}

byte lowmem_save[38];
byte bootstrap[] = {
    0x21, 0x4a, 0x00,   //      ld hl, 004a
    0x36, 0x00,         //      ld (hl), 0
    0x7e,               // loop:ld a, (hl)
    0xb7,               //      or a,a
    0xca, 0x3d, 0x00,   //      jp z, loop
    0xfe, 0x40,         //      cp a,0x40
    0xc2, 0x3d, 0x00,   //      jp nz, loop
    0xc3, 0x80, 0x00,   //      jp 80
    0xff                //      db ff
};

extern char **drivenames;
extern char **fivenames;
/*
 * hook up the registers and do reset processing
 * this runs the load sector 0 to physical memory 0
 */
/*
 * The media changer (sys/MEDIACHG.md): out-of-band removable-media
 * control on port 3.  A guest process - or a shell through
 * /dev/mediachg, c 3 3 - writes "@<command>\n"; the command runs
 * against the media sets loaded from the -e manifest, and the answer
 * comes back on the same port.  '@' is the leadin and the resync
 * point: a command without it is discarded, which is what makes a
 * guest that died mid-command harmless.
 */

#define MEDIA_PORT      3       /* the media changer's port (dev/devlist) */
#define MEDIA_MAX       80      /* longest command, '@' excluded */

struct mediaset {
    char *name;
    char **vol;                 /* one image path per volume */
    int nvol;
};

static struct mediaset *msets;  /* the sets, from the -e manifest */
static int nmsets;

/* what the changer last put in each drive: msets index, or -1 (empty) */
static int loaded_set[8];
static int loaded_vol[8];

static char mcbuf[MEDIA_MAX];   /* the command being received */
static int mcn;
static int mc_idle = 1;         /* waiting for the '@' leadin */
static char resp[MEDIA_MAX];    /* the staged response line */
static int resp_len;
static int resp_pos;

static void
stage_response(char *s)
{
    int n = strlen(s);

    if (n >= MEDIA_MAX)
        n = MEDIA_MAX - 1;
    memcpy(resp, s, n);
    resp[n++] = '\n';
    resp_len = n;
    resp_pos = 0;
}

/* next whitespace-delimited token from *p, advanced past it; 0 at end */
static char *
token(char **p)
{
    char *s = *p;
    char *t;

    while (*s == ' ' || *s == '\t')
        s++;
    if (*s == '\0')
        return 0;
    t = s;
    while (*t && *t != ' ' && *t != '\t')
        t++;
    if (*t)
        *t++ = '\0';
    *p = t;
    return s;
}

/* "name:number" split at the colon; -1 if malformed */
static int
split_colon(char *s, char **namep, int *nump)
{
    char *colon = strchr(s, ':');
    char *d;

    if (!colon || colon == s || colon[1] == '\0')
        return -1;
    *colon = '\0';
    *namep = s;
    for (d = colon + 1; *d; d++)
        if (*d < '0' || *d > '9')
            return -1;
    *nump = atoi(colon + 1);
    return 0;
}

static int
find_set(char *name)
{
    int i;

    for (i = 0; i < nmsets; i++)
        if (strcmp(msets[i].name, name) == 0)
            return i;
    return -1;
}

/* "djdma:0" to the physical drive number, or -1 */
static int
parse_drive(char *s)
{
    char *name;
    int logical;

    if (split_colon(s, &name, &logical) < 0)
        return -1;
    if (strcmp(name, "djdma") != 0)
        return -1;
    if (logical < 0 || logical > 7)
        return -1;
    return physdrive(logical);
}

/* eject a physical drive: close the image, sense() then says not ready */
static void
eject_drive(int drive)
{
    if (imdp[drive]) {
        imd_close(imdp[drive]);
        imdp[drive] = 0;
    }
    loaded_set[drive] = -1;
    loaded_vol[drive] = -1;
}

/* load a path into a physical drive, -1 on failure */
static int
load_drive(int drive, char *path)
{
    void *np;

    if (access(path, R_OK) != 0)
        return -1;
    np = imd_load(path, drive, 1);
    if (!np)
        return -1;
    if (imdp[drive])
        imd_close(imdp[drive]);
    imdp[drive] = np;
    return 0;
}

static void
media_execute(char *s, int n)
{
    char buf[MEDIA_MAX + 1];
    char *p, *verb, *a1, *a2, *a3;
    char *name;
    int num, set, drive;

    if (n > MEDIA_MAX)
        n = MEDIA_MAX;
    memcpy(buf, s, n);
    buf[n] = '\0';
    p = buf;

    verb = token(&p);
    if (!verb) {
        stage_response("error syntax");
        return;
    }
    a1 = token(&p);

    if (strcmp(verb, "load") == 0) {
        a2 = token(&p);
        a3 = token(&p);
        if (!a1 || !a2 || !a3 || strcmp(a2, "onto") != 0) {
            stage_response("error syntax");
            return;
        }
        if (split_colon(a1, &name, &num) < 0) {
            stage_response("error syntax");
            return;
        }
        set = find_set(name);
        if (set < 0) {
            stage_response("error noset");
            return;
        }
        if (num < 0 || num >= msets[set].nvol) {
            stage_response("error novol");
            return;
        }
        drive = parse_drive(a3);
        if (drive < 0) {
            stage_response("error nodrive");
            return;
        }
        if (load_drive(drive, msets[set].vol[num]) < 0) {
            stage_response("error ioerror");
            return;
        }
        loaded_set[drive] = set;
        loaded_vol[drive] = num;
        stage_response("ok");
        return;
    }

    if (strcmp(verb, "unload") == 0 || strcmp(verb, "eject") == 0) {
        if (!a1) {
            stage_response("error syntax");
            return;
        }
        drive = parse_drive(a1);
        if (drive < 0) {
            stage_response("error nodrive");
            return;
        }
        eject_drive(drive);
        stage_response("ok");
        return;
    }

    if (strcmp(verb, "query") == 0) {
        if (!a1) {
            stage_response("error syntax");
            return;
        }
        if (strchr(a1, ':') == 0) {
            set = find_set(a1);
            if (set < 0) {
                stage_response("error noset");
                return;
            }
            sprintf(buf, "ok %d", msets[set].nvol);
            stage_response(buf);
            return;
        }
        drive = parse_drive(a1);
        if (drive < 0) {
            stage_response("error nodrive");
            return;
        }
        if (loaded_set[drive] < 0) {
            stage_response("ok empty");
            return;
        }
        sprintf(buf, "ok %s:%d", msets[loaded_set[drive]].name,
            loaded_vol[drive]);
        stage_response(buf);
        return;
    }

    if (strcmp(verb, "next") == 0) {
        if (!a1) {
            stage_response("error syntax");
            return;
        }
        drive = parse_drive(a1);
        if (drive < 0) {
            stage_response("error nodrive");
            return;
        }
        if (loaded_set[drive] < 0) {
            stage_response("error nomedia");
            return;
        }
        set = loaded_set[drive];
        num = loaded_vol[drive] + 1;
        if (num >= msets[set].nvol) {
            stage_response("error novol");
            return;
        }
        if (load_drive(drive, msets[set].vol[num]) < 0) {
            stage_response("error ioerror");
            return;
        }
        loaded_vol[drive] = num;
        stage_response("ok");
        return;
    }

    stage_response("error syntax");
}

/* the response side of port 3: the next staged byte, 0 when exhausted */
static byte
media_in(portaddr p)
{
    if (resp_pos < resp_len)
        return resp[resp_pos++];
    return 0;
}

/* the command side: '@' resyncs, '\n' runs, anything else accumulates */
static void
media_out(portaddr p, byte v)
{
    if (v == '@') {
        mcn = 0;
        mc_idle = 0;
        resp_len = 0;           /* a stale response dies with the leadin */
        resp_pos = 0;
        return;
    }
    if (mc_idle)
        return;                 /* no leadin: drop it */
    if (v == '\n') {
        media_execute(mcbuf, mcn);
        mcn = 0;
        mc_idle = 1;
        return;
    }
    if (mcn >= MEDIA_MAX) {     /* overlong line: resync on the next '@' */
        stage_response("error syntax");
        mcn = 0;
        mc_idle = 1;
        return;
    }
    mcbuf[mcn++] = v;
}

/* load the -e manifest: one line per set, name then image paths */
static void
media_load_manifest(char *path)
{
    FILE *fp;
    char line[4096];

    if (!path)
        return;
    fp = fopen(path, "r");
    if (!fp) {
        printf("media: could not open %s\n", path);
        return;
    }
    while (fgets(line, sizeof line, fp)) {
        struct mediaset *s;
        char *p, *tok, *vol;
        int nalloc;

        p = strchr(line, '\n');
        if (p)
            *p = '\0';
        p = line;
        while (*p == ' ' || *p == '\t')
            p++;
        if (*p == '\0' || *p == '#')
            continue;

        tok = token(&p);
        if (!tok)
            continue;

        msets = realloc(msets, (nmsets + 1) * sizeof *msets);
        s = &msets[nmsets];
        s->name = strdup(tok);
        s->nvol = 0;
        s->vol = 0;
        nalloc = 0;
        while ((vol = token(&p)) != 0) {
            if (s->nvol == nalloc) {
                nalloc = nalloc ? nalloc * 2 : 8;
                s->vol = realloc(s->vol, nalloc * sizeof *s->vol);
            }
            s->vol[s->nvol++] = strdup(vol);
        }
        nmsets++;
    }
    fclose(fp);
}

static int
djdma_init()
{
    int i;
    char **s;

	djdma_vi_line = wiring_vi_line("djdma");
	register_output(DJDMA_PORT, &pulse_djdma);
	register_input(MEDIA_PORT, &media_in);
	register_output(MEDIA_PORT, &media_out);
	media_load_manifest(medias_file);
	for (i = 0; i < 8; i++)
		loaded_set[i] = loaded_vol[i] = -1;

    /*
     * The formatter's track counter starts at 0 for a fresh format.  It
     * is advanced by the downloaded routine's "advance" entry, not reset
     * by a download: fdj downloads the geometry twice (track 0's, then the
     * chosen format's) in one format, and the second download must keep the
     * already-advanced track.
     */
    fmt_cyl = 0;
    fmt_advance = 0;

    if (drivenames) {
        for (i = 0; drivenames[i] && i < FIVE; i++) {
            imdp[i] = imd_load(drivenames[i], i, 1);
            if (!imdp[i]) {
#ifndef NODEBUG
                printf("djdma_init: could not open %s\n",
                    drivenames[i]);
#endif
                return 1;
            }
        }
    }
    if (fivenames) {
        for (i = 0; fivenames[i] && i < FIVE; i++) {
            imdp[FIVE + i] = imd_load(fivenames[i], FIVE + i, 1);
            if (!imdp[FIVE + i]) {
#ifndef NODEBUG
                printf("djdma_init: could not open %s\n",
                    fivenames[i]);
#endif
                return 1;
            }
        }
    }

    /*
     * The DJDMA25 firmware boots from the first ready drive, testing the
     * 8 inch port first, and renumbers so that drive answers to 0 (manual
     * 2.6.7, section 7).  Emulated here, because the boot below reads
     * logical 0 and has to land on the drive the firmware chose.  The
     * kernel then wants the fixed numbers its dev/devlist names - 8 inch
     * at 0-3, 5 1/4 at 4-7 - so the drive is put back with SET LOGICAL
     * DRIVE (0x2e, bit 2 clear) before it runs.
     */
    if (!imdp[0] && imdp[FIVE]) {
#ifndef NODEBUG
        printf("djdma: booting the 5 1/4 inch port, its drives are now 0-3\n");
#endif
        five_first = 1;
    }

    for (i = 0; i < 38; i++) {
        lowmem_save[i] = physread(i);
        physwrite(i, 0);
    }
    for (i = 0; i < sizeof(bootstrap); i++) {
        physwrite(i + 0x38, bootstrap[i]);
    }
    /*
     * The bootstrap loads the first sector of the boot drive, and the
     * boot drive is logical 0 - which after the renumbering above may be
     * on either port.  Manual section 7: 80 hex bytes are loaded from
     * the first sector on track 0 of the disk, and the cpu is sent to
     * 000080.
     */
    if (imdp[physdrive(0)]) {
        imd_read(imdp[physdrive(0)], 0, 0,
            imd_firstsec(imdp[physdrive(0)], 0, 0), secbuf);
        copyout(secbuf, 0x80, 0x80);
    } else {
#ifndef NODEBUG
        printf("djdma: no disk to boot from\n");
#endif
    }
    physwrite(0x4a, 0x40);
    return 0;
}

int
djdma_setup()
{
#ifndef NODEBUG
    trace_djdma = register_trace("djdma");
    trace_djint = register_trace("djint");
#endif
    return 0;
}

struct driver djdma_driver = {
    "djdma",
    0,                  // usage
    &djdma_setup,       // prearg
    &djdma_init,        // startup
    &djdma_poll_func,   // poll
};

/*
 * this grammar makes the compiler call this function before main()
 * this means we can add drivers by just adding them to the link
 */
__attribute__((constructor))
void
register_djdma_driver()
{
    register_driver(&djdma_driver);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
