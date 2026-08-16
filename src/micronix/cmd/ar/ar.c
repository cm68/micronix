#include <stdio.h>
#ifdef linux
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#else
/*
 * The micronix headers, not ccc's.  ccc's <stat.h> is the CP/M struct
 * stat - mode, three times and size, and nothing else - so st_uid and
 * st_gid do not exist in it, and its <signal.h> is the CP/M signal set
 * without SIGHUP or SIGQUIT.  The sys/ names do not collide with
 * anything ccc installs, so they resolve out of the tree's own
 * include directory.  stat.h needs UINT out of types.h and struct
 * dsknod out of sys/fs.h, and says so at the top of itself.
 */
#include <types.h>
#include <sys/fs.h>
#include <sys/stat.h>
#include <sys/signal.h>
#endif

/*
 * ar_hdr and ARMAG live in ar.h beside this file, which was never
 * included - gcc let it pass because struct ar_hdr was completed by
 * the time it mattered and ARMAG happened to be an int.
 */
#include "ar.h"

struct	stat	stbuf;

int magic;
#define	V7ARMAG	0177545
#define	V6ARMAG	0177555
#define	WSARMAG	0177565
#define	HIARMAG	022372

struct	v7ar_hdr {
	char	ar_name[14];
	long	ar_date;
	char	ar_uid;
	char	ar_gid;
	int		ar_mode;
	long	ar_size;
};

struct	ar_hdr	arbuf;

#define	SKIP	1
#define	IODD	2
#define	OODD	4
#define	HEAD	8

char	*man	=	{ "mrxtdpqs" };
char	*opt	=	{ "uvnbail" };

int	signum[] = {SIGHUP, SIGINT, SIGQUIT, 0};
int	sigdone();
long	lseek();
int	rcmd();
int	dcmd();
int	xcmd();
int	tcmd();
int	pcmd();
int	mcmd();
int	qcmd();
int	scmd();
int	(*comfun)();
char	flg[26];
char	**namv;
int	namc;
char	*arnam;
char	*ponam;
/*
 * ARRAYS, not pointers to literals.  mktemp() writes the XXXXX in
 * place, and a literal is writable on micronix and read only here -
 * so a host ar segfaulted before it had created anything.  The -l
 * flag below rewrites these to sit in the current directory, which is
 * a strcpy now and not an assignment.
 */
char	tmpltnam[16]	=	"/tmp/vXXXXXX";
char	tmplt1nam[16]	=	"/tmp/v1XXXXXX";
char	tmp2nam[16]	=	"/tmp/v2XXXXXX";
char	*tfnam;
char	*tf1nam;
char	*tf2nam;
char	*file;
char	name[16];
int	af;
int	tf;
int	tf1;
int	tf2;
int	qf;
int	bastate;
char	buf[512];

char	*trim();
char	*mktemp();
char	*ctime();

main(argc, argv)
char *argv[];
{
	register i;
	register char *cp;

	for(i=0; signum[i]; i++)
		if(signal(signum[i], SIG_IGN) != SIG_IGN)
			signal(signum[i], sigdone);
	if(argc < 3)
		usage();
	/*
	 * The key, with or without a leading dash.
	 *
	 * v6 ar takes it bare - "ar cr lib obj..." - and everything
	 * written since takes "-cr" as well, which is what a makefile
	 * that used to call some other librarian will already say.
	 * Accepting both costs one line and saves the reader of a
	 * makefile from having to know which era it is in.
	 */
	cp = argv[1];
	if(*cp == '-')
		cp++;
	for(; *cp; cp++)
	switch(*cp) {
	case 'l':
	case 'v':
	case 'u':
	case 'n':
	case 'a':
	case 'b':
	case 'c':
	case 'i':
		flg[*cp - 'a']++;
		continue;

	case 'r':
		setcom(rcmd);
		continue;

	case 'd':
		setcom(dcmd);
		continue;

	case 'x':
		setcom(xcmd);
		continue;

	case 't':
		setcom(tcmd);
		continue;

	case 'p':
		setcom(pcmd);
		continue;

	case 'm':
		setcom(mcmd);
		continue;

	case 'q':
		setcom(qcmd);
		continue;
	case 's':
		setcom(scmd);
		continue;

	default:
		fprintf(stderr, "ar: bad option `%c'\n", *cp);
		done(1);
	}
	if(flg['l'-'a']) {
		strcpy(tmpltnam, "vXXXXXX");
		strcpy(tmplt1nam, "v1XXXXXX");
		strcpy(tmp2nam, "v2XXXXXX");
		}
	if(flg['i'-'a'])
		flg['b'-'a']++;
	if(flg['a'-'a'] || flg['b'-'a']) {
		bastate = 1;
		ponam = trim(argv[2]);
		argv++;
		argc--;
		if(argc < 3)
			usage();
	}
	arnam = argv[2];
	namv = argv+3;
	namc = argc-3;
	if(comfun == 0) {
		if(flg['u'-'a'] == 0) {
			fprintf(stderr, "ar: one of [%s] must be specified\n", man);
			done(1);
		}
		setcom(rcmd);
	}
	(*comfun)();
	done(notfound());
}

