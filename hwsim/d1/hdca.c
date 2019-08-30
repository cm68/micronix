/*
 * the hdca is the port-mapped, non-dma hard drive controller
 * used in the M10, M20 and M26
 */

#include "sim.h"
#include "util.h"
#include <unistd.h>
#include <strings.h>
#include <fcntl.h>
#include <stdio.h>

/*
 * drive geometry
 * we don't really use this, it's just for show.
 * the emulation uses files that are 244 tracks of 8 head of 32 sectors.
 * if we have an m20, we can't read it as an m26, etc.
 */
struct drivetype {
    char *name;
    int tracks;
    int heads;
    int secpertrack;
    int sectors;
} drives[] = {
    { "M26", 202, 8, 32, 51712 },
    { "M20", 244, 8, 21, 40992 },
    { "M10", 244, 4, 21, 20496 }
};

extern int trace_bio;
int trace_hdca;

#define SPT     32      // like an m26
#define HEADS   8       // like m26 and m20    
#define TRACKS  244     // like M10 and M20
#define SECLEN  512     // fixed sector size

#define secoff(s, h, t) \
    (512 * (((((t) * HEADS) + (h)) * SPT) + (s)))

#define DRIVES  4

static int drive[DRIVES];       // file descriptor for data file
static int track[DRIVES];       // head location

/*
 * the hdca has 1k of ram, 512 for data, and 512 for headers
 * there's a ram pointer that increments, etc.
 * for some silly reason, read data fills at +2 and the ram pointer
 * wraps around at 512. I think it has to do with the checksum timing.
 * wierdly, too, after a read, the pointer is reset to 0. this has
 * the effect of subsequent reads from the buffer getting the last 2
 * bytes read from disk.
 */
byte buffer[1024];
int ramptr;
int rampage;
#define SECTOR  0
#define HEADER  512

/*
 * port numbers and functions
 */
#define	HDCA_CONTROL 	0x50        // write control
#define	HDCA_CMD        0x51        // write command
#define	HDCA_FUNC       0x52        // write selects/step
#define HDCA_DATA       0x53        // write data

#define	HDCA_PSR     	0x50        // primary status
#define	HDCA_SSR     	0x51        // secondary status
#define	HDCA_INT     	0x52        // interrupt clear

// 0x50 - HDCA control port
#define CONT_FRENBL     0x01        // enable drive select and function register
#define CONT_RUN        0x02        // controller run enable
#define CONT_DSKCLK     0x04        // use drive clock
#define CONT_WPROT      0x08        // enable disk writes
static byte control;

// 0x50 - primary status register
#define PSR_NTRK0       0x01        // 0 if track 0
#define PSR_OPDONE      0x02        // op done
#define PSR_COMPLT      0x04        // all seeks complete
#define PSR_TIMOUT      0x08        // time out
#define PSR_NFAULT      0x10        // not fault
#define PSR_NREADY      0x20        // not ready
#define PSR_ILEVEL      0x40        // index toggle
#define PSR_HALT        0x80        // not busy
static byte psr;

// 0x51 - HDCA command register
#define W1_RAMDATA  0       // reset ram pointer to data
#define W1_READSEC  1       // read a sector
#define W1_READHDR  3       // read a sector header
#define W1_WRITESEC 5       // write a sector
#define W1_WRITEHDR 7       // write a sector header
#define W1_RAMHDR   8       // reset ram pointer to header
static byte cmd;

// 0x51 - secondary status register
#define R1_REV      0xc0    // board revision
#define R1_DATACRC  0x02    // data crc error
#define R1_SDONE    0x01    // command done (irq?)
static byte ssr;

// 0x52 HDCA function register
#define FUNC_DRV0   0x01    // drive select 0
#define FUNC_DRV1   0x02    // drive select 1
#define FUNC_STEP   0x04    // step
#define FUNC_DIR    0x08    // direction
#define FUNC_HEAD0  0x10    // head select 0
#define FUNC_HEAD1  0x20    // head select 1
#define FUNC_HEAD2  0x40    // head select 2
#define FUNC_HEAD3  0x80    // head select 3
#define FUNC_DRIVE      0x03
#define FUNC_HEAD       0xf0
static byte func;

/*
 * we need to make index toggle intelligently.
 * ok, we toggle every read of the status register after a seek until the first write header.
 * then we stop toggling until after we see a write header of sector 1.
 */
#define INDEXTOGGLE     20

int indextoggle = 1;

char *psr_b[] = { "ntrk0", "opdone", "complt", "timout", "nfault", "nready", "ilevel", "halt" };
static byte
rd_hdca_psr(portaddr p)
{
    if (indextoggle) {
        // toggle index
        psr ^= PSR_ILEVEL;
    }
    if (trace & trace_hdca) printf("hdca: input primary status %x %s\n", psr, bitdef(psr, psr_b));
    return psr;
}

static char *control_b[] = { "frenbl", "run", "dskclk", "wprot", 0, 0, 0, 0 };
static void
wr_hdca_control(portaddr p, byte v)
{
    if (trace & trace_hdca) printf("hdca: control -> 0x%x %s\n", v, bitdef(v, control_b));
    if (!(psr & PSR_NFAULT) && (v & CONT_WPROT) && !(control & CONT_WPROT)) {
        psr |= PSR_NFAULT;
    }
    control = v;
    psr = PSR_HALT | PSR_NFAULT | PSR_COMPLT | PSR_OPDONE;
}

