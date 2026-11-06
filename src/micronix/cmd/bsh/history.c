#
/*
 * UNIX shell
 *
 * cmd/bsh/history.c
 *
 * A short ring of the lines typed at an interactive shell, and the
 * csh-style expansions that name them: !!, !n, !-n, !str, !?str?, and
 * the word designators !$, !*, !^, :n and :n-m.
 *
 * The ring is the shell's own storage.  It is not a variable, it is
 * not exported to children, and nothing the shell execs can see it -
 * there is no history in the environment.
 *
 * The line is expanded as text before it is parsed, which is what
 * makes !$ and !* work: the words of the old command are substituted
 * into the new one and split the way the new line's quotes say.
 */

#include	"defs.h"

/* the library header costs c0 heap in a 64K process; name what this file calls */
extern int read(int, char *, int);
extern int write(int, char *, int);

#define	HISTSIZE	16		/* memorable lines */
#define	HISTLEN		128		/* longest line kept, with NUL */
#define	HISTEXP		256		/* longest line served, expanded */
#define	HISTWORDS	64
#define	HISTWORDLEN	(HISTLEN + HISTWORDS)

/* what a word designator selects from an event line */
#define	DWHOLE	0
#define	DLAST	1
#define	DALL	2
#define	DFIRST	3
#define	DNUM	4
#define	DRANGE	5

/*
 * A flat buffer, not a two-dimensional array: ccc does not scale or
 * initialise char [][n] correctly, and the slot arithmetic is written
 * out below.
 */
static CHAR	histbuf[HISTSIZE * HISTLEN];
static INT	histn;			/* lines added; events are 1..histn */
static CHAR	rawbuf[HISTLEN];
static CHAR	expbuf[HISTEXP];
static CHAR	wordbuf[HISTWORDLEN];
static STRING	hwords[HISTWORDS];
static INT	hnwords;
STRING		histsrc;		/* where readc is to read the line */

static int
hdigit(c)
	int c;
{
	return (c >= '0' && c <= '9');
}

static int
hspace(c)
	int c;
{
	return (c == ' ' || c == '\t');
}

static int
hisdelim(c)
	int c;
{
	return (hspace(c) || c == ':' || c == '=' || c == '(' ||
	    c == ')' || c == '"' || c == '\'' || c == '\\' ||
	    c == ';' || c == '&' || c == '|' || c == '<' || c == '>');
}

static int
hlen(s)
	register char *s;
{
	register char *p;

	p = s;
	while (*p)
		p++;
	return (p - s);
}

static void
hcopy(d, s, n)
	register char *d;
	register char *s;
	int n;
{
	while (n-- > 0)
		*d++ = *s++;
}

static void
happend(op, end, s)
	register char **op;
	register char *end;
	register char *s;
{
	register char *p;

	p = *op;
	while (*s) {
		if (p < end)
			*p++ = *s;
		s++;
	}
	*op = p;
}

/*
 * One line, without its newline, into the ring.  Empty lines are not
 * remembered, and an overlong line is cut to what a slot holds.
 */
static void
histadd(s, n)
	register char *s;
	int n;
{
	register char *p;

	if (n <= 0)
		return;
	if (n > HISTLEN - 1)
		n = HISTLEN - 1;
	histn++;
	p = histbuf + ((histn - 1) % HISTSIZE) * HISTLEN;
	hcopy(p, s, n);
	p[n] = 0;
}

/*
 * Event n, or 0 if it has fallen out of the ring or was never there.
 */
static char *
histline(n)
	int n;
{
	if (n < 1 || n > histn || n <= histn - HISTSIZE)
		return (0);
	return (histbuf + ((n - 1) % HISTSIZE) * HISTLEN);
}

/*
 * The ring, read-only, for the line editor.
 */
INT
histcount()
{
	return (histn);
}

INT
histfirst()
{
	INT f;

	f = histn - HISTSIZE + 1;
	return (f < 1 ? 1 : f);
}

STRING
histevent(n)
	int n;
{
	return (histline(n));
}

static int
hprefix(s, p, n)
	register char *s;
	register char *p;
	int n;
{
	register int i;

	for (i = 0; i < n; i++)
		if (s[i] == 0 || s[i] != p[i])
			return (0);
	return (1);
}

static int
hcontains(s, p, n)
	register char *s;
	register char *p;
	int n;
{
	register int i, j;

	if (n == 0)
		return (1);
	for (i = 0; s[i]; i++) {
		for (j = 0; j < n; j++)
			if (s[i + j] != p[j])
				break;
		if (j == n)
			return (1);
	}
	return (0);
}

static int
histfindp(s, n)
	char *s;
	int n;
{
	register int i;
	register char *l;

	for (i = histn; i > 0; i--) {
		l = histline(i);
		if (l && hprefix(l, s, n))
			return (i);
	}
	return (0);
}

