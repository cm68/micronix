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

char *pname;

char sbbuf[BSIZE];
char blkbuf[BSIZE];
char dirbuf[BSIZE];
char indbuf[BSIZE];

UINT fsize;                     /* blocks in the filesystem */
UINT isize;                     /* blocks of inodes */
UINT nextblk;                   /* next block to hand out while building */
UINT nextino;                   /* next inumber to hand out */

char *bootfile;                 /* -i, or DEFBOOT */
int force;                      /* -f: destroy what is already there */

/* said twice, so it is a string and not two */
char *inuse = "there is a filesystem here - use -f";
UINT bootfirst;                 /* first block of the boot area */
UINT bootnblk;                  /* how many of them */

struct super *sb;

#define S_ISIZE     (sb->s_isize)
#define S_FSIZE     (sb->s_fsize)
#define S_NFREE     (sb->s_nfree)
#define S_NINODE    (sb->s_ninode)
#define S_FREE(i)   (sb->s_free[i])
#define S_INODE(i)  (sb->s_inode[i])

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
 * Is this block part of the boot area?  Asked for every block in the
 * device while the free list is built.
 */
int
inboot(bn)
    UINT bn;
{
    return bn >= bootfirst && bn < bootfirst + bootnblk;
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
 */
void putlabel(buf, type)
    char *buf;
    int type;
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
        for (i = DL_OFFSET; i < BSIZE; i++) {
            if (buf[i])
                die("the boot code runs into the label - it is too big");
        }
    }
    for (i = 0; i < 4; i++)
        lp->d_magic[i] = DL_MAGIC[i];
    lp->d_version = DL_VERSION;

    lp->d_tracks = dtracks[type];
    lp->d_heads = dheads[type];
    lp->d_spt = dsecs[type];

    lp->d_cyl0 = bootfirst;
    lp->d_bootblks = bootnblk;
    lp->d_roll = dtracks[type] >> 1;    /* what mw.c adds to blk / spc */

    lp->d_fsize = fsize;
    lp->d_isize = isize;
    lp->d_swap = dtracks[type] * dheads[type] * dsecs[type] - fsize;
}

/*
 * Build the filesystem.  Everything about the device - its size, where
 * cylinder 0 is, which drive - has already been worked out by the
 * caller and is passed in; this writes through rdblk/wrblk and opens
 * only the boot file.
 */
