/*
 * common code for micronix filesystem maintenance utilities
 * brute force for everything
 */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fs.h"

char superblock[512];

struct sup *fs = (struct sup *)superblock;

int spt = 15;
int altsec = 1;
int image;
int verbose;

void
readsuper()
{
    readblk(1, superblock);
}

void
writesuper()
{
    if (superdirty) {
        writeblk(1, superblock);
    }
    superdirty = 0;
}

int superdirty;

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

int
filesize(struct dsknod *ip)
{
    return (ip->size0 << 16) + ip->size1;
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
 * zero out a block
 */
void
blkzero(char *buf)
{
    int i;
    for (i = 0; i < 512; i++) {
        buf[i] = 0;
    }
}

/*
 * get the contents of a block
 */
int
readblk(int blkno, char *buf)
{
    int realblk;

    /* block zero always reads 0 */
    if (blkno == 0) {
        blkzero(buf);
        return 0;
    }
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
 * we wrap the on-disk inode to put an inumber
 */
struct i_node {
    struct dsknod ondisk;
    int inum;
};

/*
 * read an inode, given the inum
 */
struct dsknod *
iget(int inum)
{
    int iblk;
    int offset;
    struct i_node *ip = malloc(sizeof(struct i_node));

    ip->inum = inum;

    inum--;

    iblk = secmap(inum / I_PER_BLK + INODES_START);
    offset = iblk * 512 + (inum % I_PER_BLK) * 32;
    lseek(image, offset, SEEK_SET);
    if (verbose > 2)
        fprintf(stderr, "iget: %d %x\n", inum, offset);

    if (read(image, (char *) &ip->ondisk, sizeof(struct dsknod)) < 0)
        lose("inode read");
    return &ip->ondisk;
}

void
iput(struct dsknod *dp)
{
    int iblk;
    int offset;
    struct i_node *ip = (struct i_node *)dp;
    int inum = ip->inum;

    inum--;

    iblk = secmap(inum / I_PER_BLK + INODES_START);
    offset = iblk * 512 + (inum % I_PER_BLK) * 32;
    lseek(image, offset, SEEK_SET);
    if (verbose > 2)
        fprintf(stderr, "iput: %d %x\n", inum, offset);

    if (write(image, (char *) &ip->ondisk, sizeof(struct dsknod)) < 0)
        lose("inode write");
}

void
ifree(struct dsknod *dp)
{
    struct i_node *ip = (struct i_node *)dp;
    free(ip);
}

/*
 * scribble the contents of a block
 */
int
writeblk(int blkno, char *buf)
{
    int realblk;
    int ret;

    if (blkno == 0) {
        lose("writeblk 0");
        return 0;
    }

    realblk = secmap(blkno);
    if (verbose > 1)
        fprintf(stderr, "writeblk: %d -> %d\n", blkno, realblk);

    if (lseek(image, 512 * realblk, SEEK_SET) < 0)
        lose("writeblk seek");
    if ((ret = write(image, buf, 512)) != 512) {
        printf("write ret %d\n", ret);
        lose("writeblk");
    }
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

void
printperm(int m, int set)
{
    char mb[4];

    strcpy(mb, "---");
    if (m & 0x4)
        mb[0] = 'r';
    if (m & 0x2)
        mb[1] = 'w';
    if (m & 0x1)
        mb[2] = set ? 's' : 'x';
    printf("%s", mb);
}

void
ilist(char *name, struct dsknod *ip)
{
    printf("%5d ", ((struct i_node *)ip)->inum);
    printf("%c", "-cdb"[(ip->mode >> 13) & 3]);
    printperm(ip->mode >> 6, ip->mode & ISETUID);
    printperm(ip->mode >> 3, ip->mode & ISETGID);
    printperm(ip->mode, 0);
    printf("%3d ", ip->nlinks);
    printf("%3d %3d ", ip->uid, ip->gid);
    if (ip->mode & IIO) {
        printf("%3d,%3d ", (ip->addr[0] >> 8) & 0xff, ip->addr[0] & 0xff);
    } else {
        printf("%7d ", filesize(ip));
    }
    printf("%14s\n", name);
}

/*
 * print out an inode, raw
 */
void
idump(struct dsknod *ip)
{
    int i;
    int j;
    unsigned int ft;
    unsigned int size;
    UINT indir[256];
    UINT dindir[256];
    int ino = ((struct i_node *)ip)->inum;

    if (verbose > 1)
        fprintf(stderr, "idump %d\n", i);

    if (!(ip->mode & IALLOC))
        return;

    ft = (ip->mode & ITYPE) >> 13;
    printf("inode %5d: %6o %d %s\n", ino, ip->mode, ft, itype[ft & 3]);

    size = filesize(ip);

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
 * snarf an entire directory
 */
struct dir *
getdir(struct dsknod *ip)
{
    struct dir *dp;
    int i;
    int size = filesize(ip);

    // round up to the containing block
    if (size % 512) size = (size | 511) + 1;
    dp = malloc(size);

    for (i = 0; i < size; i += 512) {
        readblk(bmap(ip, i, 0), (UCHAR *)&dp[i / 16]);
    }
    return dp;
}

/*
 * update an entire directory
 */
void
putdir(struct dsknod *ip, struct dir *dp)
{
    int i;
    int size = filesize(ip);

    for (i = 0; i < size; i += 512) {
        writeblk(bmap(ip, i, 1), (UCHAR *)&dp[i / 16]);
    }
}

/*
 * return a pointer to the i'th directory entry
 */
struct dir *
getdirent(struct dsknod *ip, int i)
{
    int b;

    if (i * 16 > filesize(ip)) {
        return 0;
    }
    b = bmap(ip, i * 16, 0);
    if (dblk != b) {
        dblk = b;
        readblk(dblk, (char *) dirbuf);
    }
    return &dirbuf[i % DENTS];
}

/*
 * look for an entry in a directory
 */
int
lookup(struct dsknod *ip, char *name)
{
    int inum = 0;
    struct dir *dp;
    int entries = filesize(ip) / 16;
    int i;

    if ((ip->mode & ITYPE) != IDIR) {
        return 0;
    }

    dp = getdir(ip);
    for (i = 0 ; i < entries ; i++) {
        if (strncmp(dp[i].name, name, 14) == 0) {
            inum = dp[i].inum;
            break;
        }
    }
    free(dp);
    return inum;
}

/*
 * given a path, return the inode
 */
struct dsknod *
namei(char *name)
{
    char *s;
    struct dsknod *ip;
    int inum;
    char chunk[20];

    ip = iget(1);
    while (*name == '/') {
        name++;
    }

    while (*name) {
        s = chunk;
        while (*name && *name != '/') {
            *s++ = *name++;
        }
        *s = 0;
        if (strlen(chunk) == 0)
            break;
        inum = lookup(ip, chunk);
        if (!inum) {
            return 0;
        }
        ifree(ip);
        ip = iget(inum);
        while (*name == '/') {
            name++;
        }
    }
    return ip;
}

/*
 * allocate a block
 */
int
balloc()
{
    int b, i;
    UINT buf[256];

    i = --fs->nfree;
    /* nfree is the current head.  we manage it */
    if (i < 0 || i >= 100) {
        printf("bad freeblock\n");
        return (0);
    }
    b = fs->free[i];

    if (b == 0) {
        printf("no space\n");
        return (0);
    }

    /*
     * we have an empty free list. 
     * read our block, copy the free list
     * and zero the block
     */
    if (fs->nfree <= 0) {
        readblk(b, (UCHAR *)buf);
        if (verbose > 2)
            secdump((UCHAR *)buf);
        fs->nfree = buf[0];
        for (i = 0; i < 100; i++) {
            fs->free[i] = buf[i + 1];
        }
        for (i = 0; i < 256; i++) {
            buf[i] = 0;
        }
        writeblk(b, (UCHAR *)buf);
    }
    superdirty = 1;
    return (b);
}

void
bfree(int blkno)
{
    int i;
    UINT buf[256];

    if (fs->nfree >= 100) {
        buf[0] = fs->nfree;
        for (i = 0; i < 100; i++)
            buf[i + 1] = fs->free[i];
        fs->nfree = 0; 
        writeblk(blkno, (UCHAR *)buf);
    }
    fs->free[fs->nfree++] = blkno;
    superdirty = 1;
}

/*
 * a cheap read cache 
 */
UINT iblk[256];
int iblkno = -1;

UINT iiblk[256];
int iiblkno = -1;

/*
 * this code does all the heavy lifting of knowing about the
 * way that block numbers are handled in the v6 filesystem
 * two modes for the 8 addr's in th inode:
 *  ILARGE, where the first 7 are indirect blocks,
 *      and the 8th is double indirect, 
 * and small, where all are block numbers for files <= 4096
 *
 * return 0 for a hole.
 */
int
bmap(struct dsknod *ip, int offset, int alloc)
{
    int lblk = offset / 512;    // logical block number
    UINT *aa;                   // address array
    int iindex;                 // indirect index
    int i;

    if (!(ip->mode & ILARGE)) {      // file is within 4k limit
        if (lblk <= 7) {
            if ((ip->addr[lblk] == 0) && alloc) {
                ip->addr[lblk] = balloc();
            }
            return ip->addr[lblk];
        } else if (!alloc) {
            return 0;
        }

        // convert to ILARGE
        iblkno = balloc();
        blkzero((char *)iblk);
        for (i = 0; i < 8; i++) {
            iblk[i] = ip->addr[i];
        }
        ip->mode |= ILARGE;
        writeblk(iblkno, (char *)iblk);
        iput(ip);     
        // fall into ILARGE case
    }

    iindex = lblk / 256;
    if (iindex == 7) {       // double indirect
        if (ip->addr[7] == 0) {
            if (!alloc) {
                return 0;
            }
            iiblkno = ip->addr[7] = balloc();
            blkzero((char *)iiblk);
            writeblk(iiblkno, (char *)iiblk);
            iput(ip);
        }
        if (iiblkno != ip->addr[7]) {
            iiblkno = ip->addr[7];
            readblk(iiblkno, (char *)iiblk);
        }
        aa = iiblk;
        iindex -= 7 * 256;
    } else {
        aa = ip->addr;
    }

    // a hole
    if (aa[iindex] == 0) {
        if (!alloc) {
            return 0;
        }
        aa[iindex] = balloc();
        if (aa == ip->addr) {
            iput(ip);
        } else {
            writeblk(iiblkno, (char *)iiblk);
        }
    }

    if (iblkno != aa[iindex]) {
        iblkno = aa[iindex];
        readblk(iblkno, (char *)iblk);
    }

    return iblk[lblk % 256];
}

/*
 * free all 256 blocks in an indirect block
 * and then free the indirect block.
 */
void
iblkfree(UINT *bp)
{
    int i;

    if (!*bp)
        return;

    readblk(*bp, (char *)iblk);
    for (i = 0; i < 256; i++) {
        if (iblk[i]) {
            bfree(iblk[i]);
            iblk[i] = 0;
        }
    }
    writeblk(*bp, (char *)iblk);
    bfree(*bp);
    *bp = 0;
}

/*
 * free all the blocks in a file
 */
void
filefree(struct dsknod *ip)
{
    int i, j;

    if (!(ip->mode & ILARGE)) {
        for (i = 0; i < 8; i++) {
            if (ip->addr[i]) {
                bfree(ip->addr[i]);
                ip->addr[i] = 0;
            }
        }
        goto done;
    }
    iblkno = -1;
    iiblkno = -1;

    for (i = 0; i < 7; i++) {
        iblkfree(&ip->addr[i]);
    }
    if (ip->addr[7]) {
        readblk(ip->addr[7], (char *)iiblk);
        for (i = 0; i < 256; i++) {
            iblkfree(&iiblk[i]);
        }
        writeblk(ip->addr[7], (char *)iiblk);
    }
done:
    ip->size0 = 0;
    ip->size1 = 0;
    iput(ip);
}

/*
 * remove a file name.  if this is the last link, remove the inode
 */
void
fileunlink(char *name)
{
    char *dirname = strdup(name);
    struct dsknod *ip;
    struct dir *dp;
    int inum = 0;
    int entries;
    int size;
    char *s;
    int i;
 
    s = rindex(dirname, '/');
    if (!s) {
        ip = iget(1);
    } else {
        *s = 0;
        ip = namei(dirname);
    }
    free(dirname);

    if ((ip->mode & ITYPE) != IDIR) {
        return;
    }

    /*
     * remove the directory entry
     */
    entries = filesize(ip) / 16;

    dp = getdir(ip);
    for (i = 0 ; i < entries; i++) {
        if (strncmp(dp[i].name, name, 14) == 0) {
            inum = dp[i].inum;
            break;
        }
    }
    if (inum != 0) {
        memmove(&dp[i], &dp[i+1], (entries - i) * sizeof(struct dir));
        memset(&dp[entries-1], 0, sizeof(struct dir));
        ip->size1 -= 16;
        size = filesize(ip);
        if ((size % 512) == 0) {
            if (ip->mode & ILARGE) {
                printf("can't handle LARGE directories\n");
            }
            bfree(ip->addr[size / 512]);
            ip->addr[size/ 512] = 0;
        }
        iput(ip);
        putdir(ip, dp);
        ifree(ip);
    }
    free(dp);
    
    if (inum == 0) {
        return;
    }

    /*
     * now maybe remove the file
     */
    ip = iget(inum);
    ip->nlinks--;
    if (ip->nlinks == 0) {
        filefree(ip);

        ip->mode = 0;
        if (fs->ninode != 100) {
            fs->inode[fs->ninode++] = inum;
        }
    }
    iput(ip);
    ifree(ip);
    superdirty = 1;
}

/*
 * create an ordinary file using a path
 */
struct dsknod *
filecreate(char *name)
{
    return 0;
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
    writeblk(bmap(ip, offset, 1), buf);
    return 512;
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