static void
open_drive(int id)
{
    int fd;
    char drivename[20];

    sprintf(drivename, "hdca-%d", id);
    fd = open(drivename, O_CREAT|O_RDWR, 0777);
#ifdef notdef
    lseek(fd, secoff(SPT, HEADS, TRACKS), SEEK_SET);
    write(fd, &fd, 1);
#endif
    drive[id] = fd;
}

char secbuf[512];

static void
wr_hdca_cmd(portaddr p, byte v)
{
    int drivefd = drive[func & FUNC_DRIVE];
    int head = buffer[HEADER+0];
    int track = buffer[HEADER+1];
    int sector = buffer[HEADER+2];
    int key = buffer[HEADER+3];
    int offset = secoff(track, head, sector);

    switch (v) {
    case 0: 
        if (trace & trace_hdca) printf("hdca cmd: set ram pointer to sector\n");
        rampage = SECTOR;
        ramptr = 0;
        psr &= ~PSR_OPDONE;
        break;
    case 8: 
        if (trace & trace_hdca) printf("hdca cmd: set ram pointer to header\n");
        rampage = HEADER;
        ramptr = 0;
        psr &= ~PSR_OPDONE;
        break;
    case 1:
        if (trace & trace_hdca) printf("hdca cmd: read sector with header bytes: %d %d %d %d\n",
            track, head, sector, key);
        lseek(drivefd, offset, SEEK_SET);
        read(drivefd, &buffer[2], 510);
        read(drivefd, &buffer[0], 2);
        if (trace & trace_bio) {
            bcopy(&buffer[2], &secbuf[0], 510);
            bcopy(&buffer[0], &secbuf[510], 2);  
            hexdump(secbuf, 512);
        }
        psr |= PSR_OPDONE;
        psr &= ~PSR_HALT;
        rampage = SECTOR;
        ramptr = 0;             // points at the last 2 bytes of the sector!
        break;
    case 3:
        if (trace & trace_hdca) printf("hdca cmd: read header\n");
        break;
    case 5: 
        if (trace & trace_hdca) printf("hdca cmd: write sector with header bytes: %d %d %d %d\n",
            track, head, sector, key);
        lseek(drivefd, offset, SEEK_SET);
        write(drivefd, &buffer, 512);
        if (trace & trace_bio) hexdump(buffer, 512);
        psr |= PSR_OPDONE;
        psr &= ~PSR_HALT;
        break;
    case 7:
        if (trace & trace_hdca) printf("hdca cmd: write header bytes: %d %d %d %d\n",
            track, head, sector, key);
        if (sector == 1) {
            indextoggle = 1;
        } else {
            indextoggle = 0;
        }
        psr |= PSR_OPDONE;
        break;
    default:
        printf("hdca cmd: unknown command\n");
        break;
    }
    if (trace & trace_hdca) printf("hdca: cmd -> 0x%x\n", v);
}

static char *func_b[] = { "drv0", "drv1", "step", "dir", "hd0", "hd1", "hd2", "hd3" };
static void
wr_hdca_func(portaddr p, byte v)
{
    byte dir_lower;
    byte head;
    byte drive;
    byte step;

    if (trace & trace_hdca) printf("hdca: func -> 0x%x %s\n", v, bitdef(v, func_b));
    dir_lower = v & FUNC_DIR ? 1 : 0;
    head = ((v & FUNC_HEAD) >> 4) ^ 0xf;
    drive = (v & FUNC_DRIVE);
    step = (v & FUNC_STEP) ? 1 : 0;
    if (trace & trace_hdca) printf("\tdrive: %d head: %d step %s %d\n", drive, head, dir_lower ? "lower" : "higher", step);
    if ((func & FUNC_STEP) == 0) {
        if (step) {
            if (dir_lower) {
                track[drive]--;
            } else {
                track[drive]++;
            }
            psr |= PSR_COMPLT;
            if (trace & trace_hdca) printf("step drive %d to track %d\n", drive, track[drive]);
            indextoggle = 1;
        }
    }
    psr &= ~PSR_NTRK0;
    if (track[drive] != 0) {
        psr |= PSR_NTRK0;
    }
    func = v;
}

static void
wr_hdca_data(portaddr p, byte v)
{
    // printf("hdca: data (%d) -> 0x%x\n", ramptr, v);
    buffer[rampage + ramptr] = v;
    ramptr = (ramptr + 1) & 0x1ff;
}

static byte
rd_hdca_data(portaddr p)
{
    byte v = buffer[ramptr];
    // printf("hdca: input data (%d) %d\n", ramptr, v);
    ramptr = (ramptr + 1) & 0x1ff;
    return v;
}

char *ssr_b[] = { "sdone", "retry", "r0", "r1", 0, 0, 0, 0 };
static byte
rd_hdca_ssr(portaddr p)
{
    if (trace & trace_hdca) printf("hdca: input secondary status %x %s\n", ssr, bitdef(ssr, ssr_b));
    return ssr;
}

static byte
rd_hdca_intclr(portaddr p)
{
    if (trace & trace_hdca) printf("hdca: input intclr\n");
    return 0xff;
}

static int
hdca_init()
{
    int i;

	register_output(HDCA_CONTROL, &wr_hdca_control);
	register_output(HDCA_CMD, &wr_hdca_cmd);
	register_output(HDCA_FUNC, &wr_hdca_func);
	register_output(HDCA_DATA, &wr_hdca_data);
	register_input(HDCA_PSR, &rd_hdca_psr);
	register_input(HDCA_SSR, &rd_hdca_ssr);
	register_input(HDCA_INT, &rd_hdca_intclr);
	register_input(HDCA_DATA, &rd_hdca_data);

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
register_hdca_driver()
{
    trace_hdca = register_trace("hdca");
    register_startup_hook(hdca_init);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
