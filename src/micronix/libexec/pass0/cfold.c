/*
 * The registries and the constant folder, split from norm.c: the
 * walker's chokepoint (pull) decides when a fold may start and
 * calls dofold; everything the fold needs to parse, price, and
 * replay lives here.  parseConst rides the same climber for #if.
 */
#include "cpp.h"
#include "lexeme.h"

#define CFSV_MAX 40	/* SZQ_MAX >= CFSV_MAX + 1: a bail replays all.
			 * 21 held the three-field pack expression the
			 * rules table is made of - nineteen tokens - with
			 * a little to grow, and 16 before that silently
			 * unfolded all 455 of them.  The awk port's objmk
			 * is 37 tokens, longer than any expression the old
			 * limits were sized for; at 21 its left terms were
			 * replayed unfolded and only the rightmost folded,
			 * so a long global built by objmk came out two
			 * bytes of zero. */

/* norm.c: the walker's replay queue, drained at the chokepoint */
extern struct token szq[];
extern unsigned char szqr, szqw;

/*
 * Sizes (stage 2 of the c0 migration): registries fed by the walks
 * this file already does, and sizeof answered on the spot.
 *
 * Aggregate layouts are packed and byte-aligned, arrays multiply,
 * unions take the max - pass1's typesize arithmetic, computed here
 * so a sizeof folds to a number before pass1 exists.  Anything the
 * registries cannot price goes downstream unfolded, where pass1
 * still answers it; the tree's .s output is byte-identical either
 * way, which is the gate this rides.
 *
 * By the time the walker sees a token, typedefs are dissolved: a
 * type is its keywords, a struct tag, or a name in the registry.
 */
struct streg {
	char *tag;			/* interned */
	unsigned short size;
	struct streg *next;
};
static struct streg *stags;

struct vreg {
	char *name;			/* interned */
	unsigned short total, elem, deref;
	unsigned char depth;
	struct vreg *next;
};
static struct vreg *vregs, *vfree;
unsigned char scopedep;

unsigned short
stfind(char *tag)
{
	struct streg *s;

	for (s = stags; s; s = s->next)
		if (s->tag == tag)
			return s->size;
	return 0;
}

void
stadd(char *tag, unsigned short size)
{
	struct streg *s;

	s = (struct streg *)permalloc(sizeof(*s));
	s->tag = tag;
	s->size = size;
	s->next = stags;
	stags = s;
}

void
vadd(char *name, unsigned short total, unsigned short elem,
    unsigned short deref)
{
	struct vreg *v;

	if (vfree) {
		v = vfree;
		vfree = v->next;
	} else
		v = (struct vreg *)permalloc(sizeof(*v));
	v->name = name;
	v->total = total;
	v->elem = elem;
	v->deref = deref;
	v->depth = scopedep;
	v->next = vregs;
	vregs = v;
}

static struct vreg *
vfind(char *name)
{
	struct vreg *v;

	for (v = vregs; v; v = v->next)
		if (v->name == name)
			return v;
	return 0;
}

void
vpop(void)
{
	struct vreg *v;

	while (vregs && vregs->depth > scopedep) {
		v = vregs;
		vregs = v->next;
		v->next = vfree;
		vfree = v;
	}
}

/* one keyword's contribution to a basic type's size */
unsigned short
kwsz(unsigned char c, unsigned short base)
{
	switch (c) {
	case CHAR:	return 1;
	case SHORT:	return 2;
	case INT:	return (base == 1 || base == 4) ? base : 2;
	case LONG:	return 4;
	case UNSIGNED:	return base ? base : 2;
	}
	return base;
}

int
szkw(unsigned char c)
{
	return c == CHAR || c == SHORT || c == INT || c == LONG ||
	    c == UNSIGNED;
}

/*
 * The base size the current spec_a describes: keywords, or a
 * struct/union tag in the registry.  Zero = not priceable (an enum,
 * an unknown tag, void).
 */

