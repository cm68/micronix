/*
 * upm_res.c - the resident half of upm.  Built with -Stext and linked
 * into the text segment; upm.h says why the halves are split.
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
ushort	logvect;	/* ee99 - logged-in drive vector */
uchar	user;		/* ee9b - the CP/M user number */
ushort	rovecto;	/* ee9f - read-only drive vector */
uchar	curdriv;	/* eea1 - current drive */
uchar	ileft;		/* eea3 - input chars left in the console buffer */
uchar	lleft;		/* eea4 - list output chars left */
uchar	oleft;		/* eea5 - output chars left */
uchar	recavai;	/* edc5 - a character is waiting */
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
 * The CCP's own.  loadfil is the program the CCP decided to run, or 0
 * to sit in the prompt; ccword and line are getword's and getline's
 * buffers.
 */
char	*loadfil;		/* ee61 */

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
	case 11:	return cconsta();
	case 12:	return cversio();
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
	case 25:	return ccurdis();
	case 26:	return cdma();
	case 27:	return null();
	case 28:	return cprotec();
	case 29:	return cgetro();
	case 30:	return null();
	case 31:	return null();
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
 * cversio - function 12.  Claims CP/M 2.2, which is what upm is.
 */
cversio()
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
	rovecto = 0;
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
	return logvect;
}

/*
 * ccurdis - function 25, the current disk.
 */
ccurdis()
{
	return curdriv;
}

/*
 * cgetro - function 29, the read-only drive vector.
 */
cgetro()
{
	return rovecto;
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
 * cconsta - function 11, console status.  1 if a character is waiting,
 * else flush output and 0.
 */
cconsta()
{
	if (ileft || recavai)
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
 * cprotec - function 28, write protect the current disk.  Sets the
 * current drive's bit in the read-only vector.
 */
cprotec()
{
	rovecto |= 1 << curdriv;
}

/*
 * main - the program.  upm has two modes and this is the whole choice:
 * load() runs the program named on the command line (DIRECT mode) and
 * ccp() sits in the CP/M prompt (INTERACTIVE mode).  loadfil is set by
 * init from the argument block the crt passes down, which is how it
 * tells the two apart.
 */
main(argc, argv)
char **argv;
{
	init(argc, argv);

	if (loadfil)
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
	trestor();
	_exit();
}

ushort	ro;			/* ee9d - read as a word by copen; the
				   read-only vector is rovecto, not this */
char	buf[64];		/* ee21 - the pathname name builds; 64 bytes
				   is the gap to loadfil at ee61 */
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

	nrec = size >> 7;		/* 128-byte records */
	if (size & 0x7F)
		nrec++;
	fcb->nrec = nrec;

	if (fcb->ex != 0 && fcb->ex >= nrec)
		return -1;

	fd = openfil(buf);
	if (fd < 0)
		return -1;

	if (ro)
		fcb->ft[0] |= 0x80;

	fcb->fd = fd;
	fcb->rrec = 0;
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
 * byte holds comes back as -1 through closefi.
 */
cclose(fcb)
struct fcb *fcb;
{
	closefi(fcb->fd);
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
			closena(buf);
			unlink(buf);
		}
		fn = 0x12;		/* search next */
		rv = 0;
	}
}

/*
 * buildpr - the drive prefix of a pathname.
 *
 * Writes the directory the FCB's drive maps onto, and a slash, into buf,
 * and returns the position after the slash - the place the filename
 * goes.  The drive byte is CP/M's encoding: 0 means the current drive,
 * 1-16 are A-P, and '?' is the same as 0.  A bare drive with no
 * directory in disktab is a selerr.
 */
