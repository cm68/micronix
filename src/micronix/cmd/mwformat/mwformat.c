/*
 * mwformat - format a Morrow HD-DMA hard disk
 *
 * cmd/mwformat/mwformat.c
 *
 *	mwformat device [track]
 *
 * Runs the controller's format-track command across a drive, one track
 * per ioctl so the progress is a line per track.  The drive is named by
 * the device node, whose minor names the model - bits 2 and up, as
 * everywhere - and the model's geometry says how many tracks to walk.
 * This is the loop stand/formatmw ran from the boot ROM (its fmt), with
 * the command itself now in the driver (sys/mw.c's fourth block entry)
 * and the walk and the reporting here.
 *
 * A format blanks the disk - every sector header rewritten and every data
 * field filled - so the label and any filesystem go with it, and mkfs
 * follows.  Only the drives this machine reaches have nodes (dev/devlist),
 * so the table below is those models.
 */

#include <types.h>
#include <stdio.h>
#include <sys/fs.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#define FORM    3               /* the driver's opcode, sys/mw.c */

static UINT dtracks[] = { 153, 306, 306, 640, 733 };
static UINT dheads[] = { 4, 4, 6, 6, 5 };
static UINT dsecs[] = { 17, 17, 17, 17, 17 };
#define NDRIVE  5

static char *pname = "mwformat";

void
die(msg)
    char *msg;
{
    fprintf(stderr, "%s: %s\n", pname, msg);
    exit(1);
}

/*
 * Which model the node names, from the minor, or -1 if none.  mkfs.c's
 * drivetype, and the same reason: the answer has to match sys/mw.c's
 * devtype, not be told.
 */
int
drivetype(device)
    char *device;
{
    struct stat sbuf;
    int type;

    if (stat(device, &sbuf) < 0)
        die("cannot stat the device");
    if ((sbuf.st_mode & IFMT) != IFBLK)
        die("not a block special file");
    type = (sbuf.st_addr[0] & 0377) >> 2;
    if (type >= NDRIVE)
        return (-1);
    return (type);
}

/*
 * Format one track: the driver's command block, its opcode in cmd[0] and
 * the track in cmd[1..2], little-endian (sys/mw.c's mwioctl).
 */
int
fmtrack(fd, track)
    int fd;
    UINT track;
{
    struct cdb r;

    r.len = 3;
    r.cmd[0] = FORM;
    r.cmd[1] = track & 0377;
    r.cmd[2] = (track >> 8) & 0377;
    r.count = 0;
    r.flags = 0;
    r.buf = 0;
    r.hole = -1;
    return (ioctl(fd, CDBCMD, &r));
}

main(argc, argv)
    int argc;
    char **argv;
{
    char *device;
    UINT track, first, last;
    int type;
    int fd;

    pname = argv[0];
    if (argc < 2 || argc > 3) {
        fprintf(stderr, "usage: %s device [track]\n", pname);
        exit(1);
    }
    device = argv[1];

    type = drivetype(device);
    if (type < 0)
        die("no drive of that minor number");

    if ((fd = open(device, 2)) < 0)
        die("cannot open the device for writing");

    if (argc == 3) {
        first = last = atoi(argv[2]);
        if (last >= dtracks[type])
            die("track out of range");
    } else {
        first = 0;
        last = dtracks[type] - 1;
    }

    printf("formatting %s: %d tracks, %d heads\n",
        device, dtracks[type], dheads[type]);
    for (track = first; track <= last; track++) {
        if (fmtrack(fd, track) < 0) {
            fprintf(stderr, "%s: format failed at track %d\n", pname, track);
            exit(1);
        }
        printf("track %d\n", track);
    }
    return (0);
}
