/*
 * mkbootimg - the boot image for one kind of drive
 *
 * micronix/stand/boot/mkbootimg.c
 *
 * A boot image is the first level, which the rom reads to 0100 and
 * jumps to, and the second level behind it, which the first level
 * loads.  The first level is a sector and the second begins at the
 * next one.
 *
 * The second half of that first sector is not the first level's.  It
 * carries a struct dlabel - the geometry, where the boot area is, and
 * the rotation sys/mw.c applies - which is how a reader can map a block
 * without inverting the arithmetic that put the area where it is.  See
 * sys/dlabel.h, and cmd/mkfs, which writes the same structure when it
 * installs a boot into a filesystem it is making.
 *
 * So the geometry belongs in the image, and an image is per drive: this
 * is built once for each entry in the drive table and the Makefile says
 * which.  Everything that knows a drive's shape then agrees by
 * construction rather than by being told twice.
 *
 * This is a host program.  It includes the target's dlabel.h so that
 * the structure it writes cannot drift from the structure mkfs writes
 * and the loader will one day read - the fields are 16 bit and this
 * writes them a byte at a time, little endian, so a host with wider
 * ints or another byte order still produces a Z80 image.
 *
 *	mkbootimg <level1> <level2> <tracks> <heads> <spt> <out>
 *	mkbootimg -d <level01> <level2> <tracks> <heads> <spt> <firstsec>
 *	    <toff> <out>
 *
 * -d is the diskette layout, and it differs in one sector.  On this
 * controller the firmware copies only 128 bytes to 0080, so the first
 * level is two things in one file: a 128 byte level 0 that chain-loads
 * the block behind it, and then the level 1 that block holds.  So the
 * first sector has level 0 in its front half, this label in its second,
 * and level 1 in the whole of the next one.  The hard disk's first level
 * is one sector that fits in the first half, which is why it needs no
 * split and takes no -d.
 *
 * firstsec is where the medium numbers its sectors from, and it is
 * written into level 0 as well as into the label, because level 0 has to
 * ask for a sector before it has read the label.  It is one on the soft
 * sectored eight inch media and zero on the hard sectored five inch one.
 * See sys/dlabel.h, sys/dj.c's ORG1 and djboot1.s's FSEC.
 *
 * -d also picks the label idiom.  A diskette is not rolled - dj.c adds a
 * fixed track offset and never rotates - so the label carries a slice
 * table instead: 'a' begins at cylinder toff and the boot owns the
 * sectors in front of it.  See sys/dlabel.h and cmd/mkfs/mkfsfunc.c,
 * which writes the same two shapes.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

/*
 * The target's header, by path rather than by -I: this is a host
 * program and its own stdio has to win.  These two are the target's
 * spellings of a 16 and an 8 bit unsigned, which is all dlabel.h needs
 * of types.h.
 */
typedef unsigned short UINT;
typedef unsigned char UINT8;
#include "../../include/sys/dlabel.h"

#define BSIZE   512
#define HEADLEN 128             /* what the DJ-DMA firmware copies, and so
                                 * how much of the first level file is
                                 * level 0 rather than level 1 */
/*
 * djboot1.s's FSECMAG, and it has to be kept equal to it: the byte it
 * marks, at the end of level 0's 128, is written here rather than
 * assembled, because where a medium numbers its sectors from is not
 * something level 0 can work out and not something this tool can either
 * without being told.  -d tells it.
 */
#define FSECMAG 0xa5

/*
 * Where a hard sectored five inch medium says which format it is.  The
 * 128 bytes the firmware copies are level 0's whole world, and its code
 * does not reach this byte, so it is the one place a diskette's sector 0
 * can carry the answer.  djopen() reads sector 0 and looks this byte up
 * in its table of formats (sys/dj.c: S->config = kp[0x5c]).
 */
#define DJCONFIG 0x5c

static unsigned char sector[BSIZE];

/*
 * A 16 bit field, little endian, wherever the host would have put it.
 */
static void
put16(int off, unsigned val)
{
    sector[off] = val & 0xff;
    sector[off + 1] = (val >> 8) & 0xff;
}

static void
die(char *s)
{
    fprintf(stderr, "mkbootimg: %s\n", s);
    exit(1);
}

/*
 * The format bytes the driver has a table row for: 35, 40 and 80 cylinder
 * five inch media, single and double sided, and the 256 byte 35 cylinder
 * one (sys/dj.c, specs[]).  A byte that is not one of these is worse than
 * no byte at all - the lookup fails and the driver quietly keeps its
 * default row - so it is refused here instead.
 */
