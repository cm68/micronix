#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "fslib.h"
#include "util.h"

struct sup *fs;
int dryrun;
int verbose;
int extra;

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
            tracec(extra, "%d: another link for %s\n", inum, df->name);
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

    if (dryrun) {
        return;
    }

    sprintf(namebuf, "%s%s", destname, name);
    unlink(namebuf);
    if ((fd = open(namebuf, O_CREAT | O_RDWR | O_TRUNC)) < 0) {
        perror(namebuf);
    }

    for (off = 0; off < size; off += 512) {
        fileread(ip, off, buf);
        if (traceflags & extra) {
            printf("\noffset %d logical block %d\n", off, off / 512);
            hexdump(buf, size - off);
        }
        write(fd, buf, (size - off > 512) ? 512 : size - off);
    }
    close(fd);
    chmod(namebuf, ip->mode & 0777);
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
        if (**argv == '-') switch ((*argv)[1]) {
        case 'n':
            dryrun++;
            // fall through
        case 'v':
            if (traceflags & verbose) traceflags |= extra;
            traceflags |= verbose;
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
        } else {
            break;
        }
    }
    if (!argc) {
        printf("need image name\n");
        return -1;
    }

    sprintf(namebuf, "mkdir -p %s\n", destname);
    tracec(verbose,  "send to %s\n", destname);
    if (!dryrun) {
        system(namebuf);
    }

    printf("copyall from image %s to %s using %s\n", *argv, destname,
        mountpt);

    i = openfs(*argv, &fs);
    if (i < 0) {
        lose("open");
    }

    df = dfname(1, mountpt);
    df->up = df;

    /*
     * read all the directory inodes and build the file list 
     */
    for (i = 1; i < fs->isize * I_PER_BLK; i++) {
        ip = iget(fs, i);
        if ((ip->mode & ITYPE) != IDIR)
            continue;
        tracec(extra, "directory %d\n", i);
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
        ip = iget(fs, i);
        if (!(ip->mode & IALLOC))
            continue;
        if ((ip->mode & ITYPE) != IDIR)
            continue;
        df = dfget(i);
        sprintf(namebuf, "mkdir -p %s%s", destname, df->fullname);
        tracec(verbose, "%s (%d)\n", namebuf, i);
        if (!dryrun) {
            system(namebuf);
        }
    }

    /*
     * now we have names and places for everything- start the work 
     */
    for (i = 1; i < fs->isize * I_PER_BLK; i++) {
        ip = iget(fs, i);
        if (!(ip->mode & IALLOC))
            continue;
        if ((ip->mode & ITYPE) != IORD)
            continue;
        /*
         * only allocated ifreg 
         */
        df = dfget(i);
        s = df->fullname;
        tracec(verbose, "regular file %d %s (length %d)\n", i, s, ip->size1);
        dofile(df->fullname, ip);
    }
}

__attribute__((constructor))
void
main_init()
{
    verbose = register_trace("verbose");
    extra = register_trace("extraverbose");
}


/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