char *
buildpr(fcb, buf)
struct fcb *fcb;
char *buf;
{
	int drive;

	drive = fcb->dr;
	if (drive == 0)
		drive = curdriv;
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

	buf = buildpr(fcb, buf);

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
char	statbuf[36];		/* ef22 - a struct stat, 36 bytes to trestor */

/*
 * The open-file table.  One four-byte entry per descriptor: the device
 * and inode the descriptor is on, read out of struct stat when the file
 * was opened.  closena matches a name by stat'ing it and looking for
 * its dev/inode here.
 */
struct fileent {
	ushort	dev;
	ushort	ino;
} files[16];			/* eea6 */

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
		buildpr(fcb, buf);
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

/*
 * cname - a micronix name into the FCB's name and type.
 *
 * The name up to the dot goes into the eight-byte name field, the three
 * letters after it into the type field, both uppercased and space-padded.
 * The drive byte is left alone - search writes it after this returns.
 */
cname(src, dst)
char *src;
char *dst;
{
	int i;
	char *dot;

	dot = src + strlen(src) - 1;
	while (dot >= src && *dot != '.')
		dot--;
	if (dot < src)
		dot = 0;

	memset(dst + 1, ' ', 11);

	i = 0;
	while (i < 8 && *src && src != dot) {
		dst[1 + i] = uc(*src & 0x7F);
		src++;
		i++;
	}

	if (dot) {
		src = dot + 1;
		i = 0;
		while (i < 3 && *src) {
			dst[9 + i] = uc(*src & 0x7F);
			src++;
			i++;
		}
	}
}

/*
 * lc / uc - the two case folds.  Whitesmith's has both, and upm calls
 * them by name, so they are reproduced as the two functions they are.
 */
lc(c)
int c;
{
	if (c >= 'A' && c <= 'Z')
		return c + 0x20;
	return c;
}

uc(c)
int c;
{
	if (c >= 'a' && c <= 'z')
		return c - 0x20;
	return c;
}

/*
 * clean - strip the high bit off every byte up to the nul.
 *
 * The CP/M R/O attribute and the wildcard marker ride the high bit of
 * the name and type characters, so they are cleared before the name is
 * used as a micronix pathname.
 */
clean(p)
char *p;
{
	while (*p) {
		*p &= 0x7F;
		p++;
	}
}

/*
 * isuniqu - does the FCB's name hold no '?' wildcard.  The thirteen
 * bytes from dr through ex are the ones a name can put a '?' in.
 */
isuniqu(fcb)
char *fcb;
{
	int i;

	for (i = 13; i != 0; i--) {
		if ((*fcb & 0x7F) == '?')
			return 0;
		fcb++;
	}
	return 1;
}

/*
 * fexists - does this pathname name a file that is there.
 */
fexists(name)
char *name;
{
	if (stat(name, statbuf) == 0)
		return 1;
	return 0;
}

/*
 * match - the CP/M wildcard match.
 *
 * The pattern is an FCB whose name has been turned into a micronix
 * pathname by name - lowercased, with '?' wildcards and the type after
 * the dot.  The entry is a directory entry, whose name sits two bytes in
 * after the inode.  '?' matches one character but not the dot and not
 * the end; an uppercase letter in the entry cannot match, because name
 * lowercased the pattern.  The two strings have to end together - a
 * match is exact, not a prefix of one side.
 *
 * strcmp returns 0 on equal, so the test below reads it that way.
 */
match(entry, pattern)
char *entry;
char *pattern;
{
	char *e;
	char *p;
	char ec, pc;

	e = entry + 2;
	name(pattern, buf);
	p = buf;

	while (strchr(p, '/'))
		p++;

	for (;;) {
		ec = *e;
		if (ec >= 'A' && ec <= 'Z')
			return 0;

		pc = *p;
		if (pc == '?') {
			if (ec != 0 && ec != '.')
				e++;
			p++;
			continue;
		}

		if (ec == 0) {
			if (strcmp(p, "") == 0)
				return 1;	/* both ended: a match */
			return 0;		/* entry ended, pattern continues */
		}

		if (ec != pc)
			return 0;

		e++;
		p++;
	}
}

/*
 * cread - the one read path, sequential and random both.
 *
 * cseek positions the file, then a 128-byte record is read into the
 * DMA buffer.  A short read is padded with ^Z and the random record is
 * reset, which is how CP/M marks the end of a text file.  Returns 0 for
 * a record, 1 at end of file, -1 on error - and closes the file when the
 * read runs dry or has already given the last record.
 */
cread(fn, fcb)
int fn;
struct fcb *fcb;
{
	int n;

	cseek(fn, fcb);
	n = read(fcb->fd, dma, 0x80);

	if (n <= 0 || fcb->size - 1 == fcb->rrec) {
		closefi(fcb->fd);
		fcb->fd = -1;
	}

	if (n < 0)
		return -1;
	if (n == 0)
		return 1;

	seqincr(fn, fcb);
	if (n != 0x80) {
		memset(dma + n, 0x1A, 0x80 - n);	/* pad with ^Z */
		fcb->rrec = 0;
	}
	return 0;
}

/*
 * seqincr - advance the record pointer after a read or write.
 *
 * Sequential mode walks cr up and carries into ex every 128 records;
 * random mode derives ex and cr from rrec.  Either way rrec advances,
 * which is what keeps the random record in step with the sequential one.
 */
seqincr(fn, fcb)
int fn;
struct fcb *fcb;
{
	if (fn == 'S') {
		fcb->cr++;
		if (fcb->cr == 0x80) {
			fcb->cr = 0;
			fcb->ex++;
			setrc(fcb);
		}
	} else {
		fcb->ex = fcb->rrec >> 7;
		fcb->cr = fcb->rrec & 0x7F;
		setrc(fcb);
	}
	fcb->rrec++;
}

/*
 * cwrite - the one write path, sequential and random both.
 *
 * Like cread but in the other direction: seek, write a full 128-byte
 * record from the DMA buffer, advance, and grow the size when the write
 * went past the old end.  A short write is an error.  The size field
 * tracks rrec rather than bytes here, which is the odd thing the binary
 * does and is reproduced as such.
 */
cwrite(fn, fcb)
int fn;
struct fcb *fcb;
{
	if (write(fcb->fd, dma, 0x80) != 0x80) {
		fcb->rrec = 0;
		return -1;
	}

	seqincr(fn, fcb);

	if (fcb->rrec > fcb->size) {
		fcb->size = fcb->rrec;
		fcb->nrec = fcb->size >> 7;
		if (fcb->size & 0x7F)
			fcb->nrec++;
		setrc(fcb);
	}
	return 0;
}

/*
 * cseek - position the file for the record about to be read or written.
 *
 * Flushes both output buffers first, so the disk sees a seek in the same
 * order the console saw the bytes.  A closed FCB is opened.  The record
 * number is worked out of the sequential ex/cr or the random r0/r1, and
 * turned into a micronix seek: the micronix seek counts 512-byte blocks
 * from the start and bytes from where that leaves you, which is why the
 * record number is split into >> 2 and (& 3) << 7.
 */
cseek(fn, fcb)
int fn;
struct fcb *fcb;
{
	int off;

	if (oleft)
		cflush();
	if (lleft)
		lflush();

	if (fcb->fd < 0) {
		name(fcb, buf);
		fcb->fd = openfil(buf);
		fcb->rrec = 0;
		setrc(fcb);
	}

	if (fn == 'S')
		off = (fcb->ex << 7) | fcb->cr;
	else {
		if (fcb->r2)
			return 0;	/* the high byte is out of this TPA's range */
		off = (fcb->r1 << 8) | fcb->r0;
	}

	if (off != 0 && off == fcb->rrec)
		return 0;		/* already there */

	seek(fcb->fd, off >> 2, 3);
	if (off & 3)
		seek(fcb->fd, (off & 3) << 7, 1);
	fcb->rrec = off;
	return 0;
}

/*
 * csize - BDOS function 35, compute the file size.
 *
 * Puts the file's size, in 128-byte records, into the three CP/M random
 * record bytes.  fsize does the stat and the division; this just names
 * the file and copies the answer into r0/r1.
 */
csize(fcb)
struct fcb *fcb;
{
	int size;

	name(fcb, buf);
	size = fsize(buf);
	fcb->r0 = size;
	fcb->r1 = size >> 8;
	fcb->r2 = 0;
}

/*
 * fsize - the size of a file in records.
 *
 * Stats the file and turns the byte count into a record count, rounded
 * up.  The size is a 24-bit little-endian count in the stat buffer at
 * offset 9; the record count is the top byte times 512 plus the bottom
 * two bytes over 128, rounded up when the low seven bits are set.
 */
fsize(name)
char *name;
{
	int nrec;

	if (stat(name, statbuf) < 0)
		return 0;

	nrec = (statbuf[9] << 9) | ((statbuf[10] | (statbuf[11] << 8)) >> 7);
	if ((statbuf[10] | (statbuf[11] << 8)) & 0x7F)
		nrec++;
	return nrec;
}

/*
 * setrand - BDOS function 36, set the random record.
 *
 * The sequential position ex/cr is folded into the 16-bit random record
 * r0/r1.  This is the inverse of what cseek's sequential branch does
 * when it spreads the same record number back out.
 */
setrand(fcb)
struct fcb *fcb;
{
	int rec;

	rec = (fcb->ex << 7) | (fcb->cr & 0x7F);
	fcb->r0 = rec;
	fcb->r1 = rec >> 8;
}

/*
 * cmake - BDOS function 22, make a file.
 *
 * A file that already exists is refused; a new one is created, closed,
 * and then opened again through copen so the FCB comes away with the
 * descriptor, the size and the record count filled in.
 */
cmake(fcb)
struct fcb *fcb;
{
	int fd;

	if (isuniqu(fcb) == 0)
		return -1;

	name(fcb, buf);
	if (fexists(buf))
		return -1;

	fd = creat(buf, 0x1FF);
	if (fd < 0)
		return -1;
	close(fd);

	return copen(fcb);
}

/*
 * rename - BDOS function 23, rename a file.
 *
 * The FCB carries both names: the old one in dr/name/ft, the new one
 * packed into the disk map at offset 0x10.  A rename is a link to the
 * new name and an unlink of the old.  Both names must be plain files,
 * and both are closed by name first so no descriptor holds the inode.
 */
rename(fcb)
struct fcb *fcb;
{
	char newname[32];
	struct fcb *nfcb;

	nfcb = (struct fcb *)((char *)fcb + 0x10);
	nfcb->dr = fcb->dr;

	if (uniqize(fcb) == 0)
		return -1;

	name(fcb, buf);
	name(nfcb, newname);

	if (isdir(buf) || isdir(newname))
		return -1;

	closena(buf);
	closena(newname);

	if (link(buf, newname) < 0)
		return -1;
	if (unlink(buf) < 0)
		return -1;

	fcb->fd = -1;
	return 0;
}

/*
 * uniqize - make the FCB's name a concrete one.
 *
 * A name with a '?' wildcard cannot be opened; this searches the
 * directory for the first match and writes the real name back over the
 * wildcard one.  Returns 1 when there is a unique name - either it was
 * already unique, or a match was found - and 0 when nothing matched.
 */
uniqize(fcb)
struct fcb *fcb;
{
	if (isuniqu(fcb))
		return 1;

	if (fcb->dr == '?')
		fcb->dr = 0;
	if (fcb->ex == '?')
		fcb->ex = 0;

	if (search(0x11, fcb, fcb) != -1)
		return 1;
	return 0;
}

/*
 * isdir - is this pathname a directory.
 *
 * Stats it and tests the mode's file-type bits against SIFDIR.  A
 * failed stat is not a directory.
 */
isdir(name)
char *name;
{
	if (stat(name, statbuf) < 0)
		return 0;
	if ((statbuf[5] & 0x60) == 0x40)	/* (stmode & 0x6000) == SIFDIR */
		return 1;
	return 0;
}

/*
 * select - BDOS function 14, select a disk.
 *
 * A drive with no entry in disktab is a selerr.  Otherwise the drive's
 * bit goes into the logged-in vector and it becomes the current drive.
 */
select(drive)
int drive;
{
	drive &= 0x0F;

	if (disktab[drive] == 0)
		selerr(drive);

	/*
	 * The two stores below are in the binary's order the other way
	 * round - logvect first, then curdriv - but c1 has no rule for
	 * a shift whose count is a local that is also stored as a value,
	 * so curdriv is stored first and the shift reads it back out of
	 * the global.  The two globals are independent, so nothing else
	 * can tell.
	 */
	curdriv = drive;
	logvect |= 1 << curdriv;
}

/*
 * The console and list output path.  Both buffer 32 bytes and flush when
 * the buffer is full; the console writes to the terminal and the list to
 * the list descriptor.
 */
putch(c)
int c;
{
	*op++ = c;
	oleft++;
	if (oleft == 0x20)
		cflush();
}

lput(c)
int c;
{
	*lp++ = c;
	lleft++;
	if (lleft == 0x20)
		lflush();
}

lflush()
{
	write(lstdesc, lbuf, lleft);
	lp = lbuf;
	lleft = 0;
}

/*
 * cflush - flush the console output buffer.  The odd thing here is the
 * flow control: if an interrupt has left a character waiting and the
 * program is not in direct console I/O, suspend hands control back until
 * the character is taken, which is upm's ^S/^Q.
 */
cflush()
{
	if (recavai && call != 0x06)
		suspend();

	write(1, obuf, oleft);
	op = obuf;
	oleft = 0;
}

/*
 * getch - a character from the console, buffered.  Flushes any pending
 * output first so a prompt is not left unwritten, refills the input
 * buffer when it is empty, and returns the next character.
 */
getch()
{
	if (oleft)
		cflush();
	if (lleft)
		lflush();
	if (ileft == 0)
		cfill();
	ileft--;
	return *ip++;
}

/*
 * cfill - refill the console input buffer.  Reads up to sixteen bytes
 * into ibuf, and if the read comes back empty or short, decides between
 * a transient error (retry) and a dead terminal (gtty fails, so exit).
 */
cfill()
{
	int n;

	for (;;) {
		n = read(0, ibuf, 0x10);
		recavai = 0;

		if (n > 0) {
			ileft = n;
			ip = ibuf;
			return;
		}

		if (n < 0)
			continue;		/* transient error, read again */

		if (gtty(0, sgtty) >= 0)
			continue;		/* EOF but the terminal lives */

		cexit();			/* the terminal is gone */
	}
}

/*
 * suspend - upm's ^S/^Q.  Reads one character directly from the
 * terminal while the output is waiting.  A ^S means the user wanted to
 * stop, so it reads again for the ^Q that lets go and reports the ^S;
 * any other character is pushed back onto the input buffer so it is not
 * lost, and returned.
 */
suspend()
{
	char ch;

	read(0, &ch, 1);
	recavai = 0;

	if (ch == 0x13) {
		read(0, &ch, 1);
		recavai = 0;
		return 0x13;
	}

	if (ileft == 0)
		ip = ibuf;
	if (ip + ileft < lbuf) {
		ip[ileft] = ch;
		ileft++;
	}
	return ch;
}

/*
 * openfil - open a file by name, and remember its identity.
 *
 * Closes any descriptor already on the inode, then opens read/write and
 * falls back to read-only - setting ro so the FCB gets the read-only
 * flag.  The dev and inode are recorded in files for closena.
 */
openfil(name)
char *name;
{
	int fd;

	if (closena(name) == 0)
		return -1;

	ro = 0;
	fd = open(name, 2);
	if (fd < 0) {
		ro = 1;
		fd = open(name, 0);
		if (fd < 0)
			return -1;
	}

	if (fd < 0x10) {
		files[fd].dev = statbuf[0] | (statbuf[1] << 8);
		files[fd].ino = statbuf[2] | (statbuf[3] << 8);
	}

	return fd;
}

/*
 * closefi - close a descriptor.  The descriptors 0 and 1 are the
 * console and not in files, so only 2-15 are touched, and the inode is
 * cleared so closena will not find it again.
 */
closefi(fd)
int fd;
{
	if (fd < 2 || fd > 15)
		return;
	close(fd);
	files[fd].ino = 0;
}

/*
 * closena - close every descriptor on the file of this name.
 *
 * Stats the name, and for each entry in files whose dev and inode
 * match, closes it.  Returns 0 when the name cannot be stat'ed.
 */
closena(name)
char *name;
{
	int i;

	if (stat(name, statbuf) < 0)
		return 0;

	for (i = 0; i < 0x10; i++) {
		if (files[i].dev == (statbuf[0] | (statbuf[1] << 8)) &&
		    files[i].ino == (statbuf[2] | (statbuf[3] << 8))) {
			close(i);
			files[i].ino = 0;
		}
	}
	return 1;
}

/*
 * echo - a character to the console, with the two CP/M specials the
 * console layer cares about.  Tab is expanded by tabecho; \r resets the
 * column and \b backs it up; a printable advances it.
 */
echo(c)
int c;
{
	if (c == '\t') {
		tabecho();
		return;
	}

	putch(c);

	if (c == '\r') {
		col = 0;
		return;
	}
	if (c == '\b') {
		if (col)
			col--;
		return;
	}
	if (c < 0x20 || c > 0x7E)
		return;
	col++;
}

/*
 * cdirio - BDOS function 6, direct console I/O.  A character other than
 * FF is output, un-echoed; FF is a poll - a character if one is ready,
 * else 0 after flushing any pending output.
 */
cdirio()
{
	int c;

	c = arg2 & 0xFF;

	if (c != 0xFF) {
		putch(c);
		return c;
	}

	if (recavai || ileft)
		return getch();

	if (oleft)
		cflush();

	return 0;
}

/*
 * cprs - BDOS function 9, print a '$'-terminated string.  Each
 * character goes through echo so tabs expand and the column tracks.
 */
cprs()
{
	char *s;

	s = (char *)arg2;
	while (*s != '$') {
		echo(*s);
		s++;
	}
}

/*
 * readbuf - BDOS function 10, a line with editing.  The first byte is
 * the buffer size, the second the count, and the characters start at +2.
 * The editing characters are CP/M's: ^C cancels (a warm boot at the
 * start of a line), ^H backs up, ^M/^J end, ^R retypes, ^X clears the
 * line, and ^E is a bare newline that does not end the input.
 */
readbuf(buf)
char *buf;
{
	int c;

	col = 0;
	buf[1] = 0;

	for (;;) {
		if (buf[0] == buf[1])
			return;

		c = getch();

		switch (c) {
		case 3:			/* ^C */
			if (col == 0) {
				rbecho(3);
				puts("\r");
				wboot();
			}
			rbecho(c);
			buf[buf[1] + 2] = c;
			buf[1]++;
			break;

		case 5:			/* ^E */
			col = 0;
			puts("\r\n");
			break;

		case 8:			/* ^H */
			if (buf[1]) {
				buf[1]--;
				erase(buf[buf[1] + 2]);
			}
			break;

		case 10:		/* ^J */
		case 13:		/* ^M */
			col = 0;
			putch('\r');
			return;

		case 18:		/* ^R */
			puts("#\r\n");
			putb(buf + 2, buf[1]);
			break;

		case 24:		/* ^X */
			while (col)
				rub();
			buf[1] = 0;
			break;

		default:
			rbecho(c);
			buf[buf[1] + 2] = c;
			buf[1]++;
			break;
		}
	}
}

/*
 * erase - undo one echoed character, the backspace half of ^H.  A tab
 * is undone by taberas; a printable by two rubs; a control or a high
 * character by one, or none.
 */
erase(c)
int c;
{
	if (c == '\t') {
		taberas();
		return;
	}
	if (c >= 0 && c < 0x20) {
		rub();
		rub();
		return;
	}
	if (c < 0x20 || c > 0x7E)
		return;
	rub();
}

/*
 * The tab and rubout machinery.  A tab stop is every eight columns;
 * tabecho remembers where each tab landed in tabs so taberas can
 * erase back to it, and the rest is space, rub and tabpos.
 */
tabpos()
{
	return (col & 7) == 0;
}

space()
{
	putch(' ');
	col++;
}

rub()
{
	if (col == 0)
		return;
	puts("\b \b");
	col--;
}

tabecho()
{
	if (ntab != 0x0C) {
		tabs[ntab] = col;
		ntab++;
	}
	space();
	while (!tabpos())
		space();
}

taberas()
{
	int c;

	if (ntab != 0)
		c = tabs[--ntab];
	else
		c = 1;

	while (col != c)
		rub();
}

/*
 * rbecho - echo a character the way the console wants it.  A tab is
 * expanded; a control character becomes a caret and its letter; a
 * printable is itself.  echo calls putch and tracks the column; this
 * is the editing version.
 */
rbecho(c)
int c;
{
	if (c == '\t') {
		tabecho();
		return;
	}
	if (c >= 0 && c < 0x20) {
		putch('^');
		putch(c + 0x40);
		col += 2;
		return;
	}
	if (c >= 0x20 && c <= 0x7E) {
		putch(c);
		col++;
	}
}

/*
 * go - jump into the CP/M TPA and run the loaded program.  The entry
 * is always 0x100; when the program returns, cexit takes over.
 */
go()
{
	(*((void (*)())0x100))();
	cexit();
}

/*
 * selerr - a drive was selected that has no directory.  Ask for one,
 * verify it is a directory, and remember it in disktab, the same way
 * the drive-assignment command does.  It loops until the answer is a
 * directory.
 */
selerr(drive)
int drive;
{
	char buf[48];

	drive &= 0x0F;

	for (;;) {
		puts("\r\nSelect a directory for drive ");
		putch(drive + 'A');
		puts(": ");
		buf[0] = '\'';
		readbuf(buf);
		puts("\r\n");
		buf[buf[1] + 2] = 0;
		cflush();

		if (isdir(buf + 2))
			break;

		puts(buf + 2);
		puts(": Not a directory\r\n");
	}

	disktab[drive] = save(buf + 2);
}
struct node base;		/* eb33 */
struct node *allocp;		/* eb37 */
char	savebuf[512];		/* the SAVE allocator's free memory */

setallo()
{
	if (allocp == 0) {
		allocp = &base;
		base.next = &base;
		base.size = 0;
	}
	return allocp;
}

alloc(n)
ushort n;
{
	ushort size;
	struct node *p;
	struct node *q;

	size = (n + 3) / 4 + 1;

	p = setallo();
	q = p->next;

	for (;;) {
		if (q->size >= size)
			break;
		if (q == p) {
			puts("Out of memory\r\n");
			cexit();
		}
		p = q;
		q = q->next;
	}

	/* take the allocation from the end of the free block */
	q->size -= size;
	q = (struct node *)((char *)q + q->size * 4);
	q->size = size;

	allocp = p;
	return (char *)q + 4;
}

save(s)
char *s;
{
	char *p;

	p = alloc(strlen(s) + 1);
	if (p == 0) {
		puts("Out of memory\r\n");
		cexit();
	}
	cpystr(p, s, 0);
	return p;
}

/*
 * trestor - put the terminal back the way it was, the inverse of what
 * tset does when a program starts.  Clears the raw-mode bits.
 */
trestor()
{
	gtty(0, sgtty);
	sgtty[4] |= 0x1A;
	sgtty[4] &= ~0x20;
	stty(0, sgtty);
}

/*
 * load - read the .com named by loadfil into the TPA and jump to it.
 * Sixteen kilobytes at a time, from 0x100 up.
 */
load()
{
	int fd;
	int offset;
	int n;

	fd = open(loadfil, 0);
	if (fd < 0) {
		perror(loadfil);
		cexit();
	}

	offset = 0x100;
	for (;;) {
		n = read(fd, offset, 0x4000);
		if (n <= 0) {
			close(fd);
			close(2);
			loaded = 1;
			go();
		}
		offset += n;
	}
}

/*
 * wboot - the warm boot, BDOS function 0 and the BIOS's WBOOT.  If a
 * program is loaded, its finishing means upm finishes too.
 */
wboot()
{
	if (loaded == 0)
		return;
	cexit();
}

/*
 * badbios - a CP/M program called a BIOS entry that is not there.
 */
badbios()
{
	puts("Bad bios call\r\n");
	cexit();
}

/*
 * ---------------------------------------------------------------------
 * The helpers the micronix libc does not provide.
 *
 * The string and byte routines the original linked from Whitesmith's
 * library are the micronix libc's own now - strlen, strcmp, memset,
 * memcpy, strchr - and upm calls those directly.  What is left here are
 * the ones libc does not have: cpystr, the variadic concatenate; puts
 * and putb, which write through putch rather than stdio; itob; and
 * signal, libu's, moved here so _stab stays resident where a running
 * .com cannot clobber it.
 *
 * The one that is not obvious is cpystr: it copies its source strings
 * into dst until it reads a null argument, then writes the terminating
 * nul and returns where it finished.  The null argument is what every
 * call ends in.
 */

/*
 * puts - print a string through putch, one character at a time, with no
 * trailing newline.  Not the stdio puts.
 */
puts(s)
char *s;
{
	while (*s)
		putch(*s++);
}

/*
 * putb - print n bytes of buf through putch.
 */
putb(buf, n)
char *buf;
int n;
{
	while (n--)
		putch(*buf++);
}

/*
 * raise - uppercase a string in place, the string form of uc.
 */
raise(s)
char *s;
{
	while (*s) {
		*s = uc(*s);
		s++;
	}
}

/*
 * cpystr - concatenate the source strings into dst.  The sources are a
 * variable number of char * arguments, ended by a null argument; each is
 * copied in turn, a single nul is written, and the position after it is
 * returned.
 */
char *
cpystr(dst, s)
char *dst;
char *s;
{
	char **ap;
	char *q;

	ap = &s;
	while ((q = *ap++) != 0) {
		while (*q)
			*dst++ = *q++;
	}
	*dst = 0;
	return dst;
}

/*
 * signal and stab - libu's, moved here so that _stab is resident: the
 * trampolines in upmsys.s read it while a .com runs, and libu's signal.o
 * would put it in the data/bss segment at 0x0100, where the .com clobbers
 * it.  _signal is the syscall stub in upmsys.s.
 */
short stab[15];
