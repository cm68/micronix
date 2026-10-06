/*
 * fdj - Morrow DJ-DMA floppy formatter
 *
 * cmd/fdj/fdj.c
 *
 *	A Micronix port of the CP/M "Morrow DJDMA Formatter revision 2"
 *	(FDJ.COM).  The original was disassembled and reconstructed in
 *	fdj.recon.c, which is the reference; this is the same program
 *	rebuilt on the kernel's ioctl(CDBCMD) instead of poking the
 *	controller directly.
 *
 *	Where CP/M let fdj own the machine - build a channel program at
 *	0x50 and OUT 0xef - here every command goes through sys/dj.c's
 *	CDBCMD ioctl, which does the address translation (r.hole), the bus
 *	arbitration, and the interrupt wait.
 *
 *	A format is a soft-sectored WRITE of every sector: on the DJ-DMA a
 *	soft-sectored write lays down the ID header and the data together,
 *	which is the format.  Each track is then verified by reading the
 *	sectors back and checking the status is OKSTAT (0x40).  The
 *	sector size and density come from the drive's own tables, not from
 *	this program.
 *
 *	usage: fdj device            menu-driven (F/G/H, C/D/E, A/B, I)
 *	       fdj device -b bytes -d drive -h holes -i inches -n density
 *	                   -p sectors -s sides -t tracks
 */

#include <types.h>
#include <stdio.h>
#include <sys/fs.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/dj.h>

/*
 * The fdj format blob.  The board's whole-track formatter is downloaded as a
 * routine (its first sixteen bytes are the geometry the host patches in) and
 * run through DJEXEC: DJFMT lays a track down from the head, DJADV advances
 * to the next one.  MEMWRITE and DJEXEC come from sys/dj.h.
 */
#define DJCODE  0x1030         /* where the blob is downloaded */
#define DJFMT   0x1040         /* fdj's format entry */
#define DJADV   0x1043         /* fdj's advance entry */
#define DJFMA   0x1049         /* fdj's format-and-advance entry */
#define H_LCODE 4              /* head: sector length code 0..3 */
#define H_DLAST 6              /* head: sectors per track + 1 */
#define H_FILL  7              /* head: the fill byte (0xe5) */

static char *pname = "fdj";

static int fd;
static int drive;
static int inches = 8, tracks = 77, sides = 1, sectors = 26, bytes = 128;
static char density = 's';              /* 's' single, 'd' double */
static int holes = 0;                   /* 0 soft sectored, 2 hard sectored */

static char secbuf[1024];               /* one sector: the fill byte */

/*
 * The format table, one entry per menu letter a..h.  These are the exact
 * geometries fdj advertised: [inches, tracks, sides, sectors, bytes,
 * density, holes].
 */
static struct {
    char inches;
    int tracks, sides, sectors, bytes;
    char density;
    int holes;
} fmt[8] = {
    { 5, 40, 1,  5, 1024, 'd', 0 },    /* a - 5.25 soft single   */
    { 5, 40, 2,  5, 1024, 'd', 0 },    /* b - 5.25 soft double   */
    { 5, 40, 1, 10,  512, 'd', 2 },    /* c - 5.25 hard single   */
    { 5, 40, 2, 10,  512, 'd', 2 },    /* d - 5.25 hard double   */
    { 5, 40, 2, 10,  512, 'd', 2 },    /* e - 5.25 hard micronix */
    { 8, 77, 1, 26,  128, 's', 0 },    /* f - 8 CP/M single      */
    { 8, 77, 2,  8, 1024, 'd', 0 },    /* g - 8 CP/M double      */
    { 8, 77, 2, 15,  512, 'd', 0 },    /* h - 8 Micronix         */
};

void
die(msg)
    char *msg;
{
    fprintf(stderr, "%s: %s\n", pname, msg);
    exit(1);
}

/*
 * Run one channel program through the driver.  cmd is filled in, run, and
 * the controller's status bytes are copied back into it.  buf/count is the
 * data phase, flags is CDB_IN for a read, and hole is where in cmd[] the
 * buffer's physical address goes (the driver translates it).
 */
static int
run(cmd, len, buf, count, flags, hole)
    char *cmd;
    int len, count, flags, hole;
    char *buf;
{
    struct cdb r;
    int i;

    if (len > sizeof(r.cmd) || count > CDBDATA)
        return -1;
    for (i = 0; i < len; i++)
        r.cmd[i] = cmd[i];
    r.len = len;
    r.count = count;
    r.flags = flags;
    r.buf = buf;
    r.hole = hole;
    if (ioctl(fd, CDBCMD, &r) < 0)
        return -1;
    for (i = 0; i < len; i++)
        cmd[i] = r.cmd[i];
    return 0;
}

/*
 * sense_drive - GSTAT.  The firmware writes the drive's dcb (characteristics),
 * slc (sector length code) and dsb (status) into the command block, and its
 * own return status at byte 5, so the whole command is six bytes and HALT
 * follows at byte 6.
 */
