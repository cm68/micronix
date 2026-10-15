/*
 * mkbootimg - a boot image: the two sectors, spliced
 *
 * micronix/stand/boot/mkbootimg.c
 *
 * A boot image is the level 2 sector, which the rom reads to 0100 and
 * jumps to, and the level 3 program behind it, which level 2 loads.  The
 * level 2 sector comes first; level 3 begins at the first sector of
 * cylinder 1 - the first sector of the second cylinder on every medium
 * this builds for - and everything between is zeros.
 *
 * Level 3 is a cylinder up and not a block up because of the eight inch
 * diskette: its cylinder 0 head 0 is single density, 26 sectors of 128
 * bytes, and block 0 is four of them.  The rest of that track is no
 * place for a four kilobyte program, and the boot area owns cylinders 0
 * and 1 on every medium anyway (the label's toff is 2), so filling
 * cylinder 0 costs nothing that was going to be used.  It also keeps
 * level 3 at one address in a raw image for every medium: cylinder 1,
 * block 0, byte spc*512.  See djboot1.s, which reads it there.
 *
 * The second half of that first sector is not level 2's.  From DL_OFFSET
 * on it is a struct dlabel - the geometry, where the boot area is, and
 * the rotation sys/mw.c applies - which is how a reader maps a block
 * without inverting the arithmetic that put the area where it is.  See
 * sys/dlabel.h.  Who fills that half in is what -d is for.
 *
 * A diskette's image carries the label itself, because for a diskette
 * the label is the medium's only description: the loader reads it to
 * find where the filesystem begins, and mnix bootflop reads it back to
 * size the image it is making.  So a diskette image is per medium and
 * -d says which.
 *
 * A hard disk's image does not, and cannot: the geometry is the
 * drive's and no image knows which drive it will land on.  What knows
 * is the installer, which is handed a boot image and a device -
 * cmd/mkfs takes the shape from the device node and writes the label
 * into the block it puts the boot in.  putlabel in cmd/mkfs/mkfsfunc.c
 * is that writer, and it is the same code that stamps a boot installed
 * any other way.  So a hard disk's second half is left zero here, for
 * that writer to fill, and this tool is never told a geometry it would
 * only be guessing at.
 *
 * Zero is what makes that legal, and it is checked rather than assumed:
 * anything at all in that half is level 2 having grown into the label,
 * which is a boot that would be quietly truncated.
 *
 * This is a host program.  It includes the target's dlabel.h so that
 * the structure it writes cannot drift from the structure mkfs writes
 * and the loader reads - the fields are 16 bit and this writes them a
 * byte at a time, little endian, so a host with wider ints or another
 * byte order still produces a Z80 image.
 *
 * What it also refuses is a level 3 program that reaches the 0ff0 the
 * kernel is loaded from: see BOOTLIMIT, below.  That is a limit every
 * level 3 shares and this is the one place all of them pass through.
 *
 *	mkbootimg <level2> <level3> <out>
 *	mkbootimg -d <level2> <level3> <tracks> <heads> <spt> <firstsec>
 *	    <toff> <out>
 *
 * -d is the diskette layout, and it differs in one thing: the label,
 * which is written into the second half of the first sector instead of
 * being left for an installer.  The first half is level 2 either way,
 * and level 2 must fit in it - every medium this tree boots from is read
 * to 0100 by something that hands over a whole 512 byte sector, the
 * floppy included, since the DJ-DMA's own bootstrap (the one that copied
 * 128 bytes to 0080) is switched off at the card and the level 1 rom
 * drives its channel program instead.  See sys/dlabel.h and djboot1.s.
 *
 * firstsec is where the medium numbers its sectors from.  It is one on
 * the soft sectored eight inch media and zero on the hard sectored five
 * inch one.  Level 2 reads it back out of the label twice over: it is
 * the first sector of a track, so it is the sector level 3 begins at,
 * and with d_spt it is how level 2 and level 3 both work out how many
 * bytes a sector holds.  It is the only place either can come from,
 * since the numbering is not derivable from the geometry.  See
 * sys/dlabel.h, sys/dj.c's ORG1 and djboot1.s's DL_1ST.
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
#include "../../include/obj.h"

#define BSIZE   512

/*
 * Where a level 3 program has to end.  Everything it has - text, data and
 * bss together - must end below 0ff0, and there is nowhere else for it to
 * go: it is unpacked at 0100 and reads the kernel in from 0ff0 up, so a
 * byte past that is written over by the first block of the load it is
 * making.  It presents as a load that goes fine and then enters nothing,
 * which is not something to leave to a machine to show: every level 3
 * program, for every medium, passes through here, so this is where the
 * limit is measured.  boot.c says the same thing at more length.
 */
