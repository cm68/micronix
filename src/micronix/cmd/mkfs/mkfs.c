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
 * The boot is the device blocks below the filesystem, which the label's
 * slice table already put in front of slice 'a', so no filesystem block
 * number reaches it and no file has to own it.  -i names the boot file
 * (DEFBOOT is /bootmw.bin); the file's first half-block goes into block
 * 0 in front of the label, and the rest from block 1 on, which is where
 * the rom and the level 2 sector expect them.  A missing file is a
 * filesystem with no boot, not an error: a data disk has no use for one.
 *
 * This is the command driver: it works out the device and the geometry,
 * opens the raw unix device, and hands the building over to mkfsfunc.c,
 * which mnix links too.  The two halves meet at rdblk/wrblk.
 */

#include <types.h>
#include <stdio.h>
#include <string.h>
#include <sys/fs.h>
#include <sys/stat.h>
#include <sys/dir.h>
#include <sys/dlabel.h>

#include "mkfs.h"
#include <unistd.h>
#include <stdlib.h>



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

/*
 * The whole-disk node that belongs to a device node.
 *
 * The label and the boot live at device block 0, which a slice node
 * cannot address: block 0 of slice a is a cylinder into the drive.  The
 * old way to reach it was to mknod a 'c' node under /tmp, which needs a
 * writable root; the boot diskette root is not writable.  The /dev the
 * diskette or a running system already carries has a whole-disk node for
 * every controller - dev/devlist names them - so this finds it by major
 * and minor and opens that instead.
 *
 * A floppy has no whole-disk slice.  Its own node is the whole disk and
 * its block 0 is the label, so that node is the answer.
 *
 * Answers the path in a static buffer, or 0 if there is no such node.
 */
static char wholename[32];

static char *
whole_node(device, majorp, minorp)
    char *device;
    UINT *majorp, *minorp;
{
    struct stat sb, ns;
    struct dir de;
    char nm[15];
    char path[32];
    UINT major, whole;
    int dfd, n, i;

    if (stat(device, &sb) < 0 || (sb.st_mode & IFMT) != IFBLK) {
        printf("mkfs: %s is not a block device\n", device);
        return (0);
    }
    major = sb.st_addr[0] >> 8;
    whole = ((sb.st_addr[0] & 0377) & ~(7 << 5)) | (DL_WHOLE << 5);
    *majorp = major;
    *minorp = whole;
    if (major == 2) {           /* a floppy: its own node is the whole */
        strncpy(wholename, device, sizeof(wholename) - 1);
        wholename[sizeof(wholename) - 1] = 0;
        return (wholename);
    }
    dfd = open("/dev", 0);
    if (dfd < 0) {
        printf("mkfs: cannot open /dev\n");
        return (0);
    }
    while ((n = read(dfd, (char *) &de, sizeof de)) == sizeof de) {
        if (de.ino == 0 || de.name[0] == '.')
            continue;
        for (i = 0; i < 14 && de.name[i]; i++)
            nm[i] = de.name[i];
        nm[i] = 0;
        strcpy(path, "/dev/");
        strcat(path, nm);
        if (stat(path, &ns) < 0 || (ns.st_mode & IFMT) != IFBLK)
            continue;
        if ((ns.st_addr[0] & 0377) != whole ||
            (ns.st_addr[0] >> 8) != major)
            continue;
        close(dfd);
        strcpy(wholename, path);
        return (wholename);
    }
    close(dfd);
    printf("mkfs: no whole-disk node for %u/%u\n", major, whole);
    return (0);
}

/*
 * Write the label's boot extent back into block 0 of the whole disk.
 *
 * The boot lives in the blocks below slice 'a' - the label command put
 * the slice there - and the label records how many blocks that is
 * (d_bootblks).  That is the one thing a filesystem can say that a bare
 * label cannot, and the host's mnix says it through putlabel() when it
 * makes a filesystem; a disk this program makes has to say it too, or
 * the next boot finds a label with no boot in it and level 1 will not
 * offer the disk.
 *
 * The label block is device block 0 of the whole-disk slice, which the
 * slice device this is writing cannot reach - block 0 of 'a' is a
 * cylinder into the drive - so a 'c' node is built the way labelgeom()
 * builds one, and that sector is read, labelled and written back.
 */
