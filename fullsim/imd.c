
/*
 * this snarfs an imd file into memory, suitable for a floppy driver to
 * to read it by cyl/head/sector
 * an imd file has the following format:
 * a text header, up to a 0x1a (control-z)
 * repeated for each track:
 * mode 	1 byte
 * cylinder	1 byte
 * head		1 byte
 * numsecs	1 byte
 * secsize	1 byte in shifts of 128
 * secmap	numsecs of sector numbers
 * cylmap	numsecs of sector numbers - never seen this
 * headmap	numsecs of sector numbers - never seen this
 * a data code:  0: no data 1: secsize bytes 2: a fill byte
 *
 * writing is a lot more problematic, since we've already snarfed the data from
 * the imd device file.  so what happens is that we create a drive file that
 * contains tracks that overlay the data from the IMD file, and loading the IMD
 * initially first reads the baseline and then overlays the newly written data
 * that is tagged with the parent.  not entirely satisfactory.
 *
 * the format of the delta file is brute force and wasteful:
 * one byte for every potential sector on the disk
 * 80 * 32 = 2560 bytes,
 *  32 bytes being the dirty sector indication
 * 80 * 32 * 1024 bytes of the actual data, where each sector
 *  1024 being every potential sector.
 * so calculating the delta offset is trivial
 */

/* imd data codes */
#define IMD_ABSENT  0
#define IMD_DATA    1
#define IMD_FILL    2
#define IMD_EOC     0x1a

/* layout of the delta file */
#define MAXSECSIZE  1024
#define MAXTRACKS   80          // 0 - 79
#define MAXSECTORS  32          // 0 - 31

#define DELTA_SIZE      MAXTRACKS * MAXSECTORS
#define DIRTY_OFF(t,s)  (((t) * MAXSECTORS) + (s))
#define DELTA_OFF(t,s)  DELTA_SIZE + (MAXSECSIZE * DIRTY_OFF(t,s))
#define MAXDELTA        DELTA_OFF(MAXTRACKS, 0)

#define DELTA_NO    0           // use the original
#define DELTA_YES   1           // delta has newer

#include "sim.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef STAND
int verbose;
void stop()
{
}

#endif

/*
 * this is our handle for the read/write .imd file support
 */
struct imd {
    char *comment;              // what it is
    int tracks;                 // how many tracks
    struct imd_trk **track;     // array of tracks
    int delta_fd;               // file descriptor for writes
    char delta_map[DELTA_SIZE]; // the dirty map
};

/*
 * cp/m disks can have a bizarre format where the first track is single density
 * with one sector size, and the rest of the disk is another sector size and density
 * entirely.  so each track can have it's own format and size.  handle this.
 * this is strongly derived from dave dunfield's file format.  kudos to him for doing
 * the hard work of inventing this abstraction.
 *
 * however, in memory, we just scribble on the same place as the original data.
 * this means that we only read the delta once.
 */
struct imd_trk {
    struct {
        unsigned char mode;
        unsigned char cyl;
        unsigned char head;
        unsigned char nsec;
        unsigned char size;
    } fixed;
    int track;
    int secsize;
    char *secmap;
    char *cylmap;
    char *headmap;
    char **data;
};

static void
dump_secmap(char *label, char *m, int s)
{
    int i;

    printf("%s: ", label);
    if (!m) {
        printf("absent\n");
        return;
    }
    for (i = 0; i < s; i++) {
        printf("%d ", m[i]);
    }
    printf("\n");
}

static void
dump_track(struct imd_trk *tp)
{
    int s;

    printf("track %d\n", tp->track);
    printf("mode: %x\n", tp->fixed.mode);
    printf("cyl: %d\n", tp->fixed.cyl);
    printf("head: %x\n", tp->fixed.head);
    printf("nsec: %d\n", tp->fixed.nsec);
    printf("size: %x\n", tp->fixed.size);
    printf("secsize: %d\n", tp->secsize);
    dump_secmap("secmap", tp->secmap, tp->fixed.nsec);
    dump_secmap("cylmap", tp->cylmap, tp->fixed.nsec);
    dump_secmap("headmap", tp->headmap, tp->fixed.nsec);

    for (s = 0; s < tp->fixed.nsec; s++) {
        printf("sector: %d\n", s);
        if (tp->data[s]) {
            hexdump(tp->data[s], tp->secsize);
        } else {
            printf("absent\n");
        }
    }
}