static unsigned char djformats[] = { 0x90, 0xb0, 0xf0, 0xa0, 0xc0, 0xd0, 0xe0, 0x10 };

static int
knownformat(unsigned c)
{
    int i;

    for (i = 0; i < sizeof djformats; i++)
        if (djformats[i] == c)
            return 1;
    return 0;
}

int
main(int argc, char **argv)
{
    FILE *in;
    FILE *out;
    char *pname;
    char *l1name;
    char *l2name;
    char *outname;
    int n;
    int off;
    int dflag;
    unsigned tracks, heads, spt, firstsec, toff, config, spc, roll, cyl0, bootblks;
    int c;

    pname = argv[0];
    dflag = 0;
    if (argc > 1 && strcmp(argv[1], "-d") == 0) {
        dflag = 1;
        argv++;
        argc--;
    }
    if (dflag ? argc != 10 : argc != 7) {
        fprintf(stderr,
            "usage: %s [-d] <level1> <level2> <tracks> <heads> <spt>%s<out>\n",
            pname, dflag ? " <firstsec> <toff> <config>" : "");
        exit(1);
    }
    l1name = argv[1];
    l2name = argv[2];
    tracks = strtoul(argv[3], 0, 0);
    heads = strtoul(argv[4], 0, 0);
    spt = strtoul(argv[5], 0, 0);
    firstsec = dflag ? strtoul(argv[6], 0, 0) : 0;
    toff = dflag ? strtoul(argv[7], 0, 0) : 0;
    config = dflag ? strtoul(argv[8], 0, 0) : 0;
    outname = argv[dflag ? 9 : 6];
    if (!tracks || !heads || !spt) {
        die("a geometry of zero");
    }
    /*
     * Zero or one, and nothing else: the firmware's own tables have one
     * numbering for hard sectored media and one for soft, and a track
     * that began at sector five would be a format nobody has.
     */
    if (firstsec > 1) {
        die("a first sector that is neither zero nor one");
    }
    if (config && !knownformat(config)) {
        die("a format byte the driver has no row for");
    }
    /*
     * Only a medium that carries one may name a format: the byte is the
     * hard sectored convention, and the media numbered from one are the
     * soft sectored ones the driver never looks the byte up for.
     */
    if (config && firstsec != 0) {
        die("a format byte on a medium that does not number from zero");
    }

    /*
     * Where the boot area is, and which idiom says so.
     *
     * A hard disk is rolled: mw.c rotates the mapping, so physical
     * cylinder 0 is not block 0 but the block whose cylinder index is
     * tracks - tracks/2, and the boot is a file inside the filesystem.
     * Computed the way mkfs computes it, and for the same reason.  A
     * diskette is not: dj.c adds toff and never rotates, so the boot is
     * simply the blocks in front of cylinder toff and no filesystem
     * block reaches them.
     */
    spc = heads * spt;
    if (dflag) {
        roll = 0;
        cyl0 = 0;
        bootblks = toff * spc;
    } else {
        roll = tracks >> 1;
        cyl0 = (tracks - roll) * spc;
        bootblks = spc;
    }

    /*
     * The first level, into the front of the sector.  On a diskette that
     * is only its first HEADLEN bytes and the read stops there on
     * purpose: the rest of the file is level 1, which belongs in the next
     * block, and the file is left positioned to be read into it below.
     */
    if (!(in = fopen(l1name, "rb"))) {
        die("cannot read the first level");
    }
    n = 0;
    while ((c = getc(in)) != EOF) {
        if (dflag ? n >= HEADLEN : n >= DL_OFFSET) {
            if (!dflag) {
                die("the first level runs into the label");
            }
            ungetc(c, in);      /* it is level 1's first byte, not ours */
            break;
        }
        sector[n++] = c;
    }
    if (dflag && n < HEADLEN) {
        die("level 0 is shorter than the firmware copies");
    }

    /*
     * And the one byte of level 0 that is the medium's, not the file's:
     * where its sectors are numbered from.  Level 0 cannot know it - the
     * label that says so is at byte 256 of the sector the firmware copied
     * only the first HEADLEN of - so it reads it out of its own last two
     * bytes instead, and level 1 reads the same byte.  The marker after it
     * is djboot1.s's, and it is checked, not assumed: a level 0 that grew
     * past its 128 bytes would have moved this byte into its own
     * instructions, and an image that did so must not be written.
     */
    if (dflag) {
        if (sector[HEADLEN - 1] != FSECMAG) {
            die("level 0 has no room for the first sector number");
        }
        sector[HEADLEN - 2] = firstsec;

        /*
         * And the format byte, where the medium has one.  A hard sectored
         * five inch diskette names its format here or the driver cannot
         * tell which of its rows describes the medium: it keeps the
         * default one - 35 cylinders, one head, ten sectors per cylinder
         * - which reaches the first 330 filesystem blocks and no further.
         * The filesystem mounts and the boot looks healthy, and what
         * fails is every read past block 330, which is where the kernel's
         * own overlay modules are read from.  A level 0 that has grown
         * into the byte is refused, not written over.
         */
        if (config) {
            if (sector[DJCONFIG]) {
                die("level 0 has grown into the format byte");
            }
            sector[DJCONFIG] = config;
        }
    }

    /*
     * And the label, into the second half of it.  Each field is placed
     * by offsetof rather than by counting: the structure is shared with
     * mkfs and the loader, and a field added to it should move these
     * rather than silently shift them all by two.
     */
    off = DL_OFFSET;
    if (off + (int)sizeof(struct dlabel) > BSIZE) {
        die("the label does not fit the sector");
    }
    memcpy(&sector[off + offsetof(struct dlabel, d_magic)], DL_MAGIC, 4);
    put16(off + offsetof(struct dlabel, d_version),
        dflag ? DL_VERS_SLICE : DL_VERSION);
    put16(off + offsetof(struct dlabel, d_tracks), tracks);
    put16(off + offsetof(struct dlabel, d_heads), heads);
    put16(off + offsetof(struct dlabel, d_spt), spt);
    put16(off + offsetof(struct dlabel, d_cyl0), cyl0);
    put16(off + offsetof(struct dlabel, d_bootblks), bootblks);
    put16(off + offsetof(struct dlabel, d_roll), roll);
    /*
     * d_fsize, d_isize and d_swap describe a filesystem, and there is
     * not one yet.  mkfs fills them in when it makes one, rewriting the
     * whole label as it does.  They are zero here, and a reader that
     * cares has to tell the difference.
     */
    put16(off + offsetof(struct dlabel, d_fsize), 0);
    put16(off + offsetof(struct dlabel, d_isize), 0);
    put16(off + offsetof(struct dlabel, d_swap), 0);
    put16(off + offsetof(struct dlabel, d_bootslice), 0);
    /*
     * The one slice a diskette has: 'a', from cylinder toff to the end
     * of the medium - a length of zero.  Without -d, toff is zero and
     * this is the empty table a rolled disk carries, which reads as the
     * whole drive and is exactly what its d_roll already says.
     */
    put16(off + offsetof(struct dlabel, d_slice[0].d_off), toff);
    put16(off + offsetof(struct dlabel, d_slice[0].d_len), 0);
    /*
     * Where this medium's sectors are numbered from.  The loader cannot
     * read the label without knowing it, so it reads the first sector of
     * the track that carries a magic and takes the answer from there;
     * this is the same answer written down, and every block after the
     * first needs it.
     */
    put16(off + offsetof(struct dlabel, d_firstsec), firstsec);

    if (!(out = fopen(outname, "wb"))) {
        die("cannot write the image");
    }
    if (fwrite(sector, 1, BSIZE, out) != BSIZE) {
        die("short write on the boot sector");
    }

    if (dflag) {
        /*
         * Level 1, which is the rest of that file, in the block after
         * level 0's.  The read above stopped at HEADLEN, so the file is
         * positioned where level 1 begins.  Pad the block out: level 2
         * lands at block 2 and must land there whatever level 1 weighs.
         */
        n = 0;
        while ((c = getc(in)) != EOF) {
            if (n >= BSIZE) {
                die("level 1 runs past its block");
            }
            putc(c, out);
            n++;
        }
        while (n++ < BSIZE) {
            putc(0, out);
        }
    }
    fclose(in);

    /* the second level follows, from the next sector */
    if (!(in = fopen(l2name, "rb"))) {
        die("cannot read the second level");
    }
    while ((c = getc(in)) != EOF) {
        putc(c, out);
    }
    fclose(in);
    if (fclose(out)) {
        die("write failed");
    }
    return 0;
}

/* vim: tabstop=4 shiftwidth=4 expandtab: */
