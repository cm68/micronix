/*
 * common code for micronix filesystem maintenance utilities
 * brute force for everything
 */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "fslib.h"
#include "util.h"

#ifdef USE_LIBDSK
#include <libdsk.h>
#endif

#ifdef USE_LIBIMD
#include "imd.h"
#endif

#if !defined(USE_LIBDSK) && !defined(USE_LIBIMD)
#define USE_IMAGE
#endif

int spt = 15;
int altsec = 1;

int traceflags;
int trace_fs;

/*
 * wrapper struct.  we pass around struct sup but cast it to
 * struct image to get at private data
 */
struct image {
    union {
        struct sup fs;
        char superblock[512];
    } sb;
    int superdirty;
#ifdef USE_IMAGE
    int fd;
#endif
#ifdef USE_LIBIMD
    void *drive;
#endif
#ifdef USE_LIBDSK
    DSK_PDRIVER *drive;
#endif
};

void
closefs(struct sup *arg)
{
    struct image *i = (struct image *)arg;

    if (i->superdirty) {
        writeblk(arg, 1, i->sb.superblock);
    }
    i->superdirty = 0;
#ifdef USE_IMAGE
    close(i->fd);
#endif
#ifdef USE_LIBIMD
    imd_close(i->drive);
#endif
#ifdef USE_LIBDSK
    dsk_close(i->drive);
#endif
}

void
closefs_hook(int i, void *arg)
{
    closefs(arg);
}

int
openfs(char *filesystem, struct sup **fsp)
{
    int ret;
    struct image *i = malloc(sizeof(struct image));

#ifdef USE_LIBIMD
    ret = imd_load(filesystem, 0, 1);
    i->drive = ret;
#endif
#ifdef USE_IMAGE
    ret = open(filesystem, O_RDWR);
    i->fd = ret;
#endif
#ifdef USE_LIBDSK
    ret = dsk_open(drive, filesystem, 0, 0);
#endif
    if (ret > 0) {
        *fsp = (struct sup *)i;
        on_exit(closefs_hook, (void *)i);
    } else {
        free((struct sup *)i);
    }
    readblk(*fsp, 1, i->sb.superblock);
    return ret;
}

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

#ifdef USE_LIBIMD
int secsize;
int secs;
void
blktopos(int drive, int blkno, int *cyl, int *head, int *sec)
{
    int trk;
    int realblk;

    if (secsize == 0) {
        imd_trkinfo(drive, 2, 0, &secs, &secsize);
        trace(trace_bio, "secs %d secsize %d\n", secs, secsize);
    }
    if (secsize != 512) {
        printf("secsize: %d\n", secsize);
        lose("not a filesystem");
    }

    realblk = secmap(blkno);

    *sec = (realblk % secs) + 1;
    trk = realblk / secs;
    *head = 0;
    *cyl = 2 + trk;
    trace(trace_bio, "blkno:%d realblk:%d cyl:%d head:%d sec:%d\n", 
        blkno, realblk, *cyl, *head, *sec);
}
#endif

/*
 * get the contents of a block
 */
int
readblk(struct sup *fs, int blkno, char *buf)
{
    int realblk;
    int ret;
    int cyl, head, sec;
    struct image *i = (struct image *)fs;

    /* block zero always reads 0 */
    if (blkno == 0) {
        bzero(buf, 512);
        return 0;
    }

    realblk = secmap(blkno);
    trace(trace_fs, "readblk: %d -> %d\n", blkno, realblk);

#ifdef USE_IMAGE
    if (lseek(i->fd, 512 * realblk, SEEK_SET) < 0)
        lose("readblk seek");
    if (read(i->fd, buf, 512) != 512)
        lose("readblk");
#endif
#ifdef USE_LIBIMD
    blktopos(i->drive, realblk, &cyl, &head, &sec); 
    ret = imd_read(i->drive, cyl, head, sec, buf);
    if (ret != 512) {
        printf("read lose %d\n", ret);
        lose("imd_read");
    }
#endif
    return 0;
}

