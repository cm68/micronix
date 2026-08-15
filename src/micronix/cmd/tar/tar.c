/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 * Tape Archival Program
 *
 * 2.11BSD tar (tar.c 5.7, Berkeley 4/26/86), ported to micronix.
 *
 * cmd/tar/tar.c
 *
 * What the port took out, and why - each of these is a thing the
 * machine does not have, not a thing the program did not need:
 *
 *	symbolic links	micronix has none, so the S_IFLNK case, lstat
 *			and the h flag are gone.  Extracting a tape
 *			that carries one prints what it was and moves
 *			on; the entry is header-only, so there is
 *			nothing to skip.
 *
 *	the u flag	update leaned on "sort | awk" through system(),
 *			and neither is on this machine.  The r flag
 *			(append) stays; it never needed them.
 *
 *	the m flag	m meant "do not restore mtimes", and there is
 *			no utime or smdate call to restore them with,
 *			so m is the only behavior there is.  setimes
 *			and the directory-mtime stack went with it.
 *
 *	mag tape ioctls	MTIOCTOP and friends; backtape is an lseek
 *			now.  The /dev/rmt digit options went too -
 *			name the archive with f, or it is /dev/mt0.
 *
 * And what it had to say differently:
 *
 *	st_size		the micronix inode splits the size into a high
 *			byte and a low word; fsize() puts them back
 *			together, and hsize carries the size decoded
 *			from a tape header, which never was a stat.
 *
 *	chown		takes the owner packed as uid | gid << 8, and
 *			only the super-user may make the call, so its
 *			failure is ignored the same way BSD ignored it.
 *
 *	getcwd		not in the library; the one at the bottom of
 *			this file walks the .. chain matching inodes,
 *			the way pwd does, without moving the process.
 *
 *	telldir/seekdir	added to libc's opendir for this program: tar
 *			closes the directory while it recurses and
 *			seeks back after, which on a system with 16
 *			file slots is worth keeping.
 *
 * vim: tabstop=8 shiftwidth=8 noexpandtab:
 */

#include <types.h>
#include <stdio.h>
#include <sys/fs.h>
#include <sys/stat.h>
#include <sys/dir.h>
#include <dirent.h>
#include <sys/signal.h>

#define TBLOCK	512
#define NBLOCK	20
#define NAMSIZ	100
#define MAXPATHLEN 512

#define	writetape(b)	writetbuf(b, 1)

/*
 * the inode keeps the size as a high byte over a low word; this is
 * the long it means.  min() and max() come from types.h.
 */
#define	fsize(sp)	(((long)(sp)->d.d_size0 << 16) + (sp)->d.d_size1)

union hblock {
	char dummy[TBLOCK];
	struct header {
		char name[NAMSIZ];
		char mode[8];
		char uid[8];
		char gid[8];
		char size[12];
		char mtime[12];
		char chksum[8];
		char linkflag;
		char linkname[NAMSIZ];
	} dbuf;
};

struct linkbuf {
	UINT	inum;
	UINT	devnum;
	int	count;
	char	*pathname;
	struct	linkbuf *nextp;
};

union	hblock dblock;
union	hblock *tbuf;
struct	linkbuf *ihead;
struct	stat stbuf;

/*
 * the size out of a tape header.  The BSD original decoded it into
 * stbuf.st_size; the micronix stat has no such long, so it rides
 * beside the struct instead.  putfile sets it from fsize() so that
 * the w-flag display and tomodes read the same place either way.
 */
long	hsize;

int	rflag;
int	xflag;
int	vflag;
int	tflag;
int	cflag;
int	fflag;
int	iflag;
int	oflag;
int	pflag;
int	wflag;
int	Bflag;
int	Fflag;

int	mt;
int	term;
int	chksum;
int	recno;
int	first;
int	prtlinkerr;
int	freemem = 1;
int	nblock = 0;
int	onintr();
int	onquit();
int	onhup();