/*
 * Constant folding at the chokepoint (stage 3 of the migration).
 *
 * A constant beginning a subexpression - the previous token said
 * an operand starts here - is parsed as far as constants go, with
 * the same grammar core the #if evaluator uses: pass1's operator
 * table, long arithmetic.  If an operator was consumed and
 * everything reduced, one number replaces the run - INUMBER for
 * int, LNUMBER when a long operand appeared - typed the way
 * pass1's own fold would have typed it.  The moment anything
 * non-constant shows up, every consumed token replays through the
 * queue and pass1 sees the original, byte for byte.
 */
extern long capply(unsigned char op, long a, long b);
extern long cunary(unsigned char op, long a);
extern int cfprio(unsigned char t);

static struct token cfsv[CFSV_MAX];
static unsigned char cfn;
static struct token cfcur;
static unsigned char cfbad;
static unsigned char cflong;
static unsigned char cfops;
static unsigned char cfbin;	/* a real binary op folded */

static unsigned char cfdir;	/* directive mode: tokens from cur */

static int
cfstep(void)
{
	if (cfdir) {
		gettoken();
		tokcpy(&cfcur, &cur);
		return 0;
	}
	if (cfn >= CFSV_MAX) {
		cfbad = 1;
		return 1;
	}
	tokcpy(&cfsv[(int)cfn], &cfcur);
	cfn++;
	pull(&cfcur);
	return 0;
}

static long cfxp(int limit);
static long cfsizeof(void);

static long
cfpm(void)
{
	long v;
	unsigned char op;

	switch (cfcur.type) {
	case NUMBER:
	case INUMBER:
	case LNUMBER:
		v = cfcur.v.numeric;
		/* a literal past sixteen bits is long by magnitude,
		 * exactly as pass1 types it.  (Spelled without a UL
		 * suffix: this compiler's lexer knows only 'l'.) */
		if (cfcur.type == LNUMBER || (v >> 16) != 0)
			cflong = 1;
		if (cfstep())
			return 0;
		return v;
	case LPAR:
		if (cfstep())
			return 0;
		/*
		 * A type cast, "(type)expr".  The folder's own arithmetic is
		 * long-wide - capply works in long - so a widening cast only
		 * has to set cflong, and a narrowing one truncates the value
		 * here.  Before this, "(long)1 << 24" bailed out at the
		 * keyword, c0's "never fold longs" left it a tree, and a
		 * static initialiser wrote two bytes of zero where a four-byte
		 * long belonged.
		 *
		 * Only the integer keywords are folded.  A pointer or struct
		 * cast names a place, not a number: "(char *)" meets the '*'
		 * where the ')' belongs and bails, leaving pass1 to handle it
		 * exactly as before.
		 */
		if (szkw(cfcur.type)) {
			unsigned short size = 0;

			while (szkw(cfcur.type)) {
				size = kwsz(cfcur.type, size);
				if (cfstep())
					return 0;
			}
			if (cfcur.type != RPAR) {
				cfbad = 1;
				return 0;
			}
			cfstep();
			cfops++;
			v = cfpm();
			if (cfbad)
				return 0;
			/* the cast's width is the answer's, whatever came in */
			if (size > 2)
				cflong = 1;
			else if (size == 2)
				cfbin = 1;
			if (size == 1)
				v &= 0xff;
			else if (size == 2)
				v &= 0xffff;
			return v;
		}
		v = cfxp(13);
		if (cfbad)
			return 0;
		if (cfcur.type != RPAR) {
			cfbad = 1;
			return 0;
		}
		cfstep();
		cfops++;	/* parens count: (5) folds to 5 */
		return v;
	case MINUS:
	case PLUS:
	case BANG:
	case TWIDDLE:
		op = cfcur.type;
		if (cfstep())
			return 0;
		cfops++;
		return cunary(op, cfpm());
	case SYM:
	case SIZEOF_KW:
		/* in a directive an undefined name is zero, which is
		 * what #if has always meant by it; sizeof never was
		 * a directive operand */
		if (cfdir) {
			cfstep();
			return 0;
		}
		if (cfcur.type == SIZEOF_KW) {
			if (cfstep())
				return 0;
			return cfsizeof();
		}
	}
	cfbad = 1;
	return 0;
}

