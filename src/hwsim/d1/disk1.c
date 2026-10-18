/*
 * the CompuPro DISK 1 and DISK 1A - an Intel 8272 (NEC uPD765) floppy
 * disk controller behind CompuPro's own glue.  The 8272 is not
 * intelligent: the host writes it a command a byte at a time through its
 * data register, watches its main status register between bytes, and
 * reads the result bytes back the same way.  The glue around it is three
 * registers - a drive select register that also picks the data rate, a
 * status register that reports the chip's interrupt pin, and a motor
 * register that also turns the board's boot EPROM off - plus a 24 bit
 * DMA address register that the chip's data phase follows by itself.
 *
 * So this card is a small state machine hung off the four port block at
 * 0xc0 rather than an IOPB the host fills in memory: every command runs
 * synchronously, in response to the write that completes it, exactly as
 * hddma.c and disk3.c do with their command blocks.
 *
 * The DISK 1A carries both drive banks at this one block, four 8 inch
 * drives and four 5 1/4 inch ones.  The command byte's unit field is two
 * bits, so the bank is the drive select register's data rate bit: the
 * unit names one of four and the register says which four.  Eight
 * drives, 0-3 the 8 inch bank and 4-7 the 5 1/4, as the driver's minor
 * numbering has it.
 */

#include "sim.h"
#include "hwsim.h"
#include "vimap.h"
#include "util.h"
#include "media.h"
#include "imd.h"
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

static int disk1_vi_line = -1;  /* the VI line, from the -w wiring; -1 = not wired */

/*
 * A card that shares its VI line with another needs its own bit in the
 * open collector mask set_vi keeps per line.  The DJ-DMA is the other
 * floppy controller and holds bit 0; this one takes bit 1.
 */
#define DISK1_CARDID    1

#define D1_MSR      0xc0        /* read: main status; write: drive select */
#define D1_DATA     0xc1        /* read/write: the FDC data register */
#define D1_STAT     0xc2        /* read: drive status; write: DMA address */
#define D1_MOTOR    0xc3        /* write: motor and boot EPROM */

/* the main status register */
#define D1_RQM      0x80        /* the data register wants or holds a byte */
#define D1_DIO      0x40        /* ... and it is the chip's byte, not the host's */

/* the drive select register */
#define D1_FIVE     0x20        /* 5 1/4 inch data rate: this is the bank bit */

/* the drive status register */
#define D1_RDY      0x01        /* drive ready */
#define D1_INT      0x80        /* the chip's interrupt pin, latched */

/* the 8272's commands; the low five bits are the opcode */
#define FDC_READTRK 0x02
#define FDC_SPECIFY 0x03
#define FDC_SENSEDRV 0x04
#define FDC_WRITE   0x05
#define FDC_READ    0x06
#define FDC_RECAL   0x07
#define FDC_SENSEI  0x08
#define FDC_READID  0x0a
#define FDC_FORMAT  0x0d
#define FDC_SEEK    0x0f

#define DRIVES  8
#define MAXSECT 1024            /* the most bytes one sector can hold */

static void *handle[DRIVES];    /* the imd handle per drive */
static int curcyl[DRIVES];      /* where the head is */
static int track0[DRIVES];      /* ... and whether that is cylinder 0 */
static int secsize[DRIVES];     /* the bytes a sector holds */
static int bound[DRIVES];       /* the media changer named this drive */
static int fitted;              /* the changer has put media on this card */

static byte secbuf[MAXSECT];

#ifndef NODEBUG
int trace_disk1;
#endif

/*
 * The chip's host side is two phases and nothing else: a command is
 * collected a byte at a time, run when the last one arrives, and its
 * result handed back a byte at a time.  The main status register's RQM
 * is always set here - there is no execution phase the host waits on,
 * because the command runs inside the write that completes it - and DIO
 * says which of the two phases is live.
 */
#define PH_CMD  0
#define PH_RES  1

static byte phase;
static byte cmd[9];             /* the command bytes, as written */
static int cmd_index, cmd_len;  /* how many in, and how many wanted */
static byte res[7];             /* the result bytes, as read */
static int res_index, res_len;

static int intr;                /* the chip's interrupt pin, latched */
static byte dsr;                /* the drive select register, last written */
static byte motor;              /* the motor register, for the trace */
static int seek_drv;            /* the drive the last SEEK or RECAL named */
static int seek_end;            /* a seek has finished, unanswered */
static long dma_addr;           /* the 24 bit DMA address, MSB written first */

