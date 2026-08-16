/*	find	- walk a file tree, printing or acting on what matches	*/

/*
 * V7 find (usr/src/cmd/find.c), on micronix.
 *
 * cmd/find/find.c
 *
 * The tree walk was rewritten for micronix.  V7 find got its starting
 * directory with popen("pwd"), then chdir'ed into each directory and
 * read "." as raw blocks, recovering from a bad chdir by going back to
 * Home and down again.  None of that is available here: there is no
 * popen, and there is no getcwd to learn Home with.  Instead each
 * directory is opened by name - readdir is a 16-byte struct dir, the
 * same on-disk format pwd(1) walks - and the pathname is built as a
 * string, so the process never leaves the directory find was started
 * in.
 *
 * What the predicates see changes with the inode.  micronix's stat
 * has no st_size: the size is split across st_size0/st_size1, twenty-
 * four bits, and -size reads it that way.  There is one time field
 * where V7 kept three, so -atime reads st_rtime and -ctime is gone -
 * it would have to pretend to be -mtime, and a predicate that lies is
 * worse than one that is missing.  uid, gid and nlink are single
 * bytes in the micronix inode, which is exactly what these predicates
 * want; -type uses the micronix S_IF* constants, where S_IFREG is 0
 * rather than V7's 0100000.  -cpio is gone: it needed st_rdev and an
 * endianness test that made sense only on the PDP-11 and VAX it was
 * written between.
 *
 * -exec and -ok fork a child and execv the command.  micronix has no
 * execvp, so the command must be a path - exec does not search PATH -
 * which is the one place this find is stricter than V7's.
 *
 * vim: tabstop=4 shiftwidth=4 noexpandtab:
 */

#include <types.h>
#include <stdio.h>
#include <sys/fs.h>
#include <sys/stat.h>
#include <sys/dir.h>

#define A_DAY	86400L	/* a day full of seconds */
#define EQ(x, y)	(strcmp(x, y)==0)

int	Randlast;
char	Pathname[256];

struct anode {
	int (*F)();
	struct anode *L, *R;
} Node[100];
int Nn;		/* number of nodes */
char	*Fname;
long	Now;
int	Argc,
	Ai,
	Pi;
char	**Argv;

long	Newer;

struct stat Statb;

struct	anode	*exp(),
		*e1(),
		*e2(),
		*e3(),
		*mk();
char	*nxtarg();
char *rindex();
main(argc, argv) char *argv[];
{
	struct anode *exlist;
	int paths;
	register char *cp;

	time(&Now);
	Argc = argc; Argv = argv;
	if (argc < 3) {
usage:		pr("Usage: find path-list predicate-list\n");
		exit(1);
	}
	for (Ai = paths = 1; Ai < (argc-1); ++Ai, ++paths)
		if (*Argv[Ai] == '-' || EQ(Argv[Ai], "(") || EQ(Argv[Ai], "!"))
			break;
	if (paths == 1) /* no path-list */
		goto usage;
	if (!(exlist = exp())) { /* parse and compile the arguments */
		pr("find: parsing error\n");
		exit(1);
	}
	if (Ai < argc) {
		pr("find: missing conjunction\n");
		exit(1);
	}
	for (Pi = 1; Pi < paths; ++Pi) {
		strcpy(Pathname, Argv[Pi]);
		cp = rindex(Pathname, '/');
		Fname = cp ? cp + 1 : Pathname;
		descend(Pathname, exlist); /* to find files that match */
	}
	exit(0);
}

/* compile time functions:  priority is  exp()<e1()<e2()<e3()  */

struct anode *exp() { /* parse ALTERNATION (-o)  */
	int or();
	register struct anode * p1;

	p1 = e1() /* get left operand */ ;
	if (EQ(nxtarg(), "-o")) {
		Randlast--;
		return(mk(or, p1, exp()));
	}
	else if (Ai <= Argc) --Ai;
	return(p1);
}
struct anode *e1() { /* parse CONCATENATION (formerly -a) */
	int and();
	register struct anode * p1;
	register char *a;