/*
 * stdout here is a variable the startup code fills in, not a
 * constant, so the assignment waits for main.  A file-scope
 * "= stdout" compiles and captures nothing, and every v-flag
 * line went quietly into it.
 */
FILE	*vfile;
char	*usefile;
char	magtape[] = "/dev/mt0";
char	*malloc();
long	time();
long	lseek();
long	telldir();
char	*ctime();
char	*rindex();
char	*getcwd();
char	*getmem();

main(argc, argv)
int	argc;
char	*argv[];
{
	char *cp;

	vfile = stdout;
	if (argc < 2)
		usage();

	usefile = magtape;
	argv[argc] = 0;
	argv++;
	for (cp = *argv++; *cp; cp++)
		switch(*cp) {

		case 'f':
			if (*argv == 0) {
				fprintf(stderr,
			"tar: tapefile must be specified with 'f' option\n");
				usage();
			}
			usefile = *argv++;
			fflag++;
			break;

		case 'c':
			cflag++;
			rflag++;
			break;

		case 'o':
			oflag++;
			break;

		case 'p':
			pflag++;
			break;

		case 'r':
			rflag++;
			break;

		case 'v':
			vflag++;
			break;

		case 'w':
			wflag++;
			break;

		case 'x':
			xflag++;
			break;

		case 't':
			tflag++;
			break;

		case '-':
			break;

		case 'b':
			if (*argv == 0) {
				fprintf(stderr,
			"tar: blocksize must be specified with 'b' option\n");
				usage();
			}
			nblock = atoi(*argv);
			if (nblock <= 0) {
				fprintf(stderr,
				    "tar: invalid blocksize \"%s\"\n", *argv);
				done(1);
			}
			argv++;
			break;

		case 'l':
			prtlinkerr++;
			break;

		case 'i':
			iflag++;
			break;

		case 'B':
			Bflag++;
			break;

		case 'F':
			Fflag++;
			break;

		default:
			fprintf(stderr, "tar: %c: unknown option\n", *cp);
			usage();
		}

	if (!rflag && !xflag && !tflag)
		usage();
	if (rflag) {
		if (signal(SIGINT, SIG_IGN) != SIG_IGN)
			(void) signal(SIGINT, onintr);
		if (signal(SIGHUP, SIG_IGN) != SIG_IGN)
			(void) signal(SIGHUP, onhup);
		if (signal(SIGQUIT, SIG_IGN) != SIG_IGN)
			(void) signal(SIGQUIT, onquit);
		mt = openmt(usefile, 1);
		dorep(argv);
		done(0);
	}
	mt = openmt(usefile, 0);
	if (xflag)
		doxtract(argv);
	else
		dotable(argv);
	done(0);
}

usage()
{
	fprintf(stderr,
"tar: usage: tar -{txr}[cvfblopwBiF] [tapefile] [blocksize] file1 file2...\n");
	done(1);
}

int
openmt(tape, writing)
	char *tape;
	int writing;
{

	if (strcmp(tape, "-") == 0) {
		/*
		 * Read from standard input or write to standard output.
		 */
		if (writing) {
			if (cflag == 0) {
				fprintf(stderr,
			 "tar: can only create standard output archives\n");
				done(1);
			}
			vfile = stderr;
			mt = dup(1);
		} else {
			mt = dup(0);
			Bflag++;
		}
	} else {
		/*
		 * Use file or tape on local machine.  The open modes
		 * are v6: c makes the file fresh, r opens what is
		 * there for update so the append can read its way to
		 * the end first.
		 */
		if (writing) {
			if (cflag)
				mt = creat(tape, 0666);
			else
				mt = open(tape, 2);
		} else
			mt = open(tape, 0);
		if (mt < 0) {
			fprintf(stderr, "tar: ");
			perror(tape);
			done(1);
		}
	}
	return(mt);
}