setcom(fun)
int (*fun)();
{

	if(comfun != 0) {
		fprintf(stderr, "ar: only one of [%s] allowed\n", man);
		done(1);
	}
	comfun = fun;
}

rcmd()
{
	register f;

	init();
	getaf();
	while(!getdir()) {
		bamatch();
		if(namc == 0 || match()) {
			f = stats();
			if(f < 0) {
				if(namc)
					fprintf(stderr, "ar: cannot open %s\n", file);
				goto cp;
			}
			if(flg['u'-'a'])
				if(stbuf.st_mtime <= arbuf.ar_date) {
					close(f);
					goto cp;
				}
			mesg('r');
			copyfil(af, -1, IODD+SKIP);
			movefil(f);
			continue;
		}
	cp:
		mesg('c');
		copyfil(af, tf, IODD+OODD+HEAD);
	}
	cleanup();
}

dcmd()
{

	init();
	if(getaf())
		noar();
	while(!getdir()) {
		if(match()) {
			mesg('d');
			copyfil(af, -1, IODD+SKIP);
			continue;
		}
		mesg('c');
		copyfil(af, tf, IODD+OODD+HEAD);
	}
	install();
}

xcmd()
{
	register f;

	if(getaf())
		noar();
	while(!getdir()) {
		if(namc == 0 || match()) {
			f = creat(file, arbuf.ar_mode & 0777);
			if(f < 0) {
				fprintf(stderr, "ar: %s cannot create\n", file);
				goto sk;
			}
			mesg('x');
			copyfil(af, f, IODD);
			close(f);
			continue;
		}
	sk:
		mesg('c');
		copyfil(af, -1, IODD+SKIP);
		if (namc > 0  &&  !morefil())
			done(0);
	}
}

pcmd()
{

	if(getaf())
		noar();
	while(!getdir()) {
		if(namc == 0 || match()) {
			if(flg['v'-'a']) {
				printf("\n<%s>\n\n", file);
				fflush(stdout);
			}
			copyfil(af, 1, IODD);
			continue;
		}
		copyfil(af, -1, IODD+SKIP);
	}
}

mcmd()
{

	init();
	if(getaf())
		noar();
	tf2nam = mktemp(tmp2nam);
	close(creat(tf2nam, 0600));
	tf2 = open(tf2nam, 2);
	if(tf2 < 0) {
		fprintf(stderr, "ar: cannot create third temp\n");
		done(1);
	}
	while(!getdir()) {
		bamatch();
		if(match()) {
			mesg('m');
			copyfil(af, tf2, IODD+OODD+HEAD);
			continue;
		}
		mesg('c');
		copyfil(af, tf, IODD+OODD+HEAD);
	}
	install();
}

tcmd()
{

	if(getaf())
		noar();
	while(!getdir()) {
		if(namc == 0 || match()) {
			if(flg['v'-'a'])
				longt();
			printf("%s\n", trim(file));
		}
		copyfil(af, -1, IODD+SKIP);
	}
}

qcmd()
{
	register i, f;

	if (flg['a'-'a'] || flg['b'-'a']) {
		fprintf(stderr, "ar: abi not allowed with q\n");
		done(1);
	}
	getqf();
	for(i=0; signum[i]; i++)
		signal(signum[i], SIG_IGN);
	lseek(qf, 0l, 2);
	for(i=0; i<namc; i++) {
		file = namv[i];
		if(file == 0)
			continue;
		namv[i] = 0;
		mesg('q');
		f = stats();
		if(f < 0) {
			fprintf(stderr, "ar: %s cannot open\n", file);
			continue;
		}
		tf = qf;
		movefil(f);
		qf = tf;
	}
	close(qf);
	qf = -1;
	ranlib();
}

