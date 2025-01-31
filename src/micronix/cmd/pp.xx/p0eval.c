/*
 * EXPRESSION EVALUATOR FOR #IF
 * copyright (c) 1978 by Whitesmiths, Ltd.
 *
 * pp/p0eval.c
 *
 * Changed: <2023-06-27 14:24:55 curt>
 */

#include <stdio.h>
#include "int0.h"
#include "int01.h"
#include "int012.h"

/*
 * escape sequences for c 
 */
static char eschars[] = { 
    "btnvfrBTNVFR(!)^" 
};

static char escodes[] = {
	010, 011, 012, 013, 014, 015, 010, 011, 012,
	013, 114, 015, 0173, 0174, 0175, 0176
};

/*
 * the priority tables 
 */
static char ipri[] = {
	LTIMES, LDIVIDE, LMODULO, LPLUS, LMINUS, LLSHIFT, LRSHIFT,
	LLESS, LLEQ, LGREAT, LGEQ, LISEQ, LNOTEQ,
	LAND, LXOR, LOR, LANDAND, LOROR, LQUERY, 0
};

static char opri[] = {
	14, 14, 14, 13, 13, 12, 12, 11, 11, 11, 11, 10, 10, 9, 8, 7, 6, 5, 4
};

/*
 * the operator table 
 */
#define NOPS	54
static struct pretab optab[] = {
	"\1!", LNOT,
	"\1%", LMODULO,
	"\1&", LAND,
	"\1(", LLPAREN,
	"\1)", LRPAREN,
	"\1*", LTIMES,
	"\1+", LPLUS,
	"\1,", LCOMMA,
	"\1-", LMINUS,
	"\1.", LDOT,
	"\1/", LDIVIDE,
	"\1:", LCOLON,
	"\1;", LSCOLON,
	"\1<", LLESS,
	"\1=", LGETS,
	"\1>", LGREAT,
	"\1?", LQUERY,
	"\1[", LLBRACK,
	"\1]", LRBRACK,
	"\1^", LXOR,
	"\1{", LLCURLY,
	"\1|", LOR,
	"\1}", LRCURLY,
	"\1~", LCOMP,
	"\2!=", LNOTEQ,
	"\2&&", LANDAND,
	"\2(<", LLCURLY,
	"\2(|", LLBRACK,
	"\2++", LINCR,
	"\2--", LDECR,
	"\2->", LPOINTS,
	"\2<<", LLSHIFT,
	"\2<=", LLEQ,
	"\2=%", LGMOD,
	"\2=&", LGAND,
	"\2=*", LGTIM,
	"\2=+", LGPLU,
	"\2=-", LGMIN,
	"\2=/", LGDIV,
	"\2==", LISEQ,
	"\2=^", LGXOR,
	"\2=|", LGOR,
	"\2>)", LRCURLY,
	"\2>=", LGEQ,
	"\2>>", LRSHIFT,
	"\2\\!", LOR,
	"\2\\(", LLCURLY,
	"\2\\)", LRCURLY,
	"\2\\^", LCOMP,
	"\2|)", LRBRACK,
	"\2||", LOROR,
	"\3=<<", LGLSH,
	"\3=>>", LGRSH,
	"\3\\!!", LOROR,
};

/*
 * do escape character processing 
 */
unsigned short
doesc(buf, s, n)
char *buf;
register char *s;
unsigned short n;
{
	extern char eschars[], escodes[];
	register char *q;
	register unsigned short i, j;
	short sum;

	q = buf;
	n = max(n, 2);
	for (i = 0; i < n; ++i)
		if (*s != '\\')
			*q++ = *s++;
		else if (eschars[j = scnstr(eschars, *++s)]) {
			*q++ = escodes[j];
			++s;
			++i;
		} else if (isdigit(*s)) {
			for (sum = 0, j = 0; j < 3 && isdigit(*s); ++j, ++s)
				sum = (sum << 3) + (*s - '0');
			*q++ = 0xff & sum;
			i += j;
		} else if (*s == 'x' || *s == 'X') {
			j = btos(++s, 3, &sum, 16);
			*q++ = 0xff & sum;
			i += j + 1;
			s += j;
		} else {
			*q++ = *s++;
			++i;
		}
	return (q - buf);			/* assert 2 <= (q - buf) */
}

/*
 * parse an operator 
 */
short
dopunct(pp)
struct tlist **pp;
{
	extern struct pretab optab[];
	register short n;
	register struct tlist *p, *q;
	char buf[3];
	short tok;

	p = *pp;
	buf[0] = *p->text;
	for (n = 1, q = p->next; n < 3 && q->type == PPUNCT && q->nwhite == 0;
		 ++n, q = q->next)
		buf[n] = *q->text;
	for (; 0 < n; --n)
		if (tok = scntab(optab, NOPS, buf, n))
			break;
	if (n <= 0) {
		errmsg("illegal character: %c", *p->text);
		*pp = p->next;
		return (LPLUS);
	} else {
		for (; 0 < n; --n)
			p = p->next;
		*pp = p;
		return (tok);
	}
}

