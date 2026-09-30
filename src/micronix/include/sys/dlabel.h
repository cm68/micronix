/*
 * the disk label - what a disk says about itself
 *
 * include/sys/dlabel.h
 *
 * A Micronix hard disk used to describe itself nowhere: the geometry came
 * out of a table in the driver, specs[], keyed by the type bits of the
 * minor number, and everything - where the superblock is, where the boot
 * area is, how a block becomes a cylinder - came out of that.  Two
 * programs that disagreed about the table did not fail, they read and
 * wrote different disks while both reporting success.
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
 *	what the driver maps is detectable rather than silent
 *
 *	host tools can read a hard disk image at all, and the first
 *	ones could only do it by carrying a copy of the driver's table
 *
 *	the first level boot can check it before loading anything - it
 *	is already holding this sector
 *
 * The label describes everything relative to cylinder 0, which is the
 * most self description this format can support without changing what a
 * v6 filesystem looks like.
 *
 * What a disk says about its filesystems is a table of slices: eight of
 * them, lettered a through h, each an offset and a length in cylinders.
 * They may overlap and nothing is checked - a slice is a window on the
 * drive, and two filesystems over one region is the operator's error and
 * always was.  The one slice the table does not place is 'c', the whole
 * disk including this label: it starts at cylinder 0 and is never rolled
 * (DL_WHOLE), so its block 0 is the sector the rom reads, on every disk.
 * Block 0 is never part of a filesystem, so the label sits outside every
 * filesystem on the disk and cannot be destroyed by making one.
 *
 * The boot is not a slice but an attribute of the drive, because not
 * every block device wants one: how many blocks below the filesystems it
 * owns, and which slice it loads its filesystem from.  Every disk made
 * before the table existed carries an empty one, and an empty table has
 * no other reading than one slice covering the whole drive - which is
 * d_roll below doing today's exact mapping.  So the old disks need
 * nothing done to them, and no reader branches on d_version: the
 * generation is in the values.
 */

#define DL_OFFSET   256         /* into the boot sector, the second half */
#define NSLICE      8           /* a through h */

/*
 * 'c', the whole drive, and the one slice whose position is not in the
 * table above it: cylinder 0, no roll, to the end of the drive.  So its
 * block 0 is device block 0 - the sector the rom reads, and the one this
 * label is in - and a program holding the drive's 'c' can read the label
 * without knowing anything the label would tell it.  That is what makes
 * the label maintainable: the tool opens hd0c, reads block 0, and has
 * the geometry; without it the tool would need the geometry to find the
 * geometry.
 *
 * Nothing that exists today reads 'c': the minor numbers in use name the
 * filesystems, and a rolled disk's is 'a'.  What changes is what an
 * unwritten 'c' entry means - zero and zero used to be the whole drive
 * rolled like every other entry, and is now the whole drive from
 * cylinder 0.
 */
#define DL_WHOLE    2

/*
 * One slice: a window on the drive, in cylinders.  A length of zero is
 * the whole disk from d_off on, which is what an unwritten entry means
 * and therefore what every disk that predates this table carries.  An
 * offset in cylinders and not in blocks is the point: a block number is
 * 16 bits and a drive's worth of them is not, so a slice's position is
 * added after the division that turns a block into a cylinder and never
 * has to be a block number at all.
 */
