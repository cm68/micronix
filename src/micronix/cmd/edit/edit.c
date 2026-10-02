/*
 * edit - the Software Tools line editor
 *
 * cmd/edit/edit.c
 *
 * The line editor from Kernighan & Plauger's "Software Tools"
 * (Addison-Wesley, 1976), Chapter 6.  The Ratfor source is beside this
 * file (the *.r files, with the common blocks ctxt cpat cbuf cfile
 * cscrat clines); this is a straight translation of it into C.
 *
 * It is NOT the same program as cmd/vi (STEVIE) and not the same as
 * ed - the command set and the regular expression dialect (?, @, [ ],
 * { }, *, %, $) are the Software Tools ones, and the man page spells
 * them out.
 *
 * Translation notes:
 *   - Ratfor strings are 1-based; the C keeps them 1-based, so every
 *     character array is one longer than it looks and [0] is unused.
 *   - Ratfor passes everything by reference, so the functions that
 *     advance an index or set a status take those as pointers.
 *   - The text lives on a scratch file; the buf array is a linked list
 *     of 5-word line descriptors (PREV NEXT MARK SEEKADR LENG).
 *   - All I/O is raw read/write on descriptors, as the original's was.
 *
 * vim: tabstop=4 shiftwidth=4 noexpandtab:
 */
/*
 * The raw system calls edit drives, declared the way ed.c does rather
 * than by pulling in unistd.h: that header also declares a seek(), and
 * edit has an io primitive of its own by that name (below), which the
 * redeclaration would collide with.
 */
long	lseek();
extern int read(), write(), open(), close(), creat(), unlink();

#define	EOS	'\0'
#define	NEWLINE	'\n'
#define	BLANK	' '
#define	TAB	'\t'
#define	PERIOD	'.'
#define	COMMA	','
#define	SEMICOL	';'
#define	PLUS	'+'
#define	MINUS	'-'
#define	SLASH	'/'
#define	BACKSLASH '\\'

#define	EOF	(-1)
#define	OK	1
#define	ERR	0
#define	YES	1
#define	NO	0

#define	STDIN	0
#define	STDOUT	1
#define	STDERR	2

#define	READ	0
#define	WRITE	1
#define	READWRITE 2

#define	MAXLINE	1000
#define	MAXPAT	128
#define	MAXBUF	1000
#define	BUFENT	5
#define	LINE0	1
#define	PREV	0
#define	NEXT	1
#define	MARK	2
#define	SEEKADR	3
#define	LENG	4
#define	FORWARD	0
#define	BACKWARD (-1)

#define	APPENDCOM 'a'
#define	CHANGE	  'c'
#define	DELCOM	  'd'
#define	ENTER	  'e'
#define	PRINTFIL  'f'
#define	READCOM	  'r'
#define	WRITECOM  'w'
#define	INSERT	  'i'
#define	PRINTCUR  '='
#define	MOVECOM	  'm'
#define	QUIT	  'q'
#define	SUBSTITUTE 's'
#define	GLOBAL	  'g'
#define	EXCLUDE	  'x'
#define	PRINT	  'p'

/* the common blocks */
char	txt[MAXLINE+1];	/* ctxt: text line for matching/output */
char	pat[MAXPAT+1];	/* cpat: pattern */
int	buf[MAXBUF+1];	/* cbuf: line descriptors */
int	lastbf;
char	savfil[MAXLINE+1];	/* cfile: remembered file name */
int	scr, scrend;	/* cscrat: scratch file */
int	line1, line2, nlines, curln, lastln;	/* clines */

int	_argc_;
char	**_argv_;

/* ---- the io library ---- */

int
getc(fd)
int fd;
{
	unsigned char c;

	if (read(fd, &c, 1) != 1)
		return EOF;
	return (int)c;
}

void
putc(c, fd)
int c, fd;
{
	char ch = c;

	write(fd, &ch, 1);
}

int
getlin(s, fd)
char *s;
int fd;
{
	int c, i;

	i = 1;
	for (;;) {
		c = getc(fd);
		if (c == EOF) {
			if (i == 1)
				return EOF;
			break;
		}
		s[i++] = c;
		if (c == NEWLINE)
			break;
	}
	s[i] = EOS;
	return OK;
}

