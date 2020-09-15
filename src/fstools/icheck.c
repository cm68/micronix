
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "fs.h"

#define	NINODE	16*64

#define	NB	10

struct dsknod inode[NINODE];

#define	BMAPSIZE	8192
#define	MAXBLK		(BMAPSIZE * 8)

int sflg;

int nfile;
int nspcl;
int nlarg;
int nvlarg;
int nindir;
int nvindir;
int ndir;
int nused;
int nfree;
int ino;
int ndup;
int blist[10] = { -1 };

int nerror;
unsigned char bitmap[BMAPSIZE];

void pass1(struct dsknod *ip);

int chk(UINT blkno, UINT lblkno, char *mesg);

void
makefree()
{
    int i;

    fs->nfree = 0;
    fs->ninode = 0;
    fs->flock = 0;
    fs->ilock = 0;
    fs->time = 0;
    bfree(0);
    for (i = fs->fsize - 1; i >= fs->isize + 2; i--) {
        if ((bitmap[(i >> 3)] & (1 << (i & 0x7))) == 0)
            bfree(i);
    }
    writesuper();
}

void
check(file)
    char *file;
{
    int i, j, k;
    struct dsknod *ip;

    image = open(file, sflg ? 2 : 0);
    if (image < 0) {
        printf("cannot open %s\n", file);
        nerror |= 04;
        return;
    }
    printf("%s:\n", file);
    nfile = 0;
    nspcl = 0;
    nlarg = 0;
    nvlarg = 0;
    nindir = 0;
    nvindir = 0;
    ndir = 0;
    nused = 0;
    nfree = 0;
    ndup = 0;

    /*
     * clear the bitmap 
     */
    for (i = 0; i < BMAPSIZE; i++)
        bitmap[i] = 0;

    /*
     * process all the inodes 
     */
    readsuper();
    for (ino = 1; ino < fs->isize * I_PER_BLK; ino++) {
        ip = iget(ino);
        pass1(ip);
        ifree(ip);
    }

    if (verbose > 2)
        printf("freehead: %d\n", fs->free[0]);

    /*
     * check for freelist dups 
     */
    while (i = balloc()) {
        if (chk(i, 0, "free"))
            break;
        nfree++;
    }
    if (ndup) {
        printf("%d dups in free\n", ndup);
        nerror |= 02;
    }

    /*
     * rebuild the freelist 
     */
    if (sflg) {
        makefree();
        return;
    }

    /*
     * count the allocated blocks in the bitmap 
     */
    j = 0;
    for (i = 0; i < BMAPSIZE; i++) {
        for (k = bitmap[i]; k; k >>= 1)
            j++;
    }
    if (j != fs->fsize - fs->isize + 3)
        printf("missing %5d %5d\n", j, fs->fsize - fs->isize + 3);
    printf("spcl  %6d\n", nspcl);
    printf("files %6d\n", nfile);
    printf("large %6d\n", nlarg);
    if (nvlarg)
        printf("huge  %6d\n", nvlarg);
    printf("direc %6d\n", ndir);
    printf("indir %6d\n", nindir);
    if (nvindir)
        printf("indir2%6d\n", nvindir);
    printf("used  %6d\n", nused);
    printf("free  %6d\n", nfree);
    close(image);
}

void
pass1(struct dsknod *ip)
{
    UINT buf[256];              // indirect block
    UINT vbuf[256];             // double indirect
    int i, j;

    if (!(ip->mode & IALLOC))
        return;

    if (ip->mode & IIO) {
        nspcl++;
        return;
    }

    if ((ip->mode & ITYPE) == IDIR)
        ndir++;
    else
        nfile++;

    if (verbose)
        idump(ip);

    if ((ip->mode & ILARGE) != 0) {
        nlarg++;
        for (i = 0; i < 7; i++)
            if (ip->addr[i] != 0) {
                nindir++;
                if (chk(ip->addr[i], i * 256, "indirect"))
                    continue;
                readblk(ip->addr[i], (UCHAR *)buf);
                if (verbose)
                    secdump((UCHAR *)buf);
                for (j = 0; j < 256; j++)
                    if (buf[j] != 0)
                        chk(buf[j], i * 256 + j, "data (large)");
            }
        if (ip->addr[7]) {
            nvlarg++;
            if (chk(ip->addr[7], 7 * 256, "indirect"))
                return;
            readblk(ip->addr[7], (UCHAR *)buf);
            if (verbose)
                secdump((UCHAR *)buf);
            for (i = 0; i < 256; i++)
                if (buf[i] != 0) {
                    nvindir++;
                    if (chk(buf[i], (7 + i) * 256, "2nd indirect"))
                        continue;
                    readblk(buf[i], (UCHAR *)vbuf);
                    if (verbose)
                        secdump((UCHAR *)vbuf);
                    for (j = 0; j < 256; j++)
                        if (vbuf[j])
                            chk(vbuf[j], (7 + i) * 256 + j,
                                "data (very large)");
                }
        }
        return;
    }
    for (i = 0; i < 8; i++) {
        if (ip->addr[i] != 0)
            chk(ip->addr[i], i, "data (small)");
    }
}

/*
 * check to see if a block is in the allocation bitmap
 */
int
chk(blkno, lblkno, mesg)
    UINT blkno;
    UINT lblkno;
    char *mesg;
{
    unsigned char mask;
    int index;
    int i;

    if (ino)
        nused++;

    if (blkno < fs->isize + 2 || blkno >= fs->fsize) {
        printf("%d bad; inode=%d, lblkno=%d class=%s\n", blkno, ino, lblkno,
            mesg);
        return (1);
    }

    mask = 1 << (blkno & 0x07);
    index = blkno >> 3;
    if (index > sizeof(bitmap))
        printf("lose: blkno %d bigger than bitmap\n", blkno);

    if (bitmap[index] & mask) {
        printf("%d dup; inode=%d, lblkno=%d class=%s\n", blkno, ino, lblkno,
            mesg);
        ndup++;
    }
    bitmap[index] |= mask;
    for (i = 0; blist[i] != -1; i++)
        if (blkno == blist[i])
            printf("%d arg; inode=%d, lblkno=%d class=%s\n", blkno, ino,
                lblkno, mesg);
    return (0);
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
        if (**argv == '-')
            switch ((*argv)[1]) {
            case 'v':
                verbose++;
                continue;
            case 's':
                sflg++;
                continue;

            case 'b':
                lp = blist;
                while (lp < &blist[NB - 1] && (n = atoi(argv[1]))) {
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
