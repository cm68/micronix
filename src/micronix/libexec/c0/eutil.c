/*
 * expression node constructors and shared helpers, split from
 * expr.c so no single translation unit carries the whole
 * expression machinery.
 */

#include <stdlib.h>
#include <string.h>
#include "p1core.h"
#include "p1expr.h"
#include "p1type.h"
#include "p1name.h"
#include "p1lex.h"

#ifdef DEBUG
int exprAllocCnt = 0;
int exprCurCnt = 0;
int exprHighWater = 0;
#endif

/* Check if token is a type keyword - exported for declare.c */
int
isTypeToken(unsigned char t)
{
    /* Type keywords are in range 128-138 (INT, CHAR, ... VOID);
     * ENUM never reaches pass1 - cpp lowers enums to unsigned char */
    return (t >= INT && t <= VOID);
}

/*
 * Counter for generating synthetic string literal names (prefix "str")
 * Exported so it can be reset between phases.
 *
 * Not a char: plain char is signed here, so the 129th string in a file
 * was named "str-128" and the assembler stopped at a label with a minus
 * sign in it.  The ones after that reused names the earlier strings
 * already had, which would have been worse had it got that far.
 */
unsigned short globalStrCtr = 0;

/*
 * Create a new expression tree node
 *
 * Allocates and zero-initializes an expression structure, setting the
 * operator and left child pointer. This is the basic expression node
 * allocator used throughout the parser.
 *
 * Parameters:
 *   op   - Operator token (e.g., PLUS, DEREF, CONST)
 *   left - Left child expression (can be NULL)
 *
 * Returns:
 *   Pointer to newly allocated and initialized expression node
 */
struct expr *
mkexpr(unsigned char op, struct expr *left)
{
	struct expr *e;

	e = (struct expr *)galloc(sizeof(*e));
	e->op = op;
	e->left = left;
#ifdef DEBUG
	exprAllocCnt++;
	exprCurCnt++;
	if (exprCurCnt > exprHighWater)
		exprHighWater = exprCurCnt;
#endif
	return e;
}

/*
 * Create expression node with type, value, and flags initialized
 *
 * Convenience wrapper around mkexpr() that also sets the type, value (v),
 * and flags fields. This reduces code duplication for common expression
 * construction patterns, especially for constants and typed operations.
 *
 * Parameters:
 *   op    - Operator token
 *   left  - Left child expression (can be NULL)
 *   type  - Type of the expression (pass NULL to skip setting)
 *   v     - Constant value (for CONST nodes) or other numeric data
 *   flags - Expression flags (E_CONST, E_RESOLVED, etc.)
 *
 * Returns:
 *   Pointer to newly allocated expression with all fields initialized
 */
struct expr *
mkexprI(unsigned char op, struct expr *left, struct type *type,
	 unsigned long v, int flags)
{
	struct expr *e;

	e = mkexpr(op, left);
	if (type) {
		e->type = type;
	}
	e->v = v;
	e->flags = flags;
	return e;
}

/*
 * Build a complete binary node: op over left and right with the type
 * filled in.  Six places built one by hand, several statements each,
 * every one a store through a frame pointer.  One call is cheaper.
 */
struct expr *
mkbin(unsigned char op, struct expr *l, struct expr *r, struct type *t)
{
    struct expr *e = mkexpr(op, l);

    e->right = r;
    e->type = t;
    return e;
}

/*
 * Free just an expression node without its children
 * Used when restructuring trees (e.g., unwrapping DEREF)
 */
void
freeNode(struct expr *e)
{
	if (!e) return;
	free(e);
#ifdef DEBUG
	exprCurCnt--;
#endif
}

/*
 * An address is sixteen bits, so a long index into one is not long
 * arithmetic: it is narrowed and added.  Nothing put that conversion
 * in the tree, so pass2 met an address add with one long operand,
 * had no rule spelling it at two widths, and emitted nothing at all -
 * "flags = buf[off++]" with a long off read address zero and stored
 * nowhere.  A constant index needs no special case: foldNode's
 * unary arm folds the NARROW around it away.
 */
struct expr *
narrowidx(struct expr *e)
{
    struct expr *n;

    if (!e || !e->type || e->type->size <= inttype->size ||
        (e->type->flags & TF_AGGREGATE))
        return e;
    n = mkexpr(NARROW, e);
    n->type = inttype;
    return n;
}

