/*
 * The source layer, split from norm.c: lexer -> enum lowering ->
 * typedef dissolution.  What the walker pulls through srcget() is
 * already free of ENUM, TYPEDEF, and every typedef name.  The
 * walker above it lives in norm.c; srcget and tdinit are the whole
 * interface.
 */
#include "cpp.h"
#include "lexeme.h"

/* cpp.c: the lexer the enum layer draws from */
extern void lex_get(struct token *);

/*
 * The source layer: lexer -> enum lowering -> typedef dissolution.
 * What the walker pulls is already free of ENUM, TYPEDEF, and every
 * typedef name.
 */

/*
 * Enum lowering (from filtenum).  Enum constants are glorified
 * #defines: each goes into the macro table as its value, the type
 * itself is rewritten to unsigned char, and a bare declaration
 * vanishes.  Constant names are global for the rest of the file,
 * exactly like #define.
 */
static struct token eqbuf[2];	/* CHAR + the lookahead, at most */
static unsigned char eqn, eqr;

static long enum_expr(struct token *t);

/* t holds the current lookahead on entry and exit */
static long
enum_prim(struct token *t)
{
	long v;

	if (t->type == MINUS) {
		lex_get(t);
		return -enum_prim(t);
	}
	if (t->type == TWIDDLE) {
		lex_get(t);
		return ~enum_prim(t);
	}
	if (t->type == LPAR) {
		lex_get(t);
		v = enum_expr(t);
		if (t->type == RPAR)
			lex_get(t);
		else
			gripe(ER_C_EV);
		return v;
	}
	/* an enum constant is an int however the value was spelled */
	if (t->type == NUMBER || t->type == LNUMBER) {
		v = t->v.numeric;
		lex_get(t);
		return v;
	}
	gripe(ER_C_EV);
	if (t->type != END && t->type != E_O_F)
		lex_get(t);
	return 0;
}

static long
enum_term(struct token *t)
{
	long v = enum_prim(t);

	while (t->type == STAR || t->type == TIMES) {
		lex_get(t);
		v *= enum_prim(t);
	}
	return v;
}

static long
enum_expr(struct token *t)
{
	long v = enum_term(t);
	unsigned char op;

	while (t->type == PLUS || t->type == MINUS) {
		op = t->type;
		lex_get(t);
		if (op == PLUS)
			v += enum_term(t);
		else
			v -= enum_term(t);
	}
	return v;
}

static void
epull(struct token *out)
{
	struct token t;
	struct token syn;
	char def[48];
	char *p;
	long val;

	if (eqr < eqn) {
		tokcpy(out, &eqbuf[eqr++]);
		if (eqr == eqn)
			eqr = eqn = 0;
		return;
	}

	lex_get(&t);
	while (t.type == ENUM) {
		lex_get(&t);			/* consume 'enum' */
		if (t.type == SYM)
			lex_get(&t);		/* tag: documentation only */

		if (t.type == BEGIN) {
			val = 0;
			lex_get(&t);
			while (t.type != END && t.type != E_O_F) {
				if (t.type != SYM) {
					gripe(ER_C_EV);
					lex_get(&t);
					continue;
				}
				/* build "NAME=" while the name is live */
				p = def;
				{
					char *n = t.v.name;
					while (*n && p < def + 32)
						*p++ = *n++;
				}
				*p++ = '=';
				lex_get(&t);
				if (t.type == ASSIGN) {
					lex_get(&t);
					val = enum_expr(&t);
				}
				fmtlong(p, val);
				addDefine(def);
				val++;
				if (t.type == COMMA)
					lex_get(&t);
			}
			if (t.type == END)
				lex_get(&t);	/* consume '}' */
			else
				gripe(ER_C_EV);
		}

		/* t is now the token after the enum construct */
		if (t.type == SEMI) {
			/* bare "enum [tag] { ... };" - swallow entirely */
			lex_get(&t);
			continue;
		}
		/* type reference: replace with 'unsigned char' */
		toksynth(out, UNSIGNED);
		toksynth(&syn, CHAR);
		tokcpy(&eqbuf[eqn++], &syn);
		tokcpy(&eqbuf[eqn++], &t);
		return;
	}
	tokcpy(out, &t);
}

/*
 * Typedef dissolution (from filttdef).  A typedef is a declarator
 * with a hole where the name sits; using the name composes the
 * use-site declarator into the hole.  See the phase-1a commit for
 * the full rules - the engine is unchanged, its output queue is now
 * the walker's token source.
 */
struct tdent {
	char *name;			/* interned */
	struct token *spec, *pre, *post;
	unsigned char nspec, npre, npost;
	struct tdent *next;
};
static struct tdent *tdefs;