/*
 * The header is 26 bytes on disk, and these are what make it so.
 *
 * It used to be read and written as a struct, which put the compiler
 * in charge of the file format.  sizeof(int) is 2 here and 4 on the
 * host; sizeof(long) is 4 here and 8 there; and a host build pads the
 * struct besides.  Two builds of this one file produced archives the
 * other could not read.  That did not matter while ar was only ever a
 * guest program, and it matters now that it is to be the librarian
 * for the cross build as well - the library a host ar writes has to
 * be the library the linker reads on either side.
 *
 * So every field goes out a byte at a time, low byte first, which is
 * the order a z80 stores them in and the order the archives already
 * on disk are written in.  wslib has always done it this way and that
 * is exactly why it works as a cross tool.
 *
 *	name	14	as typed, null padded
 *	date	 4
 *	uid	 1
 *	gid	 1
 *	mode	 2
 *	size	 4
 */
#define	AR_HDRSIZ	26

/*
 * The symbol index, which is what ranlib(1) makes and what "ar s"
 * makes here.
 *
 * It is an ordinary member and it is always the first one, named
 * __.SYMDEF as v7 names it.  A linker that does not know about it
 * reads it as a member whose contents are not an object, skips it,
 * and finds everything else where it expects; that is the whole
 * reason for putting the index IN the archive rather than beside it.
 *
 * The contents are ours, not v7's.  v7 writes pairs of longs - an
 * offset and an index into a string table - which costs eight bytes a
 * symbol and needs long arithmetic to walk.  This is a sixteen bit
 * machine and the archive that needs indexing most is 36K, so:
 *
 *	2 bytes		count of entries
 *	per entry:
 *	  3 bytes	offset of the member's HEADER from byte 0
 *	  n bytes	the symbol's name, NUL terminated
 *
 * Five bytes plus the name.  The offset points at the header and not
 * at the object inside it, so a reader seeks there and carries on
 * exactly as it would have done had it walked the archive itself.
 *
 * THREE bytes and not two.  Two was the first shape of this and it
 * capped a usable archive at 64K, which was not a comfortable margin:
 * libc.a was 40K the day it went in, 61 per cent of the way there and
 * still growing.  Two was chosen to keep the arithmetic inside a
 * sixteen bit machine's register; three keeps that too - it is a load
 * and two shifts, no long anywhere - and moves the ceiling to 16M,
 * which this format will not reach.  It costs one byte a symbol: 253
 * of them on libc.a, against a limit that stops being thought about.
 *
 * Four bytes is what v7 uses and would need long arithmetic on the
 * side that reads it, which is the machine with the least room for it.
 *
 * If an archive somehow passes 16M, no index is written at all and the
 * archive stays correct - a linker falls back to reading every member,
 * which is what it did before there were indexes.  Being slow is a
 * fair price; being wrong is not.
 */
#define	SYMDEF	"__.SYMDEF"
#define	ARMAXOFF 16777215L

/*
 * Two things out of the object format, spelled here rather than by
 * including wsobj.h: that header is the linker's and brings a great
 * deal else with it, and these are the only two an archiver needs to
 * know to tell an object from anything else in the archive.  They are
 * MAGIC and CONF_SYMASK there, and must agree.
 */
#define	OBJMAGIC	0x99
#define	CONF_SYMASK	0x07

putfield(p, v, n)
char *p;
long v;
{
	while (n--) {
		*p++ = v & 0xff;
		v >>= 8;
	}
}

long
getfield(p, n)
char *p;
{
	long v;

	v = 0;
	while (--n >= 0)
		v = (v << 8) | (p[n] & 0xff);
	return (v);
}

/*
 * the two byte magic, likewise
 */
putmag(fd, mag)
{
	char b[2];

	b[0] = mag & 0xff;
	b[1] = (mag >> 8) & 0xff;
	if (write(fd, b, 2) != 2)
		wrerr();
}

getmag(fd)
{
	char b[2];

	if (read(fd, b, 2) != 2)
		return (-1);
	return ((b[0] & 0xff) | ((b[1] & 0xff) << 8));
}

puthdr(fd)
{
	char b[AR_HDRSIZ];
	register i;

	for (i = 0; i < 14; i++)
		b[i] = arbuf.ar_name[i];
	putfield(&b[14], arbuf.ar_date, 4);
	b[18] = arbuf.ar_uid;
	b[19] = arbuf.ar_gid;
	putfield(&b[20], (long)arbuf.ar_mode, 2);
	putfield(&b[22], arbuf.ar_size, 4);
	if (write(fd, b, AR_HDRSIZ) != AR_HDRSIZ)
		wrerr();
}

