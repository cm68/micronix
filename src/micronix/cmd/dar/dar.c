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

/*
 * The geometry descriptor - one 34-byte entry of the formats table at
 * 0x9e44, matched by format() and pointed to by geom.  Only the fields
 * space() reads have been placed: total_clusters at offset 12 and the
 * bytes-per-cluster at offset 14; the rest is padding until the other
 * fields are read back out of the disassembly.
 */
struct geom {
	int	pad[6];		/* offsets 0..11 */
	int	total_clusters;	/* offset 12 */
	int	cluster_size;	/* offset 14, bytes per cluster */
};

struct geom *geom;	/* 0x9e42 - the matched format descriptor */
unsigned char *fatbuf;	/* 0x8bdc - the FAT image */

/*
 * readfat - read one 12-bit FAT entry for a cluster.  Entry n sits at
 * byte n*3/2; an even cluster takes the low 12 bits, an odd one the
 * high 12.  (The disassembly masks with AND 0xff / AND 0x0f; this is
 * the same read, still to be byte-checked against H36b0.)
 */
readfat(cluster)
int cluster;
{
	register int offset, word;

	offset = cluster + (cluster >> 1);	/* n * 3 / 2 */
	word = fatbuf[offset] | (fatbuf[offset + 1] << 8);
	return cluster & 1 ? word >> 4 : word & 0xfff;
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
	exit(2);
}

badopt()
{
	printf("usage: dar device {cdprtx} files ...\n");
	exit(2);
}

/*
 * Below here are stubs.  execute() is the driver that reads the flag
 * cells and runs the operation; menu() is the interactive loop; the
 * rest is the storage stack.  Each is filled in as the DOS directory
 * entry and FAT layout is read back out of the disassembly.
 */
execute()
{
	if (copyflag)
		docopy();
	if (putflag)
		doput();
	if (delflag)
		dodelete();
	/* TODO: rename, touch, extract */
}

menu()
{
	/* TODO: getdiskname, read the directory, run operations in a loop */
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
		if (readfat(cluster) == 0)
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

docopy()
{
	/* TODO: the copy-FROM-DOS operation */
}

doput()
{
	/* TODO: the copy-TO-DOS operation */
}

dodelete()
{
	/* TODO: delete a DOS file */
}