/*
 * One set of split arrays per live expansion.  Depth grows only
 * through a typedef inside a declarator of another typedef's use,
 * and one more for the K&R parameter line a terminator can begin.
 */
#define MAXDEPTH 3
static struct tokarray pres[MAXDEPTH], posts[MAXDEPTH];
static unsigned char xdepth;
static struct tokarray tdspec;

/* the expansion output queue - the one buffer between the source
 * layer and the walker */
static struct pendbuf tdq;

static struct tdent *
tdfind(char *name)
{
	struct tdent *t;

	for (t = tdefs; t; t = t->next)
		if (t->name == name)	/* interned: pointer compare */
			return t;
	return 0;
}

/* a sink is the queue (null) or a collection in progress */
static void
sink1(struct tokarray *sink, struct token *t)
{
	if (sink)
		tarr_push(sink, t);
	else
		pend_push(&tdq, t);
}

static void
sinkn(struct tokarray *sink, struct token *buf, int n)
{
	int i;

	for (i = 0; i < n; i++)
		sink1(sink, &buf[i]);
}

static void
sinkt(struct tokarray *sink, unsigned char type)
{
	struct token t;

	toksynth(&t, type);
	sink1(sink, &t);
}

/*
 * Pull the next token, letting line housekeeping flow straight out.
 */
static struct token tdbktok;
static unsigned char tdbkhv;

static void
tdpull(struct token *t)
{
	if (tdbkhv) {
		tdbkhv = 0;
		tokcpy(t, &tdbktok);
		return;
	}
	for (;;) {
		epull(t);
		if (t->type != NEWLINE && t->type != LINENO)
			return;
		pend_push(&tdq, t);
	}
}

static void expand(struct tdent *e, struct token *t,
    struct tokarray *sink);

/*
 * Collect one balanced ( ) or [ ] group into the sink, expanding
 * typedef names inside.  On entry t holds the opener; on exit the
 * first token past the closer.
 */
static void
group(struct token *t, struct tokarray *sink)
{
	unsigned char d = 0;
	struct tdent *e;

	for (;;) {
		if (t->type == LBRACK || t->type == LPAR) {
			d++;
		} else if (t->type == RBRACK || t->type == RPAR) {
			d--;
			sink1(sink, t);
			tdpull(t);
			if (!d)
				return;
			continue;
		} else if (t->type == SYM &&
		    (e = tdfind(t->v.name)) != 0) {
			expand(e, t, sink);
			/* the nested expansion sank its own terminator;
			 * account for it if it closed a level */
			if (t->type == RBRACK || t->type == RPAR) {
				d--;
				if (!d) {
					tdpull(t);
					return;
				}
			}
			tdpull(t);
			continue;
		}
		sink1(sink, t);
		tdpull(t);
	}
}

/*
 * Split one (possibly abstract) declarator into pre / name / post,
 * expanding typedef names inside its groups.  The caller hands in
 * the first token; the terminator comes back in t.
 */
static void
splitdecl(struct token *t, struct tokarray *pre, struct token *name,
    struct tokarray *post)
{
	unsigned char pdepth = 0;

	tarr_reset(pre);
	tarr_reset(post);
	name->type = 0;

	while (t->type == STAR || t->type == TIMES || t->type == LPAR) {
		if (t->type == LPAR)
			pdepth++;
		tarr_push(pre, t);
		tdpull(t);
	}
	if (t->type == SYM) {
		if (!tdfind(t->v.name)) {
			tokcpy(name, t);
			tdpull(t);
		} else {
			/*
			 * A typedef name in the name position: the
			 * declarator's NAME shadowing the typedef, or a
			 * fresh K&R type line.  What follows tells them
			 * apart: a star or another name means type line.
			 */
			struct token peek;

			tdpull(&peek);
			if (peek.type == STAR || peek.type == TIMES ||
			    peek.type == SYM) {
				tokcpy(&tdbktok, &peek);
				tdbkhv = 1;
			} else {
				tokcpy(name, t);
				tokcpy(t, &peek);
			}
		}
	}
	for (;;) {
		if (t->type == RPAR && pdepth) {
			pdepth--;
			tarr_push(post, t);
			tdpull(t);
			continue;
		}
		if (t->type == LBRACK || t->type == LPAR) {
			group(t, post);
			continue;
		}
		return;
	}
}

/*
 * Emit one wrapped declarator: pre_t [(] pre_u name post_u [)]
 * post_t.  Parens exactly when the use-site starts prefix-ish and
 * the hole continues with a postfix.
 */
static void
wrap(struct tdent *e, struct tokarray *pre, struct token *name,
    struct tokarray *post, struct tokarray *sink)
{
	int parens = pre->count && e->npost;

