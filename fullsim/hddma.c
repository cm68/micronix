/*
 * the hddma is an intelligent controller with an onboard 8x300 that
 * processes requests placed into s100 memory when tickled by an
 * out to HDDMA_PORT
 * although it is possible to do this with a child process and get
 * real concurrency, this is way overkill.  let's just do this with
 * everything being done synchronously.
 *
 * this will be complicated in that a channel command may set an interrupt
 * line.  this just changes the sense of what the output to HDDMA_PORT
 * does to an intack, which causes the next channel command in series
 * to be fetched.
 * so, all work in this driver happens in response to, and in series with, 
 * an output instruction, which may be in an interrupt handler
 *
 * this controller is also called the hdc-dma, hdcdma, etc.
 *
 * booting/formatting/sysgen problems:
 * mon447: reads 512 sector, which fails on a 1k (cp/m) formatted disk
 * mon375: reads 1024 sector, which fails on a 512 (micronix) formatted disk
 * to make the mon447 work with cp/m, if and only if we are reading block
 * 0 of the disk, we ignore the sector size set to 512 aud use 1024 instead
 */

#include "sim.h"
#include <fcntl.h>
#include <stdio.h>

#define	HDDMA_PORT 	0x55	// hddma attention port
#define	HDDMA_RESET	0x54	// hddma reset port

#define	DEF_CCA     0x50	    // hddma default channel command address

#define DRIVES  4
static int drive[DRIVES];       // file descriptor
static int track[DRIVES];       // where are we - used for format and seek
static int secsz[DRIVES];       // sector size from specify/format

int trace_hddma;
extern int trace_bio;

/*
 * the in-ram format for a hd-dma command block
 */
struct hd_cmd {
    byte seldir;        // select drive and direction
#define DRV_MASK    0x03
#define STEP_DOWN   0x10
    byte step_low;      // step count
    byte step_high;
   
    byte selhd;         // select drive, head, low, precomp
#define HEAD_MASK   0x1c
#define HEAD_SHIFT  2
#define HEAD_CMP    0x7     // complement bits
#define LOW_CURR    0x40
#define PRECOMP     0x80

    byte dma_low;       // dma address
    byte dma_mid;
    byte dma_high;

    /*
     * the next 4 bytes are used differently for the read/write, 
     * format and specify commands
     */
    byte arg0;
    byte arg1;
    byte arg2;
    byte arg3;

    /*
     * for read/write, the arg bytes really are match bytes for the 4 byte sector header
     * however, since i need to map the cyl/head/sector to a data file, i enforce a match
     * with the above selhd bits, the track #, and the bytes here
     */
#define cyl_low     arg0
#define cyl_high    arg1
#define hd          arg2
#define sec         arg3

    /*
     * format is a different story - the arg bytes are used to set formatting informatio
     */
#define gap3        arg0    // gap3 size
#define sptneg      arg1    // negated sector count
#define fseccode     arg2    // a size code 
#define     FSECSIZE(k) 128 * (((k) ^ 0xff) + 1);
#define fill        arg3    // fill byte

    /*
     * the specify command is used to tell about the drive
     */
#define steprate    arg1    // units of 100 microseconds 0 = buffered
#define INTERRUPT   0x80    // assert interrupt after command
#define settle      arg2    // head settle in microseconds
#define sseccode     arg3   // a size code
#define     SSECSIZE(k) 128 * ((k) + 1)

    byte opcode;
#define OP_READ     0       // read data
#define OP_WRITE    1       // write data
#define OP_RHEAD    2       // read header
#define OP_FMT      3       // format track
#define OP_SPEC     4       // specify constants
#define OP_SENSE    5       // sense drive status
#define OP_NOP      6       // nothing

    byte status;
#define BUSY        0x00    // drive busy
#define UNREADY     0x01    // drive not ready
#define NOHDR       0x04    // header not found
#define NODATA      0x05    // data not found
#define OVERRUN     0x06    // data overrun
#define DATACRC     0x07    // data crc error
#define WRFAULT     0x08    // write fault
#define HDRCRC      0x09    // header crc error
#define BADCMD      0xA0    // illegal command
#define GOOD        0xff    // good completion

    /*
     * the sense command returns a bitmask in here instead of the following codes
     * these are active LOW
     */
#define SS_TRK0     0x01    // at track 0
#define SS_WFLT     0x02    // write fault
#define SS_RDY      0x04    // drive ready
#define SS_SDONE    0x08    // seek done
#define SS_INDEX    0x10    // toggles each rev
#define SS_DONE     0xe0    // set after command

    byte link_low;          // next command link
    byte link_mid;
    byte link_high;
} command;

static int channel_reset = 1;
static paddr channel;       // channel command address
static paddr dmaaddr;       // the 24 bit dma address
static char secbuf[2048];
static int enable_intr;

/*
 * these are hugely wasteful, so I am opting for a minimum of 512
 * for sector sizes, total sectors per track
 * 128      56
 * 256      32
 * 512      17
 * 1024     9
 * 2048     4
 */
#define SPT     17 
#define HEADS   8
#define SECLEN  2048
#define TRACKS  2048

#define secoff(s, h, t) \
    (SECLEN * (((((t) * HEADS) + (h)) * SPT) + (s)))
    
static char *sense_b[] = { "trk0", "wfault", "ready", "seekcomplete", "index", 0, 0, 0 };
static char *cmdname[] = { "read", "write", "readhead", "format", "specify", "sense", "nop" };
/*
 * start the channel
 */
