/*
 * K&R function normalization, split from norm.c: the file-scope
 * engagement that turns
 *
 *	int foo(a, b)		int foo(int a, char *b)
 *	int a;		->	{
 *	char *b;
 *	{
 *
 * and, through kout, is the one place file-scope tokens leave the
 * walker.  norm.c calls knr() and knrinit(); everything the header
 * buffering needs lives here.
 */
#include <stdlib.h>
#include <string.h>
#include "cpp.h"
#include "lexeme.h"

/*
 * K&R function normalization (from filtknr) - file scope only.
 *
 *	int foo(a, b)		int foo(int a, char *b)
 *	int a;		->	{
 *	char *b;
 *	{
 *
 * The header is buffered from its first type token (or bare name -
 * implicit int) through the { or ; that ends it, then re-emitted in
 * ANSI form.  Anything that stops looking like a K&R definition
 * flushes what was buffered and steps aside.
 */
#define PARAM_MAX 16
struct kparm {
	char *name;
	struct token *type;
	char type_len;
	char stars;
	struct token *post;	/* fn-ptr declarator tail: `)(int)` */
	char post_len;
};
static struct kparm kparms[PARAM_MAX];
static unsigned char kp_cnt;

static struct tokarray rtype_a;
static struct tokarray ptype_a;
static struct tokarray tail_a;
static struct tokarray ptail_a;
static int kp_stars;
static int kp_adim;	/* [] groups seen on the declarator in hand */
static int kp_pdepth;
static int kp_preopen;
static char *cur_pname;
static struct token fname;
static struct token save_lp;

/* mid declarator-list at file scope: later declarators share the
 * first one's base type - no implicit int for them */
int dlist;

/* file-scope emission: track aggregate heads so a following { is
 * recognized as a struct body, not a statement block */
void
kout(struct token *t)
{
	if (t->type == STRUCT || t->type == UNION || t->type == ENUM)
		inagg = 1;
	else if (t->type != SYM && t->type != BEGIN)
		inagg = 0;
	kscan(t);
	out(t);
}

static void
koutarr(struct tokarray *a)
{
	int i;

	for (i = 0; i < a->count; i++)
		kout(&a->buf[i]);
}

/* synthesized header tokens go through the same tracking - a TIMES
 * or RPAR must clear a struct head just like a real one, or the
 * body { of a struct-param function reads as a struct body */
static void
kouta(unsigned char type, struct token *ref)
{
	struct token t;

	t.type = type;
	t.v.numeric = 0;
	t.lineno = ref->lineno;
	t.filename = ref->filename;
	kout(&t);
}

static void
koutt(unsigned char type)
{
	struct token t;

	toksynth(&t, type);
	kout(&t);
}

static struct kparm *
find_kparm(char *name)
{
	register struct kparm *pp = kparms;
	unsigned char n = kp_cnt + 1;

	while (--n) {
		if (pp->name[0] == name[0] && strcmp(pp->name, name) == 0)
			return pp;
		pp++;
	}
	return 0;
}

static void
save_ptype(char *name, int stars)
{
	register struct kparm *pp = find_kparm(name);

	if (pp && ptype_a.count > 0) {
		register struct token *d;
		struct token *s;
		int n;

		n = ptype_a.count;
		d = pp->type = (struct token *)xalloc(n * sizeof(struct token));
		s = ptype_a.buf;
		while (n--)
			tokcpy(d++, s++);
		pp->type_len = ptype_a.count;
		pp->stars = stars;
		n = ptail_a.count;
		if (n > 0) {
			d = pp->post = (struct token *)xalloc(n * sizeof(struct token));
			s = ptail_a.buf;
			while (n--)
				tokcpy(d++, s++);
			pp->post_len = ptail_a.count;
		}
	}
	tarr_reset(&ptail_a);
	/* base type stays buffered: char *a, *b; reuses it for b */
}

