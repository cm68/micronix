/*
 * access an imd file with delta support
 *
 * an imd file is just a header followed by a bunch of tracks.
 *
 * lib/imdlib.c
 * Changed: <2021-12-23 15:41:31 curt>
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "../include/imd.h"
#include "../include/util.h"

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
        if (tp->data[s]) {
            printf("sector: %d\n", s);
            fflush(stdout);
            hexdump(tp->data[s], tp->secsize);
        } else {
            printf("sector: %d absent\n", s);
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
    if (read(fd, tp, sizeof(tp->fixed)) != sizeof(tp->fixed)) {
bomb:
        free(tp);
        return 0;
    }

    tp->secsize = 0x80 << tp->fixed.size;

    // make space for and read the sector map
    nsec = tp->fixed.nsec;
    tp->secmap = malloc(nsec);
    if (read(fd, tp->secmap, nsec) != nsec) {
        goto bomb;
    }

    // if there's a cylinder map, make space and read it in
    if (tp->fixed.head & 0x80) {
        tp->cylmap = malloc(nsec);
        if (read(fd, tp->cylmap, nsec) != nsec) {
            goto bomb;
        }
    } else {
        tp->cylmap = 0;
    }

    // if there's a headmap, make space and read it in
    if (tp->fixed.head & 0x40) {
        tp->headmap = malloc(nsec);
        if (read(fd, tp->headmap, nsec) != nsec) {
            goto bomb;
        }
    } else {
        tp->headmap = 0;
    }

    tp->head = tp->fixed.head & 0x3f;

    // allocate space for nsec sector pointers to point at data
    tp->data = malloc(sizeof(char *) * nsec);

    /* read a sector byte type and build the sector */
    for (s = 0; s < nsec; s++) {

        if (read(fd, &c, 1) != 1) {
            goto bomb;
        }
        if (c) {
            tp->data[s] = malloc(tp->secsize);

            if (c == 2) {   // type 2 is a sector full of the next single byte
                if (read(fd, &c, 1) != 1) {
                    goto bomb;
                }
                memset(tp->data[s], c, tp->secsize);
            } else {        // anything else is a sector full of data
                if (read(fd, tp->data[s], tp->secsize) != tp->secsize) {
                    goto bomb;
                }
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
/*
 * Attach the write overlay.  Split out of imd_load so the raw loader can
 * use it too: a flat sector image gets the same delta file, with the same
 * name and layout, so writing to a raw disk behaves exactly as writing to
 * an IMD one and neither original image is ever touched.
 */
static void
attach_delta(struct imd *ip, char *fname, int create_delta)
{
    struct imd_trk *tp;
    char *namebuf;
    int fd;
    int cyl, head, sec, offset;

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
    free(namebuf);
}

/*
 * A raw sector image has no structure to read - just the sectors, in
 * order, cylinder by cylinder and head by head within each cylinder.  We
 * build the same track structures an IMD file would have produced, so
 * nothing downstream, the delta included, can tell the difference.
 *
 * The geometry has to come from somewhere and a flat file carries none,
 * so it is recognised by size.  djdma.4 lists the formats Micronix
 * supports; these are the ones whose image sizes are unambiguous.
 */
static struct rawfmt {
    long size;
    int cyls;
    int heads;
    int spt;
    int secsize;
    int skew;
    int firstsec;               // lowest numbered sector on a track
    char *what;
} rawfmts[] = {
    { 40*2*10*512, 40, 2, 10, 512, 0, 1, "5 1/4 inch, 40 track, double sided, 512 byte sectors" },
    { 40*1*10*512, 40, 1, 10, 512, 0, 1, "5 1/4 inch, 40 track, single sided, 512 byte sectors" },
    { 35*2*10*512, 35, 2, 10, 512, 0, 1, "5 1/4 inch, 35 track, double sided, 512 byte sectors" },
    { 35*1*10*512, 35, 1, 10, 512, 0, 1, "5 1/4 inch, 35 track, single sided, 512 byte sectors" },
    { 80*2*10*512, 80, 2, 10, 512, 0, 1, "5 1/4 inch, 80 track, double sided, 512 byte sectors" },
    { 77*2*8*512,  77, 2,  8, 512, 0, 1, "8 inch, double sided, 512 byte sectors" },
    { 77*1*26*128, 77, 1, 26, 128, 0, 1, "8 inch, single sided, 128 byte sectors" },
    { 0, 0, 0, 0, 0, 0, 0, 0 }
};

/*
 * Where a raw image's geometry comes from.
 *
 * A flat file carries none, but the archived Micronix floppies say it in
 * their name: they are symlinks to bdev(2,N), and N is the Micronix minor
 * device number for the drive they came off.  devnum reads that, and
 * fslib.c has long decoded the same bits to read these images from the
 * host side - djdma.4's encoding with one addition:
 *
 *      bit 2   5 1/4 inch rather than 8 inch
 *      bit 3   alternate sectoring, ie the sectors are skewed
 *      bit 4   1024 byte sectors rather than 512
 *
 * A plain file with no such link falls back to recognising the size.
 */
static int
raw_geometry(char *fname, struct rawfmt *fp, long size)
{
    char dt;
    int major, minor;

    if (devnum(fname, &dt, &major, &minor) != 0 || major != 2) {
        return 0;
    }

    /*
     * The 1024 byte bit picks how many sectors are on a track, but it does
     * not change the unit these images are addressed in.  fslib reads them
     * with readblk, which seeks to 512 * block and skews block numbers with
     * this same spt - so a track here is ten 512 byte blocks, whatever the
     * drive's own sectors were.  Describing it any other way puts every
     * block after the first in the wrong place, and it has to agree with
     * fslib or the simulator and mnix will read the same disk differently.
     */
    if (minor & 0x04) {
        fp->spt = (minor & 0x10) ? 10 : 9;
        fp->what = "5 1/4 inch";
    } else {
        fp->spt = (minor & 0x10) ? 16 : 15;
        fp->what = "8 inch";
    }
    fp->secsize = 512;

    /*
     * Where sector numbering starts, from the DJDMA firmware's own
     * tables (DJ49.MAC, "to determine the lowest numbered sector"):
     *
     *      8" soft sectored    26, 26, 15, 8 sectors   all from 1
     *      5" soft sectored     5 sectors               from 1
     *      5" hard sectored    10 sectors               from 0
     *
     * Ten sectors on a five inch disk is the hard sectored North Star
     * format, and it counts from zero.  Numbering it from one puts every
     * sector one place out and invents a sector ten that is not there.
     */
    fp->firstsec = ((minor & 0x04) && fp->spt == 10) ? 0 : 1;
    fp->skew = (minor & 0x08) ? 1 : 0;

    /*
     * fslib treats these as a linear run of tracks - blkno/spt is the
     * track number - and never has to say which side that track is on.
     * A controller does: it is told a cylinder and a head, and both are
     * on the wire.  Calling every track its own cylinder makes the two
     * agree about the first track and about nothing after it.
     *
     * The loader settles it.  This disk's filesystem starts at block 40
     * (its superblock says 760 blocks of the 800 that are here, so forty
     * are reserved), and the loader looks for it at cylinder 2 - which is
     * block 40 only if a cylinder is two tracks.  Five inch drives are
     * double sided, so a five inch image is heads-per-cylinder 2 and half
     * as many cylinders.
     */
    fp->heads = (minor & 0x04) ? 2 : 1;
    fp->cyls = size / (fp->spt * fp->secsize * fp->heads);
    if (fp->cyls == 0 ||
        (long)fp->cyls * fp->heads * fp->spt * fp->secsize != size) {
        printf("%s: bdev(2,%d) says %d byte sectors, %d per track, "
            "which does not divide %ld bytes\n",
            fname, minor, fp->secsize, fp->spt, size);
        return 0;
    }
    fp->size = size;
    return 1;
}

/*
 * Lay out the sector identifiers for one track.
 *
 * With no skew the sectors sit in the order they are numbered.  With it,
 * the disk was written two apart, and fslib's secmap() gives the rule:
 * logical sector n was recorded at position 2n mod spt, with an extra
 * step once past the end of an even numbered track so the second pass
 * falls between the first rather than back on top of it.
 *
 * secmap here runs the other way round - it says which sector is at each
 * position, because that is what translate_sector looks up - so this
 * inverts that mapping.  Sector numbers start at 1.
 */
static void
raw_secmap(char *secmap, int spt, int skew, int firstsec)
{
    int i;

    /*
     * A raw image holds its sectors in sector number order: the nth
     * sector of a track is the nth 512 bytes of that track in the file.
     *
     * It is tempting to bake fslib's interleave into this map, since that
     * is the only skew anywhere in sight, but that skew belongs to the
     * filesystem and not to the medium.  The driver maps a filesystem
     * block to a sector number and then asks for that sector; the medium
     * only has to find it.  Applying the skew here as well maps every
     * block twice and lands somewhere unrelated.
     *
     * The second level boot is the proof.  It is loaded from the sector
     * numbered one, and at that sector's own place in the file it begins
     * "ld sp,0100 / call 0a67 / jp 0000" - a program's entry point.  One
     * skew further along the same bytes start in the middle of an
     * instruction.
     */
    for (i = 0; i < spt; i++) {
        secmap[i] = i + firstsec;
    }
}

static void *
raw_load(char *fname, int drive, int create_delta)
{
    struct rawfmt *fp;
    struct rawfmt named;
    struct imd *ip;
    struct imd_trk *tp;
    struct stat sb;
    int fd, cyl, head, sec;

    if (stat(fname, &sb) < 0)
        return 0;

    /*
     * the name knows best; the size table is the fallback
     */
    if (raw_geometry(fname, &named, sb.st_size)) {
        fp = &named;
    } else {
        for (fp = rawfmts; fp->size; fp++) {
            if (fp->size == sb.st_size)
                break;
        }
    }
    if (!fp->size) {
        printf("%s: %ld bytes is not a raw disk image size I know\n",
            fname, (long)sb.st_size);
        return 0;
    }
    if ((fd = open(fname, O_RDONLY)) < 0)
        return 0;

    printf("%s: raw image, %s, %d cyl, %d head, %d x %d byte sectors%s\n",
        fname, fp->what, fp->cyls, fp->heads, fp->spt, fp->secsize,
        fp->skew ? ", skewed" : "");
    printf("%s: sectors numbered from %d\n", fname, fp->firstsec);

    ip = malloc(sizeof(*ip));
    memset(ip, 0, sizeof(*ip));
    ip->comment = strdup(fp->what);
    ip->drive = drive;
    ip->cyls = fp->cyls;
    ip->heads = fp->heads;

    for (cyl = 0; cyl < fp->cyls; cyl++) {
        for (head = 0; head < fp->heads; head++) {
            tp = malloc(sizeof(*tp));
            memset(tp, 0, sizeof(*tp));
            tp->fixed.cyl = cyl;
            tp->fixed.head = head;
            tp->fixed.nsec = fp->spt;
            tp->head = head;
            tp->secsize = fp->secsize;
            tp->secmap = malloc(fp->spt);
            raw_secmap(tp->secmap, fp->spt, fp->skew, fp->firstsec);
            tp->data = malloc(fp->spt * sizeof(char *));
            for (sec = 0; sec < fp->spt; sec++) {
                /*
                 * Sectors are numbered from 1 and laid down in order: a
                 * raw image has no interleave of its own.  Skew is the
                 * driver's business, which djdma.4 calls alternate
                 * sectoring and gives its own minor device numbers.
                 */
                tp->data[sec] = malloc(fp->secsize);
                if (read(fd, tp->data[sec], fp->secsize) != fp->secsize) {
                    printf("%s: short read at cyl %d head %d sector %d\n",
                        fname, cyl, head, sec + 1);
                    close(fd);
                    return 0;
                }
            }
            ip->tracks[trknum(cyl, head)] = tp;
        }
    }
    close(fd);

    attach_delta(ip, fname, create_delta);
    return ip;
}

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
    if (fd < 0)
        return 0;

    /*
     * IMD files announce themselves; anything else we try to take as a
     * flat sector image.  Dunfield's format opens its comment with the
     * program name, and every image in this tree begins "IMD ".
     */
    {
        char magic[4];

        if (read(fd, magic, 4) != 4) {
            close(fd);
            return 0;
        }
        if (memcmp(magic, "IMD ", 4) != 0) {
            close(fd);
            return raw_load(fname, drive, create_delta);
        }
        lseek(fd, 0, SEEK_SET);
    }

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
    ip->cyls = 0;
    ip->heads = 0;

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

    attach_delta(ip, fname, create_delta);
    return ip;
}

/*
 * The lowest numbered sector on a track.  Sector numbering does not
 * always start at one - see the firmware tables quoted in raw_load - and
 * a caller walking a whole track has to start where the track starts.
 */
int
imd_firstsec(void *vp, int cyl, int head)
{
    struct imd *ip = (struct imd *)vp;
    struct imd_trk *tp = ip->tracks[trknum(cyl, head)];
    int i, low;

    if (!tp || !tp->secmap || !tp->fixed.nsec) {
        return 1;
    }
    low = tp->secmap[0];
    for (i = 1; i < tp->fixed.nsec; i++) {
        if (tp->secmap[i] < low) {
            low = tp->secmap[i];
        }
    }
    return low;
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