void
putlin(s, fd)
char *s;
int fd;
{
	int i;

	for (i = 1; s[i] != EOS; i++)
		;
	write(fd, s + 1, i - 1);
}

int
length(s)
char *s;
{
	int n;

	for (n = 0; s[n+1] != EOS; n++)
		;
	return n;
}

int
index(s, c)
char *s;
int c;
{
	int i;

	for (i = 1; s[i] != EOS; i++)
		if (s[i] == c)
			return i;
	return 0;
}

void
scopy(from, i, to, j)
char *from, *to;
int i, j;
{
	for (; from[i] != EOS; i++, j++)
		to[j] = from[i];
	to[j] = EOS;
}

int
addset(c, s, i, max)
int c;
char *s;
int *i, max;
{
	if (*i >= max)
		return NO;
	s[(*i)++] = c;
	return YES;
}

int
ctoi(s, i)
char *s;
int *i;
{
	int n;

	for (n = 0; s[*i] >= '0' && s[*i] <= '9'; (*i)++)
		n = n * 10 + s[*i] - '0';
	return n;
}

void
skipbl(s, i)
char *s;
int *i;
{
	while (s[*i] == BLANK || s[*i] == TAB)
		(*i)++;
}

void
putdec(n, w)
int n, w;
{
	char tmp[16];
	int i;

	if (n < 0) {
		putc('-', STDOUT);
		n = -n;
	}
	i = 0;
	do {
		tmp[i++] = '0' + n % 10;
		n /= 10;
	} while (n > 0);
	while (i < w) {
		putc(' ', STDOUT);
		w--;
	}
	while (--i >= 0)
		putc(tmp[i], STDOUT);
}

void
remark(s)
char *s;
{
	putlin(s, STDERR);
	putc(NEWLINE, STDERR);
}

int
min(a, b)
int a, b;
{
	return (a < b) ? a : b;
}

int
getarg(n, s, max)
int n;
char *s;
int max;
{
	char *a;
	int i;

	if (n < 1 || n >= _argc_)
		return EOF;
	a = _argv_[n];
	for (i = 1; i < max && *a; i++)
		s[i] = *a++;
	s[i] = EOS;
	return OK;
}

/* the file layer: create/remove/seek, on the raw descriptors */
int
create(name, mode)
char *name;
int mode;
{
	int fd;

	fd = creat(name, 0666);
	if (fd < 0)
		return ERR;
	if (mode == READWRITE) {
		close(fd);
		fd = open(name, READWRITE);
		if (fd < 0)
			return ERR;
	}
	return fd;
}

void
remove(name)
char *name;
{
	unlink(name);
}

void
seek(offset, fd)
int offset, fd;
{
	lseek(fd, (long)offset, 0);
}

void
readf(s, len, fd)
char *s;
int len, fd;
{
	read(fd, s + 1, len);
}

void
cant(name)
char *name;
{
	remark("can't create scratch file");
	exit(1);
}

/* ---- the pattern matcher ----
 * The Software Tools regular expression dialect: % and $ anchor to the
 * start and end of the line, ? matches any character, @c quotes c,
 * [..] and [^..] are character classes with a-b ranges, * is closure,
 * and {..} groups for the substitute command's @n.  match returns YES
 * if pat matches anywhere in lin; amatch returns the position just past
 * the leftmost-longest match at lin[i], or 0.
 */

static int ngrp;
static int grp_st[10], grp_en[10];

static int amatch(char *, int, char *, int);
static int patskip(char *, int);

static int
matchone(lin, i, pat, j)
char *lin, *pat;
int i, j;
{
	switch (pat[j]) {
	case '?':
		return (lin[i] != EOS) ? i + 1 : 0;
	case '@':
		return (lin[i] == pat[j+1]) ? i + 1 : 0;
	case '[': {
		int neg = 0, matched = 0, lo, hi;

		j++;
		if (pat[j] == '^') {
			neg = 1;
			j++;
		}
		for (;;) {
			if (pat[j] == EOS)
				return 0;
			lo = pat[j++];
			if (pat[j] == '-' && pat[j+1] != ']') {
				hi = pat[j+1];
				j += 2;
			} else
				hi = lo;
			if (lin[i] >= lo && lin[i] <= hi)
				matched = 1;
			if (pat[j] == ']') {
				j++;
				break;
			}
		}
		if (matched == neg)
			return 0;
		return i + 1;
	}
	case '{': {
		int g, en;

		g = ++ngrp;
		en = amatch(lin, i, pat, j + 1);	/* body, stops at } */
		if (en == 0)
			return 0;
		grp_st[g] = i;
		grp_en[g] = en;
		return en;
	}
	default:
		return (lin[i] == pat[j]) ? i + 1 : 0;
	}
}