static int
histfindc(s, n)
	char *s;
	int n;
{
	register int i;
	register char *l;

	for (i = histn; i > 0; i--) {
		l = histline(i);
		if (l && hcontains(l, s, n))
			return (i);
	}
	return (0);
}

static void
histerr(s, n)
	register char *s;
	int n;
{
	write(2, "bsh: !", 6);
	write(2, s, n);
	write(2, ": event not found\n", 18);
}

/*
 * Split an event line into words, keeping the quotes that were typed
 * in the word, since the words are substituted back into a line that
 * the parser will quote afterwards.  Words are separated by unquoted
 * blanks.
 */
static int
histsplit(s)
	register char *s;
{
	register char *q;
	register char *t;

	q = wordbuf;
	t = s;
	hnwords = 0;
	while (*t) {
		while (hspace(*t))
			t++;
		if (*t == 0)
			break;
		if (hnwords < HISTWORDS)
			hwords[hnwords++] = q;
		while (*t) {
			if (*t == '\\' && t[1]) {
				*q++ = *t++;
				*q++ = *t++;
				continue;
			}
			if (*t == '\'') {
				*q++ = *t++;
				while (*t && *t != '\'')
					*q++ = *t++;
				if (*t)
					*q++ = *t++;
				continue;
			}
			if (*t == '"') {
				*q++ = *t++;
				while (*t && *t != '"') {
					if (*t == '\\' && t[1])
						*q++ = *t++;
					*q++ = *t++;
				}
				if (*t)
					*q++ = *t++;
				continue;
			}
			if (hspace(*t))
				break;
			*q++ = *t++;
		}
		*q++ = 0;
	}
	return (hnwords);
}

/*
 * Read one history spec at *ip (which points at the '!').  Returns 0
 * and leaves *pp past the spec, *evp at the event, *kindp at the word
 * designator and *d0p/*d1p the range.  Returns -1 if this '!' is not
 * a spec at all (a lone or blank-followed bang).
 */
static int
histspec(ip, pp, evp, kindp, d0p, d1p)
	register char *ip;
	char **pp;
	int *evp;
	int *kindp;
	int *d0p;
	int *d1p;
{
	register char *p;
	register char *s;
	int ev, kind, n, m, len;

	p = ip + 1;
	ev = 0;
	kind = DWHOLE;
	n = 0;
	m = 0;
	*d0p = -1;
	*d1p = -1;

	if (*p == '!') {
		ev = histn;
		p++;
	} else if (*p == '-' && hdigit(p[1])) {
		p++;
		while (hdigit(*p))
			n = n * 10 + *p++ - '0';
		ev = histn + 1 - n;
	} else if (hdigit(*p)) {
		while (hdigit(*p))
			n = n * 10 + *p++ - '0';
		ev = n;
	} else if (*p == '?') {
		s = ++p;
		while (*p && *p != '?')
			p++;
		len = p - s;
		if (*p == '?')
			p++;
		ev = histfindc(s, len);
	} else if (*p == '$' || *p == '*' || *p == '^') {
		ev = histn;
		if (*p == '$')
			kind = DLAST;
		else if (*p == '*')
			kind = DALL;
		else
			kind = DFIRST;
		p++;
		*evp = ev;
		*kindp = kind;
		*pp = p;
		return (0);
	} else if (*p == ':') {
		ev = histn;
	} else if (*p == 0 || hisdelim(*p)) {
		return (-1);
	} else {
		s = p;
		while (*p && !hisdelim(*p))
			p++;
		len = p - s;
		ev = histfindp(s, len);
	}

	if (*p == ':') {
		p++;
		if (*p == '$') {
			kind = DLAST;
			p++;
		} else if (*p == '*') {
			kind = DALL;
			p++;
		} else if (*p == '^') {
			kind = DFIRST;
			p++;
		} else if (hdigit(*p)) {
			n = 0;
			while (hdigit(*p))
				n = n * 10 + *p++ - '0';
			kind = DNUM;
			*d0p = n;
			if (*p == '-') {
				p++;
				m = 0;
				while (hdigit(*p))
					m = m * 10 + *p++ - '0';
				kind = DRANGE;
				*d1p = m;
			}
		} else {
			return (-1);
		}
	}
	*evp = ev;
	*kindp = kind;
	*pp = p;
	return (0);
}

/*
 * Append the chosen words of event ev to the output.
 */
static void
histrep(ev, kind, d0, d1, op, end)
	int ev, kind, d0, d1;
	register char **op;
	register char *end;
{
	register char *l;
	register int i, last;

	l = histline(ev);
	if (l == 0)
		return;
	if (kind == DWHOLE) {
		happend(op, end, l);
		return;
	}
	histsplit(l);
	last = hnwords - 1;
	switch (kind) {
	case DLAST:
		if (last >= 0)
			happend(op, end, hwords[last]);
		break;
	case DALL:
		for (i = 1; i <= last; i++) {
			if (i > 1)
				happend(op, end, " ");
			happend(op, end, hwords[i]);
		}
		break;
	case DFIRST:
		if (last >= 1)
			happend(op, end, hwords[1]);
		break;
	case DNUM:
		if (d0 >= 0 && d0 <= last)
			happend(op, end, hwords[d0]);
		break;
	case DRANGE:
		for (i = d0; i <= d1 && i <= last; i++) {
			if (i > d0)
				happend(op, end, " ");
			if (i >= 0)
				happend(op, end, hwords[i]);
		}
		break;
	}
}

