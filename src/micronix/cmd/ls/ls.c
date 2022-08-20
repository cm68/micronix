/*
 * list file or directory - v7 version, patched for micronix
 *
 * cmd/ls/ls.c
 * Changed: <2022-08-20 12:56:22 curt>
 */

/*
#include <sys/param.h>
*/
#include <types.h>
#include <stdio.h>
#include <sys/fs.h>
#include <sys/stat.h>
#include <sys/dir.h>

#define	major(x)	(((x) >> 8) & 0xff)
#define	minor(x)	((x) & 0xff)

#define S_IREAD		04         /* read permission */
#define S_IWRITE	02         /* write permission */
#define S_IEXEC		01         /* execute permission */
#define	DIRSIZ		14

#define WIDTH 80

#define	NFILES	1024
FILE	*pwdf = 0, *dirf = 0;
/* char	stdbuf[BUFSIZ]; */

struct lbuf {
	union {
		char	lname[15];
		char	*namep;
	} ln;
	char	ltype;
	short	lnum;
	short	lflags;
	short	lnl;
	short	luid;
	short	lgid;
	long	lsize;
	long	lmtime;
};

lwide = 0;
int errors = 0;
int fc = 0;
int maxn = 0;
int xflg = 1;
int	aflg = 0;
int dflg = 0;
int lflg = 0;
int sflg = 0;
int tflg = 0;
int uflg = 0;
int iflg = 0;
int fflg = 0;
int gflg = 0;
int cflg = 0;
int	rflg = 1;
long	year = 0;
int	flags = 0;
int	lastuid	= -1;
char	tbuf[16] = 0;
long	tblocks = 0;
int	statreq = 0;
struct	lbuf	*flist[NFILES] = 0;
struct	lbuf	**lastp = flist;
struct	lbuf	**firstp = flist;
char	*dotp	= ".";

char	*makename();
struct	lbuf *gstat();
char	*ctime();
long	nblock();

#define	ISARG	0100000

/* return the number of digits an integer needs to print */
wide(i)
int i;
{
    int r = 1;
    while (i /= 10) {
        r++;
    }
    return r;
}

int
main(argc, argv)
char *argv[];
{
	int i;
	register struct lbuf *ep, **ep1;
	register struct lbuf **slastp;
	struct lbuf **epp;
	struct lbuf lb;
	char *t;
	int compar();

	/* setbuf(stdout, stdbuf); */
	time(&lb.lmtime);
	year = lb.lmtime - 6L*30L*24L*60L*60L; /* 6 months ago */
	if (--argc > 0 && *argv[1] == '-') {
		argv++;
		while (*++*argv) switch (**argv) {

		case 'a':
			aflg++;
			continue;

		case 's':
			xflg = 0;
			sflg++;
			statreq++;
			continue;

		case 'd':
			dflg++;
			continue;

		case 'g':
			gflg++;
			continue;

		case 'l':
			xflg = 0;
			lflg++;
			statreq++;
			continue;

		case 'r':
			rflg = -1;
			continue;

		case 't':
			tflg++;
			statreq++;
			continue;

		case 'u':
			uflg++;
			continue;

		case 'c':
			cflg++;
			continue;

		case 'i':
			xflg = 0;
			iflg++;
			continue;

		case 'f':
			fflg++;
			continue;

		default:
			continue;
		}
		argc--;
	}
	if (fflg) {
		aflg++;
		lflg = 0;
		sflg = 0;
		tflg = 0;
		statreq = 0;
	}
	if(lflg) {
		t = "/etc/passwd";
		if(gflg)
			t = "/etc/group";
		pwdf = fopen(t, "r");
	}
	if (argc == 0) {
		argc++;
		argv = &dotp - 1;
	}
	for (i=0; i < argc; i++) {
		if ((ep = gstat(*++argv, 1))==NULL)
			continue;
		ep->ln.namep = *argv;
		ep->lflags |= ISARG;
	}
	qsort(firstp, lastp - firstp, sizeof *lastp, compar);
    lwide = 0;
    for (epp = firstp; epp < slastp; epp++) {
		ep = *epp;
        i = wide(ep->lnl);
        if (i > lwide) lwide = i;
    }
	slastp = lastp;
	for (epp=firstp; epp<slastp; epp++) {
		ep = *epp;
		if (ep->ltype=='d' && dflg==0 || fflg) {
			if (argc>1) {
				if (epp != firstp) printf("\n");
				printf("%s:\n", ep->ln.namep);
				fc = 0;
			}
			lastp = slastp;
			maxn = readdir(ep->ln.namep);
			if (fflg==0)
				qsort(slastp,lastp - slastp,sizeof *lastp,compar);
			if (lflg || sflg)
				printf("total %D\n", tblocks);
			if (!xflg) {
                lwide = 0;
				for (ep1=slastp; ep1 < lastp; ep1++) {
                    ep = *ep1;
                    i = wide(ep->lnl);
                    if (i > lwide) lwide = i;
				}
				for (ep1=slastp; ep1 < lastp; ep1++) {
					pentry(*ep1);
				}
			} else {
				char fbuf[10];
				char cols = WIDTH / (maxn + 1);
				char lines = ((lastp - slastp) + (cols - 1)) / cols;
				char j;
#ifdef notdef
				char total = lastp - slastp;
				for (i = 0; i < total; i++) {
					printf("%s ", slastp[i]->ln.lname);
				}
				printf("\ntotal %d lines %d cols %d maxn %d WIDTH %d\n",
					total, lines, cols, maxn, WIDTH);
#endif
				sprintf(fbuf, "%%-%ds ", maxn);
				for (i = 0; i < lines; i++) {
					for (j = 0; j < cols; j++) {
						ep1 = &slastp[i + (j * lines)];
						printf(fbuf, (*ep1)->ln.lname);
					}
					printf("\n");
				}
			}
		} else 
			pentry(ep);
	}
	exit(errors ? 1 : 0);
}

