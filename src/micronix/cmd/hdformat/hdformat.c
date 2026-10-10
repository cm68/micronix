/*
 * hdformat - probe and format a HDCA drive (the M10, M20 and M26)
 *
 * cmd/hdformat/hdformat.c
 *
 *	hdformat -p device					probe the geometry
 *	hdformat -m model [-c cyl] [-h heads] [-s spt] device	format
 *
 * The HDCA is the port-mapped, non-DMA Winchester controller of the
 * M10, M20 and M26.  It has no command block of its own, so a program
 * that wants to talk to it does so through the driver's raw ioctl
 * (include/sys/ioctl.h's CDBCMD door), the way mwformat drives the
 * HD-DMA: a block of opcode and head/track/sector, with the data phase
 * in a buffer.  The driver runs it against the same interrupt-driven
 * phases a read or a write uses (sys/hdca.c).
 *
 * What a drive is comes from the command line: a model name names one
 * of the three Discus drives this controller reached, or -c, -h and -s
 * say it outright, and a model plus those flags is the model with those
 * numbers overridden.  The device node is not consulted - it names a
 * drive and a slice, and no geometry at all.
 *
 * A formatted drive carries its geometry in its sector headers: each
 * header is four bytes, head, track, sector, key.  The probe reads them
 * to find the drive's shape - step out one track at a time until a
 * header stops matching (the cylinders), select each head until one
 * stops matching (the heads), and read a whole revolution of headers to
 * find the highest sector number (the sectors per track).  The format
 * writes those headers, one track and head at a time, in the interleave
 * the controller's own formatter used.
 *
 * vim: tabstop=4 shiftwidth=4 noexpandtab:
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
#include <sys/hdca.h>

/*
 * The drives this controller reached, and their shape.  These are the
 * numbers FORMATHD.COM carried (src/hwsim/resources/cpm22/formathd.asm):
 * the M26 is 202 tracks of 8 heads of 32 sectors, the M10 is 244 of 4
 * of 21, and the M20 is the M10's geometry with the M26's eight heads.
 */
struct model
{
    char *name;
    char *mfg;                  /* manufacturer, into the label */
    char *model;                /* model number, into the label */
    UINT cyl;                   /* cylinders, tracks per surface */
    UINT8 heads;
    UINT8 spt;                  /* sectors per track */
};

static struct model models[] = {
    {"m10", "", "M10", 244, 4, 21},
    {"m20", "", "M20", 244, 8, 21},
    {"m26", "", "M26", 202, 8, 32},
};

#define NMODEL (sizeof models / sizeof models[0])

extern int errno;
char *pname = "hdformat";

static int devfd;		/* the whole-disk slice, hdac etc. */
static int drive;		/* which of the controller's four drives */
static char *devname;		/* the slice named on the command line */
static UINT cyl, heads, spt;	/* the geometry being laid down */
static char *mfg, *model;	/* the drive's own name, into the label */

usage()
{
	fprintf(stderr, "usage: %s -p device\n", pname);
	fprintf(stderr, "       %s -m model [-c cyl] [-h heads] [-s spt] device\n",
	    pname);
	fprintf(stderr, "  model is m10, m20 or m26; device is hdac, hdbc, ...\n");
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
 * Run one command block: an opcode and the head, track and sector to
 * seek to, with the data phase, when there is one, through buf.  Returns
 * the status the driver answered in cmd[HDLEN], or -1 if the ioctl
 * itself failed.
 */
int
run(op, head, track, sector, count, flags, buf)
	int op, head, track, sector, count, flags;
	char *buf;
{
	struct cdb r;
	int st;

	memset((char *) &r, 0, sizeof r);
	r.cmd[0] = op;
	r.cmd[1] = head;
	r.cmd[2] = track;
	r.cmd[3] = sector;
	r.len = HDLEN;
	r.count = count;
	r.flags = flags;
	r.buf = buf;
	r.hole = -1;

	if (ioctl(devfd, CDBCMD, &r) < 0)
		return (-1);
	st = r.cmd[HDLEN] & 0377;
	return (st);
}

/*
 * Read one sector header at head/track into hd[4].
 */
int
readhdr(head, track, hd)
	int head, track;
	char *hd;
{
	return (run(HDCA_READHDR, head, track, 0, 4, CDB_IN, hd));
}

/*
 * Probe the drive's shape by reading its sector headers.  The track
 * scan steps out one track at a time until a header reads back a
 * different track; the head scan selects each head until one stops
 * matching; the sector scan reads a revolution of headers and reports
 * the highest sector number seen.
 */
probe()
{
	char hd[4];
	int cyln, headn, first, maxsec;
	unsigned long blocks;

	if (run(HDCA_HOME, 0, 0, 0, 0, 0, 0))
		die("cannot home the drive");

	for (cyln = 0; ; cyln++) {
		if (readhdr(0, cyln, hd) || hd[1] != cyln)
			break;
	}
	if (cyln == 0)
		die("no sector headers: the drive is not formatted");

	if (run(HDCA_HOME, 0, 0, 0, 0, 0, 0))
		die("cannot home the drive");

	for (headn = 0; ; headn++) {
		if (readhdr(headn, 0, hd) || hd[0] != headn)
			break;
	}

	readhdr(0, 0, hd);
	first = hd[2];
	maxsec = first;
	for (;;) {
		readhdr(0, 0, hd);
		if (hd[2] > maxsec)
			maxsec = hd[2];
		if (hd[2] == first)
			break;
	}

	blocks = (unsigned long) cyln * headn * maxsec;
	printf("%s: %d cylinders, %d heads, %d sectors/track, "
	    "%lu blocks of 512 bytes\n",
	    devname, cyln, headn, maxsec, blocks);
}

/*
 * The physical interleave: the order the controller writes the sector
 * headers in, so the next sector arrives under the head in time to be
 * read.  FORMATHD's two tables, one for each sector count the drives
 * come in; a count neither knows is written sequential.
 */
static UINT8 skewm26[] = {
    22, 12, 2, 23, 13, 3, 24, 14, 4, 25, 15, 5, 26, 16, 6,
    27, 17, 7, 28, 18, 8, 29, 19, 9, 30, 20, 10, 31, 21, 11, 32, 1
};
static UINT8 skewm10[] = {
    8, 15, 2, 9, 16, 3, 10, 17, 4, 11,
    18, 5, 12, 19, 6, 13, 20, 7, 14, 21, 1
};

/*
 * The sector number the header written at position i carries.
 */
int
skewat(i)
	int i;
{
	if (spt == 32)
		return (skewm26[i]);
	if (spt == 21)
		return (skewm10[i]);
	return (i + 1);
}

/*
 * Format the drive: one FORMATTRACK per head of one track, across the
 * whole geometry.  A format is a single blocking call that writes every
 * sector header of a track inside one revolution.  This program builds
 * the whole header table - head, track, sector and key for each sector,
 * in the interleave order - and hands it to the driver in one data
 * phase; the driver only writes what it is given.
 */
format()
{
	UINT track, head;
	char hdr[32 * 4];
	int i, key;

	if (run(HDCA_HOME, 0, 0, 0, 0, 0, 0))
		die("cannot home the drive");

	printf("%s: formatting %d cylinders, %d heads, %d sectors\n",
	    devname, cyl, heads, spt);

	for (track = 0; track < cyl; track++) {
		for (head = 0; head < heads; head++) {
			key = (track == 0) ? 0x80 : 0;
			for (i = 0; i < spt; i++) {
				hdr[i * 4 + 0] = head;
				hdr[i * 4 + 1] = track;
				hdr[i * 4 + 2] = skewat(i);
				hdr[i * 4 + 3] = key;
			}
			if (run(HDCA_FORMAT, head, track, spt, spt * 4, 0, hdr)) {
				fprintf(stderr, "format failed at track %d head %d, errno %d\n",
				    track, head, errno);
				die("format failed");
			}
		}
		printf("track %d\n", track);
	}
	printf("formatted %d cylinders\n", cyl);
}

/*
 * Write the label: what the drive is, at DL_OFFSET in block 0, after the
 * format has just filled the whole platter.  The slice table is empty -
 * the whole drive as 'c' - and the drive's own name goes in too.
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
	lp->d_firstsec = 1;		/* the HDCA numbers sectors from one */
	/* timing: the HDCA controller commands none of it, so it is zero */
	if (mfg)
		strncpy(lp->d_mfg, mfg, sizeof (lp->d_mfg));
	if (model)
		strncpy(lp->d_model, model, sizeof (lp->d_model));

	if (lseek(devfd, 0L, 0) < 0)
		die("cannot seek to block 0");
	if (write(devfd, buf, 512) != 512)
		die("cannot write block 0");
}

main(argc, argv)
	int argc;
	char **argv;
{
	struct model *mp, *row;
	char *arg;
	int i, n, probeonly;
	int cylset, headsset, sptset;
	UINT wcyl, wheads, wspt;
	struct stat sbuf;

	pname = argv[0];
	devname = 0;
	mp = 0;
	mfg = model = 0;
	probeonly = 0;
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

		if (arg[1] == 'p') {
			probeonly = 1;
			continue;
		}
		if (arg[1] == 'm') {
			if (++i == argc)
				usage();
			for (row = models; row < &models[NMODEL]; row++)
				if (strcmp(argv[i], row->name) == 0)
					break;
			if (row == &models[NMODEL])
				die("no such drive model");
			mp = row;
			continue;
		}

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
	drive = sbuf.st_addr[0] & 3;

	if ((devfd = open(devname, 2)) < 0)
		die("cannot open the device for writing");

	if (probeonly) {
		probe();
		exit(0);
	}

	/*
	 * The geometry: the model named on the command line, then the three
	 * flags, which override it.  There is no other source - the device
	 * node says nothing, and a drive about to be formatted is one whose
	 * label, if it has one, is about to be wrong.
	 */
	if (mp) {
		cyl = mp->cyl;
		heads = mp->heads;
		spt = mp->spt;
		mfg = mp->mfg;
		model = mp->model;
	}
	if (cylset)
		cyl = wcyl;
	if (headsset)
		heads = wheads;
	if (sptset)
		spt = wspt;

	if (cyl == 0 || heads == 0 || spt == 0)
		die("no geometry: name a model with -m, or give -c and -h");
	if (cyl > 255 || heads > 8 || spt > 32)
		die("that geometry does not fit the HDCA");

	format();
	putlabel();
	exit(0);
}
