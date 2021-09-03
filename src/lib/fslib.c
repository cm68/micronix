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
#include <time.h>

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

extern int traceflags;
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
closefs(struct super *fs)
{
    struct image *i = (struct image *)fs;

    if (fs->s_fmod) {
        writeblk(fs, 1, i->sb.superblock);
    }
    fs->s_fmod = 0;
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

#ifdef notdef
struct super *fs;
struct dsknod *ip;
struct dir *dp;
#endif
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
    return (ip->d_size0 << 16) + ip->d_size1;
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

int 
inumof(struct dsknod *dp)
{
    struct i_node *ip = (struct i_node *)dp;
    return ip->inum;
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
        fs->s_isize, fs->s_fsize, fs->s_nfree, fs->s_ninode,
        mytime(timeswap(fs->s_time)));
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
    printf("%c", "-cdb"[(dp->d_mode >> 13) & 3]);
    printperm(dp->d_mode >> 6, dp->d_mode & ISUID);
    printperm(dp->d_mode >> 3, dp->d_mode & ISGID);
    printperm(dp->d_mode, 0);
    printf("%3d ", dp->d_nlink);
    printf("%3d %3d ", dp->d_uid, dp->d_gid);
    if (dp->d_mode & IIO) {
        printf("%3d,%3d ", 
            (dp->d_addr[0] >> 8) & 0xff, dp->d_addr[0] & 0xff);
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

    if (!(dp->d_mode & IALLOC))
        return;

    isummary("", dp);

    ft = (dp->d_mode & IFMT) >> 13;
    printf("inode %5d: %6o %d %s\n", ip->inum, dp->d_mode, ft, itype[ft & 3]);

    size = filesize(dp);

    printf("\tmode: %o links: %d uid: %d gid: %d size: %d %s\n",
        dp->d_mode, dp->d_nlink, dp->d_uid, dp->d_gid,
        size, dp->d_mode & ILARG ? "LARGE" : "");

    printf("\trtime: %s ", mytime(dp->d_atime));
    printf("wtime: %s\n", mytime(dp->d_mtime));

    if (dp->d_mode & IIO) {
        int maj = (dp->d_addr[0] >> 8) & 0xff;
        int min = dp->d_addr[0] & 0xff;

        printf("\tmaj: %d min: %d\n", maj, min);
    } else {
        printf("\tblocklist:\n\t\t");
        for (i = 0; i < 8; i++) {
            printf("%d ", dp->d_addr[i]);
        }
        printf("\n");
        if (dp->d_mode & ILARG) {
            for (i = 0; i < 7; i++) {
                if (dp->d_addr[i]) {
                    printf("indirect %d\n", dp->d_addr[i]);
                    readblk(ip->fs, dp->d_addr[i], (char *) indir);
                    blocklist(indir);
                }
            }
            if (dp->d_addr[7]) {
                printf("double indirect\n");
                readblk(ip->fs, dp->d_addr[7], (char *) dindir);
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
    if ((dp->d_mode & IFMT) == IFDIR) {
        for (i = 0; i < 8; i++) {
            if (i * 512 > size)
                break;
            readblk(ip->fs, dp->d_addr[i], dbuf);
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

    if ((dp->d_mode & IFMT) != IFDIR) {
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

    i = --fs->s_nfree;
    /* nfree is the current head.  we manage it */
    if (i < 0 || i >= 100) {
        printf("bad freeblock\n");
        return (0);
    }
    b = fs->s_free[i];

    if (b == 0) {
        printf("no space\n");
        return (0);
    }

    /*
     * we have an empty free list. 
     * read our block, copy the free list
     * and zero the block
     */
    if (fs->s_nfree <= 0) {
        readblk(fs, b, (UINT8 *)buf);
        fs->s_nfree = buf[0];
        for (i = 0; i < 100; i++) {
            fs->s_free[i] = buf[i + 1];
        }
        for (i = 0; i < 256; i++) {
            buf[i] = 0;
        }
        writeblk(fs, b, (UINT8 *)buf);
    }
    fs->s_fmod = 1;
    return (b);
}

void
bfree(struct super *fs, int blkno)
{
    int i;
    UINT buf[256];

    if (fs->s_nfree >= 100) {
        buf[0] = fs->s_nfree;
        for (i = 0; i < 100; i++)
            buf[i + 1] = fs->s_free[i];
        fs->s_nfree = 0; 
        writeblk(fs, blkno, (UINT8 *)buf);
    }
    fs->s_free[fs->s_nfree++] = blkno;
    fs->s_fmod = 1;
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
 *  ILARG, where the first 7 are indirect blocks,
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

    if (!(dp->d_mode & ILARG)) {      // file is within 4k limit
        if (lblk <= 7) {
            if ((dp->d_addr[lblk] == 0) && alloc) {
                dp->d_addr[lblk] = balloc(ip->fs);
            }
            return dp->d_addr[lblk];
        } else if (!alloc) {
            return 0;
        }

        // convert to ILARG
        iblkno = balloc(ip->fs);
        bzero((char *)iblk, 512);
        for (i = 0; i < 8; i++) {
            iblk[i] = dp->d_addr[i];
            dp->d_addr[i] = 0;
        }
        dp->d_addr[0] = iblkno;
        dp->d_mode |= ILARG;
        writeblk(ip->fs, iblkno, (char *)iblk);
        iput(dp);     
        // fall into ILARG case
    }

    iindex = lblk / 256;
    if (iindex == 7) {       // double indirect
        if (dp->d_addr[7] == 0) {
            if (!alloc) {
                return 0;
            }
            iiblkno = dp->d_addr[7] = balloc(ip->fs);
            bzero((char *)iiblk, 512);
            writeblk(ip->fs, iiblkno, (char *)iiblk);
            iput(dp);
        }
        if (iiblkno != dp->d_addr[7]) {
            iiblkno = dp->d_addr[7];
            readblk(ip->fs, iiblkno, (char *)iiblk);
        }
        aa = iiblk;
        iindex -= 7 * 256;
    } else {
        aa = dp->d_addr;
    }

    // an indirect hole
    if (aa[iindex] == 0) {
        if (!alloc) {
            return 0;
        }
        aa[iindex] = balloc(ip->fs);
        if (aa == dp->d_addr) {
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

    if (dp->d_mode & IIO) {
        return;
    }
    if (!(dp->d_mode & ILARG)) {
        for (i = 0; i < 8; i++) {
            if (dp->d_addr[i]) {
                bfree(ip->fs, dp->d_addr[i]);
                dp->d_addr[i] = 0;
            }
        }
        goto done;
    }
    iblkno = -1;
    iiblkno = -1;

    for (i = 0; i < 7; i++) {
        iblkfree(ip->fs, &dp->d_addr[i]);
    }
    if (dp->d_addr[7]) {
        readblk(ip->fs, dp->d_addr[7], (char *)iiblk);
        for (i = 0; i < 256; i++) {
            iblkfree(ip->fs, &iiblk[i]);
        }
        writeblk(ip->fs, dp->d_addr[7], (char *)iiblk);
    }
done:
    dp->d_mode &= ~ILARG;
    dp->d_size0 = 0;
    dp->d_size1 = 0;
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

    if ((dp->d_mode & IFMT) != IFDIR) {
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
        dp->d_size1 -= 16;
        size = filesize(dp);
        if ((size % 512) == 0) {
            if (dp->d_mode & ILARG) {
                printf("can't handle ILARG directories\n");
            }
            bfree(fs, dp->d_addr[size / 512]);
            dp->d_addr[size/ 512] = 0;
        }
        iput(dp);
        putdir(dp, dirp);
    }
    ifree(dp);
    
    if (inum == 0) {
        return;
    }

    /*
     * now maybe remove the file
     */
    dp = iget(fs, inum);
    dp->d_nlink--;
    if (dp->d_nlink == 0) {
        filefree(dp);

        dp->d_mode = 0;
        if (fs->s_ninode != 100) {
            fs->s_inode[fs->s_ninode++] = inum;
        }
    }
    iput(dp);
    ifree(dp);
    fs->s_fmod = 1;
}

/*
 * link an inode to a name
 */
void
filelink(struct super *fs, char *path, int inum)
{
    char *dirname = strdup(path);
    char *name;
    struct dsknod *dp;
    struct dir *dirp;
    int entries;
    int size;
    int i;
 
    /*
     * get the directory inode
     */
    name = rindex(dirname, '/');
    if (!name) {
        dp = iget(fs, 1);
        name = dirname; 
    } else {
        *name++ = 0;
        dp = namei(fs, dirname);
    }

    if ((dp->d_mode & IFMT) != IFDIR) {
        goto lose;
    }

    /*
     * scan the directory for unallocated entries
     */
    entries = filesize(dp) / 16;

    dirp = getdir(dp);
    for (i = 0 ; i < entries; i++) {
        if (dirp[i].ino == 0) {
            break;
        }
    }

    /* fell off bottom */
    if (i >= entries) {
        dp->d_size1 += 16;
        /* if we need a new block */
        if ((dp->d_size1 & 511) == 0) {
            dirp = realloc(dirp, dp->d_size1);
        }
    }
    dirp[i].ino = inum;
    strncpy(dirp[i].name, name, 14);
    putdir(dp, dirp);
    iput(dp);
    fs->s_fmod = 1;

lose:
    if (dp) ifree(dp);
    free(dirname);
}

/*
 * get an inode from the freelist and allocate it.
 * if the freelist is empty, fail - too lazy to code
 */
int
ialloc(struct super *fs, UINT mode)
{
    int inum;
    struct dsknod *dp;
    int i;

    mode &= ~ILARG;

    if (fs->s_ninode <= 0) {
        return 0;
    }
    inum = fs->s_inode[--fs->s_ninode];
    dp = iget(fs, inum);
    if (dp->d_mode & IALLOC) {
        return 0;
    }

    dp->d_mode = IALLOC | mode;
    dp->d_nlink = 0;
    dp->d_uid = dp->d_gid = 0;
    dp->d_size1 = dp->d_size0 = 0;
    for (i = 0; i < 8; i++) {
        dp->d_addr[i] = 0;
    }
    printf("allocated inum %d\n", inum);
    iput(dp);
    ifree(dp);

    fs->s_fmod = 1;
    return inum;
}

/*
 * create an ordinary file using a path
 */
struct dsknod *
filecreate(struct super *fs, char *name)
{
    int inum;

    inum = ialloc(fs, IFREG);
    if (inum == 0) {
        printf("ialloc failed\n");
        return 0;
    }
    filelink(fs, name, inum);
    return (namei(fs, name));
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
