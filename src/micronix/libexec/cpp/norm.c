/*
 * norm.c - unified statement normalizer
 *
 * One recursive walker replacing the filtdecl -> filtbrace -> filtctrl
 * tail of the filter pipeline.  It pulls tokens from upstream (filtknr)
 * and pushes the normalized stream straight to emit - the C call stack
 * carries what the three filters kept in hand-rolled continuations:
 * context stacks, saved tokens, redispatch states, and the malloc'd
 * copy of an outer for's increment.
 *
 * The transformations are unchanged:
 *   - local declaration initializers split into assignments (statics
 *     and arrays keep theirs inline)
 *   - unbraced control-structure bodies get synthetic braces
 *   - loops lower to labels and gotos, switch gains its break label,
 *     break/continue become gotos to the innermost matching target
 *
 * Output-byte fidelity: .x line markers derive from per-token line
 * stamps, and synthesized tokens stamp the lexer's current position -
 * so every synthesis here happens at the same stream offset as in the
 * filters it replaces (a synthetic { is emitted only after the body's
 * first token has been pulled, a loop header only after the condition's
 * closing paren, a deferred if-} only after the else-check token).
 * Buffering boundaries match for the same reason: a for's three
 * clauses are collected fully and emitted at the final paren, exactly
 * as filtctrl did.
 */
#include <stdlib.h>
#include "cpp.h"
#include "lexeme.h"

/* cpp.c: the lexer wrapper the source layer draws from */
extern void lex_get(struct token *);

/* the cooked-token source: lexer + enum lowering + typedef expansion */
void srcget(struct token *);

/* one-token pushback: the else-check and statement-end lookaheads */
static struct token backtok;
static unsigned char haveback;

static int next_label;

/*
 * break/continue targets of the innermost enclosing construct.
 * pfx 0 = no target (the keyword passes through raw).  Loops set
 * both; switch sets only the break target, so a continue inside a
 * switch still reaches the enclosing loop.  Nesting saves and
 * restores these in the recursion's locals.
 */
static char brkpfx;
static int brknum;
static char cntpfx;
static int cntnum;
static char cntsfx;

static unsigned char bdepth;	/* statement-block brace depth */
unsigned char inagg;	/* file scope: struct/union/enum head seen */

/* control-clause buffers; only a for's increment outlives a body,
 * and that one lives in do_for's frame */
static struct tokarray cond_a;
static struct tokarray init_a;

/* declaration buffers (a declaration never nests inside another) */
static struct tokarray spec_a;
static struct tokarray dini_a;

#define NAME_MAX 16
struct dname {
	char *name;
	char star_count;
};
static struct dname names[NAME_MAX];
static unsigned char name_cnt;
static unsigned char cur_stars;

#define ASSIGN_MAX 16
struct dinit {
	char *name;
	struct token *init;
	int init_len;
};
static struct dinit assigns[ASSIGN_MAX];
static unsigned char assign_cnt;

static void stmt(struct token *t);
static void exprstmt(struct token *t);
static void aggprime(void);
static void mpush(char *tag, unsigned char isu);
static void mtok(struct token *t);
static void arrstart(char *name, unsigned char stars);
static void arrtok(struct token *t);
static void arrdone(void);

extern int cfprio(unsigned char t);

/*
 * Token I/O
 */
struct token szq[SZQ_MAX];
unsigned char szqr, szqw;
static unsigned char cfprev;	/* last token type delivered by pull */
unsigned char cflimit;	/* how much may fold here: after a
				 * binary op only tighter operators
				 * may - folding "3 + 2" inside
				 * "x - 3 + 2" would reassociate */
unsigned char incf;

static int
cfoldable(unsigned char t)
{
	return t == NUMBER || t == INUMBER || t == LNUMBER ||
	    t == LPAR || t == MINUS || t == PLUS || t == TWIDDLE ||
	    t == BANG || t == SIZEOF_KW;
}

/* is a token type one after which an operand may begin? */
static int
atoperand(unsigned char p)
{
	if (p == SYM || p == RPAR || p == RBRACK || p == NUMBER ||
	    p == INUMBER || p == LNUMBER || p == STRING ||
	    p == INCR || p == DECR)
		return 0;
	return 1;
}


void
pull(struct token *t)
{
	if (haveback) {
		haveback = 0;
		tokcpy(t, &backtok);
		goto track;
	}
	if (szqr < szqw) {
		/* replaying: no re-interception, but the context
		 * still advances - a stale one once turned a binary
		 * plus into a unary one */
		tokcpy(t, &szq[szqr++]);
		if (szqr == szqw)
			szqr = szqw = 0;
		goto track;
	}
	srcget(t);
	if (!incf && t->type != NEWLINE && t->type != LINENO) {
		/*
		 * The paren after if/while/for/switch is structure the
		 * walker parses, not an expression's - folding
		 * "while (1)" to "while 1" dismantled the loop.
		 */
		if (cfoldable(t->type) && atoperand(cfprev) &&
		    !(t->type == LPAR && (cfprev == IF ||
		    cfprev == WHILE || cfprev == FOR ||
		    cfprev == SWITCH)))
			dofold(t);
	}
track:
	if (!incf && t->type != NEWLINE && t->type != LINENO) {
		cfprev = t->type;
		if (t->type == QUES || t->type == COLON)
			cflimit = 12;
		else {
			int p = cfprio(t->type);

			cflimit = p ? p - 1 : 13;
		}
	}
}

void
pushb(struct token *t)
{
	tokcpy(&backtok, t);
	haveback = 1;
}

void
out(struct token *t)
{
	emitStructTok(t);
}

/* synthesize at the lexer's current position - stamp parity with
 * the old filters requires calling this at matching stream offsets */
void
outt(unsigned char type)
{
	struct token t;

	toksynth(&t, type);
	out(&t);
}

