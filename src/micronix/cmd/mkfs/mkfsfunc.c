/*
 * the worker half of mkfs - the filesystem builder, shared by the mkfs
 * command and by mnix.  It writes through rdblk/wrblk, which the caller
 * defines, and knows nothing about what those open: the command driver
 * opens the raw unix device, mnix opens the simulated drive image.
 *
 * cmd/mkfs/mkfsfunc.c
 */

#include <types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/fs.h>
#include <sys/dir.h>
#include <sys/dlabel.h>

#include "mkfs.h"

/*
 * The clock, for the superblock stamp.  This is the declaration the
 * tree's own unistd.h carries - ccc rejects any other as a disagreement
 * with it - and it is repeated here because a host build's unistd.h is
 * the system one, which declares no time() at all.  The return is the
 * one part of it not to trust: libunix's time.s explains that a micronix
 * long comes back in the shadow bank, so every caller in the tree,
 * this one included, reads the value from the *tp store instead.
 */
int time(long *);

char *pname;

char sbbuf[BSIZE];
char blkbuf[BSIZE];
char dirbuf[BSIZE];
char indbuf[BSIZE];

UINT fsize;                     /* blocks in the filesystem */
UINT isize;                     /* blocks of inodes */
UINT nextblk;                   /* next block to hand out while building */
UINT nextino;                   /* next inumber to hand out */

int force;                      /* -f: destroy what is already there */

/* said twice, so it is a string and not two */
char *inuse = "there is a filesystem here - use -f";

struct super *sb;

#define S_ISIZE     (sb->s_isize)
#define S_FSIZE     (sb->s_fsize)
#define S_NFREE     (sb->s_nfree)
#define S_NINODE    (sb->s_ninode)
#define S_FREE(i)   (sb->s_free[i])
#define S_INODE(i)  (sb->s_inode[i])
#define S_TIME      (sb->s_time)
#define S_MAGIC     (sb->s_magic)
#define S_BFREE     (sb->s_bfree)
#define S_IFREE     (sb->s_ifree)

void die(s)
    char *s;
{
    fprintf(stderr, "%s: %s\n", pname, s);
    exit(1);
}

void zero(buf)
    char *buf;
{
    int i;

    for (i = 0; i < BSIZE; i++)
        buf[i] = 0;
}

/*
 * Hand out a block while building.  Everything allocated this way is
 * below nextblk, and the free list is built from nextblk upward, so
 * nothing given out here can also end up free.
 */
UINT
allocblk()
{
    if (nextblk >= fsize)
        die("filesystem too small to build");
    return nextblk++;
}

/*
 * Put a block on the free list, exactly as v6 does: a hundred numbers
 * live in the superblock, and when they fill up they are written into
 * the block being freed and that block becomes the head of the chain.
 *
 * Called addfree and not bfree because fslib - which mnix links - has
 * a bfree of its own.
 */
void addfree(bn)
    UINT bn;
{
    int i;

    if (S_NFREE >= NICFREE) {
        ((UINT *) blkbuf)[0] = S_NFREE;
        for (i = 0; i < NICFREE; i++)
            ((UINT *) blkbuf)[i + 1] = S_FREE(i);
        wrblk(bn, blkbuf);
        S_NFREE = 0;
    }
    S_FREE(S_NFREE) = bn;
    S_NFREE++;
}

/*
 * Read and write an inode.  Inode 1 is the root and lives at the start
 * of block 2, so inode n is n-1 into the ilist.
 */
struct dsknod *
getdsk(ino, buf)
    UINT ino;
    char *buf;
{
    rdblk(INOSTART + (ino - 1) / IPERBLK, buf);
    return ((struct dsknod *) buf) + ((ino - 1) % IPERBLK);
}

void putdsk(ino, buf)
    UINT ino;
    char *buf;
{
    wrblk(INOSTART + (ino - 1) / IPERBLK, buf);
}

void clrdsk(ip)
    struct dsknod *ip;
{
    int i;

