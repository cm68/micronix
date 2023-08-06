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
#include "hwsim.h"
#include "imd.h"
#include "util.h"
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdio.h>

#define DELAYED_DJINT 1

int trace_djdma;
extern int trace_bio;

extern int terminal_fd_in;
extern int terminal_fd_out;

#define DJDMA_INTERRUPT 1
#define DJDMA_INT_DELAY (30 * 1000)

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
    byte dcb;       // drive config byte
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
    read_djmem(), write_djmem(), djexec(), djunknown();

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
    { 0xa0, 8, 0, read_djmem, "read controller memory" },
    { 0xa1, 8, 0, write_djmem, "write controller memory" },
    { 0xa2, 3, 0, djexec, "execute controller" }
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
static void *imdp[8];
static int need_intack;

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
        channel += 2;
        need_intack = 0;
        set_vi(DJDMA_INTERRUPT, 0, 0);
    } else {
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
    dmaaddr = physread(channel + 1) + 
        (physread(channel + 2) << 8) +
        (physread(channel + 3) << 16);
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
    drive = physread(channel + 3);

    tracec(trace_bio|trace_djdma, "drive:%d cylinder:%d sec:%d head:%d",
            drive, cyl, sec, head);
    dparams[drive].current = cyl;
    /* read drive, getfdprmtrk, sec, head into dmaaddr */
    if (imdp[drive]) {
        bytes = imd_read(imdp[drive], cyl, head, sec, secbuf);
        if (bytes > 0) {
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

#ifdef DELAYED_DJINT
static void
post_djdma_int(int a)
{
    set_vi(DJDMA_INTERRUPT, 0, 1);
}
#endif

/*
 * generate an interrupt.  the next output pulse is an intack, and does
 * not start the channel.  the doc is definitely ambiguous, but we generate the status immediately.
 */
static unsigned char
setintr()
{
    need_intack = 1;
    djdma_running = 0;
#ifdef DELAYED_DJINT
    time_out("djdma_setintr", DJDMA_INT_DELAY, post_djdma_int, 0);
#else
    set_vi(DJDMA_INTERRUPT, 0, 1);
#endif
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

    cyl = physread(channel + 1);
    sec = physread(channel + 2);
    head = sec & 0x80 ? 1 : 0;
    sec &= 0x7f;
    drive = physread(channel + 3);

    tracec(trace_bio|trace_djdma, "drive:%d cyl:%d sec:%d head:%d",
        drive, cyl, sec, head);
    dparams[drive].current = cyl;
    if (imdp[drive]) {
        imd_trkinfo(imdp[drive], cyl, head, 0, &bytes);
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

    drive = physread(channel + 1);

    // look at track 1 to determine density and sidedness - a hack
    imd_trkinfo(imdp[drive], 1, 0, &nsecs, &secsize);

    imd_trkinfo(imdp[drive], 1, 1, &nsecs2, &secsize2);

    // SB1_DS SB1_HDLD SB1_HARD SB1_FIVE SB1_MTRCON SB1_NORDY SB1_NOHDLD
    // SB2_128 SB2_256 SB2_512 SB2_1024
    // SB3_INDEX SB3_RDY SB3_SERIN SB3_TRK0 SB3_WPROT SB3_DSDD8

    switch(secsize) {
    case 128:
        dcb = SB1_HDLD;
        slc = SB2_128;
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
        printf("fung wha secsize %d nsecs %d\n", secsize, nsecs);
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
    unsigned char drive;

    drive = physread(channel + 1);
    tracec(trace_djdma, "%d", drive);
    return S_NORMAL;
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
    int i;
    int bytes;

    cyl = physread(channel + 1);
    head = physread(channel + 2);
    drive = physread(channel + 3);
    sectab = 
        physread(channel + 4) + 
        (physread(channel + 5) << 8) +
        (physread(channel + 6) << 16);
    imd_trkinfo(imdp[drive], cyl, head, &secs, &secsize);

    dparams[drive].current = cyl;

    tracec(trace_djdma, "drive:%d cyl:%d head:%x sectab:%x secs:%d", 
        drive, cyl, head, sectab, secs);
    for (i = 0; i < secs; i++) {
        if (physread(sectab + i) == 0xff) {
            continue;
        }
        bytes = imd_read(imdp[drive], cyl, head, i + 1, secbuf);
        if (bytes > 0) {

            copyout(secbuf, dmaaddr + secsize * i, bytes);
            // if (trace & trace_djdma) printf("\tcopyout to %x for %d\n", dmaaddr + secsize * i, bytes);
            physwrite(sectab + i, S_NORMAL);
        } else {
            physwrite(sectab + i, S_NOREAD);
        }
    }
    return S_NORMAL;
}

/*
 * write track
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

    cyl = physread(channel + 1);
    head = physread(channel + 2);
    drive = physread(channel + 3);
    sectab = 
        physread(channel + 4) + 
        (physread(channel + 5) << 8) +
        (physread(channel + 6) << 16);
    imd_trkinfo(imdp[drive], cyl, head, &secs, &secsize);

    dparams[drive].current = cyl;

    tracec(trace_djdma, "drive:%d cyl:%d head:%x sectab:%x secs:%d", 
        drive, cyl, head, sectab, secs);
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

    drive = physread(channel + 1);
    tracks = physread(channel + 2);

    tracec(trace_djdma, "drive:%d tracks:%d", drive, tracks);
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

int serial_poll = 0;

static int
djdma_poll_func()
{
    int bytes;
    char conschar;

    // if serial polling and there is space
    if (serial_poll && (physread(SERFLAG) != S_NORMAL)) {
        ioctl(terminal_fd_in, FIONREAD, &bytes);
        if (bytes) {
            if (read(terminal_fd_in, &conschar, 1) != 1) {
                printf("djdma_poll_func: read problem\n");
                return 1;
            }
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

    source = physread(channel + 1) + (physread(channel + 2) << 8) + (physread(channel + 3) << 16);
    count = physread(channel + 4) + physread(channel + 5); 
    dest = physread(channel + 6) + (physread(channel + 7) << 8); 
    
    if (dest < DPARAM || (dest + count - DPARAM) > sizeof(dparams)) {
        printf("\twrite_djmem 0x%x outside of DPARAM\n", dest);
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

    source = physread(channel + 1) + (physread(channel + 2) << 8) + (physread(channel + 3) << 16);
    count = physread(channel + 4) + physread(channel + 5); 
    dest = physread(channel + 6) + (physread(channel + 7) << 8); 

    tracec(trace_djdma, "read mem %x from %x for %d", dest, source, count);
    return S_NORMAL;
}

/*
 * execute controller code
 */
static unsigned char
djexec()
{
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
/*
 * hook up the registers and do reset processing
 * this runs the load sector 0 to physical memory 0
 */
static int
djdma_init()
{
    int i;
    char **s;

	register_output(DJDMA_PORT, &pulse_djdma);

    if (drivenames) {
        for (i = 0; drivenames[i]; i++) {
            imdp[i] = imd_load(drivenames[i], i, 1);
            if (!imdp[i]) {
                printf("djdma_init: could not open %s\n",
                    drivenames[i]);
                return 1;
            }
        }
    }

    for (i = 0; i < 38; i++) {
        lowmem_save[i] = physread(i);
        physwrite(i, 0);
    }
    for (i = 0; i < sizeof(bootstrap); i++) {
        physwrite(i + 0x38, bootstrap[i]);
    }
    imd_read(imdp[0], 0, 0, 1, secbuf);
    copyout(secbuf, 0x80, 0x80);
    physwrite(0x4a, 0x40);
    return 0;
}

int
djdma_setup()
{
    trace_djdma = register_trace("djdma");
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