static void
outat(unsigned char type, struct token *ref)
{
	struct token t;

	t.type = type;
	t.v.numeric = 0;
	t.lineno = ref->lineno;
	t.filename = ref->filename;
	out(&t);
}

void
outarr(struct tokarray *a)
{
	int i;

	for (i = 0; i < a->count; i++)
		out(&a->buf[i]);
}

/* __XnS: - label plus the semicolon that makes it a statement */
static void
outlab(char pfx, int num, char sfx)
{
	struct token t;
	char buf[16];

	fmtstr(buf, "__%c%d%c", pfx, num, sfx);
	toksynthnam(&t, LABEL, intern(buf));
	out(&t);
	outt(SEMI);
}

static void
outgoto(char pfx, int num, char sfx)
{
	struct token t;
	char buf[16];

	outt(GOTO);
	fmtstr(buf, "__%c%d%c", pfx, num, sfx);
	toksynthnam(&t, SYM, intern(buf));
	out(&t);
}

/*
 * Declaration splitting (from filtdecl)
 */
static int
specs_static(void)
{
	register struct token *tp = spec_a.buf;
	int n = spec_a.count;

	while (n--) {
		if (tp->type == STATIC)
			return 1;
		tp++;
	}
	return 0;
}

static struct dinit *
init_for(char *name)
{
	register struct dinit *ap = assigns;
	unsigned char n = assign_cnt + 1;

	while (--n) {
		if (ap->name == name)
			return ap;
		ap++;
	}
	return 0;
}

/* type SYM [, SYM]* ; - initializers stay inline only for statics */
static void
emit_decl(void)
{
	register struct dname *np;
	unsigned char n;
	struct dinit *ap;
	int j;
	int keep = specs_static();
	struct token tmp;
	struct token *ref = &spec_a.buf[0];

	outarr(&spec_a);

	np = names;
	n = name_cnt + 1;
	while (--n) {
		for (j = np->star_count; j > 0; j--)
			outat(STAR, ref);
		tmp.type = SYM;
		tmp.v.name = np->name;
		tmp.lineno = ref->lineno;
		tmp.filename = ref->filename;
		out(&tmp);
		if (keep && (ap = init_for(np->name))) {
			outat(ASSIGN, ref);
			for (j = 0; j < ap->init_len; j++)
				out(&ap->init[j]);
		}
		if (n > 1)
			outat(COMMA, ref);
		np++;
	}
	outat(SEMI, ref);
}

/* name = init ; for each captured initializer */
static void
emit_assigns(void)
{
	register struct dinit *ap;
	unsigned char n;
	int j;
	struct token tmp;
	struct token *ref;

	ap = assigns;
	n = assign_cnt + 1;
	while (--n) {
		ref = ap->init_len > 0 ? &ap->init[0] : &spec_a.buf[0];
		tmp.type = SYM;
		tmp.v.name = ap->name;
		tmp.lineno = ref->lineno;
		tmp.filename = ref->filename;
		out(&tmp);
		outat(ASSIGN, ref);
		for (j = 0; j < ap->init_len; j++)
			out(&ap->init[j]);
		outat(SEMI, ref);
		free(ap->init);
		ap++;
	}
	assign_cnt = 0;
}

static void
drop_assigns(void)
{
	register struct dinit *ap = assigns;
	unsigned char n = assign_cnt + 1;

	while (--n)
		free((ap++)->init);
	assign_cnt = 0;
}

static void
save_init(char *name)
{
	if (assign_cnt < ASSIGN_MAX && dini_a.count > 0) {
		register struct token *d;
		struct token *s;
		struct dinit *ap;
		int n;

		n = dini_a.count;
		ap = &assigns[assign_cnt++];
		ap->name = name;
		d = ap->init = (struct token *)xalloc(n * sizeof(struct token));
		s = dini_a.buf;
		while (n--)
			tokcpy(d++, s++);
		ap->init_len = dini_a.count;
	}
	tarr_reset(&dini_a);
}

static void
save_name(char *name)
{
	if (name_cnt < NAME_MAX) {
		register struct dname *np = &names[name_cnt++];

		np->name = name;
		np->star_count = cur_stars;
	}
	cur_stars = 0;
}


static unsigned char spec_isagg;

static unsigned short
specbase(void)
{
	unsigned short base = 0;
	int i;

	spec_isagg = 0;
	for (i = 0; i < spec_a.count; i++) {
		unsigned char c = spec_a.buf[i].type;

		if (c == STRUCT || c == UNION) {
			spec_isagg = 1;
			if (i + 1 < spec_a.count &&
			    spec_a.buf[i+1].type == SYM)
				return stfind(spec_a.buf[i+1].v.name);
			return 0;
		}
		if (c == ENUM)
			return 0;
		if (szkw(c))
			base = kwsz(c, base);
	}
	return base;
}

/* the scalar/pointer declarators finish_decl is about to emit */
static void
sizedecl(void)
{
	unsigned short base = specbase();
	unsigned char i;

	for (i = 0; i < name_cnt; i++) {
		unsigned char st = names[i].star_count;

		if (st)
			vadd(names[i].name, 2, 0,
			    st > 1 ? 2 : base);
		else if (base && spec_isagg)
			vadd(names[i].name, base, 0, 0);
	}
}

/*
 * An array declarator: decl() streams the brackets, this collects
 * the verdict.  Multiplied counts, one bracket group for an
 * indexable element size, anything unpriceable drops the entry.
 */
static char *arr_name;
static unsigned char arr_stars;
static unsigned short arr_base;
static unsigned short arr_cnt;
static unsigned char arr_nbrk;
static unsigned char arr_bad;

static void
arrstart(char *name, unsigned char stars)
{
	arr_name = name;
	arr_stars = stars;
	arr_base = specbase();
	arr_cnt = 0;
	arr_nbrk = 0;
	arr_bad = 0;
}