/*
 * scribble the contents of a block
 */
int
writeblk(struct sup *fs, int blkno, char *buf)
{
    int realblk;
    int ret;
    int cyl, head, sec;
    struct image *i = (struct image *)fs;

    if (blkno == 0) {
        lose("writeblk 0");
        return 0;
    }

    realblk = secmap(blkno);
    trace(trace_fs, "writeblk: %d -> %d\n", blkno, realblk);

#ifdef USE_IMAGE
    if (lseek(i->fd, 512 * realblk, SEEK_SET) < 0)
        lose("writeblk seek");
    if ((ret = write(i->fd, buf, 512)) != 512) {
        printf("write ret %d\n", ret);
        lose("writeblk");
    }
#endif

#ifdef USE_LIBIMD
    blktopos(i->drive, realblk, &cyl, &head, &sec); 
    ret = imd_write(i->drive, cyl, head, sec, buf);
    if (ret != 512) {
        printf("write lose %d\n", ret);
        lose("imd_write");
    }
#endif

    return 0;
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

#ifdef notdef
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
#endif

int
filesize(struct dsknod *ip)
{
    return (ip->size0 << 16) + ip->size1;
}

void
secdump(unsigned char *buf)
{
    hexdump(buf, 512);
}

/*
 * print out a directory block
 */
void
dirdump(char *buf, int size)
{
    struct dir *dp;

    printf("dirdump: %d\n", size);
    if (size > 512)
        size = 512;
    for (dp = (struct dir *) buf; dp < (struct dir *) &buf[size]; dp++) {
        printf("%5d: %14s\n", dp->inum, dp->name);
    }
}

/*
 * we wrap the on-disk inode to put an inumber
 */
struct i_node {
    struct dsknod ondisk;
    struct sup *fs;
    int inum;
};

struct dsknod inodeblk[I_PER_BLK];
int inblkno = 0;

/*
 * read an inode, given the inum
 */
struct dsknod *
iget(struct sup *fs, int inum)
{
    int iblk;
    int offset;
    struct i_node *ip = malloc(sizeof(struct i_node));

    ip->inum = inum;
    ip->fs = fs;
    inum--;

    iblk = INODES_START + (inum / I_PER_BLK);
    if (inblkno != iblk) {
        inblkno = iblk;
        readblk(fs, inblkno, (char *)inodeblk);
    } 
    bcopy(&inodeblk[inum % I_PER_BLK], &ip->ondisk, sizeof(struct dsknod));

    // isummary("", &ip->ondisk);
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

    iblk = INODES_START + (inum / I_PER_BLK);
    if (inblkno != iblk) {
        inblkno = iblk;
        readblk(ip->fs, inblkno, (char *)inodeblk);
    } 
    bcopy(&ip->ondisk, &inodeblk[inum % I_PER_BLK], sizeof(struct dsknod));
    writeblk(ip->fs, inblkno, (char *)inodeblk);
}

void
ifree(struct dsknod *dp)
{
    struct i_node *ip = (struct i_node *)dp;
    free(ip);
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
isummary(char *name, struct dsknod *dp)
{
    int i;

    printf("%5d ", ((struct i_node *)dp)->inum);
    printf("%c", "-cdb"[(dp->mode >> 13) & 3]);
    printperm(dp->mode >> 6, dp->mode & ISETUID);
    printperm(dp->mode >> 3, dp->mode & ISETGID);
    printperm(dp->mode, 0);
    printf("%3d ", dp->nlinks);
    printf("%3d %3d ", dp->uid, dp->gid);
    if (dp->mode & IIO) {
        printf("%3d,%3d ", (dp->addr[0] >> 8) & 0xff, dp->addr[0] & 0xff);
    } else {
        printf("%7d ", filesize(dp));
        for (i = 0; i < 8; i++) {
            printf("%d ", dp->addr[i]);
        }
    }
    printf("%14s\n", name);
}

/*
 * print out an inode, raw
 */
void
idump(struct dsknod *dp)
{
    int i;
    int j;
    unsigned int ft;
    unsigned int size;
    UINT indir[256];
    UINT dindir[256];
    struct i_node *ip = (struct i_node *)dp;

    if (!(dp->mode & IALLOC))
        return;

    isummary("", dp);

    ft = (dp->mode & ITYPE) >> 13;
    printf("inode %5d: %6o %d %s\n", ip->inum, dp->mode, ft, itype[ft & 3]);

    size = filesize(dp);

    printf("\tmode: %o links: %d uid: %d gid: %d size: %d %s\n",
        dp->mode, dp->nlinks, dp->uid, dp->gid,
        size, dp->mode & ILARGE ? "LARGE" : "");

    printf("\trtime: %s ", mytime(dp->rtime));
    printf("wtime: %s\n", mytime(dp->wtime));

    if (dp->mode & IIO) {
        int maj = (dp->addr[0] >> 8) & 0xff;
        int min = dp->addr[0] & 0xff;

        printf("\tmaj: %d min: %d\n", maj, min);
    } else {
        printf("\tblocklist:\n\t\t");
        for (i = 0; i < 8; i++) {
            printf("%d ", dp->addr[i]);
        }
        printf("\n");
        if (dp->mode & ILARGE) {
            for (i = 0; i < 7; i++) {
                if (dp->addr[i]) {
                    printf("indirect %d\n", dp->addr[i]);
                    readblk(ip->fs, dp->addr[i], (char *) indir);
                    blocklist(indir);
                }
            }
            if (dp->addr[7]) {
                printf("double indirect\n");
                readblk(ip->fs, dp->addr[7], (char *) dindir);
                blocklist(dindir);
                for (i = 0; i < 256; i++) {
                    printf("indirect %d\n", i);
                    if (dindir[i]) {
                        readblk(ip->fs, dindir[i], (char *) indir);
                        blocklist(indir);
                    }
                }
            }
        }
    }
    if ((dp->mode & ITYPE) == IDIR) {
        for (i = 0; i < 8; i++) {
            if (i * 512 > size)
                break;
            readblk(ip->fs, dp->addr[i], dbuf);
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
getdir(struct dsknod *dp)
{
    struct dir *dirp;
    int i;
    int size = filesize(dp);
    struct i_node *ip = (struct i_node *)dp;

    // round up to the containing block
    if (size % 512) size = (size | 511) + 1;
    dirp = malloc(size);

    for (i = 0; i < size; i += 512) {
        readblk(ip->fs, bmap(dp, i, 0), (UCHAR *)&dirp[i / 16]);
    }
    return dirp;
}

/*
 * update an entire directory
 */
void
putdir(struct dsknod *dp, struct dir *dirp)
{
    int i;
    int size = filesize(dp);
    struct i_node *ip = (struct i_node *)dp;

    for (i = 0; i < size; i += 512) {
        writeblk(ip->fs, bmap(dp, i, 1), (UCHAR *)&dirp[i / 16]);
    }
}

/*
 * return a pointer to the i'th directory entry
 */
struct dir *
getdirent(struct dsknod *dp, int i)
{
    int b;
    struct i_node *ip = (struct i_node *)dp;

    if (i * 16 > filesize(dp)) {
        return 0;
    }
    b = bmap(dp, i * 16, 0);
    if (dblk != b) {
        dblk = b;
        readblk(ip->fs, dblk, (char *) dirbuf);
    }
    return &dirbuf[i % DENTS];
}

/*
 * look for an entry in a directory
 */
int
lookup(struct dsknod *dp, char *name)
{
    int inum = 0;
    struct dir *dirp;
    int entries = filesize(dp) / 16;
    int i;

    if ((dp->mode & ITYPE) != IDIR) {
        return 0;
    }

    dirp = getdir(dp);
    for (i = 0 ; i < entries ; i++) {
        if (strncmp(dirp[i].name, name, 14) == 0) {
            inum = dirp[i].inum;
            break;
        }
    }
    free(dirp);
    return inum;
}

/*
 * given a path, return the inode
 */
struct dsknod *
namei(struct sup *fs, char *name)
{
    char *s;
    struct dsknod *dp;
    int inum;
    char chunk[20];

    dp = iget(fs, 1);
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
        inum = lookup(dp, chunk);
        if (!inum) {
            return 0;
        }
        ifree(dp);
        dp = iget(fs, inum);
        while (*name == '/') {
            name++;
        }
    }
    return dp;
}

/*
 * allocate a block
 */
int
balloc(struct sup *fs)
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
        readblk(fs, b, (UCHAR *)buf);
        fs->nfree = buf[0];
        for (i = 0; i < 100; i++) {
            fs->free[i] = buf[i + 1];
        }
        for (i = 0; i < 256; i++) {
            buf[i] = 0;
        }
        writeblk(fs, b, (UCHAR *)buf);
    }
    ((struct image *)fs)->superdirty = 1;
    return (b);
}

void
bfree(struct sup *fs, int blkno)
{
    int i;
    UINT buf[256];

    if (fs->nfree >= 100) {
        buf[0] = fs->nfree;
        for (i = 0; i < 100; i++)
            buf[i + 1] = fs->free[i];
        fs->nfree = 0; 
        writeblk(fs, blkno, (UCHAR *)buf);
    }
    fs->free[fs->nfree++] = blkno;
    ((struct image *)fs)->superdirty = 1;
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
bmap(struct dsknod *dp, int offset, int alloc)
{
    struct i_node *ip = (struct i_node *)dp;
    int lblk = offset / 512;    // logical block number
    UINT *aa;                   // address array
    int iindex;                 // indirect index
    int i;

    if (!(dp->mode & ILARGE)) {      // file is within 4k limit
        if (lblk <= 7) {
            if ((dp->addr[lblk] == 0) && alloc) {
                dp->addr[lblk] = balloc(ip->fs);
            }
            return dp->addr[lblk];
        } else if (!alloc) {
            return 0;
        }

        // convert to ILARGE
        iblkno = balloc(ip->fs);
        bzero((char *)iblk, 512);
        for (i = 0; i < 8; i++) {
            iblk[i] = dp->addr[i];
            dp->addr[i] = 0;
        }
        dp->addr[0] = iblkno;
        dp->mode |= ILARGE;
        writeblk(ip->fs, iblkno, (char *)iblk);
        iput(dp);     
        // fall into ILARGE case
    }

    iindex = lblk / 256;
    if (iindex == 7) {       // double indirect
        if (dp->addr[7] == 0) {
            if (!alloc) {
                return 0;
            }
            iiblkno = dp->addr[7] = balloc(ip->fs);
            bzero((char *)iiblk, 512);
            writeblk(ip->fs, iiblkno, (char *)iiblk);
            iput(dp);
        }
        if (iiblkno != dp->addr[7]) {
            iiblkno = dp->addr[7];
            readblk(ip->fs, iiblkno, (char *)iiblk);
        }
        aa = iiblk;
        iindex -= 7 * 256;
    } else {
        aa = dp->addr;
    }

    // an indirect hole
    if (aa[iindex] == 0) {
        if (!alloc) {
            return 0;
        }
        aa[iindex] = balloc(ip->fs);
        if (aa == dp->addr) {
            iput(dp);
        } else {
            writeblk(ip->fs, iiblkno, (char *)iiblk);
        }
    }

    if (iblkno != aa[iindex]) {
        iblkno = aa[iindex];
        readblk(ip->fs, iblkno, (char *)iblk);
    }

    if ((iblk[lblk % 256] == 0) && alloc) {
        iblk[lblk % 256] = balloc(ip->fs);
        writeblk(ip->fs, iblkno, (char *)iblk);
    }
    return iblk[lblk % 256];
}

/*
 * free all 256 blocks in an indirect block
 * and then free the indirect block.
 */
void
iblkfree(struct sup *fs, UINT *bp)
{
    int i;

    if (!*bp)
        return;

    readblk(fs, *bp, (char *)iblk);
    for (i = 0; i < 256; i++) {
        if (iblk[i]) {
            bfree(fs, iblk[i]);
            iblk[i] = 0;
        }
    }
    writeblk(fs, *bp, (char *)iblk);
    bfree(fs, *bp);
    *bp = 0;
}

/*
 * free all the blocks in a file
 */
void
filefree(struct dsknod *dp)
{
    struct i_node *ip = (struct i_node *)dp;
    int i, j;

    if (dp->mode & IIO) {
        return;
    }
    if (!(dp->mode & ILARGE)) {
        for (i = 0; i < 8; i++) {
            if (dp->addr[i]) {
                bfree(ip->fs, dp->addr[i]);
                dp->addr[i] = 0;
            }
        }
        goto done;
    }
    iblkno = -1;
    iiblkno = -1;

    for (i = 0; i < 7; i++) {
        iblkfree(ip->fs, &dp->addr[i]);
    }
    if (dp->addr[7]) {
        readblk(ip->fs, dp->addr[7], (char *)iiblk);
        for (i = 0; i < 256; i++) {
            iblkfree(ip->fs, &iiblk[i]);
        }
        writeblk(ip->fs, dp->addr[7], (char *)iiblk);
    }
done:
    dp->mode &= ~ILARGE;
    dp->size0 = 0;
    dp->size1 = 0;
    iput(dp);
}

/*
 * remove a file name.  if this is the last link, remove the inode
 */
void
fileunlink(struct sup *fs, char *name)
{
    char *dirname = strdup(name);
    struct dsknod *dp;
    struct dir *dirp;
    int inum = 0;
    int entries;
    int size;
    char *s;
    int i;
 
    s = rindex(dirname, '/');
    if (!s) {
        dp = iget(fs, 1);
    } else {
        *s = 0;
        dp = namei(fs, dirname);
    }
    free(dirname);

    if ((dp->mode & ITYPE) != IDIR) {
        return;
    }

    /*
     * remove the directory entry
     */
    entries = filesize(dp) / 16;

    dirp = getdir(dp);
    for (i = 0 ; i < entries; i++) {
        if (strncmp(dirp[i].name, name, 14) == 0) {
            inum = dirp[i].inum;
            break;
        }
    }
    if (inum != 0) {
        memmove(&dp[i], &dp[i+1], (entries - i) * sizeof(struct dir));
        memset(&dp[entries-1], 0, sizeof(struct dir));
        dp->size1 -= 16;
        size = filesize(dp);
        if ((size % 512) == 0) {
            if (dp->mode & ILARGE) {
                printf("can't handle LARGE directories\n");
            }
            bfree(fs, dp->addr[size / 512]);
            dp->addr[size/ 512] = 0;
        }
        iput(dp);
        putdir(dp, dirp);
        ifree(ip);
    }
    free(dp);
    
    if (inum == 0) {
        return;
    }

    /*
     * now maybe remove the file
     */
    ip = iget(fs, inum);
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
    ((struct image *)fs)->superdirty = 1;
}

/*
 * create an ordinary file using a path
 */
struct dsknod *
filecreate(struct sup *fs, char *name)
{
    return 0;
}

/*
 * read a block at the logical file offset
 * offset is block aligned
 */
int
fileread(struct dsknod *dp, int offset, char *buf)
{
    struct i_node *ip = (struct i_node *)dp;
    int valid = filesize(dp) - offset;

    readblk(ip->fs, bmap(dp, offset, 0), buf);
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
filewrite(struct dsknod *dp, int offset, char *buf)
{
    struct i_node *ip = (struct i_node *)dp;
    writeblk(ip->fs, bmap(dp, offset, 1), buf);
    return 512;
}

__attribute__((constructor))
void
libfs_init()
{
    trace_fs = register_trace("fslib");
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
