/*
 * imdlib.c
 *
 * an imd file is just a header followed by a bunch of tracks.
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#include "imd.h"
#include "util.h"

int trace_imd;

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

void
imd_dump_track(struct imd_trk *tp)
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
            fflush(stdout);
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
static struct imd_trk *
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

void
imd_close(void *vp)
{
    struct imd *ip = (struct imd *)vp;
    if (!ip)
        return;
    if (ip->delta_fd) {
        close(ip->delta_fd);
    }
}

/*
 * load an imd file and return the struct
 */
void *
imd_load(char *fname, int drive, int create_delta)
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
    ip->drive = drive;

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
                        trace(trace_imd, "imd_load_delta cyl %d head %d sec %d offset %d\n",
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
static int
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
        if (traceflags & trace_imd) {
            imd_dump_track(tp);
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
imd_write(void *vp, int cyl, int head, int osec, char *buf)
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
    trace(trace_imd, "imd_write drive %d cyl %d head %d tsec %d osec %d offset %d\n",
        ip->drive, cyl, head, tsec, osec, offset);
    lseek(ip->delta_fd, offset, SEEK_SET);
    write(ip->delta_fd, buf, tp->secsize);
    if (traceflags & trace_imd) hexdump(buf, tp->secsize);
    return (tp->secsize);
}

/*
 * copy the data from a sector in imd to a buffer
 * return the number of bytes
 */
int
imd_read(void *vp, int cyl, int head, int osec, char *buf)
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

    trace(trace_imd, "imd_read drive %d cyl %d head %d tsec %d osec %d\n",
        ip->drive, cyl, head, tsec, osec);
    
    // if reading an absent block, supply zeros
    if (!tp->data[tsec]) {
        tp->data[tsec] = malloc(tp->secsize);
        bzero(tp->data[tsec], tp->secsize);
    }
    memcpy(buf, tp->data[tsec], tp->secsize); 
    if (traceflags & trace_imd) hexdump(buf, tp->secsize);
    return (tp->secsize);
}

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

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