dorep(argv)
	char *argv[];
{
	register char *cp, *cp2;
	char wdir[MAXPATHLEN], tempdir[MAXPATHLEN], *parent;

	if (!cflag) {
		getdir();
		do {
			passtape();
			if (term)
				done(0);
			getdir();
		} while (!endtape());
		backtape();
	}

	(void) getcwd(wdir);
	while (*argv && ! term) {
		cp2 = *argv;
		if (!strcmp(cp2, "-C") && argv[1]) {
			argv++;
			if (chdir(*argv) < 0) {
				fprintf(stderr, "tar: can't change directories to ");
				perror(*argv);
			} else
				(void) getcwd(wdir);
			argv++;
			continue;
		}

		if (*argv[0] == '/'){
			parent = "";
		} else {
			parent = wdir;
		}

		for (cp = *argv; *cp; cp++)
			if (*cp == '/')
				cp2 = cp;
		if (cp2 != *argv) {
			*cp2 = '\0';
			if (chdir(*argv) < 0) {
				fprintf(stderr, "tar: can't change directories to ");
				perror(*argv);
				continue;
			}
			parent = getcwd(tempdir);
			*cp2 = '/';
			cp2++;
		}
		putfile(*argv++, cp2, parent);
		if (chdir(wdir) < 0) {
			fprintf(stderr, "tar: cannot change back?: ");
			perror(wdir);
		}
	}
	putempty();
	putempty();
	flushtape();
	if (prtlinkerr == 0)
		return;
	for (; ihead != NULL; ihead = ihead->nextp) {
		if (ihead->count == 0)
			continue;
		fprintf(stderr, "tar: missing links to %s\n", ihead->pathname);
	}
}

endtape()
{
	return (dblock.dbuf.name[0] == '\0');
}

getdir()
{
	register struct stat *sp;
	int i;

top:
	readtape((char *)&dblock);
	if (dblock.dbuf.name[0] == '\0')
		return;
	sp = &stbuf;
	sscanf(dblock.dbuf.mode, "%o", &i);
	sp->st_mode = i;
	sscanf(dblock.dbuf.uid, "%o", &i);
	sp->st_uid = i;
	sscanf(dblock.dbuf.gid, "%o", &i);
	sp->st_gid = i;
	sscanf(dblock.dbuf.size, "%lo", &hsize);
	sscanf(dblock.dbuf.mtime, "%lo", &sp->st_mtime);
	sscanf(dblock.dbuf.chksum, "%o", &chksum);
	if (chksum != (i = checksum())) {
		fprintf(stderr, "tar: directory checksum error (%d != %d)\n",
		    chksum, i);
		if (iflag)
			goto top;
		done(2);
	}
}

passtape()
{
	long blocks;
	char *bufp;

	if (dblock.dbuf.linkflag == '1')
		return;
	blocks = hsize;
	blocks += TBLOCK-1;
	blocks /= TBLOCK;

	while (blocks-- > 0)
		(void) readtbuf(&bufp, TBLOCK);
}