static void
attention(portaddr p, byte v)
{
    int offset;
    byte status;
    paddr link;
    byte drv;
    int steps;
    int secsize;
    int i;
    int drivefd;
    int head;

    if (channel_reset) {
        channel_reset = 0;
        channel = physread(DEF_CCA) + (physread(DEF_CCA+1) << 8) + (physread(DEF_CCA+2) << 16);
    }

    if (trace & trace_hddma) printf("hddma: ");

    copyin(&command, channel, sizeof(command));
    if ((command.opcode >= 0) && (command.opcode <= OP_NOP)) {
        if (trace & trace_hddma) printf("%s\n", cmdname[command.opcode]);
    } else {
        printf("unknown command %d\n", command.opcode);
    }

    drv= command.seldir & DRV_MASK;
    drivefd = drive[drv];
    steps = command.step_low + (command.step_high << 8);
    dmaaddr = command.dma_low + (command.dma_mid << 8) + (command.dma_high << 16);
    link = command.link_low + (command.link_mid << 8) + (command.link_high << 16);
    secsize = secsz[drv];
    // special boot hack
    if ((command.opcode == OP_READ) && 
        (drv == 0) && (command.sec == 0) && 
        (command.hd == 0) && (track[drv] == 0)) {
        secsize = 1024;
    }
    head = ((command.selhd & HEAD_MASK) >> HEAD_SHIFT) ^ HEAD_CMP;
    if (trace & trace_hddma) {
        printf("drive: %d track: %d step: %d %s head: %x %s%s",
            drv, track[drv], steps, command.seldir & STEP_DOWN ? "down" : "up", 
            head, command.selhd & LOW_CURR ? "lowcurr " : "", 
            command.selhd & PRECOMP ? "precomp " : "");
        printf("args %x %x %x %x ", command.arg0, command.arg1, command.arg2, command.arg3);
        printf("dmaaddr: 0x%x link 0x%x secsize %d\n", dmaaddr, link, secsize);
    }

    if (command.seldir & STEP_DOWN) {
        if (track[drv] >= steps) {
            track[drv] -= steps;
        } else {
            track[drv] = 0;
        }
    } else {
        track[drv] += steps;
    }
    i = command.cyl_low + (command.cyl_high << 8);
    offset = secoff(command.sec, command.hd, track[drv]);
 
    switch (command.opcode) {
    case OP_READ:
        if (track[drv] != i) {
            printf("track lossage %d != %d\n", track[drv], i);
        }
        lseek(drivefd, offset, SEEK_SET);
        read(drivefd, &secbuf, secsize);
        copyout(&secbuf, dmaaddr, secsize);
        if (trace & V_BIO) hexdump(secbuf, secsize);
        command.status = GOOD;
        break;
    case OP_WRITE:
        if (track[drv] != i) {
            printf("track lossage %d != %d\n", track[drv], i);
        }
        lseek(drivefd, offset, SEEK_SET);
        copyin(&secbuf, dmaaddr, secsize);
        if (trace & V_BIO) hexdump(secbuf, secsize);
        write(drivefd, &secbuf, secsize);
        command.status = GOOD;
        break;
    case OP_RHEAD:
        command.status = GOOD;
        break;
    case OP_FMT:                                // format a whole track
        secsize = FSECSIZE(command.fseccode);
        if (trace & trace_hddma)
            printf("gap3: %d scnt: %d secsize: %d fill: %x\n",
                command.gap3, command.sptneg ^ 0xff, secsize, command.fill);
        for (i = 0; i < secsize; i++) {
            secbuf[i] = command.fill;
        }
        for (i = 0; i < (command.sptneg ^ 0xff); i++) {
            offset = secoff(i, head, track[drv]);
            lseek(drivefd, offset, SEEK_SET);
            write(drivefd, &secbuf, secsize);
        }
        command.status = GOOD;
        break;
    case OP_SPEC:
        secsize = SSECSIZE(command.sseccode);
        if (trace & trace_hddma) {
            printf("steprate: %d ms ", command.steprate & 0x7f);
            if (command.steprate & INTERRUPT) {
                printf("interrupt requested ");
            }
            printf("settle: %d ms ", command.settle & 0x7f);
            printf("secsize %d\n", secsize);
        }
        secsz[drv] = secsize; 
        command.status = GOOD;
        break;
    case OP_SENSE:
        command.status = SS_DONE | SS_WFLT;
        if (track[drv] != 0) {
            command.status |= SS_TRK0;
        }
        if (trace & trace_hddma)
            printf("sense: %x %s\n", command.status, bitdef(command.status ^ 0xff, sense_b));
        break;
    case OP_NOP:
        command.status = GOOD;
        break;
    default:
        command.status = BADCMD;
        break;
    }
    copyout(&command, channel, sizeof(command));
#ifdef notdef
    if (enable_intr) {
        irq(HDDMA_INTR);
    }
#endif
    channel = link;
}

/*
 * reset the hd-dma
 */
static void
reset(portaddr p, byte v)
{
    if (trace & trace_hddma)
        printf("hddma: reset\n");
    channel_reset = 1;
}

static void
open_drive(int id)
{
    int fd;
    char drivename[20];

    sprintf(drivename, "hddma-%d", id);
    fd = open(drivename, O_CREAT|O_RDWR, 0777);
    /* lseek(fd, secoff(SPT, HEADS, TRACKS), SEEK_SET);
    write(fd, &fd, 1);
    */
    drive[id] = fd;
}

     
static int
hddma_init()
{
    int i;

	register_output(HDDMA_PORT, &attention);
	register_output(HDDMA_RESET, &reset);
    for (i = 0; i < DRIVES; i++) {
        open_drive(i);
    }
    return 0;
}

/*
 * this grammar makes the compiler call this function before main()
 * this means we can add drivers by just adding them to the link
 */
__attribute__((constructor))
void
register_hddma_driver()
{
    trace_hddma = register_trace("hddma");
    register_startup_hook(hddma_init);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
