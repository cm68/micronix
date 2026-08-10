/*
 * the disk label - what a disk says about itself
 *
 * include/sys/dlabel.h
 *
 * A Micronix hard disk is not self describing, and the geometry it needs
 * is not written anywhere on it: sys/mw.c holds a table, specs[], keyed
 * by minor >> 2, and everything - where the superblock is, where the
 * boot area is, how a block becomes a cylinder - comes out of that.  Two
 * programs that disagree about the table do not fail, they read and
 * write different disks while both reporting success.
 *
 * It is worse than it sounds, because sys/mw.c does not put block 0 at
 * cylinder 0.  It rotates by half the cylinder count, so the superblock
 * - filesystem block 1, the thing you would look for first - lives at
 * physical cylinder 76 on a five meg drive and 153 on a ten.  Nothing
 * outside the kernel can find it without already knowing which drive it
 * is looking at.
 *
 * There is exactly one place on the device that can be found knowing
 * nothing at all: cylinder 0, head 0, sector 0, because that is what the
 * rom reads to boot.  So that is where the description goes.  mkfs owns
 * that sector - it reserves the whole cylinder for the boot - and the
 * boot code needs the first half of it, so the label takes the second.
 *
 * What that buys, in the order it matters:
 *
 *	mkfs records the geometry it believed, so a disagreement with
 *	the kernel's table is detectable rather than silent
 *
 *	host tools can read a hard disk image at all, which today they
 *	cannot, without carrying a copy of specs[]
 *
 *	the first level boot can check it before loading anything - it
 *	is already holding this sector
 *
 * The label describes everything relative to cylinder 0, which is the
 * most self description this format can support without changing what a
 * v6 filesystem looks like.
 */

#define DL_OFFSET   256         /* into the boot sector, the second half */

struct dlabel {
    char d_magic[4];            /* DL_MAGIC, readable in a hex dump */
    UINT d_version;             /* of this structure */

    /*
     * The drive, as mkfs understood it.  A reader that disagrees with
     * these has found the wrong drive or the wrong table, and should say
     * so rather than carry on.
     */
    UINT d_tracks;              /* cylinders */
    UINT d_heads;
    UINT d_spt;                 /* sectors per track */

    /*
     * The rotation, written out rather than re-derived, because the
     * deriving is where the mistakes happen.  d_cyl0 is the filesystem
     * block that sits at physical cylinder 0 - block 5236 of 10404 on a
     * five meg - and it is where the boot area begins.
     */
    UINT d_cyl0;
    UINT d_bootblks;            /* how many blocks of it the boot owns */

    /*
     * The rotation itself, so that a reader can map any block and not
     * just find the boot.  sys/mw.c computes
     *
     *	cyl = blk / spc + d_roll, wrapped at d_tracks
     *
     * and today d_roll is tracks >> 1.  It is recoverable from d_cyl0
     * and the geometry, but only by inverting that - which is the step
     * this whole structure exists to stop people doing.  A driver
     * reading its geometry from here needs this field, not a derivation.
     */
    UINT d_roll;

    /*
     * And the filesystem that was made on it.
     */
    UINT d_fsize;               /* blocks in the filesystem */
    UINT d_isize;               /* blocks of inodes */
    UINT d_swap;                /* blocks left at the end, not in the fs */
};

#define DL_MAGIC    "MWDL"      /* Morrow Winchester disk label */
#define DL_VERSION  1

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
