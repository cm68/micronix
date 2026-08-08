/*
 * installboot - give the boot area to a file, and put the boot in it
 *
 * cmd/installboot/installboot.c
 *
 * The DJ/DMA rom boots a floppy off its reserved tracks, but a hard disk
 * has no reserved tracks: mkfs takes the whole device apart from what it
 * is told to leave at the end for swap.  The rom still reads cylinder 0
 * head 0 sector 0 and enters what it finds there, so that sector has to
 * hold a boot and has to keep holding it.
 *
 * The only thing in a v6 filesystem that stops a block being handed out
 * is an inode claiming it.  So this makes a file that owns cylinder 0
 * and writes the boot into it, and from then on the filesystem itself is
 * what protects the boot area - fsck will defend it, and anything that
 * allocates will step around it.
 *
 * It runs on the raw device, straight after mkfs and before the mount,
 * because that is the one moment when nothing is cached and the free
 * lists are exactly what mkfs just wrote.  A mounted kernel holds the
 * superblock and the inodes in core and would write its own copies back
 * over ours at umount, saying nothing.
 *
 * The blocks are not taken off the free list here - they are owned and
 * still queued to be handed out, which is the state icheck calls "dup".
 * Run icheck -s afterwards to rebuild the list from what the inodes
 * claim; m5init already runs a check at that point in the install.
 */

#include <types.h>
#include <stdio.h>
#include <sys/fs.h>
#include <sys/dir.h>
#include <errno.h>

#define SUPERBLK    1               /* where the superblock lives */
#define INOSTART    2               /* first inode block */
#define IPERBLK     16              /* inodes to a block */
#define DPERBLK     32              /* directory entries to a block */
#define ROOTINO     1
#define BSIZE       512
#define NADDR       8               /* addresses in an inode */
#define APERBLK     256             /* addresses in an indirect block */

/*
 * The drives, from the specs table in sys/mw.c.  Only the geometry
 * matters here: mw.c maps a block to a cylinder as blk/spc plus half the
 * cylinder count, wrapping, so cylinder 0 is not block 0 - it is halfway
 * up the block numbers, and this is where that gets worked out.
 */
char *dnames[] = { "m5", "m10", "m16", 0 };
int dtracks[] = { 153, 306, 306 };
int dheads[] = { 4, 4, 6 };
int dsecs[] = { 17, 17, 17 };

char *pname;
int dev = -1;
int verbose;

char sbbuf[BSIZE];
char blkbuf[BSIZE];
char dirbuf[BSIZE];
char indbuf[BSIZE];
struct super *sb;

usage()
{
    fprintf(stderr, "usage: %s [-v] -%s <device> <bootfile> [<name>]\n",
        pname, "m5|-m10|-m16");
    fprintf(stderr, "\trun on an unmounted device, after mkfs\n");
    fprintf(stderr, "\tfollow with icheck -s to rebuild the free list\n");
    exit(1);
}

die(s)
    char *s;
{
    fprintf(stderr, "%s: %s\n", pname, s);
    exit(1);
}

/*
 * A block is 512 bytes at 512 times its number, on the raw device.  On a
 * mounted filesystem this would be the kernel's business; here it is
 * ours, and it is the same arithmetic the kernel would do.
 */
rdblk(bn, buf)
    int bn;
    char *buf;
{
    if (lseek(dev, bn * (long) BSIZE, 0) == -1)
        die("seek");
    if (read(dev, buf, BSIZE) != BSIZE)
        die("read");
}

wrblk(bn, buf)
    int bn;
    char *buf;
{
    if (lseek(dev, bn * (long) BSIZE, 0) == -1)
        die("seek");
    if (write(dev, buf, BSIZE) != BSIZE)
        die("write");
}

/*
 * Take an inode number off the superblock's list, and a block off the
 * free list.  Straight after mkfs both lists are full, so the awkward
 * cases - an empty inode list wanting a scan of the ilist, a free list
 * wanting the next chain block read in - cannot arise yet, and rather
 * than write code that would never run we refuse and say why.
 */
int
getino()
{
    if (sb->s_ninode <= 0)
        die("no free inodes in the superblock - is this freshly mkfs'd?");
    return sb->s_inode[--sb->s_ninode];
}

int
getblk()
{
    if (sb->s_nfree <= 1)
        die("free list nearly empty - is this freshly mkfs'd?");
    return sb->s_free[--sb->s_nfree];
}

/*
 * Read and write an inode.  Inode 1 is the root, and it lives at the
 * start of block 2, so inode n is (n-1) into the ilist.
 */
struct dsknod *
getdsk(ino, buf)
    int ino;
    char *buf;
{
    rdblk(INOSTART + (ino - 1) / IPERBLK, buf);
    return ((struct dsknod *) buf) + ((ino - 1) % IPERBLK);
}

putdsk(ino, buf)
    int ino;
    char *buf;
{
    wrblk(INOSTART + (ino - 1) / IPERBLK, buf);
}

/*
 * Add a name to the root directory.  A slot with a zero inumber is free,
 * and after mkfs there are plenty in the first block, so this only ever
 * has to fill one in.
 */