pentry(ap)
struct lbuf *ap;
{
	struct { 
		char dminor, dmajor;
	};
	register t;
	register struct lbuf *p;
	register char *cp;
    char lprec[10];

	if (xflg) {
		t = WIDTH / (maxn + 1);
		if (++fc == t) {
			printf("\n");
			fc = 0;
		}
		return;
	}
	p = ap;
	if (p->lnum == -1)
		return;
	if (iflg)
		printf("%5u ", p->lnum);
	if (sflg)
	printf("%4D ", nblock(p->lsize));
	if (lflg) {
        sprintf(lprec, "%%%dd ", lwide+1);
		putchar(p->ltype);
		pmode(p->lflags);
		printf(lprec, p->lnl);
		t = p->luid;
		if(gflg)
			t = p->lgid;
		if (getname(t, tbuf)==0)
			printf("%-6.6s", tbuf);
		else
			printf("%-6d", t);
		if (p->ltype=='b' || p->ltype=='c')
			printf("%3d,%3d", major((int)p->lsize), minor((int)p->lsize));
		else
			printf("%7ld", p->lsize);
		cp = ctime(&p->lmtime);
		if(p->lmtime < year)
			printf(" %-7.7s %-4.4s ", cp+4, cp+20); else
			printf(" %-12.12s ", cp+4);
	}
	if (p->lflags&ISARG)
		printf("%s\n", p->ln.namep);
	else
		printf("%.14s\n", p->ln.lname);
}

getname(uid, buf)
int uid;
char buf[];
{
	int j, c, n, i;

	if (uid==lastuid)
		return(0);
	if(pwdf == NULL)
		return(-1);
	rewind(pwdf);
	lastuid = -1;
	do {
		i = 0;
		j = 0;
		n = 0;
		while((c=fgetc(pwdf)) != '\n') {
			if (c==EOF)
				return(-1);
			if (c==':') {
				j++;
				c = '0';
			}
			if (j==0)
				buf[i++] = c;
			if (j==2)
				n = n*10 + c - '0';
		}
	} while (n != uid);
	buf[i++] = '\0';
	lastuid = uid;
	return(0);
}

long
nblock(size)
long size;
{
	return((size + 511) >> 9);
}

pmode(aflag) {

	char c = (aflag & S_1WRITE) ? 'l' : 'w';

	putchar((aflag & 0400) ? 'r' : '-');
	putchar((aflag & 0200) ? c : '-');
	putchar((aflag & 0100) ? ((aflag & S_ISUID) ? 's' : 'x') : '-');
	putchar((aflag & 0040) ? 'r' : '-');
	putchar((aflag & 0020) ? c : '-');
	putchar((aflag & 0010) ? ((aflag & S_ISGID) ? 's' : 'x') : '-');
	putchar((aflag & 0004) ? 'r' : '-');
	putchar((aflag & 0002) ? c : '-');
	putchar((aflag & 0001) ? 'x' : '-');
}

#ifdef notdef
int	m1[] = { 1, S_IREAD >> 0, 'r', '-' };
int	m2[] = { 1, S_IWRITE>>0, 'w', '-' };
int	m3[] = { 2, S_SUID, 's', S_IEXEC>>0, 'x', '-' };
int	m4[] = { 1, S_IREAD>>3, 'r', '-' };
int	m5[] = { 1, S_IWRITE>>3, 'w', '-' };
int	m6[] = { 2, S_SGID, 's', S_IEXEC>>3, 'x', '-' };
int	m7[] = { 1, S_IREAD>>6, 'r', '-' };
int	m8[] = { 1, S_IWRITE>>6, 'w', '-' };
int	m9[] = { 2, S_STICKY, 't', S_IEXEC>>6, 'x', '-' };