gethdr(fd)
{
	char b[AR_HDRSIZ];
	register i;

	if (read(fd, b, AR_HDRSIZ) != AR_HDRSIZ)
		return (0);
	for (i = 0; i < 14; i++)
		arbuf.ar_name[i] = b[i];
	arbuf.ar_date = getfield(&b[14], 4);
	arbuf.ar_uid = b[18];
	arbuf.ar_gid = b[19];
	arbuf.ar_mode = getfield(&b[20], 2);
	arbuf.ar_size = getfield(&b[22], 4);
	return (AR_HDRSIZ);
}

init()
{

	tfnam = mktemp(tmpltnam);
	close(creat(tfnam, 0600));
	tf = open(tfnam, 2);
	if(tf < 0) {
		fprintf(stderr, "ar: cannot create temp file\n");
		done(1);
	}
	putmag(tf, ARMAG);
}

getaf()
{
	af = open(arnam, 0);
	if(af < 0)
		return(1);
	if (getmag(af) != ARMAG) {
		fprintf(stderr, "ar: %s not in archive format\n", arnam);
		done(1);
	}
	return(0);
}

getqf()
{
	if ((qf = open(arnam, 2)) < 0) {
		if(!flg['c'-'a'])
			fprintf(stderr, "ar: creating %s\n", arnam);
		close(creat(arnam, 0666));
		if ((qf = open(arnam, 2)) < 0) {
			fprintf(stderr, "ar: cannot create %s\n", arnam);
			done(1);
		}
		putmag(qf, ARMAG);
	}
	else if (getmag(qf) != ARMAG) {
		fprintf(stderr, "ar: %s not in archive format\n", arnam);
		done(1);
	}
}

usage()
{
	printf("usage: ar [%s][%s] archive files ...\n", opt, man);
	done(1);
}

noar()
{

	fprintf(stderr, "ar: %s does not exist\n", arnam);
	done(1);
}

sigdone()
{
	done(100);
}

done(c)
{

	if(tfnam)
		unlink(tfnam);
	if(tf1nam)
		unlink(tf1nam);
	if(tf2nam)
		unlink(tf2nam);
	exit(c);
}

notfound()
{
	register i, n;

	n = 0;
	for(i=0; i<namc; i++)
		if(namv[i]) {
			fprintf(stderr, "ar: %s not found\n", namv[i]);
			n++;
		}
	return(n);
}

morefil()
{
	register i, n;

	n = 0;
	for(i=0; i<namc; i++)
		if(namv[i])
			n++;
	return(n);
}

/*
 * Read one little endian word out of a buffer.  The object header and
 * its symbol table are all sixteen bit fields; getfield() beside this
 * does the same for the archive header's own, which are four byte and
 * signed, so they do not serve for each other.
 */
arword(p)
char *p;
{
	return ((p[0] & 0xff) | ((p[1] & 0xff) << 8));
}

/*
 * Walk one member's symbol table, calling back for each DEFINED
 * GLOBAL.  Returns 0 if the member is not an object at all, which is
 * how __.SYMDEF itself and anything else in the archive is passed
 * over.
 *
 * The layout is the one ld reads: a sixteen byte header, then text,
 * then data, then the symbol table.  An entry is a two byte value, a
 * type byte and the name, and the name's length comes out of the
 * config byte, so the table's stride is per object and not a
 * constant.  A symbol counts when it is both defined - type bits 0-2
 * naming a segment rather than nothing - and global.
 */
arsyms(fd, base, size, fn, arg)
long base, size;
int (*fn)();
char *arg;
{
	char hdr[16];
	char ent[20];
	int symlen, stride, nsym, i;
	long symtab, text, data, off;

	if (size < 16L)
		return (0);
	if (lseek(fd, base, 0) < 0)
		return (0);
	if (read(fd, hdr, 16) != 16)
		return (0);
	if ((hdr[0] & 0xff) != OBJMAGIC)
		return (0);

	symlen = (hdr[1] & CONF_SYMASK) * 2 + 1;
	stride = symlen + 3;
	symtab = arword(&hdr[2]) & 0xffffL;
	text = arword(&hdr[4]) & 0xffffL;
	data = arword(&hdr[6]) & 0xffffL;
	nsym = symtab / stride;
	off = base + 16L + text + data;

	if (stride > sizeof(ent) - 1)
		return (0);		/* a symlen this file cannot hold */

	for (i = 0; i < nsym; i++) {
		if (lseek(fd, off, 0) < 0)
			return (0);
		if (read(fd, ent, stride) != stride)
			return (0);
		off += stride;
		/*
		 * bit 2 says defined, bit 3 says global.  ld spells the
		 * first as "the segment is not SEG_EXT" and arrives at
		 * the same place.
		 */
		if ((ent[2] & 0x04) && (ent[2] & 0x08)) {
			ent[3 + symlen] = '\0';
			(*fn)(arg, &ent[3]);
		}
	}
	return (1);
}

