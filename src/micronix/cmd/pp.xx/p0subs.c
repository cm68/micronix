/*
 * DEFINE/INCLUDE SUBROUTINES
 * copyright (c) 1978 by Whitesmiths, Ltd.
 *
 * pp/p0subs.c
 *
 * Changed: <2023-07-01 06:23:13 curt>
 */

#include <stdio.h>
#include "int0.h"


/*
 * the hash table definitions
 */
static struct def *htable[NHASH] = { 0 };

/*
 * allocate a new token for the tlist and copy a previous into it
 */
struct tlist *
buytl(p, link)
register struct tlist *p, *link;
{
	register struct tlist *q;

	q = lalloc(sizeof(*q), link);
	q->type = p->type;
	q->white = p->white;
	q->nwhite = p->nwhite;
	q->text = p->text;
	q->ntext = p->ntext;
	return (q);
}

/*
 * expand a single define line
 */
struct tlist *
dodef(pd, qn, args)
struct tlist *pd, *qn;
struct alist *args;
{
	register struct tlist **qb, *r, *s;
	struct alist *pa;
	short i, nargs;
	struct tlist *pe, *px, *qs;

	qb = &qs;
	nargs = 0;
	if (pd->nwhite || !punct(pd, '('))
		r = pd;
	else {
		r = pd->next;
		if (r->type == PIDENT) {
			nargs = 1;
			r = r->next;
		}
		while (r && !punct(r, ')'))
			if (punct(r, ',') && r->next->type == PIDENT) {
				r = r->next->next;
				++nargs;
			} else
				r = NULL;
		if (!r) {
			perror("bad #define arguments");
			r = pd;
		} else
			r = r->next;
	}
	for (pe = r; r->type != PEOL;) {
		if (r->type == PIDENT)
			for (s = pd->next, i = 0; i < nargs; ++i) {
				if (r->ntext == s->ntext
					&& cmpbuf(r->text, s->text, s->ntext))
					break;
				s = s->next->next;
			}
		if (r->type == PIDENT && i < nargs) {
			for (pa = args; 0 < i && pa; --i)
				pa = pa->next;
			if (pa)
				for (px = pa->astart; px != pa->aend; px = px->next) {
					*qb = buytl(px, r->next);
					if (px == pa->astart) {
						(*qb)->white = r->white;
						(*qb)->nwhite = r->nwhite;
					}
					qb = &(*qb)->next;
				}
			r = free(r, r->next);
		} else {
			*qb = r;
			qb = &(*qb)->next;
			r = r->next;
		}
	}
	while (pd != pe)
		pd = free(pd, pd->next);
	*qb = qn;
	return (qs);
}

/*
 * expand #defines in a line of tokens
 */
struct tlist *
doexp(qs)
struct tlist *qs;
{
	register struct tlist *pd, *q, **qb;
	struct alist *args;
	char *s;
	struct tlist *qn;

	for (qb = &qs; q = *qb;)
		if (q->type != PIDENT || !(s = lookup(q->text, q->ntext))
			|| *s == '(' && (q->next->nwhite || !punct(q->next, '(')))
			qb = &q->next;
		else {
			pd = stotl(s);
			args = NULL;
			if (*s == '(')
				qn = getargs(q->next, &args);
			else
				qn = q->next;
			*qb = dodef(pd, qn, args);
			if (*qb != qn) {
				(*qb)->white = q->white;
				(*qb)->nwhite = q->nwhite;
			}
			frelst(args, NULL);
			frelst(q, qn);
		}
	return (qs);
}

#ifdef notdef
/*
 * format error messages
 */
errfmt(fmt, args)
char *fmt, *args;
{
	extern short errfd;
	register unsigned short *p;
	register char *f, *s;
	char buf[10];

	for (p = &args, f = fmt; *f; ++f)
		if (*f != '%')
			write(errfd, f, 1);
		else if (*++f == 'c') {
			*buf = *p++;
			write(errfd, buf, 1);
		} else if (*f == 's')
			write(errfd, buf, itob(buf, *p++, 10));
		else {
			s = *p++;
			write(errfd, s, (*f == 'b') ? *p++ : lenstr(s));
		}
}
#endif

/*
 * get an argument list
 */
