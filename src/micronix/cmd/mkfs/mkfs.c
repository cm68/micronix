/*
 * mkfs - make an empty file system, and optionally a boot area in it
 *
 * cmd/mkfs/mkfs.c
 *
 *	mkfs device [size]
 *	mkfs device [-exclude]
 *	mkfs -i bootfile device [size|-exclude]
 *
 * A numerical argument without a leading minus is the size to make the
 * filesystem, in blocks.  With a leading minus it is a count of blocks to
 * leave at the end of the device, which is how swap space is kept out of
 * the filesystem.  Given neither, the whole device is used.  The device
 * size is worked out from the drive, so it never has to be told.
 *
 *
 * the boot area, and why it belongs here
 * --------------------------------------
 *
 * A floppy has reserved tracks and the rom boots out of them.  A hard
 * disk has none: the filesystem covers the whole device except what is
 * left at the end for swap.  The rom still reads cylinder 0 head 0
 * sector 0 and enters what it finds, so something has to keep that
 * sector from being handed out to a file.
 *
 * The only thing in a v6 filesystem that stops a block being allocated
 * is an inode claiming it.  This was a separate program - installboot -
 * which ran after mkfs, wrote such an inode, and then needed icheck -s
 * to rebuild the free list, because between the two the blocks were both
 * owned and still queued to be handed out.  That is the state icheck
 * calls "dup", and it is the one filesystem error that quietly destroys
 * data.
 *
 * Doing it here removes the window entirely.  mkfs builds the free list,
 * so it can simply not put those blocks on it, and there is never a
 * moment when two things own a block.  It also saves a binary on a
 * floppy that has no room for one.
 *
 * Where the boot area is takes some working out, because sys/mw.c does
 * not map block 0 to cylinder 0.  It rotates:
 *
 *	cyl = blk / spc;  cyl += tracks >> 1;  if (cyl >= tracks) cyl -= tracks;
 *
 * so cylinder 0 is the block whose cylinder index is tracks - tracks/2,
 * which is halfway up the disk.  On the 5 meg that is block 5236 of
 * 10404, in the middle of ordinary file space.  Getting this wrong
 * writes a boot into the middle of a filesystem and says it worked.
 */

/*
 * the superblock, by hand
 * -----------------------
 *
 * struct super is 415 bytes, mostly s_free[100] and s_inode[100], and
 * ccc holds member offsets in a byte - so it cannot parse the type at
 * all.  With SMALL_STRUCTS defined, sys/fs.h leaves the struct out and
 * the fields are reached by offset into the block buffer instead.
 *
 * The offsets below are the on-disk layout, which is the thing that
 * actually matters: a Z80 packs this struct with no padding, and a host
 * compiler would align s_time and produce a different one.  So this is
 * not merely a workaround for a compiler limit - it is the more correct
 * of the two ways to write it, and the reason the accessors take the
 * buffer rather than a struct pointer.
 *
 * Undefine SMALL_STRUCTS and the struct comes back and the same code
 * compiles against it, so this can be backfilled with a one line change
 * once big structs work.
 */
#define SMALL_STRUCTS   1

#include <types.h>
#include <stdio.h>
#include <sys/fs.h>
#include <sys/dir.h>
#include <sys/stat.h>

#define BSIZE       512
#define SUPERBLK    1           /* the superblock */
#define INOSTART    2           /* first inode block */
#define IPERBLK     16          /* inodes to a block */
#define ROOTINO     1
#define NICFREE     100         /* free block list in the superblock */
#define NICINOD     100         /* free inode list in the superblock */
#define APERBLK     256         /* block numbers in an indirect block */

/*
 * The drives, from specs[] in sys/mw.c.  The minor number picks one:
 * mw.c does "type = minor(dev) >> 2", the low two bits being the
 * partition, so m5a is minor 0, m10a is 4 and m16a is 8.
 *
 *	tracks	heads	sectors		blocks
 *	153	4	17		10404	5 meg
 *	306	4	17		20808	10 meg
 *	306	6	17		31212	16 meg
 *	640	6	17		65280	32 meg
 *	733	5	17		62305	40 meg
 */
#define NDRIVE      5
UINT dtracks[NDRIVE] = { 153, 306, 306, 640, 733 };
UINT dheads[NDRIVE] = { 4, 4, 6, 6, 5 };
UINT dsecs[NDRIVE] = { 17, 17, 17, 17, 17 };

char *pname;
int dev = -1;

char sbbuf[BSIZE];
char blkbuf[BSIZE];
char dirbuf[BSIZE];
char indbuf[BSIZE];

UINT fsize;                     /* blocks in the filesystem */
UINT isize;                     /* blocks of inodes */
UINT nextblk;                   /* next block to hand out while building */
UINT nextino;                   /* next inumber to hand out */

char *bootfile;                 /* -i, or zero */
UINT bootfirst;                 /* first block of the boot area */
UINT bootnblk;                  /* how many of them */

