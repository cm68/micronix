/*
 * upm_res.c - the resident half of upm.  Built with -Stext and linked
 * into the text segment; upm.h says why the halves are split.
 *
 * The resident half is two units: this one ends where upm_res2.c begins,
 * a boundary the machine's compiler forces (see upm_res2.c).
 */

#include "upm.h"


/*
 * The globals the small handlers touch.  Addresses are from the .dis and
 * are the one thing here that cannot be wrong: they are read out of the
 * binary, not recovered.
 *
 * They live in the text segment, above the CP/M TPA, not in the data
 * segment - the linker laid upm's own state up with its code, and left
 * the bottom of memory to the CCP and the .com it loads.
 *
 * uchar and ushort are Whitesmith's, from <hitech.h> via <types.h>.
 */

uchar	lstdesc;	/* ee83 - the list device descriptor, 1 = console */
char	*lstdev;		/* ee84 - the list device path */
uchar	*op;		/* ee1b - the console output pointer */
uchar	*ip;		/* ee1d - the console input pointer */
uchar	*lp;		/* ee1f - the list output pointer */
uchar	obuf[32];	/* edcb - the console output buffer */
uchar	ibuf[16];	/* edeb - the console input buffer, to lbuf */
uchar	lbuf[32];	/* edfb - the list output buffer */
uchar	sgtty[6];	/* ef0a - a struct sgtty, to dirbuf */
ushort	dma;		/* ee86 - the CP/M DMA address */
uchar	iobyte;	/* ee88 - the CP/M I/O byte */
uchar	call;		/* ee89 - the BDOS function number, from cpm */
ushort	col;		/* ee97 - console column, a word */
uchar	ntab;		/* ee8a - the number of remembered tab stops */
uchar	tabs[12];	/* ee8b - the columns of those tab stops */
ushort	logvector;	/* ee99 - logged-in drive vector */
uchar	user;		/* ee9b - the CP/M user number */
ushort	rovector;	/* ee9f - read-only drive vector */
uchar	curdrive;	/* eea1 - current drive */
uchar	ileft;		/* eea3 - input chars left in the console buffer */
uchar	lleft;		/* eea4 - list output chars left */
uchar	oleft;		/* eea5 - output chars left */
uchar	recavail;	/* edc5 - a character is waiting */
ushort	verbose;	/* edc9 - the -v flag, tested as a word */
uchar	loaded;	/* edc7 - a program is loaded */

/*
 * errno lives here, not in libc, so it is resident: the syscall stubs
 * write it while a .com runs, and libc's errno.o would put it in the
 * data segment at 0x0100, where the .com clobbers it.
 */
int	errno;

/*
 * cpm's own argument cell.  cpm stores the function's argument here
 * because the small handlers read it from the global rather than from a
 * frame.
 */
ushort	arg2;		/* f047 - the BDOS argument, from cpm */

/*
 * The deferred last record.  A CP/M write is a full 128-byte record, but
 * the CP/M 3 byte-count extension (set file attributes with F6' in s2)
 * lets the final record be short.  Rather than write every record and then
 * truncate the micronix file back down, upm holds the most recent record
 * back and writes it only on the next record write or on close - at close
 * with just the byte-count bytes.  pending is a copy of the DMA record,
 * because the program is free to reuse its DMA buffer as soon as the write
 * returns.
 */
char	pending[128];		/* the deferred record */
char	pending_valid;		/* 1 when pending holds a record */
struct fcb *pending_fcb;	/* the FCB the deferred record belongs to */
ushort	pending_rec;		/* its record number */

void	flushpending();
void	flushbyte();

/*
 * The CCP's own.  loadfile is the program the CCP decided to run, or 0
 * to sit in the prompt; ccword and line are getword's and getline's
 * buffers.
 */
char	*loadfile;		/* ee61 */

/*
 * cpm - the BDOS dispatcher.
 *
 * entry calls cpm(func, arg) with the function number and the
 * argument the CP/M program passed in C and DE.  It stores both, prints
 * the function number under -v, then jumps to the handler with the
 * argument on the stack, so that a frame-using handler finds it at DE+4
 * and a small handler finds it in arg2.
 *
 * The forty-way dispatch is a switch.  ccc turns a dense switch into the
 * forty-word jump table the binary keeps as cpmtab, so writing it this
 * way is the reconstruction of the source, not a transcription of the
 * object.  The frame-using handlers take the argument; the small ones
 * read arg2.
 */