    for (i = 0; i < sizeof(struct dsknod); i++)
        ((char *) ip)[i] = 0;
}

/*
 * Write the label into the second half of the boot sector.
 *
 * This is the only place on the device that can be found without already
 * knowing what the device is, so it is the only place a description of
 * it is worth anything.  See sys/dlabel.h.
 *
 * The boot code has the first half.  If it has grown into the second we
 * stop rather than write over it: a boot with its tail replaced by a
 * label loads and runs and does something else.
 *
 * The geometry, the roll and the slice table are already in buf - the
 * label command wrote them, or a diskette's boot image carried them in -
 * and are preserved.  This adds only what a filesystem can say and they
 * cannot: its block and inode counts, and the boot's extent below it.
 *
 *	dsize = d_fsize + d_bootblks	(the boot is below the fs)
 */
void putlabel(buf)
    char *buf;
{
    struct dlabel *lp;
    int i;

    /*
     * The second half is either empty or already a label.  stand builds
     * one boot image per drive with the geometry in it - see
     * stand/mkbootimg.c - so a boot file arriving here with our own
     * magic at DL_OFFSET is that, and it is written over: this knows
     * the filesystem as well as the geometry and the built-in one does
     * not.  Anything else in there is the boot code having grown into
     * the label, which is a boot that would be quietly truncated.
     */
    lp = (struct dlabel *) &buf[DL_OFFSET];
    if (lp->d_magic[0] != DL_MAGIC[0] || lp->d_magic[1] != DL_MAGIC[1] ||
        lp->d_magic[2] != DL_MAGIC[2] || lp->d_magic[3] != DL_MAGIC[3]) {
        /*
         * The last two bytes are the boot's entry address, which the
         * HDCA rom reads to know where to load the first level, so they
         * are not boot code that has grown into the label and are left
         * alone.  Everything else in the second half must be empty.
         */
        for (i = DL_OFFSET; i < BSIZE - 2; i++) {
            if (buf[i])
                die("the boot code runs into the label - it is too big");
        }
    }
    for (i = 0; i < 4; i++)
        lp->d_magic[i] = DL_MAGIC[i];

    lp->d_fsize = fsize;
    lp->d_isize = isize;

    /*
     * The boot is below the filesystem, not in it: it owns the device
     * blocks in front of slice 'a', and slice a's offset in cylinders is
     * the whole of the description.  The geometry, the roll and the slice
     * table are already in buf and are preserved; this adds the boot's
     * extent and the filesystem's numbers, both of which came in zero.
     */
    lp->d_version = DL_VERS_SLICE;
    lp->d_cyl0 = 0;
    lp->d_roll = 0;
    lp->d_bootslice = 0;
    lp->d_bootblks = lp->d_slice[0].d_off *
        (UINT) lp->d_heads * lp->d_spt;
}

/*
 * Build the filesystem.  Everything about the device - its size, which
 * drive, where its slice begins - has already been worked out by the
 * caller and is passed in; this writes through rdblk/wrblk and opens
 * nothing.  The boot is not here: it is the blocks below the filesystem,
 * which no filesystem block number reaches, so the filesystem never owns
 * it and no file has to.
 */
