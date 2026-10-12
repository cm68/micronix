/*
 * The disk label, decoded.
 *
 * sys/dlabel.c
 *
 * Every block driver learns a drive the same way - read the label off
 * the disk and believe it - so the decode is here, once.  What a slice
 * means changed in several drivers at once, and a disk read by one
 * driver and written by another is the failure the label exists to
 * prevent.  What stays with each driver is the part that is genuinely
 * its own: the decision to refuse a disk that says nothing.
 */

#include <types.h>
#include <sys/sys.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <sys/dlabel.h>
#include <errno.h>

extern int copy();              /* leaf mem.s: kernel-to-kernel copy */

int
dllabel(cdev, sl, g, table, tm)
    UINT cdev;                  /* the drive's own 'c' device number */
    UINT sl;                    /* the slice this open is bound to */
    register struct dlgeom *g;
    struct slicemap *table;     /* cached slice map, or 0 for none */
    struct dltiming *tm;        /* timing out, or 0 to drop it */
{
    static struct buf *b;
    static struct dlabel lab;
    static char *p;
    static UINT off, len;
    static UINT otracks, oroll;
    static UINT8 oheads, osectors;
    static UINT8 got, i;

    otracks = g->tracks;
    oheads = g->heads;
    osectors = g->sectors;
    oroll = g->roll;

    /*
     * Block 0 of 'c' is reached by the ordinary mapping, and the mapping
     * divides by the geometry - so a caller that knows nothing yet is
     * viewed as one head of one sector for the length of the read.
     * Block 0 is cylinder 0, head 0, sector 0 either way, and it is the
     * only block this view can reach.
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
         * The window is taken to lift the label out of it and given back
         * at once: nothing sleeps and nothing else maps it in between,
         * which is the whole of the window's discipline (uio.c).  Only
         * the label is lifted, not the sector, so the hold is as short
         * as the read was.
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
            g->firstsec = lab.d_firstsec;
            if (tm) {
                tm->step = lab.d_step;
                tm->seek = lab.d_seek;
                tm->precomp = lab.d_precomp;
                tm->lowcur = lab.d_lowcur;
            }
            if (table) {
                for (i = 0; i < NSLICE; i++) {
                    table[i].sm_off = lab.d_slice[i].d_off;
                    table[i].sm_len = lab.d_slice[i].d_len;
                    table[i].sm_roll = lab.d_slice[i].d_roll;
                }
            }
        }
    }
    if (!got) {
        /*
         * No label.  What the caller knows stands, so its geometry (or
         * nothing) is what maps.  A caller that knew nothing is left
         * with spc and maxblk zero, which maps block 0 and nothing else
         * - what a labeler needs, and the only operation a label-less
         * disk permits.
         */
        g->tracks = otracks;
        g->heads = oheads;
        g->sectors = osectors;
        g->roll = oroll;
        if (oheads == 0 || osectors == 0) {
            g->spc = 0;
            g->maxblk = 0;
            return (0);
        }
    }

    /*
     * The slice the minor number named.  Its offset is a cylinder, added
     * after the division that turns a block into a cylinder, so it never
     * has to be a block number.  Its roll is its own, so a slice can put
     * its inode table in the middle of its own platter region.
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
            g->roll = lab.d_slice[sl].d_roll;
        }
        /*
         * A slice that is not carved (length zero) maps nothing, and one
         * naming cylinders the drive does not have is refused rather than
         * wrapped, since wrapping would map the slice onto the front of
         * the drive and say nothing.  The bound is written as a remainder
         * because an offset and a length are both free to be 65535 and
         * their sum is not.
         */
        if (len == 0 || off >= g->tracks || len > g->tracks - off) {
            g->spc = 0;
            u.error = ENXIO;
            return (-1);
        }
    }
    g->cylstart = off;
    g->len = len;

    /*
     * The bound is the slice's and not the drive's, because what is
     * checked against it is a filesystem block and a filesystem lives
     * within one slice.  A slice longer than a block number can count is
     * clamped rather than wrapped - 64K blocks is the most any filesystem
     * can use, so a longer slice has no blocks to lose - and the division
     * guards the multiply that would overflow first.
     */
    g->spc = g->heads * g->sectors;
    if (len > 0xffff / g->spc)
        g->maxblk = 0xffff;
    else
        g->maxblk = len * g->spc - 1;
    return (got);
}

/*
 * Decode one slice of a drive against its cached map: the slice's
 * offset in cylinders, its roll (0 for 'c'), its length, and its highest
 * legal block number.  g is the drive's geometry, table the slice map
 * dllabel() cached.  Returns 0, or ENXIO for a slice that is not carved
 * or runs off the drive.
 */
int
dlslice(g, table, sl, offp, rollp, lenp, maxblkp)
    register struct dlgeom *g;
    struct slicemap *table;
    UINT sl;
    UINT *offp, *rollp, *lenp, *maxblkp;
{
    static UINT off, len, roll, maxblk;

    if (sl == DL_WHOLE) {
        off = 0;
        len = g->tracks;
        roll = 0;
    } else {
        off = table[sl].sm_off;
        len = table[sl].sm_len;
        roll = table[sl].sm_roll;
        if (len == 0 || off >= g->tracks || len > g->tracks - off)
            return (ENXIO);
    }
    if (len > 0xffff / g->spc)
        maxblk = 0xffff;
    else
        maxblk = len * g->spc - 1;
    *offp = off;
    *rollp = roll;
    *lenp = len;
    *maxblkp = maxblk;
    return (0);
}

/*
 * Reload the label out of the buffer being written to block 0 of 'c'
 * and update *g, *table and *tm with what it now says.  Writing the
 * label reloads the partition table - this is that side effect, and it
 * is shared by every block driver, which is why it lives here rather
 * than as a copy in each driver's strategy routine.  Returns 1 when the
 * buffer carried a readable label, 0 otherwise.
 */
int
dlreload(b, g, table, tm)
    struct buf *b;
    struct dlgeom *g;
    struct slicemap *table;
    struct dltiming *tm;
{
    static struct dlabel lab;
    static char *p;
    static UINT8 i;

    p = bhold(b);
    copy(&p[DL_OFFSET], &lab, sizeof lab);
    brel();
    if (lab.d_magic[0] != DL_MAGIC[0] || lab.d_magic[1] != DL_MAGIC[1] ||
        lab.d_magic[2] != DL_MAGIC[2] || lab.d_magic[3] != DL_MAGIC[3] ||
        !lab.d_tracks || !lab.d_heads || !lab.d_spt)
        return (0);
    g->tracks = lab.d_tracks;
    g->heads = lab.d_heads;
    g->sectors = lab.d_spt;
    g->firstsec = lab.d_firstsec;
    if (tm) {
        tm->step = lab.d_step;
        tm->seek = lab.d_seek;
        tm->precomp = lab.d_precomp;
        tm->lowcur = lab.d_lowcur;
    }
    if (table) {
        for (i = 0; i < NSLICE; i++) {
            table[i].sm_off = lab.d_slice[i].d_off;
            table[i].sm_len = lab.d_slice[i].d_len;
            table[i].sm_roll = lab.d_slice[i].d_roll;
        }
    }
    return (1);
}

/*
 * vim: set tabstop=4 shiftwidth=4 expandtab:
 */
