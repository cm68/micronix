/*
 * use the imd library to do command-ish things with IMD files
 *
 * tools/imd.c
 * Changed: <2021-12-23 16:01:35 curt>
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
 * Where a filesystem block lands on the medium when the driver is doing
 * alternate sectoring - the ALT bit, bit 3 of a floppy node's minor.
 * sio() in sys/dj.c asks for every other sector of a track in turn, so
 * block l of a track is recorded at the sector numbered 2l, wrapped
 * back into the track, and stepped over the wrap only on the tracks
 * that have an even number of sectors.  secmap() in lib/fslib.c is the
 * same rule on the host side, and the two have to agree: this is what
 * makes an IMD this tool writes read back the way a shipped diskette
 * does.
 */
static int
altskew(int l, int spt)
{
	int s = l << 1;

	if (!(spt & 1) && s >= spt)
		s++;
	return s % spt;
}

/*
 * write out a new imd file that contains the data for the old data plus the delta
 *
 * altcyl, when it is not zero, is the first cylinder that carries
 * filesystem rather than the boot in front of it: its tracks and every
 * track after it are written alternated, the way the driver will read
 * them.  The boot is left alone because no driver stands between the
 * loader and the controller - it reads sectors by number, and a loader
 * laid down alternated would not run.  See altskew.
 */
void
merge_imd(struct imd *imd, char *filename, int altcyl)
{
    char merge[100];
    struct imd_trk *tp;
    int trk;
    int sec;
    int i;
    int l;
    int nsec;
    int skew;
    int inv[SECTORS];
    char value;
    char type;
    char *buf;
    int fd;

    /*
     * O_TRUNC: the merge is the whole file written from scratch, and a
     * merge beside an older longer one would otherwise keep that one's
     * tail past the end of the tracks just written.  Re-merging a disk
     * is the normal case - the images are built in a loop - and an IMD
     * with a few hundred stray bytes on the end reads as a track header
     * where there is no track.
     */
    sprintf(merge, "%s-merge", filename);
    fd = open(merge, O_RDWR|O_CREAT|O_TRUNC, 0777);
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
        /*
         * The secmap stays the identity and the alternation goes in the
         * data, which is where a shipped diskette has it: the sector
         * written in position sec is the block the driver will look for
         * there, altskew's inverse.
         */
        nsec = tp->fixed.nsec;
        skew = altcyl && tp->fixed.cyl >= altcyl && nsec <= SECTORS;
        if (skew)
            for (l = 0; l < nsec; l++)
                inv[altskew(l, nsec)] = l;
        for (sec = 0; sec < nsec; sec++) {
            buf = tp->data[skew ? inv[sec] : sec];
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
    printf("\t-a <cyl>\talternate-sector the tracks from cylinder <cyl> on\n");
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
    int altcyl = 0;

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
            case 'a':
                /*
                 * -a takes an argument, attached (-a2) or on its own
                 * (-a 2), so it cannot share a bundle with what
                 * follows it.
                 */
                if (s[1])
                    s++;
                else if (argc) {
                    s = *argv++;
                    argc--;
                } else
                    usage(0);
                altcyl = atoi(s);
                s += strlen(s);
                continue;
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
        if (merge) merge_imd(ip, *argv, altcyl);
        if (summarize) summarize_imd(ip, *argv);
        argv++;
    }
    exit(0);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
