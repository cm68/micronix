/*
 * a command line micronix filesystem reader/writer/lister
 *
 * tools/mnix.c
 * Changed: <2023-06-19 19:16:00 curt>
 *
 * the interface really needs to look like tar: 
 *
 * mar -xf <image> micronix       - extract /micronix
 * mar -rf <image> micronix       - replace or add /micronix
 * mar -df <image> micronix       - delete /micronix
 * mar -tf <image> [file list]    - list of files
 * mar -cf <image> [ <size> ]     - create an empty filesystem 
 *
 * with the usual -v option for verbosity
 */
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>

#include "../micronix/include/types.h"
#include "../micronix/include/sys/fs.h"
#include "../micronix/include/sys/dir.h"
#include "../include/fslib.h"
#include "../include/util.h"

int traceflags;

char *filesystem;
int nerror;
char **command;

int ls();
int cat();
int emptycmd();
int infocmd();
int dumpcmd();
int readcmd();
int writecmd();
int rmcmd();
int mkdircmd();
int rmdircmd();
int fsinfo();
int iinfo();
int blkcmd();
int setblkcmd();

struct cmdtab
{
    char *name;
    int (*handler)(int n, char **a);
    char *usage;
} cmds[] = {
    {"inode", iinfo, "inode [-f] <inum>" },
    {"fsinfo", fsinfo, "fsinfo" },
    {"empty", emptycmd, "empty <file>" },
    {"info", infocmd, "info <file>" },
    {"dump", dumpcmd, "dump <file>" },
    {"ls", ls, "ls [-a] <path>" },
    {"cat", cat, "cat <file>" },
    {"read", readcmd, "read <src> <dest>" },
    {"write", writecmd, "write [-k] <src> <dest>" },
    {"rm", rmcmd, "rm <file>" },
    {"mkdir", mkdircmd, "mkdir <directory>" },
    {"rmdir", rmdircmd, "rmdir <directory>" },
    {"block", blkcmd, "block [-e] <blkno>" },
    {"setblk", setblkcmd, "setblk <path> <blkno> ..." }
};

void
usage(char *e)
{
    int i;
    printf("%s [-f <filesystem>] <command>\n", e);
    printf("commands:\n");
    for (i = 0; i < sizeof(cmds) / sizeof(cmds[0]); i++) {
        printf("\t%s\n", cmds[i].usage);
    }
}

/*
 * a seperate command arg processor
 */
char opt[26];       // indexed by 'a' to 'z'

void
readopt(int *argc, char ***argv, char *allowed)
{
    char c;
    char *s;

    (*argv)++;
    s = **argv;
    (*argc)--;
    if (*s == '-') {
        while (c = *++s) {
            if ((c >= 'a') && (c <= 'z') && index(allowed, c)) {
                opt[c-'a']++;
            } else {
                printf("bogus option: %c\n", c);
                break;
            }
        }
        (*argc)--;
        (*argv)++;
    }
    return;
}

struct super *fs;

int
main(argc, argv)
    int argc;
    char **argv;
{
    int i;
    char *pname = argv[0];

    putenv("TZ=GMT");
    tzset();

    filesystem = getenv("MNIXFILESYSTEM");

    while (--argc) {
        argv++;
        if (**argv == '-')
            switch ((*argv)[1]) {
            case 'v':
                traceflags = -1;
                continue;
            case 'f':
                filesystem = *++argv;
                argc--;
                continue;
            default:
                printf("Bad flag\n");
        } else {
            break;
        }
    }

    if (!argc) {
        usage(pname);
        exit(0);
    }
    if (!filesystem) {
        filesystem = "testfs";
    }
    if (traceflags) {
        printf("filesystem: %s\n", filesystem);
    }
    i = openfsrw(filesystem, &fs, 1);
    if (i < 0) {
        printf("can't open %s\n", filesystem);
        exit(errno);
    }

    if (traceflags) {
        printf("superblock\n");
        dumpsb(fs);
    }

    nerror = -1;
    for (i = 0; i < sizeof(cmds) / sizeof(cmds[0]); i++) {
        if (strcmp(cmds[i].name, *argv) == 0) {
            nerror = (*cmds[i].handler) (argc, argv);
            break;
        }
    }
    if (nerror == -1) {
        usage(pname);
    }
    closefs(fs);
    return (nerror);
}