/*
 * The chip's interrupt pin.  It is level: asserted when a command
 * finishes, dropped when the result is read or a new command starts, so
 * that the next completion is a fresh edge for the 8259 behind it.
 */
static void
d1_intr(void)
{
    set_vi(disk1_vi_line, DISK1_CARDID, intr ? 1 : 0);
}

static int select_path(int id, char *path);

/* the media changer's end of the same thing: a path, not a name we made up */
static int
disk1_media_load(int unit, char *path)
{
    bound[unit] = 1;            /* ours, so select_path does not report it */
    if (select_path(unit, path) < 0)
        return -1;
    fitted = 1;
    curcyl[unit] = 0;
    track0[unit] = 1;
    return 0;
}

static void
disk1_media_eject(int unit)
{
    if (handle[unit]) {
        imd_close(handle[unit]);
        handle[unit] = 0;
    }
    bound[unit] = 1;            /* ... and it stays empty until told otherwise */
}

static struct mediadev disk1_media = {
    "disk1", 0, DRIVES, disk1_media_load, disk1_media_eject
};

/*
 * open a drive's backing store.  A floppy wants the write overlay, not
 * the hard disks' in-place runs: a diskette image is small, its framing
 * must not be disturbed by a write, and the overlay is where a format's
 * sectors land.
 */
static int
select_path(int id, char *path)
{
    void *np;
    int nsec, ss;

    np = imd_load(path, id, IMD_DELTA);
    if (!np)
        return -1;
    if (handle[id])
        imd_close(handle[id]);
    handle[id] = np;
    imd_trkinfo(handle[id], 0, 0, &nsec, &ss);
    secsize[id] = ss ? ss : 512;
    /*
     * If nobody has bound this drive yet, the name we just made up is a
     * -D host override or the card's own default, and the changer does
     * not know about it: tell it, so a query does not report empty.
     */
    if (!bound[id])
        media_bound(&disk1_media, id, path);
    bound[id] = 1;
    return 0;
}

static int
select_drive(int id)
{
    char drivename[20];

    if (handle[id])
        return 0;
    if (bound[id])
        return -1;              /* the changer said what is here, and it is nothing */
    sprintf(drivename, "disk1-%d", id);
    if (select_path(id, drive_resolve(drivename)) < 0) {
        printf("disk1: no drive %s, command refused\n", drivename);
        return -1;
    }
    return 0;
}

/*
 * The unit field is two bits and the bank is the drive select register,
 * so this is where the eight drives come from.
 */
static int
d1_drive(byte unit)
{
    return (unit & 3) | ((dsr & D1_FIVE) ? 4 : 0);
}

/*
 * The sector size code the chip's commands carry: N, the power of two
 * that makes 128 bytes.  A track that holds 512 byte sectors is N = 2.
 */
static int
d1_code(int secsize)
{
    int n;

    for (n = 0; n < 7 && (128 << n) < secsize; n++)
        ;
    return n;
}

/*
 * The DMA address the chip follows.  The register is written three
 * bytes, most significant first, and the kernel puts a physical address
 * in it as a 4K segment above a 12 bit offset - the same conversion
 * hddma.c makes, and with the same special case: a boot loader hands
 * over a flat 16 bit address, which reads here as a zero top byte.
 */
static paddr
d1_phys(void)
{
    if ((dma_addr >> 16) == 0)
        return dma_addr;
    return ((dma_addr >> 16) << 12) | (dma_addr & 0xfff);
}

/*
 * How long a command is and how many result bytes it leaves.  The 8272's
 * three lengths are all here: the nine byte read and write, the six byte
 * format, and the short ones.
 */
static void
d1_setup(byte op)
{
    switch (op & 0x1f) {
    case FDC_READTRK:
    case FDC_WRITE:
    case FDC_READ:
        cmd_len = 9;
        res_len = 7;
        break;
    case FDC_READID:
        cmd_len = 2;
        res_len = 7;
        break;
    case FDC_FORMAT:
        cmd_len = 6;
        res_len = 7;
        break;
    case FDC_SPECIFY:
        cmd_len = 3;
        res_len = 0;
        break;
    case FDC_SENSEDRV:
        cmd_len = 2;
        res_len = 1;
        break;
    case FDC_RECAL:
        cmd_len = 2;
        res_len = 0;
        break;
    case FDC_SENSEI:
        cmd_len = 1;
        res_len = 2;
        break;
    case FDC_SEEK:
        cmd_len = 3;
        res_len = 0;
        break;
    default:
        cmd_len = 1;
        res_len = 1;
        break;
    }
}

