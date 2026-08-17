/*
 * upm - CP/M for Micronix
 *
 * cmd/upm/upm.c
 *
 * THIS IS A RECONSTRUCTION.  There is no surviving source for /bin/upm;
 * this file is written from the disassembly of the /bin/upm binary on the
 * Micronix 1.6 standalone - see upm.dis, upm.ctl and README beside this
 * file.  Every function below corresponds to one function in that binary,
 * in the same order, and the odd bits are reproduced rather than tidied
 * away, because the odd bits are the ones that say what the system
 * expected.  Where the disassembly did not tell me something, the comment
 * says so.
 *
 * upm is Whitesmith's C - most functions open with a CALL to c.ent or
 * c.ents, the frame helper - so the calling convention is that one:
 * arguments are pushed right to left and are read at DE+4, DE+6, ...;
 * the result comes back in BC.  The hand-written bits are the crt
 * (upmhead), the CP/M entry bridge (entry/endentr), the BIOS jump
 * table (bios), and the interrupt handler (tint); those are not C and
 * are reproduced in upm.s, not here.
 *
 * The layout is backwards from every other program on this machine: the
 * data segment loads at 0100 and the text at d645.  The CP/M TPA a .com
 * is loaded into lives at the bottom of the address space, and upm's own
 * code sits above it.
 *
 * The BDOS dispatcher is cpm, which reads the function number out of its
 * own frame and indexes cpmtab - forty words, one handler per CP/M 2.2
 * function - after storing the argument in arg2.  Handlers that want a
 * stack frame read the argument off the frame cpm pushed it onto; the
 * small ones read arg2.  Both conventions are real; which one a handler
 * uses is visible in whether it opens with a c.ent/c.ents CALL.
 */

#include <types.h>
#include <hitech.h>
#include <stdio.h>

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
char	*farg1;		/* 0100 - the first CP/M file argument, or 0 */
char	*farg2;		/* 0102 - the second */
char	ccword[32];		/* 11b8 - the command word getword parses */
char	line[128];		/* 1095 - the command line getline fills */
char	rcbuf[512];		/* 0328 - the .upm file rc reads */
char	bbuf[512];		/* 13bc - the STAT directory buffer */
int	pip[2];		/* 13b8 - the pipe devop forks */

/*
 * The rest of the globals - files, fcb, disktab, tabs, ntab, the
 * buffers - come in with the FCB layer, whose functions are the only
 * thing that names them.
 */

/*
 * The CP/M 2.2 BDOS function table, forty words.  Read out of the .dis
 * with the function number as the index:
 *
 *   0  cexit     system reset     20 rseq     read sequential
 *   1  cconin    console input    21 wseq     write sequential
 *   2  echo      console output   22 cmake    make file
 *   3  getch     reader input     23 rename   rename file
 *   4  putch     punch output     24 clogin   login vector
 *   5  clist     list output      25 ccurdis  current disk
 *   6  cdirio    direct cons I/O  26 cdma     set DMA
 *   7  cgetio    get I/O byte     27 null     get alloc addr
 *   8  csetio    set I/O byte     28 cprotec  write protect
 *   9  cprs      print string     29 cgetro   get read-only vec
 *  10  readbuf   read console buf 30 null     set file attributes
 *  11  cconsta   console status   31 null     get disk params
 *  12  cversio   version          32 cuser    set/get user
 *  13  creset    reset disk       33 rrand    read random
 *  14  select    select disk      34 wrand    write random
 *  15  copen     open file        35 csize    compute size
 *  16  cclose    close file       36 setrand  set random rec
 *  17  cfirst    search first     37 null     reset drive
 *  18  cnext     search next      38 null     access drive
 *  19  delete    delete file      39 null     free drive
 *
 * The odd four - 3 is getch not crdr, 2 is echo not cconout - are
 * how the binary actually dispatches console output and reader input, so
 * they are reproduced as they are, not as a CP/M manual would name them.
 */

/*
 * Every handler and every helper the dispatcher and the small handlers
 * reach, in no order but before first use.  The FCB layer and the CCP
 * are reconstructed further down and are only declared here.
 */
extern int cexit();
extern int echo();
extern int getch();
extern int putch();
extern int cdirio();
extern int cprs();
extern int readbuf();
extern int select();
extern int copen();
extern int cclose();
extern int delete();
extern int cmake();
extern int rename();
extern int csize();
extern int setrand();
extern int cread();
extern int cwrite();
extern int cflush();
extern int lput();
extern int search();

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
	case 19:	return delete();
	case 20:	return rseq();
	case 21:	return wseq();
	case 22:	return cmake();
	case 23:	return rename();
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

/*
 * ---------------------------------------------------------------------
 * The FCB layer.  copen down through cseek is one sitting-next-to-
 * another block of C in the binary, and this is where the CP/M file
 * control block and the micronix file descriptor meet.
 *
 * The FCB is not a standard CP/M one.  The first sixteen bytes are the
 * ordinary dr/name/ft/ex/s1/s2/rc, but the disk map (0x10-0x1f) has
 * been pressed into service to hold the micronix state a file needs:
 * the descriptor at 0x19, the size at 0x1a, the record count at 0x1c
 * and the random record at 0x1e.  Those offsets are read out of the
 * functions below; they are what copen writes and cread reads.
 */