/*
 * The two callbacks arsyms() drives, and the state they fill.
 *
 * Two passes over the archive: the first only counts, so that the
 * exact amount of memory can be asked for rather than a limit
 * invented; the second writes the names down.  A librarian on a
 * sixty-four kilobyte machine cannot afford a table sized for the
 * worst archive anyone might ever build.
 */
int	rlnsym;			/* symbols seen */
long	rlnamsz;		/* bytes their names take, NULs included */
char	*rlnames;		/* the names, end to end */
char	*rlnp;			/* fills rlnames on the second pass */
int	*rlmemb;		/* which member each symbol came from */
int	rlmi;			/* the member being scanned */

/*
 * "ar s archive" - build the index and nothing else, which is what
 * ranlib(1) is.  The other commands do it for themselves at the end,
 * so this is for an archive that arrived from somewhere that does
 * not, and for putting one back after something has been done to it
 * by hand.
 */
scmd()
{
	ranlib();
}

rlcount(arg, name)
char *arg, *name;
{
	rlnsym++;
	rlnamsz += strlen(name) + 1;
}

rlgather(arg, name)
char *arg, *name;
{
	rlmemb[rlnsym] = rlmi;
	strcpy(rlnp, name);
	rlnp += strlen(name) + 1;
	rlnsym++;
}

/*
 * Build the symbol index: ranlib, and "ar s".
 *
 * Runs over the FINISHED archive rather than being woven into the
 * commands that write it, which is what ranlib has always been and
 * what keeps r, d, m and q from having to know anything about it.
 * They rebuild the archive; this rebuilds the index over the top.
 *
 * Three walks.  One to count the symbols and the members, one to
 * collect the names, and one to copy the archive out with the index
 * in front.  The offsets cannot be known until the index's own size
 * is, since it sits at the front and moves everything behind it.
 */
