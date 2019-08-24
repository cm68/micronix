/*
 * an imd file is just a header followed by a bunch of tracks.
 *
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
 * that is tagged with the parent. it's named <imd file name>-delta
 *
 * the format of this delta file is brute force and wasteful: all are 5M
 * one byte for every potential sector on the disk
 * 2 * 80 * 32 = 5120 bytes,
 *  32 bytes being the dirty sector indication
 *  2 * 80 * 32 * 1024 bytes of the actual data, where each sector
 *  1024 being every potential sector.
 * so calculating the delta offset is trivial
 *
 * building this file with -DSTAND will define a program that does some reporting
 * of imd file contents, and additionally can write a new imd file from the original
 * plus the delta
 */

/* imd data codes */
#define IMD_ABSENT  0
#define IMD_DATA    1
#define IMD_FILL    2
#define IMD_EOC     0x1a

/* layout of the delta file */
#define MAXSECSIZE  1024
#define HEADS       2                   // 0, 1
#define CYLINDERS   80                  // 0 - 79
#define TRACKS      CYLINDERS * HEADS
#define SECTORS     32                  // 0 - 31

#define trknum(c, h) (((c) * HEADS) + (h))

#define DELTA_SIZE      CYLINDERS * HEADS * SECTORS
#define DIRTY_OFF(t, h, s)  (((trknum(t, h) * SECTORS) + (s)))
#define DELTA_OFF(t, h, s)  (DELTA_SIZE + (MAXSECSIZE * DIRTY_OFF(t, h, s)))
#define MAXDELTA        DELTA_OFF(TRACKS, 0, 0)

#define DELTA_NO    0           // use the original
#define DELTA_YES   1           // delta has newer

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sim.h"

int create_delta = 1;
int trace_imd;

#ifdef STAND

int trace;
int trace_bio;

void stop()
{
}
#endif

/*
 * this is our handle for the read/write .imd file support
 */
struct imd {
    char *comment;                      // what it is
    int heads;                          // number of actual heads
    int cyls;                           // number of cylinders
    struct imd_trk *tracks[TRACKS];     // array of tracks - indexed by trknum(c, h)
    int delta_fd;                       // file descriptor for writes
    char delta_map[DELTA_SIZE];         // the dirty map
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
 * 
 * here's splitting a hair:  a track is a hunk of recorded data.  it has an address of (cyl, head)
 * it does not have an externally known 'track number' - that's for internal use
 */
struct imd_trk {
    struct {
        unsigned char mode;
        unsigned char cyl;
        unsigned char head;
        unsigned char nsec;
        unsigned char size;
    } fixed;
    int head;
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

    printf("cyl: %d\n", tp->fixed.cyl);
    printf("head: %x\n", tp->head);
    printf("mode: %x\n", tp->fixed.mode);
    printf("fhead: %x\n", tp->fixed.head);
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