struct tlist *
getargs(ip, ap)
struct tlist *ip;
struct alist **ap;
{
	register struct alist **qb, *q;
	register struct tlist *p;
	short npar;

	p = ip;
	for (qb = ap, p = p->next; p && !punct(p, ')'); qb = &q->next) {
		*qb = q = alloc(sizeof(**ap));
		q->astart = p;
		for (npar = 0; p && (0 < npar || !punct(p, ',')
							 && !punct(p, ')')); p = p->next)
			if (punct(p, '('))
				++npar;
			else if (punct(p, ')'))
				--npar;
		if (p) {
			q->aend = p;
			if (punct(p, ','))
				p = p->next;
		}
	}
	if (p)
		return (p->next);
	else {
		perror("bad macro arguments");
		*ap = NULL;
		return (ip);
	}
}

#ifdef notdef
XXXXXXX
/*
 * get an included file name
 * this needs to handle the include path
 * and the <foo.h>
 * three cases, of which the first is archaic
 * #include foo
 * #include "foo.h"
 * #include <foo.h>
 */
char *
getfname(p)
register struct tlist *p;
{
	extern char *iprefix;
    extern struct incpath *incs;
    struct incpath *inctry;
    
	register char *q, *s;
	unsigned short n;
	short i;
	short fd;

	if (p->type == PIDENT) {
		q = buybuf(p->text, p->ntext + 1); /* DIRTY */
		q[p->ntext] = '\0';
	} else if (p->type == PSTRING) {
		q = buybuf(p->text + 1, p->ntext - 1);
		q[p->ntext - 2] = '\0';
	} else if (punct(p, '<')) {
		for (s = p->next->text; *s != '>' && !iswhite(*s); ++s);
		n = s - p->next->text;
		q = alloc(lenstr(iprefix) + n + 1);
		for (s = iprefix; *s; s += (s[i]) ? i + 1 : i) {
			cpybuf(q, s, i = scnstr(s, '|'));
			cpybuf(q + i, p->next->text, n);
			q[i + n] = '\0';
			if (0 <= (fd = open(q, READ, 1))) {
				close(fd);
				break;
			}
		}
	} else
		q = NULL;
	return (q);
}
#endif

/*
 * hash into symbol table
 */
struct def **
hash(s, n)
register char *s;
register unsigned short n;
{
	extern struct def *htable[];
	register short sum;

	for (sum = 0; 0 < n; --n)
		sum += *s++;
	return (&htable[sum % NHASH]);
}

/*
 * install definition in symbol table
 */
install(s, n, pd)
char *s;
register unsigned short n;
char *pd;
{
	register struct def **qb, *q;

	n = min(n, LENNAME);
	qb = hash(s, n);
	*qb = q = lalloc(sizeof(*q), *qb);
	cpybuf(q->dname, s, n);
	q->dnlen = n;
	q->defn = pd;
}

/*
 * lookup a definition in symbol table
 */
struct tlist *
lookup(s, n)
register char *s;
register unsigned short n;
{
	register struct def *q;

	n = min(n, LENNAME);
	for (q = *hash(s, n); q; q = q->next)
		if (n == q->dnlen && cmpbuf(s, q->dname, n))
			return (q->defn);
	return (NULL);
}

/*
 * get next file argument
 */
struct incl *
nxtfile()
{
	extern char pflag;
    extern char **sources;
    extern int srcfiles;
	struct incl *pi;
    FILE *in;

    if (!srcfiles--) {
        return NULL;
    }
    if (!strcmp(*sources, "-")) {
        in = stdin;
    } else if (!(in = fopen(*sources, "r"))) {
        perror(*sources);
        return NULL;
    }

	pi = alloc(sizeof(*pi));
    pi->fname = *sources;
	pflag = YES;
	pi->nline = 0;
    pi->file = in;
	return (pi);
}

/*
 * put an error message with line numbers
 */
errmsg(fmt, a0, a1)
char *fmt, *a0, *a1;
{
	extern short nerrors;
	extern struct incl *pincl;

	if (!pincl)
		fprintf(stderr, "EOF: ");
	else if (!pincl->fname)
		fprintf(stderr, "%d: ", pincl->nline);
	else
		fprintf(stderr, "%s %d: ", pincl->fname, pincl->nline);
	fprintf(stderr, fmt, a0, a1);
	fprintf(stderr, "\n");
	++nerrors;
}