static void
arrtok(struct token *t)
{
	if (!arr_name)
		return;
	if (t->type == NUMBER || t->type == INUMBER) {
		unsigned short v = (unsigned short)t->v.numeric;

		arr_cnt = arr_cnt ? arr_cnt * v : v;
	} else if (t->type == RBRACK)
		arr_nbrk++;
	else if (t->type != LBRACK)
		arr_bad = 1;	/* an expression: not priced */
}

static void
arrdone(void)
{
	unsigned short el;

	if (arr_name && !arr_bad && arr_cnt) {
		el = arr_stars ? 2 : arr_base;
		if (el)
			vadd(arr_name, el * arr_cnt,
			    arr_nbrk == 1 ? el : 0, 0);
	}
	arr_name = 0;
}

/*
 * Member pricing, fed a token at a time from aggrpass.  A small
 * frame per nested body; the same declarator walk as everywhere,
 * shrunk to what members can be.
 */
struct mframe {
	char *tag;
	unsigned short off;
	unsigned short base;
	unsigned short arr;
	unsigned char isunion;
	unsigned char ptr;
	unsigned char nbrk;
	unsigned char pd;
	unsigned char inbrk;
	unsigned char bad;
};
#define MAXMFR 4
static struct mframe mfr[MAXMFR];
static unsigned char mfeed;	/* aggrpass feeds the pricer */
static char mtop = -1;
static unsigned char msawtag;
static char *mtag;
static unsigned char mkind;

static void
mmember(struct mframe *m)
{
	unsigned short sz;

	sz = m->ptr ? 2 : m->base;
	if (m->arr)
		sz *= m->arr;
	if (!sz)
		m->bad = 1;
	else if (m->isunion) {
		if (sz > m->off)
			m->off = sz;
	} else
		m->off += sz;
	m->ptr = 0;
	m->arr = 0;
	m->nbrk = 0;
}

static void
mpush(char *tag, unsigned char isu)
{
	struct mframe *m;

	if (mtop + 1 >= MAXMFR) {
		mfr[MAXMFR - 1].bad = 1;
		return;
	}
	m = &mfr[(int)++mtop];
	m->tag = tag;
	m->off = 0;
	m->base = 0;
	m->arr = 0;
	m->isunion = isu;
	m->ptr = 0;
	m->nbrk = 0;
	m->pd = 0;
	m->inbrk = 0;
	m->bad = 0;
}

static void
mpop(void)
{
	struct mframe *m = &mfr[(int)mtop];

	/* an anonymous body has no tag to register under, which is why
	 * mpush takes a null one; the size is still the enclosing
	 * member's, and mpop below hands it up */
	if (!m->bad && m->tag)
		stadd(m->tag, m->off);
	mtop--;
	if (mtop >= 0) {
		/* "} name;" continues as the enclosing member */
		if (m->bad)
			mfr[(int)mtop].bad = 1;
		mfr[(int)mtop].base = m->off;
	}
}

static void
mtok(struct token *t)
{
	struct mframe *m;

	if (t->type == NEWLINE || t->type == LINENO)
		return;
	if (msawtag) {
		msawtag = 0;
		if (t->type == SYM) {
			mtag = t->v.name;
			return;
		}
		mtag = 0;
		/* anonymous body: the kind survives to the BEGIN */
	}
	if (t->type == STRUCT || t->type == UNION) {
		msawtag = 1;
		mkind = (t->type == UNION) ? 2 : 1;
		return;
	}
	if (mtop < 0)
		return;
	m = &mfr[(int)mtop];
	if (mkind && t->type == BEGIN) {
		mpush(mtag, mkind == 2);
		mtag = 0;
		mkind = 0;
		return;
	}
	if (mtag) {
		/* "struct x" by reference; a pointer member never
		 * needs the base, mmember flags the ones that do */
		m->base = stfind(mtag);
		mtag = 0;
		mkind = 0;
	}
	if (m->inbrk) {
		if (t->type == NUMBER || t->type == INUMBER)
			m->arr = m->arr ? m->arr *
			    (unsigned short)t->v.numeric
			    : (unsigned short)t->v.numeric;
		else if (t->type == RBRACK) {
			if (!m->arr)
				m->bad = 1;
			m->nbrk++;
			m->inbrk = 0;
		} else
			m->bad = 1;
		return;
	}
	switch (t->type) {
	case STAR:	if (!m->pd) m->ptr++; break;
	case LPAR:	m->pd++; break;
	case RPAR:	if (m->pd) m->pd--; break;
	case LBRACK:	if (!m->pd) m->inbrk = 1; break;
	case COMMA:	if (!m->pd) mmember(m); break;
	case SEMI:	mmember(m);
			m->base = 0;
			break;
	case BEGIN:	mpush(0, 0); break;	/* anonymous body */
	case END:	mpop(); break;
	default:
		if (szkw(t->type))
			m->base = kwsz(t->type, m->base);
		break;
	}
}

/*
 * sizeof, answered from the registries.  On entry the SIZEOF_KW has
 * been pulled; the walk consumes the operand.  What folds becomes
 * an INUMBER - typed int like the construct it replaces, not by
 * magnitude - and what does not is replayed for pass1 through a
 * small queue ahead of the source.
 */
/*
 * File-scope declarations, watched off kout's stream.  One flat
 * walk: specs, stars, a name, brackets, an initializer to skip.
 * Function declarators drop the name (nothing may sizeof one);
 * a struct body does not pass this way at all - norm_run routes
 * it through aggrpass - so a tag reference resolves through the
 * registry by the time the following declarators arrive.
 */
#define KS_SPECS	0
#define KS_DECL		1
#define KS_BRK		2
#define KS_INIT		3
static unsigned char ks_st;
static unsigned short ks_base;
static unsigned char ks_stars;
static char *ks_name;
static unsigned short ks_cnt;
static unsigned char ks_nbrk;
static unsigned char ks_bad;
static unsigned char ks_pd;
static unsigned char ks_id;
static unsigned char ks_awtag;
static unsigned char ks_isagg;
static char *ks_tag;		/* aggregate head, for norm_run */
static unsigned char ks_kind;

