
/*
 * this is the V6 dcheck - validate the link count
 * it simply does a pass through all directories, counting file references.
 * then, it does another pass verifying that the link count is the same as 
 * the reference count
 */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "../micronix/include/types.h"
#include "../micronix/include/sys/sup.h"
#include "../micronix/include/sys/inode.h"
#include "../micronix/include/sys/dir.h"
#include "../include/fslib.h"

int traceflags;
int headpr;

#define NI 20

/*
 * a list of inumbers to look for on the command line 
 */
int iilist[NI] = { -1 };

int ino;
int nerror;
int nfiles;

char *ecount;

struct sup *fs;

/*
 * given a directory inode, increment the link count on the inumber in ecount
 */
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

    // read every directory entry
    for (doff = 0; (dp = getdirent(ip, doff)) != 0; doff++) {
        if (dp->ino == 0)
            continue;
        // if we are trying to find an inode name, print it
        for (i = 0; iilist[i] != -1; i++) {
            if (iilist[i] == dp->ino) {
                printf("%5d %d/%.14s\n", dp->ino, ino, dp->name);
            }
        }
        ecount[dp->ino]++;
    }
}

/*
 * given an inode, check the link count against ecount
 */
void
pass2(struct dsknod *ip)
{
    int i;

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
    struct dsknod *dp;
    int ret;

    ret = openfs(file, &fs);
    if (ret < 0) {
        printf("cannot open %s\n", file);
        return;
    }

    printf("%s:\n", file);

    nfiles = fs->isize * 16;

    ecount = (char *)malloc(nfiles);
    for (ino = 1; ino < nfiles; ino++)
        ecount[ino] = 0;

    for (ino = 1; ino < nfiles; ino++) {
        dp = iget(fs, ino);
        pass1(dp);
        ifree(dp);
    }
    headpr = 0;
    for (ino = 1; ino < nfiles; ino++) {
        dp = iget(fs, ino);
        pass2(dp);
        ifree(dp);
    }
    free(ecount);
    closefs(fs);
}

int
main(argc, argv)
    int argc;
    char **argv;
{
    char **p;
    int n, *lp;

    while (--argc) {
        argv++;
        if (**argv == '-') switch ((*argv)[1]) {
        case 'v':
            traceflags = -1;
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
