/*
 * the HDCA backend: the port-mapped Winchester controller
 *
 * cmd/format/hdca.c
 *
 * The HDCA is the controller of the M10, M20 and M26.  It has no command
 * block of its own, so a program that wants to talk to it does so through
 * the driver's raw ioctl (include/sys/ioctl.h's CDBCMD door): a block of
 * opcode and head/track/sector, with the data phase in a buffer.
 *
 * A formatted drive carries its geometry in its sector headers: each
 * header is four bytes, head, track, sector, key.  The probe reads them
 * to find the drive's shape - step out one track at a time until a
 * header stops matching, select each head until one stops matching, and
 * read a whole revolution of headers to find the highest sector number.
 *
 * A format writes those headers and nothing else, so the data fields -
 * and the label in block 0 - survive it.  That is why the label write
 * below keeps a carved drive's table: reformatting headers is not the
 * same as blanking a disk.
 */

#include <types.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/fs.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/dlabel.h>
#include <sys/hdca.h>
#include "format.h"

/*
 * The drives this controller reached, and their shape.  These are the
 * numbers FORMATHD.COM carried: the M26 is 202 tracks of 8 heads of 32
 * sectors, the M10 is 244 of 4 of 21, and the M20 is the M10's geometry
 * with the M26's eight heads.
 */
struct model {
    char *name;
    char *mfg;
    char *model;
    UINT cyl;
    UINT8 heads;
    UINT8 spt;
};

static struct model models[] = {
    {"m10", "", "M10", 244, 4, 21},
    {"m20", "", "M20", 244, 8, 21},
    {"m26", "", "M26", 202, 8, 32},
};

#define	NMODEL (sizeof models / sizeof models[0])

extern int errno;

/*
 * The physical interleave: the order the controller writes the sector
 * headers in.  FORMATHD's two tables, one for each sector count the
 * drives come in; a count neither knows is written sequential.
 */
static UINT8 skewm26[] = {
    22, 12, 2, 23, 13, 3, 24, 14, 4, 25, 15, 5, 26, 16, 6,
    27, 17, 7, 28, 18, 8, 29, 19, 9, 30, 20, 10, 31, 21, 11, 32, 1
};

static UINT8 skewm10[] = {
    8, 15, 2, 9, 16, 3, 10, 17, 4, 11,
    18, 5, 12, 19, 6, 13, 20, 7, 14, 21, 1
};

/*
 * Run one command block: an opcode and the head, track and sector to
 * seek to, with the data phase, when there is one, through buf.  Returns
 * the status the driver answered in cmd[HDLEN], or -1 if the ioctl
 * itself failed.
 */
static int
run(op, head, track, sector, count, flags, buf)
    int op, head, track, sector, count, flags;
    char *buf;
{
    struct cdb r;
    int st;
    r.addrbits = 0;

    memset((char *) &r, 0, sizeof r);
    r.cmd[0] = op;
    r.cmd[1] = head;
    r.cmd[2] = track;
    r.cmd[3] = sector;
    r.len = HDLEN;
    r.count = count;
    r.flags = flags;
    r.buf = (UINT8 *) buf;
    r.hole = -1;

    if (ioctl(devfd, CDBCMD, &r) < 0)
        return (-1);
    st = r.cmd[HDLEN] & 0377;
    return (st);
}

static
readhdr(head, track, hd)
    int head, track;
    char *hd;
{
    return (run(HDCA_READHDR, head, track, 0, 4, CDB_IN, hd));
}

/*
 * Probe the drive's shape by reading its sector headers.
 */
static
probe()
{
    char hd[4];
    int cyln, headn, firstsec, maxsec;
    unsigned long blocks;

    if (run(HDCA_HOME, 0, 0, 0, 0, 0, 0))
        die("cannot home the drive");

    for (cyln = 0; ; cyln++) {
        if (readhdr(0, cyln, hd) || hd[1] != cyln)
            break;
    }
    if (cyln == 0)
        die("no sector headers: the drive is not formatted");

    if (run(HDCA_HOME, 0, 0, 0, 0, 0, 0))
        die("cannot home the drive");

    for (headn = 0; ; headn++) {
        if (readhdr(headn, 0, hd) || hd[0] != headn)
            break;
    }

    readhdr(0, 0, hd);
    firstsec = hd[2];
    maxsec = firstsec;
    for (;;) {
        readhdr(0, 0, hd);
        if (hd[2] > maxsec)
            maxsec = hd[2];
        if (hd[2] == firstsec)
            break;
    }

    blocks = (unsigned long) cyln * headn * maxsec;
    printf("%s: %d cylinders, %d heads, %d sectors/track, "
        "%lu blocks of 512 bytes\n",
        devname, cyln, headn, maxsec, blocks);
}

/*
 * The sector number the header written at position i carries.
 */
static
skewat(i)
    int i;
{
    if (spt == 32)
        return (skewm26[i]);
    if (spt == 21)
        return (skewm10[i]);
    return (i + 1);
}

/*
 * Format the drive: one FORMATTRACK per head of one track, across the
 * whole geometry.  This builds the whole header table - head, track,
 * sector and key for each sector, in the interleave order - and hands it
 * to the driver in one data phase.
 */
static
format_drive()
{
    UINT track, head;
    char hdr[32 * 4];
    int i, key;

    if (run(HDCA_HOME, 0, 0, 0, 0, 0, 0))
        die("cannot home the drive");

    printf("%s: formatting %d cylinders, %d heads, %d sectors\n",
        devname, cyl, heads, spt);

    for (track = 0; track < cyl; track++) {
        for (head = 0; head < heads; head++) {
            key = (track == 0) ? 0x80 : 0;
            for (i = 0; i < spt; i++) {
                hdr[i * 4 + 0] = head;
                hdr[i * 4 + 1] = track;
                hdr[i * 4 + 2] = skewat(i);
                hdr[i * 4 + 3] = key;
            }
            if (run(HDCA_FORMAT, head, track, spt, spt * 4, 0, hdr)) {
                fprintf(stderr, "format failed at track %d head %d, errno %d\n",
                    track, head, errno);
                die("format failed");
            }
        }
        printf("track %d\n", track);
    }
    printf("formatted %d cylinders\n", cyl);
}

void
hdca_main()
{
    struct model *mp, *row;

    if (opts & O_PROBE) {
        probe();
        return;
    }

    mp = 0;
    mfg = model = 0;
    cyl = heads = spt = 0;

    if (opts & O_MODEL) {
        for (row = models; row < &models[NMODEL]; row++)
            if (strcmp(omodel, row->name) == 0)
                break;
        if (row == &models[NMODEL])
            die("no such drive model");
        mp = row;
    }

    if (mp) {
        cyl = mp->cyl;
        heads = mp->heads;
        spt = mp->spt;
        mfg = mp->mfg;
        model = mp->model;
    } else if (getlabel() != 0) {
        cyl = lcyl;
        heads = lheads;
        spt = lspt;
    }
    if (opts & O_CYL)
        cyl = ocyl;
    if (opts & O_HEADS)
        heads = oheads;
    if (opts & O_SPT)
        spt = ospt;

    if (cyl == 0 || heads == 0 || spt == 0)
        die("no geometry: name a model with -m, or give -c and -h");
    if (cyl > 255 || heads > 8 || spt > 32)
        die("that geometry does not fit the HDCA");

    format_drive();
    putlabel(1, 0, 1);

    printf("label: %d cylinders, %d heads, %d sectors, slices a b d e f g h\n",
        cyl, heads, spt);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