/*
 * A read or a write of one sector, or of the run of sectors from the one
 * asked for through EOT.  The chip finds the sector whose header matches
 * and moves it between the disk and the address the DMA register holds;
 * nothing passes through the data register.
 */
static void
d1_transfer(int write)
{
    int drv, head, cyl, sect, eot, n, nsec, ss, i;
    paddr pa;
    int notready;

    drv = d1_drive(cmd[1]);
    head = (cmd[1] >> 2) & 1;
    cyl = cmd[2];
    sect = cmd[4];
    eot = cmd[6];
    n = 128 << (cmd[5] & 7);
    pa = d1_phys();

    res[3] = cyl;
    res[4] = head;
    res[5] = sect;
    res[6] = cmd[5];

    notready = (select_drive(drv) < 0);
    if (notready) {
        res[0] = 0x08 | (head << 2) | (drv & 3);    /* not ready */
        res[1] = 0;
        res[2] = 0;
        return;
    }
    if (n > MAXSECT || eot < sect) {
        res[0] = 0x40 | (head << 2) | (drv & 3);
        res[1] = 0x04;                              /* no data */
        res[2] = 0;
        return;
    }
    if (cyl != curcyl[drv]) {
        /* the head is not where the command says it is */
        res[0] = 0x40 | (head << 2) | (drv & 3);
        res[1] = 0;
        res[2] = 0x10;                              /* wrong cylinder */
        return;
    }

    imd_trkinfo(handle[drv], cyl, head, &nsec, &ss);
    if (nsec == 0) {
        res[0] = 0x40 | (head << 2) | (drv & 3);
        res[1] = 0x04;                              /* no data */
        res[2] = 0;
        return;
    }

    for (i = sect; i <= eot; i++) {
        if (!write) {
            if (!imd_read(handle[drv], cyl, head, i, (char *)secbuf)) {
                res[0] = 0x40 | (head << 2) | (drv & 3);
                res[1] = 0x04;                      /* no data */
                res[2] = 0;
                return;
            }
            copyout(secbuf, pa, n);
        } else {
            copyin(secbuf, pa, n);
            if (!imd_write(handle[drv], cyl, head, i, (char *)secbuf)) {
                res[0] = 0x40 | (head << 2) | (drv & 3);
                res[1] = 0x02;                      /* not writable */
                res[2] = 0;
                return;
            }
        }
        pa += n;
        if (i > 0xff)
            break;                                  /* the byte is one wide */
    }

    /* the sector number the chip stopped on, and a clean finish */
    res[0] = (head << 2) | (drv & 3);
    res[1] = 0;
    res[2] = 0;
    res[5] = (i > eot) ? eot : i;
    trace(trace_disk1, "disk1: %s drv %d cyl %d head %d sect %d eot %d n %d\n",
        write ? "write" : "read", drv, cyl, head, sect, eot, n);
}

/*
 * Format a whole track.  The address the DMA register holds points at
 * the chip's own table: four bytes per sector - cylinder, head, sector
 * number and size code - laid down in the order they are to be written.
 * Only the sector numbers are of any use to the image, and they are
 * expected to run consecutively from the first.
 */
static void
d1_format(void)
{
    int drv, head, n, sc, fill, i, first;
    paddr pa;
    byte id[4];

    drv = d1_drive(cmd[1]);
    head = (cmd[1] >> 2) & 1;
    n = 128 << (cmd[2] & 7);
    sc = cmd[3];
    fill = cmd[5];
    pa = d1_phys();

    res[3] = curcyl[drv];
    res[4] = head;
    res[6] = cmd[2];

    if (select_drive(drv) < 0 || sc < 1 || sc > 32 || n > MAXSECT) {
        res[0] = 0x08 | (head << 2) | (drv & 3);    /* not ready */
        res[1] = 0;
        res[2] = 0;
        return;
    }

    first = 1;
    for (i = 0; i < sc; i++) {
        copyin(id, pa + i * 4, sizeof(id));
        if (i == 0)
            first = id[2];
        else if (id[2] != first + i)
            trace(trace_disk1, "disk1: format id %d is %d, not %d\n",
                i, id[2], first + i);
    }

    imd_setfmt(handle[drv], curcyl[drv], head, first, sc, n, 1, fill);
    secsize[drv] = n;
    trace(trace_disk1, "disk1: format drv %d cyl %d head %d from %d x %d fill %02x\n",
        drv, curcyl[drv], head, first, sc, fill);

    res[0] = (head << 2) | (drv & 3);
    res[1] = 0;
    res[2] = 0;
    res[5] = first + sc - 1;
}

