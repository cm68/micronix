/*
 * dar - exchange files with a DOS diskette
 *
 * cmd/dar/dar.c
 *
 * Reconstructed from /bin/dar off the Micronix 1.67 patch
 * (disks/Late/mw1.67_patch/bin/dar), a Whitesmith's binary with no
 * source.  dar.dist is that binary, dar.dis is disas's output, and
 * dar.ctl names what has been read out of it so far - this file is
 * written against those names and README records the shape of the
 * program.
 *
 * dar reads an IBM/DOS-format floppy in a Micronix drive and moves
 * files between the two filesystems.  usage:
 *
 *	dar device {cdprtx} files ...
 *
 * c copies FROM the DOS diskette, p copies TO it, and d, r, t, x are
 * the single-file operations, of which at most one may be given; v is
 * verbose.  With no operation flag dar runs its menu.  The command
 * line is parsed by doargs() into one flag cell per operation, and
 * execute() runs whatever was flagged.
 *
 * The storage stack, named top to bottom in dar.ctl:
 *
 *	menu / execute / doput / docopy / mkfile / space / format
 *	  -> readfat / writefat / getsec / putsec       (sector access)
 *	    -> readsector / writesector / seeksec / readboot  (512-byte)
 *	      -> read / write / seek / open / close    (marshalling)
 *	        -> the data-segment syscall stubs
 *
 * This is a skeleton: doargs and the option dispatch are read out of
 * the disassembly; execute and everything below it are stubs that
 * still need the DOS directory-entry and FAT layout read back before
 * they can be filled in.
 *
 * vim: tabstop=4 shiftwidth=4 noexpandtab:
 */
#include <stdio.h>
#include <types.h>
#include <string.h>
#include <sys/fs.h>
#include <sys/stat.h>
#include <unistd.h>

/*
 * The operation flags, one 16-bit cell per option letter.  doargs sets
 * exactly one of the first six; v is independent.
 */
int	copyflag;	/* c - copy FROM the DOS diskette */
int	delflag;	/* d - delete a DOS file */
int	putflag;	/* p - copy TO the DOS diskette */
int	renflag;	/* r - rename a DOS file */
int	touchflag;	/* t - touch a DOS file */
int	xflag;		/* x - extract */
int	verboseflag;	/* v - the space report */

char	*devname;	/* the device argument, argv[1] */

/*
 * The DOS diskette geometry, read off the boot sector, and the buffers
 * the sector layer reads and writes into.  Sizes and offsets are still
 * to be read back out of the data segment; these are placeholders.
 */
int	devsize;	/* the 4-byte device size */
int	doneflag;	/* H87d1 - set when a final step is due */
char	*name;		/* the file name the operations act on */

/*
 * The geometry descriptor - one 34-byte entry of the formats table at
 * 0x9e44, matched by format() and pointed to by geom.  The 17 words, in
 * order, read from the table's own entries:
 */
struct geom {
	int	signature;	/* 0: tracks * sectors-per-track (320/360/640/720) */
	int	spc;		/* 2: sectors per cluster */
	int	reserved;	/* 4: reserved sectors */
	int	spt;		/* 6: sectors per track */
	int	heads;		/* 8: 8 or 9 */
	int	bytes_per_sec;	/* 10: 512 */
	int	total_clusters;	/* 12 */
	int	cluster_size;	/* 14: bytes per cluster (512 or 1024) */
	int	num_fats;	/* 16: 1 or 2 */
	int	root_entries;	/* 18: 16 or 32 */
	int	field20;	/* 20: 64 or 112 */
	int	field22;	/* 22: 4 or 7 */
	int	field24;	/* 24: 1 or 2 */
	int	media;		/* 26: media descriptor */
	int	field28;	/* 28: 3 or 5 */
	int	field30;	/* 30: 7/9/10/12 */
	int	field32;	/* 32: 1 or 2 */
};

struct geom *geom;	/* 0x9e42 - the matched format descriptor */
unsigned char *fatbuf;	/* 0x8bdc - the FAT image */
int	devfd;		/* the opened device */
unsigned char sectorbuf[512];	/* 0xa018 - the sector buffer getsec fills */

