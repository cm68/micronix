/*
 * df - disk free space
 *
 * cmd/df/df.c
 *
 * df reports the free and used blocks and inodes of each filesystem.
 * A filesystem that is mounted gets its numbers read live out of the
 * kernel's mount table, which the allocators keep current on every
 * allocation (a static array, so it costs nothing); an unmounted one
 * gets them from its superblock, which mkfs seeds and sync() refreshes.
 * Used is the difference: fsize - free blocks, and the whole I-list -
 * isize * 16, sixteen inodes to a block - minus free inodes.
 *
 * The kernel's mount table is reached through a pointer in low memory
 * (0x1005, next to ps's process-table pointer at 0x1003) and /dev/mem,
 * and it is the authority on what is mounted - a mounted filesystem's
 * live counts come from it, an unmounted one's from its superblock.
 * /etc/mtab names which filesystems to report when no argument is given.
 * An argument is reported whether mounted or not, and may be given with
 * or without a "/dev/" prefix.
 *
 * vim: tabstop=4 shiftwidth=4 noexpandtab:
 */

#include <types.h>
#include <stdio.h>
#include <string.h>
#include <sys/fs.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <mtab.h>

#define NMOUNT 4		/* the kernel's NMOUNT (sys/sys.h) */
#define SUPERBLK 1		/* the superblock */
#define MEMORY "/dev/mem"
#define MPTAB 0x1005		/* low memory: &mlist, beside &plist at 0x1003 */
#define INOPB 16		/* inodes per block, as iget.c's "isize << 4" */

struct mtab mtab[NMOUNT];
struct mount mountab[NMOUNT];
struct super sblock;
int nmount;
int headed;			/* the column heading has been printed */

int readmtab();
int readmounts();
int finddev();
void report();
int super();

main(argc, argv)
    int argc;
    char **argv;
{
    int i;
    char *np, *special;

    nmount = readmtab();
    readmounts();

    if (argc == 1) {
        for (i = 0; i < nmount; i++)
            report(mtab[i].special);
        exit(0);
    }

    for (i = 1; i < argc; i++) {
        np = argv[i];
        if ((special = strrchr(np, '/')) != 0)
            special++;          /* the name after the last slash */
        else
            special = np;
        report(special);
    }
    exit(0);
}

/*
 * Read /etc/mtab, one 64-byte record at a time.
 */
int
readmtab()
{
    int fd;
    int n;

    n = 0;
    if ((fd = open("/etc/mtab", 0)) < 0)
        return 0;
    while (n < NMOUNT && read(fd, &mtab[n], sizeof mtab[n]) == sizeof mtab[n]) {
        if (mtab[n].directory[0] == 0 || mtab[n].special[0] == 0)
            continue;
        n++;
    }
    close(fd);
    return n;
}

/*
 * Read the kernel's mount table through /dev/mem: a two-byte pointer at
 * MPTAB names the table, the way ps reads the process table at 0x1003.
 */
int
readmounts()
{
    unsigned mptr;
    int mem;
    int n;

    if ((mem = open(MEMORY, 0)) < 0)
        return 0;
    if (seek(mem, MPTAB, 0) < 0 || read(mem, &mptr, sizeof mptr) != sizeof mptr) {
        close(mem);
        return 0;
    }
    if (seek(mem, mptr, 0) < 0) {
        close(mem);
        return 0;
    }
    n = read(mem, (char *)mountab, sizeof mountab);
    close(mem);
    return n / sizeof(struct mount);
}

/*
 * Find the mount entry for a device number and return its totals and
 * its live free counts.
 */
int
finddev(devnum, fsizep, isizep, bfreep, ifreep)
    int devnum;
    UINT *fsizep, *isizep, *bfreep, *ifreep;
{
    int i;

    for (i = 0; i < NMOUNT; i++) {
        if (mountab[i].dev == 0 || mountab[i].dev != devnum)
            continue;
        *fsizep = mountab[i].fsize;
        *isizep = mountab[i].isize;
        *bfreep = mountab[i].bfree;
        *ifreep = mountab[i].ifree;
        return 1;
    }
    return 0;
}

/*
 * Report one filesystem: if it is mounted, its live counts come from the
 * mount table; if not, its last-synced counts come from the superblock.
 */
void
report(special)
    char *special;
{
    struct stat st;
    char dev[32];
    UINT fsize, isize, bfree, ifree;
    UINT totinode, usedblk, usedinode;

    strcpy(dev, "/dev/");
    strcat(dev, special);

    /*
     * The kernel's mount table is the authority on what is mounted,
     * and it is asked first: a mounted device's superblock lives in
     * its pinned buffer, and reading it through the block device would
     * disturb that buffer's hold.  A device not in the table is read
     * from its superblock.
     */
    if (stat(dev, &st) >= 0 &&
        finddev(st.st_addr[0], &fsize, &isize, &bfree, &ifree)) {
        /* live counts are already in fsize/isize/bfree/ifree */
    } else if (super(dev, &fsize, &isize, &bfree, &ifree)) {
        /* read from the superblock */
    } else {
        return;
    }

    totinode = isize * INOPB;
    usedblk = fsize - bfree;
    usedinode = totinode - ifree;

    if (!headed) {
        printf("%-14s %6s %6s %6s %6s\n",
               "device", "free", "used", "ifree", "iused");
        headed = 1;
    }
    printf("%-14s %6u %6u %6u %6u\n", dev, bfree, usedblk, ifree, usedinode);
}

/*
 * Read an unmounted filesystem's totals and counts from its superblock.
 * Returns 1 and fills the pointers, or 0 if the device is not a
 * Micronix filesystem.
 */
int
super(dev, fsizep, isizep, bfreep, ifreep)
    char *dev;
    UINT *fsizep, *isizep, *bfreep, *ifreep;
{
    int fd;

    if ((fd = open(dev, 0)) < 0) {
        perror(dev);
        return 0;
    }
    seek(fd, SUPERBLK, 3);
    if (read(fd, (char *)&sblock, sizeof sblock) != sizeof sblock) {
        perror(dev);
        close(fd);
        return 0;
    }
    close(fd);

    if (sblock.s_magic != FsMAGIC) {
        fprintf(stderr, "%s: not a file system\n", dev);
        return 0;
    }
    *fsizep = sblock.s_fsize;
    *isizep = sblock.s_isize;
    *bfreep = sblock.s_bfree;
    *ifreep = sblock.s_ifree;
    return 1;
}
