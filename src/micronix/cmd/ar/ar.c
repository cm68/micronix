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

char	*man	=	{ "mrxtdpq" };
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
	cp = argv[1];
	for(cp = argv[1]; *cp; cp++)
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
	printf("%3d/%1d", arbuf.ar_uid, arbuf.ar_gid);
	printf("%7D", arbuf.ar_size);
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