/*
 * The DOS directory entry, the standard 32-byte layout (confirmed by the
 * code: stride 0x20, attribute at 0x0b, first cluster at 0x1a).
 */
struct dirent {
	char	name[11];	/* 0-10: filename (8) + extension (3) */
	char	attr;		/* 11: attribute */
	char	reserved[10];	/* 12-21 */
	int	time;		/* 22-23 */
	int	date;		/* 24-25 */
	int	cluster;	/* 26-27: first cluster */
	long	size;		/* 28-31: file size */
};

/*
 * fatentry - read one 12-bit FAT entry for a cluster.  Entry n sits at
 * byte n*3/2; an even cluster takes the low 12 bits, an odd one the
 * high 12.  (The disassembly masks with AND 0xff / AND 0x0f; this is
 * the same read, still to be byte-checked against H36b0.)
 */
fatentry(cluster)
int cluster;
{
	register int offset, word;

	offset = cluster + (cluster >> 1);	/* n * 3 / 2 */
	word = fatbuf[offset] | (fatbuf[offset + 1] << 8);
	return cluster & 1 ? word >> 4 : word & 0xfff;
}

/*
 * say - print one message.  The disassembly's H03c1; every prompt and
 * diagnostic goes through it, and the strings already carry their own
 * newline where they want one.
 */
say(s)
char *s;
{
	printf("%s", s);
}

main(argc, argv)
char **argv;
{
	doargs(argc, argv);
	execute();
	if (verboseflag)
		space();
	exit(1);
}

/*
 * doargs - walk the command line.  With no arguments at all dar runs
 * its menu; otherwise argv[1] is the device, argv[2] the operation
 * letters, and argv[3..] the file names.
 */
doargs(argc, argv)
char **argv;
{
	if (argc == 1)
		menu();
	if (argc > 1)
		devname = argv[1];
	if (argc > 2)
		optscan(argv[2]);
	/* TODO: argc > 3 - the file names, argv[3..] */
}

/*
 * optscan - run one operation letter through the switch that doargs
 * decoded (the table at 0x0261).  Each handler sets its flag cell.
 */
optscan(s)
char *s;
{
	register char *p;

	for (p = s; *p; p++) {
		switch (*p) {
		case 'c':
			copyflag = 1;
			break;
		case 'd':
			delflag = 1;
			break;
		case 'p':
			putflag = 1;
			break;
		case 'r':
			renflag = 1;
			break;
		case 't':
			touchflag = 1;
			break;
		case 'x':
			xflag = 1;
			break;
		case 'v':
			verboseflag = 1;
			break;
		case '-':
			break;
		case '?':
			usage();
		default:
			badopt();
		}
	}
}

/*
 * usage / badopt - both print "usage: dar device {cdprtx} files ..."
 * and exit; badopt is what an unrecognized letter falls through to.
 */
usage()
{
	printf("usage: dar device {cdprtx} files ...\n");
	printf("Example: dar mfa xv\n");
	exit(2);
}

badopt()
{
	printf("usage: dar device {cdprtx} files ...\n");
	printf("Example: dar mfa xv\n");
	exit(2);
}

/*
 * Below here are the operations.  execute() is the driver that reads the
 * flag cells and runs the flagged operation; the dispatch below is read
 * straight out of the disassembly (0x03ed): check the device, build the
 * /dev name, detect the format, read the FAT, then run exactly one of
 * delete / rename / touch / extract-or-put.  The copy flag (c) is acted
 * on inside readfat, not here.
 */
execute()
{
	if (!checkdev())
		badopt();
	mkdevname();
	dispatch();
	format();
	readfat();
	if (delflag)
		dodelete();
	else if (renflag)
		dorename();
	else if (touchflag)
		dotouch();
	else if (xflag || putflag)
		doextract();
	if (doneflag)
		finish();
}

/*
 * space - the -v report.  Walks the FAT from cluster 2 to the total,
 * counting the entries that read back free, then prints
 * "Diskette <n>% full, <k>K free space left".
 */