	p1 = e2();
	a = nxtarg();
	if (EQ(a, "-a")) {
And:
		Randlast--;
		return(mk(and, p1, e1()));
	} else if (EQ(a, "(") || EQ(a, "!") || (*a=='-' && !EQ(a, "-o"))) {
		--Ai;
		goto And;
	} else if (Ai <= Argc) --Ai;
	return(p1);
}
struct anode *e2() { /* parse NOT (!) */
	int not();

	if (Randlast) {
		pr("find: operand follows operand\n");
		exit(1);
	}
	Randlast++;
	if (EQ(nxtarg(), "!"))
		return(mk(not, e3(), (struct anode *)0));
	else if (Ai <= Argc) --Ai;
	return(e3());
}
struct anode *e3() { /* parse parens and predicates */
	int exeq(), ok(), glob(),  mtime(), atime(), user(),
		group(), size(), perm(), links(), print(),
		type(), ino(), newer();
	struct anode *p1;
	int i;
	int s;
	register char *a, *b;

	a = nxtarg();
	if (EQ(a, "(")) {
		Randlast--;
		p1 = exp();
		a = nxtarg();
		if (!EQ(a, ")")) goto err;
		return(p1);
	}
	else if (EQ(a, "-print")) {
		return(mk(print, (struct anode *)0, (struct anode *)0));
	}
	b = nxtarg();
	s = *b;
	if (s=='+') b++;
	if (EQ(a, "-name"))
		return(mk(glob, (struct anode *)b, (struct anode *)0));
	else if (EQ(a, "-mtime"))
		return(mk(mtime, (struct anode *)atoi(b), (struct anode *)s));
	else if (EQ(a, "-atime"))
		return(mk(atime, (struct anode *)atoi(b), (struct anode *)s));
	else if (EQ(a, "-user")) {
		if ((i=getunum("/etc/passwd", b)) == -1) {
			if (gmatch(b, "[0-9][0-9][0-9]*")
			|| gmatch(b, "[0-9][0-9]")
			|| gmatch(b, "[0-9]"))
				return mk(user, (struct anode *)atoi(b), (struct anode *)s);
			pr("find: cannot find -user name\n");
			exit(1);
		}
		return(mk(user, (struct anode *)i, (struct anode *)s));
	}
	else if (EQ(a, "-inum"))
		return(mk(ino, (struct anode *)atoi(b), (struct anode *)s));
	else if (EQ(a, "-group")) {
		if ((i=getunum("/etc/group", b)) == -1) {
			if (gmatch(b, "[0-9][0-9][0-9]*")
			|| gmatch(b, "[0-9][0-9]")
			|| gmatch(b, "[0-9]"))
				return mk(group, (struct anode *)atoi(b), (struct anode *)s);
			pr("find: cannot find -group name\n");
			exit(1);
		}
		return(mk(group, (struct anode *)i, (struct anode *)s));
	} else if (EQ(a, "-size"))
		return(mk(size, (struct anode *)atoi(b), (struct anode *)s));
	else if (EQ(a, "-links"))
		return(mk(links, (struct anode *)atoi(b), (struct anode *)s));
	else if (EQ(a, "-perm")) {
		for (i=0; *b ; ++b) {
			if (*b=='-') continue;
			i <<= 3;
			i = i + (*b - '0');
		}
		return(mk(perm, (struct anode *)i, (struct anode *)s));
	}
	else if (EQ(a, "-type")) {
		i = s=='d' ? S_IFDIR :
		    s=='b' ? S_IFBLK :
		    s=='c' ? S_IFCHR :
		    s=='f' ? S_IFREG :
		    0;
		return(mk(type, (struct anode *)i, (struct anode *)0));
	}
	else if (EQ(a, "-exec")) {
		i = Ai - 1;
		while (!EQ(nxtarg(), ";"));
		return(mk(exeq, (struct anode *)i, (struct anode *)0));
	}
	else if (EQ(a, "-ok")) {
		i = Ai - 1;
		while (!EQ(nxtarg(), ";"));
		return(mk(ok, (struct anode *)i, (struct anode *)0));
	}
	else if (EQ(a, "-newer")) {
		if (stat(b, &Statb) < 0) {
			pr("find: cannot access "), pr(b), pr("\n");
			exit(1);
		}
		Newer = Statb.st_mtime;
		return mk(newer, (struct anode *)0, (struct anode *)0);
	}
err:	pr("find: bad option "), pr(a), pr("\n");
	exit(1);
}
struct anode *mk(f, l, r)
int (*f)();
struct anode *l, *r;
{
	Node[Nn].F = f;
	Node[Nn].L = l;
	Node[Nn].R = r;
	return(&(Node[Nn++]));
}