/*
 * Unwrap a DEREF node, returning the child and freeing the DEREF
 * Saves the dereferenced type before unwrapping
 * Returns: the unwrapped type (what was e->type before unwrapping)
 */
struct type *
unwrapDeref(struct expr **ep)
{
	struct expr *e = *ep;
	struct type *t;
	if (!e || e->op != DEREF) return e ? e->type : (struct type *)0;
	t = e->type;
	*ep = e->left;
	e->left = NULL;
	freeNode(e);
	return t;
}

/*
 * Free an expression tree recursively
 *
 * Performs a post-order traversal of the expression tree, freeing all
 * child nodes before freeing the parent. This ensures proper memory
 * deallocation without leaks.
 *
 * Note: Does not free any associated type structures (those are managed
 * in the global type cache) or symbol name strings (those are managed
 * by the name table).
 *
 * Parameters:
 *   e - Root of expression tree to free (NULL-safe)
 */
void
FreeExpr(struct expr *e)
{
	if (!e)
		return;
	FreeExpr(e->left);
	FreeExpr(e->right);
	FreeExpr(e->next);
	/* For STRING expressions with synthetic name, free the name and init expr */
	if (e->op == STRING && e->var) {
		struct name *strname = (struct name *)e->var;
		FreeExpr(strname->u.init);
		free((void *)e->v);
		free(strname);
	}
	free(e);
#ifdef DEBUG
	exprCurCnt--;
#endif
}

/*
 * Whether the first argument is still in HL when the body reads it.
 *
 * It arrives there, and the body's first expression is what takes HL
 * away, so a parameter named anywhere in that first expression is
 * still reachable - as the operand of the very first load, as the
 * index added to a base, as the argument pushed for the first call.
 * One expression later it is not: something has computed into HL and
 * the parameter has to come back off the frame.
 *
 * So the window is one expression wide, and a label shuts it early.
 * cpp lowers a loop to a label above its test, and under one there is
 * no first expression - only a first iteration, with HL rebuilt on
 * every pass through.  stfind and tdfind are that shape, naming their
 * parameter once, in a test that runs once per list element.
 *
 * Phase 1 records and phase 2 asks, which works because process()
 * runs the two of them over one function at a time.
 */
unsigned short hlArgSym;
static unsigned char hlShut;

/* start of a function body - nothing has taken HL yet */
void
hlOpen(void)
{
    hlArgSym = 0;
    hlShut = 0;
}

/* the window is over: an expression has been and gone, or a label */
void
hlClose(void)
{
    hlShut = 1;
}

/* a parameter was named while HL could still be holding it */
static void
hlArg(unsigned short id)
{
    if (!hlShut && !hlArgSym)
        hlArgSym = id;
}

/*
 * Skip an expression without building a tree (phase 1)
 * Consumes tokens to stay synchronized with the lexer.
 * Mirrors parseExpr structure but doesn't allocate.
 */
