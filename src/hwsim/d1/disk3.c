/*
 * the CompuPro DISK 3 - an intelligent ST-506 controller, the HD-DMA's
 * shape with an 8085 instead of an 8x300.  The host fills a sixteen byte
 * IOPB in memory and writes the attention port; the board copies the
 * IOPB, runs the command, moves any data to or from the address the IOPB
 * names, and writes the status back.  Everything is done synchronously,
 * in response to the output instruction, exactly as hddma.c does it.
 *
 * The attention port is write-only and the byte written decides what
 * happens: 0xff holds the 8085 in reset, and 0x00 is the attention pulse.
 * The first attention after a reset fetches an initial IOPB from physical
 * 0x50 and keeps only its LINK; later attentions fetch the IOPB the
 * previous LINK named - for the single-IOPB driver, the IOPB itself.
 */

#include "sim.h"
#include "hwsim.h"
#include "vimap.h"
#include "util.h"
#include "imd.h"
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

static int disk3_vi_line = -1;  /* the VI line, from the -w wiring; -1 = not wired */

#define DISK3_PORT  0x90            /* the attention port (0x90 and 0x91) */

#define DRIVES  4
static void *handle[DRIVES];        /* the imd handle per drive */
static int secsize[DRIVES];         /* bytes per sector, from SPECIFY */
static int sectrk[DRIVES];          /* sectors per track, from SPECIFY */
static int heads[DRIVES];           /* heads per cylinder, from SPECIFY */
static int curcyl[DRIVES];          /* where the head is, from HOME/SEEK */

#ifndef NODEBUG
int trace_disk3;
#endif

/*
 * The IOPB the board reads out of memory - sixteen bytes, the whole of
 * the interface, matching include/sys/disk3.h.
 */
struct d3_iopb {
    byte command;
    byte status;
    byte drive;
    byte arg[7];
    byte dma_low, dma_mid, dma_high;
    byte link_low, link_mid, link_high;
} iopb;

/*
 * The command byte: the opcode is the low six bits, bit 7 asks for an
 * interrupt, bit 6 chains to the next IOPB.
 */
#define D3_CONTINUE 0x40
#define D3_INT      0x80

#define D3_NOOP     0x00
#define D3_VERSION  0x01
#define D3_GLOBAL   0x02
#define D3_SPECIFY  0x03
#define D3_SETMAP   0x04
#define D3_HOME     0x05
#define D3_SEEK     0x06
#define D3_READHDR  0x07
#define D3_RW       0x08
#define D3_RELOCATE 0x09
#define D3_FORMAT   0x0a
#define D3_FMTBAD   0x0b
#define D3_STATUS   0x0c
#define D3_SELECT   0x0d
#define D3_EXAMINE  0x0e
#define D3_MODIFY   0x0f

#define D3_OK       0xff
#define D3_NOTRDY   0x02
#define D3_ARGERR   0x01
#define D3_BADCMD   0xfe

#define D3_ABS      0xff            /* the GLOBAL mode: absolute sectors */
#define D3_READ     1               /* the R/W direction */
#define D3_WRITE    0

static int reset_pending = 1;       /* 1: the next attention is the first */
static paddr iopb_addr;             /* where the IOPB is fetched from */
static byte secbuf[4096];
static byte d3_enable_intr;

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
    sprintf(drivename, "disk3-%d", id);
    handle[id] = imd_load(drive_resolve(drivename), id, IMD_INPLACE);
    if (!handle[id]) {
        printf("disk3: no drive %s, command refused\n", drivename);
        return -1;
    }
    imd_trkinfo(handle[id], 0, 0, &nsec, &ss);
    if (nsec == 0) {
        printf("disk3: no drive %s, command refused\n", drivename);
        handle[id] = 0;         /* an empty image is not a drive; don't cache it */
        return -1;
    }
    secsize[id] = ss ? ss : 512;
    return 0;
}

/*
 * The SPECIFY table, read from the IOPB's DATA address: eleven two-byte
 * fields, low byte first.
 */
static void
d3_specify(paddr where, int drv)
{
    static unsigned char sp[22];
    int i;

    copyin(sp, where, sizeof(sp));
    sectrk[drv] = sp[6] | (sp[7] << 8);
    heads[drv] = sp[8] | (sp[9] << 8);
    secsize[drv] = sp[4] | (sp[5] << 8);
    for (i = 0; i < 22; i++)
        sp[i] = 0;                  /* not needed, keep ccc quiet */
}

/*
 * Run one IOPB.  The board copies it out of memory, does the command, and
 * copies it back with the status - so this reads, acts, and writes back.
 */