#ifdef SMALL_STRUCTS

/*
 * Where the superblock's fields are, counted in bytes from the front of
 * the block.  Two byte fields throughout, so the arrays are 200 bytes
 * each and nothing needs alignment.
 *
 *	0	s_isize		206	s_ninode
 *	2	s_fsize		208	s_inode[100]
 *	4	s_nfree		408	s_flock, s_ilock, s_fmod
 *	6	s_free[100]	411	s_time		415 total
 */
#define SB_ISIZE    0
#define SB_FSIZE    2
#define SB_NFREE    4
#define SB_FREE     6
#define SB_NINODE   206
#define SB_INODE    208

UINT *
sbword(off)
    int off;
{
    return (UINT *) &sbbuf[off];
}

#define S_ISIZE     (*sbword(SB_ISIZE))
#define S_FSIZE     (*sbword(SB_FSIZE))
#define S_NFREE     (*sbword(SB_NFREE))
#define S_NINODE    (*sbword(SB_NINODE))
#define S_FREE(i)   (sbword(SB_FREE)[i])
#define S_INODE(i)  (sbword(SB_INODE)[i])

#else

struct super *sb;

#define S_ISIZE     (sb->s_isize)
#define S_FSIZE     (sb->s_fsize)
#define S_NFREE     (sb->s_nfree)
#define S_NINODE    (sb->s_ninode)
#define S_FREE(i)   (sb->s_free[i])
#define S_INODE(i)  (sb->s_inode[i])

#endif

usage()
{
    fprintf(stderr, "usage: %s [-i bootfile] device [size|-exclude]\n", pname);
    fprintf(stderr, "\tsize is in 512 byte blocks, -exclude leaves that\n");
    fprintf(stderr, "\tmany at the end of the device, say for swap\n");
    fprintf(stderr, "\t-i gives cylinder 0 to a file and writes the boot\n");
    fprintf(stderr, "\tinto it, so the free list never offers it\n");
    exit(1);
}

die(s)
    char *s;
{
    fprintf(stderr, "%s: %s\n", pname, s);
    exit(1);
}

rdblk(bn, buf)
    UINT bn;
    char *buf;
{
    if (lseek(dev, bn * (long) BSIZE, 0) == -1)
        die("seek");
    if (read(dev, buf, BSIZE) != BSIZE)
        die("read");
}

wrblk(bn, buf)
    UINT bn;
    char *buf;
{
    if (lseek(dev, bn * (long) BSIZE, 0) == -1)
        die("seek");
    if (write(dev, buf, BSIZE) != BSIZE)
        die("write");
}

zero(buf)
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
 */
bfree(bn)
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
    if (!bootfile)
        return 0;
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

putdsk(ino, buf)
    UINT ino;
    char *buf;
{
    wrblk(INOSTART + (ino - 1) / IPERBLK, buf);
}

clrdsk(ip)
    struct dsknod *ip;
{
    int i;

    for (i = 0; i < sizeof(struct dsknod); i++)
        ((char *) ip)[i] = 0;
}

/*
 * Which drive, from the minor number of the block special file we were
 * given.  Being told would be worse than working it out - a wrong answer
 * here puts the boot area somewhere inside the filesystem.
 */
int
drivetype(device)
    char *device;
{
    struct stat sbuf;
    int type;

    if (stat(device, &sbuf) < 0)
        die("cannot stat the device");
    if ((sbuf.st_mode & IFMT) != IFBLK)
        die("not a block special file");

    type = (sbuf.st_addr[0] & 0377) >> 2;
    if (type >= NDRIVE)
        die("no drive of that minor number");
    return type;
}

