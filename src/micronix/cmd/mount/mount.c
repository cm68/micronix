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
 * The three forms:
 *	mount				list /etc/mtab
 *	mount special directory [-r]	mount the special on the directory
 *	mount directory			mount the device /etc/fstab names
 *	mount -a			mount every entry in /etc/fstab
 *
 * /etc/fstab is one entry a line, a device and its mount point, in
 * either order of the two columns is not allowed - device first, then
 * directory, the way mount(2) takes them.  A device without a leading
 * slash is taken to live in /dev.
 *
 * vim: tabstop=4 shiftwidth=4 noexpandtab:
 */

#include <types.h>
#include <stdio.h>
#include <string.h>
#include <sys/fs.h>
#include <sys/dir.h>
#include <mtab.h>

#define NMOUNT 10		/* the kernel's NMOUNT (sys/sys.h) */
#define SUPERBLK 1
#define ILISTBLK (SUPERBLK + 1)	/* the I-list follows the superblock */
#define ROOTINO 1
#define MAXFSTAB 8

struct mtab mtab[NMOUNT];
struct super sblock;

/* the fstab, read once into these */
char fstab_dev[MAXFSTAB][32];
char fstab_dir[MAXFSTAB][32];
int nfstab;

void list();
int checkfs();
int rootdir();
void addmtab();
void readfstab();
char *devof();
char *specialpath();
int ismounted();
int domount();
void mountall();

main(argc, argv)
    int argc;
    char **argv;
{
    int ro;
    int mf;
    char *dev;

    mf = open("/etc/mtab", 0);
    if (mf >= 0) {
        read(mf, (char *)mtab, sizeof mtab);
        close(mf);
    }

    if (argc == 1) {
        list();
        exit(0);
    }
    if (argc == 2 && strcmp(argv[1], "-a") == 0) {
        readfstab();
        mountall();
        exit(0);
    }
    if (argc == 2) {
        readfstab();
        if ((dev = devof(argv[1])) == 0) {
            fprintf(stderr, "%s: no entry in fstab\n", argv[1]);
            exit(1);
        }
        exit(domount(dev, argv[1], 0));
    }
    if (argc == 3 || argc == 4) {
        ro = (argc == 4);
        exit(domount(argv[1], argv[2], ro));
    }
    fprintf(stderr, "usage: mount [special directory [-r]] | directory | -a\n");
    exit(1);
}

/*
 * Mount one device on one directory: check the filesystem, mount it
 * (falling back to read-only if a read-write mount is refused), and
 * record the mount in /etc/mtab.  Returns 0 on success.
 */
int
domount(dev, dir, ro)
    char *dev, *dir;
    int ro;
{
    dev = specialpath(dev);

    if (!checkfs(dev)) {
        fprintf(stderr, "%s: not a file system\n", dev);
        return 1;
    }
    if (mount(dev, dir, ro) < 0) {
        /*
         * Maybe the only problem was a read-only diskette: retry the
         * mount read-only before giving up, the way the original did.
         */
        if (!ro) {
            ro = 1;
            if (mount(dev, dir, ro) < 0) {
                perror("mount");
                return 1;
            }
        } else {
            perror("mount");
            return 1;
        }
    }
    addmtab(dev, dir);
    return 0;
}

/*
 * Mount everything /etc/fstab names, in order, skipping the entries
 * that are already mounted.  A failure is reported and the rest go on.
 */
void
mountall()
{
    int i;

    for (i = 0; i < nfstab; i++) {
        if (ismounted(fstab_dev[i]))
            continue;
        domount(fstab_dev[i], fstab_dir[i], 0);
    }
}

/*
 * Read /etc/fstab: one entry a line, device then mount point, with
 * blank lines and # comments skipped.
 */
void
readfstab()
{
    FILE *f;
    char line[128];
    char *p, *dev, *dir;

    nfstab = 0;
    f = fopen("/etc/fstab", "r");
    if (f == NULL)
        return;
    while (nfstab < MAXFSTAB && fgets(line, sizeof line, f) != NULL) {
        p = line;
        while (*p == ' ' || *p == '\t')
            p++;
        if (*p == '\0' || *p == '\n' || *p == '#')
            continue;
        dev = p;
        while (*p != ' ' && *p != '\t' && *p != '\n' && *p != '\0')
            p++;
        *p++ = '\0';
        while (*p == ' ' || *p == '\t')
            p++;
        if (*p == '\0' || *p == '\n')
            continue;
        dir = p;
        while (*p != ' ' && *p != '\t' && *p != '\n' && *p != '\0')
            p++;
        *p = '\0';
        strcpy(fstab_dev[nfstab], dev);
        strcpy(fstab_dir[nfstab], dir);
        nfstab++;
    }
    fclose(f);
}

/*
 * The device /etc/fstab pairs with this mount point, or 0.
 */
char *
devof(dir)
    char *dir;
{
    int i;

    for (i = 0; i < nfstab; i++)
        if (strcmp(fstab_dir[i], dir) == 0)
            return fstab_dev[i];
    return 0;
}

/*
 * The device as mount(2) wants it: a name without a slash lives in /dev.
 */
char *
specialpath(name)
    char *name;
{
    static char path[sizeof "/dev/" + 32];

    if (name[0] == '/')
        return name;
    strcpy(path, "/dev/");
    strcat(path, name);
    return path;
}

/*
 * Is this device (by its bare name, "dja" not "/dev/dja") mounted?
 */
int
ismounted(name)
    char *name;
{
    char *n;
    int i;

    if ((n = strrchr(name, '/')) != 0)
        n++;
    else
        n = name;
    for (i = 0; i < NMOUNT; i++)
        if (mtab[i].directory[0] && strcmp(mtab[i].special, n) == 0)
            return 1;
    return 0;
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
