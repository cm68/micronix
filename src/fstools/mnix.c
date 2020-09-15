
/*
 * this is the V6 dcheck
 * it simply does a pass through all directories, counting file references.
 * then, it does another pass verifying that the link count is the same as the reference count
 */
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <errno.h>

#include "fs.h"

int verbose;
char *filesystem;
int nerror;
char **command;

int ls();
int cat();
int dumpcmd();
int readcmd();
int writecmd();
int rmcmd();
int mkdircmd();
int rmdircmd();

struct cmdtab
{
    char *name;
    int (*handler)(int n, char **a);
    char *usage;
} cmds[] = {
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

    readsuper();

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

void
list(char *name, int opts)
{
    struct dsknod *ip, *f;
    struct dir *dp;
    int i;

    ip = namei(name);
    if (!ip) {
        printf("%s: not found\n", name);
        return;
    }

    if ((ip->mode & ITYPE) == IDIR) {
        for (i = 0; i < ((ip->size0 << 16) + ip->size1) / 16; i++) {
            dp = getdirent(ip, i);
            f = iget(dp->inum);
            ilist(dp->name, f);
            ifree(f);
        }
    } else {
        ilist(name, ip);
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

    ip = namei(name);
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

    ip = namei(name);
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
        dump(buf, valid);
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

    ip = namei(*a++);
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
    ip = namei(destname);
    if (!ip) {
        printf("can't find file %s\n", destname);
        ip = filecreate(destname);
        if (!ip) {
            return 2;
        }
    }

    if ((ip->mode & ITYPE) != IORD) {
        printf("need regular file\n");
        return 2;
    }

    return 0;

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
    writesuper();
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

    fileunlink(*a);
    writesuper();
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
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
