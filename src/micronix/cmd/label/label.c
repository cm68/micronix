/*
 * label - read and write a hard disk's label
 *
 * cmd/label/label.c
 *
 *	label device				dump the label
 *	label -w cylinders/heads/sectors device	set the drive geometry
 *	label -s device slice start [end]	set one slice
 *	label -i device				ask the drive its capacity
 *
 * The label is what a disk says about itself: the geometry, where the
 * boot is, the roll, the size of the filesystem that was put on it, and a
 * table of eight slices.  It is struct dlabel (sys/dlabel.h) in the
 * second half of the boot sector - the sector the rom reads, physical
 * cylinder 0 head 0 sector 0 - and it is the only description of the
 * drive that is not a table compiled into a program.
 *
 * The device named must be the whole-disk slice, 'c'.  That is the one
 * slice whose position is not the label's to give: it starts at cylinder
 * 0 of the drive and is never rolled, so its block 0 is physical block 0
 * and reading it is reading the label.  The driver enforces that (mw.c,
 * ide.c), and this program will not write to any other slice, because
 * block 0 of a filesystem's slice is somewhere in the middle of that
 * filesystem and a label written there would be a label written over
 * somebody's data.  So the device is hd0c, hd1c, ide0c - drive, then c.
 *
 * Reading and writing the sector goes through the ordinary block device,
 * not a kernel call of its own: a block device opened for I/O passes the
 * offset's block number straight through (imap.c, an IIO file), so
 * lseek(fd, 0L, 0) is block 0 and a read or write of 512 bytes is that
 * one sector.  Only the label half of it is ever changed - the program
 * reads the whole sector, edits bytes DL_OFFSET..511 in place, and writes
 * the sector back - so the boot code in the first half survives.
 *
 * Writing a label to a disk that has none is the case that has to work
 * from nothing, and it is also the only case that needs care: the
 * geometry is being *declared* rather than corrected.  -w on an
 * unlabelled disk writes the roll as half the cylinders, which is what
 * mw.c assumes for an unlabelled disk and what the host fslib assumes for
 * one (fslib.c, "fall back on what mw.c would have assumed"), so the disk
 * maps the same way before and after and nothing declared here can move
 * data that is already on the drive.  -w on a disk that does have a label
 * changes the three geometry numbers and nothing else, leaving the roll,
 * the boot and the slice table exactly as they were.
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
#include <sys/dlabel.h>
#include <sys/ioctl.h>

#define BSIZE	512			/* the boot sector, and the block size */

/*
 * The block device majors (dev/devlist): 4 is the ide card and 5 the scsi
 * adapter.  They are the two drivers with an inquiry ioctl.
 */
#define IDEMAJ	4
#define NCRMAJ	5

extern int errno;			/* errno.h has the numbers, not the
					 * object - each program declares it */

char *pname = "label";
char *devname = 0;
int devfd = -1;
char buf[BSIZE];
unsigned char idbuf[512];	/* the identify block -i reads */
unsigned char capbuf[8];	/* the read-capacity answer -i reads */

/*
 * What the command line asked for.  -w is the geometry, at most once;
 * -s is one or more slices.  They are collected and then applied to the
 * one copy of the sector in buf, so "label -w ... -s ..." on a single
 * command line labels a disk and fills in its table with one write.
 */
int wgeom;				/* -w was given */
UINT gtracks, gheads, gspt;

struct setop {
	UINT so_sl;			/* which slice, 0..7 */
	UINT so_off;			/* first cylinder */
	UINT so_len;			/* cylinders, 0 = to the end */
};
struct setop sets[NSLICE];
int nsets;

usage()
{
	fprintf(stderr, "usage: %s device\n", pname);
	fprintf(stderr, "       %s -w cylinders/heads/sectors device\n", pname);
	fprintf(stderr, "       %s -s device slice start [end]\n", pname);
	fprintf(stderr, "       %s -i device\n", pname);
	fprintf(stderr, "  device is the whole-disk slice: hd0c, hd1c, ide0c\n");
	exit(1);
}

