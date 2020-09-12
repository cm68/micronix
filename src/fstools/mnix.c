
/*
 * this is the V6 dcheck
 * it simply does a pass through all directories, counting file references.
 * then, it does another pass verifying that the link count is the same as the reference count
 */
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "fs.h"

int verbose;
char *filesystem;
int nerror;
char **command;

char superblock[512];
struct sup *fs;

int ls();
int cat();
int dumpcmd();
int readcmd();

struct cmdtab
{
    char *name;
    int (*handler)(int n, char **a);
    char *usage;
} cmds[] = {
    {"dump", dumpcmd, "dump <file>" },
    {"ls", ls, "ls [-a] <path>" },
    {"read", readcmd, "read <src> <dest>" },
    {"cat", cat, "cat <file>" }
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
                verbose++;
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
    if (verbose) {
        printf("filesystem: %s\n", filesystem);
    }
    image = open(filesystem, O_RDWR);
    if (image < 0) {
        printf("can't open %s\n", filesystem);
        exit(errno);
    }
    readblk(1, superblock);
    fs = (struct sup *) &superblock;

    if (verbose) {
        dumpsb(fs);
    }
    if (verbose > 1) {
        secdump(superblock);
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
    return (nerror);
}

#define	LS_A	1
#define	LS_L	2

#define	DENTS	(512 / 16)

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
ilist(char *name, int inum, int opts)
{
    struct dsknod *ip = iget(inum);

    printf("%5d ", inum);
    printf("%c", "-cdb"[(ip->mode >> 13) & 3]);
    printperm(ip->mode >> 6, ip->mode & ISETUID);
    printperm(ip->mode >> 3, ip->mode & ISETGID);
    printperm(ip->mode, 0);
    printf("%3d ", ip->nlinks);
    printf("%3d %3d ", ip->uid, ip->gid);
    if (ip->mode & IIO) {
        printf("%3d,%3d ", (ip->addr[0] >> 8) & 0xff, ip->addr[0] & 0xff);
    } else {
        printf("%7d ", (ip->size0 << 16) + ip->size1);
    }
    printf("%14s\n", name);
    free(ip);
}

#ifdef notdef
struct dir *
getdirent(struct dsknod *ip, int i)
{
    static struct dir dirbuf[DENTS];
    static int dblk = -1;
    int b;

    b = i / DENTS;
    i %= DENTS;

    if (dblk != ip->addr[b]) {
        dblk = ip->addr[b];
        readblk(dblk, (char *) dirbuf);
    }
    return &dirbuf[i];
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
#endif

char dirname[20];
int dino;

struct dsknod *
walkpath(char *name)
{
    char *s;
    struct dsknod *ip;

    dino = 1;
    ip = iget(dino);

    while (*name) {

        s = dirname;
        while (*name == '/')
            name++;
        while (*name && *name != '/') {
            *s++ = *name++;
        }
        *s = 0;
        if (strlen(dirname) == 0)
            break;
        dino = lookup(ip, dirname);
        if (!dino) {
            return 0;
        }
        free(ip);
        ip = iget(dino);
    }
    return ip;
}

void
list(char *name, int opts)
{
    struct dsknod *ip;
    struct dir *dp;
    int i;

    ip = walkpath(name);
    if (!ip) {
        return;
    }

    if ((ip->mode & ITYPE) == IDIR) {
        for (i = 0; i < ((ip->size0 << 16) + ip->size1) / 16; i++) {
            dp = getdirent(ip, i);
            if (dp->name[0] == '.' && !(opts & LS_A))
                continue;
            ilist(dp->name, dp->inum, opts);
        }
    } else {
        ilist(dirname, dino, opts);
    }
    free(ip);
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
            case 'l':
                opts |= LS_L;
                break;
            case 'a':
                opts |= LS_A;
                break;
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

    ip = walkpath(name);
    if (!ip) {
        return;
    }

    if ((ip->mode & ITYPE) == IDIR) {
        return;
    }

    size = (ip->size0 << 16) + ip->size1;
    for (i = 0; i < size; i += 512) {
        valid = fileread(ip, i, buf);
        write(1, buf, valid);
    }
    free(ip);
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

    ip = walkpath(name);
    if (!ip) {
        return;
    }

    if ((ip->mode & ITYPE) == IDIR) {
        return;
    }

    size = (ip->size0 << 16) + ip->size1;
    for (i = 0; i < size; i += 512) {
        valid = fileread(ip, i, buf);
        dump(buf, valid);
    }
    free(ip);
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
        printf("read <src> <dest>\n");
        return 1;
    }

    ip = walkpath(*a++);
    if (!ip || ((ip->mode & ITYPE) != IORD)) {
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
    free(ip);
    close(outfd);
    return 0;
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
