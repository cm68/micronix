/*
 * mkfs - make an empty file system, and optionally a boot area in it
 *
 * cmd/mkfs/mkfs.c
 *
 *	mkfs device [size]
 *	mkfs device [-exclude]
 *	mkfs -i bootfile device [size|-exclude]
 *	mkfs -b blocks device [size|-exclude]
 *
 * A numerical argument without a leading minus is the size to make the
 * filesystem, in blocks.  With a leading minus it is a count of blocks to
 * leave at the end of the device, which is how swap space is kept out of
 * the filesystem.  Given neither, the whole device is used.  The device
 * size is worked out from the drive: the minor number of the node names
 * the model - the model is bits 2 and up of it - and the model's geometry
 * is the size.
 *
 * -b <blocks> says the size outright instead, for the nodes that name no
 * drive at all (dev/devlist's ide0c and scsi0c are minor 64, hd1c is 73)
 * and for a disk whose node names one model while the disk is another.
 *
 * It takes a block count and not a geometry because here those are the
 * same fact: the tables below are a list of models, each one a geometry
 * and the capacity that geometry multiplies out to, so the capacity names
 * the row.  A count that is no model's is refused rather than guessed at,
 * because the boot area and the label both go where the geometry puts
 * them, and there is nowhere to put them without one.
 *
 * The drivers read that geometry off the disk itself (sys/dlabel.c, and
 * the label in the boot sector).  mkfs cannot: the label is at cylinder
 * 0, and where cylinder 0 is is the thing the geometry is being asked
 * for.
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

#include "mkfs.h"

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
    fprintf(stderr, "usage: %s [-i bootfile] [-b blocks] device [size|-exclude]\n",
        pname);
    fprintf(stderr, "\tsize is in 512 byte blocks, -exclude leaves that\n");
    fprintf(stderr, "\tmany at the end of the device, say for swap\n");
    fprintf(stderr, "\t-i gives cylinder 0 to a file and writes the boot\n");
    fprintf(stderr, "\tinto it, so the free list never offers it\n");
    fprintf(stderr, "\t-b is the size of the device, for a node whose\n");
    fprintf(stderr, "\tminor number does not name the drive\n");
    exit(1);
}

/*
 * Which drive, from the minor number of the block special file we were
 * given, or -1 if the minor names none.  Being told would be worse than
 * working it out - a wrong answer here puts the boot area somewhere
 * inside the filesystem - so the minor is only believed when it says
 * something.  It says nothing on the nodes the hard disks are reached by
 * (dev/devlist's ide0c is minor 64, and 64 is bigger than any model), and
 * then -b has to.
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
        return (-1);
    return (type);
}

/*
 * The drive with that many blocks, or -1.  The tables are a list of
 * models and a model's capacity is its geometry multiplied out, so a
 * number off the drive names the row the way the row names the number.
 *
 * A bare count is not enough to write with - the boot area goes at a
 * block the geometry picks out, the label goes with the boot area, and
 * nothing else in mkfs knows a geometry - so a count no model has is the
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

main(argc, argv)
    int argc;
    char **argv;
{
    char *device;
    char *arg;
    UINT dsize;                 /* the whole device */
    UINT blocks;                /* -b: the whole device, told outright */
    int bset;                   /* and whether it was told */
    UINT exclude;
    UINT given;
    UINT fsize;
    UINT isize;
    UINT bootfirst;
    UINT bootnblk;
    UINT spc;
    int type;
    char *bfile;
    int f;

    pname = argv[0];
    bfile = DEFBOOT;
    exclude = 0;
    given = 0;
    blocks = 0;
    bset = 0;
    f = 0;

    /*
     * The manual says "mkfs device [size]" and m5init says
     * "mkfs -f -1024 /dev/m5a", so the shipped one takes them in either
     * order and so does this.  Every argument is one of five things and
     * each says which it is: -i takes the boot file, -b the block count
     * of the device, a minus and digits is an exclusion, digits alone are
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
            if (arg[1] == 'b' && arg[2] == 0) {
                if (argc < 1)
                    usage();
                argc--;
                blocks = atoi(*argv++);
                bset = 1;
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

    type = drivetype(device);
    if (bset) {
        type = blocksdrive(blocks);
        if (type < 0)
            die("no drive of that many blocks");
    } else if (type < 0) {
        die("no drive of that minor number");
    }
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
    printf("I-list: %d blocks, %d inodes\n", isize, isize * IPERBLK);

    /*
     * Where the boot area is, before the worker's in-use check reads it.
     */
    spc = dheads[type] * (UINT) dsecs[type];
    bootfirst = (dtracks[type] - (dtracks[type] >> 1)) * spc;
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