die(s)
	char *s;
{
	fprintf(stderr, "%s: %s\n", pname, s);
	exit(1);
}

/*
 * Decimal, with no sign and no base: every number on this command line is
 * a cylinder, a head count or a sector count, and none can be negative.
 * ep is left on the first character that is not a digit.
 */
UINT
dec(s, ep)
	char *s;
	char **ep;
{
	UINT v;

	v = 0;
	while (*s >= '0' && *s <= '9') {
		v = v * 10 + (*s - '0');
		s++;
	}
	*ep = s;
	return v;
}

/*
 * cylinders/heads/sectors, all three present and non-zero - a label with
 * a zero in it is one the driver refuses to use (mw.c, ide.c).
 */
getgeom(s)
	char *s;
{
	char *p;

	gtracks = dec(s, &p);
	if (*p++ != '/')
		return 0;
	gheads = dec(p, &p);
	if (*p++ != '/')
		return 0;
	gspt = dec(p, &p);
	if (*p || !gtracks || !gheads || !gspt)
		return 0;
	if (gtracks > 0xffff || gheads > 0xff || gspt > 0xff)
		return 0;
	return 1;
}

/*
 * A slice is named by letter a..h or by number 0..7: the letters are how
 * the label reads, and the numbers are how a minor number encodes it.
 */
getslice(s, sp)
	char *s;
	UINT *sp;
{
	if (*s >= 'a' && *s <= 'a' + NSLICE - 1 && s[1] == 0) {
		*sp = *s - 'a';
		return 1;
	}
	if (*s >= 'A' && *s <= 'A' + NSLICE - 1 && s[1] == 0) {
		*sp = *s - 'A';
		return 1;
	}
	if (*s >= '0' && *s < '0' + NSLICE && s[1] == 0) {
		*sp = *s - '0';
		return 1;
	}
	return 0;
}

/*
 * A cylinder number, and nothing after it.
 */
getnum(s, vp)
	char *s;
	UINT *vp;
{
	char *p;

	*vp = dec(s, &p);
	return *p == 0;
}

getlabel()
{
	return (struct dlabel *) &buf[DL_OFFSET];
}

islabel()
{
	register struct dlabel *lp;

	lp = getlabel();
	return lp->d_magic[0] == DL_MAGIC[0] && lp->d_magic[1] == DL_MAGIC[1] &&
	       lp->d_magic[2] == DL_MAGIC[2] && lp->d_magic[3] == DL_MAGIC[3] &&
	       lp->d_tracks && lp->d_heads && lp->d_spt;
}

/*
 * Block 0 - physical cylinder 0 head 0 sector 0.  On the whole-disk
 * slice that is the boot sector, and the label is in its second half.
 */
readsector()
{
	if (lseek(devfd, 0L, 0) < 0)
		die("cannot seek to block 0");
	if (read(devfd, buf, BSIZE) != BSIZE)
		die("cannot read block 0");
}

writesector()
{
	if (lseek(devfd, 0L, 0) < 0)
		die("cannot seek to block 0");
	if (write(devfd, buf, BSIZE) != BSIZE)
		die("cannot write block 0");
}

/*
 * Writing a label anywhere but the whole-disk slice would put it in the
 * middle of a filesystem - block 0 of slice 'a' is not block 0 of the
 * drive - so the slice is checked before anything is written.
 *
 * The device number comes out of the inode's first block pointer and not
 * out of st_dev.  st_dev is the device the inode lives on, which for a
 * node in /dev is the root filesystem - slice 'a' of the drive that was
 * mounted, and so a check that always refuses.  The node's own number is
 * i_addr[0], which is what the kernel opens the device from (fio.c) and
 * where the driver takes its mapping.
 */
checkwhole()
{
	struct stat st;
	UINT sl;

	if (fstat(devfd, (char *) &st) < 0)
		die("cannot stat the device");
	sl = (st.st_addr[0] >> 5) & 7;
	if (sl != DL_WHOLE) {
		fprintf(stderr, "%s: %s: slice %c is not the whole disk\n",
			pname, devname, 'a' + sl);
		exit(1);
	}
}

