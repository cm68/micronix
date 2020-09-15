#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "fs.h"

char *mountpt = "/";
char *destname = "destdir";

char fsbuf[512];

struct dfile
{
    UINT inum;
    char *name;
    char *fullname;
    struct dfile *up;
    struct dfile *next;
};

char *strdup();

struct dfile *head;

char namebuf[100];

struct dfile *
dfget(UINT inum)
{
    struct dfile *df;

    for (df = head; df; df = df->next) {
        if (df->inum == inum) {
            break;
        }
    }
    return df;
}

struct dfile *
dfname(UINT inum, char *name)
{
    struct dfile *df;

    df = dfget(inum);
    if (df) {
        if (df->name) {
            if (verbose)
                printf("another name for %d\n", inum);
        } else {
            df->name = strdup(name);
        }
    } else {
        df = malloc(sizeof(*df));
        df->fullname = 0;
        df->inum = inum;
        if (name) {
            df->name = strdup(name);
        } else {
            df->name = 0;
        }
        df->next = head;
        head = df;
    }
    return df;
}

char *
iname(UINT i)
{
    struct dfile *df;

    for (df = head; df; df = df->next) {
        if (df->inum == i) {
            return (df->fullname);
        }
    }
    printf("lose: no name for %d\n", i);
}

void
descend(struct dfile *df)
{
    if (!df)
        return;
    if (df->inum == 1) {
        return;
    }
    descend(df->up);
    strcat(namebuf, "/");
    strcat(namebuf, df->name);
}

void
dofile(char *name, struct dsknod *ip)
{
    int fd;
    int off;
    int size = ip->size1 + (ip->size0 << 16);
    char buf[512];

    sprintf(namebuf, "%s%s", destname, name);
    if ((fd = open(namebuf, O_CREAT | O_RDWR | O_TRUNC)) < 0) {
        perror(namebuf);
    }

    if (verbose)
        printf("file contents of %s size %d\n", name, size);
    for (off = 0; off < size; off += 512) {
        fileread(ip, off, buf);
        if (verbose) {
            printf("\noffset %d logical block %d\n", off, off / 512);
            dump(buf, size - off);
        }
        write(fd, buf, (size - off > 512) ? 512 : size - off);
    }
    close(fd);
}

int
main(int argc, char **argv)
{
    int i;
    int d;
    struct dsknod *ip;
    struct dir *dp;
    struct dfile *df;
    char *s;
    struct dfile *pd;

    while (--argc) {
        argv++;
        if (**argv == '-')
            switch ((*argv)[1]) {
            case 'v':
                verbose++;
                continue;

            case 'd':
                destname = *++argv;
                argc--;
                continue;
            case 'm':
                mountpt = *++argv;
                argc--;
                continue;
            default:
                printf("Bad flag\n");
            }
    }

    sprintf(namebuf, "mkdir -p %s\n", destname);
    if (verbose) {
        printf("send to %s\n", destname);
    }
    system(namebuf);

    printf("copyall from image %s to %s using %s\n", *argv, destname,
        mountpt);
    if ((image = open(*argv, O_RDONLY)) < 0)
        lose("open");

    readsuper();

    df = dfname(1, mountpt);
    df->up = df;

    /*
     * read all the directory inodes and build the file list 
     */
    for (i = 1; i < fs->isize * I_PER_BLK; i++) {
        ip = iget(i);
        if ((ip->mode & ITYPE) != IDIR)
            continue;
        if (verbose)
            printf("directory %d\n", i);
        pd = dfname(i, 0);
        for (d = 0; (dp = getdirent(ip, d)); d++) {
            if (strcmp(dp->name, ".") == 0)
                continue;
            if (strcmp(dp->name, "..") == 0)
                continue;
            df = dfname(dp->inum, dp->name);
            df->up = pd;
        }
    }
    /*
     * resolve names 
     */
    for (df = head; df; df = df->next) {
        namebuf[0] = '\0';
        descend(df);
        df->fullname = strdup(namebuf);
    }

    /*
     * let's make directories 
     */
    for (i = 1; i < fs->isize * I_PER_BLK; i++) {
        ip = iget(i);
        if (!(ip->mode & IALLOC))
            continue;
        if ((ip->mode & ITYPE) != IDIR)
            continue;
        df = dfget(i);
        sprintf(namebuf, "mkdir -p %s%s", destname, df->fullname);
        system(namebuf);
    }

    /*
     * now we have names and places for everything- start the work 
     */
    for (i = 1; i < fs->isize * I_PER_BLK; i++) {
        ip = iget(i);
        if (!(ip->mode & IALLOC))
            continue;
        if ((ip->mode & ITYPE) != IORD)
            continue;
        /*
         * only allocated ifreg 
         */
        df = dfget(i);
        s = df->fullname;
        if (verbose)
            printf("regular file %d %s (length %d)\n", i, s, ip->size1);
        dofile(df->fullname, ip);
    }
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