/* the merged ANSI-style declaration */
static void
emit_ansi(void)
{
	register struct kparm *pp;
	unsigned char n;
	int j;
	struct token tmp;

	/* synthesize the implicit int: c0 requires a return type */
	if (rtype_a.count == 0 && !dlist)
		kouta(INT, &fname);
	else
		koutarr(&rtype_a);
	kout(&fname);
	kouta(LPAR, &fname);

	pp = kparms;
	n = kp_cnt + 1;
	while (--n) {
		if (pp->type_len > 0) {
			j = pp->type_len;
			{
				struct token *s = pp->type;
				while (j--)
					kout(s++);
			}
		} else {
			/* K&R default: untyped params are int */
			kouta(INT, &fname);
		}
		for (j = pp->stars; j > 0; j--)
			kouta(TIMES, &fname);
		tmp.type = SYM;
		tmp.v.name = pp->name;
		tmp.lineno = fname.lineno;
		tmp.filename = fname.filename;
		kout(&tmp);
		for (j = 0; j < pp->post_len; j++)
			kout(&pp->post[j]);
		if (n > 1)
			kouta(COMMA, &fname);
		free(pp->type);
		pp->type = 0;
		free(pp->post);
		pp->post = 0;
		pp->post_len = 0;
		pp++;
	}

	kouta(RPAR, &fname);
	/* declarator tail: `)) (args)` of void (*signal(...))(args) */
	koutarr(&tail_a);
}

static void
kreset(void)
{
	register struct kparm *pp = kparms;
	unsigned char n = kp_cnt + 1;

	while (--n) {
		free(pp->type);
		pp->type = 0;
		free(pp->post);
		pp->post = 0;
		pp->post_len = 0;
		pp++;
	}
	tarr_reset(&rtype_a);
	kp_cnt = 0;
	tarr_reset(&ptype_a);
	tarr_reset(&tail_a);
	tarr_reset(&ptail_a);
	kp_preopen = 0;
	kp_pdepth = 0;
}

/* knr() states */
#define K_RTYPE		0	/* buffering the return type */
#define K_NAME		1	/* saw a candidate function name */
#define K_PARAMS	2	/* inside the () name list */
#define K_PDECL		3	/* reading K&R parameter declarations */
#define K_PTYPE		4	/* buffering one parameter's type */
#define K_TAIL		5	/* declarator tail: `)) (args)` */

/*
 * Abort: this stopped looking like K&R.  Re-emit what was consumed
 * (with the implicit int - an abort happens only after `name (`,
 * always a function) and let the rest flow.  st is the state the
 * machine was in when it gave up.
 */
static void
abort_knr(int st)
{
	register struct kparm *pp;
	unsigned char n;
	int i;
	struct token tmp;

	if (rtype_a.count == 0 && !dlist)
		kouta(INT, &fname);
	else
		koutarr(&rtype_a);
	kout(&fname);
	kout(&save_lp);
	pp = kparms;
	n = kp_cnt + 1;
	while (--n) {
		toksynthnam(&tmp, SYM, pp->name);
		kout(&tmp);
		if (n > 1 || st == K_PARAMS)
			koutt(COMMA);
		pp++;
	}
	if (st == K_PDECL || st == K_PTYPE || st == K_TAIL) {
		/* the ) was consumed on leaving K_PARAMS - put it back,
		 * then flush the tail and any partial K&R param decl */
		koutt(RPAR);
		koutarr(&tail_a);
		koutarr(&ptype_a);
		for (i = 0; i < kp_stars; i++)
			koutt(TIMES);
		if (cur_pname) {
			toksynthnam(&tmp, SYM, cur_pname);
			kout(&tmp);
		}
		koutarr(&ptail_a);
		tarr_reset(&ptype_a);
		tarr_reset(&ptail_a);
		kp_stars = 0;
		kp_adim = 0;
		kp_pdepth = 0;
		cur_pname = 0;
	}
	/* in K_PARAMS the ) is still upcoming and flows through */
	tarr_reset(&rtype_a);
	tarr_reset(&tail_a);
	kp_preopen = 0;
	kp_cnt = 0;
}

/* flush a non-function: buffered tokens, then the terminator - a {
 * goes back for dispatch (it opens a struct body or a block) */