space()
{
	register int free, cluster, total;

	free = 0;
	for (cluster = 2; cluster < geom->total_clusters; cluster++)
		if (fatentry(cluster) == 0)
			free++;
	total = geom->total_clusters - 2;

	printf("Diskette ");
	printnum((total - free) * 100 / total);
	printf("%% full, ");
	printnum(free * geom->cluster_size / 1024);
	printf("K free space left\n");
}

/*
 * printnum - the number formatter say()'s numeric arguments run through
 * (H0985/H09ae in the disassembly).
 */
printnum(n)
int n;
{
	printf("%d", n);
}

checkdev()
{
	struct stat st;

	if (stat(devname, &st) < 0) {
		printf("%s", devname);
		return 0;
	}
	if ((st.st_mode & S_IFBLK) != S_IFBLK) {	/* not a block device */
		say(devname);
		say(": Not block special\n");
		return 0;
	}
	return 1;
}

/*
 * mkdevname - find the device in /etc/mtab and build its /dev path.
 * H1106.  Opens /etc/mtab, reads it 64 bytes at a time, and for each
 * entry builds "/dev/<name>", stat's it, and - when it is the block
 * device the caller asked for - prints the name and exits.  The exact
 * mtab field layout and the match field are still to be read back;
 * this captures the shape.
 */
mkdevname()
{
	int fd;
	char mtab[64];
	char path[80];
	struct stat st;

	fd = open("/etc/mtab", 0);
	if (fd < 0)
		return;

	for (;;) {
		if (read(fd, mtab, sizeof mtab) != sizeof mtab) {
			close(fd);
			return;
		}
		if (mtab[0] == 0)
			continue;
		/* TODO: the mtab entry is not just a bare name; split the real
		 * device name out of it and match against the caller's geometry.
		 * H6cf2 builds the path as "/dev/%s" of that name. */
		sprintf(path, "/dev/%s", mtab);
		if (stat(path, &st) == 0
		    && (st.st_mode & S_IFBLK) == S_IFBLK) {
			printf("%s", mtab);
			exit(0);
		}
	}
}

dispatch()
{
	/*
	 * H1214/H1226.  Open the device with the right mode: delete/rename
	 * need read-write (2), the read operations read-only (0).  The
	 * result (H8bdb) drives the retry/error path.
	 */
	int mode;

	mode = (delflag || renflag) ? 2 : 0;
	devfd = open(devname, mode);
	/* TODO: retry once, then printf(devname) + error on failure */
}

/*
 * The formats table at 0x9e44 - the known DOS diskette geometries, 17
 * words each, zero-terminated.  The signature word is tracks *
 * sectors-per-track: 320/360 are the 40-track 5.25" disks (8/9 sectors),
 * 640/720 the 80-track ones.
 */
int	formats[] = {
	320, 5, 1, 8, 8, 512, 315, 512, 1, 16, 64, 4, 1, 254, 3, 7, 1,
	360, 5, 1, 9, 9, 512, 353, 512, 1, 16, 64, 4, 2, 252, 5, 9, 2,
	360, 5, 1, 8, 9, 512, 315, 512, 1, 16, 64, 4, 1, 254, 3, 7, 1,
	640, 5, 2, 8, 8, 512, 316, 1024, 2, 32, 112, 7, 1, 255, 3, 10, 1,
	720, 5, 2, 9, 9, 512, 355, 1024, 2, 32, 112, 7, 2, 253, 5, 12, 2,
	720, 5, 2, 8, 9, 512, 316, 1024, 2, 32, 112, 7, 1, 255, 3, 10, 1,
	0
};

seeksec(fd, sector)
int fd, sector;
{
	/* whence 3 is the kernel's "sector" seek - it multiplies by 512 */
	seek(fd, sector, 3);
}

readsector(fd, buf)
int fd;
char *buf;
{
	read(fd, buf, 512);
}

writesector(fd, buf)
int fd;
char *buf;
{
	write(fd, buf, 512);
}

/*
 * getstr / putstr - the string layer.  dar builds a line-oriented text
 * file over the raw sectors: getstr pumps bytes out of the sector buffer
 * (refilling with readsector when the counter underflows) into buf until
 * 0xff (end of file) or 0x0a (end of line), and putstr writes a line
 * back, flushing 0x0a-terminated sectors.  The counters and the exact
 * flush flag are still approximate.
 */