	sinkn(sink, e->pre, e->npre);
	if (parens)
		sinkt(sink, LPAR);
	sinkn(sink, pre->buf, pre->count);
	if (name->type)
		sink1(sink, name);
	sinkn(sink, post->buf, post->count);
	if (parens)
		sinkt(sink, RPAR);
	sinkn(sink, e->post, e->npost);
}

/*
 * Save a token array into the permanent arena.
 */
#ifdef DEBUG
long tdkeepB;		/* poolstats: bytes kept for entries */
int tdkeepN;
#endif

static struct token *
keep(struct tokarray *a, unsigned char *n)
{
	struct token *p;
	int i;

	*n = a->count;
	if (!a->count)
		return 0;
	p = (struct token *)permalloc(a->count * sizeof(struct token));
#ifdef DEBUG
	tdkeepB += a->count * sizeof(struct token);
	tdkeepN++;
#endif
	for (i = 0; i < a->count; i++)
		tokcpy(&p[i], &a->buf[i]);
	return p;
}

/*
 * Pass tokens through until a depth-0 comma or semicolon, expanding
 * typedef names met on the way - an initialiser can hold a cast or
 * a sizeof.  The terminator comes back in t, already sunk.
 */
static void
drain(struct token *t, struct tokarray *sink)
{
	unsigned char d = 0;
	struct tdent *e;

	for (;;) {
		tdpull(t);
		if (t->type == SYM && (e = tdfind(t->v.name)) != 0) {
			expand(e, t, sink);
			if (t->type == SEMI ||
			    (t->type == COMMA && !d))
				return;
			if (t->type == RPAR || t->type == RBRACK) {
				if (!d)
					return;
				d--;
			}
			continue;
		}
		/*
		 * BEGIN and END count too.  A comma inside braces
		 * separates initializers, not declarators - "T a[2] =
		 * { 1, 2 }" ended the declarator at the comma, and the
		 * caller then emitted it a second time as the one that
		 * introduces the next declarator, so pass1 was handed
		 * "{ 1 , , 2 }".  A single-element initializer has no
		 * comma and was fine, which is why this survived.
		 */
		if (t->type == LPAR || t->type == LBRACK ||
		    t->type == BEGIN) {
			d++;
		} else if (t->type == RPAR || t->type == RBRACK ||
		    t->type == END) {
			if (!d) {
				sink1(sink, t);
				return;
			}
			d--;
		} else if (t->type == SEMI ||
		    (t->type == COMMA && !d)) {
			sink1(sink, t);
			return;
		}
		sink1(sink, t);
	}
}

/*
 * A typedef name met in the stream: emit the specs once, then wrap
 * declarators until the list ends.  On return the terminator has
 * been sunk and t holds it.
 */
static void
expand(struct tdent *e, struct token *t, struct tokarray *sink)
{
	struct token name;
	struct tokarray *pre, *post;
	int have = 0;

	if (xdepth >= MAXDEPTH) {
		error("typedefs nested too deep");
		return;
	}
	pre = &pres[xdepth];
	post = &posts[xdepth];
	xdepth++;

	sinkn(sink, e->spec, e->nspec);
	for (;;) {
		if (!have)
			tdpull(t);
		have = 0;
		splitdecl(t, pre, &name, post);
		wrap(e, pre, &name, post, sink);
		if (t->type == ASSIGN) {
			sink1(sink, t);
			drain(t, sink);
			if (t->type != COMMA)
				break;
		}
		if (t->type != COMMA) {
			/* the terminator can itself be a typedef name:
			 * a K&R parameter line begins with one */
			if (t->type == SYM) {
				struct tdent *e3 = tdfind(t->v.name);

				if (e3) {
					expand(e3, t, sink);
					break;
				}
			}
			sink1(sink, t);
			break;
		}
		/* past the comma: ours, or somebody else's? */
		sink1(sink, t);
		tdpull(t);
		if (t->type == STAR || t->type == TIMES ||
		    t->type == LPAR) {
			have = 1;
			continue;
		}
		if (t->type == SYM) {
			struct tdent *e2 = tdfind(t->v.name);

			if (!e2) {
				have = 1;
				continue;
			}
			expand(e2, t, sink);
			break;
		}
		/* a type keyword or anything else: not our list */
		sink1(sink, t);
		break;
	}
	xdepth--;
}

/*
 * The whole typedef declaration, "typedef" already consumed.
 * Nothing goes downstream except a struct body, which is streamed
 * one member token per srcget call to keep the queue small.
 */
static unsigned char tdbody;	/* mid-body: tdcur is live */
static unsigned char tdfin;	/* body closed: resume the specs */
static unsigned char tddepth;
static struct token tdcur;
static unsigned char aftertag;

