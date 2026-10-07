/*
 * The disk label, decoded.
 *
 * sys/dlabel.c
 *
 * sys/mw.c and sys/ide.c both learn a drive's geometry the same way -
 * read the label off the disk and believe it - and each learned it with
 * its own copy of this code, down to the two ENXIO guards and the block
 * bound at the end.  The duplicated text is the least of it: what a
 * slice means had to be changed in two places, and a disk read by one
 * driver and written by the other is the failure the label exists to
 * prevent.
 *
 * So the decode is here, once.  What stays with each driver is the part
 * that is genuinely its own: mw.c's table of drive models, which stands
 * in for a label the disk does not have, and each driver's own decision
 * to refuse a disk that says nothing when it can fall back on nothing.
 */

#include <types.h>
#include <sys/sys.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <sys/dlabel.h>
#include <errno.h>

extern int copy();              /* leaf mem.s: kernel-to-kernel copy */

int
dllabel(cdev, sl, g, table)
    UINT cdev;                  /* the drive's own 'c' device number */
    UINT sl;                    /* the slice this open is bound to */
    register struct dlgeom *g;
    struct dslice *table;       /* cached slice table, or 0 for none */
{
    static struct buf *b;
    static struct dlabel lab;
    static char *p;
    static UINT off, len;
    static UINT otracks, oroll;
    static UINT8 oheads, osectors;
    static int got, i;

    otracks = g->tracks;
    oheads = g->heads;
    osectors = g->sectors;
    oroll = g->roll;

    /*
     * Block 0 of 'c' is reached by the ordinary mapping, and the
     * mapping divides by the geometry - so a caller that knows nothing
     * yet is viewed as one head of one sector for the length of the
     * read.  Block 0 is cylinder 0, head 0, sector 0 either way, and it
     * is the only block this view can reach, so a table row's timings
     * are never consulted for a step that is never taken.
     */
    g->cylstart = 0;
    g->roll = 0;
    g->maxblk = 0;
    g->heads = oheads ? oheads : 1;
    g->sectors = osectors ? osectors : 1;
    g->spc = g->heads * g->sectors;

    got = 0;
    if ((b = bread(0, cdev)) != 0) {
        /*
         * The window is taken to lift the label out of it and given
         * back at once: nothing sleeps and nothing else maps it in
         * between, which is the whole of the window's discipline
         * (uio.c).  Only the label is lifted, not the sector, so the
         * hold is as short as the read was.
         */
        p = bhold(b);
        copy(&p[DL_OFFSET], &lab, sizeof lab);
        brel();
        brelse(b);

        g->tracks = lab.d_tracks;
        g->heads = lab.d_heads; /* UINT8: a label claiming 256 heads is
                                 * a label claiming none */
        g->sectors = lab.d_spt;
        if (lab.d_magic[0] == DL_MAGIC[0] && lab.d_magic[1] == DL_MAGIC[1]
            && lab.d_magic[2] == DL_MAGIC[2]
            && lab.d_magic[3] == DL_MAGIC[3]
            && g->tracks && g->heads && g->sectors) {
            got = 1;
            g->roll = lab.d_roll;
            g->firstsec = lab.d_firstsec;
            if (table) {
                for (i = 0; i < NSLICE; i++) {
                    table[i].d_off = lab.d_slice[i].d_off;
                    table[i].d_len = lab.d_slice[i].d_len;
                }
            }
        }
    }
    if (!got) {
        /*
         * No label.  What the caller knows stands, against the empty
         * slice table - which is what every disk made before the table
         * existed carries, and what a disk laid down by something that
         * wrote no label is.  An empty entry is offset zero and length
         * zero, and that is one slice covering the whole drive running
         * through the roll: exactly the layout those disks have.
         */
        g->tracks = otracks;
        g->heads = oheads;
        g->sectors = osectors;
        g->roll = oroll;
        if (oheads == 0 || osectors == 0) {
            /*
             * And nothing at all to map with.  'c' is the exception -
             * block 0 of it is device block 0 whatever the geometry
             * turns out to be - but that is the caller's to make, out
             * of maxblk 0 and a slice of its own.
             */
            g->spc = 0;
            g->maxblk = 0;
            return (0);
        }
    }

    /*
     * The slice the minor number named.  Its offset is a cylinder,
     * added after the division that turns a block into a cylinder, so
     * it never has to be a block number - which is the point of it,
     * since a block number is 16 bits and a drive's worth of them is
     * not.  A length of zero is the whole drive from the offset on.
     */
    if (sl == DL_WHOLE) {
        /*
         * 'c' consults no entry: the whole drive from cylinder 0 and
         * never rolled, which is what makes its block 0 device block 0.
         */
        off = 0;
        len = g->tracks;
        g->roll = 0;
    } else {
        off = 0;
        len = 0;
        if (got) {
            off = lab.d_slice[sl].d_off;
            len = lab.d_slice[sl].d_len;
        }
        /*
         * A table naming cylinders the drive does not have is refused
         * rather than wrapped, since wrapping would map the slice onto
         * the front of the drive and say nothing.  The bound is written
         * as a remainder because an offset and a length are both free
         * to be 65535 and their sum is not.
         */
        if (off >= g->tracks || len > g->tracks - off) {
            g->spc = 0;
            u.error = ENXIO;
            return (-1);
        }
        if (len == 0)
            len = g->tracks - off;
    }
    g->cylstart = off;

    /*
     * The bound is the slice's and not the drive's, because what is
     * checked against it is a filesystem block and a filesystem lives
     * within one slice.  A slice longer than a block number can count
     * is clamped rather than wrapped - 64K blocks is the most any
     * filesystem can use, so a longer slice has no blocks to lose - and
     * the division guards the multiply that would overflow first.
     */
    g->spc = g->heads * g->sectors;
    if (len > 0xffff / g->spc)
        g->maxblk = 0xffff;
    else
        g->maxblk = len * g->spc - 1;
    return (got);
}

/*
 * Decode one slice of a drive against its cached table: the slice's
 * offset in cylinders, its roll (0 for 'c'), and its highest legal block
 * number.  g is the drive's geometry, table the slice table dllabel()
 * cached.  Returns 0, or ENXIO for a slice that runs off the drive.  A
 * length of zero is the whole drive from the offset on - the one slice a
 * disk made before slices carries.
 */
int
dlslice(g, table, sl, offp, rollp, maxblkp)
    register struct dlgeom *g;
    struct dslice *table;
    UINT sl;
    UINT *offp, *rollp, *maxblkp;
{
    static UINT off, len, roll, maxblk;

    if (sl == DL_WHOLE) {
        off = 0;
        len = g->tracks;
        roll = 0;
    } else {
        off = table[sl].d_off;
        len = table[sl].d_len;
        roll = g->roll;
        if (off >= g->tracks || len > g->tracks - off)
            return (ENXIO);
        if (len == 0)
            len = g->tracks - off;
    }
    if (len == 0)
        maxblk = 0;
    else if (len > 0xffff / g->spc)
        maxblk = 0xffff;
    else
        maxblk = len * g->spc - 1;
    *offp = off;
    *rollp = roll;
    *maxblkp = maxblk;
    return (0);
}

/*
 * vim: set tabstop=4 shiftwidth=4 expandtab:
 */