addroot(ino, name)
    int ino;
    char *name;
{
    struct dsknod *rp;
    struct dir *dp;
    int rblk;
    int i;
    int j;
    int k;

    rp = getdsk(ROOTINO, blkbuf);
    if (rp->d_mode & ILARG)
        die("root directory is a large file - not expected after mkfs");

    for (i = 0; i < NADDR; i++) {
        rblk = rp->d_addr[i];
        if (rblk == 0)
            continue;
        rdblk(rblk, dirbuf);
        dp = (struct dir *) dirbuf;
        for (j = 0; j < DPERBLK; j++, dp++) {
            if (dp->ino != 0)
                continue;
            dp->ino = ino;
            for (k = 0; k < 14 && name[k]; k++)
                dp->name[k] = name[k];
            wrblk(rblk, dirbuf);

            /*
             * The directory got no bigger - the slot was already inside
             * its size - unless we are past where it said it ended.
             */
            rp = getdsk(ROOTINO, blkbuf);
            i = (j + 1) * sizeof(struct dir);
            if (rp->d_size1 < i)
                rp->d_size1 = i;
            rp->d_nlink++;      /* the new directory's .. */
            putdsk(ROOTINO, blkbuf);
            return;
        }
    }
    die("no free slot in the root directory");
}

main(argc, argv)
    int argc;
    char **argv;
{
    struct dsknod *ip;
    char *device;
    char *bootfile;
    char *name;
    char *fname;
    int type = -1;
    int dirino;
    int filino;
    int dirblk;
    int indblk;
    int first;
    int spc;
    int nblk;
    int bfd;
    int got;
    int i;
    int n;

    pname = argv[0];
    name = "boot";
    fname = "bootmw";

    while (--argc > 0 && **++argv == '-') {
        if (argv[0][1] == 'v') {
            verbose++;
            continue;
        }
        for (i = 0; dnames[i]; i++) {
            if (strcmp(&argv[0][1], dnames[i]) == 0) {
                type = i;
                break;
            }
        }
        if (type < 0)
            usage();
    }
    if (argc < 2 || type < 0)
        usage();

    device = *argv++;
    bootfile = *argv++;
    argc -= 2;
    if (argc > 0)
        name = *argv;

    /*
     * Where cylinder 0 is, in blocks.  mw.c adds half the cylinder count
     * and wraps, so the cylinder that comes out as 0 is the one whose
     * block index is tracks minus tracks/2, and a whole cylinder is
     * heads times sectors.
     */
    spc = dheads[type] * dsecs[type];
    first = (dtracks[type] - (dtracks[type] >> 1)) * spc;
    nblk = spc;

    if ((dev = open(device, 2)) < 0)
        die("cannot open the device for writing");
    rdblk(SUPERBLK, sbbuf);
    sb = (struct super *) sbbuf;

    if (first + nblk > sb->s_fsize)
        die("cylinder 0 is past the end of this filesystem - wrong drive?");

    if ((bfd = open(bootfile, 0)) < 0)
        die("cannot open the boot file");

    printf("%s: cylinder 0 is blocks %d through %d\n",
        device, first, first + nblk - 1);

    /*
     * The directory first, so that if anything goes wrong later there is
     * an empty directory rather than a file nothing points at.
     */
    dirino = getino();
    filino = getino();
    dirblk = getblk();
    for (i = 0; i < BSIZE; i++)
        dirbuf[i] = 0;
    ((struct dir *) dirbuf)[0].ino = dirino;
    ((struct dir *) dirbuf)[0].name[0] = '.';
    ((struct dir *) dirbuf)[1].ino = ROOTINO;
    ((struct dir *) dirbuf)[1].name[0] = '.';
    ((struct dir *) dirbuf)[1].name[1] = '.';
    ((struct dir *) dirbuf)[2].ino = filino;
    for (i = 0; i < 14 && fname[i]; i++)
        ((struct dir *) dirbuf)[2].name[i] = fname[i];
    wrblk(dirblk, dirbuf);

    ip = getdsk(dirino, blkbuf);
    for (i = 0; i < sizeof(struct dsknod); i++)
        ((char *) ip)[i] = 0;
    ip->d_mode = IALLOC | IFDIR;    /* mode 0: nothing may walk in here */
    ip->d_nlink = 2;
    ip->d_size1 = 3 * sizeof(struct dir);
    ip->d_addr[0] = dirblk;
    putdsk(dirino, blkbuf);

    /*
     * Then the file.  A cylinder is more than the eight addresses an
     * inode holds on every drive here, so it is always a large file and
     * always wants an indirect block.
     */
    indblk = getblk();
    for (i = 0; i < BSIZE; i++)
        indbuf[i] = 0;
    for (i = 0; i < nblk && i < APERBLK; i++)
        ((UINT *) indbuf)[i] = first + i;
    wrblk(indblk, indbuf);

    ip = getdsk(filino, blkbuf);
    for (i = 0; i < sizeof(struct dsknod); i++)
        ((char *) ip)[i] = 0;
    ip->d_mode = IALLOC | ILARG;    /* a regular file, mode 0 */
    ip->d_nlink = 1;
    ip->d_size0 = nblk >> 7;
    ip->d_size1 = (nblk & 0177) * BSIZE;
    ip->d_addr[0] = indblk;
    putdsk(filino, blkbuf);

    addroot(dirino, name);

    /*
     * Now the boot itself, straight into the blocks the file owns.  The
     * first of them is what the rom reads, so the first 512 bytes of the
     * boot file had better be a first level boot built to run where the
     * rom enters it.
     */
    for (n = 0; n < nblk; n++) {
        for (i = 0; i < BSIZE; i++)
            blkbuf[i] = 0;
        got = read(bfd, blkbuf, BSIZE);
        if (got <= 0)
            break;
        wrblk(first + n, blkbuf);
        if (verbose)
            printf("\tblock %d\n", first + n);
    }
    close(bfd);

    /*
     * And the superblock last: it is the thing that says which inodes and
     * blocks we took, and writing it before the rest would leave a disk
     * claiming things that had not been written yet.
     */
    wrblk(SUPERBLK, sbbuf);
    close(dev);

    printf("/%s/%s is inode %d: %d blocks from %d, %d of them written\n",
        name, fname, filino, nblk, first, n);
    printf("now run icheck -s %s to rebuild the free list\n", device);
    exit(0);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