static FILE *infile = 0;

bufaddc(c)
char c;
{
    infile->_ptr[infile->_cnt++] = c;
}

bufadd(s)
char *s;
{
    while (*s) {
        bufaddc(*s++);
    }
}

/*
 * install predefined args
 * this is a pretty collosal hack:
 * the first source file buffer is not read yet, so we fill
 * the buffer with definitions
 * we build up an initial file that contains
 * lines of the form:
 * #define FOO <value or default>
 *
 * NB: we manipulate the internals of the FILE structure
 */
predef()
{
	extern unsigned short pchar;
    extern struct incl *pincl;
    extern int ndefs;
    extern char *defines[];
	register short i;
	char *s;
    char noval;
    char c;

    infile = pincl->file;
    infile->_ptr = infile->_base;
    infile->_cnt = 0;

	for (i = 0; i < ndefs; i++) {
        noval = 1;
        bufaddc(pchar);
        bufadd("define ");
        for (s = defines[i]; c = *s; s++) {
            if (c == '=') {
                c = ' ';
                noval = 0;
            }
            bufaddc(c);
        } 
        if (noval) {
            bufadd(" 1");
        }
        bufaddc('\n');
    }
}

/*
 * is this a punctuation mark
 */
char
punct(p, c)
register struct tlist *p;
char c;
{
	return (p->ntext == 1 && *p->text == c);
}

/*
 * scan a table of predefined strings
 */
int
scntab(p, hi, s, n)
struct pretab *p;
unsigned short hi;
char *s;
unsigned short n;
{
	register unsigned short j;
	register char *q, *r;
	unsigned short i, lo;
	short x;

	for (lo = 0; lo < hi;) {
		i = (hi + lo) >> 1;
		r = p[i].prename;
		if (!(x = *r++ - n))
			for (j = 0, q = s; j < n; ++j)
				if (x = *r++ - *q++)
					break;
		if (x < 0)
			lo = i + 1;
		else if (x == 0)
			return (p[i].pretype);
		else
			hi = i;
	}
	return (0);
}

/*
 * convert a line to token list
 */
struct tlist *
stotl(s)
register char *s;
{
	register struct tlist *q, **qb;
	struct tlist *qs;

	qb = &qs;
	for (;;) {
		q = alloc(sizeof(*q));
		q->white = s;
		while (iswhite(*s) && *s != '\n')
			++s;
		q->nwhite = s - q->white;
		q->text = s;
		if (*s == '\n') {
			++s;
			q->type = PEOL;
		} else if (isalpha(*s) || *s == '_' || LENNAME != 8 && *s == '$') {
			while (isalpha(*s) || *s == '_' || LENNAME != 8 && *s == '$'
				   || isdigit(*s))
				++s;
			q->type = PIDENT;
		} else if (isdigit(*s)) {
			while (isalpha(*s) || isdigit(*s))
				++s;
			q->type = PNUM;
		} else if (*s == '"' || *s == '\'') {
			for (; *++s != *q->text && *s != '\n';)
				if (*s == '\\' && s[1] != '\n')
					++s;
				else if (*s == '\\')
					break;
			if (*s == *q->text)
				++s;
			else
				perror("unbalanced %c", *q->text);
			q->type = (*q->text == '"') ? PSTRING : PCHCON;
			if (STRSIZE < s - q->text) {
				perror("string too long");
				q->text = s - STRSIZE;
			}
		} else {
			++s;
			q->type = PPUNCT;
		}
		q->ntext = s - q->text;
		*qb = q;
		qb = &q->next;
		if (q->type == PEOL)
			return (qs);
	}
}

/*
 * remove latest definiton of a symbol
 */
undef(s, n)
register char *s;
unsigned short n;
{
	register struct def **qb, *q;

	n = min(n, LENNAME);
	for (qb = hash(s, n); q = *qb; qb = &q->next)
		if (n == q->dnlen && cmpbuf(s, q->dname, n))
			break;
	if (q) {
		free(q->defn, NULL);
		*qb = free(q, q->next);
	}
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