static void
ksdone(void)
{
	unsigned short el;

	if (ks_name && !ks_bad) {
		if (ks_cnt) {
			el = ks_stars ? 2 : ks_base;
			if (el)
				vadd(ks_name, el * ks_cnt,
				    ks_nbrk == 1 ? el : 0, 0);
		} else if (ks_stars)
			vadd(ks_name, 2, 0,
			    ks_stars > 1 ? 2 : ks_base);
		else if (ks_base && ks_isagg)
			vadd(ks_name, ks_base, 0, 0);
	}
	ks_name = 0;
	ks_stars = 0;
	ks_cnt = 0;
	ks_nbrk = 0;
	ks_bad = 0;
	ks_isagg = 0;
}

void
kscan(struct token *t)
{
	if (t->type == NEWLINE || t->type == LINENO)
		return;
	if (ks_awtag) {
		ks_awtag = 0;
		if (t->type == SYM) {
			ks_tag = t->v.name;
			ks_base = stfind(ks_tag);
			ks_isagg = 1;
			return;
		}
		/*
		 * An anonymous body, and it has no tag to be registered
		 * under.  This kept the last one seen, so
		 *
		 *	struct big { char x[100]; };
		 *	static struct { char y[6]; } anchor;
		 *
		 * priced the anonymous body and filed its size under
		 * "big" - and stadd pushes while stfind takes the first
		 * match, so the wrong answer shadowed the right one and
		 * sizeof(struct big) became 6.  mtok has always cleared
		 * its copy here; this one never did.
		 */
		ks_tag = 0;
	}
	if (t->type == STRUCT || t->type == UNION) {
		ks_awtag = 1;
		ks_kind = (t->type == UNION) ? 2 : 1;
		ks_base = 0;
		ks_isagg = 0;
		if (ks_st == KS_INIT)
			return;
		ks_st = KS_SPECS;
		return;
	}
	if (t->type == ENUM) {
		ks_kind = 0;
		ks_base = 0;
		ks_isagg = 0;
		ks_bad = 1;
		return;
	}
	switch (ks_st) {
	case KS_SPECS:
		if (szkw(t->type)) {
			ks_base = kwsz(t->type, ks_base);
			return;
		}
		if (t->type == STAR) {
			ks_stars++;
			ks_st = KS_DECL;
			return;
		}
		if (t->type == SYM) {
			ks_name = t->v.name;
			ks_st = KS_DECL;
			return;
		}
		if (t->type == SEMI) {
			ks_base = 0;
			ks_bad = 0;
		}
		return;
	case KS_DECL:
		switch (t->type) {
		case STAR:	if (!ks_pd && !ks_name) ks_stars++;
				break;
		case SYM:	if (!ks_pd && !ks_name)
					ks_name = t->v.name;
				break;
		case LPAR:	if (!ks_pd && ks_name)
					ks_name = 0;  /* a function */
				ks_pd++;
				break;
		case RPAR:	if (ks_pd) ks_pd--; break;
		case LBRACK:	if (!ks_pd) ks_st = KS_BRK; break;
		case ASSIGN:	if (ks_pd) break;
				ksdone();
				ks_st = KS_INIT;
				ks_id = 0;
				break;
		case COMMA:	if (!ks_pd) ksdone(); break;
		case SEMI:	ksdone();
				ks_base = 0;
				ks_st = KS_SPECS;
				break;
		}
		return;
	case KS_BRK:
		if (t->type == NUMBER || t->type == INUMBER)
			ks_cnt = ks_cnt ? ks_cnt *
			    (unsigned short)t->v.numeric
			    : (unsigned short)t->v.numeric;
		else if (t->type == RBRACK) {
			if (!ks_cnt)
				ks_bad = 1;
			ks_nbrk++;
			ks_st = KS_DECL;
		} else
			ks_bad = 1;
		return;
	case KS_INIT:
		switch (t->type) {
		case BEGIN:
		case LPAR:
		case LBRACK:	ks_id++; break;
		case END:
		case RPAR:
		case RBRACK:	if (ks_id) ks_id--; break;
		case COMMA:	if (!ks_id) ks_st = KS_DECL; break;
		case SEMI:	if (!ks_id) {
					ks_base = 0;
					ks_st = KS_SPECS;
				}
				break;
		}
		return;
	}
}

static void sizedecl(void);

static void
finish_decl(void)
{
	sizedecl();
	if (name_cnt > 0) {
		int keep = specs_static();

		emit_decl();
		if (keep)
			drop_assigns();
		else
			emit_assigns();
	}
	tarr_reset(&spec_a);
	name_cnt = 0;
	cur_stars = 0;
}

/*
 * Stream a brace-balanced region verbatim: the body of a struct or
 * union defined mid-function or at file scope.  Members are not
 * declarations to split - a bitfield's colon would tear one apart.
 * The BEGIN has been emitted; consumes through the matching END.
 */
static void
aggrpass(void)
{
	struct token t;
	int d = 1;

	for (;;) {
		pull(&t);
		if (t.type == E_O_F)
			return;
		if (t.type == BEGIN)
			d++;
		else if (t.type == END && --d == 0) {
			if (mfeed) {
				mtok(&t);
				mfeed = 0;
			}
			out(&t);
			return;
		}
		if (mfeed)
			mtok(&t);
		out(&t);
	}
}

/*
 * Prime the member pricer from spec_a: the body about to stream is
 * the definition of this tag.  Enums have no members to price.
 */