static int
amatch(lin, i, pat, j)
char *lin, *pat;
int i, j;
{
	int k, save;

	for (;;) {
		if (pat[j] == EOS || pat[j] == '}')
			return i;
		if (pat[j] == '%')
			return (i == 1) ? amatch(lin, i, pat, j + 1) : 0;
		if (pat[j] == '$')
			return (lin[i] == EOS) ? amatch(lin, i, pat, j + 1) : 0;
		if (pat[j+1] == '*') {
			save = i;
			do {
				k = amatch(lin, i, pat, j + 2);
				if (k > 0)
					return k;
				i = matchone(lin, i, pat, j);
			} while (i > save);
			return 0;
		}
		i = matchone(lin, i, pat, j);
		if (i == 0)
			return 0;
		j = patskip(pat, j);
	}
}

static int
patskip(pat, j)
char *pat;
int j;
{
	switch (pat[j]) {
	case '@':
		return j + 2;
	case '[': {
		j++;
		if (pat[j] == '^')
			j++;
		while (pat[j] != EOS && pat[j] != ']')
			j++;
		return (pat[j] == ']') ? j + 1 : j;
	}
	case '{': {
		int d = 1;

		j++;
		while (d > 0 && pat[j] != EOS) {
			if (pat[j] == '{')
				d++;
			else if (pat[j] == '}')
				d--;
			j++;
		}
		return j;
	}
	default:
		return j + 1;
	}
}

int
match(lin, pat)
char *lin, *pat;
{
	int i;

	for (i = 1; lin[i] != EOS; i++) {
		ngrp = 0;
		if (amatch(lin, i, pat, 1) > 0)
			return YES;
	}
	return NO;
}

int
makpat(lin, i, delim, pat)
char *lin, *pat;
int i, delim;
{
	int j;

	for (j = 1; lin[i] != delim && lin[i] != EOS && lin[i] != NEWLINE; ) {
		if (lin[i] == '@' && lin[i+1] != EOS)
			i++;
		pat[j++] = lin[i++];
	}
	if (lin[i] != delim)
		return ERR;
	pat[j] = EOS;
	return i + 1;
}

int
maksub(lin, i, delim, sub)
char *lin, *sub;
int i, delim;
{
	int j;

	for (j = 1; lin[i] != delim && lin[i] != EOS; ) {
		if (lin[i] == '@' && lin[i+1] != EOS && lin[i+1] != '0')
			i++;
		sub[j++] = lin[i++];
	}
	sub[j] = EOS;
	return i;
}

void
catsub(txt, k, m, sub, new, j, max)
char *txt, *sub, *new;
int k, m, *j, max;
{
	int i, n;

	for (i = 1; i < k; i++)
		if (addset(txt[i], new, j, max) == NO)
			return;
	for (i = 1; sub[i] != EOS; i++) {
		if (sub[i] == '&')
			for (n = k; n < m; n++)
				if (addset(txt[n], new, j, max) == NO)
					return;
		else if (sub[i] == '@' && sub[i+1] >= '1' && sub[i+1] <= '9') {
			int g = sub[i+1] - '0';

			i++;
			for (n = grp_st[g]; n < grp_en[g]; n++)
				if (addset(txt[n], new, j, max) == NO)
					return;
		} else if (addset(sub[i], new, j, max) == NO)
			return;
	}
}

/* ---- the editor ---- */

void	relink();	/* called by setbuf before its definition below */