/*
 * Report the next sector header.  Nothing is moving, so this is the
 * first sector of the track the head is on - and the head itself is the
 * one the command named, because there is no rotation here to find it
 * with.
 */
static void
d1_readid(void)
{
    int drv, head, nsec, ss;
    byte st3;

    drv = d1_drive(cmd[1]);
    head = (cmd[1] >> 2) & 1;
    st3 = (head << 2) | (drv & 3);

    if (select_drive(drv) < 0) {
        res[0] = 0x08 | st3;
        res[1] = res[2] = 0;
        res[3] = res[4] = res[5] = res[6] = 0;
        return;
    }
    imd_trkinfo(handle[drv], curcyl[drv], head, &nsec, &ss);
    res[0] = st3;
    res[1] = 0;
    res[2] = 0;
    res[3] = curcyl[drv];
    res[4] = head;
    res[5] = imd_firstsec(handle[drv], curcyl[drv], head);
    res[6] = d1_code(ss ? ss : 512);
}

/*
 * What a SENSE DRIVE STATUS says about a drive: ready, two sided, write
 * protected and at track 0 are the bits that mean something here.
 */
static void
d1_sensedrv(void)
{
    int drv, head;
    byte st3;

    drv = d1_drive(cmd[1]);
    head = (cmd[1] >> 2) & 1;
    st3 = (head << 2) | (drv & 3);

    if (select_drive(drv) < 0)
        res[0] = st3;
    else {
        if (track0[drv])
            st3 |= 0x10;
        res[0] = st3 | D1_RDY;
    }
}

/*
 * Run the command whose last byte has just arrived.  Every 8272 command
 * ends here, and the ones with no result phase raise their interrupt
 * from here too - except SENSE INTERRUPT STATUS, which is the answer to
 * that interrupt rather than a cause of one.
 */
static void
d1_execute(void)
{
    byte op = cmd[0] & 0x1f;
    int drv;

    switch (op) {
    case FDC_READ:
        d1_transfer(0);
        break;
    case FDC_WRITE:
        d1_transfer(1);
        break;
    case FDC_READTRK:
        d1_transfer(0);         /* a whole track, R through EOT */
        break;
    case FDC_FORMAT:
        d1_format();
        break;
    case FDC_READID:
        d1_readid();
        break;
    case FDC_SENSEDRV:
        d1_sensedrv();
        break;
    case FDC_SPECIFY:
        trace(trace_disk1, "disk1: specify %02x %02x\n", cmd[1], cmd[2]);
        break;
    case FDC_RECAL:
        drv = d1_drive(cmd[1]);
        /*
         * A card with no media on it is a card that is not in the
         * machine: a drive exists in the simulator whether or not a
         * file does (opening an unwritten name makes a blank diskette),
         * so the file cannot be the presence test.  Only the media
         * changer fitting a diskette to one of these drives puts the
         * card on the bus.  The kernel's probe tickles every driver it
         * places, and a phantom card asserting its line would claim one
         * another controller owns - trailing an interrupt level that
         * never drops and so masking that controller's completions.
         */
        if (!fitted || select_drive(drv) < 0) {
            res[0] = 0x08 | (drv & 3);  /* not ready, and no interrupt */
            intr = 0;
            return;
        }
        seek_drv = drv;
        curcyl[drv] = 0;
        track0[drv] = 1;
        seek_end = 1;
        trace(trace_disk1, "disk1: recalibrate drv %d\n", drv);
        break;
    case FDC_SEEK:
        drv = d1_drive(cmd[1]);
        seek_drv = drv;
        curcyl[drv] = cmd[2];
        track0[drv] = (cmd[2] == 0);
        seek_end = 1;
        trace(trace_disk1, "disk1: seek drv %d to %d\n", drv, cmd[2]);
        break;
    case FDC_SENSEI:
        /*
         * The answer to a seek, and the only thing that clears it.  The
         * chip reports the seek-end bit and the unit, then the cylinder
         * the head really landed on.
         */
        res[0] = (seek_end ? 0x20 : 0) | (seek_drv & 3);
        res[1] = curcyl[seek_drv];
        seek_end = 0;
        intr = 0;
        return;                 /* no interrupt of its own */
    default:
        trace(trace_disk1, "disk1: undefined command %02x\n", cmd[0]);
        res[0] = 0x80;          /* invalid command */
        break;
    }

    intr = 1;
}