#define BOOTLIMIT   0xff0

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
 * Refuse a level 3 that reaches the limit.  The object header is the one
 * the loader reads, include/obj.h; dataoff is where data begins, so
 * dataoff + data + bss is the first byte of memory level 3 does not own -
 * bss is counted because it is memory even though it is not in the file.
 */
static void
checklevel3(char *name, FILE *in)
{
    struct obj h;
    unsigned end;

    if (fread(&h, 1, sizeof h, in) != sizeof h || h.ident != OBJECT) {
        die("the level 3 program is not an object file");
    }
    end = h.textoff + h.text + h.data + h.bss;
    if (end > BOOTLIMIT) {
        fprintf(stderr, "mkbootimg: %s ends at %#x, past %#x\n",
            name, end, BOOTLIMIT);
        exit(1);
    }
    rewind(in);
}

int
main(int argc, char **argv)
{
    FILE *in;
    FILE *out;
    char *pname;
    char *l2name;
    char *l3name;
    char *outname;
    int n;
    int off;
    int dflag;
    unsigned tracks = 0, heads = 0, spt = 0, firstsec = 0, toff = 0;
    unsigned spc = 0, bootblks = 0, loadaddr = 0;
    int c;

    pname = argv[0];
    dflag = 0;
    /*
     * -a <addr> writes the entry address into the sector's last two bytes
     * (510-511).  The HDCA's rom reads those two bytes as the address it
     * loads the level 2 sector to and then jumps to, where the HD-DMA rom
     * needs no such address - it always reads the sector to 0100 and
     * enters there.  See mon447.s boothd.
     */
    if (argc > 2 && strcmp(argv[1], "-a") == 0) {
        loadaddr = strtoul(argv[2], 0, 0);
        argv += 2;
        argc -= 2;
    }
    if (argc > 1 && strcmp(argv[1], "-d") == 0) {
        dflag = 1;
        argv++;
        argc--;
    }
    if (dflag ? argc != 9 : argc != 4) {
        fprintf(stderr,
            "usage: %s <level2> <level3> <out>\n"
            "       %s -d <level2> <level3> <tracks> <heads> <spt>"
            " <firstsec> <toff> <out>\n",
            pname, pname);
        exit(1);
    }
    l2name = argv[1];
    l3name = argv[2];
    outname = argv[dflag ? 8 : 3];
    if (dflag) {
        tracks = strtoul(argv[3], 0, 0);
        heads = strtoul(argv[4], 0, 0);
        spt = strtoul(argv[5], 0, 0);
        firstsec = strtoul(argv[6], 0, 0);
        toff = strtoul(argv[7], 0, 0);
        if (!tracks || !heads || !spt) {
            die("a geometry of zero");
        }
        /*
         * Zero or one, and nothing else: the driver's own tables have
         * one numbering for hard sectored media and one for soft, and a
         * track that began at sector five would be a format nobody has.
         */
        if (firstsec > 1) {
            die("a first sector that is neither zero nor one");
        }

        /*
         * Where the boot area is.  A diskette is not rolled - dj.c adds
         * toff and never rotates - so the boot is simply the blocks in
         * front of cylinder toff and no filesystem block reaches them.
         */
        spc = heads * spt;
        bootblks = toff * spc;
    }

    /*
     * The level 2 sector, into the front of the block.  It stops at
     * DL_OFFSET whatever it is being built for: the half from there on is
     * the label, either the one written below or the one mkfs will write,
     * and level 2 growing into it is a boot that would be quietly
     * truncated.
     */
    if (!(in = fopen(l2name, "rb"))) {
        die("cannot read the level 2 sector");
    }
    n = 0;
    while ((c = getc(in)) != EOF) {
        if (n >= DL_OFFSET) {
            die("the level 2 sector runs into the label");
        }
        sector[n++] = c;
    }
    fclose(in);

    /*
     * And the label, into the second half of it - on a diskette only.
     * A hard disk's half is left as the static buffer found it, zero,
     * for mkfs to fill: see the header.  Each field is placed by
     * offsetof rather than by counting, because the structure is shared
     * with mkfs and the loader and a field added to it should move
     * these rather than silently shift them all by two.
     */
    if (dflag) {
        off = DL_OFFSET;
        if (off + (int)sizeof(struct dlabel) > BSIZE) {
            die("the label does not fit the sector");
        }
        memcpy(&sector[off + offsetof(struct dlabel, d_magic)], DL_MAGIC, 4);
        put16(off + offsetof(struct dlabel, d_version), DL_VERSION);
        put16(off + offsetof(struct dlabel, d_tracks), tracks);
        put16(off + offsetof(struct dlabel, d_heads), heads);
        put16(off + offsetof(struct dlabel, d_spt), spt);
        put16(off + offsetof(struct dlabel, d_firstsec), firstsec);
        /*
         * A diskette's timing is the controller's, not the medium's, and
         * a diskette has no write precompensation or low write current,
         * so both are "never".  The kernel's dj.c reads and ignores all
         * four.
         */
        sector[off + offsetof(struct dlabel, d_step)] = 0;
        sector[off + offsetof(struct dlabel, d_seek)] = 0;
        put16(off + offsetof(struct dlabel, d_precomp), 0xffff);
        put16(off + offsetof(struct dlabel, d_lowcur), 0xffff);
        put16(off + offsetof(struct dlabel, d_bootblks), bootblks);
        put16(off + offsetof(struct dlabel, d_bootslice), 0);
        /*
         * The one slice a diskette has: 'a', a filesystem from cylinder
         * toff to the end of the medium, never rolled.
         */
        put16(off + offsetof(struct dlabel, d_slice[0].d_off), toff);
        put16(off + offsetof(struct dlabel, d_slice[0].d_len), tracks - toff);
        put16(off + offsetof(struct dlabel, d_slice[0].d_roll), 0);
        sector[off + offsetof(struct dlabel, d_slice[0].d_type)] = SL_FS;
    }

    if (loadaddr)
        put16(BSIZE - 2, loadaddr);

    if (!(out = fopen(outname, "wb"))) {
        die("cannot write the image");
    }
    if (fwrite(sector, 1, BSIZE, out) != BSIZE) {
        die("short write on the boot sector");
    }

    if (dflag) {
        /*
         * The rest of cylinder 0, empty.  Level 3 begins at the first
         * sector of cylinder 1 - a controller addresses sectors, and
         * the first sector of a track is the one number mkbootimg knows
         * before the medium does, since it is the label's d_firstsec -
         * so the cylinder in front of it is zeros.  That is one rule
         * for every medium, and the byte offset level 3 lands at is
         * always the medium's blocks per cylinder in 512 byte blocks.
         *
         * The extra block on a medium whose sectors are two blocks is
         * in that fill, not a case of its own: the fill is the whole
         * cylinder, so the second block of level 2's own sector is
         * simply the first of the zeros.  Which media those are is
         * djio.c's rule and has to stay equal to it: a soft sectored
         * medium with ten or sixteen blocks to a track is 1024 bytes a
         * sector.  See the header and djboot1.s.
         */
        for (n = BSIZE; n < (int) spc * BSIZE; n++)
            putc(0, out);
    }

    /* the level 3 program, at that sector */
    if (!(in = fopen(l3name, "rb"))) {
        die("cannot read the level 3 program");
    }
    checklevel3(l3name, in);
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
