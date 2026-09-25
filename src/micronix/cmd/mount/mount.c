/*
 * mount - mount a filesystem
 *
 * cmd/mount/mount.c
 *
 * THIS IS A RECONSTRUCTION.  There is no surviving source for /bin/mount;
 * this file is written from the v7 mount (extra/v7/usr/src/cmd/mount.c)
 * and the strings and shape of the /bin/mount binary on the 1.6
 * filesystem.  The v7 program was the template; the mtab layout, the
 * NMOUNT limit and the mount(2) call are Micronix's.
 *
 * One thing is deliberately newer than the binary: it checks the
 * superblock's magic number (s_magic) to decide whether a device holds a
 * filesystem, instead of reading the root inode and its directory as the
 * original did.  "Mount refuses to mount a special file which does not
 * contain a file system" (mount.1) is now one field instead of three
 * reads.
 *
 * mount with no argument lists /etc/mtab; with a special and a directory
 * it mounts the special on the directory and adds an entry.  The mtab is
 * init's root entry plus whatever mount and umount have written.
 *
 * vim: tabstop=4 shiftwidth=4 noexpandtab:
 */

#include <types.h>
#include <stdio.h>
#include <string.h>
#include <sys/fs.h>
#include <mtab.h>

#define NMOUNT 4		/* the kernel's NMOUNT (sys/sys.h) */
#define SUPERBLK 1

struct mtab mtab[NMOUNT];
struct super sblock;

void list();
int checkfs();
void addmtab();

main(argc, argv)
    int argc;
    char **argv;
{
    int ro;
    int mf;

    mf = open("/etc/mtab", 0);
    if (mf >= 0) {
        read(mf, (char *)mtab, sizeof mtab);
        close(mf);
    }

    if (argc == 1) {
        list();
        exit(0);
    }
    if (argc < 3 || argc > 4) {
        fprintf(stderr, "usage: mount [special directory [-r]]\n");
        exit(1);
    }

    ro = (argc == 4);

    if (!checkfs(argv[1])) {
        fprintf(stderr, "%s: not a file system\n", argv[1]);
        exit(1);
    }
    if (mount(argv[1], argv[2], ro) < 0) {
        perror("mount");
        exit(1);
    }
    addmtab(argv[1], argv[2]);
    exit(0);
}

/*
 * List the mounted filesystems, one per line, "special on directory".
 */
void
list()
{
    int i;

    for (i = 0; i < NMOUNT; i++)
        if (mtab[i].directory[0])
            printf("%s on %s\n", mtab[i].special, mtab[i].directory);
}

/*
 * Does this device hold a Micronix filesystem?  One field, read through
 * the block device (which for a mounted filesystem is the buffer cache's
 * live copy).
 */
int
checkfs(special)
    char *special;
{
    int fd;
    int ok;

    if ((fd = open(special, 0)) < 0)
        return 0;
    ok = 0;
    seek(fd, SUPERBLK, 3);
    if (read(fd, (char *)&sblock, sizeof sblock) == sizeof sblock)
        ok = (sblock.s_magic == FsMAGIC);
    close(fd);
    return ok;
}

/*
 * Add a mount to the table and rewrite /etc/mtab.  The special file's
 * name is stored with its directory stripped - "dja", not "/dev/dja".
 */
void
addmtab(special, dir)
    char *special, *dir;
{
    char *name;
    int i;
    int mf;

    if ((name = strrchr(special, '/')) != 0)
        name++;
    else
        name = special;

    for (i = 0; i < NMOUNT; i++) {
        if (mtab[i].directory[0])
            continue;
        strcpy(mtab[i].special, name);
        strcpy(mtab[i].directory, dir);
        break;
    }

    if ((mf = creat("/etc/mtab", 0644)) < 0)
        return;
    write(mf, (char *)mtab, sizeof mtab);
    close(mf);
}
