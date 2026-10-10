/*
 * the disk label - what a disk says about itself
 *
 * include/sys/dlabel.h
 *
 * A Micronix disk is self-describing.  There is exactly one place on it
 * that can be found knowing nothing at all - cylinder 0, head 0, sector
 * 0, the sector the rom reads to boot - and that is where this label
 * lives, in the second half of the boot sector.  From it a driver learns
 * everything it needs to drive the disk without a table: the geometry,
 * the drive's timing, where its filesystems and swap live, and how to
 * boot it.  This is one format shared by every block driver (mw, ide,
 * ncr, dj, disk2, disk3, hdca), decoded once in sys/dlabel.c.
 *
 * The label is the drive speaking, not the controller: geometry and
 * timing are properties of the drive, so both are here.  A drive that
 * has no label has no geometry, and the only operations possible on it
 * are labelling and formatting, through its whole-disk slice 'c'.
 */

#define DL_OFFSET   256         /* into the boot sector, the second half */
#define NSLICE      8           /* a through h */

/*
 * What a slice is used for.  A filesystem and a swap area are the two
 * things a slice can hold; anything else is unused.  'c', the whole
 * disk, is always unused.
 */
#define SL_UNUSED   0
#define SL_FS       1           /* a v6 filesystem */
#define SL_SWAP     2           /* a swap area */

/*
 * 'c', the whole drive, and the one slice whose extent is not in the
 * table: it starts at cylinder 0 and is never rolled, so its block 0 is
 * device block 0 - the sector the rom reads, and the one this label is
 * in.  That is what makes the label maintainable: a tool opens hd0c,
 * reads block 0, and has the geometry, without needing the geometry to
 * find the geometry.
 */
#define DL_WHOLE    2

/*
 * One slice: a window on the drive, in cylinders.
 *
 * d_off and d_len place it; a length of zero means "not carved yet", so
 * a freshly formatted disk's table carries types and no extents.  The
 * offset is in cylinders and not in blocks on purpose: a block number is
 * 16 bits and a drive's worth of them is not, so a slice's position is
 * added after the division that turns a block into a cylinder and never
 * has to be a block number at all.
 *
 * d_roll rotates the mapping within the slice, so a filesystem can put
 * its inode table in the middle of its platter region rather than at the
 * edge; zero, the ordinary case, is no rotation.
 *
 * d_type is one byte, what the slice is for (above).  d_resv keeps the
 * entry eight bytes so the next entry's word fields stay aligned, and is
 * reserved.
 */
struct dslice {
    UINT d_off;                 /* where it starts, in cylinders */
    UINT d_len;                 /* how many cylinders, 0 = not carved */
    UINT d_roll;                /* rotation within the slice, 0 = none */
    UINT8 d_type;               /* SL_FS, SL_SWAP, or SL_UNUSED */
    UINT8 d_resv;               /* reserved, keeps the entry 8 bytes */
};

struct dlabel {
    char d_magic[4];            /* DL_MAGIC, readable in a hex dump */
    UINT d_version;             /* of this structure, DL_VERSION */

    /*
     * The drive, as the formatter wrote it.  A reader that disagrees with
     * these has found the wrong drive or the wrong table, and should say
     * so rather than carry on.
     */
    UINT d_tracks;              /* cylinders */
    UINT d_heads;
    UINT d_spt;                 /* 512-byte sectors (blocks) per track */

    /*
     * The number of the first sector on a track, which is not always
     * zero and is not derivable from the geometry: a soft sectored
     * medium numbers its sectors from one and a hard sectored one from
     * zero.  The kernel gets this from the drive (sys/dj.c); the loader
     * has nothing to ask, so the medium says it here.
     */
    UINT d_firstsec;

    /*
     * The drive's timing - properties of the drive, not the controller,
     * so the drive carries them.  d_step is the step pulse delay and
     * d_seek the head settle time after a seek, both in 100 us.  d_precomp
     * and d_lowcur name the cylinder where write precompensation begins
     * and the one where the write current drops to its low setting;
     * 0xffff means the drive wants neither.
     */
    UINT8 d_step;
    UINT8 d_seek;
    UINT d_precomp;             /* 0xffff = never */
    UINT d_lowcur;              /* 0xffff = never */

    /*
     * The boot, which is an attribute of the drive and not of any slice.
     * d_bootblks is how many blocks the boot owns from device block 0
     * (0 = no boot), and d_bootslice is the slice it loads its
     * filesystem from.  Nothing inside a filesystem is the boot.
     */
    UINT d_bootblks;
    UINT d_bootslice;

