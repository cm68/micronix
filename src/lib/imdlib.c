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
#include "../include/kv.h"
#include "../include/util.h"
#include "../include/lockfile.h"

#ifndef NODEBUG
int trace_imd;
#endif

static void apply_fmt(struct imd_trk *tp, int nsec, int size, int mode,
    int firstsec);
static int imd_grow(struct imd *ip, int cyl, int head);
static struct imd_trk *imd_track(struct imd *ip, int cyl, int head);

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
get_track(int fd, int inplace)
{
    struct imd_trk *tp;
    char c;
    int nsec;
    int s;

    // make space for and read the track header
    tp = malloc(sizeof(*tp));
    {
        unsigned char hdr[5];

        if (read(fd, hdr, 5) != 5) {
bomb:
            free(tp);
            return 0;
        }
        tp->fixed.mode = hdr[0];
        tp->fixed.cyl = hdr[1]; /* the low byte; imd_load infers the high
                                 * byte from track order */
        tp->fixed.head = hdr[2];
        tp->fixed.nsec = hdr[3];
        tp->fixed.size = hdr[4];
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
    tp->data_off = malloc(sizeof(long) * nsec);

    /* read a sector byte type and build the sector */
    for (s = 0; s < nsec; s++) {

        if (read(fd, &c, 1) != 1) {
            goto bomb;
        }
        /*
         * An in-place image keeps its data on the disk, not in memory:
         * record where this sector's blob is and step over it.  Only a
         * fixed-size data run has a real offset; a fill or an absent
         * sector has no blob of its own, so it is not addressable.
         */
        if (inplace && c == IMD_DATA) {
            tp->data_off[s] = lseek(fd, 0, SEEK_CUR);
            tp->data[s] = 0;
            if (lseek(fd, tp->secsize, SEEK_CUR) < 0)
                goto bomb;
            continue;
        }
        tp->data_off[s] = -1;
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
    if (ip->delta)
        kv_close(ip->delta);
    if (ip->lock_fd >= 0)
        close(ip->lock_fd);
    if (ip->fd >= 0)
        close(ip->fd);
}

/*
 * load an imd file and return the struct
 */
/*
 * The write overlay's name: the image's own, with -delta on the end.  It
 * is a key store (kv.h), not a fixed layout - keyed by cylinder, head and
 * sector slot - so it holds only what was written: sector data, and no
 * geometry or framing of its own.
 */
#define DELTASUFFIX "-delta"

/*
 * A sector's data from the overlay: copy it into its slot.
 */
static void
apply_data_cb(void *key, int klen, void *val, int vlen, void *arg)
{
    struct imd *ip = (struct imd *)arg;
    unsigned char *k = key;
    struct imd_trk *tp;
    int slot;

    /*
     * The key is four bytes - cylinder low, cylinder high, head, sector
     * slot - because a cylinder is one byte on the wire but a drive of
     * more than 255 cylinders has a high byte the delta has to carry.
     * Slot 0xff is the never-written sentinel.
     */
    if (klen != 4 || k[3] == 0xff)
        return;
    tp = imd_track(ip, k[0] | (k[1] << 8), k[2]);
    if (!tp)
        return;
    slot = k[3];
    if (slot >= tp->fixed.nsec)
        return;
    if (!tp->data[slot])
        tp->data[slot] = malloc(tp->secsize);
    memcpy(tp->data[slot], val, vlen);
}

/*
 * Attach the write overlay: a key store beside the image, read-only for
 * a caller that will only read (merge), read/write and created for one
 * that writes (the simulator, the host tools).  It holds only sector
 * data; the framing is in the image, so the data applies straight onto
 * the tracks the image already laid out.
 */
static int
attach_delta(struct imd *ip, char *fname, int mode)
{
    char *namebuf;

    /*
     * A writable image is locked so two tools never write the same disk
     * at once.  The lock is held for as long as the image is open and
     * dropped by imd_close - or by the kernel when the holder exits, for
     * any reason - so a short-lived tool and a long-lived simulator
     * contend on it the same way.  In-place mode locks too: it writes the
     * image directly, and the same mutual exclusion applies.
     */
    ip->lock_fd = -1;
    if (imd_writable(mode)) {
        ip->lock_fd = acquire_lock(fname);
        if (ip->lock_fd < 0)
            return -1;
    }

    /* an in-place image has no overlay; its sectors are the file's own */
    if (imd_inplace(mode))
        return 0;

    namebuf = malloc(strlen(fname) + sizeof(DELTASUFFIX) + 1);
    sprintf(namebuf, "%s%s", fname, DELTASUFFIX);
    ip->delta = kv_open(namebuf, mode == IMD_DELTA);
    free(namebuf);

    if (ip->delta) {
        kv_foreach(ip->delta, apply_data_cb, ip);
    }
    return 0;
}

/*
 * The single density track an eight inch medium opens with: 26 sectors of
 * 128 bytes, numbered from one, which is what an eight inch drive is
 * formatted with at cylinder 0 head 0 and not a choice anyone makes.  The
 * boot is its first four - one 512 byte block of it - and every cylinder
 * above it is double density with the sector size the rest of this file
 * describes.  See stand/roms/l1boot.s's djfm, which reads it as four
 * sectors because that is the only way it can be read.
 */
#define FM_NSEC     26
#define FM_SSIZE    128

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
    int fm;                     // and cylinder 0 head 0 is an fm track
    char *what;
} rawfmts[] = {
    /*
     * Ten sectors on a five inch disk is the hard sectored format - see
     * the firmware tables in raw_geometry below - so these count from
     * zero, which is what a name of bdev(2,N) already said and what this
     * table has to say too.  It said one, which put every sector of such
     * an image one place out: the loader's own label check catches that
     * now, and did not before there was one.
     */
    { 40*2*10*512, 40, 2, 10, 512, 0, 0, 0, "5 1/4 inch, 40 track, double sided, 512 byte sectors" },
    { 40*1*10*512, 40, 1, 10, 512, 0, 0, 0, "5 1/4 inch, 40 track, single sided, 512 byte sectors" },
    { 35*2*10*512, 35, 2, 10, 512, 0, 0, 0, "5 1/4 inch, 35 track, double sided, 512 byte sectors" },
    { 35*1*10*512, 35, 1, 10, 512, 0, 0, 0, "5 1/4 inch, 35 track, single sided, 512 byte sectors" },
    { 80*2*10*512, 80, 2, 10, 512, 0, 0, 0, "5 1/4 inch, 80 track, double sided, 512 byte sectors" },
    { 77*2*8*512,  77, 2,  8, 512, 0, 1, 1, "8 inch, double sided, 512 byte sectors" },
    { 77*1*26*128, 77, 1, 26, 128, 0, 1, 1, "8 inch, single sided, 128 byte sectors" },

    /*
     * The two eight inch media this tree builds bootable diskettes for -
     * see stand/boot/GNUmakefile's FLOPPIES and cmd/mkfs/mkfs.h.  They
     * have to be here rather than left to the name, because the name
     * cannot say them: a bdev minor encodes five inch against eight and
     * a sector size, and then derives the heads from which of the two it
     * was (raw_geometry below), so there is no minor for an eight inch
     * drive with two heads and no way to write one down.  The size is
     * unambiguous, so the size is what says it.
     *
     * firstsec is one: eight inch soft sectored media number from one,
     * per the firmware tables quoted in raw_geometry.
     */
    { 77*1*15*512, 77, 1, 15, 512, 0, 1, 1, "8 inch, single sided, 512 byte sectors" },
    { 77*2*15*512, 77, 2, 15, 512, 0, 1, 1, "8 inch, double sided, 512 byte sectors" },

    /*
     * The DOS double-density diskettes dar exchanges with: the five and
     * a quarter set (8 or 9 sectors, single or double sided) and the
     * 3 1/2 inch 720K.  A raw .img of one of these is a linear dump -
     * sectors in order, no skew, numbered from one - which is how dar
     * reads them with its device's alternate-sectoring bit clear.
     */
    { 40*1*8*512,  40, 1,  8, 512, 0, 1, 0, "5 1/4 inch, 40 track, single sided, 8 sector, 512 byte sectors" },
    { 40*1*9*512,  40, 1,  9, 512, 0, 1, 0, "5 1/4 inch, 40 track, single sided, 9 sector, 512 byte sectors" },
    { 40*2*8*512,  40, 2,  8, 512, 0, 1, 0, "5 1/4 inch, 40 track, double sided, 8 sector, 512 byte sectors" },
    { 40*2*9*512,  40, 2,  9, 512, 0, 1, 0, "5 1/4 inch, 40 track, double sided, 9 sector, 512 byte sectors" },
    { 80*2*9*512,  80, 2,  9, 512, 0, 1, 0, "3 1/2 inch, 80 track, double sided, 9 sector, 512 byte sectors" },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0 }
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
     * An eight inch medium opens with a single density track - see
     * FM_NSEC - and that is what the bit that is clear here means: a five
     * and a quarter inch drive has one sector size all the way down.
     */
    fp->fm = (minor & 0x04) ? 0 : 1;

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
     * is the only skew a raw image is ever read with, but the interleave
     * is not this map's business.  The driver maps a filesystem block to
     * a sector number and then asks for that sector; the medium only has
     * to find it.  Applying the skew here as well maps every block twice
     * and lands somewhere unrelated.
     *
     * Which order the data itself is in is the caller's business, and the
     * two callers differ: mnix bootflop writes a raw in block order -
     * unskewed, the order this map describes - and merge_imd in tools/imd.c
     * writes an IMD in the alternated order a shipped diskette has.
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
raw_load(char *fname, int drive, int mode)
{
    struct rawfmt *fp;
    struct rawfmt named;
    struct imd *ip;
    struct imd_trk *tp;
    struct stat sb;
    int fd, cyl, head, sec;
    int sbyte, sz;
    int fm, nsec, ssize, fsec;

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

    /*
     * How a raw image got interpreted is worth saying, since the name is
     * the only thing that says it - but on stderr, not stdout.  A caller
     * writing sectors to a pipe gets this in the middle of the data
     * otherwise, which is a hard thing to notice and a confusing thing to
     * debug.  The simulator sends its stdout to the logfile and leaves
     * stderr on the monitor, so it still sees this either way.
     */
    fprintf(stderr,
        "%s: raw image, %s, %d cyl, %d head, %d x %d byte sectors%s, "
        "sectors numbered from %d%s\n",
        fname, fp->what, fp->cyls, fp->heads, fp->spt, fp->secsize,
        fp->skew ? ", skewed" : "", fp->firstsec,
        fp->fm ? ", fm track 0" : "");

    ip = malloc(sizeof(*ip));
    memset(ip, 0, sizeof(*ip));
    ip->fd = -1;                    /* a raw image has no track records */

    /*
     * The comment is the file's header, and it is the same field
     * imd_load fills from a real IMD: everything through the EOC, which
     * is what imd -s strips and what merge_imd writes back out ahead of
     * a fresh one.  A raw image has no header of its own, so one is made
     * here, magic first - a file that does not begin "IMD " is not one
     * this library will read back, and a merge of a raw image without it
     * produced a file nothing could open.
     */
    {
        char hdr[128];

        snprintf(hdr, sizeof(hdr), "IMD %s%c", fp->what, IMD_EOC);
        ip->comment = strdup(hdr);
    }
    ip->drive = drive;
    ip->mode = mode;
    ip->cyls = fp->cyls;
    ip->heads = fp->heads;
    ip->spt = SECTORS;
    ip->tracks = calloc(fp->cyls * MAXHEADS, sizeof(struct imd_trk *));

    for (cyl = 0; cyl < fp->cyls; cyl++) {
        for (head = 0; head < fp->heads; head++) {
            /*
             * The one track on the medium whose shape is not the rest of
             * it: an eight inch drive's cylinder 0 head 0 is single
             * density, 26 sectors of 128 bytes.  It is the first 3328
             * bytes of the image, and it is also the track the boot is
             * on, so a reader that asks it for four 128 byte sectors -
             * which is the only way to read a block there, and what
             * l1boot.s does - gets the boot's 512 bytes.
             *
             * The first sector is one here whatever the medium says
             * elsewhere, because that is what the track is: the four the
             * boot is in are sectors 1 to 4 (djfm starts at one).
             */
            fm = fp->fm && cyl == 0 && head == 0;
            nsec = fm ? FM_NSEC : fp->spt;
            ssize = fm ? FM_SSIZE : fp->secsize;
            fsec = fm ? 1 : fp->firstsec;

            tp = malloc(sizeof(*tp));
            memset(tp, 0, sizeof(*tp));
            tp->fixed.cyl = cyl;
            tp->fixed.head = head;
            tp->fixed.nsec = nsec;
            tp->head = head;
            tp->secsize = ssize;

            /*
             * The size byte in a track header is the exponent of the
             * sector size, not the size: a reader takes it as
             * 0x80 << size.  get_track fills the pair from one byte and
             * merge_imd writes the header back out verbatim, so filling
             * only secsize here merged to a file whose sectors read back
             * at a quarter of their length - the first track parsed and
             * the rest of the medium was garbage.
             */
            for (sz = 128, sbyte = 0; sz < ssize; sz <<= 1)
                sbyte++;
            tp->fixed.size = sbyte;
            tp->secmap = malloc(nsec);
            raw_secmap(tp->secmap, nsec, fp->skew, fsec);
            tp->data = malloc(nsec * sizeof(char *));
            for (sec = 0; sec < nsec; sec++) {
                /*
                 * Sectors are numbered from 1 and laid down in order: a
                 * raw image has no interleave of its own.  Skew is the
                 * driver's business, which djdma.4 calls alternate
                 * sectoring and gives its own minor device numbers.
                 */
                tp->data[sec] = malloc(ssize);
                if (read(fd, tp->data[sec], ssize) != ssize) {
                    printf("%s: short read at cyl %d head %d sector %d\n",
                        fname, cyl, head, sec + 1);
                    close(fd);
                    return 0;
                }
            }
            ip->tracks[imd_trknum(ip, cyl, head)] = tp;

            /*
             * And past the rest of that track's own place in the image,
             * which is the one thing about this that a reader has to be
             * told.  The image is the medium's blocks in order, so the
             * cylinder in front of the first double density one is
             * blocks * 512 bytes of file whatever shape the track on it
             * is: the boot's 3328 bytes are the front of that, and the
             * bytes between them and the end of the cylinder are the
             * fm track's room and nothing else's.  Skipping them keeps
             * every later track where the block number says it is -
             * which is where mkbootimg and bootflop put it, and where a
             * diskette written by a machine would have it.
             */
            if (fm) {
                if (lseek(fd, (long) fp->spt * fp->secsize -
                        (long) FM_NSEC * FM_SSIZE, SEEK_CUR) < 0) {
                    printf("%s: cannot skip past the fm track\n", fname);
                    close(fd);
                    return 0;
                }
            }
        }
    }
    close(fd);

    if (attach_delta(ip, fname, mode) < 0) {
        free(ip->comment);
        free(ip);
        return 0;
    }
    return ip;
}

/*
 * A blank disk: no tracks, no geometry, learned as it is formatted.  A
 * writable open creates the "IMD " header so the image exists to be
 * written; a read-only open has nothing to read and answers nothing.
 */
static void *
blank_load(char *fname, int drive, int mode)
{
    struct imd *ip;
    int fd;

    if (!imd_writable(mode))
        return 0;

    fd = open(fname, O_RDWR | O_CREAT | O_TRUNC, 0777);
    if (fd < 0)
        return 0;
    write(fd, "IMD ", 4);
    write(fd, "\x1a", 1);

    ip = calloc(1, sizeof(*ip));
    ip->fd = fd;                    /* open to append track records */
    ip->mode = mode;
    ip->comment = malloc(6);
    memcpy(ip->comment, "IMD ", 4);
    ip->comment[4] = IMD_EOC;
    ip->comment[5] = 0;
    ip->drive = drive;
    ip->spt = SECTORS;

    if (attach_delta(ip, fname, mode) < 0) {
        free(ip->comment);
        free(ip);
        return 0;
    }
    return ip;
}

void *
imd_load(char *fname, int drive, int mode)
{
    struct imd *ip;
    int clen = 0;
    int fd;
    struct imd_trk *tp;
    struct imd_trk **tmp;
    int ntrk, nalloc, i;
    int cylhi = 0, prevcyl = -1;
    int offset;
    int cyl;
    int head;
    int sec;
    int tid;
    char *namebuf;
    char c;

    fd = open(fname, imd_writable(mode) ? O_RDWR : O_RDONLY);
    if (fd < 0)
        return blank_load(fname, drive, mode);

    /*
     * IMD files announce themselves; anything else we try to take as a
     * flat sector image.  Dunfield's format opens its comment with the
     * program name, and every image in this tree begins "IMD ".
     */
    {
        char magic[4];

        if (read(fd, magic, 4) != 4) {
            close(fd);
            return blank_load(fname, drive, mode);
        }
        if (memcmp(magic, "IMD ", 4) != 0) {
            close(fd);
            return raw_load(fname, drive, mode);
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
    memset(ip, 0, sizeof(*ip));
    ip->fd = -1;
    ip->mode = mode;
    ip->comment = malloc(clen + 1);

    lseek(fd, 0, SEEK_SET);
    read(fd, ip->comment, clen);
    ip->comment[clen] = 0;
    ip->drive = drive;
    ip->cyls = 0;
    ip->heads = 0;

    /*
     * Read the tracks into a scratch list first: the tracks array is
     * indexed by cylinder times heads, and the head count is not known
     * until the last track is in, so it cannot be sized while reading.
     */
    tmp = malloc(16 * sizeof(struct imd_trk *));
    ntrk = 0;
    nalloc = 16;
    while ((tp = get_track(fd, imd_inplace(mode))) != 0) {
        if (ntrk == nalloc) {
            nalloc *= 2;
            tmp = realloc(tmp, nalloc * sizeof(struct imd_trk *));
        }
        /*
         * The on-disk cylinder is one byte, and a drive of more than 256
         * cylinders wraps it.  Tracks come off the image in cylinder
         * order, so the high byte is the number of times the low byte has
         * wrapped - a decrease means the run went past 255.
         */
        if (tp->fixed.cyl < prevcyl)
            cylhi++;
        prevcyl = tp->fixed.cyl;
        tp->fixed.cyl = cylhi * 256 + tp->fixed.cyl;
        tmp[ntrk++] = tp;
        if (tp->fixed.cyl > ip->cyls) ip->cyls = tp->fixed.cyl;
        if (tp->fixed.head > ip->heads) ip->heads = tp->fixed.head;
    }
    if (mode)
        ip->fd = fd;            /* open to append track records or write in place */
    else
        close(fd);

    if (ntrk) {
        ip->cyls++;
        ip->heads++;
    }
    ip->spt = SECTORS;
    if (ip->cyls) {
        ip->tracks = calloc(ip->cyls * MAXHEADS, sizeof(struct imd_trk *));
        for (i = 0; i < ntrk; i++) {
            tp = tmp[i];
            ip->tracks[imd_trknum(ip, tp->fixed.cyl, tp->head)] = tp;
        }
    }
    free(tmp);

    if (attach_delta(ip, fname, mode) < 0) {
        close(ip->fd);          /* the open above succeeded; do not leak it */
        free(ip->comment);
        free(ip->tracks);
        free(ip);
        return 0;
    }
    return ip;
}

/*
 * Whether an image is laid down in place: every sector is a fixed data
 * run, so each sits at a stable offset and a write is a pwrite.  A hard
 * disk is built this way; a floppy is compressed (fill/absent runs) and
 * written through a delta.  Peek at the first track - its sector type
 * bytes say which - without disturbing the caller's own open.  Returns
 * a boolean: 1 in-place, 0 delta.
 */
int
imd_is_inplace(char *fname)
{
    unsigned char magic[4];
    unsigned char hdr[5];
    unsigned char c;
    int fd, nsec, s;

    if ((fd = open(fname, O_RDONLY)) < 0)
        return 0;
    if (read(fd, magic, 4) != 4 || memcmp(magic, "IMD ", 4) != 0) {
        close(fd);
        return 0;               /* raw image: a floppy */
    }
    do {
        if (read(fd, &c, 1) != 1) {
            close(fd);
            return 0;
        }
    } while (c != IMD_EOC);

    if (read(fd, hdr, 5) != 5) {
        close(fd);
        return 0;
    }
    nsec = hdr[3];
    lseek(fd, nsec, SEEK_CUR);          /* secmap */
    if (hdr[2] & 0x80)
        lseek(fd, nsec, SEEK_CUR);      /* cylmap */
    if (hdr[2] & 0x40)
        lseek(fd, nsec, SEEK_CUR);      /* headmap */
    for (s = 0; s < nsec; s++) {
        if (read(fd, &c, 1) != 1) {
            close(fd);
            return 0;
        }
        if (c != IMD_DATA) {
            close(fd);
            return 0;
        }
        lseek(fd, 0x80 << hdr[4], SEEK_CUR);    /* the data run */
    }
    close(fd);
    return 1;
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
    struct imd_trk *tp;
    int i, low;

    if ((cyl >= ip->cyls) || (head >= ip->heads))
        return 1;
    tp = ip->tracks[imd_trknum(ip, cyl, head)];
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
    struct imd_trk *tp;

    if ((cyl >= ip->cyls) || (head >= ip->heads)) {
        if (secs) *secs = 0;
        if (secsize) *secsize = 0;
        return;
    }
    tp = ip->tracks[imd_trknum(ip, cyl, head)];
    if (!tp) {
        if (secs) *secs = 0;
        if (secsize) *secsize = 0;
    } else {
        if (secs) *secs = tp->fixed.nsec;
        if (secsize) *secsize = tp->secsize;
    }
}

/*
 * the track's FM/MFM density - the IMD "mode" byte, 0 for single and
 * 1 for double.  A format set it; a disk that has never been formatted
 * still carries whatever its original image said.
 */
int
imd_density(void *vp, int cyl, int head)
{
    struct imd *ip = (struct imd *)vp;
    struct imd_trk *tp;

    if ((cyl >= ip->cyls) || (head >= ip->heads))
        return 0;
    tp = ip->tracks[imd_trknum(ip, cyl, head)];
    if (!tp) return 0;
    return tp->fixed.mode;
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
#ifndef NODEBUG
        if (traceflags & trace_imd) {
            imd_dump_track(tp);
        }
#endif
        /*
         * Not zero: zero is a sector index, and the track's first
         * sector lives there.  Returning it would answer a request
         * for a sector this track does not have with the contents of
         * the one it does - the boot sector, usually - which reads as
         * success and hides the mistake.  That is how a loader asking
         * for sector 0 of a medium that numbers from 1 looked, on the
         * simulation, like a loader that had found what it wanted.
         * -1 is what the caller tests for; see imd_read and imd_write.
         */
        return -1;
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
    struct imd_trk *tp;
    unsigned char key[4];
    int tsec;       // translated sector

    if (ip->mode == IMD_INPLACE) {
        /* write the sector's own data run, in place, no overlay */
        if ((cyl >= ip->cyls) || (head >= ip->heads))
            return 0;
        tp = ip->tracks[imd_trknum(ip, cyl, head)];
        if (!tp)
            return 0;
        tsec = translate_sector(tp, osec, head);
        if (tsec < 0 || tp->data_off[tsec] < 0)
            return 0;           /* no such sector, or not a data run */
        if (pwrite(ip->fd, buf, tp->secsize, tp->data_off[tsec]) != tp->secsize)
            return 0;
        trace(trace_imd, "imd_write drive %d cyl %d head %d tsec %d osec %d\n",
            ip->drive, cyl, head, tsec, osec);
        return (tp->secsize);
    }

    if (!ip->delta) {
        return 0;
    }

    tp = imd_track(ip, cyl, head);
    if (!tp)
        return 0;
    tsec = translate_sector(tp, osec, head);
    if (tsec < 0) {
        return 0;               /* no such sector on this track */
    }

    /* could be an absent block */
    if (!tp->data[tsec]) {
        tp->data[tsec] = malloc(tp->secsize);
    }
    memcpy(tp->data[tsec], buf, tp->secsize);
    key[0] = cyl & 0xff;
    key[1] = (cyl >> 8) & 0xff;
    key[2] = head;
    key[3] = tsec;
    kv_put(ip->delta, key, 4, buf, tp->secsize);
    trace(trace_imd, "imd_write drive %d cyl %d head %d tsec %d osec %d\n",
        ip->drive, cyl, head, tsec, osec);
#ifndef NODEBUG
    if (traceflags & trace_imd) hexdump(buf, tp->secsize);
#endif
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
    int trk = imd_trknum(ip, cyl, head);
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

    if (tsec < 0) {
        return 0;               /* no such sector on this track */
    }

    if (imd_inplace(ip->mode)) {
        /* read the sector's own data run; an absent one supplies zeros */
        if (tp->data_off[tsec] >= 0) {
            if (pread(ip->fd, buf, tp->secsize, tp->data_off[tsec]) != tp->secsize)
                return 0;
        } else if (tp->data[tsec]) {
            memcpy(buf, tp->data[tsec], tp->secsize);
        } else {
            bzero(buf, tp->secsize);
        }
        return (tp->secsize);
    }

    // if reading an absent block, supply zeros
    if (!tp->data[tsec]) {
        tp->data[tsec] = malloc(tp->secsize);
        bzero(tp->data[tsec], tp->secsize);
    }
    memcpy(buf, tp->data[tsec], tp->secsize);
#ifndef NODEBUG
    if (traceflags & trace_imd) hexdump(buf, tp->secsize);
#endif
    return (tp->secsize);
}

/*
 * The medium's block 0: the 512 bytes whose second half is the label.
 *
 * The first track is not always 512 byte sectors.  An eight inch
 * diskette's cylinder 0 head 0 is single density - 26 sectors of 128
 * bytes - and block 0 is the first four of them.  A five inch medium's
 * sectors are 1024 bytes, and block 0 is the first half of one.  A hard
 * disk is plain 512 byte sectors, which maps straight through.
 *
 * Read or write the block whatever shape it has, and return 512.  A
 * write on a 1K medium reads the sector first and writes it back whole,
 * because the label only owns half of it.  Return 0 if the medium has no
 * such block.
 */
int
imd_block0(void *vp, char *buf, int write)
{
    struct imd *ip = (struct imd *)vp;
    int secs = 0, secsize = 0, first, n;
    char tmp[MAXSECSIZE];

    imd_trkinfo(vp, 0, 0, &secs, &secsize);
    if (!secs || !secsize)
        return 0;
    first = imd_firstsec(vp, 0, 0);

    if (secsize == 128) {
        for (n = 0; n < 4; n++) {
            if (write) {
                if (imd_write(vp, 0, 0, first + n, buf + n * 128) != 128)
                    return 0;
            } else if (imd_read(vp, 0, 0, first + n, buf + n * 128) != 128)
                return 0;
        }
        return 512;
    }
    if (secsize < 512)
        return 0;

    if (write) {
        if (secsize > 512) {
            if (imd_read(vp, 0, 0, first, tmp) != secsize)
                return 0;
            memcpy(tmp, buf, 512);
            if (imd_write(vp, 0, 0, first, tmp) != secsize)
                return 0;
        } else if (imd_write(vp, 0, 0, first, buf) != 512)
            return 0;
    } else {
        if (imd_read(vp, 0, 0, first, secsize > 512 ? tmp : buf) != secsize)
            return 0;
        if (secsize > 512)
            memcpy(buf, tmp, 512);
    }
    return 512;
}

/*
 * Rebuild one track's layout.  The caller owns the data afterwards:
 * imd_setfmt fills it with the format's fill byte, and the delta loader
 * leaves it empty for the delta's own sector data.  Shared so the two
 * never disagree on how a track is laid out.
 */
static void
apply_fmt(struct imd_trk *tp, int nsec, int size, int mode, int firstsec)
{
    int i;

    for (i = 0; i < tp->fixed.nsec; i++)
        if (tp->data[i])
            free(tp->data[i]);
    free(tp->data);
    free(tp->data_off);
    free(tp->secmap);

    tp->fixed.nsec = nsec;
    tp->fixed.size = size;
    tp->fixed.mode = mode;
    tp->secsize = 0x80 << size;
    tp->secmap = malloc(nsec);
    for (i = 0; i < nsec; i++)
        tp->secmap[i] = firstsec + i;
    tp->data = calloc(nsec, sizeof(char *));
    tp->data_off = 0;
}

/*
 * Grow the image to cover a track at cylinder cyl, head head.  The head
 * stride is fixed (imd.h), so a new head only widens the geometry; a new
 * cylinder extends the track array and the tracks already placed do not
 * move.  This is how a blank disk learns its shape as it is formatted.
 */
static int
imd_grow(struct imd *ip, int cyl, int head)
{
    int ncyl = cyl + 1;

    if (head + 1 > ip->heads)
        ip->heads = head + 1;
    if (ncyl > ip->cyls) {
        int old = ip->cyls;
        struct imd_trk **nt = realloc(ip->tracks,
            ncyl * MAXHEADS * sizeof(struct imd_trk *));

        if (!nt)
            return 0;
        memset(nt + old * MAXHEADS, 0,
            (ncyl - old) * MAXHEADS * sizeof(struct imd_trk *));
        ip->tracks = nt;
        ip->cyls = ncyl;
    }
    return 1;
}

/*
 * The track at cylinder cyl, head head, created if the image does not
 * have one yet.  The caller fills its layout; the caller's caller fills
 * its sectors.
 */
static struct imd_trk *
imd_track(struct imd *ip, int cyl, int head)
{
    struct imd_trk *tp;
    int trk = imd_trknum(ip, cyl, head);

    if (!imd_grow(ip, cyl, head))
        return 0;
    tp = ip->tracks[trk];
    if (!tp) {
        tp = malloc(sizeof(*tp));
        memset(tp, 0, sizeof(*tp));
        tp->fixed.cyl = cyl;
        tp->fixed.head = head;
        tp->head = head;
        ip->tracks[trk] = tp;
    }
    return tp;
}

/*
 * Write one track's record to the image: the fixed header and sector map
 * that say how the track is laid out, then a fill code for each sector.
 * The framing is out of band to the data, like the label, so it goes into
 * the image and the delta never sees it.
 */
static void
write_track(struct imd *ip, struct imd_trk *tp, int fill)
{
    unsigned char hdr[5];
    unsigned char fb = (unsigned char)fill;
    unsigned char code;
    int i;

    hdr[0] = tp->fixed.mode;
    hdr[1] = tp->fixed.cyl & 0xff;
    hdr[2] = tp->fixed.head;
    hdr[3] = tp->fixed.nsec;
    hdr[4] = tp->fixed.size;
    write(ip->fd, hdr, 5);
    write(ip->fd, tp->secmap, tp->fixed.nsec);

    if (ip->mode == IMD_INPLACE) {
        /*
         * Each sector is a fixed data run, so its bytes sit at a stable
         * offset.  Lay down the data codes and allocate a hole for each
         * sector's data - the file grows to its full size, the bytes stay
         * unwritten, and the offset is kept for pwrite.
         */
        code = IMD_DATA;
        for (i = 0; i < tp->fixed.nsec; i++) {
            write(ip->fd, &code, 1);
            tp->data_off[i] = lseek(ip->fd, 0, SEEK_CUR);
            lseek(ip->fd, tp->secsize, SEEK_CUR);
        }
        ftruncate(ip->fd, lseek(ip->fd, 0, SEEK_CUR));
    } else {
        code = IMD_FILL;
        for (i = 0; i < tp->fixed.nsec; i++) {
            write(ip->fd, &code, 1);
            write(ip->fd, &fb, 1);
        }
    }
}

int
imd_setfmt(void *vp, int cyl, int head, int firstsec, int secs, int secsize,
    int mode, int fill)
{
    struct imd *ip = (struct imd *)vp;
    struct imd_trk *tp;
    int i, sbyte;

    tp = imd_track(ip, cyl, head);
    if (!tp)
        return 0;

    for (sbyte = 0, i = 128; i < secsize; i <<= 1)
        sbyte++;

    apply_fmt(tp, secs, sbyte, mode, firstsec);

    if (ip->mode == IMD_INPLACE) {
        /* the offset table; the data itself lives in the file's holes */
        tp->data_off = malloc(sizeof(long) * secs);
        if (ip->fd >= 0)
            write_track(ip, tp, fill);
        return 1;
    }

    for (i = 0; i < secs; i++) {
        tp->data[i] = malloc(secsize);
        memset(tp->data[i], fill, secsize);
    }

    /*
     * Persist the framing to the image, out of band, the way the label
     * is.  A raw image has no track records of its own, so its fill
     * alone lands in the overlay as data.
     */
    if (ip->fd >= 0) {
        write_track(ip, tp, fill);
    } else if (ip->delta) {
        for (i = 0; i < secs; i++)
            imd_write(ip, cyl, head, firstsec + i, tp->data[i]);
    }
    return 1;
}

/*
 * this grammar makes the compiler call this function before main()
 * this means we can add drivers by just adding them to the link
 */
__attribute__((constructor))
void
register_imd_driver()
{
#ifndef NODEBUG
    trace_imd = register_trace("imd");
#endif
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