int	sectorcount;	/* bytes left in the sector buffer */
unsigned char *sectorptr;	/* next byte in the sector buffer */
char	outbuf[512];	/* 0xa028 - the output line buffer */
int	outcount;	/* bytes left in outbuf */
int	outflag;	/* the flush flag (0xa02e bit 0x40) */

char *
getstr(buf)
char *buf;
{
	char *p;
	int c;

	p = buf;
	for (;;) {
		if (--sectorcount < 0) {
			readsector(devfd, sectorbuf);
			sectorptr = sectorbuf;
			sectorcount = 512;
		}
		c = *sectorptr++;
		if (c == 0xff) {	/* end of file */
			*p = '\0';
			if (p == buf)
				return 0;
			return buf;
		}
		if (c == '\n') {	/* end of line */
			*p = '\0';
			return buf;
		}
		*p++ = c;
	}
}

putstr(s)
char *s;
{
	strcpy(outbuf, s);
	if (--outcount < 0) {
		*outbuf = '\n';
		writesector(devfd, outbuf);
		outcount = 512;
	}
	if (outflag)
		writesector(devfd, outbuf);
}

readboot()
{
	/*
	 * H13bf.  Seek to sector 0, read the boot sector, and return the
	 * geometry signature format() matches against - the bytes-per-sector
	 * field (a word at offset 11 of the boot sector).  The device fd and
	 * the sector buffer it reads into are still to be placed.
	 */
	int bps;

	seeksec(devfd, 0);
	readsector(devfd, sectorbuf);
	bps = sectorbuf[11] | (sectorbuf[12] << 8);
	return bps;
}

/*
 * readfat - H47ee.  Loads the FAT image, and when copyflag (c) is set
 * runs the copy-from-DOS operation.  The geometry fields it touches are
 * offset 8 (sectors-per-FAT) and offset 26 (media descriptor); the FAT
 * image is fatbuf, read into at 0x400 a sector.  Still to be filled in
 * from the disassembly: the exact H48ca FAT read and the copy loop.
 */
readfat()
{
	if (copyflag) {
		memset(fatbuf, 0, 0x400);
		/* TODO: media descriptor (geom[26]) into fatbuf[0], the FAT
		 * read, and the copy-from-DOS loop; sets doneflag = 1 */
		doneflag = 1;
		return;
	}
	/* TODO: H48ca - read the FAT and verify the media descriptor */
}

/*
 * format - H38d2.  Match the device geometry (readboot) against the
 * formats table; the matched entry becomes geom.  ": Unknown format"
 * and exit when none matches.
 */
format()
{
	int *p;

	for (p = formats; *p != 0; p += 17) {
		if (*p == readboot()) {
			geom = (struct geom *)p;
			return;
		}
	}
	say(": Unknown format\n");
	exit(0);
}

/*
 * The operations.  Each follows the same shape: findfile the name,
 * notfound if it is absent, then do the operation.  The directory/FAT
 * helpers below are the named leaf functions from dar.ctl; their bodies
 * are the last thing to fill in.
 */
notfound()
{
	say(name);
	say(": No such file or directory\n");
}

dodelete()
{
	if (!findfile(name))
		notfound();
	delfile(name);		/* mark the entry 0xe5 */
	findfree();
	addentry();
}

dorename()
{
	/* TODO: the new name is the next argument; findfile + rewrite */
}

dotouch()
{
	if (!findfile(name))
		notfound();
	/* TODO: refresh the entry's time/date */
}

doextract()
{
	if (!findfile(name))
		notfound();
	/* TODO: the copy loop through the FAT chain */
}

finish()
{
	clustosec();
	/* TODO: the rest of the finalize */
}

/*
 * The directory/FAT leaf helpers, named in dar.ctl but not yet read
 * back into C.  Each is a stub until its body is reconstructed.
 */
findfile(f)
char *f;
{
	/* H1fda - walk the directory, match name[11].  The exact compare
	 * (H6c47) pads and case-folds; this is the shape. */
	struct dirent *de;
	int i;

	de = (struct dirent *)sectorbuf;
	for (i = 0; i < 16; i++, de++)
		if (de->name[0] != 0x00 && strncmp(de->name, f, 11) == 0)
			return 1;
	return 0;
}