void
skipExpr(unsigned char pri)
{
    static unsigned char szskip;
    unsigned char p, is_assign;
    struct name *np = NULL;
    char namebuf[32];
    char *symname;
    int size;

    /* Handle prefix/primary */
    switch (cur.type) {
    case NUMBER:
    case INUMBER:
    case LNUMBER:
        gettoken();
        break;

    case STRING:
        /* Phase 1: emit string literal data */
        size = (unsigned char)cur.v.str[0];
        symname = (char *)cur.v.str + 1;
        fmtstr(namebuf, "str%d", globalStrCtr++);
        setSeg(SEG_TEXT);
        asmLabel(namebuf);
        asmDbStr((unsigned char *)symname, size);
        gettoken();
        while (cur.type == STRING)
            gettoken();
        break;

    case SYM:
        np = findName(cur.v.id, 0);
        if (np && np->kind == kfunarg)
            hlArg(cur.v.id);
        if (np && np->level > 1 && np->kind != kelem && !szskip &&
            !(np->type->flags & (TF_FUNC|TF_ARRAY))) {
            if (np->w.r.ref_count < 255)
                np->w.r.ref_count++;
        }
        gettoken();
        break;

    case LPAR:
        gettoken();
        if (isCastStart()) {
            parseTypeName();
            expect(RPAR, ER_E_SP);
            skipExpr(OP_PRI_MULT - 1);
        } else {
            skipExpr(0);
            expect(RPAR, ER_E_SP);
        }
        break;

    case AND:
        /*
         * Address-of, in the pass that decides which locals get a
         * register.  A variable whose address is taken cannot live in
         * one, and phase 2 finds that out too late: allocation happens
         * as the function header is emitted, before a statement has
         * been looked at.  So the flag is set here, where it is still
         * in time to be believed - "p = &v" used to leave v in BC and
         * compile as "p = v".
         *
         * Explicitly register is not a thing to work around but a
         * thing to refuse: it has no address to take.
         */
        gettoken();
        if (cur.type == SYM &&
            next.type != ARROW && next.type != LBRACK) {
            /*
             * Only a bare &v pins v itself.  "&fc->name[8]" is the
             * address of something fc points AT - fc is merely read,
             * and refusing it broke every K&R source that walks a
             * register struct pointer the day parameters started
             * honoring the keyword.  "&s.x" stays strict: the dot
             * means the object's own storage.
             */
            np = findName(cur.v.id, 0);
            /*
             * An undeclared name has none, and this is phase 1 - it
             * runs before the phase that says so, so dereferencing
             * the answer here turned "&undeclared" into a segfault
             * with no diagnostic from anything.  Nothing to pin if
             * there is no object; leave it to phase 2 to report.
             */
            if (np && np->level > 1) {
                if (np->sclass & SC_REGISTER)
                    gripe(ER_E_RA);
                else
                    np->w.r.addr_taken = 1;
            }
        }
        skipExpr(OP_PRI_MULT - 1);
        break;

    case MINUS:
    case TWIDDLE:
    case BANG:
    case STAR:
    case INCR:
    case DECR:
        gettoken();
        skipExpr(OP_PRI_MULT - 1);
        break;

    case SIZEOF:
        /*
         * The operand is never evaluated, so a name inside it is
         * not a reference the register allocator should weigh.
         * The preprocessor folds most of these away entirely, and
         * an allocation that depends on whether a sizeof survived
         * to this pass would flap.
         */
        gettoken();
        szskip++;
        if (cur.type == LPAR) {
            gettoken();
            if (isCastStart()) {
                parseTypeName();
                expect(RPAR, ER_E_SP);
            } else {
                skipExpr(0);
                expect(RPAR, ER_E_SP);
            }
        } else {
            skipExpr(OP_PRI_MULT - 1);
        }
        szskip--;
        break;

    default:
        return;
    }

    /* Handle postfix operators */
    while (cur.type == LPAR || cur.type == LBRACK || cur.type == DOT ||
           cur.type == ARROW || cur.type == INCR || cur.type == DECR) {
        if (cur.type == LBRACK) {
            gettoken();
            skipExpr(0);
            expect(RBRACK, ER_E_SP);
        } else if (cur.type == LPAR) {
            gettoken();
            if (cur.type != RPAR) {
                skipExpr(OP_PRI_COMMA);
                while (cur.type == COMMA) {
                    gettoken();
                    skipExpr(OP_PRI_COMMA);
                }
            }
            expect(RPAR, ER_E_SP);
        } else if (cur.type == DOT || cur.type == ARROW) {
            /* Track aggregate ref for register allocation */
            if (np && np->level > 1 && np->w.r.agg_refs < 255)
                np->w.r.agg_refs++;
            gettoken();
            if (cur.type == SYM)
                gettoken();
        } else {
            gettoken();
        }
    }

    /* Handle binary operators */
    while (1) {
        p = binopPri(cur.type);
        if (p == 0 || (pri != 0 && p >= pri))
            break;
        if (cur.type == QUES) {
            gettoken();
            skipExpr(0);
            expect(COLON, ER_E_SP);
            /* as parseExpr: the false arm stops at a comma, and the
             * two phases have to walk the tokens the same way */
            skipExpr(OP_PRI_COMMA);
        } else {
            is_assign = isasgn(cur.type);
            gettoken();
            skipExpr(is_assign ? 0 : p);
        }
    }
}

/*
 * Process increment/decrement (prefix or postfix)
 */