cpm(func, arg)
int func;
int arg;
{
	call = func;

	if (verbose) {
		putch(func + 'A');
		cflush();
	}

	arg2 = arg;

	switch (func) {
	case 0:		return cexit();
	case 1:		return cconin();
	case 2:		return echo(arg);
	case 3:		return getch();
	case 4:		return putch(arg);
	case 5:		return clist();
	case 6:		return cdirio();
	case 7:		return cgetio();
	case 8:		return csetio();
	case 9:		return cprs();
	case 10:	return readbuf(arg);
	case 11:	return cconstat();
	case 12:	return cversion();
	case 13:	return creset();
	case 14:	return select();
	case 15:	return copen(arg);
	case 16:	return cclose();
	case 17:	return cfirst();
	case 18:	return cnext();
	case 19:	return delete(arg);
	case 20:	return rseq();
	case 21:	return wseq();
	case 22:	return cmake(arg);
	case 23:	return rename(arg);
	case 24:	return clogin();
	case 25:	return ccurdisk();
	case 26:	return cdma();
	case 27:	return null();
	case 28:	return cprotect();
	case 29:	return cgetro();
	case 30:	return setatt(arg);
	case 31:	return getparm();
	case 32:	return cuser();
	case 33:	return rrand();
	case 34:	return wrand();
	case 35:	return csize();
	case 36:	return setrand();
	}
	return null();
}

/*
 * The small BDOS handlers.  These do not take a frame - they read arg2
 * and return in BC - so they are reproduced as plain functions.
 */

/*
 * null - the unknown-function handler.  Returns 0.
 */
null()
{
	return 0;
}

/*
 * getparm - function 31, get the disk parameter block address.
 *
 * upm has no physical CP/M disk, but CP/M programs probe the DPB to size
 * their own buffers, so return a fixed, plausible one.
 */
struct dpb {
	int	spt;		/* sectors per track */
	char	bsh;		/* block shift factor */
	char	blm;		/* block shift mask */
	char	exm;		/* extent mask */
	int	dsm;		/* disk storage max */
	int	drm;		/* directory entry max */
	char	al0;
	char	al1;
	int	cks;		/* dir check vector size */
	int	off;		/* disk offset */
};

struct dpb dpb = {
	26, 3, 7, 0, 242, 63, 32, 0, 16, 2
};

getparm()
{
	return &dpb;
}

/*
 * cdma - function 26, set DMA address.  Returns the address it set.
 */
cdma()
{
	dma = arg2;
	return arg2;
}

/*
 * cconin - function 1, console input.  Reads a character and echoes it.
 */
cconin()
{
	uchar c;

	c = getch();
	echo(c);
	return c;
}

/*
 * clist - function 5, list output.  To the list device unless the list
 * descriptor says the console.
 */
clist()
{
	if (lstdesc == 1)
		putch(arg2);
	else
		lput(arg2);
}

/*
 * cgetio - function 7, get the I/O byte.
 */
cgetio()
{
	return iobyte;
}

/*
 * csetio - function 8, set the I/O byte.
 */
csetio()
{
	iobyte = arg2;
}

/*
 * cversion - function 12.  Claims CP/M 2.2, which is what upm is.
 */
cversion()
{
	return 0x22;
}

/*
 * creset - function 13, reset the disk system.  Restores the DMA
 * address to the default 80h and clears the read-only vector.
 */
creset()
{
	dma = 0x80;
	rovector = 0;
}

/*
 * cfirst / cnext - functions 17 and 18, search first and next.  Both
 * hand off to search with the search function code and the FCB; the DMA
 * address is where the directory entry is written.
 */
cfirst()
{
	return search(0x11, arg2, dma);
}

cnext()
{
	return search(0x12, arg2, dma);
}

/*
 * clogin - function 24, the logged-in drive vector.
 */
clogin()
{
	return logvector;
}

/*
 * ccurdisk - function 25, the current disk.
 */
ccurdisk()
{
	return curdrive;
}

/*
 * cgetro - function 29, the read-only drive vector.
 */
cgetro()
{
	return rovector;
}

/*
 * cuser - function 32, get or set the user number.  FF is the query.
 */
cuser()
{
	uchar u;

	u = user;
	if ((arg2 & 0xFF) != 0xFF) {
		u = arg2 & 0x1F;
		user = u;
	}
	return u;
}

/*
 * cconstat - function 11, console status.  1 if a character is waiting,
 * else flush output and 0.
 */
cconstat()
{
	if (ileft || recavail)
		return 1;
	if (oleft == 0)
		return 0;
	cflush();
	return 0;
}

/*
 * rseq / wseq / rrand / wrand - functions 20, 21, 33, 34.  The four
 * read/write operations.  53h is 'S' and 52h is 'R', so the first
 * argument to cread/cwrite is the letter of the mode: sequential or
 * random.  arg2 is the FCB.
 */
