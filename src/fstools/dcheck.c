
/*
 * this is the V6 dcheck
 * it simply does a pass through all directories, counting file references.
 * then, it does another pass verifying that the link count is the same as the reference count
 */
#include <stdlib.h>
#include <stdio.h>
#include "fs.h"

#define	NINODE	16 * 64

struct dsknod inode[NINODE];
union
{
    struct sup sbl;
    char buf[512];
} sblu;

#define sblock sblu.sbl

int sflg;
int headpr;

#define NI 20

int iilist[NI] = { -1 };

char ecount[NINODE];
int ino;
int nerror;
int nfiles;

void
pass1(struct dsknod *ip)
{
    int doff;
    struct dir *dp;
    int i;

    if ((ip->mode & IALLOC) == 0)
        return;
    if ((ip->mode & ITYPE) != IDIR)
        return;
    doff = 0;
    for (doff = 0; (dp = getdirent(ip, doff)) != 0; doff++) {
        if (dp->inum == 0)
            continue;
        for (i = 0; iilist[i] != -1; i++)
            if (iilist[i] == dp->inum)
                printf("%5d %d/%.14s\n", dp->inum, ino, dp->name);
        ecount[dp->inum]++;
    }
}

void
pass2(struct dsknod *ip)
{
    register i;

    i = ino;
    if ((ip->mode & IALLOC) == 0 && ecount[i] == 0)
        return;
    if (ip->nlinks == ecount[i] && ip->nlinks != 0)
        return;
    if (headpr == 0) {
        printf("entries	link cnt\n");
        headpr++;
    }
    printf("%d	%d	%d\n", ino, ecount[i], ip->nlinks);
}

void
check(file)
    char *file;
{
    image = open(file, 0);
    if (image < 0) {
        printf("cannot open %s\n", file);
        return;
    }
    headpr = 0;
    printf("%s:\n", file);
    sync();
    readblk(1, (char *) &sblock);
    nfiles = sblock.isize * 16;
    for (ino = 1; ino < nfiles; ino++)
        ecount[ino] = 0;
    for (ino = 1; ino < nfiles; ino++) {
        pass1(iget(ino));
    }
    for (ino = 1; ino < nfiles; ino++) {
        pass2(iget(ino));
    }
}

main(argc, argv)
    char **argv;
{
    char **p;
    int n, *lp;

    while (--argc) {
        argv++;
        if (**argv == '-')
            switch ((*argv)[1]) {
            case 'v':
                verbose++;
                continue;
            case 's':
                sflg++;
                continue;

            case 'i':
                lp = iilist;
                while (lp < &iilist[NI - 1] && (n = atoi(argv[1]))) {
                    *lp++ = n;
                    argv++;
                    argc--;
                }
                *lp++ = -1;
                continue;

            default:
                printf("Bad flag\n");
            }
        check(*argv);
    }
    return (nerror);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