static
putboot(device)
    char *device;
{
    char *path;
    char buf[BSIZE];
    UINT major, minor;
    int fd;

    if ((path = whole_node(device, &major, &minor)) == 0)
        return;
    if ((fd = open(path, 2)) < 0)
        return;
    if (lseek(fd, 0L, 0) >= 0 && read(fd, buf, BSIZE) == BSIZE) {
        putlabel(buf);
        if (lseek(fd, 0L, 0) < 0 || write(fd, buf, BSIZE) != BSIZE)
            die("cannot write the label");
    }
    close(fd);
}

/*
 * Write the boot below the filesystem, the other half of what putboot
 * records.  Block 0's first half is the level 2 sector and its second
 * half is the label, so the boot file supplies the first half and the
 * label already in the block is kept.  Level 3 is the rest of the file,
 * from block 1 on.
 *
 * The HDCA reads level 2's load address from the last two bytes of the
 * sector - mkbootimg -a puts them there - and those bytes are outside
 * the label.  Take them from the boot file, as mnix's hardboot does.
 */
static
writeboot(device, bfile)
    char *device;
    char *bfile;
{
    char *path;
    char blk[BSIZE];
    char la[2];
    UINT major, minor;
    int fd, bfd, got;
    int n, i;

    if ((path = whole_node(device, &major, &minor)) == 0)
        return;
    if ((fd = open(path, 2)) < 0)
        return;
    if ((bfd = open(bfile, 0)) < 0) {
        close(fd);
        return;
    }
    if (lseek(fd, 0L, 0) < 0 || read(fd, blk, BSIZE) != BSIZE)
        goto out;
    if (read(bfd, blk, DL_OFFSET) < 0)
        goto out;
    if (major == 1 && lseek(bfd, (long) BSIZE - 2, 0) >= 0) {
        if (read(bfd, la, 2) == 2) {
            blk[BSIZE - 2] = la[0];
            blk[BSIZE - 1] = la[1];
        }
    }
    if (lseek(fd, 0L, 0) < 0 || write(fd, blk, BSIZE) != BSIZE)
        goto out;
    /*
     * Level 3 from block 1.  The boot file's own second half is not
     * the medium's label - mkbootimg leaves it empty for the writer -
     * so skip it to the block boundary and go on from there.
     */
    lseek(bfd, (long) BSIZE, 0);
    for (n = 1; ; n++) {
        got = read(bfd, blk, BSIZE);
        if (got <= 0)
            break;
        for (i = got; i < BSIZE; i++)
            blk[i] = 0;
        if (lseek(fd, (long) n * BSIZE, 0) < 0 ||
            write(fd, blk, BSIZE) != BSIZE)
            break;
    }
out:
    close(bfd);
    close(fd);
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
    char *path;
    struct stat sb;
    struct dlabel *lp;
    UINT major, minor, sl, off, len;
    char buf[BSIZE];
    int fd;

    if (stat(device, &sb) < 0 || (sb.st_mode & IFMT) != IFBLK)
        return (0);
    sl = (sb.st_addr[0] >> 5) & 7;
    if ((path = whole_node(device, &major, &minor)) == 0)
        return (0);
    if ((fd = open(path, 0)) < 0)
        return (0);
    if (lseek(fd, 0L, 0) < 0 || read(fd, buf, BSIZE) != BSIZE) {
        close(fd);
        return (0);
    }
    close(fd);
    lp = (struct dlabel *) &buf[DL_OFFSET];
    if (lp->d_magic[0] != DL_MAGIC[0] || lp->d_magic[1] != DL_MAGIC[1] ||
        lp->d_magic[2] != DL_MAGIC[2] || lp->d_magic[3] != DL_MAGIC[3] ||
        lp->d_tracks == 0 || lp->d_heads == 0 || lp->d_spt == 0)
        return (0);
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
     * The boot code is not this program's to place: it is the device
     * blocks below the filesystem, which the label's slice table already
     * put in front of slice 'a'.  What this does place is the label's
     * record of that extent, after the filesystem is built.
     */
    if ((dev = open(device, 2)) < 0)
        die("cannot open the device for writing");

    domkfs(fsize, isize, dsize, f);
    putboot(device);
    /*
     * After domkfs, not before: opening the whole-disk node to write
     * the boot sets the drive's mapping to the whole disk, and the
     * filesystem writes must still be going through the slice's.
     */
    writeboot(device, bfile);

    close(dev);
    exit(0);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