void
setbuf()
{
	int k;

	savfil[1] = EOS;
	scr = create("scratch", READWRITE);
	if (scr == ERR)
		cant("scratch");
	scrend = 0;
	lastbf = LINE0;
	txt[1] = EOS;
	maklin(txt, 1, &k);		/* empty line 0 */
	relink(k, k, k, k);
	curln = 0;
	lastln = 0;
}

void
clrbuf()
{
	close(scr);
	remove("scratch");
}

void
relink(a, x, y, b)
int a, x, y, b;
{
	buf[x + PREV] = a;
	buf[y + NEXT] = b;
}

int
getind(line)
int line;
{
	int j, k;

	k = LINE0;
	for (j = 0; j < line; j++)
		k = buf[k + NEXT];
	return k;
}

int
nextln(line)
int line;
{
	int n = line + 1;

	return (n > lastln) ? 0 : n;
}

int
prevln(line)
int line;
{
	int n = line - 1;

	return (n < 0) ? lastln : n;
}

int
maklin(lin, i, newind)
char *lin;
int i, *newind;
{
	int j, txtend;

	if (lastbf + BUFENT > MAXBUF)
		return ERR;
	txtend = 1;
	for (j = i; lin[j] != EOS; ) {
		addset(lin[j], txt, &txtend, MAXLINE);
		j++;
		if (lin[j-1] == NEWLINE)
			break;
	}
	if (addset(EOS, txt, &txtend, MAXLINE) == NO)
		return ERR;
	seek(scrend, scr);
	buf[lastbf + SEEKADR] = scrend;
	buf[lastbf + LENG] = length(txt);
	putlin(txt, scr);
	scrend += buf[lastbf + LENG];
	buf[lastbf + MARK] = NO;
	*newind = lastbf;
	lastbf += BUFENT;
	return j;
}

int
inject(lin)
char *lin;
{
	int i, k1, k2, k3;

	for (i = 1; lin[i] != EOS; ) {
		i = maklin(lin, i, &k3);
		if (i == ERR)
			return ERR;
		k1 = getind(curln);
		k2 = getind(nextln(curln));
		relink(k1, k3, k3, k2);
		relink(k3, k2, k1, k3);
		curln++;
		lastln++;
	}
	return OK;
}

int
gettxt(line)
int line;
{
	int j, k;

	k = getind(line);
	seek(buf[k + SEEKADR], scr);
	readf(txt, buf[k + LENG], scr);
	j = buf[k + LENG] + 1;
	txt[j] = EOS;
	return k;
}

int
append(line, glob)
int line, glob;
{
	char lin[MAXLINE+1];

	if (glob == YES)
		return ERR;
	curln = line;
	for (;;) {
		if (getlin(lin, STDIN) == EOF)
			return EOF;
		if (lin[1] == PERIOD && lin[2] == NEWLINE)
			return OK;
		if (inject(lin) == ERR)
			return ERR;
	}
}

int
delete(from, to, status)
int from, to, *status;
{
	int k1, k2;

	if (from <= 0) {
		*status = ERR;
		return ERR;
	}
	k1 = getind(prevln(from));
	k2 = getind(nextln(to));
	lastln -= (to - from + 1);
	curln = prevln(from);
	relink(k1, k2, k1, k2);
	*status = OK;
	return OK;
}

int
move(line3)
int line3;
{
	int k0, k1, k2, k3, k4, k5;

	if (line1 <= 0 || (line1 <= line3 && line3 <= line2))
		return ERR;
	k0 = getind(prevln(line1));
	k3 = getind(nextln(line2));
	k1 = getind(line1);
	k2 = getind(line2);
	relink(k0, k3, k0, k3);
	if (line3 > line1) {
		curln = line3;
		line3 -= (line2 - line1 + 1);
	} else
		curln = line3 + (line2 - line1 + 1);
	k4 = getind(line3);
	k5 = getind(nextln(line3));
	relink(k4, k1, k2, k5);
	relink(k2, k5, k4, k1);
	return OK;
}

int
doprnt(from, to)
int from, to;
{
	int i;

	if (from <= 0)
		return ERR;
	for (i = from; i <= to; i++) {
		gettxt(i);
		putlin(txt, STDOUT);
	}
	curln = to;
	return OK;
}