char *nxtarg() { /* get next arg from command line */
	static strikes = 0;

	if (strikes==3) {
		pr("find: incomplete statement\n");
		exit(1);
	}
	if (Ai>=Argc) {
		strikes++;
		Ai = Argc + 1;
		return("");
	}
	return(Argv[Ai++]);
}

/* execution time functions */
and(p)
register struct anode *p;
{
	return(((*p->L->F)(p->L)) && ((*p->R->F)(p->R))?1:0);
}
or(p)
register struct anode *p;
{
	 return(((*p->L->F)(p->L)) || ((*p->R->F)(p->R))?1:0);
}
not(p)
register struct anode *p;
{
	return( !((*p->L->F)(p->L)));
}
glob(p)
struct anode *p;
{
	return(gmatch(Fname, (char *)p->L));
}
print()
{
	puts(Pathname);
	return(1);
}
mtime(p)
struct anode *p;
{
	return(scomp((int)((Now - Statb.st_mtime) / A_DAY), (int)p->L, (char)p->R));
}
atime(p)
struct anode *p;
{
	return(scomp((int)((Now - Statb.st_rtime) / A_DAY), (int)p->L, (char)p->R));
}
user(p)
struct anode *p;
{
	return(scomp(Statb.st_uid, (int)p->L, (char)p->R));
}
ino(p)
struct anode *p;
{
	return(scomp((int)Statb.st_ino, (int)p->L, (char)p->R));
}
group(p)
struct anode *p;
{
	return((int)p->L == Statb.st_gid);
}
links(p)
struct anode *p;
{
	return(scomp(Statb.st_nlink, (int)p->L, (char)p->R));
}
size(p)
struct anode *p;
{
	long sz;

	/* micronix has no st_size: 24 bits across st_size0 and st_size1 */
	sz = Statb.st_size1 + ((long)Statb.st_size0 << 16);
	return(scomp((int)((sz + 511) >> 9), (int)p->L, (char)p->R));
}
perm(p)
struct anode *p;
{
	register i;
	i = ((char)p->R == '-') ? (int)p->L : 07777; /* '-' means only arg bits */
	return((Statb.st_mode & i & 07777) == (int)p->L);
}
type(p)
struct anode *p;
{
	return((Statb.st_mode & S_IFMT) == (int)p->L);
}
exeq(p)
struct anode *p;
{
	fflush(stdout); /* to flush possible `-print' */
	return(doex((int)p->L));
}
ok(p)
struct anode *p;
{
	int c;  int yes;
	yes = 0;
	fflush(stdout); /* to flush possible `-print' */
	pr("< "), pr(Argv[(int)p->L]), pr(" ... "), pr(Pathname), pr(" >?   ");
	fflush(stderr);
	if ((c=getchar())=='y') yes = 1;
	while (c!='\n')
		if (c==EOF)
			exit(2);
		else
			c = getchar();
	if (yes) return(doex((int)p->L));
	return(0);
}
newer()
{
	return Statb.st_mtime > Newer;
}

/* support functions */
scomp(a, b, s) /* funny signed compare */
register a, b;
register char s;
{
	if (s == '+')
		return(a > b);
	if (s == '-')
		return(a < (b * -1));
	return(a == b);
}

