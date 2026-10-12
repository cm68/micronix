/*
 * scsiformat - format a SCSI disk through the NCR 5380
 *
 * cmd/scsiformat/scsiformat.c
 *
 *	scsiformat [-c cyl] [-h heads] [-s spt] device
 *
 * A SCSI disk formats itself: the host sends FORMAT UNIT and the target
 * does the work, so there is no track walk here the way mwformat and
 * hdformat have - one command, and the drive is blank.  What this tool
 * then does is the part the drive cannot: write the disk label (the
 * geometry and the prototype slice table) into block 0, so the freshly
 * formatted drive can be opened and partitioned.
 *
 * The geometry is the target's own and is not a table's, so there is no
 * -m model.  -c, -h and -s say it outright, and without them the geometry
 * already in the label is kept, so a disk can be re-formatted without
 * re-typing its shape.
 *
 * The device is the drive's whole-disk slice, 'c' - /dev/scsi0c - the
 * same requirement cmd/label/label.c has and for the same reason: block 0
 * of that slice is the label's sector whatever the geometry turns out to
 * be.  FORMAT UNIT goes through the driver's raw command block
 * (include/sys/ioctl.h, CDBCMD), the way mwformat drives the HD-DMA.
 *
 * See docs/DISKLABEL.md for the label, and include/sys/ioctl.h for the
 * command-block interface.
 */

#include <types.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/fs.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/dlabel.h>
#include <sys/ncr.h>

#define CFORMAT	0x04		/* the SCSI opcode: the drive formats itself */
#define CDBLEN	10		/* the adapter's fixed command length */

extern int errno;
char *pname = "scsiformat";

static int devfd;		/* the whole-disk slice, scsi0c etc. */
static int drive;		/* the target's id, low three bits of the minor */
static char *devname;		/* the slice named on the command line */
static UINT cyl, heads, spt;	/* the geometry being laid down */

usage()
{
	fprintf(stderr, "usage: %s [-c cyl] [-h heads] [-s spt] device\n",
	    pname);
	fprintf(stderr, "  device is scsi0c, scsi1c, ... (the whole-disk slice)\n");
	exit(1);
}

die(s)
	char *s;
{
	fprintf(stderr, "%s: %s\n", pname, s);
	exit(1);
}

/*
 * Decimal, with no sign: every number on this command line is a
 * cylinder, a head or a sector count.
 */
UINT
dec(s)
	char *s;
{
	UINT v;

	v = 0;
	while (*s >= '0' && *s <= '9')
		v = v * 10 + (*s++ - '0');
	return (v);
}

/*
 * Send FORMAT UNIT.  The CDB is six bytes on a real bus - opcode, the
 * LUN and the format options, then four bytes of interleave and control
 * - but the adapter always moves CDBLEN bytes, so the block is padded
 * with zeroes.  There is no data phase: the drive finds its own geometry.
 */
void
format()
{
	struct cdb r;

	memset((char *) &r, 0, sizeof r);
	r.cmd[0] = CFORMAT;
	r.len = CDBLEN;
	r.count = 0;
	r.flags = 0;
	r.buf = 0;
	r.hole = -1;			/* SCSI: the adapter holds the address itself */

	if (ioctl(devfd, CDBCMD, &r) < 0)
		die("the driver refused FORMAT UNIT");
}

/*
 * Build the prototype slice table a freshly formatted disk carries.  The
 * layout is mwformat's: track 0 holds the boot and the label, 'a' is the
 * biggest filesystem that fits, 'b' is swap, and d..h are filesystems
 * until the drive runs out.  'c' is the whole disk and never an entry.
 */
void
protoslices(lp)
	struct dlabel *lp;
{
	UINT spc, off, left, maxfs, bsz, sz;
	int i;

	spc = heads * spt;
	off = 1;			/* cylinder 0: the boot + label */
	left = cyl - 1;

	maxfs = 0xffff / spc;		/* a block number is 16 bits */

	bsz = (2048 + spc - 1) / spc;	/* b, the swap: 1M, whole cylinders */
	if (bsz > left)
		bsz = left;

	if (left > bsz) {
		sz = left - bsz;
		if (maxfs < sz)
			sz = maxfs;
		lp->d_slice[0].d_off = off;
		lp->d_slice[0].d_len = sz;
		lp->d_slice[0].d_type = SL_FS;
		off += sz;
		left -= sz;
	}

	if (bsz) {
		lp->d_slice[1].d_off = off;
		lp->d_slice[1].d_len = bsz;
		lp->d_slice[1].d_type = SL_SWAP;
		off += bsz;
		left -= bsz;
	}

	for (i = 3; i < NSLICE; i++) {
		if (left == 0)
			break;
		sz = (maxfs < left) ? maxfs : left;
		lp->d_slice[i].d_off = off;
		lp->d_slice[i].d_len = sz;
		lp->d_slice[i].d_type = SL_FS;
		off += sz;
		left -= sz;
	}
}

