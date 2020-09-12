
/*
 * common code for micronix filesystem maintenance utilities
 * brute force for everything
 */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fs.h"

int spt = 15;
int altsec = 1;
int image;
int verbose;

/*
 * do sector skew
 */
UINT
secmap(UINT blkno)
{
    int trk = blkno / spt;
    int sec = blkno % spt;

    if (altsec) {
        sec <<= 1;
        if (!(spt & 1) && sec >= spt) {
            sec++;
        }
        sec %= spt;
    }
    return (trk * spt + sec);
}

void
lose(char *s)
{
    fprintf(stderr, "%s", s);
    exit(1);
}

char *itype[] = {
    "IFREG",
    "CDEV",
    "DIR",
    "BDEV"
};

struct sup *fs;
struct dsknod *ip;
struct dir *dp;
char dbuf[512];

/*
 * a kinda cute hexdump
 */
void
dump(unsigned char *b, int size)
{
    int i;
    int c;
    unsigned char cb[17];

    if (size > 512)
        size = 512;

    cb[0] = cb[16] = 0;

    for (i = 0; i < size; i++) {
        if (i % 16 == 0)
            printf("%s\n%3x: ", cb, i);
        c = b[i];
        printf("%02x ", c);
        if (c < ' ' || c >= 127)
            c = '.';
        cb[i % 16] = c;
    }
    while (i % 16) {
        printf("   ");
        cb[i % 16] = ' ';
        i++;
    }
    printf("%s\n", cb);
}

void
secdump(unsigned char *buf)
{
    dump(buf, 512);
}

/*
 * print out a directory block
 */
void
dirdump(char *buf, int size)
{
    struct dir *dp;

    if (verbose)
        fprintf(stderr, "dirdump: %d\n", size);
    if (size > 512)
        size = 512;
    for (dp = (struct dir *) buf; dp < (struct dir *) &buf[size]; dp++) {
        printf("%5d: %14s\n", dp->inum, dp->name);
    }
}

/*
 * get the contents of a block
 */
int
readblk(int blkno, char *buf)
{
    int realblk;

    realblk = secmap(blkno);
    if (verbose > 1)
        fprintf(stderr, "readblk: %d -> %d\n", blkno, realblk);

    if (lseek(image, 512 * realblk, SEEK_SET) < 0)
        lose("readblk seek");
    if (read(image, buf, 512) != 512)
        lose("readblk");
    return 0;
}

/*
 * scribble the contents of a block
 */
int
writeblk(int blkno, char *buf)
{
    int realblk;

    realblk = secmap(blkno);
    if (verbose > 1)
        fprintf(stderr, "writeblk: %d -> %d\n", blkno, realblk);

    if (lseek(image, 512 * realblk, SEEK_SET) < 0)
        lose("writeblk seek");
    if (write(image, buf, 512) != 512)
        lose("writeblk");
    return 0;
}

ULONG
timeswap(ULONG x)
{
    ULONG ret;
    union
    {
        ULONG t;
        UINT tw[2];
        UCHAR tb[4];
    } tu;

    tu.t = x;

    ret = tu.tb[1];
    ret |= tu.tb[0] << 8;
    ret |= tu.tb[3] << 16;
    ret |= tu.tb[2] << 24;

    return ret;
}

/*
 * micronix timestamps seem to longs in a bizarre byte order
 */
char *
mytime(ULONG t)
{
    static char timebuf[100];
    union
    {
        ULONG t;
        UCHAR tb[4];
    } tu;
    time_t t1;

    tu.t = t;
    t1 = tu.tb[2];
    t1 |= tu.tb[3] << 8;
    t1 |= tu.tb[0] << 16;
    t1 |= tu.tb[1] << 24;

    // sprintf(timebuf, "%x %x %s", t, t1, ctime(&t1));
    sprintf(timebuf, "%s", ctime(&t1));
    return timebuf;
}

void
dumpsb(struct sup *fs)
{
    printf("isize: %d fsize: %d nfree: %d ninode: %d time: %s\n",
        fs->isize, fs->fsize, fs->nfree, fs->ninode,
        mytime(timeswap(fs->time)));
}

/*
 * read an inode, given the inum
 */
struct dsknod *
iget(int inum)
{
    int iblk;
    int offset;
    struct dsknod *dp = malloc(sizeof(struct dsknod));

    inum--;

    iblk = secmap(inum / I_PER_BLK + INODES_START);
    offset = iblk * 512 + (inum % I_PER_BLK) * 32;
    lseek(image, offset, SEEK_SET);
    if (verbose > 2)
        fprintf(stderr, "iget: %d %x\n", inum, offset);

    if (read(image, (char *) dp, sizeof(*dp)) < 0)
        lose("inode read");
    return dp;
}

void
blocklist(UINT * buf)
{
    int i;

    for (i = 0; i < 256; i++) {
        if ((i % 16) == 0) {
            printf("\n");
        }
        printf("%5d ", buf[i]);
    }
    printf("\n");
}

/*
 * print out an inode, raw
 */