/*
 * read the imd file for track data and build the data structure
 * which then can be interrogated for format information and data
 * assumes an open IMD file, and we've read the prior tracks.
 * stop or die silently if something unexpected happens.
 */
struct imd_trk *
get_track(int fd)
{
    struct imd_trk *tp;
    char c;
    int nsec;
    int s;
    int i;
    
    // make space for and read the track header
    tp = malloc(sizeof(*tp));
    if (read(fd, tp, sizeof(tp->fixed)) != sizeof(tp->fixed))
        return 0;

    tp->secsize = 0x80 << tp->fixed.size;

    // make space for and read the sector map
    nsec = tp->fixed.nsec;
    tp->secmap = malloc(nsec);
    if (read(fd, tp->secmap, nsec) != nsec)
        return 0;

    // if there's a cylinder map, make space and read it in
    if (tp->fixed.head & 0x80) {
        tp->cylmap = malloc(nsec);
        if (read(fd, tp->cylmap, nsec) != nsec)
            return 0;
    } else {
        tp->cylmap = 0;
    }

    // if there's a headmap, make space and read it in
    if (tp->fixed.head & 0x40) {
        tp->headmap = malloc(nsec);
        if (read(fd, tp->headmap, nsec) != nsec)
            return 0;
    } else {
        tp->headmap = 0;
    }

    // allocate space for nsec sector pointers to point at data
    tp->data = malloc(sizeof(char *) * nsec);

    /* read a sector byte type and build the sector */
    for (s = 0; s < nsec; s++) {

        if (read(fd, &c, 1) != 1)
            return 0;
        if (c) {
            tp->data[s] = malloc(tp->secsize);

            if (c == 2) {   // type 2 is a sector full of the next single byte
                if (read(fd, &c, 1) != 1) {
                    return 0;
                }
                memset(tp->data[s], c, tp->secsize);
            } else {        // anything else is a sector full of data
                if (read(fd, tp->data[s], tp->secsize) != tp->secsize)
                    return 0;
            }
        } else {            // type 0 is an absent sector
            tp->data[s] = 0;
        }
    }
    return tp;
}

/*
 * load an imd file and return the struct
 */
void *
load_imd(char *fname)
{
    struct imd *ip;
    char c = 0;
    int clen = 0;
    int fd;
    struct imd_trk *tp;
    int t;
    int offset;
    char delta[100];

    fd = open(fname, O_RDONLY);
    // what's the size of the comment?
    do {
        if (read(fd, &c, 1) != 1)
            return 0;
        clen++;
    } while (c != IMD_EOC);

    // make space for the imd header and copy the comment
    ip = malloc(sizeof(*ip));
    ip->comment = malloc(clen + 1);
    
    lseek(fd, 0, SEEK_SET);
    read(fd, ip->comment, clen);
    ip->comment[clen] = 0;
    ip->track = malloc(sizeof(struct imd_trk *) * MAXTRACKS);

    // read all the tracks in
    for (t = 0; t < MAXTRACKS; t++) {
        ip->track[t] = tp = get_track(fd);
        if (!tp) {
            break;
        } 
        tp->track = t;
    }    
    ip->tracks = t;
    ip->track = realloc(ip->track, sizeof(struct imd_trk *) * ip->tracks);
    close(fd);
    
    // now, create the delta file or read it - make sure it's full sized
    sprintf(delta, "%s-delta", fname);
    fd = open(delta, O_RDWR|O_CREAT, 0777);
    c = 0;
    lseek(fd, MAXDELTA, SEEK_SET);
    write(fd, &c, 1);
    fsync(fd);
    ip->delta_fd = fd;

    /* read the delta map */
    lseek(fd, 0, SEEK_SET);
    read(fd, &ip->delta_map, DELTA_SIZE);

    for (t = 0; t < MAXTRACKS; t++) {
        tp = ip->track[t];
        for (c = 0; c < MAXSECTORS; c++) {
            if (ip->delta_map[DIRTY_OFF(t,c)] == DELTA_YES) {
                offset = DELTA_OFF(t,c);
                if (verbose & V_BIO) printf("imd_load_delta trk %d side %d sec %d offset %d\n",
                    t, 0, c, offset);
                lseek(fd, offset, SEEK_SET);
                if (!tp->data[c]) {
                    tp->data[c] = malloc(tp->secsize);
                }
                read(fd, tp->data[c], tp->secsize);
            }
        }
    }
    return ip;
}