putfile(longname, shortname, parent)
	char *longname;
	char *shortname;
	char *parent;
{
	int infile = 0;
	long blocks;
	char buf[TBLOCK];
	char *bigbuf;
	register char *cp;
	struct direct *dp;
	DIR *dirp;
	register int i;
	long l;
	char newparent[NAMSIZ+64];
	int	maxread;
	int	hint;		/* amount to write to get "in sync" */

	i = stat(shortname, &stbuf);
	if (i < 0) {
		fprintf(stderr, "tar: ");
		perror(longname);
		return;
	}
	hsize = fsize(&stbuf);
	if (checkw('r', longname) == 0)
		return;
	if (Fflag && checkf(shortname, stbuf.st_mode, Fflag) == 0)
		return;

	switch (stbuf.st_mode & S_IFMT) {
	case S_IFDIR:
		for (i = 0, cp = buf; *cp++ = longname[i++];)
			;
		*--cp = '/';
		*++cp = 0  ;
		if (!oflag) {
			if ((cp - buf) >= NAMSIZ) {
				fprintf(stderr, "tar: %s: file name too long\n",
				    longname);
				return;
			}
			hsize = 0;
			tomodes(&stbuf);
			strcpy(dblock.dbuf.name,buf);
			sprintf(dblock.dbuf.chksum, "%6o", checksum());
			(void) writetape((char *)&dblock);
		}
		sprintf(newparent, "%s/%s", parent, shortname);
		if (chdir(shortname) < 0) {
			perror(shortname);
			return;
		}
		if ((dirp = opendir(".")) == NULL) {
			fprintf(stderr, "tar: %s: directory read error\n",
			    longname);
			if (chdir(parent) < 0) {
				fprintf(stderr, "tar: cannot change back?: ");
				perror(parent);
			}
			return;
		}
		while ((dp = readdir(dirp)) != NULL && !term) {
			if (dp->d_ino == 0)
				continue;
			if (!strcmp(".", dp->d_name) ||
			    !strcmp("..", dp->d_name))
				continue;
			strcpy(cp, dp->d_name);
			l = telldir(dirp);
			closedir(dirp);
			putfile(buf, cp, newparent);
			dirp = opendir(".");
			seekdir(dirp, l);
		}
		closedir(dirp);
		if (chdir(parent) < 0) {
			fprintf(stderr, "tar: cannot change back?: ");
			perror(parent);
		}
		break;

	case S_IFREG:
		if ((infile = open(shortname, 0)) < 0) {
			fprintf(stderr, "tar: ");
			perror(longname);
			return;
		}
		tomodes(&stbuf);
		if (strlen(longname) >= NAMSIZ) {
			fprintf(stderr, "tar: %s: file name too long\n",
			    longname);
			close(infile);
			return;
		}
		strcpy(dblock.dbuf.name, longname);
		if (stbuf.st_nlink > 1) {
			struct linkbuf *lp;
			int found = 0;

			for (lp = ihead; lp != NULL; lp = lp->nextp)
				if (lp->inum == stbuf.st_ino &&
				    lp->devnum == stbuf.st_dev) {
					found++;
					break;
				}
			if (found) {
				strcpy(dblock.dbuf.linkname, lp->pathname);
				dblock.dbuf.linkflag = '1';
				sprintf(dblock.dbuf.chksum, "%6o", checksum());
				(void) writetape( (char *) &dblock);
				if (vflag)
					fprintf(vfile, "a %s link to %s\n",
					    longname, lp->pathname);
				lp->count--;
				close(infile);
				return;
			}
			lp = (struct linkbuf *) getmem(sizeof(*lp));
			if (lp != NULL) {
				lp->pathname = getmem(strlen(longname)+1);
				if (lp->pathname != NULL) {
					lp->nextp = ihead;
					ihead = lp;
					lp->inum = stbuf.st_ino;
					lp->devnum = stbuf.st_dev;
					lp->count = stbuf.st_nlink - 1;
					strcpy(lp->pathname, longname);
				}
			}
		}
		blocks = (hsize + (TBLOCK-1)) / TBLOCK;
		if (vflag)
			fprintf(vfile, "a %s %ld blocks\n", longname, blocks);
		sprintf(dblock.dbuf.chksum, "%6o", checksum());
		hint = writetape((char *)&dblock);
		maxread = nblock * TBLOCK;
		if ((bigbuf = malloc((unsigned)maxread)) == 0) {
			maxread = TBLOCK;
			bigbuf = buf;
		}

		while ((i = read(infile, bigbuf, min((hint*TBLOCK), maxread))) > 0
		  && blocks > 0) {
		  	register int nblks;

			nblks = ((i-1)/TBLOCK)+1;
		  	if (nblks > blocks)
		  		nblks = blocks;
			hint = writetbuf(bigbuf, nblks);
			blocks -= nblks;
		}
		close(infile);
		if (bigbuf != buf)
			free(bigbuf);
		if (i < 0) {
			fprintf(stderr, "tar: Read error on ");
			perror(longname);
		} else if (blocks != 0 || i != 0)
			fprintf(stderr, "tar: %s: file changed size\n",
			    longname);
		while (--blocks >=  0)
			putempty();
		break;

	default:
		fprintf(stderr, "tar: %s is not a file. Not dumped\n",
		    longname);
		break;
	}
}