void
idump(int ino, struct dsknod *ip)
{
    int i;
    int j;
    unsigned int ft;
    unsigned int size;
    UINT indir[256];
    UINT dindir[256];

    if (verbose > 1)
        fprintf(stderr, "idump %d\n", i);

    if (!(ip->mode & IALLOC))
        return;

    ft = (ip->mode & ITYPE) >> 13;
    printf("inode %5d: %6o %d %s\n", ino, ip->mode, ft, itype[ft & 3]);

    size = (ip->size0 << 16) + ip->size1;

    printf("\tmode: %o links: %d uid: %d gid: %d size: %d %s\n",
        ip->mode, ip->nlinks, ip->uid, ip->gid,
        size, ip->mode & ILARGE ? "LARGE" : "");

    printf("\trtime: %s ", mytime(ip->rtime));
    printf("wtime: %s\n", mytime(ip->wtime));

    if (ip->mode & IIO) {
        int maj = (ip->addr[0] >> 8) & 0xff;
        int min = ip->addr[0] & 0xff;

        printf("\tmaj: %d min: %d\n", maj, min);
    } else {
        printf("\tblocks:\n\t\t");
        for (i = 0; i < 8; i++) {
            printf("%d ", ip->addr[i]);
            if (ip->mode & ILARGE) {
                if (!ip->addr[i])
                    continue;
                printf("indirect\n");
                readblk(ip->addr[i], (char *) indir);
                blocklist(indir);
            }
        }
        if ((ip->mode & ILARGE) && ip->addr[7]) {
            printf("double indirect\n");
            readblk(ip->addr[7], (char *) dindir);
            blocklist(dindir);
            for (i = 0; i < 256; i++) {
                printf("indirect %d\n", i);
                readblk(dindir[i], (char *) indir);
                blocklist(indir);
            }
        }
        printf("\n");
    }
    if ((ip->mode & ITYPE) == IDIR) {
        for (i = 0; i < 8; i++) {
            if (i * 512 > size)
                break;
            readblk(ip->addr[i], dbuf);
            dirdump(dbuf, size - (i * 512));
        }
    }
}

#define DENTS 32

static struct dir dirbuf[DENTS];
static int dblk = -1;

/*
 * return a pointer to the i'th directory entry
 */
struct dir *
getdirent(struct dsknod *ip, int i)
{
    int b;

    if (i * 16 > ((ip->size0 << 16) + ip->size1)) {
        return 0;
    }
    b = bmap(ip, i * 16, 0);
    if (dblk != b) {
        dblk = b;
        readblk(dblk, (char *) dirbuf);
    }
    return &dirbuf[i % DENTS];
}

int
lookup(struct dsknod *ip, char *name)
{
    int i;
    struct dir *dp;

    if ((ip->mode & ITYPE) != IDIR) {
        return 0;
    }

    for (i = 0; i < ((ip->size0 << 16) + ip->size1) / 16; i++) {
        dp = getdirent(ip, i);
        if (strncmp(dp->name, name, 14) == 0) {
            // printf("%5d: %14s\n", dp->inum, dp->name);
            return dp->inum;
        }
    }
    return 0;
}

#ifdef notdef
struct dir *
dread(struct dsknod *ip, int off)
{
    register b;
    static UINT ibuf[256];
    static UCHAR buf[512];

    if (off > (ip->size1 + (ip->size0 << 16)))
        return (0);
    b = bmap(ip, off, 0);

    if ((off & 0777) == 0) {
        if (off == 0177000) {
            printf("Monstrous directory\n");
            return (0);
        }
        if ((ip->mode & ILARGE) == 0) {
            if (off >= 010000 || (b = ip->addr[off >> 9]) == 0)
                return (0);
            readblk(b, buf);
        } else {
            if (off == 0) {
                if (ip->addr[0] == 0)
                    return (0);
                readblk(ip->addr[0], (char *) ibuf);
            }
            if ((b = ibuf[(off >> 9) & 0177]) == 0)
                return (0);
            readblk(b, buf);
        }
    }

    return (&buf[off & 0777]);
}
#endif

UINT iblk[256];
int iblkno = -1;

int
bmap(struct dsknod *ip, int offset, int alloc)
{
    int blkno = offset / 512;
    int i;

#ifdef notdef
    if ((off & 0777) == 0) {
        if (off == 0177000) {
            printf("Monstrous directory\n");
            return (0);
        }
        if ((ip->mode & ILARGE) == 0) {
            if (off >= 010000 || (b = ip->addr[off >> 9]) == 0)
                return (0);
            readblk(b, buf);
        } else {
            if (off == 0) {
                if (ip->addr[0] == 0)
                    return (0);
                readblk(ip->addr[0], (char *) ibuf);
            }
            if ((b = ibuf[(off >> 9) & 0177]) == 0)
                return (0);
            readblk(b, buf);
        }
    }
#endif

    if (ip->mode & ILARGE) {
        i = blkno / 256;
        if (i < 7) {
            if (iblkno != ip->addr[i]) {
                iblkno = ip->addr[i];
                readblk(iblkno, (char *) iblk);
            }
            blkno = iblk[blkno % 256];
        } else {
            printf("double indirect\n");
        }
    } else {
        if (offset > (512 * 8)) {
            printf("bmap large range problem\n");
            return 0;
        }
        blkno = ip->addr[offset / 512];
    }
    return blkno;
}

/*
 * read a block at the logical file offset
 * offset is block aligned
 */
int
fileread(struct dsknod *ip, int offset, char *buf)
{
    int size = (ip->size0 << 16) + ip->size1;
    int valid = size - offset;

    readblk(bmap(ip, offset, 0), buf);
    if (valid > 512) {
        valid = 512;
    }
    return valid;
}

/*
 * write a block into a file.  
 * rewrites the block if it is allocated already,
 * and allocates a block if it needs to.
 * we might write the inode and superblock
 */
int
filewrite(struct dsknod *ip, int offset, char *buf)
{
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
