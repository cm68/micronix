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
 * size comes from the disk's own label: the driver reads the label off
 * block 0 of the whole-disk slice at open (sys/dlabel.c), and mkfs reads
 * it the same way through a whole-disk node it builds.  A disk without a
 * label has no geometry to make a filesystem from, and is refused.
 *
 * It takes a block count and not a geometry because here those are the
 * same fact: the tables below are a list of models, each one a geometry
 * and the capacity that geometry multiplies out to, so the capacity names
 * the row.  A count that is no model's is refused rather than guessed at,
 * because the boot area and the label both go where the geometry puts
 * them, and there is nowhere to put them without one.
 *
 * This is the command driver: it works out the device and the geometry,
 * opens the raw unix device, and hands the building over to mkfsfunc.c,
 * which mnix links too.  The two halves meet at rdblk/wrblk.
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
    if (lseek(dev, bn * (long) BSIZE, 0) == -1)
        die("seek");
    if (read(dev, buf, BSIZE) != BSIZE)
        die("read");
}

void wrblk(bn, buf)
    UINT bn;
    char *buf;
{
    if (lseek(dev, bn * (long) BSIZE, 0) == -1)
        die("seek");
    if (write(dev, buf, BSIZE) != BSIZE)
        die("write");
}

void usage()
{
    fprintf(stderr, "usage: %s [-i bootfile] device [size|-exclude]\n",
        pname);
    fprintf(stderr, "\tsize is in 512 byte blocks, -exclude leaves that\n");
    fprintf(stderr, "\tmany at the end of the device, say for swap\n");
    fprintf(stderr, "\t-i gives cylinder 0 to a file and writes the boot\n");
    fprintf(stderr, "\tinto it, so the free list never offers it\n");
    exit(1);
}

/*
 * The drive with that many blocks, or -1.  The tables are a list of
 * models and a model's capacity is its geometry multiplied out, so a
 * number off the drive names the row the way the row names the number.
 *
 * The label carries the geometry; this maps it back to the table row,
 * whose dtoff[] and dfirst[] say the two things the label does not (the
 * boot offset and the first sector number).  A count no model has is the
 * caller's mistake and is reported as one.
 */
int
blocksdrive(blocks)
    UINT blocks;
{
    int t;

    for (t = 0; t < NDRIVE; t++)
        if (dtracks[t] * (UINT) dheads[t] * dsecs[t] == blocks)
            return (t);
    return (-1);
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
labelgeom(device, tracksp, headsp, sptp)
    char *device;
    UINT *tracksp, *headsp, *sptp;
{
    static char path[] = "/tmp/mkfs.c";
    struct stat sb;
    struct dlabel *lp;
    UINT major, minor;
    char buf[BSIZE];
    int fd;

    if (stat(device, &sb) < 0 || (sb.st_mode & IFMT) != IFBLK)
        return (0);
    major = sb.st_addr[0] >> 8;
    minor = sb.st_addr[0] & 0377;
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
    UINT exclude;
    UINT given;
    UINT fsize;
    UINT isize;
    UINT bootfirst;
    UINT bootnblk;
    UINT spc;
    UINT tracks, heads, spt;    /* the geometry, from the label */
    int type;
    char *bfile;
    int f;

    pname = argv[0];
    bfile = DEFBOOT;
    exclude = 0;
    given = 0;
    f = 0;

    /*
     * The manual says "mkfs device [size]" and m5init says
     * "mkfs -f -1024 /dev/hd0a", so the shipped one takes them in either
     * order and so does this.  Every argument is one of five things and
     * each says which it is: -i takes the boot file, a minus and digits
     * is an exclusion, digits alone are
     * a size, and what is left is the device.
     *
     * -f is what m5init has always passed, and it now means what it
     * looks like: go ahead and destroy what is there.  Without it, a
     * disk that already holds a filesystem is left alone.
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
    if (given && exclude)
        die("give a size or an exclusion, not both");

    /*
     * The geometry comes from the disk's own label, which the labeler
     * wrote and the driver reads back at open.  A disk without one has
     * nothing to make a filesystem from: label it (or format it) first.
     */
    if (!labelgeom(device, &tracks, &heads, &spt))
        die("no label on the device");
    type = blocksdrive(tracks * (UINT) heads * spt);
    if (type < 0)
        die("no drive of that geometry");
    dsize = tracks * (UINT) heads * spt;
    printf("Device size: %u blocks\n", dsize);

    if (given) {
        fsize = given;
        if (fsize > dsize)
            die("that is bigger than the device");
    } else {
        if (exclude >= dsize)
            die("nothing left after the exclusion");
        fsize = dsize - exclude;
    }
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
     * Where the boot area is, before the worker's in-use check reads it.
     */
    spc = heads * (UINT) spt;
    bootfirst = (tracks - (tracks >> 1)) * spc;
    bootnblk = spc;

    if ((dev = open(device, 2)) < 0)
        die("cannot open the device for writing");

    domkfs(fsize, isize, bootfirst, bootnblk, dsize, type, bfile, f);

    close(dev);
    exit(0);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