/*
 * The data register, written: this is a command byte.  The first of a
 * command drops the interrupt pin, so that the completion of this
 * command is a fresh edge rather than a level that never moved.
 */
static void
d1_data_out(portaddr p, byte v)
{
    if (phase != PH_CMD)
        return;                 /* the result has not been read yet */

    if (cmd_index == 0) {
        intr = 0;
        d1_intr();
        d1_setup(v);
    }
    if (cmd_index < (int)sizeof(cmd))
        cmd[cmd_index] = v;
    cmd_index++;
    if (cmd_index < cmd_len)
        return;

    cmd_index = 0;
    d1_execute();
    if (res_len) {
        phase = PH_RES;
        res_index = 0;
        d1_intr();
    } else {
        phase = PH_CMD;
        d1_intr();
    }
}

/*
 * ... and read: this is a result byte.  Reading the last of them drops
 * the interrupt pin, which is what the real chip does.
 */
static byte
d1_data_in(portaddr p)
{
    byte v;

    if (phase != PH_RES)
        return 0xff;

    v = res[res_index++];
    if (res_index >= res_len) {
        phase = PH_CMD;
        intr = 0;
        d1_intr();
    }
    return v;
}

/*
 * The main status register.  RQM is always set: a byte is always either
 * wanted or waiting, because the command runs inside the write that
 * completes it.
 */
static byte
d1_msr_in(portaddr p)
{
    return D1_RQM | ((phase == PH_RES) ? D1_DIO : 0);
}

/*
 * The drive select register.  It is the board's configuration, not a
 * per-command unit select: the unit comes from the command byte, and
 * this decides the data rate (and so the bank) and how the glue drives
 * the cable.
 */
static void
d1_dsr_out(portaddr p, byte v)
{
    dsr = v;
    trace(trace_disk1, "disk1: drive select %02x\n", v);
}

/*
 * The drive status register, read: the chip's interrupt pin as the board
 * copies it, and the first drive's ready line.
 */
static byte
d1_stat_in(portaddr p)
{
    /*
     * The ready line is the selected drive's and there is no selected
     * drive between commands, so it is reported up: what tells a caller
     * that a drive has no media is the command's own not-ready status,
     * which is where the chip gets it from too.
     */
    return (intr ? D1_INT : 0) | D1_RDY;
}

/*
 * The DMA address register, written three bytes most significant first.
 */
static void
d1_dma_out(portaddr p, byte v)
{
    dma_addr = ((dma_addr << 8) | v) & 0xffffff;
    trace(trace_disk1, "disk1: dma address %06lx\n", dma_addr);
}

/*
 * The motor register.  Bit 0 low turns the board's boot EPROM off and
 * only a system reset brings it back; bits 4-7 run the motors.  Nothing
 * here has a motor to stop, so this is kept for the trace alone.
 */
static void
d1_motor_out(portaddr p, byte v)
{
    motor = v;
    trace(trace_disk1, "disk1: motors %02x\n", motor);
}

static int
disk1_init()
{
    disk1_vi_line = wiring_vi_line("disk1");
    register_input(D1_MSR, &d1_msr_in);
    register_output(D1_MSR, &d1_dsr_out);
    register_input(D1_DATA, &d1_data_in);
    register_output(D1_DATA, &d1_data_out);
    register_input(D1_STAT, &d1_stat_in);
    register_output(D1_STAT, &d1_dma_out);
    register_output(D1_MOTOR, &d1_motor_out);
    register_mediadev(&disk1_media);
    return 0;
}

static int
disk1_setup()
{
#ifndef NODEBUG
    trace_disk1 = register_trace("disk1");
#endif
    return 0;
}

struct driver disk1_driver = {
    "disk1",
    0,
    &disk1_setup,
    &disk1_init,
    0
};

__attribute__((constructor))
void
register_disk1_driver()
{
    register_driver(&disk1_driver);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