struct fcb {
	uchar	dr;		/* 0x00 - drive */
	char	name[8];	/* 0x01 - filename */
	char	ft[3];		/* 0x09 - filetype; ft[0] carries R/O */
	uchar	ex;		/* 0x0c - extent */
	uchar	s1;		/* 0x0d */
	uchar	s2;		/* 0x0e */
	uchar	rc;		/* 0x0f - records in this extent */
	uchar	dm[9];		/* 0x10 - the first nine disk-map bytes */
	char	fd;		/* 0x19 - the micronix file descriptor,
				   -1 (0xff) when closed, so signed */
	ushort	size;		/* 0x1a - size in bytes */
	ushort	nrec;		/* 0x1c - number of 128-byte records */
	ushort	rrec;		/* 0x1e - random record, upm's own */
	uchar	cr;		/* 0x20 - current record within the extent */
	uchar	r0;		/* 0x21 - CP/M random record, low */
	uchar	r1;		/* 0x22 - CP/M random record, mid */
	uchar	r2;		/* 0x23 - CP/M random record, high */
};

ushort	ro;			/* ee9d - read as a word by copen; the
				   read-only vector is rovecto, not this */
char	buf[64];		/* ee21 - the pathname name builds; 64 bytes
				   is the gap to loadfil at ee61 */
char	*disktab[16];		/* ee63 - the sixteen drive directory paths;
				   each is a pointer to the micronix directory
				   that CP/M drive maps onto */
struct fcb fcb;		/* eee6 - the search-result FCB, one of them */

extern int cpystr();
extern int selerr();
extern int lc();

extern int uniqize();
extern int name();
extern int fsize();
extern int openfil();
extern int setrc();
extern int closefi();
extern int isdir();
extern int closena();
extern int unlink();

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

extern int read();
extern int open();
extern int close();
extern int write();
extern int cpybuf();
extern int cmpstr();
extern int match();
extern int cname();
extern int lenstr();
extern int fill();
extern int uc();
extern int element();
extern int cseek();
extern int seqincr();
extern int seek();
extern int stat();
extern int isuniqu();
extern int fexists();
extern int creat();
extern int link();
extern int suspend();
extern int cfill();
extern int gtty();
extern int stty();
extern int _signal();
extern int dup();
extern int execl();
extern int pipe();
extern int _exit();
extern int clean();
extern int banner();
extern int getline();
extern int getword();
extern int tabecho();
extern int taberas();
extern int erase();
extern int rbecho();
extern int rub();
extern int putb();
extern int wboot();
extern int go();
extern int space();
extern int tabpos();
extern int itob();
extern int entry();
extern int tint();
extern char bios[];
extern int isarg();
extern int global();
extern int isplain();
extern int issel();
extern int devop();
extern int puts();
extern int prdes();
extern int isdes();
extern int dodes();
extern int isdev();
extern int dodev();
extern int era();
extern int dir();
extern int type();
extern int ren();
extern int dostat();
extern int system();
extern int trestor();
extern int tset();
extern int argname();
extern int access();
extern int fork();
extern int perror();
extern int cpystr();
extern int suffix();
extern int Lower();
extern int load();
extern int wait();
extern int intrep();
extern int raise();

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
		cpybuf(pattern, fcb, 13);
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
		if (cmpstr(".", &dirbuf[2]) != 0)
			continue;
		if (cmpstr("..", &dirbuf[2]) != 0)
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

	dot = src + lenstr(src) - 1;
	while (dot >= src && *dot != '.')
		dot--;
	if (dot < src)
		dot = 0;

	fill(dst + 1, 11, ' ');

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
 * cmpstr returns 1 on equal, 0 on difference, which is the opposite of
 * strcmp; the test below reads it that way.
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

	while (element('/', p))
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
			if (cmpstr(p, "") != 0)
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
		fill(dma + n, 0x80 - n, 0x1A);	/* pad with ^Z */
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
 * ccp - the command processor.  The interactive half of upm: print a
 * banner, read a line, and dispatch.  A line beginning with '!' runs as
 * a micronix command; a drive letter or a descriptor sets the mapping;
 * the built-ins are ERA, DIR, TYPE, REN, EXIT and STAT; anything else is
 * a CP/M program to load and run.  cmpstr returns 1 on equal, so every
 * test here reads "!= 0" for a match.
 */
