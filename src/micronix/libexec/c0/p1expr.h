/*
 * expressions: the tree, its flags, and operator precedence
 *
 * Split out of cc1.h, which every source included for everything.
 * That cost more than tidiness: cpp builds a table for every name it
 * sees, and the whole of cc1.h left it room for forty-six more
 * declarations - not enough for any real source, which is why pass1
 * could not be compiled on CP/M at all.  A file that wants types and
 * not statements now says so and pays for what it uses.
 *
 * cc1.h includes all of these, so anything not yet narrowed still
 * gets what it always did.
 */
#ifndef _P1EXPR_H
#define _P1EXPR_H

#include "p1base.h"

/*
 * expressions hold computations
 */
struct expr {
	unsigned char flags;
#define	E_CONST     0x01
#define E_RESOLVED	0x02
#define	E_FUNARG    0x04
#define E_POSTFIX   0x08
	unsigned char op;
	struct expr *left;
	struct expr *right;
	/*
	 * There is no up.  There was, and every construction site wrote
	 * it, but nothing in the tree ever read one - the walks all go
	 * down.  Two bytes a node on the machine that runs out of nodes,
	 * and a store or two at every place a node is built.
	 */
	struct expr *next;

	struct type *type;
    /* for STRING expressions, cast to (struct name *) to get synthetic name */
	struct var *var;      

	unsigned long v;
};

/*
 * Operator precedence levels for expression parsing
 * Lower numbers bind tighter (higher precedence)
 * When parseExpr(N) encounters operator with priority >= N, it stops
 */
#define	PRI_ALL        0   /* parse all operators regardless of precedence */

#define OP_PRI_NONE     0   /* not an operator */
#define OP_PRI_PRIM     1   /* postfix/member access: . -> [] () */
#define OP_PRI_MULT     3   /* multiplicative: * / % */
#define OP_PRI_ADD      4   /* additive: + - */
#define OP_PRI_SHIFT    5   /* bitwise shift: << >> */
#define OP_PRI_REL      6   /* relational: < <= > >= */
#define OP_PRI_EQUAL    7   /* equality: == != */
#define OP_PRI_BITAND   8   /* bitwise AND: & */
#define OP_PRI_BITXOR   9   /* bitwise XOR: ^ */
#define OP_PRI_BITOR   10   /* bitwise OR: | */
#define OP_PRI_LOGAND  11   /* logical AND: && */
#define OP_PRI_LOGOR   12   /* logical OR: || */
#define OP_PRI_COND    13   /* conditional: ?: */
#define OP_PRI_ASSIGN  14   /* assignment: = += -= *= /= %= &= |= ^= <<= >>= */
#define OP_PRI_COMMA   15   /* comma: , */

extern struct expr *mkexpr(unsigned char op, struct expr *left);
extern struct expr *mkexprI(unsigned char op, struct expr *left,
    struct type *type, unsigned long v, int flags);
extern struct expr *parseExpr(unsigned char priority);
extern int isTypeToken(unsigned char t);
extern struct expr *mkbin(unsigned char op, struct expr *l,
    struct expr *r, struct type *t);
extern void freeNode(struct expr *e);
extern struct expr *narrowidx(struct expr *e);
extern struct type *unwrapDeref(struct expr **ep);
extern void skipExpr(unsigned char pri);
extern unsigned short hlArgSym;  /* parameter named while HL still held it */
extern void hlOpen(void);
extern void hlClose(void);
extern struct expr *mkIncDec(struct expr *operand,
    unsigned char inc_op, unsigned char is_postfix);
extern int isaggr(struct type *t);
extern int ptrcompat(struct type *lt, struct type *rt);
extern unsigned char binopPri(unsigned char t);
/*
 * parseConst leaves its answer here rather than returning it.  A long
 * return travels in HL:DE and costs every call site the unpacking;
 * the value is consumed immediately after the call at both callers,
 * so a single cell is enough - even a re-entrant call would have
 * finished with it before the outer one stores its own.
 */
extern unsigned long constVal;
void parseConst(unsigned char priority);
extern void FreeExpr(struct expr *e);
extern struct expr *foldTree(struct expr *e);
extern unsigned short globalStrCtr;  /* string literal counter ("str") */

extern void statement(void);
extern void declaration(void);
struct name;
extern struct type *redeclOld;	/* type a reused entry already had */
extern struct local *capLocals(void);
extern struct local *mklocal(struct name *n);
extern void emitFuncPre(struct name *func);
extern void emitGlobalAsm(char *text);
extern void emitAsmStmt(char *text);
extern char *getAsmText(void);
extern void emitGv(struct name *var);
extern void emitExpr(struct expr *e);
extern void emitOperand(struct expr *e, struct type *t);
extern int cntCondLbls(struct expr *e);
extern void emitLabel(char *base, char *suffix);
extern void emitGoto(char *base, char *suffix);

/*
 * Moved out of p1stmt.h, where they sat beside the switch
 * tables for no reason but history: each of these is about the
 * thing this header declares, and leaving them there made every
 * caller take the statement machinery too.
 */
char isRegvar(struct expr *e);	/* takes a struct expr */

#endif