static int
sense_drive(d, dcb, slc, dsb)
    int d;
    char *dcb, *slc, *dsb;
{
    char cmd[8];

    cmd[0] = STATUS; cmd[1] = d;
    cmd[2] = cmd[3] = cmd[4] = cmd[5] = 0;
    cmd[6] = HALT; cmd[7] = 0;
    if (run(cmd, 8, 0, 0, 0, -1) < 0)
        return -1;
    *dcb = cmd[2];
    *slc = cmd[3];
    *dsb = cmd[4];
    return 0;
}

/*
 * build_head - fill the format blob's sixteen-byte head with one track's
 * geometry.  The board's formatter reads this: the sector length code, the
 * sectors-per-track (plus one), and the fill byte.
 */
static void
build_head(head, nsec, secsize)
    char *head;
    int nsec, secsize;
{
    int lcode = 0, s;

    memset(head, 0, 16);
    head[1] = tracks;
    for (s = secsize; s > 128; s >>= 1)
        lcode++;
    head[H_LCODE] = lcode;
    head[H_DLAST] = nsec + 1;
    head[H_FILL] = 0xe5;
}

/*
 * send_memwrite - MEMWRITE: copy src (count bytes) into the controller's
 * memory at dest.  This is how the format blob reaches the board.
 *
 * MEMWRITE is eight bytes and carries no status of its own, so HALT is
 * appended: without it the driver's wait status would sit on dest's high
 * byte (the last byte of the command) and be overwritten with NOSTAT.
 */
static int
send_memwrite(src, dest, count)
    char *src;
    int dest, count;
{
    char cmd[10];

    cmd[0] = MEMWRITE; cmd[1] = cmd[2] = cmd[3] = 0;      /* hole here */
    cmd[4] = count & 0xff; cmd[5] = count >> 8;
    cmd[6] = dest & 0xff; cmd[7] = dest >> 8;
    cmd[8] = HALT; cmd[9] = 0;
    return run(cmd, 10, src, count, 0, 1);
}

/*
 * send_gomem - DJEXEC: run the routine the board has at addr.  Returns the
 * status the routine left in the command block.
 */
static int
send_gomem(addr)
    int addr;
{
    char cmd[4];

    cmd[0] = DJEXEC;
    cmd[1] = addr & 0xff; cmd[2] = addr >> 8;
    cmd[3] = 0;                                          /* status */
    if (run(cmd, 4, 0, 0, 0, -1) < 0)
        return -1;
    return cmd[3] & 0377;
}

/*
 * read_sector - SETDMA + SREAD.  The sector's data comes back into buf;
 * returns the read's status.
 */
static int
read_sector(d, track, side_sector, buf, nbytes)
    int d, track, side_sector, nbytes;
    char *buf;
{
    char cmd[11];

    cmd[0] = SETDMA; cmd[1] = cmd[2] = cmd[3] = 0;      /* hole here */
    cmd[4] = SREAD; cmd[5] = track; cmd[6] = side_sector; cmd[7] = d;
    cmd[8] = 0;                                          /* status */
    cmd[9] = HALT; cmd[10] = 0;
    if (run(cmd, 11, buf, nbytes, CDB_IN, 1) < 0)
        return -1;
    return cmd[8] & 0377;
}

/*
 * The geometry of one track.  Track 0 of an 8 inch diskette is the
 * single-density IBM-3740 boot track - 26 sectors of 128 bytes - whatever
 * the chosen 8 inch format; the rest of the disk uses the table's geometry.
 * A 5 1/4 inch disk has no such split.
 */
static void
track_geometry(track, nsec, nbytes, firstsec)
    int track;
    int *nsec, *nbytes, *firstsec;
{
    if (inches == 8 && track == 0) {
        *nsec = 26;
        *nbytes = 128;
    } else {
        *nsec = sectors;
        *nbytes = bytes;
    }
    *firstsec = (holes == 2) ? 0 : 1;   /* hard sectored counts from 0 */
}

/*
 * verify_track - read every sector back and require OKSTAT.
 */
static int
verify_track(track)
    int track;
{
    int nsec, nbytes, firstsec;
    int side, s, st;

    track_geometry(track, &nsec, &nbytes, &firstsec);
    for (side = 0; side < sides; side++)
        for (s = 0; s < nsec; s++) {
            st = read_sector(drive, track,
                (side ? 0x80 : 0) | (firstsec + s), secbuf, nbytes);
            if (st != OKSTAT) {
                fprintf(stderr,
                    "%s: track %d side %d sector %d: verify status %x\n",
                    pname, track, side, firstsec + s, st);
                return -1;
            }
        }
    return 0;
}