    /*
     * The filesystems and swap, a through h.
     */
    struct dslice d_slice[NSLICE];

    /*
     * What the drive calls itself, written by the formatter so a
     * labelled drive can be identified without a table.  Both are
     * NUL-terminated and the rest of each field is zero.
     */
    char d_mfg[16];             /* manufacturer, e.g. "Maxtor" */
    char d_model[16];           /* model number, e.g. "XT-1140" */
};

#define DL_MAGIC    "MWDL"      /* Micronix disk label */
#define DL_VERSION  3           /* the slice + timing + type format */

/*
 * The geometry a driver needs to map a block: the label as the kernel
 * uses it, rather than as the disk carries it.  Every block driver's
 * struct info (or struct hd) begins with these fields, in this order and
 * of these types, and sys/dlabel.c writes through this view of them - so
 * no driver copies a field in or out, and none gives up its own names for
 * its own struct.
 *
 * maxblk, spc, roll, cylstart and len are not in the label; they are
 * decoded from it for the slice being mapped.
 *
 * Three structures and one agreement, and the compiler checks none of
 * it: a driver that reorders or widens a field above its own stops
 * agreeing with this one, and what breaks is the geometry - a seek that
 * lands somewhere else - not the build.  Each driver says so where its
 * struct is defined.
 */
struct dlgeom
{
    UINT tracks;                /* cylinders */
    UINT8 heads;
    UINT8 sectors;              /* sectors per track */
    UINT maxblk;                /* highest legal block number in a slice */
    UINT spc;                   /* sectors per cylinder */
    UINT roll;                  /* the slice's d_roll */
    UINT cylstart;              /* first cylinder of the slice */
    UINT8 firstsec;             /* first sector number on a track */
    UINT len;                   /* the slice's length, in cylinders */
};

/*
 * The drive's timing, decoded from the label.  It is not part of
 * struct dlgeom above: it is how a drive is driven, not how a block is
 * mapped, and only the MFM drivers - mw.c and disk3.c - command it.  The
 * drivers whose controller finds the sector itself (ide, ncr) and the
 * diskette (dj, disk2) read it and drop it.  dllabel() fills *tm only
 * when the caller passes one.
 */
struct dltiming
{
    UINT8 step;                 /* step pulse delay, 100 us */
    UINT8 seek;                 /* head settle after seek, 100 us */
    UINT precomp;               /* where write precomp begins, 0xffff never */
    UINT lowcur;                /* where low write current begins, 0xffff never */
};

/*
 * One slice, as a driver caches it: the offset, length and roll it needs
 * to map a block.  The label's own entry carries a type and a reserved
 * byte beside these, which a driver has no use for - the type belongs to
 * the tools that read the label - so the cache is this smaller thing.
 */
struct slicemap
{
    UINT sm_off;                /* where it starts, in cylinders */
    UINT sm_len;                /* how many cylinders, 0 = not carved */
    UINT sm_roll;               /* rotation within the slice, 0 = none */
};

/*
 * Read the disk label from block 0 of cdev and set *g to the geometry of
 * slice sl.  table, when given, is the cached slice map; tm, when given,
 * is filled with the drive's timing.
 *
 * cdev is the drive's own 'c', which the caller builds out of the dev it
 * was opened with.  That is the whole reason the label can be read at
 * all: 'c' is the one slice that consults no table, starting at cylinder
 * 0 and never rolled, so its block 0 is device block 0 on every disk.
 *
 * On entry *g holds what the caller knows of the drive, and the return
 * value says what it holds on return:
 *
 *	1   the label was read; *g describes slice sl
 *	0   no label on the drive; *g is the caller's own geometry.  A
 *	    caller that knew nothing is left with spc and maxblk zero,
 *	    which maps block 0 and nothing else - all that can be told
 *	    about a drive nothing is known about, and what a labeler
 *	    needs.
 *     -1   the label names no such slice; *g maps nothing, and u.error
 *	    is ENXIO.
 */
extern int dllabel();

/*
 * Decode one slice of a drive against its cached map: offset in
 * cylinders, roll (0 for 'c'), length, and highest legal block number.
 * Returns 0, or ENXIO for a slice that is not carved or runs off the
 * drive.
 */
extern int dlslice();

/*
 * vim: set tabstop=4 shiftwidth=4 expandtab:
 */