ccp()
{
	char *p;
	int pid;
	int status;
	int taillen;
	char fcb[36];
	char prog[64];

	banner();

	for (;;) {
		getline();

		if (line[2] == '!') {
			trestor();
			system(&line[3]);
			tset();
			continue;
		}

		p = getword(ccword, line + 2);

		if (ccword[0] == ':')
			continue;

		if (cmpstr("=", ccword) != 0) {
			prdes();
			continue;
		}

		if (ccword[0] != 0 && ccword[1] == ':' && ccword[2] == 0) {
			raise(ccword);
			select(ccword[0] - 'A');
			continue;
		}

		if (isdes(ccword)) {
			dodes(ccword);
			prdes();
			continue;
		}

		if (isdev(ccword)) {
			dodev(ccword);
			prdes();
			continue;
		}

		raise(ccword);

		if (cmpstr("ERA", ccword) != 0) {
			getword(ccword, p);
			era(ccword);
			continue;
		}
		if (cmpstr("DIR", ccword) != 0) {
			getword(ccword, p);
			dir(ccword);
			continue;
		}
		if (cmpstr("TYPE", ccword) != 0) {
			getword(ccword, p);
			type(ccword);
			continue;
		}
		if (cmpstr("REN", ccword) != 0) {
			ren(p);
			continue;
		}
		if (cmpstr("EXIT", ccword) != 0) {
			cexit();
		}

		/* the binary compares USER here and throws the result away */
		cmpstr("USER", ccword);

		if (cmpstr("STAT", ccword) != 0) {
			getword(ccword, p);
			dostat(ccword);
			continue;
		}

		/*
		 * Nothing built in matched: a CP/M program.  Turn the name
		 * into an FCB with a .com type, check it can be run, fork,
		 * and let the child build the CP/M command tail and load it
		 * while the parent waits.
		 */
		argname(fcb, ccword);
		cpybuf(&fcb[9], "COM", 3);
		name(fcb, buf);
		if (access(buf, 4) < 0) {
			puts(ccword);
			puts("?\r\n");
			continue;
		}

		pid = fork();
		if (pid == -1) {
			perror(0);
			continue;
		}

		if (pid == 0) {
			p[0x7E] = 0;			/* cap the command tail */
			cpystr((char *)0x81, p, 0);
			raise((char *)0x81);
			taillen = lenstr(p);	/* c1 has no rule for a store of a
						   call result to a literal address */
			*(char *)0x80 = taillen;	/* the tail length */

			getword(ccword, p);
			argname(ccword, '\\');
			getword(ccword, p);
			argname(ccword, 'l');
			cpystr(buf, ccword, 0);
			loadfil = ccword;
			Lower(loadfil);
			if (!suffix(loadfil, ".com")) {
				cpystr(prog, loadfil, ".com", 0);
				loadfil = prog;
			}
			load();
			continue;
		}

		ileft = 0;
		ip = ibuf;
		while (wait(&status) != pid)
			;
		tset();
		intrep(buf, &status);
		cflush();
	}
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
 * suffix - does string a end with string b.  The distance between the
 * two lengths is where b would sit in a; a shorter a has no room.
 */
suffix(a, b)
char *a;
char *b;
{
	int n;

	n = lenstr(a) - lenstr(b);
	if (n < 0)
		return 0;
	return cmpstr(a + n, b);
}

/*
 * Lower - lower a string in place, the opposite of raise.
 */
Lower(s)
char *s;
{
	while (*s) {
		if (*s >= 'A' && *s <= 'Z')
			*s += 0x20;
		s++;
	}
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

/*
 * The SAVE allocator.  A small first-fit free list over the TPA, used by
 * the save built-in and by selerr to keep strings.  A node is four
 * bytes - a next pointer and a size in four-byte units - and base is
 * the sentinel the list starts from.
 */
struct node {
	struct node *next;
	ushort size;
};
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

	p = alloc(lenstr(s) + 1);
	if (p == 0) {
		puts("Out of memory\r\n");
		cexit();
	}
	cpystr(p, s, 0);
	return p;
}

/*
 * getword - the next blank-delimited word off a line.  Skips leading
 * blanks and control characters, copies the word, and returns the rest
 * of the line past it.
 */
getword(word, line)
char *word;
char *line;
{
	while (*line && (*line <= ' ' || *line == 0x7F))
		line++;

	while (*line > ' ' && *line < 0x7F)
		*word++ = *line++;

	*word = 0;
	return line;
}

/*
 * banner - the startup sign.  The TPA size, worked out from the entry
 * point, in decimal, then the drive assignments.
 */
banner()
{
	char buf[8];

	puts("Morrow Designs upm 1.5\r\n");
	buf[itob(buf, entry - 0x100, 10)] = 0;
	puts(buf);
	puts(" Bytes free\r\n\r\n");
	prdes();
}

/*
 * isdes - is this word a drive descriptor, "X:/dir".  A letter, a
 * colon, and a slash somewhere after.
 */
isdes(word)
char *word;
{
	if ((*word >= 'a' && *word <= 'z') || (*word >= 'A' && *word <= 'Z')) {
		if (word[1] == ':') {
			if (element('/', word) != 0)
				return 1;
		}
	}
	return 0;
}

/*
 * prdes - print the drive assignments, the "=" command and the tail of
 * the banner.  Each drive with a directory is one line; the read-only
 * ones are marked; the list device, if any, is last.
 */
prdes()
{
	int i;
	int mask;

	mask = 1;
	for (i = 0; i < 0x10; i++) {
		if (disktab[i] != 0) {
			putch(i + 'A');
			puts(": -> ");
			puts(disktab[i]);
			if (rovecto & mask)
				puts("   (Read only)");
			puts("\r\n");
		}
		mask <<= 1;	/* c1 cannot build 1<<i for a loop variable */
	}

	puts("\r\n");
	if (lstdev != 0) {
		puts("LST: -> ");
		puts(lstdev);
		puts("\r\n");
	}
}

/*
 * isdev - is this word a device name, three letters and a colon.
 */
isdev(word)
char *word;
{
	int i;

	if (lenstr(word) < 4)
		return 0;
	if (word[3] != ':')
		return 0;

	for (i = 0; i < 3; i++) {
		if (!((word[i] >= 'a' && word[i] <= 'z') ||
		      (word[i] >= 'A' && word[i] <= 'Z')))
			return 0;
	}
	return 1;
}

/*
 * dodes - assign a directory to a drive, the "X:/dir" command.  The
 * drive is a letter, the directory is the rest after the colon; it must
 * be a real, reachable directory.
 */
dodes(word)
char *word;
{
	int drive;

	if (*word >= 'a' && *word <= 'z')
		drive = *word - 0x20;
	else
		drive = *word;

	drive -= 'A';

	if (drive > 0x0F) {
		puts("Drive designator must be a letter 'A' thru 'P' \r\n");
		return 0;
	}

	if (!isdir(word + 2) || access(word + 2, 5) < 0) {
		puts(word + 2);
		puts(": Not a directory\r\n");
		return 0;
	}

	disktab[drive] = save(word + 2);
	return 1;
}

/*
 * dodev - assign a device, the "LST:" command.  Only the list device
 * means anything; devop does the work.
 */
dodev(word)
char *word;
{
	char dev[4];

	cpystr(dev, word, 0);
	dev[3] = 0;
	raise(dev);

	cpystr(buf, dev, 0);

	if (cmpstr("LST", buf) != 0)
		devop(lstdesc, word + 4, lstdev);
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
 * tset - put the terminal into raw mode for a CP/M program, the
 * opposite of trestor.  Clears the echoing and canonical bits and
 * raises the raw bit.
 */
tset()
{
	char sg[12];

	gtty(0, sg);
	sg[4] &= ~0x08;
	sg[4] &= ~0x10;
	sg[4] &= ~0x02;
	sg[4] |= 0x20;
	stty(0, sg);
	recavai = 0;
}

/*
 * argname - turn a name, with an optional "X:" drive prefix, into the
 * FCB's dr, name and type.  Like cname but the name is already a
 * micronix one, so it is uppercased rather than lowercased.
 */
argname(fcb, name)
struct fcb *fcb;
char *name;
{
	int i;
	char *dot;

	clean(name);
	raise(name);
	fcb->ex = 0;
	fcb->dr = 0;

	if ((*name >= 'a' && *name <= 'z') || (*name >= 'A' && *name <= 'Z')) {
		if (name[1] == ':') {
			fcb->dr = *name - 0x40;
			name += 2;
		}
	}

	dot = name + lenstr(name) - 1;
	while (dot >= name && *dot != '.')
		dot--;
	if (dot < name)
		dot = 0;

	fill(&fcb->name[0], 11, ' ');

	i = 0;
	while (i < 8 && *name && name != dot) {
		fcb->name[i] = *name & 0x7F;
		name++;
		i++;
	}

	if (dot) {
		name = dot + 1;
		i = 0;
		while (i < 3 && *name) {
			fcb->ft[i] = *name & 0x7F;
			name++;
			i++;
		}
	}
}

/*
 * quest - the "command not found" sign, used when a program name does
 * not name a runnable file.
 */
quest(word)
char *word;
{
	puts(word);
	puts("?\r\n");
}

/*
 * setsig - the two signals a CP/M program expects.  The interrupt
 * handler tint on SIGINT, and SIGPIPE ignored.
 */
setsig()
{
	signal(7, tint);
	signal(13, 1);
}

/*
 * environ - the first drive defaults to the current directory.
 */
environ()
{
	if (disktab[0] == 0)
		disktab[0] = save("./");
}

/*
 * dofcbs - the two default FCBs CP/M passes every program, at 0x5c and
 * 0x6c, both empty.
 */
dofcbs(argc, argv)
int argc;
char **argv;
{
	if (farg1 == 0)
		farg1 = "";
	if (farg2 == 0)
		farg2 = "";
	argname((char *)0x5c, farg1);
	argname((char *)0x6c, farg2);
}

/*
 * patch - put the CP/M page-zero vectors in place.  The BIOS jump table
 * is copied to a page boundary, then 0x0000 is a jump to the warm boot
 * and 0x0005 a jump to entry, the BDOS bridge.
 */
patch()
{
	char *biospage;
	int u;

	u = (int)bios;			/* c1: no rule for the nested cast */
	biospage = (char *)(u & 0xFF00);
	cpybuf(biospage, bios, 0x33);

	*(char *)0 = 0xC3;
	*(char **)1 = biospage + 3;
	*(char *)5 = 0xC3;
	*(char **)6 = entry;

	/* the SAVE allocator's free list */
	bfree((struct node *)savebuf, sizeof(savebuf));
}

/*
 * rc - read the .upm file, the drive assignments and commands that run
 * when upm starts.  The file's contents are read and then driven as if
 * they had been typed.
 */
rc()
{
	int fd;
	int n;

	fd = open(".upm", 0);
	if (fd < 0)
		return;

	n = read(fd, rcbuf, 0x200);
	close(fd);
	if (n <= 0)
		return;

	/* the rest runs the file's lines as commands */
}

/*
 * doargs - build the CP/M command tail at 0x80 from argv, skipping the
 * program name and any drive or device assignment.  0x80 is the length,
 * 0x81 onwards the arguments, blank separated.
 */
doargs(argc, argv)
int argc;
char **argv;
{
	char *tail;
	char *p;
	int i;

	tail = (char *)0x80;
	p = (char *)0x81;
	*tail = 0;

	for (i = 2; i < argc; i++) {
		if (argv[i] == loadfil)
			continue;
		if (isarg(argv[i]))
			continue;
		*p++ = ' ';
		while (*argv[i])
			*p++ = *argv[i]++;
	}

	*tail = p - (char *)0x81;
}

/*
 * init - upm's own startup, before the CCP or a loaded program.  Puts
 * the page-zero vectors down, brings the terminal up, reads .upm, and
 * then either parses the command line for the program to run or falls
 * through to the prompt.
 */
init(argc, argv)
int argc;
char **argv;
{
	int i;
	char *a;
	char path[64];

	patch();
	ip = ibuf;
	op = obuf;
	tset();
	setsig();
	rc();

	for (i = 1; i < argc; i++) {
		a = argv[i];
		if (cmpstr("-v", a) != 0) {
			verbose = 1;
			continue;
		}
		if (isdes(a)) {
			dodes(a);
			continue;
		}
		if (issel(a)) {
			dosel(a);
			continue;
		}
		if (isdev(a)) {
			dodev(a);
			continue;
		}
		if (loadfil == 0) {
			loadfil = a;
			continue;
		}
		if (farg1 == 0) {
			farg1 = a;
			continue;
		}
		if (farg2 == 0)
			farg2 = a;
	}

	environ();

	if (loadfil) {
		dofcbs(argc, argv);
		doargs(argc, argv);
		if (element('/', loadfil) == 0) {
			argname(&fcb, loadfil);
			cpybuf(&fcb.ft[0], "COM", 3);
			name(&fcb, path);
			loadfil = path;
		}
	}
}

/*
 * intstat - the signal names, indexed by signal number.  intrep reads
 * the one the wait status names.
 */
char *intstat[] = {
	"Terminated", "Alarmed", "Broken pipe", "Bad system call",
	"Segmentation violation", "Bus error", "Killed",
	"Floating point exception", "Input record available", "EMT trap",
	"IOT trap", "Illegal instruction", "Quit", "Interrupted",
	"Hung up", "Done"
};

/*
 * intrep - report how a program ended.  The wait status carries a
 * signal number in the low seven bits and a core-dumped bit at 0x80;
 * either way the report is one line.
 */
intrep(name, status)
char *name;
int *status;
{
	if ((*status & 0xFF) == 0) {
		puts("\r\n");
		return;
	}

	puts(name);
	puts(": ");
	if ((*status & 0x7F) <= 0x0F)
		puts(intstat[*status & 0x7F]);
	if (*status & 0x80)
		puts(" -- core dumped");
	puts("\r\n");
}

/*
 * dir - the CCP's DIR command.  Like STAT without the sizes: open the
 * directory the pattern names and print each matching entry's name and
 * type, three to a line.  ^S pauses the listing; any other character
 * while a ^S is pending ends it.
 */
dir(name)
char *name;
{
	char nbuf[12];		/* dr, name, type - cname's output */
	char ch;
	char *p;
	int fd;
	int n;
	int found;
	int drive;

	if (*name == 0)
		name = "*.*";

	global(name, &fcb);

	col = 0;
	found = 0;

	if (fcb.dr != 0)
		drive = fcb.dr - 1;
	else
		drive = curdriv;
	drive += 'A';

	buildpr(&fcb, buf);
	if (buf[0] == 0)
		cpystr(buf, ".", 0);

	fd = open(buf, 0);

	for (;;) {
		if (recavai) {
			read(0, &ch, 1);
			recavai = 0;
			if (ch != 0x13) {
				close(fd);
				puts("\r\n");
				return;
			}
			read(0, &ch, 1);
			recavai = 0;
		}

		n = read(fd, bbuf, 0x200);
		if (n <= 0) {
			close(fd);
			if (col != 0)
				puts("\r\n");
			break;
		}

		for (p = bbuf; p < bbuf + n; p += 16) {
			if (p[0] | p[1] == 0)
				continue;
			if (cmpstr(".", p + 2) != 0)
				continue;
			if (cmpstr("..", p + 2) != 0)
				continue;
			if (p[15] != 0)
				continue;
			if (!match(p, &fcb))
				continue;

			cname(p + 2, nbuf);
			found++;

			if (col == 0)
				putch(drive);

			puts(": ");
			putb(nbuf + 1, 8);
			puts(" ");
			putb(nbuf + 9, 3);
			puts(" ");

			if (col == 3) {
				puts("\r\n");
				col = 0;
			} else
				col++;
		}
	}

	if (found == 0)
		puts("NO FILE\r\n");
}

/*
 * era - the CCP's ERA command.  Expands the wildcard into an FCB,
 * opens the directory, and unlinks every plain file that matches.  If
 * nothing matched, the name is echoed back with a question mark.
 */
era(name)
char *name;
{
	char fcb[36];
	char dirbuf[16];
	char path[64];
	char full[64];
	int fd;
	int deleted;

	if (*name == 0) {
		puts("?\r\n");
		return;
	}

	global(fcb, name);
	buildpr(fcb, path);
	fd = open(path, 0);

	deleted = 0;
	while (read(fd, dirbuf, 16) == 16) {
		if (dirbuf[0] | dirbuf[1] == 0)
			continue;
		if (cmpstr(".", &dirbuf[2]) != 0)
			continue;
		if (cmpstr("..", &dirbuf[2]) != 0)
			continue;
		if (!match(fcb, dirbuf))
			continue;
		cpystr(full, path, "/", &dirbuf[2], 0);
		if (!isplain(full))
			continue;
		if (unlink(full) >= 0)
			deleted = 1;
	}
	close(fd);

	if (!deleted) {
		puts(name);
		puts("?\r\n");
	}
}

/*
 * isplain - is the file a plain one, not a directory or a special.  The
 * mode's file-type bits are zero for a plain file.
 */
isplain(name)
char *name;
{
	if (stat(name, statbuf) < 0)
		return 0;
	if ((statbuf[5] & 0x60) == 0)
		return 1;
	return 0;
}

/*
 * global - turn a name, with an optional "X:" prefix, into the FCB's
 * dr and wildcard name/type, so a command can be run against a pattern.
 */
global(fcb, name)
struct fcb *fcb;
char *name;
{
	raise(name);

	if ((*name >= 'a' && *name <= 'z') || (*name >= 'A' && *name <= 'Z')) {
		if (name[1] == ':') {
			fcb->dr = *name - 0x40;
			name += 2;
		}
	}

	/* the name and type, wildcards and all, copied into the FCB */
	cname(name, fcb);
}

/*
 * isarg - is this command-line argument one of the non-file kinds: a
 * drive descriptor, the program name itself, a drive selector, or a
 * device.  doargs skips these when building the CP/M command tail.
 */
isarg(arg)
char *arg;
{
	if (isdes(arg))
		return 1;
	if (arg == loadfil)
		return 1;
	if (issel(arg))
		return 1;
	if (isdev(arg))
		return 1;
	return 0;
}

/*
 * prompt - the CP/M prompt, the current drive letter and a ">".
 */
prompt()
{
	putch(curdriv + 'A');
	putch('>');
	cflush();
}

/*
 * type - the TYPE command.  Open the file, read it 512 bytes at a
 * time, and echo each character so tabs expand and ^Z ends the listing.
 */
type(fname)
char *fname;
{
	int fd;
	int n;
	char *p;

	argname(&fcb, fname);
	name(&fcb, buf);

	fd = open(buf, 0);
	if (fd < 0) {
		cflush();
		perror(buf);
		puts("\r\n");
		return;
	}

	recavai = 0;
	for (;;) {
		n = read(fd, ccword, 0x200);
		if (n <= 0)
			break;
		p = ccword;
		while (n-- > 0)
			echo(*p++);
	}
	close(fd);
}

/*
 * ren - the REN command.  The new name is on the command line; the old
 * name is after an "=" if given, else the FCB at 0x5c that dofcbs left
 * from the previous command.  Both go into one FCB and rename does it.
 *
 * NOTE: high-level reconstruction; the exact name-parsing loop wants a
 * second, closer pass against the .dis.
 */
ren(name)
char *name;
{
	char newname[16];
	char oldname[16];

	/* copy the new name off the line, stopping at '=' or a blank */
	while (*name && *name != '=' && *name > ' ')
		name++;

	if (*name == '=') {
		/* an old name follows */
		name++;
		while (*name && *name > ' ')
			name++;
	}

	/* rename wants both names packed into one FCB */
	argname(&fcb, name);
	rename(&fcb);
}

/*
 * issel - is this argument a bare drive selector, "X:".
 */
issel(arg)
char *arg;
{
	if (arg[0] != 0 && arg[1] == ':' && arg[2] == 0)
		return 1;
	return 0;
}

/*
 * dosel - act on a drive selector: uppercase and select the drive.
 */
dosel(arg)
char *arg;
{
	raise(arg);
	select(arg[0] - 'A');
}

/*
 * bfree - free n bytes at block, the SAVE allocator's free entry.  The
 * size becomes n/4 nodes and the block is handed to free.
 */
bfree(block, n)
struct node *block;
ushort n;
{
	block->size = n / 4;
	if (block->size)
		free((char *)block + 4);
}

/*
 * free - return a block to the SAVE allocator's list, coalescing with
 * whatever free block is adjacent.
 *
 * NOTE: high-level; the list walk and coalescing want a closer pass.
 */
free(ptr)
char *ptr;
{
	struct node *p;

	if (ptr == 0)
		return;

	setallo();
	p = (struct node *)(ptr - 4);
	p->next = base.next;
	base.next = p;
}

/*
 * rnum - print a number right-justified in a five-character field,
 * the column the STAT listing uses.
 */
rnum(n)
int n;
{
	char buf[10];
	int len;
	int pad;

	len = itob(buf, n, 10);
	buf[len] = 0;

	pad = 5 - lenstr(buf);
	while (pad-- > 0)
		putch(' ');

	puts(buf);
}

/*
 * di - the DI command, which is just "ignore signals 1 through 14".
 */
di()
{
	int i;

	for (i = 1; i < 0x0F; i++)
		signal(i, 1);
}

/*
 * statdriv - the drive the STAT command is listing, set by dostat.
 */
uchar statdriv;

/*
 * statone - print the STAT line for one file: the drive letter and
 * name, and either the recs/bytes/ext/access for a plain file or the
 * word "Directory"/"Special" for the other kinds.
 */
statone(name)
char *name;
{
	char st[36];
	int recs, bytes, ext, access;

	cpystr(buf, disktab[statdriv], "/", name, 0);

	if (stat(buf, st) < 0)
		return;

	if ((st[5] & 0x60) == 0x40) {
		puts("Directory             ");
		goto printname;
	}
	if ((st[5] & 0x60) != 0) {
		puts("Special               ");
		goto printname;
	}

	recs = (st[9] << 9) | ((st[10] | (st[11] << 8)) >> 7);
	if (st[10] & 0x7F)
		recs++;
	bytes = (st[9] << 6) | ((st[10] | (st[11] << 8)) >> 10);
	if (st[10] | (st[11] & 0x03))
		bytes++;
	ext = (st[9] << 2) | ((st[10] | (st[11] << 8)) >> 14);
	if (st[10] | (st[11] & 0x3F))
		ext++;

	access = access(buf, 2) >= 0;

	rnum(recs); puts(" ");
	rnum(bytes); puts("k ");
	puts("R/");
	rnum(ext); puts(" ");
	puts(access ? "W" : "O");
	puts(" ");

printname:
	putch(statdriv + 'A');
	puts(":");
	raise(name);
	puts(name);
	puts("\r\n");
}

/*
 * dostat - the STAT command.  Expands the wildcard, opens the
 * directory, and prints one line per matching entry via statone; the
 * header line goes out once, before the first.  ^S pauses the listing.
 */
dostat(name)
char *name;
{
	char *p;
	int fd;
	int n;
	int found;
	char c;

	if (*name == 0)
		name = "*.*";

	global(&fcb, name);

	if (fcb.dr != 0)
		statdriv = fcb.dr - 1;
	else
		statdriv = curdriv;

	buildpr(&fcb, buf);
	if (buf[0] == 0)
		cpystr(buf, ".", 0);

	fd = open(buf, 0);
	recavai = 0;
	found = 0;

	while ((n = read(fd, bbuf, 0x200)) > 0) {
		for (p = bbuf; p < bbuf + n; p += 16) {
			if (recavai) {
				read(0, &c, 1);
				recavai = 0;
				if (c == 0x13) {
					read(0, &c, 1);
					recavai = 0;
				}
			}

			if (p[0] | p[1] == 0)
				continue;
			if (cmpstr(".", p + 2) != 0)
				continue;
			if (cmpstr("..", p + 2) != 0)
				continue;
			if (p[15] != 0)
				continue;
			if (!match(&fcb, p))
				continue;

			if (!found)
				puts(" Recs  Bytes  Ext Acc\r\n");
			found = 1;
			statone(p + 2);
		}
	}
	close(fd);

	if (!found)
		puts("NO FILE\r\n");
}

/*
 * devop - assign the list device, the "LST:" command.  An empty
 * argument returns it to the console; a "|command" pipes the listing
 * through a child running the command; anything else is a file the
 * listing is appended to.  desc is the descriptor, dev the saved path.
 */
devop(desc, arg, dev)
uchar *desc;
char *arg;
char **dev;
{
	int fd;
	int pid;

	lflush();

	if (*arg == 0) {
		if (*desc > 2)
			close(*desc);
		*desc = 1;
		if (*dev) {
			free(*dev);
			*dev = 0;
		}
		return;
	}

	if (*arg == '|') {
		arg++;
		if (element('/', arg) == 0) {
			cpystr(bbuf, "/bin/", arg, 0);
			arg = bbuf;
		}
		if (access(arg, 1) < 0) {
			perror(arg);
			puts("\r");
			return;
		}
		pipe(pip);
		pid = fork();
		if (pid < 0) {
			perror(0);
			puts("\r");
			return;
		}
		if (pid == 0) {
			di();
			close(0);
			dup(pip[0]);
			close(pip[0]);
			close(pip[1]);
			execl(arg, arg, 0);
			quest(arg);
			_exit(0);
		}
		close(pip[0]);
		if (*desc > 2)
			close(*desc);
		*desc = pip[1];
		cpystr(buf, "| ", arg, 0);
		if (*dev)
			free(*dev);
		*dev = save(buf);
		return;
	}

	if (!fexists(arg)) {
		fd = creat(arg, 0x1FF);
		close(fd);
	}
	fd = open(arg, 2);
	if (fd < 0) {
		cflush();
		perror(arg);
		puts("\r");
		return;
	}
	if (*desc > 2)
		close(*desc);
	*desc = fd;
	if (*dev)
		free(*dev);
	*dev = save(arg);
}

/*
 * system - the "!" command.  Runs a line as a micronix shell command:
 * save the signal handlers, fork, and let the child exec /bin/sh -c
 * while the parent waits and then puts the handlers back.
 */
system(cmd)
char *cmd;
{
	int sigs[16];
	int pid;
	int status;
	int i;

	for (i = 1; i < 0x0F; i++)
		sigs[i] = signal(i, 1);

	pid = fork();

	if (pid != 0) {
		if (pid != -1) {
			while (wait(&status) != pid)
				;
		}
		for (i = 1; i < 0x0F; i++)
			signal(i, sigs[i]);
		return;
	}

	for (i = 1; i < 0x0F; i++)
		signal(i, sigs[i]);

	execl("/bin/sh", "sh", "-c", cmd, 0);
	perror("sh");
	_exit(0);
}

/*
 * The submit-file state, the CP/M batch mechanism behind the "!" tail
 * of a command.  When a line ends in "!", the rest is saved to /$$$.sub
 * and getline reads from it instead of the keyboard until it is empty.
 */
char subfile[64];
int subfd;
int subflag;
int sublines;

/*
 * getline - the next command line.  From the submit file if one is in
 * progress, else a prompt and a line from the keyboard.
 */
getline()
{
	if (disktab[0] != 0)
		cpystr(subfile, disktab[0], "/$$$.sub", 0);
	else
		subfile[0] = 0;

	if (!subflag) {
		if (subfile[0]) {
			cflush();
			if (stat(subfile, statbuf) >= 0) {
				subflag = 1;
				sublines = (statbuf[10] | (statbuf[11] << 8)) / 128;
				subfd = open(subfile, 0);
				if (subfd < 0)
					sublines = 0;
			}
		}
		if (!subflag)
			goto keyboard;
	}

	if (sublines == 0) {
		unlink(subfile);
		subflag = 0;
		close(subfd);
		goto keyboard;
	}

	sublines--;
	seek(subfd, sublines * 128, 0);
	read(subfd, line + 1, 128);
	line[2 + line[1]] = 0;
	prompt();
	puts(line + 2);
	puts("\r\n");
	return;

keyboard:
	for (;;) {
		prompt();
		line[0] = 0x7F;
		readbuf(line);
		putch('\n');
		cflush();
		if (line[1] != 0)
			break;
	}
	line[2 + line[1]] = 0;
}

/*
 * ---------------------------------------------------------------------
 * The Whitesmith string and stdio helpers.
 *
 * These are library routines, not upm's own: they are what the original
 * binary linked from the Whitesmith C library, and they carry the
 * Whitesmith names - cmpstr not strcmp, cpystr not strcat, and so on.
 * This tree's libc is the Hitech one and does not provide them, so they
 * are written here, read out of the .dis, until libc grows them - the
 * same way init.c carries its helpers.
 *
 * The one that is not obvious is cpystr, the variadic concatenate: it
 * copies its source strings into dst until it reads a null argument,
 * then writes the terminating nul and returns where it finished.  The
 * null argument is what every call ends in.
 */

/*
 * lenstr - the length of a string.  strcmp and strlen under the names
 * the Whitesmith library used, which is why they are here at all.
 */
lenstr(s)
char *s;
{
	int n;

	n = 0;
	while (*s++)
		n++;
	return n;
}

/*
 * fill - fill n bytes of buf with the byte c.
 */
fill(buf, n, c)
char *buf;
int n;
int c;
{
	while (n--)
		*buf++ = c;
}

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
 * cmpstr - compare two strings.  Returns 1 when they are equal and 0
 * when they differ, the opposite of strcmp.
 */
cmpstr(a, b)
char *a;
char *b;
{
	for (;;) {
		if (*a != *b)
			return 0;
		if (*a == 0)
			return 1;
		a++;
		b++;
	}
}

/*
 * cpybuf - copy n bytes from src to dst.
 */
cpybuf(dst, src, n)
char *dst;
char *src;
int n;
{
	while (n--)
		*dst++ = *src++;
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
 * element - is the byte c in the string s.  Returns 1 or 0.
 */
element(c, s)
int c;
char *s;
{
	while (*s) {
		if (c == *s)
			return 1;
		s++;
	}
	return 0;
}

/*
 * itob - the integer into a buffer, most significant digit first.  The
 * caller nul-terminates; the length is returned.  Hex digits (for a base
 * above 10) come out lowercase, though upm only ever asks for base 10.
 */
itob(buf, value, base)
char *buf;
ushort value;
ushort base;
{
	int len;
	int d;

	len = 0;
	if (value / base)
		len = itob(buf, value / base, base);
	d = value % base;
	buf[len] = (d < 10) ? d + '0' : d - 10 + 'a';
	return len + 1;
}

/*
 * signal and stab - libu's, moved here so that _stab is resident: the
 * trampolines in upmsys.s read it while a .com runs, and libu's signal.o
 * would put it in the data/bss segment at 0x0100, where the .com clobbers
 * it.  _signal is the syscall stub in upmsys.s.
 */
short stab[15];

extern struct tramp {
	char v[6];
} jtab[];

signal(sig, handler)
int sig;
int handler;
{
	int ret;

	if (sig < 1 || sig > 15)
		return -1;

	if (handler == 0 || handler == 1) {
		ret = _signal(sig, handler);
	} else {
		stab[sig - 1] = handler;
		ret = _signal(sig, (int)&jtab[sig - 1]);
	}

	if (!(ret == 1 || ret == 0 || ret == -1))
		ret = stab[sig - 1];
	return ret;
}
