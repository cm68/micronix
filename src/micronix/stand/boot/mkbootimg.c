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
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

/*
 * The target's header, by path rather than by -I: this is a host
 * program and its own stdio has to win.  UINT is the target's spelling
 * of a 16 bit unsigned, which is all dlabel.h needs of types.h.
 */
typedef unsigned short UINT;
#include "../../include/sys/dlabel.h"

#define BSIZE   512

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

int
main(int argc, char **argv)
{
    FILE *in;
    FILE *out;
    int n;
    int off;
    unsigned tracks, heads, spt, spc, roll, cyl0;
    int c;

    if (argc != 7) {
        fprintf(stderr,
            "usage: %s <level1> <level2> <tracks> <heads> <spt> <out>\n",
            argv[0]);
        exit(1);
    }
    tracks = strtoul(argv[3], 0, 0);
    heads = strtoul(argv[4], 0, 0);
    spt = strtoul(argv[5], 0, 0);
    if (!tracks || !heads || !spt) {
        die("a geometry of zero");
    }

    /*
     * Where the boot area is, computed the way mkfs computes it and
     * for the same reason: mw.c rotates the mapping, so physical
     * cylinder 0 is not block 0 but the block whose cylinder index is
     * tracks - tracks/2.
     */
    spc = heads * spt;
    roll = tracks >> 1;
    cyl0 = (tracks - roll) * spc;

    /* the first level, into the front of the sector */
    if (!(in = fopen(argv[1], "rb"))) {
        die("cannot read the first level");
    }
    n = 0;
    while ((c = getc(in)) != EOF) {
        if (n >= DL_OFFSET) {
            die("the first level runs into the label");
        }
        sector[n++] = c;
    }
    fclose(in);

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
    put16(off + offsetof(struct dlabel, d_version), DL_VERSION);
    put16(off + offsetof(struct dlabel, d_tracks), tracks);
    put16(off + offsetof(struct dlabel, d_heads), heads);
    put16(off + offsetof(struct dlabel, d_spt), spt);
    put16(off + offsetof(struct dlabel, d_cyl0), cyl0);
    put16(off + offsetof(struct dlabel, d_bootblks), spc);
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

    if (!(out = fopen(argv[6], "wb"))) {
        die("cannot write the image");
    }
    if (fwrite(sector, 1, BSIZE, out) != BSIZE) {
        die("short write on the boot sector");
    }

    /* the second level follows, from the next sector */
    if (!(in = fopen(argv[2], "rb"))) {
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