ranlib()
{
	int fd, ofd, i, n, nmemb;
	long off, size, idxsz, base;
	long *msize;		/* each member, header and data and pad */
	long *moff;		/* and where it lands in the new archive */
	/*
	 * Its own template.  mktemp() substitutes IN PLACE, so the one
	 * init() used has had its XXXXXX eaten by the time a command
	 * that rebuilds the archive gets here, and asking again gives
	 * back the name already in use - "cannot create temp file",
	 * from a librarian that had just written the archive
	 * successfully.
	 */
	char tmpl[16];
	char *tname;
	char hb[AR_HDRSIZ];

	if ((fd = open(arnam, 0)) < 0) {
		fprintf(stderr, "ar: cannot open %s\n", arnam);
		return;
	}
	if (getmag(fd) != ARMAG) {
		fprintf(stderr, "ar: %s not in archive format\n", arnam);
		close(fd);
		return;
	}

	/*
	 * Walk one: how many members, how many symbols, how long are
	 * the names.  An existing __.SYMDEF is passed over here and
	 * never copied, so running this twice does not stack indexes.
	 */
	nmemb = 0;
	rlnsym = 0;
	rlnamsz = 0;
	off = 2;
	while (lseek(fd, off, 0) >= 0 && gethdr(fd) == AR_HDRSIZ) {
		size = arbuf.ar_size;
		base = off + AR_HDRSIZ;
		if (strncmp(arbuf.ar_name, SYMDEF, 14) != 0) {
			nmemb++;
			arsyms(fd, base, size, rlcount, (char *)0);
		}
		off = base + size + (size & 1);
	}
	if (nmemb == 0) {
		close(fd);
		return;
	}

	idxsz = 2 + (long)rlnsym * 3 + rlnamsz;

	msize = (long *)malloc((nmemb + 1) * sizeof(long));
	moff = (long *)malloc((nmemb + 1) * sizeof(long));
	rlnames = malloc((int)rlnamsz + 1);
	rlmemb = (int *)malloc((rlnsym + 1) * sizeof(int));
	if (!msize || !moff || !rlnames || !rlmemb) {
		fprintf(stderr, "ar: out of memory for the index\n");
		close(fd);
		return;
	}

	/*
	 * Walk two: the names, and each member's size, and from those
	 * the offset it will have once the index is in front of it.
	 */
	rlnp = rlnames;
	rlnsym = 0;
	rlmi = 0;
	off = 2;
	base = 2 + AR_HDRSIZ + idxsz + (idxsz & 1);
	lseek(fd, 2L, 0);
	while (lseek(fd, off, 0) >= 0 && gethdr(fd) == AR_HDRSIZ) {
		size = arbuf.ar_size;
		if (strncmp(arbuf.ar_name, SYMDEF, 14) != 0) {
			msize[rlmi] = AR_HDRSIZ + size + (size & 1);
			moff[rlmi] = base;
			base += msize[rlmi];
			arsyms(fd, off + AR_HDRSIZ, size, rlgather, (char *)0);
			rlmi++;
		}
		off += AR_HDRSIZ + size + (size & 1);
	}

	/*
	 * If the finished archive would not fit in sixteen bits there
	 * is no index to be had.  Say so once and leave the archive
	 * exactly as it was: correct, and read the slow way.
	 */
	if (base > ARMAXOFF) {
		fprintf(stderr,
		    "ar: %s is %ld bytes, past 16M; no index written\n",
		    arnam, base);
		close(fd);
		return;
	}

	/*
	 * Walk three: out it goes, index first.
	 */
	strcpy(tmpl, tmpltnam[0] == '/' ? "/tmp/sXXXXXX" : "sXXXXXX");
	tname = mktemp(tmpl);
	close(creat(tname, 0600));
	if ((ofd = open(tname, 2)) < 0) {
		fprintf(stderr, "ar: cannot create temp file\n");
		close(fd);
		return;
	}
	putmag(ofd, ARMAG);

	for (i = 0; i < 14; i++)
		arbuf.ar_name[i] = i < strlen(SYMDEF) ? SYMDEF[i] : '\0';
	/*
	 * The archive's own mtime, not zero - which showed as 1969 in
	 * "ar tv" and is the sort of thing that makes a reader wonder
	 * what else is wrong.
	 *
	 * v7's ranlib put a date here so a linker could compare it with
	 * the archive's and warn when the index was older, which is the
	 * one thing this cannot be: it is rebuilt by every command that
	 * writes the archive.  So the date is for a person reading a
	 * listing and for nothing else, and the archive's own is the
	 * honest answer to "when was this made".
	 */
	arbuf.ar_date = stat(arnam, &stbuf) < 0 ? 0 : stbuf.st_mtime;
	arbuf.ar_uid = 0;
	arbuf.ar_gid = 0;
	arbuf.ar_mode = 0444;
	arbuf.ar_size = idxsz;
	puthdr(ofd);

	hb[0] = rlnsym & 0xff;
	hb[1] = (rlnsym >> 8) & 0xff;
	if (write(ofd, hb, 2) != 2)
		wrerr();
	rlnp = rlnames;
	for (n = 0; n < rlnsym; n++) {
		hb[0] = moff[rlmemb[n]] & 0xff;
		hb[1] = (moff[rlmemb[n]] >> 8) & 0xff;
		hb[2] = (moff[rlmemb[n]] >> 16) & 0xff;
		if (write(ofd, hb, 3) != 3)
			wrerr();
		i = strlen(rlnp) + 1;
		if (write(ofd, rlnp, i) != i)
			wrerr();
		rlnp += i;
	}
	if (idxsz & 1) {
		hb[0] = '\0';
		if (write(ofd, hb, 1) != 1)
			wrerr();
	}

	/* and the members, in the order they were already in */
	off = 2;
	lseek(fd, 2L, 0);
	while (lseek(fd, off, 0) >= 0 && gethdr(fd) == AR_HDRSIZ) {
		size = arbuf.ar_size;
		if (strncmp(arbuf.ar_name, SYMDEF, 14) != 0) {
			puthdr(ofd);
			lseek(fd, off + AR_HDRSIZ, 0);
			for (size += (size & 1); size > 0; size -= n) {
				n = size > 512 ? 512 : size;
				if (read(fd, buf, n) != n)
					break;
				if (write(ofd, buf, n) != n)
					wrerr();
			}
			size = arbuf.ar_size;
		}
		off += AR_HDRSIZ + size + (size & 1);
	}
	close(fd);

	/* over the top of the original */
	if ((fd = creat(arnam, 0666)) < 0) {
		fprintf(stderr, "ar: cannot rewrite %s\n", arnam);
		close(ofd);
		unlink(tname);
		return;
	}
	lseek(ofd, 0L, 0);
	while ((n = read(ofd, buf, 512)) > 0)
		if (write(fd, buf, n) != n)
			wrerr();
	close(fd);
	close(ofd);
	unlink(tname);

	if (flg['v'-'a'])
		fprintf(stderr, "ar: %d symbols in %d members\n",
		    rlnsym, nmemb);
}

