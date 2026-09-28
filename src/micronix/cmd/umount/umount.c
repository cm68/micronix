/*
 * umount - dismount a filesystem
 *
 * cmd/umount/umount.c
 *
 * THIS IS A RECONSTRUCTION.  There is no surviving source for
 * /bin/umount; this file is written from the v7 umount
 * (extra/v7/usr/src/cmd/umount.c) and the strings and shape of the
 * /bin/umount binary.  The mtab layout and the umount(2) call are
 * Micronix's.
 *
 * umount special, or umount directory: sync, call umount(2) on the
 * special, and drop its entry from /etc/mtab.  The v7 program took only
 * the special; naming the mount point instead is looked up in
 * /etc/mtab, which records both halves of the mount.
 *
 * vim: tabstop=4 shiftwidth=4 noexpandtab:
 */

#include <types.h>
#include <stdio.h>
#include <string.h>
#include <mtab.h>

#define NMOUNT 4		/* the kernel's NMOUNT (sys/sys.h) */

struct mtab mtab[NMOUNT];

void delmtab();
char *specialof();

main(argc, argv)
    int argc;
    char **argv;
{
    char *name;
    char *special;
    int mf;

    if (argc != 2) {
        fprintf(stderr, "usage: umount special | directory\n");
        exit(1);
    }

    sync();

    mf = open("/etc/mtab", 0);
    if (mf >= 0) {
        read(mf, (char *)mtab, sizeof mtab);
        close(mf);
    }

    if ((special = specialof(argv[1])) == 0)
        special = argv[1];

    if ((name = strrchr(special, '/')) != 0)
        name++;
    else
        name = special;

    if (umount(special) < 0) {
        perror("umount");
        exit(1);
    }
    delmtab(name);
    exit(0);
}

/*
 * The special file to dismount.  The argument is either the special
 * itself ("/dev/dja") or the directory it was mounted on ("/mnt"), and
 * the mount table holds both.  A mount point is a full path, so it is
 * matched whole; a special is matched on its last element, which is how
 * the table stores it.  A mount point has to give back the path in /dev,
 * since the table keeps only the name.
 *
 * A name in neither column goes back to the caller, which passes it on to
 * umount(2) - a mounted filesystem missing from a stale /etc/mtab is still
 * the kernel's to dismount.
 */
char *
specialof(arg)
    char *arg;
{
    static char path[sizeof "/dev/" + sizeof mtab[0].special];
    char *name;
    int i;

    for (i = 0; i < NMOUNT; i++) {
        if (mtab[i].directory[0] == 0)
            continue;
        if (strcmp(mtab[i].directory, arg) == 0) {
            strcpy(path, "/dev/");
            strcat(path, mtab[i].special);
            return path;
        }
    }

    if ((name = strrchr(arg, '/')) != 0)
        name++;
    else
        name = arg;

    for (i = 0; i < NMOUNT; i++) {
        if (mtab[i].directory[0] == 0)
            continue;
        if (strcmp(mtab[i].special, name) == 0)
            return arg;
    }

    return 0;
}

/*
 * Drop name from the mount table and rewrite /etc/mtab.
 */
void
delmtab(name)
    char *name;
{
    int i;
    int mf;

    for (i = 0; i < NMOUNT; i++) {
        if (mtab[i].directory[0] == 0)
            continue;
        if (strcmp(mtab[i].special, name) != 0)
            continue;
        mtab[i].directory[0] = 0;
        mtab[i].special[0] = 0;
        break;
    }

    if ((mf = creat("/etc/mtab", 0644)) < 0)
        return;
    write(mf, (char *)mtab, sizeof mtab);
    close(mf);
}