rseq()
{
	return cread(0x53, arg2);
}

wseq()
{
	return cwrite(0x53, arg2);
}

rrand()
{
	return cread(0x52, arg2);
}

wrand()
{
	return cwrite(0x52, arg2);
}

/*
 * cprotect - function 28, write protect the current disk.  Sets the
 * current drive's bit in the read-only vector.
 */
cprotect()
{
	rovector |= 1 << curdrive;
}

/*
 * setatt - function 30, set file attributes.
 *
 * The FCB's read-only bit in ft[0] decides the write permission: a
 * read-only file loses its write bits, a read-write file gains write
 * wherever it has read.  chmod applies the new mode.
 */
setatt(fcb)
struct fcb *fcb;
{
	int mode;

	if (uniqize(fcb) == 0)
		return -1;		/* no file */

	name(fcb, buf);		/* build the micronix path name */

	if (stat(buf, statbuf) < 0)
		return -1;		/* no such file */

	if (fcb->s2 & 0x40)		/* F6' - set the byte count */
		fcb->bc = fcb->cr;

	mode = statbuf[4] | (statbuf[5] << 8);

	if (fcb->ft[0] & 0x80)		/* read-only */
		mode &= ~0222;		/* take away write permission */
	else				/* read-write */
		mode |= ((mode & 0444) >> 1);	/* write where read */

	if (chmod(buf, mode) < 0)	/* apply the new mode */
		return -1;

	return 0;
}

/*
 * main - the program.  upm has two modes and this is the whole choice:
 * load() runs the program named on the command line (DIRECT mode) and
 * ccp() sits in the CP/M prompt (INTERACTIVE mode).  loadfile is set by
 * init from the argument block the crt passes down, which is how it
 * tells the two apart.
 */
main(argc, argv)
char **argv;
{
	init(argc, argv);

	if (loadfile)
		load();
	else
		ccp();
}

/*
 * cexit - the exit path, and also BDOS function 0, the warm boot.  A
 * CP/M program that has finished jumps to wboot, which comes here; so
 * this is what runs when a .com returns.  It prints the CR that CP/M
 * always leaves after a program, flushes both output devices, restores
 * the terminal tint put in raw mode, and exits.
 */
cexit()
{
	putch('\r');
	cflush();
	lflush();
	trestore();
	_exit();
}

ushort	ro;			/* ee9d - read as a word by copen; the
				   read-only vector is rovector, not this */
char	buf[64];		/* ee21 - the pathname name builds; 64 bytes
				   is the gap to loadfile at ee61 */
char	*disktab[16];		/* ee63 - the sixteen drive directory paths;
				   each is a pointer to the micronix directory
				   that CP/M drive maps onto */
struct fcb fcb;		/* eee6 - the search-result FCB, one of them */

/*
 * copen - BDOS function 15, open file.
 *
 * Turns the FCB's CP/M name into a micronix pathname (via uniqize and
 * name), sizes the file to know how many records it holds, checks the
 * requested extent is inside it, opens the file, and records the
 * descriptor in the FCB.  Returns 0, or -1 on any failure.
 */
copen(fcb)
struct fcb *fcb;
{
	int fd;
	int nrec;
	int size;

	if (uniqize(fcb) == 0)
		return -1;

	name(fcb, buf);
	size = fsize(buf);
	fcb->size = size;
	fcb->bc = statbuf[10] & 0x7F;	/* the last record's byte count */

	nrec = size >> 7;		/* 128-byte records */
	if (size & 0x7F)
		nrec++;
	fcb->nrec = nrec;

	if (fcb->ex != 0 && fcb->ex >= nrec)
		return -1;

	fd = openfile(buf);
	if (fd < 0)
		return -1;

	if (ro)
		fcb->ft[0] |= 0x80;

	fcb->fd = fd;
	fcb->currec = 0;
	setrc(fcb);
	return 0;
}

/*
 * setrc - the record count for the current extent.
 *
 * The rc field is how many records are live in this extent: 0x80 for a
 * full one, or the low seven bits of the size for the last, partial one.
 * The last extent is ex == nrec-1; if the size falls on a record
 * boundary even that one is 0x80.
 */
setrc(fcb)
struct fcb *fcb;
{
	int rc;

	if (fcb->nrec == 0)
		rc = 0;
	else if (fcb->ex == fcb->nrec - 1 && (fcb->size & 0x7F))
		rc = fcb->size & 0x7F;
	else
		rc = 0x80;

	fcb->rc = rc;
}

