/*
 * write out the sectors of a disk image, in sector number order
 *
 * This is the raw view: sectors come back through imd_read, which is the
 * path the simulator's disk controllers take, so a sector here is the
 * sector a controller would fetch.  That is what you want for anything
 * outside the filesystem - boot blocks, the second level loader, the
 * reserved tracks - and it is not what mnix's block command gives you.
 * That one maps a block through the driver's interleave, reads block 0 as
 * zeros, and cannot address a 128 byte sector at all.
 *
 * Raw images get their geometry from their name, so they have to be
 * called bdev(2,minor) or be a symlink to one; IMD files carry their own.
 *
 * tools/dumpsec.c
 */

#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>

#include "../include/imd.h"
#include "../include/util.h"

int traceflags;
int trace_bio;

char *progname;
int quiet;

void
usage(char c)
{
    if (c) printf("unknown option %c\n", c);
    printf("usage: %s [options] <image> ...\n", progname);
    printf("\t-c <cyl>\tfirst cylinder (default 0)\n");
    printf("\t-n <count>\tcylinders to write (default 1)\n");
    printf("\t-q\t\tno track layout on stderr\n");
    printf("sectors are written to stdout\n");
    exit(1);
}

/*
 * one track, every sector it has, lowest numbered first.  A track does
 * not have to start at sector 1 - a hard sectored five inch disk starts
 * at 0 - so ask it where it starts rather than assuming.
 */
void
dumptrack(void *ip, int cyl, int head, char *fname)
{
    char buf[MAXSECSIZE];
    int nsec, secsize, first, n, got;

    imd_trkinfo(ip, cyl, head, &nsec, &secsize);
    if (nsec <= 0) {
        return;
    }
    first = imd_firstsec(ip, cyl, head);

    if (!quiet) {
        fprintf(stderr, "%s: cyl %d head %d: %d sectors of %d, from %d\n",
            fname, cyl, head, nsec, secsize, first);
    }
    for (n = 0; n < nsec; n++) {
        got = imd_read(ip, cyl, head, first + n, buf);
        if (got != secsize) {
            fprintf(stderr, "%s: cyl %d head %d sec %d: read %d of %d\n",
                fname, cyl, head, first + n, got, secsize);
            continue;
        }
        fwrite(buf, 1, secsize, stdout);
    }
}

int
main(int argc, char **argv)
{
    void *ip;
    char *s;
    int cyl = 0;
    int ncyl = 1;
    int c, head;

    progname = *argv++;
    argc--;

    while (argc) {
        s = *argv;
        if (*s++ != '-')
            break;
        argv++;
        argc--;
        while (*s) {
            switch (*s) {
            case 'c':
            case 'n':
                if (!argc) usage(*s);
                c = *s;
                if (c == 'c') cyl = atoi(*argv);
                else ncyl = atoi(*argv);
                argv++;
                argc--;
                break;
            case 'q':
                quiet++;
                break;
            case 'h':
                usage(0);
                break;
            default:
                usage(*s);
                break;
            }
            s++;
        }
    }

    if (!argc) {
        usage(0);
    }

    while (argc--) {
        ip = imd_load(*argv, 0, 0);
        if (!ip) {
            fprintf(stderr, "%s: can't load %s\n", progname, *argv);
            exit(1);
        }
        for (c = cyl; c < cyl + ncyl; c++) {
            for (head = 0; head < HEADS; head++) {
                dumptrack(ip, c, head, *argv);
            }
        }
        argv++;
    }
    exit(0);
}