/*
 * sizeof, a primary of the folder's grammar.  The operand is a
 * type - keywords, a registered tag, the abstract array a
 * typedef'd array dissolves to - or a registered variable, with
 * *p and arr[i] reaching through the entry.  An unpriceable
 * operand sets cfbad and the whole expression replays for pass1.
 * The result counts as an operation: it was never a literal, and
 * pass1 types its own answer int.
 */
static long
cfsizeof(void)
{
	unsigned short ans = 0;
	unsigned short base = 0;
	unsigned char parens = 0, stars = 0, kind = 0;
	struct vreg *vr;

	if (cfcur.type == LPAR) {
		parens = 1;
		if (cfstep())
			return 0;
	}
	while (cfcur.type == STAR) {
		stars++;
		if (cfstep())
			return 0;
	}
	if (szkw(cfcur.type)) {
		kind = 1;
		while (szkw(cfcur.type)) {
			base = kwsz(cfcur.type, base);
			if (cfstep())
				return 0;
		}
	} else if (cfcur.type == STRUCT || cfcur.type == UNION) {
		kind = 1;
		if (cfstep())
			return 0;
		if (cfcur.type != SYM) {
			cfbad = 1;
			return 0;
		}
		base = stfind(cfcur.v.name);
		if (cfstep())
			return 0;
	} else if (cfcur.type == SYM && !stars) {
		if ((vr = vfind(cfcur.v.name)) == 0) {
			cfbad = 1;
			return 0;
		}
		ans = vr->total;
		if (cfstep())
			return 0;
		if (cfcur.type == LBRACK) {
			ans = vr->elem;	/* any constant index */
			if (cfstep())
				return 0;
			if (cfcur.type != NUMBER &&
			    cfcur.type != INUMBER) {
				cfbad = 1;
				return 0;
			}
			if (cfstep())
				return 0;
			if (cfcur.type != RBRACK) {
				cfbad = 1;
				return 0;
			}
			if (cfstep())
				return 0;
		}
	} else if (cfcur.type == SYM && stars == 1) {
		if ((vr = vfind(cfcur.v.name)) == 0 || !vr->deref) {
			cfbad = 1;
			return 0;
		}
		ans = vr->deref;
		if (cfstep())
			return 0;
	} else {
		cfbad = 1;
		return 0;
	}
	/* a member selection, call, or further subscript reaches past
	 * what the registry priced - replay for pass1 */
	if (cfcur.type == DOT || cfcur.type == ARROW ||
	    (!kind && (cfcur.type == LBRACK || cfcur.type == LPAR))) {
		cfbad = 1;
		return 0;
	}
	if (kind) {
		while (cfcur.type == STAR) {
			stars++;
			if (cfstep())
				return 0;
		}
		ans = stars ? 2 : base;
		/* the abstract array a typedef'd array becomes -
		 * pass1 never could read one */
		while (cfcur.type == LBRACK) {
			unsigned short cnt;

			if (cfstep())
				return 0;
			if (cfcur.type != NUMBER &&
			    cfcur.type != INUMBER) {
				cfbad = 1;
				return 0;
			}
			cnt = (unsigned short)cfcur.v.numeric;
			if (cfstep())
				return 0;
			if (cfcur.type != RBRACK) {
				cfbad = 1;
				return 0;
			}
			ans *= cnt;
			if (cfstep())
				return 0;
		}
	}
	if (parens) {
		if (cfcur.type != RPAR) {
			cfbad = 1;
			return 0;
		}
		if (cfstep())
			return 0;
	}
	if (!ans) {
		cfbad = 1;
		return 0;
	}
	cfops++;
	cfbin = 1;	/* an operation: pass1 types its answer int */
	return (long)ans;
}