static void
aggprime(void)
{
	char *tag = 0;
	unsigned char isu = 0;
	int i;

	for (i = 0; i < spec_a.count; i++) {
		unsigned char c = spec_a.buf[i].type;

		if (c == ENUM)
			return;
		if (c == UNION)
			isu = 1;
		if ((c == STRUCT || c == UNION) &&
		    i + 1 < spec_a.count &&
		    spec_a.buf[i+1].type == SYM)
			tag = spec_a.buf[i+1].v.name;
	}
	mpush(tag, isu);
	mfeed = 1;
}

/* decl() states */
#define D_DECL	0	/* collecting specs, stars, names */
#define D_NAME	1	/* after a declarator name */
#define D_INIT	2	/* collecting a scalar initializer */
#define D_ARR	3	/* inside an array declarator's brackets */
#define D_ARRE	4	/* just past the closing bracket */
#define D_AINIT	5	/* an array's inline initializer */

/*
 * One declaration engagement, entered on a type token wherever the
 * old filtdecl would have left ST_NORMAL - a statement head or the
 * middle of an expression (a cast's type flows through the same
 * flush).  Returns 1 when the declaration consumed its semicolon
 * (the statement is over), 0 when it flushed mid-stream and the
 * caller should keep going.
 */
int
decl(struct token *t)
{
	struct token cur;
	int st = D_DECL;
	int xtag;		/* struct/union/enum: next SYM is the tag */
	int pdepth = 0;		/* paren depth in initializers */
	int idepth = 0;		/* brace depth in an array initializer */
	int arrd = 0;		/* bracket depth in an array declarator */
	int i;

	tarr_reset(&spec_a);
	name_cnt = 0;
	assign_cnt = 0;
	cur_stars = 0;
	xtag = (t->type == STRUCT || t->type == UNION || t->type == ENUM);
	tarr_push(&spec_a, t);

	for (;;) {
		pull(&cur);
		if (cur.type == E_O_F) {
			outarr(&spec_a);
			tarr_reset(&spec_a);
			return 1;
		}

		if (cur.type == BEGIN) {
			if (st == D_AINIT) {
				idepth++;
				out(&cur);
				continue;
			}
			if (st == D_DECL || st == D_NAME) {
				/*
				 * Aggregate definition in declaration
				 * position: flush the specs and stream
				 * the body verbatim.
				 */
				register struct token *tp = spec_a.buf;
				int n = spec_a.count;
				int isagg = 0;

				while (n--) {
					if (tp->type == STRUCT ||
					    tp->type == UNION ||
					    tp->type == ENUM) {
						isagg = 1;
						break;
					}
					tp++;
				}
				if (isagg)
					aggprime();
				outarr(&spec_a);
				out(&cur);
				tarr_reset(&spec_a);
				name_cnt = 0;
				cur_stars = 0;
				if (isagg)
					aggrpass();
				return 0;
			}
			/* D_INIT and the rest: straight out (mirrors the
			 * old intercept; a brace here is broken input) */
			out(&cur);
			continue;
		}
		if (cur.type == END) {
			if (st == D_AINIT) {
				idepth--;
				out(&cur);
				continue;
			}
			out(&cur);
			continue;
		}

		switch (st) {
		case D_DECL:
			if (is_type_tok(&cur)) {
				xtag = (cur.type == STRUCT ||
				    cur.type == UNION || cur.type == ENUM);
				tarr_push(&spec_a, &cur);
				continue;
			}
			if (cur.type == STAR) {
				cur_stars++;
				xtag = 0;
				continue;
			}
			if (cur.type == SYM) {
				if (xtag) {
					tarr_push(&spec_a, &cur);
					xtag = 0;
					continue;
				}
				save_name(cur.v.name);
				st = D_NAME;
				continue;
			}
			/* not a declaration: flush specs, stars, token */
			outarr(&spec_a);
			for (i = 0; i < cur_stars; i++)
				outt(STAR);
			out(&cur);
			tarr_reset(&spec_a);
			cur_stars = 0;
			return 0;

		case D_NAME:
			if (cur.type == ASSIGN) {
				st = D_INIT;
				pdepth = 0;
				idepth = 0;
				tarr_reset(&dini_a);
				continue;
			}
			if (cur.type == COMMA) {
				st = D_DECL;
				continue;
			}
			if (cur.type == SEMI) {
				/* the declaration's own ; comes from
				 * emit_decl; the source's is dropped */
				finish_decl();
				return 1;
			}
			if (cur.type == STAR) {
				out(&cur);
				continue;
			}
			if (cur.type == LPAR || cur.type == LBRACK) {
				/*
				 * Function or array declarator: emit
				 * type, stars, name, opener; a function's
				 * tail flows through unchanged, an
				 * array's dimension is streamed and the
				 * declaration continues after it.
				 */
				struct token tmp;
				struct dname *np = &names[name_cnt - 1];
				struct token *ref = spec_a.count > 0 ?
				    &spec_a.buf[0] : &cur;

				/*
				 * The declarators before this one are a
				 * declaration in their own right, and have to
				 * be emitted before it.
				 *
				 * They used to be dropped on the floor.  A
				 * function declarator emits its own name here
				 * and then leaves the walk - the rest of the
				 * line flows through unchanged, which is why
				 * the names AFTER it survived - and nothing
				 * emitted what had been collected in names[]
				 * ahead of it.  So
				 *
				 *	char *file, *s_getmsg(), msg[80];
				 *
				 * reached pass1 as if "file" had never been
				 * written, and its use eight lines later was
				 * reported as an unknown name being called.
				 * Declaring the function that returns a
				 * pointer in among the variables that will
				 * hold its result is an ordinary K&R habit -
				 * there is nowhere else to put it before
				 * prototypes - so old sources are full of it.
				 *
				 * name_cnt is dropped first so the function's
				 * own name is not emitted twice: it is the
				 * last one collected, and it is written out
				 * below with its parenthesis.
				 *
				 * Only for a function.  An array declarator
				 * stays inside the walk - "the type is kept"
				 * below - so the names before it are still
				 * pending and finish_decl emits them with the
				 * ones after, which is right and is left
				 * alone.
				 */
				name_cnt--;
				if (cur.type == LPAR && name_cnt > 0) {
					int keep;

					sizedecl();
					keep = specs_static();
					emit_decl();
					if (keep)
						drop_assigns();
					else
						emit_assigns();
					name_cnt = 0;
				}

				outarr(&spec_a);
				for (i = np->star_count; i > 0; i--)
					outat(STAR, ref);
				tmp.type = SYM;
				tmp.v.name = np->name;
				tmp.lineno = ref->lineno;
				tmp.filename = ref->filename;
				out(&tmp);
				out(&cur);
				if (cur.type == LPAR) {
					tarr_reset(&spec_a);
					return 0;
				}
				/* the type is kept: declarators after
				 * the array still split */
				arrstart(np->name, np->star_count);
				arrd = 1;
				st = D_ARR;
				continue;
			}
			finish_decl();
			out(&cur);
			return 0;

		case D_ARR:
			out(&cur);
			arrtok(&cur);
			if (cur.type == LBRACK)
				arrd++;
			else if (cur.type == RBRACK && --arrd == 0)
				st = D_ARRE;
			continue;

		case D_ARRE:
			if (cur.type == LBRACK) {
				out(&cur);
				arrd = 1;
				st = D_ARR;
				continue;
			}
			if (cur.type == ASSIGN) {
				/*
				 * An array initializer stays inline: an
				 * aggregate cannot become an assignment,
				 * and its only legal homes want it
				 * inline anyway.
				 */
				arrdone();
				out(&cur);
				pdepth = 0;
				idepth = 0;
				st = D_AINIT;
				continue;
			}
			if (cur.type == COMMA) {
				/* close this declaration, keep the type:
				 * char buf[12], *p  ->  two declarations */
				arrdone();
				outt(SEMI);
				st = D_DECL;
				continue;
			}
			if (cur.type == SEMI) {
				arrdone();
				outt(SEMI);
				finish_decl();
				return 1;
			}
			/* not a shape this splits: flush and step aside */
			arr_name = 0;
			out(&cur);
			tarr_reset(&spec_a);
			name_cnt = 0;
			return 0;

		case D_AINIT:
			if (cur.type == LPAR)
				pdepth++;
			else if (cur.type == RPAR)
				pdepth--;
			if (cur.type == COMMA && idepth == 0 && pdepth == 0) {
				outt(SEMI);
				st = D_DECL;
				continue;
			}
			if (cur.type == SEMI && idepth == 0 && pdepth == 0) {
				outt(SEMI);
				finish_decl();
				return 1;
			}
			out(&cur);
			continue;

		case D_INIT:
			if (cur.type == LPAR)
				pdepth++;
			else if (cur.type == RPAR)
				pdepth--;
			if (cur.type == COMMA && pdepth == 0 && idepth == 0) {
				save_init(names[name_cnt - 1].name);
				st = D_DECL;
				continue;
			}
			if (cur.type == SEMI && pdepth == 0 && idepth == 0) {
				save_init(names[name_cnt - 1].name);
				finish_decl();
				return 1;
			}
			tarr_push(&dini_a, &cur);
			continue;
		}
	}
}