void
domkfs(fs, is, bfirst, bnblk, dsize, type, bfile, f)
    UINT fs, is, bfirst, bnblk, dsize;
    int type, f;
    char *bfile;
{
    struct dsknod *ip;
    UINT rootblk;
    UINT dirblk;
    UINT indblk;
    UINT dirino;
    UINT filino;
    UINT n;
    int bfd;
    int got;
    int i;

    fsize = fs;
    isize = is;
    bootfirst = bfirst;
    bootnblk = bnblk;
    bootfile = bfile;
    force = f;

    /*
     * Is there already something here?  Two ways to tell, and either is
     * enough to stop: the label at cylinder 0 is the good one, and a
     * disk made before there were labels falls back on whether block 1
     * reads like a superblock.
     */
    if (!force) {
        struct dlabel *lp;

        rdblk(bootfirst, blkbuf);
        lp = (struct dlabel *) &blkbuf[DL_OFFSET];
        if (lp->d_magic[0] == DL_MAGIC[0] && lp->d_magic[1] == DL_MAGIC[1] &&
            lp->d_magic[2] == DL_MAGIC[2] && lp->d_magic[3] == DL_MAGIC[3]) {
            printf("Label: %d/%d/%d, %d blocks\n", lp->d_tracks,
                lp->d_heads, lp->d_spt, lp->d_fsize);
            die(inuse);
        }

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

    /* the boot block, and the ilist, all zero */
    wrblk(0, blkbuf);
    for (n = 0; n < isize; n++)
        wrblk(INOSTART + n, blkbuf);

    sb = (struct super *) sbbuf;
    for (i = 0; i < BSIZE; i++)
        sbbuf[i] = 0;
    S_ISIZE = isize;
    S_FSIZE = fsize;

    nextblk = INOSTART + isize;
    nextino = ROOTINO;

    if (bootfirst + bootnblk > fsize)
        die("cylinder 0 falls outside the filesystem");
    if (bootfirst < nextblk)
        die("cylinder 0 lands in the ilist - filesystem too small");
    printf("Boot area: blocks %d through %d, cylinder 0\n",
        bootfirst, bootfirst + bootnblk - 1);

    /*
     * The root directory, and the boot directory and file if we are
     * making one.  These come off the front of the data area, which is
     * below where the free list starts, so they are never freed.
     */
    nextino = ROOTINO;
    rootblk = allocblk();
    nextino++;                          /* inode 1 is the root */

    dirino = nextino++;
    filino = nextino++;
    dirblk = allocblk();
    indblk = allocblk();

    /* the root directory: . and .. and, if there is one, boot */
    zero(dirbuf);
    ((struct dir *) dirbuf)[0].ino = ROOTINO;
    ((struct dir *) dirbuf)[0].name[0] = '.';
    ((struct dir *) dirbuf)[1].ino = ROOTINO;
    ((struct dir *) dirbuf)[1].name[0] = '.';
    ((struct dir *) dirbuf)[1].name[1] = '.';
    ((struct dir *) dirbuf)[2].ino = dirino;
    ((struct dir *) dirbuf)[2].name[0] = 'b';
    ((struct dir *) dirbuf)[2].name[1] = 'o';
    ((struct dir *) dirbuf)[2].name[2] = 'o';
    ((struct dir *) dirbuf)[2].name[3] = 't';
    n = 3 * sizeof(struct dir);
    wrblk(rootblk, dirbuf);

    ip = getdsk(ROOTINO, blkbuf);
    clrdsk(ip);
    ip->d_mode = IALLOC | IFDIR | 0777;
    ip->d_nlink = 3;
    ip->d_size1 = n;
    ip->d_addr[0] = rootblk;
    putdsk(ROOTINO, blkbuf);

    {
        /*
         * A directory nothing can walk into, holding a file nothing can
         * open.  Two inodes to make the boot that much harder to lose to
         * a careless command, and both are free.
         */
        zero(dirbuf);
        ((struct dir *) dirbuf)[0].ino = dirino;
        ((struct dir *) dirbuf)[0].name[0] = '.';
        ((struct dir *) dirbuf)[1].ino = ROOTINO;
        ((struct dir *) dirbuf)[1].name[0] = '.';
        ((struct dir *) dirbuf)[1].name[1] = '.';
        ((struct dir *) dirbuf)[2].ino = filino;
        ((struct dir *) dirbuf)[2].name[0] = 'b';
        ((struct dir *) dirbuf)[2].name[1] = 'o';
        ((struct dir *) dirbuf)[2].name[2] = 'o';
        ((struct dir *) dirbuf)[2].name[3] = 't';
        ((struct dir *) dirbuf)[2].name[4] = 'm';
        ((struct dir *) dirbuf)[2].name[5] = 'w';
        wrblk(dirblk, dirbuf);

        ip = getdsk(dirino, blkbuf);
        clrdsk(ip);
        ip->d_mode = IALLOC | IFDIR;    /* mode 0 */
        ip->d_nlink = 2;
        ip->d_size1 = 3 * sizeof(struct dir);
        ip->d_addr[0] = dirblk;
        putdsk(dirino, blkbuf);

        /*
         * A cylinder is more blocks than the eight an inode holds on
         * every drive in the table, so the file is always large and
         * always wants the indirect block.
         */
        zero(indbuf);
        for (i = 0; i < bootnblk && i < APERBLK; i++)
            ((UINT *) indbuf)[i] = bootfirst + i;
        wrblk(indblk, indbuf);

        ip = getdsk(filino, blkbuf);
        clrdsk(ip);
        ip->d_mode = IALLOC | ILARG;    /* a regular file, mode 0 */
        ip->d_nlink = 1;
        ip->d_size0 = bootnblk >> 7;
        ip->d_size1 = (bootnblk & 0177) * BSIZE;
        ip->d_addr[0] = indblk;
        putdsk(filino, blkbuf);
    }

    /*
     * The free list.  Built from the top of the device downward, so the
     * numbers left in the superblock are the low ones and a fresh
     * filesystem allocates from the front.  The boot area is simply
     * never offered - which is the whole point of doing this here.
     *
     * Seeded with a single zero entry, which is what the end of the
     * chain looks like to alloc.
     */
    S_NFREE = 1;
    S_FREE(0) = 0;
    for (n = fsize - 1; n >= nextblk; n--) {
        if (!inboot(n))
            addfree(n);
        if (n == 0)
            break;
    }

    /* and the free inodes */
    S_NINODE = 0;
    for (n = nextino; n <= isize * IPERBLK && S_NINODE < NICINOD; n++)
        S_INODE(S_NINODE++) = n;

    /*
     * The boot itself, into the blocks the file now owns.  The first of
     * them is what the rom reads and enters, so the front of the boot
     * file has to be a first level boot built to run where the rom puts
     * it.
     */
    bfd = open(bootfile, 0);
    if (bfd < 0)
        printf("No boot: cannot open %s\n", bootfile);

    for (n = 0; n < bootnblk; n++) {
        zero(blkbuf);
        if (bfd >= 0) {
            got = read(bfd, blkbuf, BSIZE);
            if (got <= 0) {
                close(bfd);
                bfd = -1;
                got = 0;
            }
        }
        /*
         * The label goes in whether or not there was a boot to put
         * around it: an unbootable disk still has to be able to say what
         * it is.  The first block is written even when the file was
         * empty, because that block is the label.
         */
        if (n == 0) {
            putlabel(blkbuf, type);
        } else if (bfd < 0) {
            break;
        }
        wrblk(bootfirst + n, blkbuf);
    }
    if (bfd >= 0)
        close(bfd);
    if (n > 1)
        printf("Boot: %d blocks written from %s\n", n, bootfile);
    printf("Label: %s at cylinder 0, %d/%d/%d roll %d\n", DL_MAGIC,
        dtracks[type], dheads[type], dsecs[type], dtracks[type] >> 1);

    /*
     * The superblock last.  It is the thing that says which blocks and
     * inodes are spoken for, and writing it first would leave a disk
     * claiming things that had not been written yet.
     */
    wrblk(SUPERBLK, sbbuf);

    printf("Function complete.\n");
}