static void
d3_run(void)
{
    byte drv;
    byte op;
    paddr dma;
    paddr link;
    unsigned long abs;
    int cyl, head, sec;
    int n;
    int i;
    int refused;
    int raise;

    copyin((byte *)&iopb, iopb_addr, sizeof(iopb));
    drv = iopb.drive & 3;
    op = iopb.command & 0x3f;
    dma = iopb.dma_low | (iopb.dma_mid << 8) | (iopb.dma_high << 16);
    link = iopb.link_low | (iopb.link_mid << 8) | (iopb.link_high << 16);

    trace(trace_disk3, "disk3: op %x drv %d dma %06x link %06x\n",
          op, drv, dma, link);
    l("d3: op %x cmd %02x drv %d dma %06x link %06x at %06x\n",
      op, iopb.command, drv, (unsigned)dma, (unsigned)link,
      (unsigned)iopb_addr);

    refused = 0;
    if (op != D3_GLOBAL && op != D3_NOOP && op != D3_VERSION &&
        select_drive(drv) < 0) {
        iopb.status = D3_NOTRDY;
        refused = 1;
        goto done;
    }

    switch (op) {
    case D3_NOOP:
        iopb.status = D3_OK;
        break;
    case D3_VERSION:
        iopb.arg[0] = 0x44;         /* "D" revision */
        iopb.status = D3_OK;
        break;
    case D3_GLOBAL:
        /* ARG1 is the mode; the driver runs absolute.  Nothing else to
         * keep: the board already knows how to address sectors. */
        d3_enable_intr = (iopb.command & D3_INT) ? 1 : 0;
        iopb.status = D3_OK;
        break;
    case D3_SPECIFY:
        d3_specify(dma, drv);
        iopb.status = D3_OK;
        break;
    case D3_HOME:
        curcyl[drv] = 0;
        iopb.status = D3_OK;
        break;
    case D3_SEEK:
        curcyl[drv] = iopb.arg[0] | (iopb.arg[1] << 8);
        iopb.status = D3_OK;
        break;
    case D3_RW:
        abs = (unsigned long)iopb.arg[1] | ((unsigned long)iopb.arg[2] << 8) |
              ((unsigned long)iopb.arg[3] << 16) | ((unsigned long)iopb.arg[4] << 24);
        n = iopb.arg[5] | (iopb.arg[6] << 8);
        if (n == 0)
            n = 1;
        /* the transfer is one buffer, so at most a few sectors */
        if (n > (int)sizeof(secbuf) / 512)
            n = sizeof(secbuf) / 512;
        if (sectrk[drv] == 0 || heads[drv] == 0) {
            iopb.status = D3_ARGERR;
            break;
        }
        for (i = 0; i < n; i++) {
            cyl = (int)(abs / (unsigned long)(heads[drv] * sectrk[drv]));
            head = (int)((abs / (unsigned long)sectrk[drv]) % heads[drv]);
            sec = (int)(abs % (unsigned long)sectrk[drv]);
            if (iopb.arg[0] == D3_READ) {
                imd_read(handle[drv], cyl, head, sec, (char *)&secbuf[i * 512]);
                copyout(&secbuf[i * 512], dma + (unsigned long)i * 512, secsize[drv]);
            } else {
                copyin(&secbuf[i * 512], dma + (unsigned long)i * 512, secsize[drv]);
                imd_write(handle[drv], cyl, head, sec, (char *)&secbuf[i * 512]);
            }
            abs++;
        }
        iopb.status = D3_OK;
        break;
    case D3_READHDR:
        cyl = curcyl[drv];
        head = 0;                   /* the driver does not use this yet */
        sec = 0;
        secbuf[0] = 0xa1;
        secbuf[1] = 0xfe;
        secbuf[2] = cyl & 0xff;
        secbuf[3] = (cyl >> 8) & 0xff;
        secbuf[4] = head;
        secbuf[5] = sec;
        copyout(secbuf, dma, 6);
        iopb.status = D3_OK;
        break;
    case D3_STATUS:
        iopb.arg[0] = 0xff;         /* drive ready, seek complete */
        iopb.status = D3_OK;
        break;
    case D3_SELECT:
    case D3_FORMAT:
    case D3_FMTBAD:
    case D3_SETMAP:
    case D3_RELOCATE:
    case D3_EXAMINE:
    case D3_MODIFY:
    default:
        iopb.status = D3_BADCMD;
        break;
    }

done:
    copyout((byte *)&iopb, iopb_addr, sizeof(iopb));
    iopb_addr = link;
    l("d3: status %02x\n", iopb.status);
    /*
     * A command for a drive that is not there is not a completion: the
     * unit does not answer, and the driver's watchdog is what turns its
     * silence into an error.  Raising the line would hold it (the card
     * keeps it until the next attention, which the driver has no reason
     * to send) and, on a shared VI line, silence every other card on it.
     */
    raise = !refused && (iopb.command & D3_INT) &&
            !(iopb.command & D3_CONTINUE) &&
            op > D3_GLOBAL;             /* a drive-requiring command finished */
    set_vi(disk3_vi_line, 1, raise ? 1 : 0);
    l("d3: vi %d raise %d\n", disk3_vi_line, raise);
}

/*
 * The attention port.  0xff holds reset; anything else is an attention.
 * The first attention after a reset fetches the initial IOPB from 0x50
 * and keeps only its LINK; later ones fetch from the previous LINK.
 */
static void
attention(portaddr p, byte v)
{
    set_vi(disk3_vi_line, 1, 0);

    if (v == 0xff) {
        reset_pending = 1;
        trace(trace_disk3, "disk3: reset\n");
        return;
    }

    if (reset_pending) {
        reset_pending = 0;
        iopb_addr = physread(0x5d) | (physread(0x5e) << 8) |
                    (physread(0x5f) << 16);
        trace(trace_disk3, "disk3: initial link %06x\n", iopb_addr);
        l("d3: initial link %06x\n", (unsigned)iopb_addr);
        return;
    }
    l("d3: attention %02x\n", v);

    d3_run();
}

static int
disk3_init()
{
    disk3_vi_line = wiring_vi_line("disk3");
    register_output(DISK3_PORT, &attention);
    register_output(DISK3_PORT + 1, &attention);
    return 0;
}

static int
disk3_setup()
{
#ifndef NODEBUG
    trace_disk3 = register_trace("disk3");
#endif
    return 0;
}

struct driver disk3_driver = {
    "disk3",
    0,
    &disk3_setup,
    &disk3_init,
    0
};

__attribute__((constructor))
void
register_disk3_driver()
{
    register_driver(&disk3_driver);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
