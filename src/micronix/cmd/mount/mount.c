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
 * reads - for the filesystems that have the field.  The signature is
 * younger than some of the filesystems in the tree: the 1983 floppies in
 * disks/dist were written before it, their superblocks stop at s_time,
 * and everything past it is whatever the shorter struct left in the rest
 * of the block.  Those disks get the original test (rootdir below), so
 * one that mounted in 1983 still mounts.
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
#include <sys/dir.h>
#include <mtab.h>

#define NMOUNT 4		/* the kernel's NMOUNT (sys/sys.h) */
#define SUPERBLK 1
#define ILISTBLK (SUPERBLK + 1)	/* the I-list follows the superblock */
#define ROOTINO 1

struct mtab mtab[NMOUNT];
struct super sblock;

void list();
int checkfs();
int rootdir();
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
 * live copy) - or, for a filesystem older than the field, the three reads
 * the original made.
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
    if (read(fd, (char *)&sblock, sizeof sblock) == sizeof sblock) {
        ok = (sblock.s_magic == FsMAGIC);
        if (!ok)
            ok = rootdir(fd);
    }
    close(fd);
    return ok;
}

/*
 * The test mount used before there was a signature to read: the root
 * inode is a directory, and the first block of the directory it names
 * begins with "." and "..", both of them the root itself.
 *
 * Inode 1 is the first entry of the I-list, so the inode is at a fixed
 * place on the disk and only its data block has to be found.  A root that
 * has grown past eight blocks has gone to the indirect addressing of
 * sys/imap.c, where the first data block is the block its first address
 * names - one level down.
 */
int
rootdir(fd)
    int fd;
{
    struct dsknod ino;
    struct dir e[2];
    UINT blk;

    seek(fd, ILISTBLK, 3);
    if (read(fd, (char *)&ino, sizeof ino) != sizeof ino)
        return 0;
    if ((ino.d_mode & IFMT) != IFDIR)
        return 0;
    blk = ino.d_addr[0];
    if (ino.d_mode & ILARG) {
        seek(fd, blk, 3);
        if (read(fd, (char *)&blk, sizeof blk) != sizeof blk)
            return 0;
    }
    seek(fd, blk, 3);
    if (read(fd, (char *)e, sizeof e) != sizeof e)
        return 0;
    if (e[0].ino != ROOTINO || e[0].name[0] != '.' || e[0].name[1] != 0)
        return 0;
    return (e[1].ino == ROOTINO && e[1].name[0] == '.' &&
            e[1].name[1] == '.' && e[1].name[2] == 0);
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