/*
 * cclose - BDOS function 16, close file.  Closes the micronix file and
 * marks the FCB closed with -1.  The fd is a signed char, so the -1 the
 * byte holds comes back as -1 through closefile.
 */
cclose(fcb)
struct fcb *fcb;
{
	flushbyte(fcb);

	closefile(fcb->fd);
	fcb->fd = -1;
	return 0;
}

/*
 * delete - BDOS function 19, delete file.  Searches the directory for
 * every match of the FCB and unlinks each one that is a plain file.
 * Returns -1 if the first search found nothing, else 0.
 */
delete(pat)
struct fcb *pat;
{
	int rv;
	int fn;

	rv = -1;
	fn = 0x11;			/* search first */
	for (;;) {
		if (search(fn, pat, &fcb) < 0)
			return rv;
		name(&fcb, buf);
		if (!isdir(buf)) {
			closename(buf);
			unlink(buf);
		}
		fn = 0x12;		/* search next */
		rv = 0;
	}
}

/*
 * buildpref - the drive prefix of a pathname.
 *
 * Writes the directory the FCB's drive maps onto, and a slash, into buf,
 * and returns the position after the slash - the place the filename
 * goes.  The drive byte is CP/M's encoding: 0 means the current drive,
 * 1-16 are A-P, and '?' is the same as 0.  A bare drive with no
 * directory in disktab is a selerr.
 */
char *
buildpref(fcb, buf)
struct fcb *fcb;
char *buf;
{
	int drive;

	drive = fcb->dr;
	if (drive == 0)
		drive = curdrive;
	else
		drive--;
	drive &= 0x0F;

	if (disktab[drive] == 0)
		selerr(drive);

	return cpystr(buf, disktab[drive], "/", 0);
}

/*
 * name - the micronix pathname for an FCB.
 *
 * The drive prefix, then the eight-character name, then a dot and the
 * three-character type.  Both are lowercased and stripped to the first
 * space; a slash, which micronix cannot have in a file name, becomes a
 * bar.  The result lands in buf, nul-terminated.
 */
name(fcb, buf)
struct fcb *fcb;
char *buf;
{
	int i;
	int c;

	if (oleft)
		cflush();

	if (fcb->dr == '?')
		fcb->dr = 0;

	buf = buildpref(fcb, buf);

	for (i = 0; i < 8; i++) {
		c = lc(fcb->name[i] & 0x7F);
		if (c == ' ')
			break;
		if (c == '/')
			c = '|';
		*buf++ = c;
	}

	for (i = 0; i < 3; i++) {
		c = lc(fcb->ft[i] & 0x7F);
		if (c == ' ')
			break;
		if (i == 0)
			*buf++ = '.';
		if (c == '/')
			c = '|';
		*buf++ = c;
	}

	*buf = 0;
}

char	dirbuf[18];		/* ef10 - one micronix directory entry (16
				   bytes) and two bytes of match state */
char	statbuf[36];		/* ef22 - a struct stat, 36 bytes to trestore */

/* the open-file table; struct fileent is in upm.h, with upm_res2.c */
struct fileent files[16];	/* eea6 */

/*
 * search - BDOS functions 17 and 18, search first and next.
 *
 * Reads the directory a micronix directory is a flat stream of
 * sixteen-byte entries in - two-byte inode, then the fourteen-char name
 * - and skips the empty entries and the . and .. entries, then matches
 * each name against the pattern copied out of the FCB.  The first match
 * is written to buf with the drive byte restored, and the search carries
 * on from where it left off on the next call, which is what makes 18 a
 * next rather than a second first.  Returns 0, or -1 at end of directory.
 */
search(fn, fcb, dst)
int fn;
struct fcb *fcb;
char *dst;
{
	static char dirfd = -1;		/* Hda58 */
	static char pattern[13];	/* Hda59 */

	if (fn == 0x11) {
		buildpref(fcb, buf);
		close(dirfd);
		dirfd = open(buf, 0);
		memcpy(pattern, fcb, 13);
	}

	for (;;) {
		if (read(dirfd, dirbuf, 16) != 16) {
			close(dirfd);
			dirfd = -1;
			return -1;
		}

		dirbuf[16] = 0;

		if (dirbuf[0] == 0 && dirbuf[1] == 0)
			continue;
		if (strcmp(".", &dirbuf[2]) == 0)
			continue;
		if (strcmp("..", &dirbuf[2]) == 0)
			continue;
		if (match(pattern, dirbuf) == 0)
			continue;

		cname(&dirbuf[2], dst);
		*dst = fcb->dr;
		return 0;
	}
}

