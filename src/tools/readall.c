/*
 * readall starts from the root and spews all the files into the destdir
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../micronix/include/types.h"
#include "../micronix/include/sys/sup.h"
#include "../micronix/include/sys/dir.h"
#include "../micronix/include/sys/inode.h"
#include "../include/fslib.h"
#include "../include/util.h"

struct sup *fs;
int dryrun;
int verbose;
int extra;

char *strdup();

char *destname = "destdir";

/*
 * given the inumber of a directory, process all the files
 */
void
dive(struct dsknod *dip, char *name)
{
    struct dsknod *ip;
    struct dir *dp;
    int i;
    int entries;
    char *namebuf;

    tracec(verbose, "directory %s (length %d)\n", name, filesize(dip));

    if ((dip->mode & ITYPE) != IDIR) {
        lose("not directory");
    }

    namebuf = malloc(strlen(name) + 15);

    sprintf(namebuf, "mkdir -p %s\n", name);
    if (!dryrun) {
        system(namebuf);
    }
    dp = getdir(dip);
    entries = filesize(dip) / sizeof(struct dir);

    for (i = 0; i < entries; i++) {
        ip = iget(fs, dp[i].ino);
        if (!(ip->mode & IALLOC))
            continue;

        sprintf(namebuf, "%s/%s", name, dp[i].name);

        switch (ip->mode & ITYPE) {
        case IDIR:
            if (strcmp(dp[i].name, ".") == 0) continue;
            if (strcmp(dp[i].name, "..") == 0) continue;
            dive(ip, namebuf);
            continue;
        case IORD:
            dofile(namebuf, ip);
            break;
        case IBIO:
        case ICIO:
            dospecial(namebuf, ip);
            break;
        }
    }
}

void
dofile(char *name, struct dsknod *ip)
{
    int fd;
    int off;
    int size = ip->size1 + (ip->size0 << 16);
    char buf[512];

    tracec(verbose, "regular file %s (length %d)\n", name, size);

    if (dryrun) {
        return;
    }

    unlink(name);
    if ((fd = open(name, O_CREAT | O_RDWR | O_TRUNC)) < 0) {
        perror(name);
    }

    for (off = 0; off < size; off += 512) {
        fileread(ip, off, buf);
        if (traceflags & extra) {
            printf("\noffset %d logical block %d\n", off, off / 512);
        }
        write(fd, buf, (size - off > 512) ? 512 : size - off);
    }
    close(fd);
    chmod(name, ip->mode & 0777);
}

/*
 * colossal hack: character and block special files
 * are done as symbolic links with special form:
 * cdev(major,minor) or bdev(major,minor)
 * zero flexibility in format. major and minor are ascii decimal
 */
void
dospecial(char *name, struct dsknod *ip)
{
    char linkname[20];

    sprintf(linkname, "%cdev(%d,%d)", 
        ((ip->mode & ITYPE) == IBIO) ? 'b' :'c',
        (ip->addr[0] >> 8) & 0xff,
        ip->addr[0] & 0xff);

    tracec(verbose, "special file %s %s\n", name, linkname);

    if (dryrun)
        return;

    unlink(name);
    symlink(linkname, name);
}

char *pname;

usage()
{
    fprintf(stderr, "usage:\n%s <options> image\n", pname);
    fprintf(stderr, "\t-d <dirname>  place files in directory\n");
    fprintf(stderr, "\t-v            increment verbosity\n");
    fprintf(stderr, "\t-n            dry run\n");
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

    pname = argv[0];

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
        default:
            usage();
        } else {
            break;
        }
    }
    if (!argc) {
        printf("need image name\n");
        return -1;
    }

    printf("copyall from image %s to %s\n", *argv, destname);

    i = openfs(*argv, &fs);
    if (i < 0) {
        lose("open");
    }

    dive(iget(fs, 1), destname);
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