doxtract(argv)
	char *argv[];
{
	long blocks, bytes;
	int ofile, i;

	for (;;) {
		if ((i = wantit(argv)) == 0)
			continue;
		if (i == -1)
			break;		/* end of tape */
		if (checkw('x', dblock.dbuf.name) == 0) {
			passtape();
			continue;
		}
		if (Fflag) {
			char *s;

			if ((s = rindex(dblock.dbuf.name, '/')) == 0)
				s = dblock.dbuf.name;
			else
				s++;
			if (checkf(s, stbuf.st_mode, Fflag) == 0) {
				passtape();
				continue;
			}
		}
		if (checkdir(dblock.dbuf.name))	/* have a directory */
			continue;
		if (dblock.dbuf.linkflag == '2') {	/* symlink */
			/*
			 * micronix has no symbolic links.  The entry
			 * is header-only, so saying so is the whole of
			 * handling it.
			 */
			fprintf(stderr,
			    "tar: %s: cannot make symbolic link to %s\n",
			    dblock.dbuf.name, dblock.dbuf.linkname);
			continue;
		}
		if (dblock.dbuf.linkflag == '1') {	/* regular link */
			unlink(dblock.dbuf.name);
			if (link(dblock.dbuf.linkname, dblock.dbuf.name) < 0) {
				fprintf(stderr, "tar: can't link %s to %s: ",
				    dblock.dbuf.name, dblock.dbuf.linkname);
				perror("");
				continue;
			}
			if (vflag)
				fprintf(vfile, "%s linked to %s\n",
				    dblock.dbuf.name, dblock.dbuf.linkname);
			continue;
		}
		if ((ofile = creat(dblock.dbuf.name,stbuf.st_mode&07777)) < 0) {
			fprintf(stderr, "tar: can't create %s: ",
			    dblock.dbuf.name);
			perror("");
			passtape();
			continue;
		}
		chown(dblock.dbuf.name,
		    stbuf.st_uid | (stbuf.st_gid << 8));
		blocks = ((bytes = hsize) + TBLOCK-1)/TBLOCK;
		if (vflag)
			fprintf(vfile, "x %s, %ld bytes, %ld tape blocks\n",
			    dblock.dbuf.name, bytes, blocks);
		for (; blocks > 0;) {
			register int nread;
			char	*bufp;
			register int nwant;

			nwant = NBLOCK*TBLOCK;
			if (nwant > (blocks*TBLOCK))
				nwant = (blocks*TBLOCK);
			nread = readtbuf(&bufp, nwant);
			if (write(ofile, bufp, (int)min(nread, bytes)) < 0) {
				fprintf(stderr,
				    "tar: %s: HELP - extract write error",
				    dblock.dbuf.name);
				perror("");
				done(2);
			}
			bytes -= nread;
			blocks -= (((nread-1)/TBLOCK)+1);
		}
		close(ofile);
		if (pflag)
			chmod(dblock.dbuf.name, stbuf.st_mode & 07777);
	}
}

dotable(argv)
	char *argv[];
{
	register int i;

	for (;;) {
		if ((i = wantit(argv)) == 0)
			continue;
		if (i == -1)
			break;		/* end of tape */
		if (vflag)
			longt(&stbuf);
		printf("%s", dblock.dbuf.name);
		if (dblock.dbuf.linkflag == '1')
			printf(" linked to %s", dblock.dbuf.linkname);
		if (dblock.dbuf.linkflag == '2')
			printf(" symbolic link to %s", dblock.dbuf.linkname);
		printf("\n");
		passtape();
	}
}