int
doread(line, file)
int line;
char *file;
{
	char lin[MAXLINE+1];
	int count, fd;

	fd = open(file, READ);
	if (fd == ERR)
		return ERR;
	curln = line;
	for (count = 0; getlin(lin, fd) != EOF; count++) {
		if (inject(lin) == ERR)
			break;
	}
	close(fd);
	putdec(count, 1);
	putc(NEWLINE, STDOUT);
	return OK;
}

int
dowrit(from, to, file)
int from, to;
char *file;
{
	int fd, line;

	fd = create(file, WRITE);
	if (fd == ERR)
		return ERR;
	for (line = from; line <= to; line++) {
		gettxt(line);
		putlin(txt, fd);
	}
	close(fd);
	putdec(to - from + 1, 1);
	putc(NEWLINE, STDOUT);
	return OK;
}

int
defalt(def1, def2, status)
int def1, def2, *status;
{
	if (nlines == 0) {
		line1 = def1;
		line2 = def2;
	}
	if (line1 > line2 || line1 <= 0)
		*status = ERR;
	else
		*status = OK;
	return *status;
}

int
ckp(lin, i, pflag, status)
char *lin;
int i, *pflag, *status;
{
	int j;

	j = i;
	if (lin[j] == PRINT) {
		j++;
		*pflag = YES;
	} else
		*pflag = NO;
	*status = (lin[j] == NEWLINE) ? OK : ERR;
	return *status;
}

int
getnum(lin, i, pnum, status)
char *lin;
int *i, *pnum, *status;
{
	static char digits[] = " 0123456789";
	int r;

	if (index(digits, lin[*i]) > 0) {
		*pnum = ctoi(lin, i);
		(*i)--;
		r = OK;
	} else if (lin[*i] == PERIOD) {
		*pnum = curln;
		r = OK;
	} else if (lin[*i] == '$') {
		*pnum = lastln;
		r = OK;
	} else if (lin[*i] == SLASH || lin[*i] == BACKSLASH) {
		if (optpat(lin, i) == ERR)
			r = ERR;
		else if (lin[*i] == SLASH)
			r = ptscan(FORWARD, pnum);
		else
			r = ptscan(BACKWARD, pnum);
	} else
		r = EOF;
	if (r == OK)
		(*i)++;
	*status = r;
	return r;
}

int
getone(lin, i, num, status)
char *lin;
int *i, *num, *status;
{
	int istart, mul, pnum, r;

	istart = *i;
	*num = 0;
	skipbl(lin, i);
	if (getnum(lin, i, num, status) == OK) {
		for (;;) {
			skipbl(lin, i);
			if (lin[*i] != PLUS && lin[*i] != MINUS) {
				*status = EOF;
				break;
			}
			mul = (lin[*i] == PLUS) ? +1 : -1;
			(*i)++;
			skipbl(lin, i);
			if (getnum(lin, i, &pnum, status) == OK)
				*num += mul * pnum;
			if (*status == EOF)
				*status = ERR;
			if (*status != OK)
				break;
		}
	}
	if (*num < 0 || *num > lastln)
		*status = ERR;

	if (*status == ERR)
		r = ERR;
	else if (*i <= istart)
		r = EOF;
	else
		r = OK;
	*status = r;
	return r;
}

int
getlst(lin, i, status)
char *lin;
int *i, *status;
{
	int num;

	line2 = 0;
	for (nlines = 0; getone(lin, i, &num, status) == OK; ) {
		line1 = line2;
		line2 = num;
		nlines++;
		if (lin[*i] != COMMA && lin[*i] != SEMICOL)
			break;
		if (lin[*i] == SEMICOL)
			curln = num;
		(*i)++;
	}
	nlines = min(nlines, 2);
	if (nlines == 0)
		line2 = curln;
	if (nlines <= 1)
		line1 = line2;
	if (*status != ERR)
		*status = OK;
	return *status;
}

int
optpat(lin, i)
char *lin;
int *i;
{
	int r;

	if (lin[*i] == EOS)
		r = ERR;
	else if (lin[*i+1] == EOS)
		r = ERR;
	else if (lin[*i+1] == lin[*i]) {
		(*i)++;
		r = OK;
	} else
		r = makpat(lin, *i + 1, lin[*i], pat);
	if (pat[1] == EOS)
		r = ERR;
	if (r == ERR) {
		pat[1] = EOS;
		return ERR;
	}
	return OK;
}

