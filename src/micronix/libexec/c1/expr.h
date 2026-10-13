/*
 * expr.h - expression tree definitions
 */
#ifndef EXPR_H
#define EXPR_H


/* destination values */
#define DEST_NONE	0	/* statement expression, discard result */
#define DEST_FLAGS	1	/* result needed in flags (conditionals) */
#define DEST_VALUE	2	/* result needed as value */
#define DEST_STACK	3	/* result pushed to stack (call args) */

/* nored flags: two reduction hints and one width demand */
#define NR_NORED	1	/* don't reduce: preserve for parent rule */
#define NR_ADDR		2	/* parent wants the address, not the place */
#define NR_BYTEWANT	4	/* value demanded as a byte by its consumer */

/*
 * A SYM/SYMREF name's kind rides the free high bits of nored (bits 3-5;
 * the codegen only ever touches the NR_* low bits).  NK_* must match
 * c0's outast.c.  The id itself is u.name / u.symref.name.
 */
#define NK_SHIFT	3
#define NK_MASK		(7 << NK_SHIFT)
#define NK_GLOBAL	0	/* _@id   global / extern / file-scope static */
#define NK_STATIC	1	/* _@func.N  function-scope static (interned) */
#define NK_LABEL	2	/* L%d */
#define NK_STRING	3	/* str%d */
#define NK_LOCAL	4	/* @id */
#define namekind(e)	(((e)->nored >> NK_SHIFT) & 7)
#define setnamekind(e,k) ((e)->nored = ((e)->nored & ~NK_MASK) | ((k) << NK_SHIFT))

/* expression node */
typedef struct Expr {
	unsigned char	op;	/* operator (lexeme token) */
	unsigned char	width;	/* type: b/B/s/S/l/L/p/v */
	unsigned char	dest;	/* destination */
	unsigned char	regs;	/* Sethi-Ullman label: regs needed */
	unsigned char	tgt;	/* target register (R_HL, R_DE, 0=any) */
	unsigned char	nored;	/* NR_* flags below */
	struct Expr	*left;	/* left child (unary: only child) */
	struct Expr	*right;	/* right child (binary ops only) */
	union {
		long		val;	/* constant value */
		char		*name;	/* symbol name */
		struct {
			unsigned char	argc;	/* call: arg count */
		} call;
		struct {
			unsigned char	reg;	/* register number */
			short	off;	/* frame offset (arrays pass -128) */
		} var;
		struct {
			unsigned short	amt;	/* inc/dec amount */
		} incdec;
		struct {
			unsigned char	off;	/* bitfield offset */
			unsigned char	wid;	/* bitfield width */
		} bf;
		struct {
			char		*name;	/* symbol name */
			short		off;	/* constant offset */
		} symref;
	} u;
} Expr;

/* tree builder functions */
Expr	*mkconst(char width, long val);
Expr	*mksym(int kind, char *name);
Expr	*mklocalvar(char width, char reg, int off);
Expr	*mkregvar(char width, char reg);
Expr	*mkindex(char width, char reg, int off);
Expr	*mkunary(int op, char width, Expr *child);
Expr	*mkbinary(int op, char width, Expr *left, Expr *right);
Expr	*mkcall(char width, int argc, Expr *func, Expr *args);
Expr	*mkarg(Expr *expr);
Expr	*mkincdec(int op, char width, Expr *e, int amt);
Expr	*mkbfext(char off, char wid, Expr *addr);
Expr	*mkbfass(char off, char wid, Expr *addr, Expr *val);
Expr	*mksymref(int kind, char *name, short off);
Expr	*mkcode(char width, char reg);

/* per-function arena: Expr nodes and interned symbol names */
char	*arena_alloc(unsigned int n);
char	*arena_strdup(char *name);
void	arena_reset(void);

/* tree operations */
void	setdest(Expr *e, char dest);
void	label(Expr *e);	/* Sethi-Ullman register labeling */
void	assign(Expr *e, unsigned char tgt);	/* register assignment */
Expr	*dupexpr(Expr *e);
void	freeexpr(Expr *e);
Expr	*rewrite(Expr *e);
/* the condition that branches when a reduced condition is false,
 * emitting the zero test first if the answer came back as a value.
 * Declared here rather than in pass2.h because it needs Expr, and
 * with empty parens zc3 reads it as taking none and objects to the
 * definition. */
char	*falsecc(Expr *e);
char	*truecc(Expr *e);

/* parser */
Expr	*readexpr(void);

#ifdef DEBUG
void	dumpexpr(Expr *e);
char	*opname(int op);
#endif

#endif
