/*
 * a command line micronix filesystem reader/writer/lister
 * the interface really needs to look like tar
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

#include "fslib.h"
#include "util.h"

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
usage()
{
    int i;
    printf("commands:\n");
    for (i = 0; i < sizeof(cmds) / sizeof(cmds[0]); i++) {
        printf("\t%s\n", cmds[i].usage);
    }
}

struct sup *fs;

int
main(argc, argv)
    int argc;
    char **argv;
{
    int i;

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

    if (!filesystem) {
        filesystem = "testfs";
    }
    if (traceflags) {
        printf("filesystem: %s\n", filesystem);
    }
    i = openfs(filesystem, &fs);
    if (i < 0) {
        printf("can't open %s\n", filesystem);
        exit(errno);
    }

    if (traceflags) {
        dumpsb(fs);
    }

    if (!argc) {
        usage();
        exit(0);
    }
    nerror = -1;
    for (i = 0; i < sizeof(cmds) / sizeof(cmds[0]); i++) {
        if (strcmp(cmds[i].name, *argv) == 0) {
            nerror = (*cmds[i].handler) (argc, argv);
            break;
        }
    }
    if (nerror == -1) {
        usage();
    }
    closefs(fs);
    return (nerror);
}

int
iinfo(int c, char **a)
{
    int inum;
    struct dsknod *ip;
    char *na[20];

    if (!c) return 1;
    inum = atoi(*++a);

    ip = iget(fs, inum);
    if (!ip) return 2;
    sprintf(na, "inum %d\n", inum);
    isummary(na, ip);
    ifree(ip); 
    return 0;
}

void
list(char *name, int opts)
{
    struct dsknod *ip, *f;
    struct dir *dp;
    int i;

    ip = namei(fs, name);
    if (!ip) {
        printf("%s: not found\n", name);
        return;
    }

    if ((ip->mode & ITYPE) == IDIR) {
        for (i = 0; i < ((ip->size0 << 16) + ip->size1) / 16; i++) {
            dp = getdirent(ip, i);
            f = iget(fs, dp->inum);
            isummary(dp->name, f);
            ifree(f);
        }
    } else {
        isummary(name, ip);
    }
    ifree(ip);
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
    struct dsknod *ip;
    struct dir *dp;
    int i;
    int size;
    char buf[512];
    int valid;

    ip = namei(fs, name);
    if (!ip) {
        printf("%s: not found\n", name);
        return;
    }

    if ((ip->mode & ITYPE) == IDIR) {
        printf("%s: is directory\n", name);
        return;
    }

    size = (ip->size0 << 16) + ip->size1;
    for (i = 0; i < size; i += 512) {
        valid = fileread(ip, i, buf);
        write(1, buf, valid);
    }
    ifree(ip);
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
    struct dsknod *ip;
    struct dir *dp;
    int i;
    int size;
    char buf[512];
    int valid;

    ip = namei(fs, name);
    if (!ip) {
        printf("%s: not found\n", name);
        return;
    }

    if ((ip->mode & ITYPE) == IDIR) {
        printf("%s: is directory\n", name);
        return;
    }

    size = (ip->size0 << 16) + ip->size1;
    for (i = 0; i < size; i += 512) {
        valid = fileread(ip, i, buf);
        hexdump(buf, valid);
    }
    ifree(ip);
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
    struct dsknod *ip;
    struct dir *dp;
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

    ip = namei(fs, *a++);
    if (!ip) {
        printf("can't find file\n");
        return 2;
    }

    if ((ip->mode & ITYPE) != IORD) {
        printf("need regular file\n");
        return 2;
    }

    outfd = open(*a, O_WRONLY | O_CREAT, 0777);
    printf("write to %s\n", *a);
    size = (ip->size0 << 16) + ip->size1;
    for (i = 0; i < size; i += 512) {
        valid = fileread(ip, i, buf);
        write(outfd, buf, valid);
    }
    ifree(ip);
    close(outfd);
    return 0;
}

int
writecmd(int c, char **a)
{
    struct dsknod *ip;
    struct dir *dp;
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
    ip = namei(fs, destname);
    if (!ip) {
        printf("can't find file %s\n", destname);
        ip = filecreate(fs, destname);
        if (!ip) {
            return 2;
        }
    }

    if ((ip->mode & ITYPE) != IORD) {
        printf("need regular file\n");
        return 2;
    }

    filefree(ip);
    i = 0;
    do {
        valid = read(infd, buf, 512);
        if (filewrite(ip, i, buf) != 512) {
            printf("write failed\n");
        }
        i += valid;
    } while (valid == 512);
    ip->size0 = i >> 16;
    ip->size1 = i & 0xffff;
    iput(ip);
    close(infd);
    return 0;
}

int 
rmcmd(int c, char **a)
{
    struct dsknod *ip;
    struct dir *dp;
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
    struct dsknod *ip;
    struct dir *dp;
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

    ip = namei(fs, *a);
    idump(ip);

    return 0;
}

int 
emptycmd(int c, char **a)
{
    struct dsknod *ip;
    struct dir *dp;
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

    ip = namei(fs, *a);
    filefree(ip);

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

    bcopy(fs->free, fl, sizeof(fl));
    fi = fs->nfree - 1;

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