/*
 * How many blocks a slice of cyls cylinders holds.  A block number is a
 * UINT, so 64K blocks is all any filesystem can use.
 *
 * The product is taken in 32 bits and not 16: cyls and spc both go up to
 * 0xffff, and a wrapped product would report the opposite of the truth.
 * It is also the only form c1 builds - dividing by spc here asks for a
 * divisor in a register home, which has no rule (rules.c, the DIV
 * block).
 */
showblocks(cyls, spc)
	UINT cyls, spc;
{
	UINT32 n;

	if (spc == 0) {
		printf(" (no geometry)");
	} else {
		n = (UINT32) cyls * (UINT32) spc;
		if (n > 0xffff)
			printf(", more than 65535 blocks");
		else
			printf(", %u blocks", cyls * spc);
	}
	printf("\n");
}

dump()
{
	register struct dlabel *lp;
	UINT i, spc, off, cyls;

	lp = getlabel();
	spc = (UINT) lp->d_heads * lp->d_spt;

	printf("%s: %s version %u\n", devname, DL_MAGIC, lp->d_version);
	printf("\tgeometry\t%u cylinders, %u heads, %u sectors/track "
		"(%u blocks/cylinder)\n",
		lp->d_tracks, lp->d_heads, lp->d_spt, spc);
	printf("\troll\t\t%u - block 0 of a rolled slice is at cylinder %u\n",
		lp->d_roll, lp->d_roll);
	printf("\tboot\t\t%u blocks, loaded from slice %c\n",
		lp->d_bootblks, 'a' + lp->d_bootslice);
	printf("\tfilesystem\t%u blocks, %u inode blocks, %u swap blocks\n",
		lp->d_fsize, lp->d_isize, lp->d_swap);

	/*
	 * The table, entry by entry: the numbers as they are on the disk,
	 * and then what they mean.  An empty entry is the whole drive, and
	 * on the whole-disk slice 'c' that is meant literally - from
	 * cylinder 0 and unrolled - while every other slice takes the roll
	 * with it, which is how a disk made before this table existed
	 * describes itself.
	 */
	printf("\tslices\t\t(offset, length) in cylinders, "
		"0 length = to the end\n");
	for (i = 0; i < NSLICE; i++) {
		if (i == DL_WHOLE) {
			printf("\t  %c\t(-, -)\tcylinders 0 through %u, "
				"never rolled", 'a' + i, lp->d_tracks - 1);
			showblocks(lp->d_tracks, spc);
			continue;
		}
		if (!lp->d_slice[i].d_off && !lp->d_slice[i].d_len) {
			printf("\t  %c\t(0, 0)\tthe whole drive, rolled by %u",
				'a' + i, lp->d_roll);
			showblocks(lp->d_tracks, spc);
			continue;
		}
		off = lp->d_slice[i].d_off;
		cyls = lp->d_slice[i].d_len;
		if (cyls == 0)
			cyls = lp->d_tracks - off;
		printf("\t  %c\t(%u, %u)\tcylinders %u through %u, rolled by %u",
			'a' + i, off, lp->d_slice[i].d_len,
			off, off + cyls - 1, lp->d_roll);
		showblocks(cyls, spc);
	}
}

/*
 * -w.  A disk that already has a label is having its geometry corrected:
 * the three numbers change and everything else - the roll, the boot
 * extent, the filesystem's size, the table - is left alone, because those
 * describe what is on the disk and changing a geometry is not what put it
 * there.  A disk with no label is being described for the first time, so
 * the roll is written as half the cylinders: that is what mw.c assumes
 * for a disk with no label, so matching it means the disk maps the same
 * way after the label as it did before it.
 */