cleanup()
{
	register i, f;

	for(i=0; i<namc; i++) {
		file = namv[i];
		if(file == 0)
			continue;
		namv[i] = 0;
		mesg('a');
		f = stats();
		if(f < 0) {
			fprintf(stderr, "ar: %s cannot open\n", file);
			continue;
		}
		movefil(f);
	}
	install();
}

install()
{
	register i;

	for(i=0; signum[i]; i++)
		signal(signum[i], SIG_IGN);
	if(af < 0)
		if(!flg['c'-'a'])
			fprintf(stderr, "ar: creating %s\n", arnam);
	close(af);
	af = creat(arnam, 0666);
	if(af < 0) {
		fprintf(stderr, "ar: cannot create %s\n", arnam);
		done(1);
	}
	if(tfnam) {
		lseek(tf, 0l, 0);
		while((i = read(tf, buf, 512)) > 0)
			if (write(af, buf, i) != i)
				wrerr();
	}
	if(tf2nam) {
		lseek(tf2, 0l, 0);
		while((i = read(tf2, buf, 512)) > 0)
			if (write(af, buf, i) != i)
				wrerr();
	}
	if(tf1nam) {
		lseek(tf1, 0l, 0);
		while((i = read(tf1, buf, 512)) > 0)
			if (write(af, buf, i) != i)
				wrerr();
	}
	close(af);
	af = -1;

	/*
	 * The archive has just been rewritten, so whatever index it had
	 * describes the old one.  Rebuild it HERE, at the one place all
	 * of r, d and m come out through, rather than in each of them:
	 * it was in cleanup() to begin with, which is rcmd's alone, so
	 * delete and move left a stale index behind - and an index that
	 * is WRONG is worse than none, because a linker believes it and
	 * takes whatever member the offset lands on.
	 *
	 * q does not come through here; it appends to the archive in
	 * place and calls ranlib() for itself.
	 */
	ranlib();
}

/*
 * insert the file 'file'
 * into the temporary file
 */
movefil(f)
{
	register char *cp;
	register i;

	cp = trim(file);
	for(i=0; i<14; i++)
		if(arbuf.ar_name[i] = *cp)
			cp++;
	/*
	 * micronix keeps a file size in three bytes, not four: a high
	 * byte and a low word, and there is no st_size to read.  The
	 * cast is load bearing - int is sixteen bits here, so shifting
	 * the high byte up without widening it first would shift it
	 * away entirely and every archive member would be recorded at
	 * its size modulo 64K.
	 */
#ifdef linux
	arbuf.ar_size = stbuf.st_size;
#else
	arbuf.ar_size = ((long)stbuf.st_size0 << 16) + stbuf.st_size1;
#endif
	arbuf.ar_date = stbuf.st_mtime;
	arbuf.ar_uid = stbuf.st_uid;
	arbuf.ar_gid = stbuf.st_gid;
	arbuf.ar_mode = stbuf.st_mode;
	copyfil(f, tf, OODD+HEAD);
	close(f);
}

stats()
{
	register f;

	f = open(file, 0);
	if(f < 0)
		return(f);
	if(fstat(f, &stbuf) < 0) {
		close(f);
		return(-1);
	}
	return(f);
}

/*
 * copy next file
 * size given in arbuf
 */
copyfil(fi, fo, flag)
{
	register i, o;
	int pe;

	if(flag & HEAD)
		puthdr(fo);
	pe = 0;
	while(arbuf.ar_size > 0) {
		i = o = 512;
		if(arbuf.ar_size < i) {
			i = o = arbuf.ar_size;
			if(i&1) {
				if(flag & IODD)
					i++;
				if(flag & OODD)
					o++;
			}
		}
		if(read(fi, buf, i) != i)
			pe++;
		if((flag & SKIP) == 0)
			if (write(fo, buf, o) != o)
				wrerr();
		arbuf.ar_size -= 512;
	}
	if(pe)
		phserr();
}

getdir()
{
	register i;

	i = gethdr(af);
	if(i != AR_HDRSIZ) {
		if(tf1nam) {
			i = tf;
			tf = tf1;
			tf1 = i;
		}
		return(1);
	}
	for(i=0; i<14; i++)
		name[i] = arbuf.ar_name[i];
	file = name;
	return(0);
}

