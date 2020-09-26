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
 * there are 4 cases:
 *   mon 447 with 512b sectors:
 *   mon 447 with 1024b sectors:
 *   mon 375 with 512b sectors:
 *   mon 375 with 1024b sectors:
 */

#include "sim.h"
#include "util.h"
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

#define HDDMA_INTERRUPT 0

#define	HDDMA_PORT 	0x55	// hddma attention port
#define	HDDMA_RESET	0x54	// hddma reset port

#define	DEF_CCA     0x50	    // hddma default channel command address

#define DRIVES  4
static int curcyl[DRIVES];	// where are we - used for format and seek
static int secsize[DRIVES];   	// sector size from specify/format
static void *handle[DRIVES];	// our handle to the harddisk module

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
     * format is a different story - the arg bytes are used to set formatting information
     */
#define gap3        arg0    // gap3 size
#define sptneg      arg1    // negated sector count
#define fseccode    arg2    // a size code 
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
static byte secbuf[2048];
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

/*
 * let's open the backing store
 */
static void
select_drive(int id)
{
    char drivename[20];

    if (handle[id]) {
    	return;
    }
    sprintf(drivename, "hddma-%d", id);
    handle[id] = drive_open(drivename);
    if (!handle[id]) {
        printf("open of %s failed\n", drivename);
    }
    secsize[id] = drive_sectorsize(handle[id], 0);
}

static char *sense_b[] = { "trk0", "wfault", "ready", "seekcomplete", "index", 0, 0, 0 };
static char *cmdname[] = { "read", "write", "readhead", "format", "specify", "sense", "nop" };
/*
 * start the channel
 */
