/*
 * a command line micronix filesystem reader/writer/lister
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

struct cmdtab
{
    char *name;
    int (*handler)(int n, char **a);
    char *usage;
} cmds[] = {
    {"inode", iinfo, "inode <inum>" },
    {"fsinfo", fsinfo, "fsinfo" },
    {"empty", emptycmd, "empty <file>" },
    {"info", infocmd, "info <file>" },
    {"dump", dumpcmd, "dump <file>" },
    {"ls", ls, "ls [-a] <path>" },
    {"cat", cat, "cat <file>" },
    {"read", readcmd, "read <src> <dest>" },
    {"write", writecmd, "write <src> <dest>" },
    {"rm", rmcmd, "rm <file>" },
    {"mkdir", mkdircmd, "mkdir <directory>" },
    {"rmdir", rmdircmd, "rmdir <directory>" }
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

    if (!c) return 1;
    inum = atoi(*++a);

    dp = iget(fs, inum);
    if (!dp) return 2;
    sprintf(na, "inum %d\n", inum);
    isummary(na, dp);
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
writecmd(int c, char **a)
{
    struct dsknod *dp;
    int i;
    int size;
    char buf[512];
    int valid;
    int infd;
    char *destname;

    a++;
    c--;

    if (c != 2) {
        return -1;
    }

    infd = open(*a, O_RDONLY);
    if (infd < 0) {
        printf("can't open %s for reading %d\n", *a, errno);
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

    filefree(dp);
    i = 0;
    do {
        valid = read(infd, buf, 512);
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
    idump(dp);

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