int
iinfo(int c, char **a)
{
    int inum;
    struct dsknod *dp;
    char na[20];

    readopt(&c, &a, "f");

    if (!c) return 1;
    inum = atoi(*a);

    dp = iget(fs, inum);
    if (!dp) return 2;
    sprintf(na, "inum %d\n", inum);
    idump(na, dp, opt['f'-'a']);
    ifree(dp); 
    return 0;
}

void
list(char *name, int opts)
{
    struct dsknod *dp, *f;
    struct dir *dirp;
    int i;

    dp = namei(fs, name);
    if (!dp) {
        printf("%s: not found\n", name);
        return;
    }

    if ((dp->d_mode & IFMT) == IFDIR) {
        for (i = 0; i < ((dp->d_size0 << 16) + dp->d_size1) / 16; i++) {
            dirp = getdirent(dp, i);
            f = iget(fs, dirp->ino);
            isummary(dirp->name, f);
            ifree(f);
        }
    } else {
        isummary(name, dp);
    }
    ifree(dp);
}

int
ls(int c, char **a)
{
    int i;
    char *s;
    char o;
    int opts = 0;

    c--;
    a++;

    while (c) {
        if (**a != '-') {
            break;
        }
        s = *a;
        s++;
        while (*s) {
            switch (o = *s++) {
            default:
                printf("unknown option %c\n", o);
                break;
            }
        }
        a++;
        c--;
    }
    if (c) {
        while (c--) {
            list(*a++, opts);
        }
    } else {
        list("/", opts);
    }
    return 0;
}

void
catfile(char *name, int opts)
{
    struct dsknod *dp;
    int i;
    int size;
    char buf[512];
    int valid;

    dp = namei(fs, name);
    if (!dp) {
        printf("%s: not found\n", name);
        return;
    }

    if ((dp->d_mode & IFMT) == IFDIR) {
        printf("%s: is directory\n", name);
        return;
    }

    size = (dp->d_size0 << 16) + dp->d_size1;
    for (i = 0; i < size; i += 512) {
        valid = fileread(dp, i, buf);
        write(1, buf, valid);
    }
    ifree(dp);
}

int
cat(int c, char **a)
{
    int i;
    char *s;
    char o;
    int opts = 0;

    c--;
    a++;

    while (c--) {
        catfile(*a++, opts);
    }
    return 0;
}

void
dumpfile(char *name, int opts)
{
    struct dsknod *dp;
    int i;
    int size;
    char buf[512];
    int valid;

    dp = namei(fs, name);
    if (!dp) {
        printf("%s: not found\n", name);
        return;
    }

    if ((dp->d_mode & IFMT) == IFDIR) {
        printf("%s: is directory\n", name);
        return;
    }

    size = (dp->d_size0 << 16) + dp->d_size1;
    for (i = 0; i < size; i += 512) {
        valid = fileread(dp, i, buf);
        hexdump(buf, valid);
    }
    ifree(dp);
}

int
dumpcmd(int c, char **a)
{
    int i;
    char *s;
    char o;
    int opts = 0;

    c--;
    a++;

    while (c--) {
        dumpfile(*a++, opts);
    }
    return 0;
}

int
readcmd(int c, char **a)
{
    struct dsknod *dp;
    int i;
    int size;
    char buf[512];
    int valid;
    int outfd;

    a++;
    c--;

    if (c != 2) {
        return -1;
    }

    dp = namei(fs, *a++);
    if (!dp) {
        printf("can't find file\n");
        return 2;
    }

    if ((dp->d_mode & IFMT) != IFREG) {
        printf("need regular file\n");
        return 2;
    }

    outfd = open(*a, O_WRONLY | O_CREAT, 0777);
    if (outfd < 0) {
        printf("can't open %s for writing: %d\n", *a, errno);
        ifree(dp);
        return 2;
    }
    printf("write to %s\n", *a);
    size = (dp->d_size0 << 16) + dp->d_size1;
    for (i = 0; i < size; i += 512) {
        valid = fileread(dp, i, buf);
        write(outfd, buf, valid);
    }
    ifree(dp);
    close(outfd);
    return 0;
}