findfree()
{
	/* H311a - the first free (0x00) or deleted (0xe5) entry. */
	struct dirent *de;
	int i;

	de = (struct dirent *)sectorbuf;
	for (i = 0; i < 16; i++, de++)
		if (de->name[0] == 0x00 || de->name[0] == 0xe5)
			return i;
	return -1;
}

addentry()
{
	/*
	 * H32be - write a directory entry.  findfile guards against a
	 * duplicate; then the name is copied in and the entry's
	 * time/date/cluster/size are filled before it is written back.
	 */
	struct dirent *de;

	if (findfile(name))
		return 0;	/* duplicate name */
	de = (struct dirent *)sectorbuf;
	strcpy(de->name, name);
	/* TODO: fill de->time/de->date (dectime), de->cluster, de->size,
	 * then writesector back out. */
	return 1;
}

delfile(f)
char *f;
{
	/* H20d0 - mark the file's entry deleted: name[0] = 0xe5. */
	struct dirent *de;
	int i;

	de = (struct dirent *)sectorbuf;
	for (i = 0; i < 16; i++, de++)
		if (de->name[0] != 0x00 && strncmp(de->name, f, 11) == 0) {
			de->name[0] = 0xe5;
			return 1;
		}
	return 0;
}

/*
 * clustosec - the cluster -> sector conversion: sector = data_start +
 * (cluster - 2) * sectors_per_cluster.  data_start is past the reserved
 * sectors, the FATs, and the root directory.  The disassembly's shift
 * path (H7d6e/H7d53) is the same arithmetic; the exact round-up still
 * to be byte-checked.
 */
clustosec(cluster)
int cluster;
{
	int spt_fat, root_sectors, data_start;

	spt_fat = (geom->total_clusters * 3 / 2 + geom->bytes_per_sec - 1)
			/ geom->bytes_per_sec;
	root_sectors = geom->root_entries * 32 / geom->bytes_per_sec;
	data_start = geom->reserved + geom->num_fats * spt_fat + root_sectors;
	return data_start + (cluster - 2) * geom->spc;
}

/*
 * nextcluster / alloccluster / readcluster - the FAT chain walk.  next
 * is fatentry(cluster); alloc returns the existing link or finds a free
 * cluster; read seeks and reads the cluster's sector.
 */
nextcluster(cluster)
int cluster;
{
	return fatentry(cluster);
}

alloccluster(cluster)
int cluster;
{
	int next;

	next = fatentry(cluster);
	if (next != 0 && next < 0xff8)
		return next;	/* already linked */
	/* TODO: scan the FAT for a free (0) cluster and link it */
	return 0;
}

readcluster(cluster, buf)
int cluster;
char *buf;
{
	seeksec(devfd, clustosec(cluster));
	readsector(devfd, buf);
	return 0;
}

readdir()
{
	/*
	 * H0cb4 - walk the directory.  It takes a target, a table of 2-byte
	 * values, and a count, and iterates: H3e11 tests each entry, H5ce2
	 * advances to the next.  The table is the directory's cluster/entry
	 * pointers; the exact test and advance are still to be read back.
	 */
	return 0;
}

getdiskname()
{
	/*
	 * H16aa.  Prompt for the disk, read the name back, and verify the
	 * device with checkdev, retrying until it is a block device.
	 */
	for (;;) {
		say("Insert diskette and close the drive door\n");
		say("Enter disk name: ");
		/* TODO: the name read (stdin, not the sector buffer) */
		if (checkdev())
			return;
	}
}

menu()
{
	/* H0ab8 - the interactive loop.  Print the menu, read a selection,
	 * and run the chosen operation until D (quit). */
	for (;;) {
		say("\tA) Copy from the DOS diskette\n");
		say("\tB) Copy to the DOS diskette\n");
		say("\tC) Display file on the DOS diskette\n");
		say("\tD) Quit\n");
		say("Selection: ");
		/* TODO: read the selection, then A->copyflag, B->putflag,
		 * C->touchflag, D->return; then execute() */
	}
}