    tp->head = tp->fixed.head & 0x3f;

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
imd_load(char *fname)
{
    struct imd *ip;
    int clen = 0;
    int fd;
    struct imd_trk *tp;
    int offset;
    int cyl;
    int head;
    int sec;
    int tid;
    char *namebuf;
    char c;

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

    // read all the tracks in and squirrel them away, accumulating cyl/head counts
    while ((tp = get_track(fd)) != 0) {
        ip->tracks[trknum(tp->fixed.cyl, tp->head)] = tp;
        if (tp->fixed.cyl > ip->cyls) ip->cyls = tp->fixed.cyl;
        if (tp->fixed.head > ip->heads) ip->heads = tp->fixed.head;
    }
    close(fd);

    ip->cyls++;
    ip->heads++;
    ip->delta_fd = 0;

    // now, create the delta file or read it - make sure it's full sized
#define DELTASUFFIX "-delta"
    namebuf = malloc(strlen(fname)+sizeof(DELTASUFFIX)+1);
    sprintf(namebuf, "%s%s", fname, DELTASUFFIX);
    if (create_delta) {
        fd = open(namebuf, O_RDWR|O_CREAT, 0777);
    } else {
        fd = open(namebuf, O_RDONLY, 0777);
    }
    if (fd < 0) {
        fd = 0;
    }
    if (fd && create_delta) {
        ftruncate(fd, MAXDELTA);
        fsync(fd);
    }
    ip->delta_fd = fd;

    if (fd) {
        // read the delta map
        lseek(fd, 0, SEEK_SET);
        read(fd, &ip->delta_map, DELTA_SIZE);

        // read the deltas
        for (cyl = 0; cyl < CYLINDERS; cyl++) {
            for (head = 0; head < HEADS; head++) {
                tp = ip->tracks[trknum(cyl, head)];
                for (sec = 0; sec < SECTORS; sec++) {
                    if (ip->delta_map[DIRTY_OFF(cyl, head, sec)] == DELTA_YES) {
                        offset = DELTA_OFF(cyl, head, sec);
                        if (trace & trace_bio) printf("imd_load_delta cyl %d head %d sec %d offset %d\n",
                            cyl, head, sec, offset);
                        lseek(fd, offset, SEEK_SET);
                        if (!tp->data[sec]) {
                            tp->data[sec] = malloc(tp->secsize);
                        }
                        read(fd, tp->data[sec], tp->secsize);
                    }
                }
            }
        }
    } else {
        memset(ip->delta_map, 0, DELTA_SIZE);
    }
    return ip;
}

void
imd_trkinfo(void *vp, int cyl, int head, int *secs, int *secsize)
{
    struct imd *ip = (struct imd *)vp;
    struct imd_trk *tp = ip->tracks[trknum(cyl, head)];
    if ((head > ip->heads - 1) || !tp) {
        if (secs) *secs = 0;
        if (secsize) *secsize = 0;
    } else {
        if (secs) *secs = tp->fixed.nsec;
        if (secsize) *secsize = tp->secsize;
    } 
}

/*
 * return the sector index, given the sector number.
 * the sector requested is usually an integer from 1 to nsec
 * and the index is always from 0 to nsec - 1.
 * but there could be skew involved; i've not seen this in a IMD.
 *
 * XXX - handle head, plus there's another map or two..
 * i've seen wierdness where head 1 has sectors 10, 11, 12,
 * maybe just subtract number of sectors per track?!
 */
static
translate_sector(struct imd_trk *tp, int sec, int head)
{
    int mysec = -1;
    int i;

#ifdef notdef
    if (head) {
        printf("head nonzero\n");
        trace |= trace_imd;
        stop();
    }
#endif
    if (tp->secmap) {
        for (i = 0; i < tp->fixed.nsec; i++) {
            if (tp->secmap[i] == sec) {
                mysec = i;
            }
        }
    }
    if (mysec == -1) {
        printf("imd: translate sector not found %d\n", sec);
        if (trace & trace_imd) {
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
imd_write(void *vp, int drive, int cyl, int head, int osec, char *buf)
{
    struct imd *ip = (struct imd *)vp;
    int trk = trknum(cyl, head);
    struct imd_trk *tp;
    char c;
    int offset;
    int tsec;       // translated sector

    if ((cyl >= ip->cyls) || (head >= ip->heads)) {
        printf("imd: bogus write %d:%d with %d:%d\n", cyl, head, ip->cyls, ip->heads);
        return 0;
    }

    if (!ip->delta_fd) {
        return 0;
    }

    tp = ip->tracks[trk];
    tsec = translate_sector(tp, osec, head);

    /* could be an absent block */
    if (!tp->data[tsec]) {
        tp->data[tsec] = malloc(tp->secsize);
    }
    memcpy(tp->data[tsec], buf, tp->secsize); 
    lseek(ip->delta_fd, DIRTY_OFF(cyl, head, tsec), SEEK_SET);
    c = DELTA_YES;
    write(ip->delta_fd, &c, 1);
    offset = DELTA_OFF(cyl, head, tsec);
    if (trace & trace_bio) printf("imd_write drive %d cyl %d head %d tsec %d osec %d offset %d\n",
        drive, cyl, head, tsec, osec, offset);
    lseek(ip->delta_fd, offset, SEEK_SET);
    write(ip->delta_fd, buf, tp->secsize);
    if (trace & trace_bio) hexdump(buf, tp->secsize);
    return (tp->secsize);
}

/*
 * copy the data from a sector in imd to a buffer
 * return the number of bytes
 */
int
imd_read(void *vp, int drive, int cyl, int head, int osec, char *buf)
{
    struct imd *ip = (struct imd *)vp;
    int trk = trknum(cyl, head);
    struct imd_trk *tp;
    int tsec;

    if ((cyl >= ip->cyls) || (head >= ip->heads)) {
        printf("imd: bogus read %d:%d with %d:%d\n", cyl, head, ip->cyls, ip->heads);
        return 0;
    }
    tp = ip->tracks[trk];
    tsec = translate_sector(tp, osec, head);

    if (trace & trace_bio) printf("imd_read drive %d cyl %d head %d tsec %d osec %d\n",
        drive, cyl, head, tsec, osec);
    
    // if reading an absent block, supply zeros
    if (!tp->data[tsec]) {
        tp->data[tsec] = malloc(tp->secsize);
        bzero(tp->data[tsec], tp->secsize);
    }
    memcpy(buf, tp->data[tsec], tp->secsize); 
    if (trace & trace_bio) hexdump(buf, tp->secsize);
    return (tp->secsize);
}

#ifdef STAND

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
        dump_track(tp);
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

    create_delta = 0;
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
        ip = (struct imd *)imd_load(*argv);
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
#else
/*
 * this grammar makes the compiler call this function before main()
 * this means we can add drivers by just adding them to the link
 */
__attribute__((constructor))
void
register_imd_driver()
{
    trace_imd = register_trace("imd");
}
#endif

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
