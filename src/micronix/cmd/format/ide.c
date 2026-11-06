/*
 * the IDE backend: a drive on the IDE card
 *
 * cmd/format/ide.c
 *
 * The IDE card has no format command in this tree's driver - the task
 * file has FORMAT TRACK and FORMAT UNIT, but nothing here sends either -
 * so this backend does the part every formatter ends with: it writes
 * the label.  On a fresh or uncarved drive that is the geometry and the
 * prototype table; on a carved one it is the geometry and the table is
 * left alone.  That is exactly what label -w does for this card, and a
 * command that says it formats a disk should leave it in the same state
 * as the ones that can.
 *
 * The geometry is the drive's own; the card and the driver have no
 * table, and a fresh drive must be told with -c, -h and -s.
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
#include "format.h"

void
ide_main()
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

    putlabel(0, 0, 1);

    printf("%s: the IDE card has no format command; label written: "
        "%d cylinders, %d heads, %d sectors, slices a b d e f g h\n",
        devname, cyl, heads, spt);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