static void
kflush(struct token *t)
{
	koutarr(&rtype_a);
	tarr_reset(&rtype_a);
	kp_preopen = 0;
	if (t->type == BEGIN)
		pushb(t);
	else
		kout(t);
}

/*
 * One file-scope engagement, entered on a type token or a bare SYM.
 * Everything through the end of the construct is consumed; on return
 * the next pull starts fresh.
 */
void
knr(struct token *t)
{
	struct token cur;
	int st;

	tarr_reset(&rtype_a);
	kp_preopen = 0;
	if (t->type == SYM) {
		/* implicit-int definition: fseek(f, o, p) char *f; */
		tokcpy(&fname, t);
		st = K_NAME;
	} else {
		tarr_push(&rtype_a, t);
		st = K_RTYPE;
	}

	for (;;) {
		pull(&cur);
		if (cur.type == E_O_F) {
			if (st == K_RTYPE)
				koutarr(&rtype_a);
			tarr_reset(&rtype_a);
			return;
		}

		switch (st) {
		case K_RTYPE:
			/* `( *` of a parenthesized declarator buffers as
			 * part of the prefix */
			if (cur.type == LPAR) {
				tarr_push(&rtype_a, &cur);
				kp_preopen++;
				continue;
			}
			if (is_type_tok(&cur) || cur.type == STAR ||
			    cur.type == TIMES) {
				tarr_push(&rtype_a, &cur);
				continue;
			}
			if (cur.type == SYM) {
				if (tag_pending(&rtype_a)) {
					tarr_push(&rtype_a, &cur);
					continue;
				}
				tokcpy(&fname, &cur);
				st = K_NAME;
				continue;
			}
			kflush(&cur);
			return;

		case K_NAME:
			if (cur.type == LPAR) {
				tokcpy(&save_lp, &cur);
				st = K_PARAMS;
				kp_pdepth = 1;
				kp_cnt = 0;
				continue;
			}
			if (cur.type == COMMA)
				dlist = 1;
			else if (cur.type == SEMI)
				dlist = 0;
			koutarr(&rtype_a);
			kout(&fname);
			tarr_reset(&rtype_a);
			kp_preopen = 0;
			if (cur.type == BEGIN)
				pushb(&cur);
			else
				kout(&cur);
			return;

		case K_PARAMS:
			if (cur.type == RPAR) {
				kp_pdepth--;
				if (kp_pdepth == 0)
					st = K_PDECL;
				continue;
			}
			if (cur.type == LPAR) {
				kp_pdepth++;
				continue;
			}
			if (cur.type != COMMA) {
				if (is_type_tok(&cur)) {
					/* types in the list = ANSI: stream
					 * the rest through its ) verbatim,
					 * or the parameter names re-engage
					 * at file scope and a list comma
					 * leaves dlist set - which cost the
					 * next bare-SYM definition its
					 * implicit int */
					abort_knr(st);
					kout(&cur);
					while (kp_pdepth > 0) {
						pull(&cur);
						if (cur.type == E_O_F)
							return;
						if (cur.type == LPAR)
							kp_pdepth++;
						else if (cur.type == RPAR)
							kp_pdepth--;
						kout(&cur);
					}
					return;
				}
				if (cur.type == SYM) {
					if (kp_cnt >= PARAM_MAX) {
						gripe(ER_C_PC);
						continue;
					}
					{
						register struct kparm *pp;

						pp = &kparms[kp_cnt++];
						pp->name = cur.v.name;
						pp->type = 0;
						pp->type_len = 0;
						pp->stars = 0;
						pp->post = 0;
						pp->post_len = 0;
					}
				}
			}
			continue;

		case K_TAIL:
			if (cur.type == RPAR && kp_preopen > 0) {
				tarr_push(&tail_a, &cur);
				kp_preopen--;
				continue;
			}
			if (cur.type == LPAR) {
				tarr_push(&tail_a, &cur);
				kp_preopen++;
				continue;
			}
			if (kp_preopen > 0) {
				tarr_push(&tail_a, &cur);
				continue;
			}
			/* tail complete: reprocess as K_PDECL */
			st = K_PDECL;
			/* FALLTHROUGH */
		case K_PDECL:
			if (cur.type == BEGIN) {
				/* function body: the merged declaration,
				 * then the { dispatches as a block */
				emit_ansi();
				dlist = 0;
				kreset();
				pushb(&cur);
				return;
			}
			if (cur.type == SEMI) {
				if (ptype_a.count == 0 && cur_pname == 0) {
					/* no K&R declarations: a prototype */
					emit_ansi();
					dlist = 0;
					kreset();
					kout(&cur);
					return;
				}
				if (cur_pname)
					save_ptype(cur_pname, kp_stars);
				cur_pname = 0;
				tarr_reset(&ptype_a);
				kp_stars = 0;
				kp_adim = 0;
				continue;
			}
			if (cur.type == COMMA && ptype_a.count == 0 &&
			    cur_pname == 0) {
				/* declarator list: type f(), g(); */
				emit_ansi();
				dlist = 1;
				kreset();
				kout(&cur);
				return;
			}
			if (is_type_tok(&cur)) {
				tarr_push(&ptype_a, &cur);
				st = K_PTYPE;
				continue;
			}
			/*
			 * A declaration that is nothing but the name:
			 *
			 *	f(a) a; { }	f(a, b) int a; b; { }
			 *
			 * K&R implicit int, and the only thing missing was
			 * permission to see it - K_PTYPE already reads the
			 * name, the commas, the stars and the [] decay, and
			 * emit_ansi already spells a parameter with no type
			 * as int.  Falling through to abort_knr instead
			 * meant the whole rewrite was abandoned, and worse
			 * than abandoned in a mixed list: "f(a, b) int a;
			 * b;" had already folded int a into the rewrite
			 * when b aborted it, so what reached c0 was neither
			 * the K&R text nor the ANSI text but half of each.
			 *
			 * Only for a name from this function's own
			 * parameter list.  Any other SYM here is not a K&R
			 * parameter declaration and still belongs to
			 * abort_knr, which is what keeps the bail-out
			 * meaning something.
			 */
			if (cur.type == SYM && find_kparm(cur.v.name)) {
				cur_pname = cur.v.name;
				st = K_PTYPE;
				continue;
			}
			if (cur.type == RPAR && kp_preopen > 0) {
				tarr_push(&tail_a, &cur);
				kp_preopen--;
				st = K_TAIL;
				continue;
			}
			abort_knr(st);
			if (cur.type == BEGIN)
				pushb(&cur);
			else
				kout(&cur);
			return;

		case K_PTYPE:
			/*
			 * Parenthesized declarator in a K&R param decl,
			 * void (*action)(int);: prefix into ptype_a, the
			 * SYM inside is the name, the rest into ptail_a.
			 */
			if (kp_pdepth > 0 || ptail_a.count > 0) {
				if (cur.type == LPAR) {
					if (cur_pname)
						tarr_push(&ptail_a, &cur);
					else
						tarr_push(&ptype_a, &cur);
					kp_pdepth++;
					continue;
				}
				if (cur.type == RPAR) {
					tarr_push(&ptail_a, &cur);
					kp_pdepth--;
					continue;
				}
				if (kp_pdepth > 0) {
					if (!cur_pname && (cur.type == STAR ||
					    cur.type == TIMES)) {
						tarr_push(&ptype_a, &cur);
						continue;
					}
					if (!cur_pname && cur.type == SYM) {
						cur_pname = cur.v.name;
						continue;
					}
					tarr_push(&ptail_a, &cur);
					continue;
				}
				/* depth 0 with a tail: fall through to
				 * the , / ; handling */
			}
			if (cur.type == LPAR && cur_pname == 0) {
				tarr_push(&ptype_a, &cur);
				kp_pdepth = 1;
				continue;
			}
			if (is_type_tok(&cur)) {
				tarr_push(&ptype_a, &cur);
				continue;
			}
			if (cur.type == STAR || cur.type == TIMES) {
				kp_stars++;
				continue;
			}
			if (cur.type == SYM) {
				if (tag_pending(&ptype_a)) {
					tarr_push(&ptype_a, &cur);
					continue;
				}
				cur_pname = cur.v.name;
				continue;
			}
			if (cur.type == COMMA) {
				/* int a, b; - same type, next param */
				int had_tail = (ptail_a.count > 0);

				if (cur_pname)
					save_ptype(cur_pname, kp_stars);
				cur_pname = 0;
				kp_stars = 0;
				kp_adim = 0;
				if (had_tail) {
					/* fn-ptr `( *` prefix is
					 * per-declarator, no sharing */
					tarr_reset(&ptype_a);
					kp_pdepth = 0;
				}
				continue;
			}
			if (cur.type == SEMI) {
				if (cur_pname)
					save_ptype(cur_pname, kp_stars);
				cur_pname = 0;
				tarr_reset(&ptype_a);
				tarr_reset(&ptail_a);
				kp_stars = 0;
				kp_adim = 0;
				kp_pdepth = 0;
				st = K_PDECL;
				continue;
			}
			/*
			 * An array parameter IS a pointer - "int a[]" as a
			 * parameter means "int *a", and the bound, if one is
			 * written, means nothing.  So a [] group is one more
			 * star and the tokens inside it are dropped, which
			 * puts "main(ac, av) int ac; char *av[];" - how every
			 * program of that vintage spells argv - into exactly
			 * the ANSI form c0 already compiles.
			 *
			 * Without this the [ matched nothing here and fell
			 * into abort_knr below: the normalizer gave up, put
			 * the original K&R text back, and ANSI-only c0 met
			 * "int foo(a) int a[];" and answered "fn array" -
			 * an honest complaint about a shape it should never
			 * have been shown.
			 *
			 * Only the FIRST group decays.  "char a[][10]" is
			 * "char (*a)[10]", which is neither a star nor a
			 * suffix, and emit_ansi can spell neither - so that
			 * one has to go back to abort_knr rather than
			 * quietly become char **a.
			 *
			 * Which means the group cannot be thrown away until
			 * the token AFTER it has been seen.  It is buffered
			 * in ptail_a - guaranteed empty here, a fn-ptr tail
			 * and an array suffix cannot both be in hand - and
			 * only then turned into a star.  If a second [
			 * follows, ptail_a still holds the first group
			 * verbatim, abort_knr flushes it where it belongs,
			 * and the text that goes to c0 is the text that came
			 * in.  Decaying first and discovering the second
			 * dimension afterwards would have handed c0
			 * "char *a[10]" - a shape it compiles WITHOUT
			 * complaint, and the wrong one.
			 */
			if (cur.type == LBRACK && cur_pname &&
			    kp_pdepth == 0 && ptail_a.count == 0 &&
			    kp_adim == 0) {
				int depth = 1;

				tarr_push(&ptail_a, &cur);
				while (depth > 0) {
					pull(&cur);
					if (cur.type == E_O_F)
						return;
					if (cur.type == LBRACK)
						depth++;
					else if (cur.type == RBRACK)
						depth--;
					tarr_push(&ptail_a, &cur);
				}
				kp_adim++;
				pull(&cur);
				if (cur.type == E_O_F)
					return;
				if (cur.type == LBRACK) {
					abort_knr(st);
					kout(&cur);
					return;
				}
				/* a plain [] after all: it is one star */
				tarr_reset(&ptail_a);
				kp_stars++;
				pushb(&cur);
				continue;
			}
			abort_knr(st);
			if (cur.type == BEGIN)
				pushb(&cur);
			else
				kout(&cur);
			return;
		}
	}
}

void
knrinit(void)
{
	tarr_setup(&rtype_a, 16);
	tarr_setup(&ptype_a, 16);
	tarr_setup(&tail_a, 8);
	tarr_setup(&ptail_a, 8);
	kp_cnt = 0;
	kp_stars = 0;
	kp_adim = 0;
	kp_pdepth = 0;
	kp_preopen = 0;
	cur_pname = 0;
	dlist = 0;
}
