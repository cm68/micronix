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
 * umount special: sync, call umount(2) on the special, and drop its
 * entry from /etc/mtab.
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

main(argc, argv)
    int argc;
    char **argv;
{
    char *name;
    int mf;

    if (argc != 2) {
        fprintf(stderr, "usage: umount special\n");
        exit(1);
    }

    sync();

    mf = open("/etc/mtab", 0);
    if (mf >= 0) {
        read(mf, (char *)mtab, sizeof mtab);
        close(mf);
    }

    if ((name = strrchr(argv[1], '/')) != 0)
        name++;
    else
        name = argv[1];

    if (umount(argv[1]) < 0) {
        perror("umount");
        exit(1);
    }
    delmtab(name);
    exit(0);
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