/*
 * Control lowering (from filtbrace + filtctrl)
 */

/* stream a ( cond ) verbatim - if and switch keep their conditions */
static void
copycond(void)
{
	struct token t;
	int d = 0;

	for (;;) {
		pull(&t);
		if (t.type == E_O_F)
			return;
		if (t.type == LPAR)
			d++;
		else if (t.type == RPAR)
			d--;
		out(&t);
		if (d <= 0)
			return;
	}
}

/*
 * Collect the tokens inside ( ... ) into a buffer, outer parens
 * dropped.  Nothing is emitted: the caller re-emits the buffer inside
 * the construct it is lowering, which is why stamps ride along on the
 * buffered tokens themselves.
 */
static void
grabcond(void)
{
	struct token t;
	int d = 0;

	tarr_reset(&cond_a);
	for (;;) {
		pull(&t);
		if (t.type == E_O_F)
			return;
		if (t.type == LPAR) {
			d++;
			if (d == 1)
				continue;
		} else if (t.type == RPAR) {
			d--;
			if (d == 0)
				return;
		}
		if (d > 0)
			tarr_push(&cond_a, &t);
	}
}

/* if ( ! ( cond ) ) { goto __XnB ; } - the loop's entry test */
static void
outcondjmp(char pfx, int n)
{
	outt(IF);
	outt(LPAR);
	outt(BANG);
	outt(LPAR);
	outarr(&cond_a);
	outt(RPAR);
	outt(RPAR);
	outt(BEGIN);
	outgoto(pfx, n, 'B');
	outt(SEMI);
	outt(END);
}

/*
 * A loop or switch body: brace-normalized, one statement or a block.
 * The synthetic { is emitted only after the first body token is in
 * hand (stamp parity), the } right after the statement completes.
 */
static void
body(void)
{
	struct token t;

	pull(&t);
	if (t.type == E_O_F)
		return;
	scopedep++;
	if (t.type == BEGIN) {
		stmt(&t);
	} else {
		outt(BEGIN);
		stmt(&t);
		outt(END);
	}
	scopedep--;
	vpop();
}

static void do_else(void);

/*
 * if: condition passes through; an unbraced body gets braces, but its
 * closing } waits until the token after the body shows whether an
 * else follows - the } must precede the else either way, and it is
 * synthesized only after that token is pulled.
 */