struct dslice {
    UINT d_off;                 /* where it starts, in cylinders */
    UINT d_len;                 /* how many cylinders, 0 = to the end */
};

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
     * The boot, written out rather than re-derived, because the deriving
     * is where the mistakes happen.  d_cyl0 is the filesystem block that
     * sits at physical cylinder 0 - block 5236 of 10404 on a five meg -
     * and it is where the boot area begins on a rolled disk; d_bootblks
     * is how many blocks of it the boot owns.
     *
     * The boot belongs to the drive and not to any slice.  Device block 0
     * is the one address that needs no geometry - it is where the first
     * level is and where this label is - and a boot laid out below the
     * filesystems owns blocks from there.  Zero says the drive has no
     * boot, which is an ordinary thing for a data disk to be.
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
     *
     * It stays a field under the slice table as well, and is not
     * vestigial: a rolled disk describes itself with it and an empty
     * table, and a slice's offset is added to it rather than replacing
     * it.  On a sliced disk it is zero.
     */
    UINT d_roll;

    /*
     * And the filesystem that was made on it.
     */
    UINT d_fsize;               /* blocks in the filesystem */
    UINT d_isize;               /* blocks of inodes */
    UINT d_swap;                /* blocks left at the end, not in the fs */

    /*
     * Which slice the boot loads its filesystem from.  Appended with the
     * table below, so a label written before either existed reads zero
     * here, which is slice 'a'.  The boot's extent is d_bootblks above,
     * and nothing inside a filesystem is the boot: the root filesystem
     * holds neither the boot nor this label in band.
     */
    UINT d_bootslice;

    /*
     * The filesystems, a through h.  Appended, so a label written before
     * the table existed reads as all zero, and all zero is one slice
     * covering the drive: the rolled layout above, which is why a disk
     * the older code made needs nothing done to it.
     */
    struct dslice d_slice[NSLICE];

    /*
     * The number of the first sector on a track, which is not always
     * zero and is not derivable from the geometry above: a soft sectored
     * medium numbers its sectors from one and a hard sectored one from
     * zero, and that is a property of the medium - whether the sectors
     * are marked by index holes in the jacket or written into the track
     * - not of how many there are.
     *
     * The kernel gets this from the drive: sys/dj.c reads the drive
     * characteristics byte, sees that a soft sectored diskette has no
     * format table to look in because it names no configuration, and
     * sets ORG1 from that.  The loader has nothing to ask - the rom has
     * already read the boot sector and the controller knows only what it
     * is told - so the medium says it here instead.  A loader that
     * ignores this reads every sector one place out on a soft sectored
     * diskette, and invents a sector past the end of every track.
     *
     * Appended, so a label written before this field existed reads zero,
     * which is the right answer for a hard disk and for a hard sectored
     * diskette, and is what the five rolled rows in cmd/mkfs write.
     */
    UINT d_firstsec;
};

#define DL_MAGIC    "MWDL"      /* Morrow Winchester disk label */
#define DL_VERSION  1           /* 1 = the roll idiom, 2 = the slice idiom;
                                 * consulted by tools, which must know
                                 * whether an inode owns the boot */
#define DL_VERS_SLICE 2         /* the slice idiom's version, written by
                                 * whoever writes a table: a diskette, and
                                 * any hard disk cmd/label gives a table.
                                 * Spelled short because ccc keeps fifteen
                                 * significant characters and silently
                                 * drops a longer macro: DL_VERSION_SLICE
                                 * is one too many, and comes out as an
                                 * undefined symbol at use and a
                                 * truncated one at the definition. */

/*
 * The geometry a driver needs to map a block: the label as the kernel
 * uses it, rather than as the disk carries it.  sys/mw.c's struct info
 * and sys/ide.c's each begin with these seven fields, in this order and
 * of these types, and the decode below writes through this view of them
 * - so neither driver copies a field in or out, and neither gives up its
 * own names for its own struct.
 *
 * Three structures and one agreement, and the compiler checks none of
 * it: a driver that reorders or widens a field above its own stops
 * agreeing with this one, and what breaks is the geometry - a seek that
 * lands somewhere else - not the build.  Both drivers say so where their
 * structs are defined.
 */
struct dlgeom
{
    UINT tracks;                /* cylinders */
    UINT8 heads;
    UINT8 sectors;              /* sectors per track */
    UINT maxblk;                /* highest legal block number in a slice */
    UINT spc;                   /* sectors per cylinder */
    UINT roll;                  /* what a block's cylinder is rotated by */
    UINT cylstart;              /* first cylinder of the slice */
};

/*
 * Read the disk label from block 0 of cdev and set *g to the geometry of
 * slice sl.
 *
 * cdev is the drive's own 'c', which the caller builds out of the dev it
 * was opened with.  That is the whole reason the label can be read at
 * all: 'c' is the one slice that consults no table, starting at cylinder
 * 0 and never rolled, so its block 0 is device block 0 - the sector the
 * rom reads and this label is in - on every disk.  It is read as the
 * disk block it is, through the buffer cache and released, so that the
 * label and the sector it sits in are one object rather than two copies
 * of one.
 *
 * On entry *g holds what the caller knows of the drive - mw.c's row of
 * its table, or nothing at all - and the return value says what it holds
 * on return:
 *
 *	1   the label was read; *g describes slice sl
 *	0   no label on the drive; *g is the caller's own geometry, placed
 *	    as an empty slice table places it, which is the whole drive.
 *	    A caller that knew nothing is left with spc and maxblk zero,
 *	    which maps block 0 and nothing else - all that can be told
 *	    about a drive nothing is known about, and what a labeler
 *	    needs.
 *     -1   the label names no such slice; *g maps nothing, and u.error
 *	    is ENXIO.
 */
extern int dllabel();

/*
 * vim: set tabstop=4 shiftwidth=4 expandtab:
 */