void
imd_trkinfo(void *vp, int trk, int *secs, int *secsize)
{
    struct imd *ip = (struct imd *)vp;
    struct imd_trk *tp = ip->track[trk];
    if (secs) *secs = tp->fixed.nsec;
    if (secsize) *secsize = tp->secsize;
}

/*
 * return the sector index, given the sector number.
 * the sector requested is usually an integer from 1 to nsec
 * and the index is always from 0 to nsec - 1.
 * but there could be skew involved; i've not seen this in a IMD.
 * XXX - handle side, plus there's another map or two..
 */
static
translate_sector(struct imd_trk *tp, int sec, int side)
{
    int mysec = -1;
    int i;

    if (side) {
        printf("side nonzero\n");
        verbose |= V_IMD;
        stop();
    }

    if (tp->secmap) {
        for (i = 0; i < tp->fixed.nsec; i++) {
            if (tp->secmap[i] == sec) {
                mysec = i;
            }
        }
    }
    if (mysec == -1) {
        printf("imd: translate sector not found %d\n", sec);
        if (verbose & V_IMD) {
            dump_track(tp);
        }
        return 0;
    }
    return mysec;
}

/*
 * find the file offset from the track structure in the imd
 * and update the in-memory sector, and then write the disk
 */
int
imd_write(void *vp, int drive, int trk, int side, int osec, char *buf)
{
    struct imd *ip = (struct imd *)vp;
    struct imd_trk *tp = ip->track[trk];
    char c;
    int offset;
    int tsec;       // translated sector

    tsec = translate_sector(tp, osec, side);

    /* could be an absent block */
    if (!tp->data[tsec]) {
        tp->data[tsec] = malloc(tp->secsize);
    }
    memcpy(tp->data[tsec], buf, tp->secsize); 
    lseek(ip->delta_fd, DIRTY_OFF(trk, tsec), SEEK_SET);
    c = DELTA_YES;
    write(ip->delta_fd, &c, 1);
    offset = DELTA_OFF(trk, tsec);
    if (verbose & V_BIO) printf("imd_write drive %d trk %d side %d osec %d tsec %d offset %d\n",
        drive, trk, side, osec, tsec, offset);
    lseek(ip->delta_fd, offset, SEEK_SET);
    write(ip->delta_fd, buf, tp->secsize);
    // if (verbose & V_BIO) hexdump(buf, tp->secsize);
    return (tp->secsize);
}

/*
 * copy the data from a sector in imd to a buffer
 * return the number of bytes
 */
int
imd_read(void *vp, int drive, int trk, int side, int osec, char *buf)
{
    struct imd *ip = (struct imd *)vp;
    struct imd_trk *tp = ip->track[trk];
    int tsec;
    int i;

    tsec = translate_sector(tp, osec, side);

    if (verbose & V_BIO) printf("imd_read drive %d trk %d side %d tsec %d osec %d\n",
        drive, trk, side, tsec, osec);
    
    memcpy(buf, tp->data[tsec], tp->secsize); 
    if (verbose & V_BIO) hexdump(buf, tp->secsize);
    return (tp->secsize);
}

#ifdef STAND
void
dump_imd(struct imd *imd, char *filename)
{
    int t;

    printf("comment: %s\n", imd->comment);
    printf("%d tracks\n", imd->tracks);

    for (t = 0; t < imd->tracks; t++) {
        dump_track(imd->track[t]);
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
    for (trk = 0; trk < imd->tracks; trk++) {
        tp = imd->track[trk];    
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
    exit(1);
}

int
main(int argc, char **argv)
{
    struct imd *ip;
    char *s;
    int dump = 0;
    int merge = 0;

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
        ip = (struct imd *)load_imd(*argv);
        if (!ip) {
            printf("can't load %s\n", *argv);
            exit(1);
        }
        if (!(dump || merge)) {
            printf("%s\n", ip->comment);
        }
        if (dump) dump_imd(ip, *argv);
        if (merge) merge_imd(ip, *argv);
        argv++;
    }
    exit(0);
}
#endif

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
