#include <unistd.h>
#include <stdio.h>
#include "fs.h"

UCHAR fsbuf[512];

#define	NB 10
int blist[NB] = { -1 };

UINT flist[256];

void
dumpflist(UINT * fl, int count)
{
    int i;

    printf("count: %d", count);
    for (i = 0; i < count; i++) {
        if ((i % 10) == 0)
            printf("\n");
        printf("%6d ", fl[i]);
    }
    printf("\n");
}

int
main(int argc, char **argv)
{
    int i;
    int k;
    struct dsknod *ip;
    struct sup *fs;
    int *lp = 0;

    while (--argc) {
        argv++;
        if (**argv == '-')
            switch ((*argv)[1]) {
            case 'v':
                verbose++;
                continue;

            case 'i':
                lp = blist;
                while (lp < &blist[NB - 1] && (i = atoi(argv[1]))) {
                    *lp++ = i;
                    argv++;
                    argc--;
                }
                *lp++ = -1;
                continue;

            default:
                printf("Bad flag\n");
            }
    }

    if (verbose > 2) {
        for (i = 0; i < 17; i++) {
            printf("%d -> %d\n", i, secmap(i));
        }
    }

    putenv("TZ=GMT");
    tzset();

    if ((image = open(*argv, O_RDONLY)) < 0)
        lose("open");

    readblk(1, fsbuf);
    fs = (struct sup *) fsbuf;

    dumpsb(fs);

    printf("free blocks:");
    dumpflist(&fs->free[0], fs->nfree);
    k = fs->free[0];
    while (k) {
        printf("chained to: \n");
        readblk(k, (char *) flist);
        if (verbose > 2)
            secdump((char *) flist);
        dumpflist(&flist[1], flist[0]);
        k = flist[1];
    }

    printf("\n");

    printf("free inodes:");
    for (i = 0; i < 100; i++) {
        if ((i % 10) == 0)
            printf("\n");
        printf("%6d ", fs->inode[i]);
    }
    printf("\n");

    if (lp) {
        for (k = 0; blist[k] != -1; k++) {
            i = blist[k];
            ip = iget(i);
            idump(i, ip);
        }
    } else {
        for (i = 1; i < fs->isize * I_PER_BLK; i++) {
            ip = iget(i);
            idump(i, ip);
        }

    }
    return (0);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