void
domkfs(fs, is, dsize, f)
    UINT fs, is, dsize;
    int f;
{
    struct dsknod *ip;
    UINT rootblk;
    UINT n;
    long now;                   /* the clock, for the superblock */
    int i;

    fsize = fs;
    isize = is;
    force = f;

    /*
     * Is there already something here?  Whether block 1 reads like a
     * superblock is the answer; the label's own record is checked by the
     * caller, which has it from the boot area below.
     */
    if (!force) {
        rdblk(SUPERBLK, blkbuf);
        sb = (struct super *) blkbuf;
        if (sb->s_isize && sb->s_isize < 1000 &&
            sb->s_fsize > sb->s_isize + 2 && sb->s_fsize <= dsize) {
            printf("Unlabelled: %d blocks\n", sb->s_fsize);
            die(inuse);
        }
    }

    /*
     * The last block first.  If the device is not as big as we were
     * told, this is where it says so, before anything has been written
     * that somebody might want back.
     */
    zero(blkbuf);
    wrblk(fsize - 1, blkbuf);

    /* the ilist, all zero */
    for (n = 0; n < isize; n++)
        wrblk(INOSTART + n, blkbuf);

    sb = (struct super *) sbbuf;
    for (i = 0; i < BSIZE; i++)
        sbbuf[i] = 0;
    S_ISIZE = isize;
    S_FSIZE = fsize;

    /*
     * The volume's time.  The kernel has no year of its own - the rtc
     * chip has no year register - so rtcinit() takes the year from the
     * superblock when the root is mounted (sys/mount.c), and the only
     * writer of s_time is the sync in sys/uio.c, which stores back a
     * time it derived from s_time in the first place.  Nothing seeded
     * that loop, so it had a fixed point at 1970: every file the system
     * wrote was stamped 1970, and make - whose rule is "the source is
     * newer, build it" - rebuilt everything on every run, forever.
     * Seed it from the clock of whoever makes the filesystem; from the
     * first sync on, the loop sustains itself.
     *
     * A 32-bit field on a micronix disk carries its high word at the
     * lower address, which is the host's long the other way up (see
     * the swap in lib/fslib.c).  A guest's longs are already in the
     * disk's order, so only a host build swaps.  INTEGER_32 is
     * types.h's mark for the host builds - linux, apple.
     */
    time(&now);
#ifdef INTEGER_32
    S_TIME = ((UINT32) now >> 16) | ((UINT32) now << 16);
#else
    S_TIME = (UINT32) now;
#endif
    S_MAGIC = FsMAGIC_DISK;

    nextblk = INOSTART + isize;
    nextino = ROOTINO;

    /*
     * The root directory: . and .. .  It comes off the front of the data
     * area, below where the free list starts, so it is never freed.  No
     * /boot is made: the boot is below the filesystem, not a file in it.
     */
    rootblk = allocblk();
    nextino++;                          /* inode 1 is the root */

    zero(dirbuf);
    ((struct dir *) dirbuf)[0].ino = ROOTINO;
    ((struct dir *) dirbuf)[0].name[0] = '.';
    ((struct dir *) dirbuf)[1].ino = ROOTINO;
    ((struct dir *) dirbuf)[1].name[0] = '.';
    ((struct dir *) dirbuf)[1].name[1] = '.';
    wrblk(rootblk, dirbuf);

    ip = getdsk(ROOTINO, blkbuf);
    clrdsk(ip);
    ip->d_mode = IALLOC | IFDIR | 0777;
    ip->d_nlink = 2;
    ip->d_size1 = 2 * sizeof(struct dir);
    ip->d_addr[0] = rootblk;
    putdsk(ROOTINO, blkbuf);

    /*
     * The free list.  Built from the top of the device downward, so the
     * numbers left in the superblock are the low ones and a fresh
     * filesystem allocates from the front.
     *
     * Seeded with a single zero entry, which is what the end of the
     * chain looks like to alloc.
     */
    S_NFREE = 1;
    S_FREE(0) = 0;
    for (n = fsize - 1; n >= nextblk; n--) {
        addfree(n);
        if (n == 0)
            break;
    }

    /* and the free inodes */
    S_NINODE = 0;
    for (n = nextino; n <= isize * IPERBLK && S_NINODE < NICINOD; n++)
        S_INODE(S_NINODE++) = n;

    /*
     * The running totals, so df reads two words instead of walking the
     * free lists.  Everything from nextblk up is free, and every inode
     * from nextino up is free.
     */
    S_BFREE = fsize - nextblk;
    S_IFREE = isize * IPERBLK - nextino + 1;

    /*
     * The superblock last.  It is the thing that says which blocks and
     * inodes are spoken for, and writing it first would leave a disk
     * claiming things that had not been written yet.
     */
    wrblk(SUPERBLK, sbbuf);

    printf("Function complete.\n");
}