int
/*
 * write <src> <dest>, and write -k <src> <dest>.
 *
 * Without -k the destination is emptied first and the contents land on
 * whatever the free list hands out.  That is what you want for an
 * ordinary file and is exactly wrong for a boot area, which has to stay
 * on the blocks the rom will read: freeing it puts those blocks back on
 * the free list and the write then lands somewhere else entirely.
 *
 * -k keeps the block list and writes through it, so the bytes go where
 * the file already is.  It is the other half of setblk - set the blocks,
 * then write -k onto them - and it refuses to grow the file, because a
 * block that had to be allocated would not be part of the area and the
 * boot would be half in it.
 */
writecmd(int c, char **a)
{
    struct dsknod *dp;
    int i;
    int size;
    char buf[512];
    int valid;
    int infd;
    char *destname;
    int keep = 0;
    int have;
    struct stat sb;

    a++;
    c--;

    if (c && strcmp(*a, "-k") == 0) {
        keep = 1;
        a++;
        c--;
    }

    if (c != 2) {
        return -1;
    }

    infd = open(*a, O_RDONLY);
    if (infd < 0) {
        /*
         * And stop.  This used to say so and carry on, which meant
         * every read below returned -1, nothing was written, and the
         * size was set from a byte count that had gone negative - so
         * a mistyped source name replaced the destination with a file
         * of 16777215 bytes.  /etc/init went that way once.
         */
        printf("can't open %s for reading: %d\n", *a, errno);
        return 2;
    }
    
    destname = *++a;
    dp = namei(fs, destname);
    if (!dp) {
        printf("can't find file %s\n", destname);
        dp = filecreate(fs, destname);
        if (!dp) {
            printf("can't create file %s\n", destname);
            return 2;
        }
    }

    if ((dp->d_mode & IFMT) != IFREG) {
        printf("need regular file\n");
        return 2;
    }

    if (keep) {
        /*
         * How much room the file already has.  bmap with alloc 0
         * answers 0 for a block that is not there, so this stops at
         * the first hole and counts what is behind it.
         */
        have = 0;
        while (bmap(dp, have * 512, 0)) {
            have++;
        }
        if (fstat(infd, &sb) == 0 && sb.st_size > have * 512) {
            printf("%s holds %d blocks, %s needs %d - refusing to grow it\n",
                destname, have, *(a - 1), (int)((sb.st_size + 511) / 512));
            close(infd);
            return 2;
        }
    } else {
        filefree(dp);
    }
    i = 0;
    do {
        valid = read(infd, buf, 512);
        if (valid < 0) {
            /*
             * Whatever has been written stays; the size is what was
             * actually read, not what the count reached after a -1.
             */
            printf("read error on the source: %d\n", errno);
            break;
        }
        if (filewrite(dp, i, buf) != 512) {
            printf("write failed\n");
        }
        i += valid;
    } while (valid == 512);
    dp->d_size0 = i >> 16;
    dp->d_size1 = i & 0xffff;
    iput(dp);
    close(infd);
    return 0;
}

int 
rmcmd(int c, char **a)
{
    struct dsknod *dp;
    int i;
    int size;
    char buf[512];
    int valid;
    int infd;
    char *dirname;
    char *filename;

    a++;
    c--;

    if (c != 1) {
        return -1;
    }

    fileunlink(fs, *a);
    return 0;
}

int 
infocmd(int c, char **a)
{
    struct dsknod *dp;
    int i;
    int size;
    char buf[512];
    int valid;
    int infd;
    char *dirname;
    char *filename;

    a++;
    c--;

    if (c != 1) {
        return -1;
    }

    dp = namei(fs, *a);
    idump(*a, dp, 0);

    return 0;
}

int 
emptycmd(int c, char **a)
{
    struct dsknod *dp;
    int i;
    int size;
    char buf[512];
    int valid;
    int infd;
    char *dirname;
    char *filename;

    a++;
    c--;

    if (c != 1) {
        return -1;
    }

    dp = namei(fs, *a);
    filefree(dp);

    return 0;
}

int 
rmdircmd(int c, char **a)
{
    return 1;
}

int 
mkdircmd(int c, char **a)
{
    return 1;
}