struct expr *
mkIncDec(struct expr *operand, unsigned char inc_op, unsigned char is_postfix)
{
    struct type *value_type;
    struct expr *e;

    if (!operand || operand->op != DEREF) {
        gripe(ER_E_LV);
        FreeExpr(operand);
        return NULL;
    }
    value_type = unwrapDeref(&operand);
    e = mkexpr(inc_op, operand);
    e->type = value_type;
    /*
     * The step, settled here.  It used to be worked out at emission
     * time from type->sub->size, which is right until something
     * retypes the node first - and an ordered comparison does exactly
     * that, cmpunsigned() replacing a pointer operand's type with
     * unsigned short so the compare goes unsigned.  So
     *
     *		while (++p < lim)
     *
     * reached the emitter with no element type left to ask about and
     * stepped the pointer by one byte.  No diagnostic; the loop ran
     * the wrong number of times and the pointer walked through the
     * middles of its elements.
     *
     * A step is a property of the operation and of the type it was
     * written against, so it is recorded when that type is in hand
     * and nobody downstream has to reconstruct it.
     */
    e->v = 1;
    if (value_type && (value_type->flags & TF_POINTER) && value_type->sub)
        e->v = value_type->sub->size;
    if (is_postfix)
        e->flags |= E_POSTFIX;
    return e;
}

/* Check if type is scalar (not pointer/array/func/aggregate) */
#define IS_SCALAR(t) (!((t)->flags & (TF_POINTER|TF_ARRAY|TF_FUNC|TF_AGGREGATE)))

/*
 * A struct or union has no value here: it cannot be assigned, passed
 * or returned.  Its address is the only handle on it, which is what
 * the member operators work through.
 *
 * This has to be diagnosed rather than left alone.  Nothing downstream
 * knows an aggregate from a scalar of the same size - the width is
 * picked from the byte count, so a four byte struct became a long and
 * anything of another size fell to the default and copied two bytes,
 * quietly losing the rest.
 */
int
isaggr(struct type *t)
{
	return t && (t->flags & TF_AGGREGATE) &&
	    !(t->flags & (TF_POINTER | TF_ARRAY | TF_FUNC));
}

/*
 * Assigning one pointer to another only means anything when they point
 * at the same thing.  Arrays decay, so an array of T and a pointer to
 * T count as the same here, and void goes with anything.
 *
 * Only fires when both sides are pointers, so pointer arithmetic like
 * "p += 4" is left alone.
 */
int
ptrcompat(struct type *lt, struct type *rt)
{
	struct type *a, *b;

	if (!lt || !rt)
		return 1;
	if (!(lt->flags & (TF_POINTER | TF_ARRAY)) ||
	    !(rt->flags & (TF_POINTER | TF_ARRAY)))
		return 1;
	a = lt->sub;
	b = rt->sub;
	if (!a || !b || a == b)
		return 1;
	if (a->size == 0 || b->size == 0)
		return 1;			/* void * */
	/* a function pointer's sub is its return type - leave those */
	if ((a->flags | b->flags) & TF_FUNC)
		return 1;
	if (a->size != b->size)
		return 0;
	if ((a->flags & TF_UNSIGNED) != (b->flags & TF_UNSIGNED))
		return 0;
	/* aggregates are not interned, so two of them are two types */
	if ((a->flags | b->flags) & TF_AGGREGATE)
		return 0;
	return 1;
}


/*
 * Parse an expression using precedence climbing algorithm
 *
 * Recursive descent parser for C expressions that implements operator
 * precedence using the precedence climbing method. This function handles:
 *   - Primary expressions (constants, variables, strings)
 *   - Prefix operators (unary -, ~, !, *, &, ++, --, sizeof)
 *   - Binary operators (arithmetic, logical, bitwise, comparison)
 *   - Postfix operators (++, --, [], (), ., ->)
 *   - Ternary conditional (? :)
 *   - Type casts
 *   - Function calls
 *   - Constant folding during parsing
 *   - Type conversions and checking
 *
 * The precedence climbing works by:
 *   1. Parse left operand (primary or prefix expression)
 *   2. While next token is binary operator with priority >= current priority:
 *      - Recursively parse right operand with operator's priority
 *      - Combine into binary expression tree
 *   3. Handle postfix operators (highest precedence)
 *
 * Lower priority numbers bind tighter (higher precedence). Passing priority 0
 * parses any expression. Passing higher priorities stops at lower-precedence
 * operators, enabling recursive parsing of right operands.
 *
 * Parameters:
 *   pri - Minimum operator priority to parse (0 = parse any expression,
 *         higher values stop at lower-precedence operators)
 *
 * Returns:
 *   Expression tree root, or NULL on parse error
 */
/*
 * parseExpr is split into parsePrefix / parsePostfix / the binary
 * precedence climb below, mostly so each piece stays small enough
 * for the hitech optimizer's fixed arenas.
 *
 * parsePrefix - terminals, prefix/unary operators, casts, sizeof.
 */