static void capture2(struct token *t);

static void
capture(struct token *t)
{
	tarr_reset(&tdspec);
	tdpull(t);
	capture2(t);
}

static void
capture2(struct token *t)
{
	struct tdent *e;
	struct token name;

	/*
	 * specs: a struct/union head, keywords, or an earlier typedef.
	 * struct/union first - the generic arm would eat the keyword
	 * and leave the tag looking like a name.
	 */
	for (;;) {
		if (t->type == STRUCT || t->type == UNION) {
			tarr_push(&tdspec, t);
			tdpull(t);
			if (t->type != SYM) {
				error("typedef of unnamed struct needs a tag");
				return;
			}
			tarr_push(&tdspec, t);
			tdpull(t);
			if (t->type == BEGIN) {
				/* the body is a real declaration of the
				 * tag, streamed downstream once */
				pend_buf(&tdq, tdspec.buf, tdspec.count);
				pend_push(&tdq, t);
				tdpull(t);
				tokcpy(&tdcur, t);
				tddepth = 1;
				tdbody = 1;
				return;
			}
			continue;
		}
		if (is_type_kw(t->type)) {
			tarr_push(&tdspec, t);
			tdpull(t);
			continue;
		}
		if (t->type == SYM && tdfind(t->v.name) != 0) {
			/* a typedef of a typedef: loud, not silent */
			error("typedef of a typedef");
			while (t->type != SEMI && t->type != E_O_F)
				tdpull(t);
			return;
		}
		break;
	}

	/* declarators: each one becomes an entry */
	for (;;) {
		splitdecl(t, &pres[0], &name, &posts[0]);
		if (!name.type) {
			error("typedef with no name");
			return;
		}
		e = (struct tdent *)permalloc(sizeof(*e));
		e->name = name.v.name;
		e->spec = keep(&tdspec, &e->nspec);
		e->pre = keep(&pres[0], &e->npre);
		e->post = keep(&posts[0], &e->npost);
		e->next = tdefs;
		tdefs = e;

		if (t->type == COMMA) {
			tdpull(t);
			continue;
		}
		if (t->type != SEMI)
			error("junk in typedef");
		return;
	}
}

void
srcget(struct token *out)
{
	struct token t;
	struct tdent *e;

	for (;;) {
		if (tdbody && !pend_has(&tdq)) {
			/* one member token per call, expansions in
			 * small bursts */
			struct tdent *m;

			for (;;) {
				if (tdcur.type == BEGIN)
					tddepth++;
				else if (tdcur.type == END) {
					if (--tddepth == 0) {
						pend_push(&tdq, &tdcur);
						tdpull(&tdcur);
						pend_tok(&tdq, SEMI);
						tdbody = 0;
						tdfin = 1;
						break;
					}
				} else if (tdcur.type == SYM &&
				    (m = tdfind(tdcur.v.name)) != 0) {
					expand(m, &tdcur, 0);
					continue;
				}
				pend_push(&tdq, &tdcur);
				tdpull(&tdcur);
				break;
			}
		}
		if (tdfin && !pend_has(&tdq)) {
			/* the body has drained; the declarators follow,
			 * starting with the token already in hand */
			tdfin = 0;
			tokcpy(&t, &tdcur);
			capture2(&t);
			continue;
		}
		if (pend_has(&tdq)) {
			pend_pop(&tdq, out);
			return;
		}
		epull(&t);
		if (t.type == E_O_F) {
			tokcpy(out, &t);
			return;
		}

		if (t.type == TYPEDEF) {
			capture(&t);
			continue;	/* nothing but the queue */
		}

		if (t.type == SYM && !aftertag &&
		    (e = tdfind(t.v.name)) != 0) {
			expand(e, &t, 0);
			continue;
		}

		/*
		 * A tag position is a different namespace: "struct
		 * Expr" must not have its tag expanded even though
		 * Expr is also a typedef name.  Member access likewise.
		 */
		aftertag = (t.type == STRUCT || t.type == UNION ||
		    t.type == DOT || t.type == ARROW);

		tokcpy(out, &t);
		return;
	}
}

/*
 * Entry points
 */

void
tdinit(void)
{
	int i;

	eqn = eqr = 0;
	pend_setup(&tdq, 16);
	tarr_setup(&tdspec, 8);
	for (i = 0; i < MAXDEPTH; i++) {
		tarr_setup(&pres[i], 8);
		tarr_setup(&posts[i], 8);
	}
	tdefs = 0;
	xdepth = 0;
	tdbkhv = 0;
	tdbody = 0;
	tdfin = 0;
	aftertag = 0;
}
