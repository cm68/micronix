/*
 * imd.c
 *
 * use the imd library to do command-ish things with IMD files
 */

#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>

#include "imd.h"
#include "util.h"

int traceflags;
int trace_bio;

void
summarize_imd(struct imd *imd, char *filename)
{
    int t;
    int maxsec = 0;
    int minsec = 99999;
    int maxsecsize = 0;
    int minsecsize = 99999;
    struct imd_trk *tp;
    int tcnt = 0;

    for (t = 0; t < TRACKS; t++) {
        tp = imd->tracks[t];
        if (!tp) continue;
        if (tp->secsize > maxsecsize) maxsecsize = tp->secsize;
        if (tp->secsize < minsecsize) minsecsize = tp->secsize;
        if (tp->fixed.nsec > maxsec) maxsec = tp->fixed.nsec;
        if (tp->fixed.nsec < minsec) minsec = tp->fixed.nsec;
        tcnt++;
    }

    printf("comment: %s\n", imd->comment);
    printf("tracks: %d cyls: %d heads: %d secs(%d-%d) secsize(%d-%d)\n\n", 
        tcnt, imd->cyls, imd->heads, minsec, maxsec, minsecsize, maxsecsize);
}

void
dump_imd(struct imd *imd, char *filename)
{
    int t;
    struct imd_trk *tp;

    printf("comment: %s\n", imd->comment);
    printf("cyls: %d heads: %d\n", imd->cyls, imd->heads);

    for (t = 0; t < TRACKS; t++) {
        tp = imd->tracks[t];
        if (!tp) continue;
        imd_dump_track(tp);
   }
}

/*
 * write out a new imd file that contains the data for the old data plus the delta
 */
void
merge_imd(struct imd *imd, char *filename)
{
    char merge[100];
    struct imd_trk *tp;
    int trk;
    int sec;
    int i;
    char value;
    char type;
    char *buf;
    int fd;
 
    sprintf(merge, "%s-merge", filename);
    fd = open(merge, O_RDWR|O_CREAT, 0777);
    write(fd, imd->comment, strlen(imd->comment) - 1);
    value = IMD_EOC;
    write(fd, &value, 1);
    for (trk = 0; trk < TRACKS; trk++) {
        tp = imd->tracks[trk];    
        if (!tp) continue;
        write(fd, &tp->fixed, sizeof(tp->fixed));
        if (tp->secmap) write(fd, tp->secmap, tp->fixed.nsec);
        if (tp->cylmap) write(fd, tp->cylmap, tp->fixed.nsec);
        if (tp->headmap) write(fd, tp->headmap, tp->fixed.nsec);
        for (sec = 0; sec < tp->fixed.nsec; sec++) {
            buf = tp->data[sec];
            if (buf) {
                type = IMD_FILL;
                value = buf[0];
                for (i = 0; i < tp->secsize; i++) {
                    if (buf[i] != value) {
                        type = IMD_DATA;
                        break;
                    }
                }
                write(fd, &type, 1);
                if (type == IMD_FILL) {
                    write(fd, &value, 1);
                } else {
                    write(fd, buf, tp->secsize);
                }
            } else {
                type = IMD_ABSENT;
                write(fd, &type, 1);                
            }
        }    
    }
    close(fd);
}

char *progname;

void
usage(char c)
{
    if (c) printf("unknown option %c\n", c);
    printf("usage: %s [options] <imd file> ...\n", progname);
    printf("\t-m\tmerge deltas\n");
    printf("\t-d\tdump data\n");
    printf("\t-s\tsummarize\n");
    exit(1);
}

int
main(int argc, char **argv)
{
    struct imd *ip;
    char *s;
    int dump = 0;
    int merge = 0;
    int summarize = 0;

    progname = *argv++;
    argc--;

    while (argc) {
        s = *argv;
        if (*s++ != '-')
            break;
        argv++;
        argc--;
        while (*s) {
            switch(*s) {
            case 's':
                summarize++;
                break;
            case 'd':
                dump++;
                break;
            case 'm':
                merge++;
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

    while (argc--) {
        printf("%s\n", *argv);
        ip = (struct imd *)imd_load(*argv, 0, 0);
        if (!ip) {
            printf("can't load %s\n", *argv);
            exit(1);
        }
        if (!merge) {
            ip->comment[strlen(ip->comment)-1] = 0;
        }
        if (!(dump || merge || summarize)) {
            printf("%s\n", ip->comment);
        }
        if (dump) dump_imd(ip, *argv);
        if (merge) merge_imd(ip, *argv);
        if (summarize) summarize_imd(ip, *argv);
        argv++;
    }
    exit(0);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
