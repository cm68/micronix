/*
 * djformat - format a Morrow DJ-DMA floppy diskette
 *
 * cmd/djformat/djformat.c
 *
 *	djformat device [track]
 *
 * Runs the controller's write-track command across a diskette, one track
 * per ioctl so the progress is a line per track.  The device node names
 * the drive (its minor), and the track count is the 77 of an 8 inch
 * drive - the only kind this machine's diskettes come as (sys/dj.c's
 * specs).
 *
 * The command is WRITETRK (0x2a), the board's built-in "write a whole
 * track", sent as a DJ-DMA program through the driver's fourth block
 * entry (djioctl, sys/dj.c).  A program is a list of the board's own
 * opcodes, each carrying its status at a fixed offset, ended by a HALT;
 * this one is the two commands a track takes - the write, then the halt.
 * What a write lays down is the controller's business, not this
 * program's: on the simulator it validates the drive and writes nothing,
 * because the image already is the format (hwsim/d1/djdma.c's writetrk).
 *
 * A format blanks the diskette, so any filesystem goes with it and mkfs
 * follows.  The tab address the write carries is left null - the real
 * board reads a sector header table there, and the simulator does not,
 * so a real format wants that table built and planted through req.hole.
 */

#include <types.h>
#include <stdio.h>
#include <sys/fs.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#define WRITETRK 0x2a           /* the board's opcode, sys/dj.h */
#define HALT     0x25
#define NTRACK   77             /* the 8 inch drive's tracks, sys/dj.c */
#define S_OK     0x40           /* "normal" completion, hwsim/d1/djdma.c */

static char *pname = "djformat";

void
die(msg)
    char *msg;
{
    fprintf(stderr, "%s: %s\n", pname, msg);
    exit(1);
}

/*
 * Format one track: a WRITETRK program ended by a HALT, ten bytes.  The
 * write's own status lands in cmd[7], which is what this checks; the halt
 * status in cmd[9] only says the program ran.
 */
int
fmtrack(fd, drive, cyl)
    int fd;
    UINT drive, cyl;
{
    struct cdb r;
    int st;

    r.len = 10;
    r.cmd[0] = WRITETRK;
    r.cmd[1] = cyl;
    r.cmd[2] = 0;               /* head 0 */
    r.cmd[3] = drive;
    r.cmd[4] = 0;               /* tab address, null: the sim ignores it */
    r.cmd[5] = 0;
    r.cmd[6] = 0;
    r.cmd[7] = 0;               /* the write's status, written back */
    r.cmd[8] = HALT;
    r.cmd[9] = 0;               /* the halt's status */
    r.count = 0;
    r.flags = 0;
    r.buf = 0;
    r.hole = -1;

    if (ioctl(fd, CDBCMD, &r) < 0)
        return (-1);
    st = r.cmd[7] & 0377;
    if (st != S_OK) {
        fprintf(stderr, "%s: track %d: status %x\n", pname, cyl, st);
        return (-1);
    }
    return (0);
}

main(argc, argv)
    int argc;
    char **argv;
{
    char *device;
    UINT drive, track, first, last;
    struct stat sbuf;
    int fd;

    pname = argv[0];
    if (argc < 2 || argc > 3) {
        fprintf(stderr, "usage: %s device [track]\n", pname);
        exit(1);
    }
    device = argv[1];

    if (stat(device, &sbuf) < 0)
        die("cannot stat the device");
    if ((sbuf.st_mode & IFMT) != IFBLK)
        die("not a block special file");
    drive = sbuf.st_addr[0] & 3;

    if ((fd = open(device, 2)) < 0)
        die("cannot open the device for writing");

    if (argc == 3) {
        first = last = atoi(argv[2]);
        if (last >= NTRACK)
            die("track out of range");
    } else {
        first = 0;
        last = NTRACK - 1;
    }

    printf("formatting %s: %d tracks\n", device, NTRACK);
    for (track = first; track <= last; track++) {
        if (fmtrack(fd, drive, track) < 0) {
            fprintf(stderr, "%s: format failed at track %d\n", pname, track);
            exit(1);
        }
        printf("track %d\n", track);
    }
    return (0);
}