static void
do_if(struct token *t0)
{
	struct token t;

	out(t0);
	copycond();
	pull(&t);
	if (t.type == E_O_F)
		return;
	if (t.type == BEGIN) {
		stmt(&t);
		pull(&t);
		if (t.type == ELSE) {
			out(&t);
			do_else();
			return;
		}
		if (t.type != E_O_F)
			pushb(&t);
		return;
	}
	outt(BEGIN);
	stmt(&t);
	pull(&t);
	if (t.type == ELSE) {
		outt(END);
		out(&t);
		do_else();
		return;
	}
	outt(END);
	if (t.type != E_O_F)
		pushb(&t);
}

/* else body; else-if chains continue without an extra brace level */
static void
do_else(void)
{
	struct token t;

	pull(&t);
	if (t.type == E_O_F)
		return;
	if (t.type == IF) {
		do_if(&t);
		return;
	}
	if (t.type == BEGIN) {
		stmt(&t);
		return;
	}
	outt(BEGIN);
	stmt(&t);
	outt(END);
}

/*
 * while (cond) body ->
 *	__WnT: ; if (!(cond)) { goto __WnB ; } body goto __WnT ; __WnB: ;
 */
static void
do_while(void)
{
	int n = next_label++;
	char obp = brkpfx, ocp = cntpfx, ocs = cntsfx;
	int obn = brknum, ocn = cntnum;

	grabcond();
	outlab('W', n, 'T');
	if (cond_a.count)
		outcondjmp('W', n);

	brkpfx = 'W'; brknum = n;
	cntpfx = 'W'; cntnum = n; cntsfx = 'T';
	body();
	brkpfx = obp; brknum = obn;
	cntpfx = ocp; cntnum = ocn; cntsfx = ocs;

	outgoto('W', n, 'T');
	outt(SEMI);
	outlab('W', n, 'B');
}

/*
 * for (init; cond; incr) body ->
 *	init ; __FnT: ; if (!(cond)) { goto __FnB ; } body
 *	__FnC: ; incr ; goto __FnT ; __FnB: ;
 * All three clauses are collected before anything is emitted, as
 * filtctrl did; only the increment outlives the body, in this frame.
 */
static void
do_for(void)
{
	int n = next_label++;
	struct token t;
	int d = 0;
	struct tokarray incr;
	char obp = brkpfx, ocp = cntpfx, ocs = cntsfx;
	int obn = brknum, ocn = cntnum;

	/* init: up to the ; at paren depth 1 */
	tarr_reset(&init_a);
	for (;;) {
		pull(&t);
		if (t.type == E_O_F)
			return;
		if (t.type == LPAR) {
			d++;
			if (d == 1)
				continue;
		} else if (t.type == RPAR) {
			d--;
		} else if (t.type == SEMI && d == 1) {
			break;
		}
		if (d > 0)
			tarr_push(&init_a, &t);
	}

	/* cond: up to the next ; */
	tarr_reset(&cond_a);
	for (;;) {
		pull(&t);
		if (t.type == E_O_F)
			return;
		if (t.type == SEMI)
			break;
		tarr_push(&cond_a, &t);
	}

	/* incr: up to the ) that closes the header */
	tarr_init(&incr, 16);
	for (;;) {
		pull(&t);
		if (t.type == E_O_F) {
			free(incr.buf);
			return;
		}
		if (t.type == RPAR) {
			if (--d == 0)
				break;
		} else if (t.type == LPAR) {
			d++;
		}
		tarr_push(&incr, &t);
	}

	if (init_a.count) {
		outarr(&init_a);
		outt(SEMI);
	}
	outlab('F', n, 'T');
	if (cond_a.count)
		outcondjmp('F', n);

	brkpfx = 'F'; brknum = n;
	cntpfx = 'F'; cntnum = n; cntsfx = 'C';
	body();
	brkpfx = obp; brknum = obn;
	cntpfx = ocp; cntnum = ocn; cntsfx = ocs;

	outlab('F', n, 'C');
	if (incr.count) {
		outarr(&incr);
		outt(SEMI);
	}
	outgoto('F', n, 'T');
	outt(SEMI);
	outlab('F', n, 'B');
	free(incr.buf);
}

/*
 * do body while (cond); ->
 *	__DnT: ; body __DnC: ; if (cond) { goto __DnT ; } __DnB: ;
 * __DnC precedes the test so continue re-tests it (C semantics).
 */
static void
do_do(void)
{
	int n = next_label++;
	struct token t;
	char obp = brkpfx, ocp = cntpfx, ocs = cntsfx;
	int obn = brknum, ocn = cntnum;

	outlab('D', n, 'T');

	brkpfx = 'D'; brknum = n;
	cntpfx = 'D'; cntnum = n; cntsfx = 'C';
	body();
	brkpfx = obp; brknum = obn;
	cntpfx = ocp; cntnum = ocn; cntsfx = ocs;

	pull(&t);
	if (t.type == E_O_F)
		return;
	if (t.type != WHILE) {
		/* not the trailing while: close the loop and step aside */
		pushb(&t);
		outlab('D', n, 'C');
		outlab('D', n, 'B');
		return;
	}
	grabcond();
	outlab('D', n, 'C');
	if (cond_a.count) {
		outt(IF);
		outt(LPAR);
		outarr(&cond_a);
		outt(RPAR);
		outt(BEGIN);
		outgoto('D', n, 'T');
		outt(SEMI);
		outt(END);
	}
	outlab('D', n, 'B');
	/* the source's trailing ; is the loop's own now */
	pull(&t);
	if (t.type != E_O_F && t.type != SEMI)
		out(&t);
}

/* switch passes through; it just gains its break label at the end */
static void
do_switch(struct token *t0)
{
	int n = next_label++;
	char obp = brkpfx;
	int obn = brknum;

	out(t0);
	copycond();

	brkpfx = 'S'; brknum = n;
	body();
	brkpfx = obp; brknum = obn;

	outlab('S', n, 'B');
}