match()
{
	register i;

	for(i=0; i<namc; i++) {
		if(namv[i] == 0)
			continue;
		if(strcmp(trim(namv[i]), file) == 0) {
			file = namv[i];
			namv[i] = 0;
			return(1);
		}
	}
	return(0);
}

bamatch()
{
	register f;

	switch(bastate) {

	case 1:
		if(strcmp(file, ponam) != 0)
			return;
		bastate = 2;
		if(flg['a'-'a'])
			return;

	case 2:
		bastate = 0;
		tf1nam = mktemp(tmplt1nam);
		close(creat(tf1nam, 0600));
		f = open(tf1nam, 2);
		if(f < 0) {
			fprintf(stderr, "ar: cannot create second temp\n");
			return;
		}
		tf1 = tf;
		tf = f;
	}
}

phserr()
{

	fprintf(stderr, "ar: phase error on %s\n", file);
}

mesg(c)
{

	if(flg['v'-'a'])
		if(c != 'c' || flg['v'-'a'] > 1)
			printf("%c - %s\n", c, file);
}

char *
trim(s)
char *s;
{
	register char *p1, *p2;

	for(p1 = s; *p1; p1++)
		;
	while(p1 > s) {
		if(*--p1 != '/')
			break;
		*p1 = 0;
	}
	p2 = s;
	for(p1 = s; *p1; p1++)
		if(*p1 == '/')
			p2 = p1+1;
	return(p2);
}

#define	IFMT	060000
#define	ISARG	01000
#define	LARGE	010000
#define	SUID	04000
#define	SGID	02000
#define	ROWN	0400
#define	WOWN	0200
#define	XOWN	0100
#define	RGRP	040
#define	WGRP	020
#define	XGRP	010
#define	ROTH	04
#define	WOTH	02
#define	XOTH	01
#define	STXT	01000

longt()
{
	register char *cp;

	pmode();
	/*
	 * The uid and gid are one byte each in the header and the
	 * struct declares them char, so a uid above 127 came out
	 * negative - "-24/-24" for 1000, which is 232 truncated and
	 * then read back signed.  Masked here rather than retyped,
	 * because the field's width is the FORMAT's and not ours.
	 */
	printf("%3d/%1d", arbuf.ar_uid & 0xff, arbuf.ar_gid & 0xff);
	/*
	 * %D is v7's spelling of a long, and this tree's own libc still
	 * takes it - see doprnt.c, which has 'D' beside 'd'.  The host's
	 * printf does not, and printed the format itself: "%7D" where
	 * the size should be.  This file is built both ways, micronix's
	 * cmd/ar being a symlink to it, so it needs a spelling both
	 * understand.
	 *
	 * An int, not a long.  The size FIELD is four bytes because the
	 * v7 format says so, but the value cannot be: a Z80 object is
	 * addressed with sixteen bits and nothing that goes in one of
	 * these archives reaches 64K.  Printing it as an int keeps long
	 * formatting out of the librarian on the machine that has the
	 * least room for it.
	 */
	printf("%7d", (int) arbuf.ar_size);
	cp = ctime(&arbuf.ar_date);
	printf(" %-12.12s %-4.4s ", cp+4, cp+20);
}

int	m1[] = { 1, ROWN, 'r', '-' };
int	m2[] = { 1, WOWN, 'w', '-' };
int	m3[] = { 2, SUID, 's', XOWN, 'x', '-' };
int	m4[] = { 1, RGRP, 'r', '-' };
int	m5[] = { 1, WGRP, 'w', '-' };
int	m6[] = { 2, SGID, 's', XGRP, 'x', '-' };
int	m7[] = { 1, ROTH, 'r', '-' };
int	m8[] = { 1, WOTH, 'w', '-' };
int	m9[] = { 2, STXT, 't', XOTH, 'x', '-' };

int	*m[] = { m1, m2, m3, m4, m5, m6, m7, m8, m9};

pmode()
{
	register int **mp;

	for (mp = &m[0]; mp < &m[9];)
		arselect(*mp++);
}

arselect(pairp)
int *pairp;
{
	register int n, *ap;

	ap = pairp;
	n = *ap++;
	while (--n>=0 && (arbuf.ar_mode&*ap++)==0)
		ap++;
	putchar(*ap);
}

wrerr()
{
	perror("ar write error");
	done(1);
}