int
ptscan(way, num)
int way, *num;
{
	*num = curln;
	for (;;) {
		*num = (way == FORWARD) ? nextln(*num) : prevln(*num);
		gettxt(*num);
		if (match(txt, pat) == YES)
			return OK;
		if (*num == curln)
			break;
	}
	return ERR;
}

int
getfn(lin, i, file)
char *lin, *file;
int i;
{
	int j, k;

	if (lin[i+1] == BLANK) {
		j = i + 2;
		skipbl(lin, &j);
		for (k = 1; lin[j] != NEWLINE; k++) {
			file[k] = lin[j];
			j++;
		}
		file[k] = EOS;
		if (k > 1)
			goto ok;
	} else if (lin[i+1] == NEWLINE && savfil[1] != EOS) {
		scopy(savfil, 1, file, 1);
		goto ok;
	}
	return ERR;
ok:
	if (savfil[1] == EOS)
		scopy(file, 1, savfil, 1);
	return OK;
}

int
getrhs(lin, i, sub, gflag)
char *lin, *sub;
int *i, *gflag;
{
	if (lin[*i] == EOS)
		return ERR;
	if (lin[*i+1] == EOS)
		return ERR;
	*i = maksub(lin, *i + 1, lin[*i], sub);
	if (*i == ERR)
		return ERR;
	if (lin[*i+1] == GLOBAL) {
		(*i)++;
		*gflag = YES;
	} else
		*gflag = NO;
	return OK;
}

int
subst(sub, gflag)
char *sub;
int gflag;
{
	char new[MAXLINE+1];
	int g, j, k, lastm, line, m, st, subbed;

	if (line1 <= 0)
		return ERR;
	for (line = line1; line <= line2; line++) {
		j = 1;
		lastm = 0;
		subbed = NO;
		gettxt(line);
		ngrp = 0;
		for (k = 1; txt[k] != EOS; ) {
			if (gflag == YES || subbed == NO)
				m = amatch(txt, k, pat, 1);
			else
				m = 0;
			if (m > 0 && lastm != m) {
				subbed = YES;
				catsub(txt, k, m, sub, new, &j, MAXLINE);
				lastm = m;
			}
			if (m == 0 || m == k) {
				addset(txt[k], new, &j, MAXLINE);
				k++;
			} else
				k = m;
		}
		if (subbed == YES) {
			if (addset(EOS, new, &j, MAXLINE) == NO)
				return ERR;
			delete(line, line, &st);
			if (inject(new) == ERR)
				return ERR;
			line2 += curln - line;
			line = curln;
		}
	}
	return OK;
}

int
ckglob(lin, i, status)
char *lin;
int i, *status;
{
	int gflag, k, line;

	if (lin[i] != GLOBAL && lin[i] != EXCLUDE)
		*status = EOF;
	else {
		gflag = (lin[i] == GLOBAL) ? YES : NO;
		i++;
		if (optpat(lin, &i) == ERR || defalt(1, lastln, status) == ERR)
			*status = ERR;
		else {
			i++;
			for (line = line1; line <= line2; line++) {
				k = gettxt(line);
				buf[k+MARK] = (match(txt, pat) == gflag) ? YES : NO;
			}
			for (line = nextln(line2); line != line1; line = nextln(line)) {
				k = getind(line);
				buf[k+MARK] = NO;
			}
			*status = OK;
		}
	}
	return *status;
}

int
doglob(lin, i, cursav, status)
char *lin;
int i, cursav, *status;
{
	int count, istart, k, line, st;

	*status = OK;
	count = 0;
	line = line1;
	istart = i;
	for (;;) {
		k = getind(line);
		if (buf[k+MARK] == YES) {
			buf[k+MARK] = NO;
			curln = line;
			cursav = curln;
			i = istart;
			if (getlst(lin, &i, &st) == OK)
				if (docmd(lin, i, YES, &st) == OK)
					count = 0;
		} else {
			line = nextln(line);
			count++;
		}
		if (count > lastln || *status != OK)
			break;
	}
	return *status;
}

