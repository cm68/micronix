/*
 * mkfs - make an empty file system
 *
 * cmd/mkfs/mkfs.c
 *
 *	mkfs device [size]
 *	mkfs -i bootfile device [size]
 *
 * A numerical argument without a leading minus is the size to make the
 * filesystem, in blocks.  Given none, the slice the device names is used.
 * The size and the geometry both come from the disk's own label: the
 * driver reads the label off block 0 of the whole-disk slice at open
 * (sys/dlabel.c), and mkfs reads it the same way through a whole-disk
 * node it builds.  A disk without a label has no geometry to make a
 * filesystem from, and is refused.
 *
 * The boot is not this program's: it is the device blocks below the
 * filesystem, which the label's slice table already put in front of slice
 * 'a', so no filesystem block number reaches it and no file has to own it.
 * The -i flag is carried for the host's mnix, which writes the boot below
 * the filesystem; here it is accepted and ignored.
 *
 * This is the command driver: it works out the device and the geometry,
 * opens the raw unix device, and hands the building over to mkfsfunc.c,
 * which mnix links too.  The two halves meet at rdblk/wrblk.
 */

#include <types.h>
#include <stdio.h>
#include <sys/fs.h>
#include <sys/stat.h>
#include <sys/dlabel.h>

#include "mkfs.h"

extern int mknod(), unlink();

int dev = -1;

/*
 * The block i/o this side of the split: straight lseek/read/write on the
 * raw unix device.  mnix defines the same two names against the image.
 */
void rdblk(bn, buf)
    UINT bn;
    char *buf;
{
    if (lseek(dev, (unsigned long) bn * BSIZE, 0) == -1)
        die("seek");
    if (read(dev, buf, BSIZE) != BSIZE)
        die("read");
}

void wrblk(bn, buf)
    UINT bn;
    char *buf;
{
    if (lseek(dev, (unsigned long) bn * BSIZE, 0) == -1)
        die("seek");
    if (write(dev, buf, BSIZE) != BSIZE)
        die("write");
}

void usage()
{
    fprintf(stderr, "usage: %s [-i bootfile] device [size]\n",
        pname);
    fprintf(stderr, "\t-i gives cylinder 0 to a file and writes the boot\n");
    fprintf(stderr, "\tinto it, so the free list never offers it\n");
    exit(1);
}

/*
 * Read the drive's geometry off its own disk label, which the labeler
 * wrote and the driver reads back at open (sys/dlabel.c).  The label is
 * in block 0 of the whole-disk slice 'c', the one block a driver can
 * reach without knowing the geometry - block 0 of 'c' is device block 0
 * however the disk is laid out - so 'c' is built here from the node's
 * own major and minor and read.  Answers the geometry, or 0 when there
 * is no label to read.
 */
int
labelgeom(device, tracksp, headsp, sptp, sizep)
    char *device;
    UINT *tracksp, *headsp, *sptp, *sizep;
{
    static char path[] = "/tmp/mkfs.c";
    struct stat sb;
    struct dlabel *lp;
    UINT major, minor, sl, off, len;
    char buf[BSIZE];
    int fd;

    if (stat(device, &sb) < 0 || (sb.st_mode & IFMT) != IFBLK)
        return (0);
    major = sb.st_addr[0] >> 8;
    minor = sb.st_addr[0] & 0377;
    sl = (minor >> 5) & 7;
    minor = (minor & ~(7 << 5)) | (DL_WHOLE << 5);
    if (mknod(path, 0160666, (major << 8) | minor) < 0)
        return (0);
    if ((fd = open(path, 0)) < 0)
        goto out;
    if (lseek(fd, 0L, 0) < 0 || read(fd, buf, BSIZE) != BSIZE) {
        close(fd);
        goto out;
    }
    close(fd);
    lp = (struct dlabel *) &buf[DL_OFFSET];
    if (lp->d_magic[0] != DL_MAGIC[0] || lp->d_magic[1] != DL_MAGIC[1] ||
        lp->d_magic[2] != DL_MAGIC[2] || lp->d_magic[3] != DL_MAGIC[3] ||
        lp->d_tracks == 0 || lp->d_heads == 0 || lp->d_spt == 0) {
        unlink(path);
        return (0);
    }
    unlink(path);
    *tracksp = lp->d_tracks;
    *headsp = lp->d_heads;
    *sptp = lp->d_spt;
    /*
     * The slice the node names, as a count of blocks: its offset and
     * length in cylinders, times the sectors per cylinder.  A length of
     * zero is the whole drive from the offset on, which is what an empty
     * table means - so a disk made before slices sizes the filesystem
     * exactly as it always did.
     */
    off = lp->d_slice[sl].d_off;
    len = lp->d_slice[sl].d_len;
    if (len == 0)
        len = lp->d_tracks - off;
    *sizep = len * (UINT) lp->d_heads * lp->d_spt;
    return (1);
out:
    unlink(path);
    return (0);
}

main(argc, argv)
    int argc;
    char **argv;
{
    char *device;
    char *arg;
    UINT dsize;                 /* the whole device */
    UINT given;
    UINT fsize;
    UINT isize;
    UINT tracks, heads, spt;    /* the geometry, from the label */
    char *bfile;
    int f;

    pname = argv[0];
    bfile = DEFBOOT;
    given = 0;
    f = 0;

    /*
     * Every argument is one of three things and each says which it is:
     * -i takes the boot file, digits alone are a size, and what is left
     * is the device.
     *
     * -f means go ahead and destroy what is there.  Without it, a disk
     * that already holds a filesystem is left alone.
     */
    device = 0;
    argc--;                     /* step over our own name */
    argv++;
    while (argc-- > 0) {
        arg = *argv++;
        if (arg[0] == '-') {
            if (arg[1] == 'f' && arg[2] == 0) {
                f = 1;
                continue;
            }
            if (arg[1] != 'i' || arg[2] != 0 || argc < 1)
                usage();
            argc--;
            bfile = *argv++;
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

    /*
     * The geometry comes from the disk's own label, which the labeler
     * wrote and the driver reads back at open.  A disk without one has
     * nothing to make a filesystem from: label it (or format it) first.
     */
    if (!labelgeom(device, &tracks, &heads, &spt, &dsize))
        die("no label on the device");
    printf("Device size: %u blocks\n", dsize);

    if (given) {
        fsize = given;
        if (fsize > dsize)
            die("that is bigger than the device");
    } else
        fsize = dsize;
    printf("File system size: %u blocks\n", fsize);

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
    printf("I-list: %u blocks, %u inodes\n", isize, isize * IPERBLK);

    /*
     * The boot is not this program's to place: it is the device blocks
     * below the filesystem, which the label's slice table already put in
     * front of slice 'a'.  The worker builds only the filesystem.
     */
    if ((dev = open(device, 2)) < 0)
        die("cannot open the device for writing");

    domkfs(fsize, isize, dsize, f);

    close(dev);
    exit(0);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