static int
do_format()
{
    char head[16];
    int first = 0, track;

    /*
     * Format: download the geometry head, then run the board's formatter
     * track by track.  An 8 inch diskette's track 0 is single density
     * (26 x 128) whatever the chosen format, so it gets its own head and
     * its own download.
     */
    if (inches == 8) {
        build_head(head, 26, 128);
        if (send_memwrite(head, DJCODE, 16) < 0)
            return -1;
        if (send_gomem(DJFMT) < 0 || send_gomem(DJADV) < 0)
            return -1;
        first = 1;
    }
    build_head(head, sectors, bytes);
    if (send_memwrite(head, DJCODE, 16) < 0)
        return -1;
    if (send_gomem(DJFMT) < 0 || send_gomem(DJADV) < 0)
        return -1;
    for (track = first + 1; track < tracks; track++) {
        if (send_gomem(DJFMA) < 0)
            return -1;
        if ((track & 7) == 7)
            putchar('.');
    }
    putchar('\n');

    /* Verify: read every sector back. */
    for (track = 0; track < tracks; track++)
        if (verify_track(track) < 0)
            return -1;
    return 0;
}

static void
apply(int k)
{
    inches = fmt[k].inches;
    tracks = fmt[k].tracks;
    sides = fmt[k].sides;
    sectors = fmt[k].sectors;
    bytes = fmt[k].bytes;
    density = fmt[k].density;
    holes = fmt[k].holes;
}

/*
 * The menu.  Returns 0 if the user chose nothing to do.
 */
static int
menu()
{
    char c;

    puts("8 inch formats");
    puts("\tF) CP/M standard single density");
    puts("\tG) Morrow CP/M double density");
    puts("\tH) Morrow Micronix");
    puts("5 inch hard sectored formats");
    puts("\tC) Single sided");
    puts("\tD) Double sided");
    puts("\tE) Morrow Micronix");
    puts("5 inch soft sectored formats");
    puts("\tA) Morrow single sided");
    puts("\tB) Morrow double sided");
    puts("I) Other");
    printf("Selection: ");

    for (;;) {
        c = getchar();
        if (c == 'a' || c == 'b' || c == 'c' || c == 'd' ||
            c == 'e' || c == 'f' || c == 'g' || c == 'h') {
            apply(c - 'a');
            return 1;
        }
        if (c == 'i') {
            /* DIY: ask for each parameter in turn */
            die("do-it-yourself mode is not yet ported");
        }
        if (c == '\n' || c == '\r' || c == '\033' || c == 'q')
            return 0;
    }
}

/*
 * The command line: fdj device [-b bytes] [-d drive] [-h holes] [-i inches]
 *                          [-n density] [-p sectors] [-s sides] [-t tracks]
 */
static int
args(argc, argv)
    int argc;
    char **argv;
{
    int i;

    for (i = 2; i < argc; i++) {
        if (argv[i][0] != '-' || argv[i][1] == 0 || argv[i][2] == 0)
            return -1;
        switch (argv[i][1]) {
        case 'b': bytes = atoi(&argv[i][2]); break;
        case 'd': drive = atoi(&argv[i][2]); break;
        case 'h': holes = (atoi(&argv[i][2]) == 1) ? 0 : 2; break;
        case 'i': inches = atoi(&argv[i][2]); break;
        case 'n': density = (atoi(&argv[i][2]) == 1) ? 's' : 'd'; break;
        case 'p': sectors = atoi(&argv[i][2]); break;
        case 's': sides = atoi(&argv[i][2]); break;
        case 't': tracks = atoi(&argv[i][2]); break;
        default: return -1;
        }
    }
    return 0;
}

main(argc, argv)
    int argc;
    char **argv;
{
    struct stat sbuf;
    char dcb, slc, dsb;

    pname = argv[0];
    if (argc < 2) {
        fprintf(stderr, "usage: %s device [options]\n", pname);
        exit(1);
    }

    if (stat(argv[1], &sbuf) < 0)
        die("cannot stat the device");
    if ((sbuf.st_mode & IFMT) != IFBLK)
        die("not a block special file");
    drive = sbuf.st_addr[0] & 7;        /* the minor's low three bits, like djopen */

    if ((fd = open(argv[1], 2)) < 0)
        die("cannot open the device for writing");

    if (argc > 2) {
        if (args(argc, argv) < 0)
            die("bad option");
    } else {
        if (!menu())
            exit(0);
    }

    /*
     * Ask the board what is in the drive, and refuse to format what it
     * cannot.  (The original did this more elaborately - checking write
     * protect and hard/soft sectoredness - and we should too.)
     */
    if (sense_drive(drive, &dcb, &slc, &dsb) < 0)
        die("sense failed");
    if (dsb & S_WP)
        die("write protected");

    /*
     * An 8 inch drive's second side is only there if the media says so; a
     * single-sided diskette in a double-sided drive is formatted on one
     * side.  The board reports it in the status byte, not the menu choice.
     */
    if (inches == 8)
        sides = (dsb & SIDE2) ? 2 : 1;

    printf("formatting %d tracks, %d sectors of %d bytes, %d sides\n",
        tracks, sectors, bytes, sides);
    if (do_format() < 0)
        exit(1);
    printf("done.\n");
    return 0;
}