main(argc, argv)
    int argc;
    char **argv;
{
    struct dsknod *ip;
    char *device;
    char *arg;
    UINT dsize;                 /* the whole device */
    UINT exclude;
    UINT given;
    UINT rootblk;
    UINT dirblk;
    UINT indblk;
    UINT dirino;
    UINT filino;
    UINT spc;
    UINT n;
    int type;
    int bfd;
    int got;
    int i;

    pname = argv[0];
    bootfile = 0;
    exclude = 0;
    given = 0;

    /*
     * The manual says "mkfs device [size]" and m5init says
     * "mkfs -f -1024 /dev/m5a", so the shipped one takes them in either
     * order and so does this.  Every argument is one of four things and
     * each says which it is: -i takes the boot file, a minus and digits
     * is an exclusion, digits alone are a size, and what is left is the
     * device.  -f is what the script passes to say do not ask; we never
     * ask, so it is accepted and ignored rather than being a usage error
     * on an invocation that has always worked.
     */
    device = 0;
    argc--;                     /* step over our own name */
    argv++;
    while (argc-- > 0) {
        arg = *argv++;
        if (arg[0] == '-' && arg[1] >= '0' && arg[1] <= '9') {
            exclude = atoi(arg + 1);
        } else if (arg[0] == '-') {
            if (arg[1] == 'f' && arg[2] == 0) {
                continue;
            }
            if (arg[1] != 'i' || arg[2] != 0 || argc < 1)
                usage();
            argc--;
            bootfile = *argv++;
        } else if (arg[0] >= '0' && arg[0] <= '9') {
            given = atoi(arg);
        } else {
            if (device)
                usage();
            device = arg;
        }
    }
    if (!device)
        usage();
    if (given && exclude)
        die("give a size or an exclusion, not both");

    type = drivetype(device);
    dsize = dtracks[type] * (UINT) dheads[type] * dsecs[type];
    printf("Device size: %d blocks\n", dsize);

    if (given) {
        fsize = given;
        if (fsize > dsize)
            die("that is bigger than the device");
    } else {
        if (exclude >= dsize)
            die("nothing left after the exclusion");
        fsize = dsize - exclude;
    }
    printf("File system size: %d blocks\n", fsize);

    if (fsize < 50)
        die("too small to be a filesystem");

    /*
     * The ilist size, from the manual: s/43 + s/1000.  Not v6's
     * s/(43 + s/1000), which is a different number - 180 blocks where
     * this gives 227 on a 9380 block filesystem, and 227 is what the
     * shipped mkfs produces.
     */
    isize = fsize / 43 + fsize / 1000;
    if (isize < 1)
        isize = 1;

    if ((dev = open(device, 2)) < 0)
        die("cannot open the device for writing");

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

#ifndef SMALL_STRUCTS
    sb = (struct super *) sbbuf;
#endif
    for (i = 0; i < BSIZE; i++)
        sbbuf[i] = 0;
    S_ISIZE = isize;
    S_FSIZE = fsize;

    nextblk = INOSTART + isize;
    nextino = ROOTINO;

    /*
     * Where the boot area is, before anything is allocated, because the
     * free list has to be built around it.
     */
    if (bootfile) {
        spc = dheads[type] * (UINT) dsecs[type];
        bootfirst = (dtracks[type] - (dtracks[type] >> 1)) * spc;
        bootnblk = spc;
        if (bootfirst + bootnblk > fsize)
            die("cylinder 0 falls outside the filesystem");
        if (bootfirst < nextblk)
            die("cylinder 0 lands in the ilist - filesystem too small");
        printf("Boot area: blocks %d through %d, cylinder 0\n",
            bootfirst, bootfirst + bootnblk - 1);
    }

    /*
     * The root directory, and the boot directory and file if we are
     * making one.  These come off the front of the data area, which is
     * below where the free list starts, so they are never freed.
     */
    nextino = ROOTINO;
    rootblk = allocblk();
    nextino++;                          /* inode 1 is the root */

    if (bootfile) {
        dirino = nextino++;
        filino = nextino++;
        dirblk = allocblk();
        indblk = allocblk();
    }

    /* the root directory: . and .. and, if there is one, boot */
    zero(dirbuf);
    ((struct dir *) dirbuf)[0].ino = ROOTINO;
    ((struct dir *) dirbuf)[0].name[0] = '.';
    ((struct dir *) dirbuf)[1].ino = ROOTINO;
    ((struct dir *) dirbuf)[1].name[0] = '.';
    ((struct dir *) dirbuf)[1].name[1] = '.';
    n = 2 * sizeof(struct dir);
    if (bootfile) {
        ((struct dir *) dirbuf)[2].ino = dirino;
        ((struct dir *) dirbuf)[2].name[0] = 'b';
        ((struct dir *) dirbuf)[2].name[1] = 'o';
        ((struct dir *) dirbuf)[2].name[2] = 'o';
        ((struct dir *) dirbuf)[2].name[3] = 't';
        n = 3 * sizeof(struct dir);
    }
    wrblk(rootblk, dirbuf);

    ip = getdsk(ROOTINO, blkbuf);
    clrdsk(ip);
    ip->d_mode = IALLOC | IFDIR | 0777;
    ip->d_nlink = bootfile ? 3 : 2;
    ip->d_size1 = n;
    ip->d_addr[0] = rootblk;
    putdsk(ROOTINO, blkbuf);

    if (bootfile) {
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
            bfree(n);
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
    if (bootfile) {
        if ((bfd = open(bootfile, 0)) < 0)
            die("cannot open the boot file");
        for (n = 0; n < bootnblk; n++) {
            zero(blkbuf);
            got = read(bfd, blkbuf, BSIZE);
            if (got <= 0)
                break;
            wrblk(bootfirst + n, blkbuf);
        }
        close(bfd);
        printf("Boot: %d blocks written from %s\n", n, bootfile);
    }

    /*
     * The superblock last.  It is the thing that says which blocks and
     * inodes are spoken for, and writing it first would leave a disk
     * claiming things that had not been written yet.
     */
    wrblk(SUPERBLK, sbbuf);
    close(dev);

    printf("Function complete.\n");
    exit(0);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