putempty()
{
	char buf[TBLOCK];

	memset(buf, 0, sizeof (buf));
	(void) writetape(buf);
}

longt(st)
	register struct stat *st;
{
	register char *cp;

	pmode(st);
	printf("%3d/%1d", st->st_uid, st->st_gid);
	printf("%7ld", hsize);
	cp = ctime(&st->st_mtime);
	printf(" %-12.12s %-4.4s ", cp+4, cp+20);
}

/*
 * the mode these tables pick apart came off the tape, so the bits
 * are the historic ones, not the micronix inode's
 */
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

pmode(st)
	register struct stat *st;
{
	register int **mp;

	for (mp = &m[0]; mp < &m[9];)
		selectbits(*mp++, st);
}

selectbits(pairp, st)
	int *pairp;
	struct stat *st;
{
	register int n, *ap;

	ap = pairp;
	n = *ap++;
	while (--n>=0 && (st->st_mode&*ap++)==0)
		ap++;
	putchar(*ap);
}

/*
 * Make all directories needed by `name'.  If `name' is itself
 * a directory on the tar tape (indicated by a trailing '/'),
 * return 1; else 0.
 */
checkdir(name)
	register char *name;
{
	register char *cp;

	/*
	 * Quick check for existence of directory.
	 */
	if ((cp = rindex(name, '/')) == 0)
		return (0);
	*cp = '\0';
	if (access(name, 0) == 0) {	/* already exists */
		*cp = '/';
		return (cp[1] == '\0');	/* return (lastchar == '/') */
	}
	*cp = '/';

	/*
	 * No luck, try to make all directories in path.
	 */
	for (cp = name; *cp; cp++) {
		if (*cp != '/')
			continue;
		*cp = '\0';
		if (access(name, 0) < 0) {
			if (mkdir(name, 0777) < 0) {
				perror(name);
				*cp = '/';
				return (0);
			}
			chown(name, stbuf.st_uid | (stbuf.st_gid << 8));
			if (pflag && cp[1] == '\0')	/* dir on the tape */
				chmod(name, stbuf.st_mode & 07777);
		}
		*cp = '/';
	}
	return (cp[-1]=='/');
}

onintr()
{
	(void) signal(SIGINT, SIG_IGN);
	term++;
}

onquit()
{
	(void) signal(SIGQUIT, SIG_IGN);
	term++;
}

onhup()
{
	(void) signal(SIGHUP, SIG_IGN);
	term++;
}

tomodes(sp)
register struct stat *sp;
{
	register char *cp;

	for (cp = dblock.dummy; cp < &dblock.dummy[TBLOCK]; cp++)
		*cp = '\0';
	sprintf(dblock.dbuf.mode, "%6o ", sp->st_mode & 07777);
	sprintf(dblock.dbuf.uid, "%6o ", sp->st_uid);
	sprintf(dblock.dbuf.gid, "%6o ", sp->st_gid);
	sprintf(dblock.dbuf.size, "%11lo ", hsize);
	sprintf(dblock.dbuf.mtime, "%11lo ", sp->st_mtime);
}

checksum()
{
	register i;
	register char *cp;

	for (cp = dblock.dbuf.chksum;
	     cp < &dblock.dbuf.chksum[sizeof(dblock.dbuf.chksum)]; cp++)
		*cp = ' ';
	i = 0;
	for (cp = dblock.dummy; cp < &dblock.dummy[TBLOCK]; cp++)
		i += *cp;
	return (i);
}

checkw(c, name)
	char *name;
{
	if (!wflag)
		return (1);
	printf("%c ", c);
	if (vflag)
		longt(&stbuf);
	printf("%s: ", name);
	return (response() == 'y');
}

