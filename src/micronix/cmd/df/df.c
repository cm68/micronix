/*
 * df - disk free space
 *
 * cmd/df/df.c
 *
 * df reports the free blocks and free inodes of each filesystem.  A
 * filesystem that is mounted gets its numbers read live out of the
 * kernel's mount table, which the allocators keep current on every
 * allocation (a static array, so it costs nothing); an unmounted one
 * gets them from its superblock, which mkfs seeds and sync() refreshes.
 *
 * The mount table is reached through a pointer in low memory (0x1005,
 * next to ps's process-table pointer at 0x1003) and /dev/mem.  "Mounted"
 * is decided by /etc/mtab, which init seeds with root and the mount
 * program keeps.  With no argument every mounted filesystem is reported;
 * an argument is reported whether mounted or not, and may be given with
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

struct mtab mtab[NMOUNT];
struct mount mountab[NMOUNT];
struct super sblock;
int nmount;

int readmtab();
int readmounts();
int mounted();
int finddev();
void report();
void super();

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
 * Is this device name in /etc/mtab?
 */
int
mounted(special)
    char *special;
{
    int i;

    for (i = 0; i < nmount; i++)
        if (strcmp(mtab[i].special, special) == 0)
            return 1;
    return 0;
}

/*
 * Find the mount entry for a device number and return its live counts.
 */
int
finddev(devnum, bfreep, ifreep)
    int devnum;
    int *bfreep, *ifreep;
{
    int i;

    for (i = 0; i < NMOUNT; i++) {
        if (mountab[i].dev == 0 || mountab[i].dev != devnum)
            continue;
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
    int bfree, ifree;

    strcpy(dev, "/dev/");
    strcat(dev, special);

    if (mounted(special)) {
        if (stat(dev, &st) < 0 || !finddev(st.st_addr[0], &bfree, &ifree)) {
            fprintf(stderr, "%s: not mounted\n", dev);
            return;
        }
        printf("%5u %5u %s\n", bfree, ifree, dev);
    } else {
        super(dev);
    }
}

/*
 * Read an unmounted filesystem's counts straight from its superblock.
 */
void
super(dev)
    char *dev;
{
    int fd;

    if ((fd = open(dev, 0)) < 0) {
        perror(dev);
        return;
    }
    seek(fd, SUPERBLK, 3);
    if (read(fd, (char *)&sblock, sizeof sblock) != sizeof sblock) {
        perror(dev);
        close(fd);
        return;
    }
    close(fd);

    if (sblock.s_magic != FsMAGIC) {
        fprintf(stderr, "%s: not a file system\n", dev);
        return;
    }
    printf("%5u %5u %s\n", sblock.s_bfree, sblock.s_ifree, dev);
}