int	*m[] = { m1, m2, m3, m4, m5, m6, m7, m8, m9};

pmode(aflag)
{
	register int **mp;

	flags = aflag;
	for (mp = &m[0]; mp < &m[sizeof(m)/sizeof(m[0])];)
		select(*mp++);
}

select(pairp)
register int *pairp;
{
	register int n;

	n = *pairp++;
	while (--n>=0 && (flags&*pairp++)==0)
		pairp++;
	putchar(*pairp);
}
#endif

char *
makename(dir, file)
char *dir, *file;
{
	static char dfile[100];
	register char *dp, *fp;
	register int i;

	dp = dfile;
	fp = dir;
	while (*fp)
		*dp++ = *fp++;
	*dp++ = '/';
	fp = file;
	for (i=0; i<DIRSIZ; i++)
		*dp++ = *fp++;
	*dp = 0;
	return(dfile);
}

int
readdir(dir)
char *dir;
{
	static struct dir dentry;
	register int j;
	register struct lbuf *ep;
	int maxl = 0;

	if ((dirf = fopen(dir, "r")) == NULL) {
		printf("%s unreadable\n", dir);
		errors++;
		return 0;
	}
	tblocks = 0;
	for(;;) {
		if (fread((char *)&dentry, sizeof(dentry), 1, dirf) != 1)
			break;
		if (dentry.ino==0
		 || aflg==0 && dentry.name[0]=='.' &&  (dentry.name[1]=='\0'
			|| dentry.name[1]=='.' && dentry.name[2]=='\0'))
			continue;
		ep = gstat(makename(dir, dentry.name), 0);
		if (ep==NULL)
			continue;
		if (ep->lnum != -1)
			ep->lnum = dentry.ino;
		for (j=0; j<DIRSIZ; j++)
			ep->ln.lname[j] = dentry.name[j];
		j = strlen(dentry.name);
		if (j > maxl) maxl = j;
	}
	fclose(dirf);
	return maxl;
}

struct lbuf *
gstat(file, argfl)
char *file;
{
	extern char *alloc();
	struct stat statb;
	register struct lbuf *rep;
	static int nomocore;

	if (nomocore)
		return(NULL);
	rep = (struct lbuf *)alloc(sizeof(struct lbuf));
	if (rep==NULL) {
		fprintf(stderr, "ls: out of memory\n");
		nomocore = 1;
		errors++;
		return(NULL);
	}
	if (lastp >= &flist[NFILES]) {
		static int msg;
		lastp--;
		if (msg==0) {
			fprintf(stderr, "ls: too many files\n");
			msg++;
			errors++;
		}
	}
	*lastp++ = rep;
	rep->lflags = 0;
	rep->lnum = 0;
	rep->ltype = '-';
	if (argfl || statreq) {
		if (stat(file, &statb) < 0) {
			printf("%s not found\n", file);
			statb.st_ino = -1;
			statb.d.d_size0 = statb.d.d_size1 = 0;
			statb.st_mode = 0;
			errors++;
			if (argfl) {
				lastp--;
				return(0);
			}
		}
		rep->lnum = statb.st_ino;
		rep->lsize = statb.d.d_size1 + (statb.d.d_size0 << 16);
		switch(statb.st_mode & S_IFMT) {

		case S_IFDIR:
			rep->ltype = 'd';
			break;

		case S_IFBLK:
			rep->ltype = 'b';
			rep->lsize = statb.st_addr[0];
			break;

		case S_IFCHR:
			rep->ltype = 'c';
			rep->lsize = statb.st_addr[0];
			break;
		}
		rep->lflags = statb.st_mode & ~S_IFMT;
		rep->luid = statb.st_uid;
		rep->lgid = statb.st_gid;
		rep->lnl = statb.st_nlink;
		if(uflg)
			rep->lmtime = statb.st_rtime;
		else if (cflg)
			rep->lmtime = statb.st_mtime;
		else
			rep->lmtime = statb.st_mtime;
		tblocks += nblock(rep->lsize);
	}
	return(rep);
}

compar(pp1, pp2)
struct lbuf **pp1, **pp2;
{
	register struct lbuf *p1, *p2;

	p1 = *pp1;
	p2 = *pp2;
	if (dflg==0) {
		if (p1->lflags&ISARG && p1->ltype=='d') {
			if (!(p2->lflags&ISARG && p2->ltype=='d'))
				return(1);
		} else {
			if (p2->lflags&ISARG && p2->ltype=='d')
				return(-1);
		}
	}
	if (tflg) {
		if(p2->lmtime == p1->lmtime)
			return(0);
		if(p2->lmtime > p1->lmtime)
			return(rflg);
		return(-rflg);
	}
	return(rflg * strcmp(p1->lflags&ISARG? p1->ln.namep: p1->ln.lname,
				p2->lflags&ISARG? p2->ln.namep: p2->ln.lname));
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */

