/*
 * the CompuPro DISK 2 - a hard disk controller for SA4000/Fujitsu 2300
 * drives.  It is not intelligent: a finite state machine finds a sector
 * header and moves its data, but the host does the seek itself, one step
 * pulse at a time, and every transfer's address comes from the SELECTOR
 * CHANNEL, which the host programmed just before the command.
 *
 * The host interface is two ports.  The low one is the control register
 * when written and the status when read; the high one is the data port,
 * written to load one of the four data registers and read to step the
 * head.  Everything is synchronous, exactly like hddma.c.
 */

#include "sim.h"
#include "hwsim.h"
#include "util.h"
#include "imd.h"
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

#define D2_CTL   0xc8
#define D2_DATA  0xc9

#define DRIVES  4
static void *handle[DRIVES];
static int secsize[DRIVES];
static int curcyl[DRIVES];

/* the SELECTOR CHANNEL's programmed transfer, from selector.c */
extern paddr sc_addr;

/* the control register bits, from include/sys/disk2.h */
#define D2_RUN    0x40
#define D2_OP2    0x20
#define D2_OP1    0x10
#define D2_OP0    0x08

/* the data registers */
#define R_DRIVE   0x00
#define R_CYL     0x08
#define R_HEAD    0x10
#define R_SECT    0x18

/* the status bits */
#define D2_SATTN  0x80

#ifndef NODEBUG
int trace_disk2;
#endif

static byte regsel;             /* the register OP1/OP0 names */
static byte direction;          /* the step direction, OP2 */
static byte curdrv;             /* the drive the CTL's US1/US0 names */
static byte reg_cyl, reg_head, reg_sect;    /* the header-match registers */
static byte status;             /* the latched status */

static byte secbuf[4096];

/*
 * open a drive's backing store
 */
static int
select_drive(int id)
{
    char drivename[20];
    int nsec, ss;

    if (handle[id])
        return 0;
    sprintf(drivename, "disk2-%d", id);
    handle[id] = imd_load(drive_resolve(drivename), id, IMD_INPLACE);
    if (!handle[id]) {
        printf("disk2: no drive %s, command refused\n", drivename);
        return -1;
    }
    imd_trkinfo(handle[id], 0, 0, &nsec, &ss);
    secsize[id] = ss ? ss : 512;
    return 0;
}

/*
 * Run a command.  op is OP2-OP0 and drive is US1-US0 from the control
 * register.  The board searches for the sector the header registers name
 * and moves it to or from the SELECTOR CHANNEL's address; the seek is the
 * host's and is already done, so the head is where the cylinder register
 * says.
 */
static void
d2_command(byte drive, byte op)
{
    int ss = secsize[drive];

    switch (op) {
    case 0x08:                  /* READ */
        imd_read(handle[drive], curcyl[drive], reg_head, reg_sect, secbuf);
        copyout(secbuf, sc_addr, ss);
        status = 0x03;
        break;
    case 0x10:                  /* WRITE */
        copyin(secbuf, sc_addr, ss);
        imd_write(handle[drive], curcyl[drive], reg_head, reg_sect, secbuf);
        status = 0x03;
        break;
    case 0x00:                  /* NULL - waits for seek complete */
    case 0x18:                  /* WRITE HEADER */
    case 0x20:                  /* READ HEADER */
    default:
        status = 0x03;
        break;
    }
}

/*
 * The control/status port.  Written with RUN set it is a command; written
 * with RUN clear it names a data register and sets the step direction.
 * Read, it is the status.
 */
static void
d2_ctl_out(portaddr p, byte v)
{
    if (v & D2_RUN) {
        curdrv = v & 3;
        if (select_drive(curdrv) < 0)
            status = 0x30;      /* overrun-ish: no drive */
        else
            d2_command(curdrv, v & 0x38);
    } else {
        regsel = v & 0x18;      /* OP1/OP0 */
        direction = v & D2_OP2;
        curdrv = v & 3;
        status = D2_SATTN;      /* transparent: busy */
    }
}

static byte
d2_ctl_in(portaddr p)
{
    return status;
}

/*
 * The data port.  Written, it loads the register the last control write
 * named; read, it sends one step pulse in the programmed direction.
 */
static void
d2_data_out(portaddr p, byte v)
{
    switch (regsel) {
    case R_DRIVE:
        curdrv = (v >> 4) & 3;  /* the drive is the high nibble */
        reg_head = v & 0xf;     /* and the head is the low */
        break;
    case R_CYL:
        reg_cyl = v;
        break;
    case R_HEAD:
        reg_head = v;
        break;
    case R_SECT:
        reg_sect = v;
        break;
    }
}

static byte
d2_data_in(portaddr p)
{
    if (direction & D2_OP2)
        curcyl[curdrv]--;
    else
        curcyl[curdrv]++;
    if (curcyl[curdrv] < 0)
        curcyl[curdrv] = 0;
    return 0;
}

static int
disk2_init()
{
    register_output(D2_CTL, &d2_ctl_out);
    register_input(D2_CTL, &d2_ctl_in);
    register_output(D2_DATA, &d2_data_out);
    register_input(D2_DATA, &d2_data_in);
    return 0;
}

static int
disk2_setup()
{
#ifndef NODEBUG
    trace_disk2 = register_trace("disk2");
#endif
    return 0;
}

struct driver disk2_driver = {
    "disk2",
    0,
    &disk2_setup,
    &disk2_init,
    0
};

__attribute__((constructor))
void
register_disk2_driver()
{
    register_driver(&disk2_driver);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