/*
 * An expression statement (or a label, case, goto, return...):
 * tokens stream through to the semicolon.  A colon hands the rest
 * back to stmt() - what follows a label is a statement of its own,
 * and after a ternary's colon the dispatch is a no-op.  A type
 * keyword hands off to decl(), which either takes the declaration
 * or flushes a cast's tokens through unchanged.
 */
static void
exprstmt(struct token *t)
{
	struct token cur;

	tokcpy(&cur, t);
	for (;;) {
		if (cur.type == E_O_F)
			return;
		if (cur.type == SEMI) {
			out(&cur);
			return;
		}
		if (cur.type == END) {
			/* statement ran into its block's close */
			pushb(&cur);
			return;
		}
		if (cur.type == COLON) {
			out(&cur);
			pull(&cur);
			if (cur.type == E_O_F)
				return;
			stmt(&cur);
			return;
		}
		if ((token_props[cur.type] & (TF_COND | TF_DO | TF_ELSE)) ||
		    cur.type == BREAK || cur.type == CONTINUE) {
			/*
			 * A control keyword mid-stream: after a LABEL
			 * token (ident: is one lexeme), or broken input.
			 * The filters engaged on these anywhere; so does
			 * the dispatch.
			 */
			stmt(&cur);
			return;
		}
		if (is_type_tok(&cur)) {
			if (decl(&cur))
				return;
			pull(&cur);
			continue;
		}
		if (cur.type == BEGIN) {
			/* a block: a labeled compound statement lands
			 * here; its contents are statements */
			stmt(&cur);
			return;
		}
		out(&cur);
		pull(&cur);
	}
}

/*
 * One statement, t already pulled.  This is the dispatch the three
 * filters each re-derived per token; nesting is the call stack.
 */
static void
stmt(struct token *t)
{
	struct token u;

	switch (t->type) {
	case BEGIN:
		out(t);
		bdepth++;
		scopedep++;
		for (;;) {
			pull(&u);
			if (u.type == E_O_F)
				return;
			if (u.type == END) {
				bdepth--;
				scopedep--;
				vpop();
				out(&u);
				return;
			}
			stmt(&u);
		}

	case IF:
		do_if(t);
		return;

	case ELSE:
		/* a dangling else: emit and take its body normally */
		out(t);
		do_else();
		return;

	case WHILE:
		do_while();
		return;

	case FOR:
		do_for();
		return;

	case DO:
		do_do();
		return;

	case SWITCH:
		do_switch(t);
		return;

	case BREAK:
	case CONTINUE:
		/*
		 * The keyword becomes a goto (or passes through when
		 * there is no target); the statement's own ; follows
		 * in the stream and still ends it.
		 */
		if (t->type == BREAK && brkpfx)
			outgoto(brkpfx, brknum, 'B');
		else if (t->type == CONTINUE && cntpfx)
			outgoto(cntpfx, cntnum, cntsfx);
		else
			out(t);
		pull(&u);
		if (u.type != E_O_F)
			exprstmt(&u);
		return;

	case SEMI:
		out(t);
		return;

	default:
		if (is_type_tok(t)) {
			if (decl(t))
				return;
			/* flushed mid-stream: the statement continues */
			pull(&u);
			if (u.type != E_O_F)
				exprstmt(&u);
			return;
		}
		exprstmt(t);
		return;
	}
}

void
norm_init(void)
{
	haveback = 0;
	next_label = 1;
	brkpfx = 0;
	cntpfx = 0;
	bdepth = 0;
	inagg = 0;
	tarr_setup(&cond_a, 48);
	tarr_setup(&init_a, 16);
	tarr_setup(&spec_a, 16);
	tarr_setup(&dini_a, 32);
	name_cnt = 0;
	assign_cnt = 0;
	cur_stars = 0;
	knrinit();
	tdinit();
	reginit();
	mtop = -1;
	mfeed = 0;
	msawtag = 0;
	mtag = 0;
	arr_name = 0;
	szqr = szqw = 0;
	cfprev = 0;
	incf = 0;
	ks_st = 0;
	ks_base = 0;
	ks_stars = 0;
	ks_name = 0;
	ks_cnt = 0;
	ks_nbrk = 0;
	ks_bad = 0;
	ks_pd = 0;
	ks_awtag = 0;
	ks_tag = 0;
	ks_kind = 0;
}

/*
 * File scope: everything streams through untouched - declarations
 * here keep their initializers - except that a struct/union body is
 * skipped verbatim (members are not statements) and a bare { opens
 * either a function body or a brace initializer, both of which the
 * statement machinery handles.
 */
void
norm_run(void)
{
	struct token t;

	for (;;) {
		pull(&t);
		if (t.type == E_O_F)
			return;
		if (t.type == BEGIN) {
			if (inagg) {
				inagg = 0;
				if (ks_kind) {
					/*
					 * ks_awtag still standing means the
					 * token after "struct" was not a
					 * name - an anonymous body, with no
					 * tag to register a size under.  It
					 * kept the last tag seen, so
					 *
					 *   struct big { char x[100]; };
					 *   static struct { char y[6]; } a;
					 *
					 * filed the anonymous body's size
					 * under "big"; stadd pushes and
					 * stfind takes the first match, so
					 * sizeof(struct big) answered 6.
					 * calloc(n, sizeof(struct buf)) then
					 * asked malloc for a fraction of
					 * what the caller went on to use.
					 */
					if (ks_awtag) {
						ks_awtag = 0;
						ks_tag = 0;
					}
					mpush(ks_tag, ks_kind == 2);
					mfeed = 1;
					ks_kind = 0;
				}
				out(&t);
				aggrpass();
				continue;
			}
			stmt(&t);
			continue;
		}
		if (t.type == SYM || is_type_tok(&t)) {
			/* a declaration or K&R function head */
			knr(&t);
			continue;
		}
		kout(&t);
		if (t.type == COMMA)
			dlist = 1;
		else if (t.type == SEMI)
			dlist = 0;
	}
}