static void
attention(portaddr p, byte v)
{
    byte status;
    paddr link;
    byte drv;
    int steps;
    int i;
    int head;

    set_vi(HDDMA_INTERRUPT, 1, 0);

    if (channel_reset) {
        channel_reset = 0;
        channel = physread(DEF_CCA) + (physread(DEF_CCA+1) << 8) + (physread(DEF_CCA+2) << 16);
    }

    trace(trace_hddma, "hddma: ");

    copyin((byte *)&command, channel, sizeof(command));

    drv = command.seldir & DRV_MASK;
    select_drive(drv);

    steps = command.step_low + (command.step_high << 8);
    dmaaddr = command.dma_low + (command.dma_mid << 8) + (command.dma_high << 16);
    link = command.link_low + (command.link_mid << 8) + (command.link_high << 16);
    head = ((command.selhd & HEAD_MASK) >> HEAD_SHIFT) ^ HEAD_CMP;

    if (traceflags & trace_hddma) {
    	if ((command.opcode >= 0) && (command.opcode <= OP_NOP)) {
    		Logc("%s ", cmdname[command.opcode]);
		} else {
			Logc("unknown command %d ", command.opcode);
		}
        Logc("drive: %d track: %d step: %d %s head: %x %s%s",
            drv, curcyl[drv], steps, command.seldir & STEP_DOWN ? "down" : "up",
            head, command.selhd & LOW_CURR ? "lowcurr " : "", 
            command.selhd & PRECOMP ? "precomp " : "");
        Logc("args %x %x %x %x ", 
            command.arg0, command.arg1, command.arg2, command.arg3);
        Logc("dmaaddr: 0x%x link 0x%x secsize %d\n", 
            dmaaddr, link, secsize[drv]);
    }

    // do the stepping that can be in every command
    if (command.seldir & STEP_DOWN) {
        if (curcyl[drv] >= steps) {
            curcyl[drv] -= steps;
        } else {
            curcyl[drv] = 0;
        }
    } else {
        curcyl[drv] += steps;
    }
 
    switch (command.opcode) {
    case OP_READ:
        if (curcyl[drv] != (command.cyl_low + (command.cyl_high << 8))) {
            Logc("\ttrack lossage %d != %d\n", curcyl[drv], i);
        }
        i = drive_read(handle[drv], curcyl[drv], command.hd, command.sec, (char *)&secbuf);
        if (i != secsize[drv]) {
        	Logc("\tread sector size mismatch %d expected %d\n", i, secsize[i]);
        }
        copyout(secbuf, dmaaddr, i);
#ifdef notdef
    	if ((drv == 0) && (command.sec == 0) && (command.hd == 0) && (track[drv] == 0)) {
    	// do boot magic here
    	}
#endif
        if (traceflags & trace_bio) hexdump(secbuf, i);
        command.status = GOOD;
        break;
    case OP_WRITE:
        if (curcyl[drv] != (command.cyl_low + (command.cyl_high << 8))) {
            Logc("\ttrack lossage %d != %d\n", curcyl[drv], i);
        }
        copyin(secbuf, dmaaddr, secsize[drv]);
        i = drive_write(handle[drv], curcyl[drv], command.hd, command.sec, (char *)&secbuf);
        if (i != secsize[drv]) {
        	Logc("\twrite sector size mismatch %d expected %d\n", i, secsize[i]);
        }
        if (traceflags & trace_bio) hexdump(secbuf, i);
        command.status = GOOD;
        break;
    case OP_RHEAD:
        command.status = GOOD;
        break;
    case OP_FMT:                                // format a whole track
    	i = FSECSIZE(command.fseccode);
        secsize[drv] = drive_sectorsize(handle[drv], i);
        tracec(trace_hddma, "\tgap3: %d scnt: %d secsize: %d fill: %x\n",
                command.gap3, command.sptneg ^ 0xff, i, command.fill);
        for (i = 0; i < secsize[drv]; i++) {
            secbuf[i] = command.fill;
        }
        for (i = 0; i < (command.sptneg ^ 0xff); i++) {
        	if (drive_write(handle[drv], curcyl[drv], head, i, (char *)secbuf) != secsize[drv]) {
        		Logc("\tformat data fill write error\n");
        	}
        }
        command.status = GOOD;
        break;
    case OP_SPEC:
        enable_intr = (command.steprate & INTERRUPT) ? 1 : 0;
        switch (command.sseccode) {
        case 0: case 1: case 3: case 7: case 0xf:
            break;
        default:
            tracec(trace_hddma, "\tbogus sector size code: %x\n", 
                command.sseccode);
            break;
        }
        command.sseccode = 3;
        i = SSECSIZE(command.sseccode);
        if (traceflags & trace_hddma) {
            Logc("\tsteprate: %d ms ", command.steprate & 0x7f);
            Logc("settle: %d ms ", command.settle & 0x7f);
            Logc("secsize %d\n", i);
        }
        if (i != secsize[drv]) {
        	Logc("\tdrive %d sectorsize mismatch on specify %d expected %d\n", 
                drv, i, secsize[drv]);
        }
        // secsz[drv] = secsize;
        command.status = GOOD;
        break;
    case OP_SENSE:
        command.status = SS_DONE | SS_WFLT;
        if (curcyl[drv] != 0) {
            command.status |= SS_TRK0;
        }
        trace(trace_hddma, "\tsense: %x %s\n", 
            command.status, bitdef(command.status ^ 0xff, sense_b));
        break;
    case OP_NOP:
        command.status = GOOD;
        break;
    default:
        command.status = BADCMD;
        break;
    }
    copyout((byte *)&command, channel, sizeof(command));
    channel = link;
    if (enable_intr) {
        trace(trace_hddma, "\thddma: set interrupt\n");
        set_vi(HDDMA_INTERRUPT, 1, 1);
    } else {
        trace(trace_hddma, "\thddma: no interrupt\n");
    }
}

/*
 * reset the hd-dma
 */
static void
reset(portaddr p, byte v)
{
    trace(trace_hddma, "hddma: reset\n");
    channel_reset = 1;
}
     
static int
hddma_init()
{
    int i;

	register_output(HDDMA_PORT, &attention);
	register_output(HDDMA_RESET, &reset);
    return 0;
}

static int
hddma_setup()
{
    trace_hddma = register_trace("hddma");
    return 0;
}

struct driver hddma_driver = {
    "hddma",
    0,
    &hddma_setup,
    &hddma_init,
    0
};

/*
 * this grammar makes the compiler call this function before main()
 * this means we can add drivers by just adding them to the link
 */
__attribute__((constructor))
void
register_hddma_driver()
{
    register_driver(&hddma_driver);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
