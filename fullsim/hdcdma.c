/*
 * the hdcdma is an intelligent controller with an onboard 8x300 that
 * processes requests placed into s100 memory when tickled by an
 * out to HDCDMA_PORT
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

#define	HDCDMA_PORT 	0x55	// hdcdma command start port
#define	HDCDMA_RESET	0xe4	// hdcdma command start port

#define	DEF_CCA		0x50	// djdma default channel command address

#ifdef notdef
/*
 * command codes for the djdma.   these are found at the CCA
 * and are variable length
 */

static unsigned char setdma(), readsec(), writesec(), sense(),
    setintr(), setretry(), setdrive(), settiming(),
    readtrk(), writetrk(), serin(), serout(),
    djhalt(), branch(), setchannel(), settrk(),
    read_djmem(), write_djmem(), djexec();

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
    { 0x24, 0, 0, setintr, "set interrupt request" },
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
    "", "hard sectored", "5.25 inch", "had motor control",
    "double density", "no drive ready", "no head load", "head loaded"
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
    "", "serin", "DSDD8", "", "index", "trk0", "wprot", "rdy"
};

static paddr resetchannel;  // the 24 bit channel command pointer
static paddr channel;       // the 24 bit channel command pointer
static paddr dmaaddr;       // the 24 bit dma address
static int retrylimit = 10;
static int djdma_running = 0;
static char secbuf[2048];
static void *imdp[8];
static int need_intack;

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
         * until we get this pulse.  fill in the status and advance
         */
        physwrite(channel + 1, S_NORMAL);
        channel += 2;
        need_intack = 0;
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
        code = physread(channel);
        for (i = 0; i < (sizeof(djcmd) / sizeof(djcmd[0])); i++) {
            if (djcmd[i].code == code) {
                cmd = &djcmd[i];
            }
        }
        if (!cmd) {
            djdma_running = 0;
            printf("unknown djdma command %d %x\n", code, code);
        } else {
            printf("djdma command %d %x %s\n", code, code, cmd->name);
            i = (cmd->handler)();
            if (cmd->status) {
                physwrite(channel + cmd->status, i);
            }
            channel += cmd->increment;
        }
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
    return 0;
}

/*
 * read a sector specifiec in 3 address bytes in channel.  1 status byte
 */
static unsigned char
readsec()
{
    unsigned char trk;
    unsigned char sec;
    unsigned char drive;
    unsigned char side;
    unsigned char status;
    int bytes;

    trk = physread(channel + 1);
    sec = physread(channel + 2);
    side = sec & 0x80;
    sec &= 0x7f;
    drive = physread(channel + 3);

    /* read drive, getfdprmtrk, sec, side into dmaaddr */
    if (imdp[drive]) {
        bytes = imd_read(imdp[drive], trk, sec, side, drive, secbuf);
        if (bytes > 0) {
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
 * generate an interrupt.  the next output pulse is an intack, and does
 * not start the channel.
 */
static unsigned char
setintr()
{
    need_intack = 1;
    return 0;
}

/*
 * write a sector using data at the dma address to (trk, sec, drive)
 */
static unsigned char
writesec()
{
    return S_PROT;
}

/*
 * sense drive status
 */
static unsigned char
sense()
{
    unsigned char drive;

    drive = physread(channel + 1);
    /* XXX - fill out drive status */
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
    return S_NORMAL;
}

/*
 * read track
 */
static unsigned char
readtrk()
{
    return S_NORMAL;
}

/*
 * write track
 */
static unsigned char
writetrk()
{
    return S_NORMAL;
}

/*
 * set track count
 */
static unsigned char
settrk()
{
    return S_NORMAL;
}

/*
 * set drive timing
 */
static unsigned char
settiming()
{
    return S_NORMAL;
}

/*
 * serial in
 */
static unsigned char
serin()
{
    return S_NORMAL;
}

/*
 * serial out
 */
static unsigned char
serout()
{
    return S_NORMAL;
}

/*
 * set djdma memory
 */
static unsigned char
write_djmem()
{
    return S_NORMAL;
}

/*
 * read djdma memory
 */
static unsigned char
read_djmem()
{
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
#endif

static void
pulse_hdcdma(portaddr p, byte v)
{
}

static void
reset_hdcdma(portaddr p, byte v)
{
}

static int
hdcdma_init()
{
	register_output(HDCDMA_PORT, &pulse_hdcdma);
	register_output(HDCDMA_RESET, &reset_hdcdma);
    return 0;
}

/*
 * this grammar makes the compiler call this function before main()
 * this means we can add drivers by just adding them to the link
 */
__attribute__((constructor))
void
register_hdcdma_driver()
{
    register_startup_hook(hdcdma_init);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */