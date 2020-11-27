/*
 * common code for micronix filesystem maintenance utilities
 * brute force for everything
 */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

#include "../micronix/include/types.h"
#include "../micronix/include/sys/fs.h"
#include "../micronix/include/sys/dir.h"

#include "../include/fslib.h"
#include "../include/util.h"
#include "../include/imd.h"

#ifdef USE_LIBDSK
#include <libdsk.h>
#endif


int spt = 15;

int traceflags;
int trace_fs;

/*
 * wrapper struct.  we pass around struct super but cast it to
 * struct image to get at private data
 */
struct image {
    union {
        struct super fs;
        char superblock[512];
    } sb;
    int superdirty;
    int driver;
    int fd;
    void *drive;
    char dt;
    int major;
    int minor;
    int altsec;
#ifdef USE_LIBDSK
    DSK_PDRIVER *drive;
#endif
};

#define DRIVER_IMAGE    1
#define DRIVER_IMD      2
#define DRIVER_LIBDSK   3

void
closefs(struct super *arg)
{
    struct image *i = (struct image *)arg;

    if (i->superdirty) {
        writeblk(arg, 1, i->sb.superblock);
    }
    i->superdirty = 0;
    switch (i->driver) {
    case DRIVER_LIBDSK:
#ifdef USE_LIBDSK
        dsk_close(i->drive);
#endif
        break;
    case DRIVER_IMD:
        imd_close(i->drive);
        break;
    case DRIVER_IMAGE:
    default:
        close(i->fd);
        break;
    }
}

int
is_sticky(char *fn)
{
    struct stat sbuf;

    if (stat(fn, &sbuf) == 0) {
        if (sbuf.st_mode & S_ISVTX) {
            return 1;
        }    
    }
    return 0;
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
int 
openfsrw(char *filesystem, struct super **fsp, int writable)
{
    int ret;
    struct image *i = malloc(sizeof(struct image));
    char *ext;
    
    i->driver = DRIVER_IMAGE;
    ext = strrchr(filesystem, '.');
    if (ext) {
        if (strcasecmp(ext, ".imd") == 0) {
            i->driver = DRIVER_IMD;
        } else if (strcasecmp(ext, ".image") == 0) {
            i->driver = DRIVER_IMAGE;
        }
    }

    switch (i->driver) {
    case DRIVER_LIBDSK:
#ifdef USE_LIBDSK
        ret = dsk_open(drive, filesystem, 0, 0);
#endif
        break;
    case DRIVER_IMD:
        i->drive = imd_load(filesystem, 0, writable);
        if (i->drive) {
            ret = 0;
        } else {
            ret = -1;
        }
        break;
    case DRIVER_IMAGE:
    default:
        ret = open(filesystem, writable ? 2 : 0);
        i->fd = ret;
        break;
    } 
 
    if (ret >= 0) {
        i->dt = ' ';
        i->altsec = 0;
        devnum(filesystem, &i->dt, &i->major, &i->minor);
        if ((i->driver == DRIVER_IMD) || 
            ((i->dt == 'b') && (i->major == 2) && (i->minor & 0x8))) {
            i->altsec = 1;
        }
        if (i->dt == ' ') {
            if (is_sticky(filesystem)) {
                i->altsec = 1;
            }
        }
        *fsp = (struct super *)i;
        readblk(*fsp, 1, i->sb.superblock);
    } else {
        free((struct super *)i);
    }
    return ret;
}

int
openfs(char *filesystem, struct super **fsp)
{
    return openfsrw(filesystem, fsp, 0);
}

/*
 * do sector skew
 */
UINT
secmap(struct super *fs, UINT blkno)
{
    int trk = blkno / spt;
    int sec = blkno % spt;
    struct image *i = (struct image *)fs;

    if (i->altsec) {
        sec <<= 1;
        if (!(spt & 1) && sec >= spt) {
            sec++;
        }
        sec %= spt;
    }
    return (trk * spt + sec);
}

int secsize;
int secs;
void
blktopos(void *drive, int blkno, int *cyl, int *head, int *sec)
{
    int trk;

    if (secsize == 0) {
        imd_trkinfo(drive, 2, 0, &secs, &secsize);
        trace(trace_fs, "secs %d secsize %d\n", secs, secsize);
    }
    if (secsize != 512) {
        printf("secsize: %d\n", secsize);
        lose("not a filesystem");
    }

    *sec = (blkno % secs) + 1;
    trk = blkno / secs;
    *head = 0;
    *cyl = 2 + trk;
    trace(trace_fs, "blkno:%d cyl:%d head:%d sec:%d\n", 
        blkno, *cyl, *head, *sec);
}

/*
 * get the contents of a block
 */
int
readblk(struct super *fs, int blkno, char *buf)
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

    realblk = secmap(fs, blkno);
    trace(trace_fs, "readblk: %d -> %d\n", blkno, realblk);

    if (i->driver == DRIVER_IMAGE) {
        if (lseek(i->fd, 512 * realblk, SEEK_SET) < 0)
            lose("readblk seek");
        if (read(i->fd, buf, 512) != 512)
            lose("readblk");
        if (traceflags & trace_fs) {
            hexdump(buf, 512);
        }
    } else if (i->driver == DRIVER_IMD) {
        blktopos(i->drive, realblk, &cyl, &head, &sec); 
        ret = imd_read(i->drive, cyl, head, sec, buf);
        if (ret != 512) {
            printf("read lose %d\n", ret);
            lose("imd_read");
        }
    }
    return 0;
}

/*
 * scribble the contents of a block
 */
int
writeblk(struct super *fs, int blkno, char *buf)
{
    int realblk;
    int ret;
    int cyl, head, sec;
    struct image *i = (struct image *)fs;

    if (blkno == 0) {
        lose("writeblk 0");
        return 0;
    }

    realblk = secmap(fs, blkno);
    trace(trace_fs, "writeblk: %d -> %d\n", blkno, realblk);

    if (i->driver == DRIVER_IMAGE) {
        if (lseek(i->fd, 512 * realblk, SEEK_SET) < 0)
            lose("writeblk seek");
        if ((ret = write(i->fd, buf, 512)) != 512) {
            printf("write ret %d\n", ret);
            lose("writeblk");
        }
    } else if (i->driver == DRIVER_IMD) {
        blktopos(i->drive, realblk, &cyl, &head, &sec); 
        ret = imd_write(i->drive, cyl, head, sec, buf);
        if (ret != 512) {
            printf("write lose %d\n", ret);
            lose("imd_write");
        }
    }
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

struct super *fs;
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
        printf("%5d: %14s\n", dp->ino, dp->name);
    }
}

/*
 * we wrap the on-disk inode to put an inumber
 */
struct i_node {
    struct dsknod ondisk;
    struct super *fs;
    int inum;
};

struct dsknod inodeblk[I_PER_BLK];
int inblkno = 0;

/*
 * read an inode, given the inum
 */
struct dsknod *
iget(struct super *fs, int inum)
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

UINT32
timeswap(UINT32 x)
{
    UINT32 ret;
    union
    {
        UINT32 t;
        UINT tw[2];
        UINT8 tb[4];
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
mytime(UINT32 t)
{
    static char timebuf[100];
    union
    {
        UINT32 t;
        UINT8 tb[4];
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
dumpsb(struct super *fs)
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
    printperm(dp->mode >> 6, dp->mode & D_ISUID);
    printperm(dp->mode >> 3, dp->mode & D_ISGID);
    printperm(dp->mode, 0);
    printf("%3d ", dp->nlink);
    printf("%3d %3d ", dp->uid, dp->gid);
    if (dp->mode & D_IIO) {
        printf("%3d,%3d ", (dp->addr[0] >> 8) & 0xff, dp->addr[0] & 0xff);
    } else {
        printf("%7d ", filesize(dp));
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

    if (!(dp->mode & D_ALLOC))
        return;

    isummary("", dp);

    ft = (dp->mode & D_IFMT) >> 13;
    printf("inode %5d: %6o %d %s\n", ip->inum, dp->mode, ft, itype[ft & 3]);

    size = filesize(dp);

    printf("\tmode: %o links: %d uid: %d gid: %d size: %d %s\n",
        dp->mode, dp->nlink, dp->uid, dp->gid,
        size, dp->mode & D_LARGE ? "LARGE" : "");

    printf("\trtime: %s ", mytime(dp->actime));
    printf("wtime: %s\n", mytime(dp->modtime));

    if (dp->mode & D_IIO) {
        int maj = (dp->addr[0] >> 8) & 0xff;
        int min = dp->addr[0] & 0xff;

        printf("\tmaj: %d min: %d\n", maj, min);
    } else {
        printf("\tblocklist:\n\t\t");
        for (i = 0; i < 8; i++) {
            printf("%d ", dp->addr[i]);
        }
        printf("\n");
        if (dp->mode & D_LARGE) {
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
    if ((dp->mode & D_IFMT) == D_IFDIR) {
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
        readblk(ip->fs, bmap(dp, i, 0), (UINT8 *)&dirp[i / 16]);
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
        writeblk(ip->fs, bmap(dp, i, 1), (UINT8 *)&dirp[i / 16]);
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

    if ((dp->mode & D_IFMT) != D_IFDIR) {
        return 0;
    }

    dirp = getdir(dp);
    for (i = 0 ; i < entries ; i++) {
        if (strncmp(dirp[i].name, name, 14) == 0) {
            inum = dirp[i].ino;
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
namei(struct super *fs, char *name)
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
balloc(struct super *fs)
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
        readblk(fs, b, (UINT8 *)buf);
        fs->nfree = buf[0];
        for (i = 0; i < 100; i++) {
            fs->free[i] = buf[i + 1];
        }
        for (i = 0; i < 256; i++) {
            buf[i] = 0;
        }
        writeblk(fs, b, (UINT8 *)buf);
    }
    ((struct image *)fs)->superdirty = 1;
    return (b);
}

void
bfree(struct super *fs, int blkno)
{
    int i;
    UINT buf[256];

    if (fs->nfree >= 100) {
        buf[0] = fs->nfree;
        for (i = 0; i < 100; i++)
            buf[i + 1] = fs->free[i];
        fs->nfree = 0; 
        writeblk(fs, blkno, (UINT8 *)buf);
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
 *  D_LARGE, where the first 7 are indirect blocks,
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

    if (!(dp->mode & D_LARGE)) {      // file is within 4k limit
        if (lblk <= 7) {
            if ((dp->addr[lblk] == 0) && alloc) {
                dp->addr[lblk] = balloc(ip->fs);
            }
            return dp->addr[lblk];
        } else if (!alloc) {
            return 0;
        }

        // convert to D_LARGE
        iblkno = balloc(ip->fs);
        bzero((char *)iblk, 512);
        for (i = 0; i < 8; i++) {
            iblk[i] = dp->addr[i];
            dp->addr[i] = 0;
        }
        dp->addr[0] = iblkno;
        dp->mode |= D_LARGE;
        writeblk(ip->fs, iblkno, (char *)iblk);
        iput(dp);     
        // fall into D_LARGE case
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
iblkfree(struct super *fs, UINT *bp)
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

    if (dp->mode & D_IIO) {
        return;
    }
    if (!(dp->mode & D_LARGE)) {
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
    dp->mode &= ~D_LARGE;
    dp->size0 = 0;
    dp->size1 = 0;
    iput(dp);
}

/*
 * remove a file name.  if this is the last link, remove the inode
 */
void
fileunlink(struct super *fs, char *name)
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

    if ((dp->mode & D_IFMT) != D_IFDIR) {
        return;
    }

    /*
     * remove the directory entry
     */
    entries = filesize(dp) / 16;

    dirp = getdir(dp);
    for (i = 0 ; i < entries; i++) {
        if (strncmp(dirp[i].name, name, 14) == 0) {
            inum = dirp[i].ino;
            break;
        }
    }
    if (inum != 0) {
        memmove(&dp[i], &dp[i+1], (entries - i) * sizeof(struct dir));
        memset(&dp[entries-1], 0, sizeof(struct dir));
        dp->size1 -= 16;
        size = filesize(dp);
        if ((size % 512) == 0) {
            if (dp->mode & D_LARGE) {
                printf("can't handle D_LARGE directories\n");
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
    ip->nlink--;
    if (ip->nlink == 0) {
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
filecreate(struct super *fs, char *name)
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