response()
{
	char c;

	c = getchar();
	if (c != '\n')
		while (getchar() != '\n')
			;
	else
		c = 'n';
	return (c);
}

checkf(name, mode, howmuch)
	char *name;
	int mode, howmuch;
{
	int l;

	if ((mode & S_IFMT) == S_IFDIR){
		if ((strcmp(name, "SCCS")==0) || (strcmp(name, "RCS")==0))
			return(0);
		return(1);
	}
	if ((l = strlen(name)) < 3)
		return (1);
	if (howmuch > 1 && name[l-2] == '.' && name[l-1] == 'o')
		return (0);
	if (strcmp(name, "core") == 0 ||
	    strcmp(name, "errs") == 0 ||
	    (howmuch > 1 && strcmp(name, "a.out") == 0))
		return (0);
	/* SHOULD CHECK IF IT IS EXECUTABLE */
	return (1);
}

done(n)
{
	exit(n);
}

/*
 * Do we want the next entry on the tape, i.e. is it selected?  If
 * not, skip over the entire entry.  Return -1 if reached end of tape.
 */
wantit(argv)
	char *argv[];
{
	register char **cp;

	getdir();
	if (endtape())
		return (-1);
	if (*argv == 0)
		return (1);
	for (cp = argv; *cp; cp++)
		if (prefix(*cp, dblock.dbuf.name))
			return (1);
	passtape();
	return (0);
}

/*
 * Does s2 begin with the string s1, on a directory boundary?
 */
prefix(s1, s2)
	register char *s1, *s2;
{
	while (*s1)
		if (*s1++ != *s2++)
			return (0);
	if (*s2)
		return (*s2 == '/');
	return (1);
}

readtape(buffer)
	char *buffer;
{
	char *bufp;

	if (first == 0)
		getbuf();
	(void) readtbuf(&bufp, TBLOCK);
	memcpy(buffer, bufp, TBLOCK);
	return(TBLOCK);
}

readtbuf(bufpp, size)
	char **bufpp;
	int size;
{
	register int i;

	if (recno >= nblock || first == 0) {
		if ((i = bread(mt, (char *)tbuf, TBLOCK*nblock)) < 0)
			mterr("read", i, 3);
		if (first == 0) {
			if ((i % TBLOCK) != 0) {
				fprintf(stderr, "tar: tape blocksize error\n");
				done(3);
			}
			i /= TBLOCK;
			if (i != nblock) {
				fprintf(stderr, "tar: blocksize = %d\n", i);
				nblock = i;
			}
			first = 1;
		}
		recno = 0;
	}
	if (size > ((nblock-recno)*TBLOCK))
		size = (nblock-recno)*TBLOCK;
	*bufpp = (char *)&tbuf[recno];
	recno += (size/TBLOCK);
	return (size);
}

writetbuf(buffer, n)
	register char *buffer;
	register int n;
{
	int i;

	if (first == 0) {
		getbuf();
		first = 1;
	}
	if (recno >= nblock) {
		i = write(mt, (char *)tbuf, TBLOCK*nblock);
		if (i != TBLOCK*nblock)
			mterr("write", i, 2);
		recno = 0;
	}

	/*
	 *  Special case:  We have an empty tape buffer, and the
	 *  users data size is >= the tape block size:  Avoid
	 *  the copy and write direct.  BIG WIN.  Add the
	 *  residual to the tape buffer.
	 */
	while (recno == 0 && n >= nblock) {
		i = write(mt, buffer, TBLOCK*nblock);
		if (i != TBLOCK*nblock)
			mterr("write", i, 2);
		n -= nblock;
		buffer += (nblock * TBLOCK);
	}

	while (n-- > 0) {
		memcpy((char *)&tbuf[recno++], buffer, TBLOCK);
		buffer += TBLOCK;
		if (recno >= nblock) {
			i = write(mt, (char *)tbuf, TBLOCK*nblock);
			if (i != TBLOCK*nblock)
				mterr("write", i, 2);
			recno = 0;
		}
	}

	/* Tell the user how much to write to get in sync */
	return (nblock - recno);
}