/*
 * Give a file a block list of our choosing.
 *
 * This exists for one job: a boot area has to be at a fixed place on the
 * platter, and the only thing in a v6 filesystem that keeps a block from
 * being handed out is an inode claiming it.  So the file is made to own
 * the blocks the hardware will read, and then writing the file writes
 * the boot area, through the same mapping the kernel and the rom both
 * use rather than a second opinion about where the sector is.
 *
 * Two things this does not do, deliberately.  It does not take the
 * blocks off the free list - run icheck -s afterwards, which rebuilds
 * the list from what the inodes claim, and until you do the blocks are
 * both owned and queued to be handed out.  And it does not write any
 * contents; set the blocks first, then write the file, and the write
 * lands on them.
 */
int
setblkcmd(int c, char **a)
{
    struct dsknod *dp;
    UINT blocks[256];
    UINT iblk[256];
    int nblk = 0;
    int iblkno;
    char *path;
    int i;

    a++;
    c--;
    if (c < 2) {
        return -1;
    }
    path = *a++;
    c--;

    while (c-- && nblk < 256) {
        blocks[nblk++] = atoi(*a++);
    }

    dp = namei(fs, path);
    if (!dp) {
        printf("can't find %s\n", path);
        return 2;
    }
    if ((dp->d_mode & IFMT) != IFREG) {
        printf("%s is not a regular file\n", path);
        return 2;
    }

    /*
     * Eight addresses in an inode.  Up to eight blocks they are the
     * blocks; past that they are indirect blocks holding 256 apiece, and
     * the file has to say so with ILARG.  A cylinder is more than eight
     * blocks on every drive we care about, so the second case is the
     * usual one here, not the exception.
     */
    if (nblk <= 8) {
        dp->d_mode &= ~ILARG;
        for (i = 0; i < 8; i++) {
            dp->d_addr[i] = (i < nblk) ? blocks[i] : 0;
        }
    } else {
        iblkno = balloc(fs);
        if (!iblkno) {
            printf("no block for the indirect\n");
            return 2;
        }
        bzero((char *)iblk, 512);
        for (i = 0; i < nblk; i++) {
            iblk[i] = blocks[i];
        }
        writeblk(fs, iblkno, (char *)iblk);
        for (i = 0; i < 8; i++) {
            dp->d_addr[i] = 0;
        }
        dp->d_addr[0] = iblkno;
        dp->d_mode |= ILARG;
    }

    /*
     * And the size, or it is an empty file with a block list: reads
     * return nothing and a write goes to the allocator instead of to the
     * blocks sitting right there in the inode.
     */
    dp->d_size0 = 0;
    dp->d_size1 = nblk * 512;
    iput(dp);

    printf("%s: %d blocks, %d bytes%s", path, nblk, nblk * 512,
        (nblk > 8) ? ", ILARG" : "");
    if (nblk > 8) {
        printf(", indirect in %d", iblkno);
    }
    printf("\nnow run icheck -s to rebuild the free list\n");
    return 0;
}

int
blkcmd(int c, char **a)
{
    unsigned char buf[512];
    unsigned char newbuf[512];
    int blkno;

    readopt(&c, &a, "e");

    if (!c) return 1;
    blkno = atoi(*a);

    readblk(fs, blkno, buf);
    if (opt['e'-'a']) {
        bcopy(buf, newbuf, 512);
        blockedit(newbuf, 512);
        if (bcmp(buf, newbuf, 512) != 0) {
            printf("dirty\n");
            writeblk(fs, blkno, newbuf);
        }
    } else {
        hexdump(buf, 512);
    } 
    return 0;
}

int
fsinfo(int c, char **a)
{
    unsigned short buf[256];
    int b;
    unsigned short fl[100];
    int fi;
    int k;

    dumpsb(fs);
    printf("free list: \n");

    bcopy(fs->s_free, fl, sizeof(fl));
    fi = fs->s_nfree - 1;

    /* scan through whole freelist from end */
    k = 0;
    while ((b = fl[fi]) != 0) {
        if (!fi) {
            readblk(fs, b, (char *)buf);
            fi = buf[0] - 1;
            bcopy(&buf[1], fl, sizeof(fl));
            printf(" ...");
        } else {
            fi--;
        }
        printf("%6d ", b);
        k++;
        if (k == 8) {
            k = 0;
            printf("\n");
        }
    }
    if (k) printf("\n");
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