/*
 * evaluate an expression 
 */
short
eval(p)
register struct tlist *p;
{
	long lv;

	if (!(p = expr(p, &lv)))
		return (NO);
	else if (p->type != PEOL) {
		errmsg("illegal #if expression");
		return (NO);
	} else
		return (lv != 0);
}

/*
 * test for operator 
 */
short
exop(pp, mask)
register struct tlist **pp;
short mask;
{
	register short tok;
	struct tlist *q;

	q = *pp;
	if (q->type == PPUNCT && (tok = dopunct(&q)) & mask) {
		*pp = q;
		return (tok);
	} else
		return (0);
}

/*
 * parse a (sub)expression 
 */
struct tlist *
expr(p, plv)
struct tlist *p;
long *plv;
{
	short op;

	if (!(p = exterm(p, plv)))
		return (NULL);
	else if (op = exop(&p, BINOP))
		return (extail(0, plv, &op, p));
	else
		return (p);
}

/*
 * compute operator priority 
 */
short
expri(op)
register short op;
{
	extern char ipri[], opri[];
	register unsigned short i;

	if (!op || !ipri[i = scnstr(ipri, op)])
		return (0);
	else
		return (opri[i]);
}

/*
 * parse a DTB... tail 
 */
struct tlist *
extail(lpr, plv, pop, p)
short lpr;
long *plv;
short *pop;
struct tlist *p;
{
	register short mpr;
	register short op;
	long lv, mv, rv;
	short rop;

	for (rop = *pop; lpr < (mpr = expri(rop)); *pop = rop) {
		if (!(p = exterm(p, &rv)));
		else if (mpr < expri(rop = exop(&p, BINOP)))
			p = extail(mpr, &rv, &rop, p);
		if (!p)
			return (NULL);
		op = *pop;
		lv = *plv;
		mv = lv - rv;
		switch (op) {
		case LPLUS:
			lv += rv;
			break;
		case LMINUS:
			lv = mv;
			break;
		case LTIMES:
			lv *= rv;
			break;
		case LDIVIDE:
			lv /= rv;
			break;
		case LMODULO:
			lv %= rv;
			break;
		case LAND:
			lv &= rv;
			break;
		case LOR:
			lv |= rv;
			break;
		case LXOR:
			lv ^= rv;
			break;
		case LLSHIFT:
			lv <<= rv;
			break;
		case LRSHIFT:
			lv >>= rv;
			break;
		case LLESS:
			lv = (mv < 0);
			break;
		case LISEQ:
			lv = (mv == 0);
			break;
		case LGREAT:
			lv = (mv > 0);
			break;
		case LLEQ:
			lv = (mv <= 0);
			break;
		case LNOTEQ:
			lv = (mv != 0);
			break;
		case LGEQ:
			lv = (mv >= 0);
			break;
		case LANDAND:
			lv = (lv && rv);
			break;
		case LOROR:
			lv = (lv || rv);
			break;
		case LQUERY:
			if (!punct(p, ':') || !(p = expr(p->next, &mv))) {
				errmsg("illegal ? : in #if");
				return (NULL);
			} else if (lv)
				lv = rv;
			else
				lv = mv;
			break;
		default:
			errmsg("illegal operator in #if");
			return (NULL);
		}
		*plv = lv;
	}
	return (p);
}

/*
 * parse a term 
 */
struct tlist *
exterm(p, plv)
struct tlist *p;
register long *plv;
{
	register unsigned short n;
	register char *s;
	short base;
	char sbuf[STRSIZE];
	short op;

	if (p->type == PNUM) {
		s = p->text;
		n = p->ntext;
		if (*s != '0')
			base = 10;
		else if (1 < n && tolower(s[1]) == 'x') {
			base = 16;
			s += 2;
			n -= 2;
		} else
			base = 8;
		if (btol(s, n, plv, base) != n) {
			errmsg("illegal number in #if");
			*plv = 0;
		}
		return (p->next);
	} else if (p->type == PCHCON) {
		s = &sbuf[1];
		n = doesc(sbuf, p->text, p->ntext) - 2;
		for (*plv = 0; n; ++s, --n)
			*plv = (*plv << 8) | *s & BYTMASK;
		return (p->next);
	} else if (punct(p, '('))
		if (!(p = expr(p->next, plv)))
			return (NULL);
		else if (!punct(p, ')')) {
			errmsg("missing ) in #if");
			return (NULL);
		} else
			return (p->next);
	else if (!(op = exop(&p, UNOP))) {
		errmsg("illegal #if syntax");
		return (NULL);
	} else if (!(p = exterm(p, plv)))
		return (NULL);
	else {
		if (op == LMINUS)
			*plv = -*plv;
		else if (op == LNOT)
			*plv = (*plv == 0);
		else if (op == LCOMP)
			*plv = ~*plv;
		else if (op != LPLUS)
			errmsg("illegal unary op in #if");
		return (p);
	}
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