static long
cfxp(int limit)
{
	long v, m;
	int p;
	unsigned char op;

	v = cfpm();
	for (;;) {
		if (cfbad)
			return 0;
		p = cfprio(cfcur.type);
		if (!p || p > limit)
			return v;
		if (cfcur.type == QUES) {
			if (cfstep())
				return 0;
			m = cfxp(13);
			if (cfbad)
				return 0;
			if (cfcur.type != COLON) {
				cfbad = 1;
				return 0;
			}
			if (cfstep())
				return 0;
			/* spelled if/else: pass2 miscompiles a
			 * long-valued ?: (garbage arm) - the first
			 * code to want one; see the pin note */
			if (v)
				cfxp(12);	/* consume, keep m */
			else
				m = cfxp(12);
			v = m;
			cfops++;
			continue;
		}
		op = cfcur.type;
		if (cfstep())
			return 0;
		v = capply(op, v, cfxp(p - 1));
		cfops++;
		cfbin = 1;
	}
}

long
parseConst(token_t stop)
{
	long v;
	struct token sv;
	unsigned char svbad, svlong, svops, svbin;

	tokcpy(&sv, &cfcur);
	svbad = cfbad;
	svlong = cflong;
	svops = cfops;
	svbin = cfbin;
	cfdir = 1;
	cfbad = 0;
	tokcpy(&cfcur, &cur);
	v = cfxp(13);
	cfdir = 0;
	cfbad = svbad;
	cflong = svlong;
	cfops = svops;
	cfbin = svbin;
	tokcpy(&cfcur, &sv);
	while (cur.type != stop && cur.type != E_O_F)
		gettoken();
	return v;
}

void
dofold(struct token *t)
{
	long v;
	int i;

	cfn = 0;
	cfbad = 0;
	cflong = 0;
	cfops = 0;
	cfbin = 0;
	incf = 1;
	tokcpy(&cfcur, t);
	v = cfxp(cflimit);
	incf = 0;
	if (cfbad || !cfops) {
		/*
		 * Nothing folded: hand back the first token and
		 * replay the rest, the follower included.
		 */
		if (cfn > 0)
			tokcpy(t, &cfsv[0]);
		for (i = 1; i < (int)cfn; i++)
			if (szqw < SZQ_MAX)
				tokcpy(&szq[szqw++], &cfsv[i]);
		if (szqw < SZQ_MAX)
			tokcpy(&szq[szqw++], &cfcur);
		return;
	}
	/*
	 * A fold with a real binary operator is typed the way
	 * pass1's fold types an operation: int, or long when a long
	 * operand appeared.  A fold of only unary signs and parens
	 * is still a literal, and a literal is typed by magnitude -
	 * pass1 gives "-1" a byte, and must keep doing so.
	 */
	toksynth(t, cflong ? LNUMBER : (cfbin ? INUMBER : NUMBER));
	t->v.numeric = v;
	/* the follower re-enters ahead of the source */
	if (szqw < SZQ_MAX)
		tokcpy(&szq[szqw++], &cfcur);
}

#ifdef DEBUG
void
sizedump(void)
{
	struct streg *sp;
	struct vreg *v;

	for (sp = stags; sp; sp = sp->next)
		fdprintf(2, "  struct %s = %d\n", sp->tag, sp->size);
	for (v = vregs; v; v = v->next)
		fdprintf(2, "  var %s = %d elem %d deref %d @%d\n",
		    v->name, v->total, v->elem, v->deref, v->depth);
}
#endif


void
reginit(void)
{
	stags = 0;
	vregs = 0;
	vfree = 0;
	scopedep = 0;
}