/*
 * Expand every history spec in a command line.  A backslash protects
 * the next character, and nothing inside single quotes is a spec.
 * Returns -1 and has reported the failure when an event is missing.
 */
int
histexpand(in, out, max)
	register char *in;
	register char *out;
	int max;
{
	char *ip;		/* not register: histspec takes its address */
	char *op;		/* not register: histrep takes its address */
	char *end;
	char *sp;
	int ev, kind, d0, d1, sq, dq, r;

	ip = in;
	op = out;
	end = out + max - 1;
	sq = 0;
	dq = 0;
	while (*ip) {
		if (*ip == '\\' && ip[1]) {
			if (op < end)
				*op++ = *ip;
			ip++;
			if (op < end)
				*op++ = *ip;
			ip++;
			continue;
		}
		if (*ip == '\'' && !dq) {
			sq = !sq;
			if (op < end)
				*op++ = *ip;
			ip++;
			continue;
		}
		if (*ip == '"' && !sq) {
			dq = !dq;
			if (op < end)
				*op++ = *ip;
			ip++;
			continue;
		}
		if (*ip != '!' || sq) {
			if (op < end)
				*op++ = *ip;
			ip++;
			continue;
		}
		sp = ip;
		r = histspec(ip, &ip, &ev, &kind, &d0, &d1);
		if (r < 0) {
			if (op < end)
				*op++ = '!';
			ip = sp + 1;
			continue;
		}
		if (histline(ev) == 0) {
			histerr(sp + 1, ip - (sp + 1));
			return (-1);
		}
		histrep(ev, kind, d0, d1, &op, end);
	}
	*op = 0;
	return (0);
}

/*
 * Expand one line, remember the expanded form (as bash does: !* after
 * "echo a !$" uses the expanded words), and serve it, with its
 * newline, to readc.  Returns the bytes served.
 */
static INT
histserve(line, len, nl)
	char *line;
	int len;
	int nl;
{
	INT n, op;

	if (histexpand(line, expbuf, HISTEXP - 1) < 0) {
		/* a spec named nothing: drop the line */
		histadd(line, len);
		expbuf[0] = NL;
		expbuf[1] = 0;
		histsrc = expbuf;
		return (1);
	}
	n = hlen(expbuf);
	if (n > 0)
		histadd(expbuf, n);
	op = n;
	if (nl)
		expbuf[op++] = NL;
	expbuf[op] = 0;
	histsrc = expbuf;
	return (op);
}

/*
 * Read one physical line and serve it, expanded, to readc.  The
 * newline is part of what is served (except at end of file), because
 * readc hands the parser exactly these bytes.
 *
 * A terminal is given to the line editor first; it returns -2 when the
 * input is not a terminal after all (-i on a pipe), and -1 at end of
 * file.  An overlong line is read here, served unexpanded and cut.
 */
INT
histread(f)
	FILE	f;
{
	register int rc, n, stored, trunc, nl;
	register char *op;
	CHAR c;			/* not register: read() takes its address */
	int elen;

	elen = histedit(f, rawbuf, HISTLEN);
	if (elen >= 0) {
		rawbuf[elen] = 0;
		return (histserve(rawbuf, elen, 1));
	}
	if (elen == -1)
		return (0);

	n = 0;
	stored = 0;
	trunc = 0;
	nl = 0;
	c = 0;
	for (;;) {
		rc = read(f->fdes, &c, 1);
		if (rc < 0) {
			if (trapnote & SIGSET) {
				newline();
				sigchk();
				continue;
			}
			break;
		}
		if (rc == 0) {
			f->feof = TRUE;
			break;
		}
		if (c == NL) {
			nl = 1;
			break;
		}
		if (n < HISTEXP - 1)
			expbuf[n++] = c;
		else
			trunc = 1;
		if (stored < HISTLEN - 1)
			rawbuf[stored++] = c;
	}
	rawbuf[stored] = 0;
	expbuf[n] = 0;

	if (n == 0 && !nl)
		return (0);

	if (!trunc && stored == n)
		return (histserve(rawbuf, stored, nl));

	if (stored > 0)
		histadd(rawbuf, stored);
	op = expbuf + n;
	if (nl)
		*op++ = NL;
	*op = 0;
	histsrc = expbuf;
	return (op - expbuf);
}

/* vim: tabstop=4 shiftwidth=4 noexpandtab: */