/*
 * Write the label: what the drive is, at DL_OFFSET in block 0, after the
 * format has blanked the platter.  The boot fields are zero - mkfs -i
 * puts a boot there and rewrites the label's numbers when it does.  It is
 * written through the block device, not as a raw command, so the sector
 * in the cache and the sector on the disk are one object (mwformat says
 * why this matters).
 */
void
putlabel()
{
	static char buf[512];
	static struct dlabel *lp;
	int i;

	if (lseek(devfd, 0L, 0) < 0)
		die("cannot seek to block 0");
	if (read(devfd, buf, 512) != 512)
		die("cannot read block 0");

	lp = (struct dlabel *) &buf[DL_OFFSET];
	memset((char *) lp, 0, sizeof (struct dlabel));
	for (i = 0; i < 4; i++)
		lp->d_magic[i] = DL_MAGIC[i];
	lp->d_version = DL_VERSION;
	lp->d_tracks = cyl;
	lp->d_heads = heads;
	lp->d_spt = spt;
	lp->d_firstsec = 0;		/* SCSI numbers sectors from zero */

	protoslices(lp);

	if (lseek(devfd, 0L, 0) < 0)
		die("cannot seek to block 0");
	if (write(devfd, buf, 512) != 512)
		die("cannot write block 0");
}

/*
 * The geometry already in the label, if there is one, so a re-format can
 * run without re-typing the shape.  Answers no if the label is absent or
 * carries no geometry.
 */
int
getlabel()
{
	static char buf[512];
	static struct dlabel *lp;
	int i;

	if (lseek(devfd, 0L, 0) < 0)
		die("cannot seek to block 0");
	if (read(devfd, buf, 512) != 512)
		die("cannot read block 0");

	lp = (struct dlabel *) &buf[DL_OFFSET];
	for (i = 0; i < 4; i++)
		if (lp->d_magic[i] != DL_MAGIC[i])
			return (0);
	if (lp->d_tracks == 0 || lp->d_heads == 0 || lp->d_spt == 0)
		return (0);

	cyl = lp->d_tracks;
	heads = lp->d_heads;
	spt = lp->d_spt;
	return (1);
}

main(argc, argv)
	int argc;
	char **argv;
{
	char *arg;
	int i, n;
	int cylset, headsset, sptset;
	UINT wcyl, wheads, wspt;
	struct stat sbuf;

	pname = argv[0];
	devname = 0;
	cyl = heads = spt = 0;
	cylset = headsset = sptset = 0;

	for (i = 1; i < argc; i++) {
		arg = argv[i];
		if (arg[0] != '-') {
			if (devname)
				usage();
			devname = arg;
			continue;
		}
		if (arg[1] == 0 || arg[2] != 0)
			usage();
		if (++i == argc)
			usage();
		n = dec(argv[i]);
		switch (arg[1]) {
		case 'c':
			wcyl = n;
			cylset = 1;
			break;
		case 'h':
			wheads = n;
			headsset = 1;
			break;
		case 's':
			wspt = n;
			sptset = 1;
			break;
		default:
			usage();
		}
	}
	if (!devname)
		usage();

	if (stat(devname, &sbuf) < 0)
		die("cannot stat the device");
	if ((sbuf.st_mode & IFMT) != IFBLK)
		die("not a block special file");
	if (((sbuf.st_addr[0] >> 5) & 7) != DL_WHOLE)
		die("that node is not the whole-disk slice 'c'");
	drive = sbuf.st_addr[0] & 7;

	if ((devfd = open(devname, 2)) < 0)
		die("cannot open the device for writing");

	/*
	 * The geometry: the flags, then the label, in that order.  A SCSI
	 * disk's shape is its own, so a fresh one has to be told (-c/-h/-s),
	 * and a re-format keeps the label's numbers when the flags are absent.
	 */
	if (!getlabel())
		cyl = heads = spt = 0;
	if (cylset)
		cyl = wcyl;
	if (headsset)
		heads = wheads;
	if (sptset)
		spt = wspt;

	if (cyl == 0 || heads == 0 || spt == 0)
		die("no geometry: give -c and -h and -s");
	if (cyl > 0xffff || heads > 0xff || spt > 0xff)
		die("a label cannot hold that geometry");

	printf("%s: formatting target %d, %d cylinders, %d heads, %d sectors\n",
	    devname, drive, cyl, heads, spt);

	format();
	putlabel();

	printf("label: %d cylinders, %d heads, %d sectors, slices a b d e f g h\n",
	    cyl, heads, spt);
	exit(0);
}

/*
 * vim: tabstop=4 shiftwidth=4 noexpandtab:
 */
