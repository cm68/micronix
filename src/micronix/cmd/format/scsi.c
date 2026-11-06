/*
 * the SCSI backend: a target behind the NCR 5380
 *
 * cmd/format/scsi.c
 *
 * A SCSI disk formats itself: the host sends FORMAT UNIT and the target
 * does the work, so there is no track walk here.  What the tool does
 * after that is the part the drive cannot: write the disk label, so the
 * freshly formatted drive can be opened and partitioned.
 *
 * The geometry is the target's own and is not a table's, so there is no
 * model name.  -c, -h and -s say it outright, and without them the
 * geometry already in the label is kept.
 *
 * FORMAT UNIT is six bytes on a real bus - opcode, the LUN and the
 * format options, then four bytes of interleave and control - but the
 * adapter always moves ten, so the block is padded with zeroes.
 */

#include <types.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/fs.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/dlabel.h>
#include <sys/ncr.h>
#include "format.h"

#define	CFORMAT	0x04		/* the SCSI opcode: the drive formats itself */
#define	CDBLEN	10		/* the adapter's fixed command length */

static
format_unit()
{
    struct cdb r;

    memset((char *) &r, 0, sizeof r);
    r.cmd[0] = CFORMAT;
    r.len = CDBLEN;
    r.count = 0;
    r.flags = 0;
    r.buf = 0;
    r.hole = -1;                /* SCSI: the adapter holds the address itself */

    if (ioctl(devfd, CDBCMD, &r) < 0)
        die("the driver refused FORMAT UNIT");
}

void
scsi_main()
{
    mfg = model = 0;
    cyl = heads = spt = 0;

    if (getlabel() == 0)
        cyl = heads = spt = 0;
    if (opts & O_CYL)
        cyl = ocyl;
    if (opts & O_HEADS)
        heads = oheads;
    if (opts & O_SPT)
        spt = ospt;

    if (cyl == 0 || heads == 0 || spt == 0)
        die("no geometry: give -c and -h and -s");
    if (cyl > 0xffff || heads > 0xff || spt > 0xff)
        die("a label cannot hold that geometry");

    printf("%s: formatting target %d, %d cylinders, %d heads, %d sectors\n",
        devname, drive, cyl, heads, spt);

    format_unit();
    putlabel(0, 0, 0);

    printf("label: %d cylinders, %d heads, %d sectors, slices a b d e f g h\n",
        cyl, heads, spt);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