doex(com)
{
	register np;
	register char *na;
	static char *nargv[50];
	static ccode;

	ccode = np = 0;
	while (na=Argv[com++]) {
		if (strcmp(na, ";")==0) break;
		if (strcmp(na, "{}")==0) nargv[np++] = Pathname;
		else nargv[np++] = na;
	}
	nargv[np] = 0;
	if (np==0) return(9);
	if (fork()) /*parent*/ wait(&ccode);
	else { /*child*/
		execv(nargv[0], nargv);
		exit(1);
	}
	return(ccode ? 0:1);
}

getunum(f, s) char *f, *s; { /* find user/group name and return number */
	register i;
	register char *sp;
	register c;
	char str[20];
	FILE *pin;

	i = -1;
	pin = fopen(f, "r");
	c = '\n'; /* prime with a CR */
	do {
		if (c=='\n') {
			sp = str;
			for (;;) {
				c = getc(pin);
				if (c == EOF) goto RET;
				*sp++ = c;
				if (c == ':') break;
			}
			*--sp = '\0';
			if (EQ(str, s)) {
				for (;;) {
					c = getc(pin);
					if (c == EOF) goto RET;
					if (c == ':') break;
				}
				sp = str;
				for (;;) {
					c = getc(pin);
					if (c == EOF) goto RET;
					if (c == ':') break;
					*sp++ = c;
				}
				*sp = '\0';
				i = atoi(str);
				goto RET;
			}
		}
	} while ((c = getc(pin)) != EOF);
 RET:
	fclose(pin);
	return(i);
}

descend(name, exlist)
struct anode *exlist;
char *name;
{
	struct direct dentry;
	int dirfd, n;
	register char *c1, *c2;
	register int i;
	char *endofname;

	if (stat(name, &Statb) < 0) {
		pr("find: bad status-- "), pr(name), pr("\n");
		return(0);
	}
	(*exlist->F)(exlist);
	if ((Statb.st_mode & S_IFMT) != S_IFDIR)
		return(1);

	if ((dirfd = open(name, 0)) < 0) {
		pr("find: cannot open "), pr(name), pr("\n");
		return(0);
	}

	/* point at the end of name; each entry is appended here */
	for (c1 = name; *c1; ++c1)
		;
	endofname = c1;

	while ((n = read(dirfd, &dentry, sizeof dentry)) == sizeof dentry) {
		if (dentry.d_ino == 0
		 || (dentry.d_name[0]=='.' && dentry.d_name[1]=='\0')
		 || (dentry.d_name[0]=='.' && dentry.d_name[1]=='.' && dentry.d_name[2]=='\0'))
			continue;
		if (endofname[-1] != '/')
			*c1++ = '/';
		Fname = c1;	/* c1 is where the leaf name is written */
		c2 = dentry.d_name;
		for (i=0; i<14; ++i)
			if (*c2)
				*c1++ = *c2++;
			else
				break;
		*c1 = '\0';
		descend(name, exlist);
		*endofname = '\0';
		c1 = endofname;
	}
	close(dirfd);
	return(1);
}

gmatch(s, p) /* string match as in glob */
register char *s, *p;
{
	if (*s=='.' && *p!='.') return(0);
	return amatch(s, p);
}

amatch(s, p)
register char *s, *p;
{
	register cc;
	int scc, k;
	int c, lc;

	scc = *s;
	lc = 077777;
	switch (c = *p) {

	case '[':
		k = 0;
		while (cc = *++p) {
			switch (cc) {

			case ']':
				if (k)
					return(amatch(++s, ++p));
				else
					return(0);

			case '-':
				cc = p[1];
				if (lc <= scc && scc <= cc)
					k = 1;
				break;

			default:
				break;
			}
			lc = cc;
			if (scc == cc)
				k = 1;
		}
		return(0);

	case '?':
	caseq:
		if (scc) return(amatch(++s, ++p));
		return(0);
	case '*':
		return(umatch(s, ++p));
	case 0:
		return(!scc);
	}
	if (c==scc) goto caseq;
	return(0);
}

umatch(s, p)
register char *s, *p;
{
	if (*p==0) return(1);
	while (*s)
		if (amatch(s++, p)) return(1);
	return(0);
}

pr(s)
char *s;
{
	fputs(s, stderr);
}