writegeom()
{
	register struct dlabel *lp;
	int i;

	lp = getlabel();
	if (!islabel()) {
		for (i = DL_OFFSET; i < BSIZE; i++)
			buf[i] = 0;
		for (i = 0; i < 4; i++)
			lp->d_magic[i] = DL_MAGIC[i];
		lp->d_version = DL_VERSION;
		lp->d_roll = gtracks >> 1;
		printf("%s: no label: writing %s version %u, roll %u\n",
			devname, DL_MAGIC, DL_VERSION, lp->d_roll);
	} else {
		printf("%s: geometry %u/%u/%u, was %u/%u/%u, "
			"roll and table left alone\n", devname,
			gtracks, gheads, gspt,
			lp->d_tracks, lp->d_heads, lp->d_spt);
	}
	lp->d_tracks = gtracks;
	lp->d_heads = gheads;
	lp->d_spt = gspt;
}

/*
 * -s.  A slice's extent is in cylinders, so writing one needs no
 * geometry - but it is checked against the drive's, because the driver
 * refuses a slice that runs past the end of the drive (mw.c, ide.c) and
 * finding that out here beats finding it out at mount.
 */
writeslice(op)
	register struct setop *op;
{
	register struct dlabel *lp;
	UINT off;

	lp = getlabel();
	off = op->so_off;

	/*
	 * Checked before the entry is set, and not after.  The label is
	 * written back whole once every -s has been taken, so an entry that
	 * is refused and left in the buffer is a refusal that changes the
	 * disk anyway - and the driver refuses the same slice at mount, so
	 * the operator is left with an error they cannot act on and a slice
	 * no filesystem will ever use.
	 */
	if (op->so_sl != DL_WHOLE &&	/* c ignores its entry */
	    (off >= lp->d_tracks ||
	     (op->so_len && (long) off + (long) op->so_len > (long) lp->d_tracks))) {
		fprintf(stderr, "%s: slice %c runs past cylinder %u\n",
			pname, 'a' + op->so_sl, lp->d_tracks);
		exit(1);
	}
	lp->d_slice[op->so_sl].d_off = op->so_off;
	lp->d_slice[op->so_sl].d_len = op->so_len;
	printf("%s: slice %c (offset %u, length %u)\n",
		devname, 'a' + op->so_sl, off, op->so_len);
}

/*
 * -i.  Ask the drive itself what it is, not the label.  The door is the
 * block switch's ioctl (ideioctl, ncrioctl), which passes a raw command
 * block to the drive and moves its data phase.  For the ide card the
 * block is IDENTIFY DEVICE, which answers with the geometry and the LBA
 * count; for the scsi adapter it is READ CAPACITY, which answers with the
 * last LBA and the block size.  The LBA count is what a label's geometry
 * should multiply out to.
 */
inquire()
{
	struct stat st;
	struct cdb r;
	unsigned long cyls, heads, spt, nblk, bsize;
	int i;

	if (fstat(devfd, (char *) &st) < 0)
		die("cannot stat the device");

	switch (st.st_addr[0] >> 8) {
	case IDEMAJ:
		r.cmd[0] = 0xe0;	/* DRVHD: master, LBA mode */
		r.cmd[1] = 0;		/* features */
		r.cmd[2] = 1;		/* sector count */
		r.cmd[3] = 0;		/* LBA 0-7 */
		r.cmd[4] = 0;		/* LBA 8-15 */
		r.cmd[5] = 0;		/* LBA 16-23 */
		r.cmd[6] = 0xec;	/* IDENTIFY DEVICE */
		r.len = 7;
		r.count = 512;
		r.flags = CDB_IN;
		r.buf = (char *) idbuf;
		r.hole = -1;
		if (ioctl(devfd, CDBCMD, &r) < 0)
			die("identify device failed");
		cyls = idbuf[2] | (idbuf[3] << 8);
		heads = idbuf[6] | (idbuf[7] << 8);
		spt = idbuf[12] | (idbuf[13] << 8);
		nblk = idbuf[120] | (idbuf[121] << 8) |
			((unsigned long) idbuf[122] << 16) |
			((unsigned long) idbuf[123] << 24);
		printf("%s: %lu cylinders, %lu heads, %lu sectors/track, "
			"%lu blocks of 512 bytes\n",
			devname, cyls, heads, spt, nblk);
		break;

	case NCRMAJ:
		for (i = 0; i < 12; i++)
			r.cmd[i] = 0;
		r.cmd[0] = 0x25;	/* READ CAPACITY */
		r.len = 10;
		r.count = 8;
		r.flags = CDB_IN;
		r.buf = (char *) capbuf;
		r.hole = -1;
		if (ioctl(devfd, CDBCMD, &r) < 0)
			die("read capacity failed");
		nblk = ((unsigned long) capbuf[0] << 24) |
			((unsigned long) capbuf[1] << 16) |
			((unsigned long) capbuf[2] << 8) | capbuf[3];
		nblk++;			/* the last LBA, plus one */
		bsize = ((unsigned long) capbuf[4] << 24) |
			((unsigned long) capbuf[5] << 16) |
			((unsigned long) capbuf[6] << 8) | capbuf[7];
		printf("%s: %lu blocks of %lu bytes\n", devname, nblk, bsize);
		break;

	default:
		die("this drive has no inquiry command");
	}
}

main(argc, argv)
	int argc;
	char **argv;
{
	int i, asked, inq;
	UINT sl, off, len;

	pname = argv[0];
	asked = 0;
	inq = 0;

	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-w") == 0) {
			if (wgeom++ || ++i >= argc || !getgeom(argv[i]))
				usage();
			if (++i >= argc)
				usage();
			devname = argv[i];
		} else if (strcmp(argv[i], "-s") == 0) {
			if (i + 3 >= argc || nsets >= NSLICE)
				usage();
			devname = argv[++i];
			if (!getslice(argv[++i], &sl))
				usage();
			if (!getnum(argv[++i], &off))
				usage();
			len = 0;
			if (i + 1 < argc && argv[i + 1][0] >= '0' &&
			    argv[i + 1][0] <= '9') {
				if (!getnum(argv[++i], &len))
					usage();
				/*
				 * An end cylinder, not a length: what was
				 * asked for is "from here to there", and the
				 * two are only the same when the slice is one
				 * cylinder long.
				 */
				if (len < off)
					die("the end cylinder is before the start");
				len = len - off + 1;
			}
			sets[nsets].so_sl = sl;
			sets[nsets].so_off = off;
			sets[nsets].so_len = len;
			nsets++;
		} else if (strcmp(argv[i], "-i") == 0) {
			if (inq++ || ++i >= argc)
				usage();
			devname = argv[i];
		} else if (argv[i][0] == '-') {
			usage();
		} else if (devname == 0) {
			devname = argv[i];
		} else {
			usage();
		}
	}
	if (devname == 0)
		usage();
	if (wgeom || nsets)
		asked = 1;

	if ((devfd = open(devname, O_RDWR)) < 0) {
		fprintf(stderr, "%s: %s: cannot open, error %d\n",
			pname, devname, errno);
		/*
		 * A drive gets one mapping, kept per drive and not per
		 * open, so the live root filesystem holds the only one
		 * there is.
		 */
		if (errno == EBUSY)
			fprintf(stderr, "%s: %s: drive is already open\n",
				pname, devname);
		exit(1);
	}

	/*
	 * An inquiry is a read too, and the one that does not touch the label
	 * sector at all: the drive answers from its own hardware.
	 */
	if (inq) {
		inquire();
		exit(0);
	}

	/*
	 * A dump is a read: it reports what is there, including no label at
	 * all, which is a thing to know and an error to a script.
	 */
	if (!asked) {
		readsector();
		if (!islabel()) {
			fprintf(stderr, "%s: %s: no %s label\n",
				pname, devname, DL_MAGIC);
			exit(1);
		}
		dump();
		exit(0);
	}

	readsector();
	checkwhole();
	if (wgeom)
		writegeom();
	if (nsets && !islabel())
		die("no label to put a slice in: -w the geometry first");
	for (i = 0; i < nsets; i++)
		writeslice(&sets[i]);

	writesector();

	/*
	 * Read it back from the drive and report that, not the buffer that
	 * was just written: what matters is what the disk now says.
	 */
	readsector();
	dump();
	exit(0);
}
