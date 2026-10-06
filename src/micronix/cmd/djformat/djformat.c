/*
 * djformat - format a Morrow DJ-DMA floppy diskette
 *
 * cmd/djformat/djformat.c
 *
 *	djformat device [track]
 *
 * Formats a diskette one track per ioctl, printing a line per track.
 * The device node names the drive (its minor), and the geometry is the
 * 77 tracks of an 8 inch drive - the only kind this machine's diskettes
 * come as (sys/dj.c's specs).
 *
 * An 8 inch diskette is two densities: track 0 is single density, 26
 * sectors of 128 bytes, and tracks 1 through 76 are double density, 15
 * sectors of 512 bytes.  Each is named to the controller with SETMP
 * (the media parameters: first sector, and sectors per track, which also
 * fixes the sector size), and then laid down with WRITETRK, the board's
 * built-in "write a whole track".  The two are sent as one DJ-DMA program
 * through the driver's fourth block entry (djioctl, sys/dj.c): SETMP,
 * then WRITETRK, then a HALT, each carrying its status at a fixed offset.
 *
 * A format blanks the diskette, so any filesystem goes with it and mkfs
 * follows.  The tab address the write carries is left null - the real
 * board reads a sector header table there to skip sectors, and a format
 * of a whole track skips none.
 */

#include <types.h>
#include <stdio.h>
#include <sys/fs.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#define SETMP    0x31           /* set media parameters, sys/dj.h */
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
 * Format one track: SETMP for its density's layout, then a WRITETRK,
 * ended by a HALT.  The write's own status lands in cmd[14], which is
 * what this checks; the halt status in cmd[16] only says the program ran.
 */
int
fmtrack(fd, drive, cyl, secs)
    int fd;
    UINT drive, cyl, secs;
{
    struct cdb r;
    int st;

    r.len = 17;
    r.cmd[0] = SETMP;           /* the density's media parameters */
    r.cmd[1] = drive;
    r.cmd[2] = 0;               /* write precomp starts at track 0 */
    r.cmd[3] = NTRACK - 1;      /* highest track */
    r.cmd[4] = 1;               /* first sector's number */
    r.cmd[5] = secs;            /* sectors per track */
    r.cmd[6] = 0;               /* the SETMP status */
    r.cmd[7] = WRITETRK;
    r.cmd[8] = cyl;
    r.cmd[9] = 0;               /* head 0 */
    r.cmd[10] = drive;
    r.cmd[11] = 0;              /* tab address, null: no sector is skipped */
    r.cmd[12] = 0;
    r.cmd[13] = 0;
    r.cmd[14] = 0;              /* the write's status, written back */
    r.cmd[15] = HALT;
    r.cmd[16] = 0;              /* the halt's status */
    r.count = 0;
    r.flags = 0;
    r.buf = 0;
    r.hole = -1;

    if (ioctl(fd, CDBCMD, &r) < 0)
        return (-1);
    st = r.cmd[14] & 0377;
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
        /* track 0 is single density; the rest are double */
        if (fmtrack(fd, drive, track, track == 0 ? 26 : 15) < 0) {
            fprintf(stderr, "%s: format failed at track %d\n", pname, track);
            exit(1);
        }
        printf("track %d\n", track);
    }
    return (0);
}