backtape()
{
	lseek(mt, -((long)TBLOCK*nblock), 1);
	recno--;
}

flushtape()
{
	int i;

	i = write(mt, (char *)tbuf, TBLOCK*nblock);
	if (i != TBLOCK*nblock)
		mterr("write", i, 2);
}

mterr(operation, i, exitcode)
	char *operation;
	int i;
{
	fprintf(stderr, "tar: tape %s error: ", operation);
	if (i < 0)
		perror("");
	else
		fprintf(stderr, "unexpected EOF\n");
	done(exitcode);
}

bread(fd, buf, size)
	int fd;
	char *buf;
	int size;
{
	int count;
	static int lastread = 0;

	if (!Bflag)
		return (read(fd, buf, size));

	for (count = 0; count < size; count += lastread) {
		lastread = read(fd, buf, size - count);
		if (lastread <= 0) {
			if (count > 0)
				return (count);
			return (lastread);
		}
		buf += lastread;
	}
	return (count);
}

/*
 * the library has no getcwd, so this is one, done the way pwd does
 * it: stat the directory some rungs of .. up, scan one more .. for
 * the entry carrying that inode, and prepend the name found.  The
 * process never moves - the rungs pile up in a string instead - so a
 * failure partway leaves nothing to chdir back from.
 *
 * Like pwd, this trusts the inode number to identify the entry, which
 * holds within one filesystem; a path that climbs through a mount
 * point is beyond it, and beyond pwd too.
 */
char *
getcwd(buf)
	char *buf;
{
	char up[MAXPATHLEN];		/* ".", then "./..", "./../.." */
	char path[MAXPATHLEN];
	char nbuf[16];
	struct stat cur, root;
	struct dir dent;
	register char *p;
	int fd, i;

	path[0] = '\0';
	strcpy(up, ".");
	if (stat("/", &root) < 0)
		nocwd();
	for (;;) {
		if (stat(up, &cur) < 0)
			nocwd();
		if (cur.st_ino == root.st_ino && cur.st_dev == root.st_dev)
			break;
		strcat(up, "/..");
		if ((fd = open(up, 0)) < 0)
			nocwd();
		dent.ino = 0;
		while (read(fd, (char *)&dent, 16) == 16)
			if (dent.ino == cur.st_ino)
				break;
		close(fd);
		if (dent.ino != cur.st_ino)
			nocwd();
		/*
		 * a directory name is 14 bytes and only null
		 * terminated when it is shorter than that
		 */
		strncpy(nbuf, dent.name, 14);
		nbuf[14] = '\0';
		i = strlen(nbuf);
		if (strlen(path) + i + 2 >= MAXPATHLEN)
			nocwd();
		/* shift path right, slash and name into the gap */
		for (p = path + strlen(path); p >= path; p--)
			p[i + 1] = *p;
		path[0] = '/';
		memcpy(path + 1, nbuf, i);
	}
	if (path[0] == '\0')
		strcpy(path, "/");
	strcpy(buf, path);
	return (buf);
}

nocwd()
{
	fprintf(stderr, "tar: cannot determine current directory\n");
	done(1);
}

getbuf()
{

	if (nblock == 0)
		nblock = NBLOCK;
	tbuf = (union hblock *)malloc((unsigned)nblock*TBLOCK);
	if (tbuf == NULL) {
		fprintf(stderr, "tar: blocksize %d too big, can't get memory\n",
		    nblock);
		done(1);
	}
}

char *
getmem(size)
{
	char *p = malloc((unsigned) size);

	if (p == NULL && freemem) {
		fprintf(stderr,
		    "tar: out of memory, link info lost\n");
		freemem = 0;
	}
	return (p);
}