int
docmd(lin, i, glob, status)
char *lin;
int i, glob, *status;
{
	char file[MAXLINE+1], sub[MAXPAT+1];
	int gflag, line3, pflag, st;

	pflag = NO;
	*status = ERR;
	if (lin[i] == APPENDCOM) {
		if (lin[i+1] == NEWLINE)
			*status = append(line2, glob);
	} else if (lin[i] == CHANGE) {
		if (lin[i+1] == NEWLINE
		    && defalt(curln, curln, &st) == OK
		    && delete(line1, line2, &st) == OK)
			*status = append(prevln(line1), glob);
	} else if (lin[i] == DELCOM) {
		if (ckp(lin, i+1, &pflag, &st) == OK
		    && defalt(curln, curln, &st) == OK
		    && delete(line1, line2, &st) == OK
		    && nextln(curln) != 0)
			curln = nextln(curln);
	} else if (lin[i] == INSERT) {
		if (lin[i+1] == NEWLINE)
			*status = append(prevln(line2), glob);
	} else if (lin[i] == PRINTCUR) {
		if (ckp(lin, i+1, &pflag, &st) == OK) {
			putdec(line2, 1);
			putc(NEWLINE, STDOUT);
		}
	} else if (lin[i] == MOVECOM) {
		i++;
		if (getone(lin, &i, &line3, &st) == EOF)
			st = ERR;
		if (st == OK
		    && ckp(lin, i, &pflag, &st) == OK
		    && defalt(curln, curln, &st) == OK)
			*status = move(line3);
	} else if (lin[i] == SUBSTITUTE) {
		i++;
		if (optpat(lin, &i) == OK
		    && getrhs(lin, &i, sub, &gflag) == OK
		    && ckp(lin, i+1, &pflag, &st) == OK
		    && defalt(curln, curln, &st) == OK)
			*status = subst(sub, gflag);
	} else if (lin[i] == ENTER) {
		if (nlines == 0 && getfn(lin, i, file) == OK) {
			scopy(file, 1, savfil, 1);
			clrbuf();
			setbuf();
			*status = doread(0, file);
		}
	} else if (lin[i] == PRINTFIL) {
		if (nlines == 0 && getfn(lin, i, file) == OK) {
			scopy(file, 1, savfil, 1);
			putlin(savfil, STDOUT);
			putc(NEWLINE, STDOUT);
			*status = OK;
		}
	} else if (lin[i] == READCOM) {
		if (getfn(lin, i, file) == OK)
			*status = doread(line2, file);
	} else if (lin[i] == WRITECOM) {
		if (getfn(lin, i, file) == OK
		    && defalt(1, lastln, &st) == OK)
			*status = dowrit(line1, line2, file);
	} else if (lin[i] == PRINT) {
		if (lin[i+1] == NEWLINE
		    && defalt(curln, curln, &st) == OK)
			*status = doprnt(line1, line2);
	} else if (lin[i] == NEWLINE) {
		if (nlines == 0)
			line2 = nextln(curln);
		*status = doprnt(line2, line2);
	} else if (lin[i] == QUIT) {
		if (lin[i+1] == NEWLINE && nlines == 0 && glob == NO)
			*status = EOF;
	}
	if (*status == OK && pflag == YES)
		*status = doprnt(curln, curln);
	return *status;
}

int
main(argc, argv)
int argc;
char **argv;
{
	char lin[MAXLINE+1];
	int cursav, i, status;

	_argc_ = argc;
	_argv_ = argv;

	setbuf();
	pat[1] = EOS;
	if (getarg(1, savfil, MAXLINE) != EOF)
		if (doread(0, savfil) == ERR)
			remark("?.");
	while (getlin(lin, STDIN) != EOF) {
		i = 1;
		cursav = curln;
		if (getlst(lin, &i, &status) == OK) {
			if (ckglob(lin, i, &status) == OK)
				status = doglob(lin, i, cursav, &status);
			else if (status != ERR)
				status = docmd(lin, i, NO, &status);
		}
		if (status == ERR) {
			remark("?.");
			curln = cursav;
		} else if (status == EOF)
			break;
	}
	clrbuf();
	return 0;
}
