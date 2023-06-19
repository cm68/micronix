/*
 * account for used blocks
 *
 * icheck goes through every inode and accounts for allocated blocks
 * we flag duplicates
 *
 * tools/icheck.c
 * Changed: <2023-06-19 05:51:26 curt>
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>

#include "../micronix/include/types.h"
#include "../micronix/include/sys/fs.h"
#include "../micronix/include/sys/dir.h"
#include "../include/fslib.h"
#include "../include/util.h"

#define	NB	10

int traceflags;

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
int used;

int blist[10] = { -1 };

unsigned short buf[256];

int nerror;

void pass(struct dsknod *dp);
int blockcheck(int blkno, char *mesg);

struct super *fs;

int bmapsize;
unsigned char *bitmap; 

#define BITINDEX(b)   ((b) >> 3)
#define BITMASK(b)  (1 << ((b) & 7))

/*
 * a block can belong to either overhead, to an inode, or in the freelist.
 * we scan the freelist for duplicates, then we scan the data blocks,
 * and then if anything is busted, we rebuild the freelist from scratch
 * this requires a complete rescan of all the inodes
 */
void
check(file)
    char *file;
{
    int b;
    int i;
    struct dsknod *dp;
    unsigned short fl[100];

    i = openfs(file, &fs);
    if (i < 0) {
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
    used = 0;

    if (bitmap) {
        free(bitmap);
    }
    bmapsize = (fs->s_fsize + 7) / 8;
    bitmap = malloc(bmapsize);
    bzero(bitmap, bmapsize);

    /*
     * if we are rebuilding the freelist, no point in scanning it
     */
    if (sflg) {
        fs->s_nfree = 0;
        fs->s_free[0] = 0;
    }

    /* copy the freelist from superblock to fl */
    bcopy(fs->s_free, fl, sizeof(fl));
    i = fs->s_nfree - 1;

    /* scan through whole freelist from end */
    while ((b = fl[i]) != 0) {
        if (!i) {
            readblk(fs, b, (char *)buf);
            i = buf[0] - 1;
            bcopy(&buf[1], fl, sizeof(fl));
        } else { 
            i--;
        }
        blockcheck(b, "free");
        nfree++;
    }

    if (ndup) {
        printf("%d dups in free\n", ndup);
        nerror |= 02;
    }

    /*
     * process all the inodes - now we know all the used blocks
     */
    for (ino = 1; ino < fs->s_isize * I_PER_BLK; ino++) {
        dp = iget(fs, ino);
        pass(dp);
        ifree(dp);
    }

    /*
     * count the allocated blocks in the bitmap 
     */
    for (i = fs->s_isize + 2; i < fs->s_fsize; i++) {
#define BITINDEX(b)   ((b) >> 3)
#define BITMASK(b)  (1 << ((b) & 7))
        if (bitmap[BITINDEX(i)] & BITMASK(i)) {
            used++;
        } else {
            if (sflg) {
                bfree(fs, i);
            }
        }
    }

    if (used != (fs->s_fsize - (fs->s_isize + 2))) {
        printf("missing %5d %5d\n", used, (fs->s_fsize - (fs->s_isize + 2)));
    }
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
    closefs(fs);
}

/*
 * process the blocks of an inode and account for it
 */
void
pass(struct dsknod *dp)
{
    int i;
    int size;
    int b;

    if (!(dp->d_mode & IALLOC))
        return;

    if (dp->d_mode & IIO) {
        nspcl++;
        return;
    }

    if ((dp->d_mode & IFMT) == IFDIR)
        ndir++;
    else
        nfile++;

    size = (dp->d_size0 << 16) + dp->d_size1;
    for (i = 0; i < size; i += 512) {
        b = bmap(dp, i, 0);
        if (b) {
            blockcheck(b, "data");
        }
    }

    if (!(dp->d_mode & ILARG)) {
        return;
    }

    nlarg++;
    for (i = 0; i < 7; i++) {
        b = dp->d_addr[i];
        if (b) {
            blockcheck(b, "indirect");
            nindir++;
        }
    }
    b = dp->d_addr[7];
    if (!b) {
        return;
    }

    blockcheck(b, "double indirect");
    nvlarg++;
    readblk(fs, b, (UINT8 *)buf);
    for (i = 0; i < 256; i++) {
        b = buf[i];
        if (!b) {
            continue;
        }
        blockcheck(b, "2nd indirect");
        nvindir++;
    }
}

/*
 * check to see if a block is in the allocation bitmap
 * if so, complain.  if not, put it there
 */
int
blockcheck(blkno, mesg)
    UINT blkno;
    char *mesg;
{
    unsigned char mask;
    int index;
    int i;

    if (ino)
        nused++;

    if (blkno < fs->s_isize + 2 || blkno >= fs->s_fsize) {
        printf("%d bad; inode=%d, class=%s\n", blkno, ino,
            mesg);
        return (1);
    }

    mask = BITMASK(blkno);
    index = BITINDEX(blkno);
    if (index > bmapsize) {
        printf("lose: blkno %d bigger than bitmap\n", blkno);
    }

    if (bitmap[index] & mask) {
        printf("%d dup; inode=%d, class=%s\n", blkno, ino, mesg);
        ndup++;
    }

    for (i = 0; blist[i] != -1; i++) {
        if (blkno == blist[i]) {
            printf("%d arg; inode=%d, class=%s\n", blkno, ino, mesg);
        }
    }

    bitmap[index] |= mask;
    return (0);
}

void
usage(char *pname)
{
    fprintf(stderr, "usage:\n%s -[vsh] [-b <blocknum> ...] <filesystem>\n",
        pname);
    fprintf(stderr, "\t-s\trebuild freelist\n");
    fprintf(stderr, "\t-v\tbe verbose\n");
    fprintf(stderr, "\t-b\tlist of blocks to find in inodes\n");
    exit(1);
}

int
main(argc, argv)
    int argc;
    char **argv;
{
    char **p;
    int n, *lp;
    char *pname = argv[0];

    while (--argc) {
        argv++;
        if (**argv == '-')
            switch ((*argv)[1]) {
            case 'v':
                traceflags = -1;
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
            case 'h':
                usage(pname);
                break;
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
