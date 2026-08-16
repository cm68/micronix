/*
 * generate expression trees
 */
#include <string.h>
#include "p1core.h"
#include "p1expr.h"
#include "p1type.h"
#include "p1name.h"
#include "p1lex.h"

extern struct expr *parsePrefix(void);
extern struct expr *parsePostfix(struct expr *e);

/*
 * Combined operator priority and flags table
 * Table covers 0x20-0x7f (96 bytes), indexed by (tok - 0x20)
 * Bits 0-3: priority (0-15), Bits 4-7: flags
 */
#define TF_ASN 0x10   /* assignment op: = += -= etc */
#define TF_CMP 0x20   /* comparison: < > <= >= == != */
#define TF_LOG 0x40   /* logical: && || */

#define P(p) (p)                     /* priority only */
#define F(f) (f)                     /* flags only */
#define PF(p,f) ((p) | (f))          /* priority + flags */

/*
 * Indexed by (token - 0x20) where tokens are from lexeme.h
 * Token values: STAR=36, PLUS=40, MINUS=41, DIV=43, MOD=44,
 * RSHIFT=45, LSHIFT=46, AND=47, OR=48, XOR=49, LAND=53, LOR=54,
 * EQ=60, NEQ=61, LE=62, LT=63, GE=64, GT=65, PLUSEQ=70..ASSIGN=80, QUES=90
 */
static unsigned char oppri[96] = {
/*0x20  32  33  34  35  STAR  37  38  39 */  0,0,0,0,P(3),0,0,0,
/*0x28  PLUS MINUS 42  DIV   MOD   RSHIFT LSHIFT AND */
        P(4),P(4),0,P(3),P(3),P(5),P(5),P(8),
/*0x30  OR    XOR   50  51  52  LAND           LOR            55 */
        P(10),P(9),0,0,0,PF(11,TF_LOG),PF(12,TF_LOG),0,
/*0x38  56  57  58  59  EQ             NEQ            LE             LT */
        0,0,0,0,PF(6,TF_CMP),PF(7,TF_CMP),PF(6,TF_CMP),PF(6,TF_CMP),
/*0x40  GE             GT             66  67  68  69  PLUSEQ         SUBEQ */
        PF(6,TF_CMP),PF(6,TF_CMP),0,0,0,0,PF(14,TF_ASN),PF(14,TF_ASN),
/*0x48  MULTEQ         DIVEQ          MODEQ          RSHIFTEQ       LSHIFTEQ       ANDEQ          OREQ           XOREQ */
        PF(14,TF_ASN),PF(14,TF_ASN),PF(14,TF_ASN),PF(14,TF_ASN),PF(14,TF_ASN),PF(14,TF_ASN),PF(14,TF_ASN),PF(14,TF_ASN),
/*0x50  ASSIGN         81  82  83  84  85  86  87 */
        PF(14,TF_ASN),0,0,0,0,0,0,0,
/*0x58  88  89  QUES  91  92  93  94  95 */
        0,0,P(13),0,0,0,0,0,
/*0x60  96  97  98  99  100 101 102 103 */  0,0,0,0,0,0,0,0,
/*0x68  104 105 106 107 108 109 110 111 */  0,0,0,0,0,0,0,0,
/*0x70  112 113 114 115 116 117 118 119 */  0,0,0,0,0,0,0,0,
/*0x78  120 121 122 123 124 125 126 127 */  0,0,0,0,0,0,0,0
};
#undef P
#undef F
#undef PF

#define OPPRI(t) ((unsigned char)(t) >= 0x20 && (unsigned char)(t) < 0x80 ? oppri[(t) - 0x20] : 0)

/* Assignment operators all have TF_ASN flag in oppri table */
#define IS_ASSIGN(t) (OPPRI(t) & TF_ASN)
#define IS_CMPLOG(t) (OPPRI(t) & (TF_CMP | TF_LOG))

/* assignment-op query for the split half - the table is private */
int
isasgn(unsigned char t)
{
    return IS_ASSIGN(t);
}

/*
 * Get binary operator precedence priority
 * Uses combined oppri[] table (bits 0-3 = priority)
 * Lexeme.h token values handled explicitly since they don't match ASCII
 */
unsigned char
binopPri(unsigned char t)
{
    /* COMMA is below table range */
    if (t == COMMA)
        return 15;
    /* Use table lookup for tokens in range 0x20-0x7f */
    if (t >= 0x20 && t < 0x80)
        return oppri[t - 0x20] & 0x0f;
    return 0;
}

struct expr *scaleptr(struct expr *e);

/*
 * The prefix arms, one worker apiece.  parsePrefix was seventeen
 * hundred instructions with eleven locals sharing one frame and one
 * register allocation; this compiler does no lifetime analysis, by
 * design, so locals that never overlap still fought for the same two
 * registers.  A function boundary is the lifetime analysis: each
 * arm's locals get the registers to themselves, and each frame holds
 * one arm's worth of spill instead of the union of all of them.
 */

struct expr *
parseExpr(unsigned char pri)
{
	unsigned char op, p, is_assignment;
	struct expr *e;
	struct expr *e1, *e2, *e3, *e4;
	struct type *assign_type;
	struct var *vp;

	assign_type = NULL;
	is_assignment = 0;

	/* Phase 1: just consume tokens, don't build tree */
	if (phase == 1) {
		skipExpr(pri);
		hlClose();	/* one expression is all HL survives */
		return NULL;
	}

	e = parsePrefix();
	if (!e) return 0;

	e = parsePostfix(e);
	if (!e) return 0;

    /*
     * the recursive nature of this expression parser will have exhausted
     * the unary operators and terminals by this point. now we have postfix
     * and binary operators to deal with
     */
    while (1) { // binary operators
        p = binopPri(cur.type);
        if (p == 0) {
            // not a binary operator, we're done
            break;
        }
        if (pri != 0 && p >= pri) {
            /*
             * operator has same or lower precedence
             * stop parsing at this level
             * (pri == 0 means PRI_ALL, so we parse all operators
             * regardless of precedence)
             */
            break;
        }

        // we have a binary operator with higher precedence (lower p value)
        op = cur.type;
        gettoken();

        /*
         * Special handling for ternary conditional operator:
         * condition ? true : false
         */
        if (op == QUES) {
            e1 = e;  /* condition */

            // Parse the true expression
            e2 = parseExpr(0);  /* true_expr */

            // Expect and consume COLON
            expect(COLON, ER_E_SP);

            /*
             * The false arm is a conditional-expression, not a full
             * one: a nested ?: belongs to it and a comma does not.
             * Parsed at priority 0 it took everything, so the comma
             * separating the next argument of a call read as the comma
             * operator and the whole rest of the argument list became
             * part of the else branch.
             *
             *	printf("%s:%d: %s\n", f ? f : "?", line, msg)
             *
             * came through as a call of two arguments whose second was
             * "f ? f : ("?", line, msg)", and printf was handed one
             * value where it wanted three.  Every diagnostic gripe()
             * printed had this shape.
             */
            e3 = parseExpr(OP_PRI_COMMA);  /* false_expr */

            // Build tree: QUES(condition, COLON(true_expr, false_expr))
            e4 = mkbin(COLON, e2, e3, NULL);
            /*
             * The type is the arms' COMMON type, not the true arm's.
             * A small constant is typed by its value, so
             * "g ? 225 : 11104" wore ubyte - the true arm's coat -
             * and the conversion to the destination then threw away
             * the high byte of whichever arm had one: the false arm
             * came back as 96.
             */
            {
                struct type *tt = e2 ? e2->type : NULL;

                if (e3->type &&
                    (!tt || e3->type->size > tt->size))
                    tt = e3->type;
                e = mkbin(QUES, e1, e4, tt);
            }

            /* Skip the rest of the loop and continue with next operator */
            continue;
        }

        /*
         * for assignment and compound assignments, unwrap DEREF
         * from left side to get lvalue address
         * Track the actual type being assigned
         */
        assign_type = NULL;
        is_assignment = IS_ASSIGN(op);

        if (is_assignment) {
            if (e->op == DEREF && isaggr(e->type)) {
                /* a struct is not an lvalue - take its address */
                gripe(ER_E_AG);
                FreeExpr(parseExpr(p));
                return e;
            }
            if (e->op == DEREF) {
                assign_type = unwrapDeref(&e);
            } else if (e->op == BFEXTRACT) {
                // Bitfield assignment - save info and change to BFASSIGN
                assign_type = e->type;
                vp = e->var;
                e1 = e;
                e = e->left;
                e1->left = NULL;
                freeNode(e1);
                e->var = vp;
                if (op == ASSIGN)
                    op = BFASSIGN;
            } else {
                /*
                 * Assignment requires an lvalue
                 * (dereference or bitfield)
                 */
                gripe(ER_E_LV);
                /*
                 * Skip this operator: parse and discard right side,
                 * then return left side
                 */
                FreeExpr(parseExpr(p));  // Parse and discard right side
                return e;  // Return left side unchanged
            }
        }

        /*
         * Parse right side based on associativity:
         * - For right-associative operators (assignments), use
         *   precedence 0 to allow chaining
         * - For left-associative operators, use precedence p to prevent
         *   same-precedence from nesting right
         */
        vp = e->var;
        e = mkexpr(op, e);
        if (is_assignment) {
            /*
             * Right-associative, so the right hand side has to admit
             * another assignment for "a = b = c" - but not a comma.
             * The right hand side of an assignment is an
             * assignment-expression, so "a = b, c" is "(a = b), c",
             * and in an argument list the comma separates arguments.
             * Parsing this at 0 swallowed the comma, and
             * "f(k += 4, l++)" became a call with one argument.
             */
            e->right = parseExpr(OP_PRI_COMMA);
        } else {
            /*
             * Left-associative: parse at same precedence to prevent
             * (a + b) + c from becoming a + (b + c)
             */
            e->right = parseExpr(p);
        }
        // For BFASSIGN, restore member info (bitoff, width)
        if (op == BFASSIGN && vp) {
            e->var = vp;
        }

        /* Determine result type */
        if (e->right) {
            if (is_assignment && assign_type) {
                if (!ptrcompat(assign_type, e->right->type))
                    gripe(ER_E_PT);
                e->type = assign_type;
            }
            else if (IS_CMPLOG(op))
                e->type = uchartype;
            /*
             * A pointer against an integer keeps the pointer type,
             * whatever the two widths are.  Picking the wider one made
             * "p + n" a long when n was, and the pointer-ness was gone:
             * the subscript path then found neither TF_POINTER nor
             * TF_ARRAY to take an element type from, left the node with
             * no type at all, and the next thing to look at it died
             * without a word.  A pointer is also the wider of the two
             * against a short, which is why only long showed it.
             */
            else if ((e->left->type->flags & (TF_POINTER|TF_ARRAY)) &&
                     !(e->right->type->flags & (TF_POINTER|TF_ARRAY)))
                e->type = e->left->type;
            else if ((e->right->type->flags & (TF_POINTER|TF_ARRAY)) &&
                     !(e->left->type->flags & (TF_POINTER|TF_ARRAY)))
                e->type = e->right->type;
            else if (e->left->type->size >= e->right->type->size)
                e->type = e->left->type;
            else
                e->type = e->right->type;

            /*
             * The integer promotions.  Anything narrower than an int
             * is an int the moment it is operated on, so the wider of
             * the two operands is only the answer once both are at
             * least that wide.  Stopping there made "c * 100" char
             * arithmetic: seven hundred kept its low byte and came out
             * as a hundred and eighty-eight.
             *
             * This is only where the width is decided.  outast.c
             * narrows it again wherever the result is stored somewhere
             * that cannot tell the difference - which is what keeps
             * byte arithmetic byte-wide when it is safe, and is why
             * the comment there calls itself the as-if rule standing
             * in for these promotions.
             */
            if (!is_assignment && !IS_CMPLOG(op) &&
                e->type->size > 0 &&
                e->type->size < inttype->size &&
                !(e->type->flags & (TF_POINTER | TF_ARRAY)))
                e->type = inttype;

            /*
             * Pointer arithmetic counts in elements, not bytes.  The
             * subscript path has always scaled - "p[2]" is right -
             * but the same sum written "p + 2" came through here and
             * did not, so it landed two bytes along instead of two
             * elements.  Done after the result type is settled,
             * because a difference of two pointers is an int and has
             * to say so.
             */
            if (op == PLUS || op == MINUS)
                e = scaleptr(e);
            /*
             * The compound forms count in elements too - "p += n" is
             * "p = p + n" - but they cannot go through scaleptr: the
             * left has had its DEREF taken off to leave an address, so
             * every compound assignment looks like a pointer there and
             * "arr[2] += 50" would scale the 50.  The type being
             * assigned is the one to ask, and unwrapDeref saved it.
             *
             * Only the plain operators were scaled at all, so doprnt's
             * varargs walk - "a += len" over an int * - advanced one
             * byte per conversion, and printf("%d %d") read its second
             * number a byte into the first.
             */
            else if ((op == PLUSEQ || op == SUBEQ) && assign_type &&
                     (assign_type->flags & (TF_POINTER | TF_ARRAY)) &&
                     assign_type->sub->size > 1 &&
                     e->right &&
                     !(e->right->type &&
                       (e->right->type->flags & (TF_POINTER | TF_ARRAY)))) {
                int esz = assign_type->sub->size;

                if (e->right->flags & E_CONST) {
                    e->right->v *= esz;
                } else {
                    e->right = mkbin(STAR, e->right,
                        mkexprI(CONST, 0, inttype,
                            (unsigned long)esz, E_CONST), inttype);
                }
            }
        } else {
            e->type = e->left->type;
        }
    }
    return e;
}

/*
 * Scale the integer side of pointer arithmetic by the element size.
 *
 * "p + n" advances by n elements, so the n has to be multiplied by
 * what p points at.  A difference between two pointers is the other
 * way round: the byte difference is divided.  An element size of one
 * needs neither, which is why char pointers always looked right.
 */
struct expr *
scaleptr(struct expr *e)
{
    struct type *lt = e->left->type, *rt = e->right->type;
    int lp = (lt->flags & (TF_POINTER|TF_ARRAY)) != 0;
    int rp = (rt->flags & (TF_POINTER|TF_ARRAY)) != 0;
    struct type *pt = lp ? lt : rt;
    struct expr *n, *scaled, **side;
    int size;

    if (!lp && !rp)
        return e;                       /* ordinary arithmetic */

    /*
     * An address is sixteen bits, so a long index into one is not
     * long arithmetic: it is narrowed and added.  The sum was typed
     * as the pointer above, but nobody put the conversion in the
     * tree, so pass2 met an address add with one long operand, had
     * no rule spelling +(H,H) at two widths, and emitted nothing at
     * all - "flags = buf[off++]" with a long off read address zero
     * and the store went nowhere.  Before the byte-pointer return
     * below, which is the case that showed it.
     */
    if (lp != rp) {
        side = lp ? &e->right : &e->left;
        *side = narrowidx(*side);
    }

    /*
     * Arithmetic on a pointer moves by the size of what it points at,
     * so a pointer to something with no size cannot do arithmetic.
     * K&R's additive operators want a pointer to an OBJECT type, and
     * void is not one - "v + 1" on a void * is a GNU extension and
     * not C.  This did it anyway: the size came out 0, the index was
     * multiplied by it, and the addition moved nowhere at all, with
     * nothing said.  A generic pointer has to be cast to what it
     * points at before it can be walked.
     */
    if (incomplete(pt->sub)) {
        gripe(ER_E_IC);
        return e;
    }

    if (!pt->sub || (size = pt->sub->size) == 1) {
        /*
         * A byte needs no scaling - but a difference is still an int
         * and not a pointer, and saying so is not optional.  The next
         * operator out asks whether its operands are pointers, and a
         * difference that still claims to be one sends it down the
         * pointer-arithmetic path: the long on the other side gets
         * narrowed to sixteen bits, because that is what an index
         * into an address ought to be.
         *
         *	t = nd - (&buf[33] - b);	char *b
         *
         * came out zero with nothing said.  char pointers only - any
         * wider element leaves a real division behind, and the
         * division carries the int type that this early return does
         * not.
         */
        if (lp && rp && e->op == MINUS)
            e->type = inttype;
        return e;                       /* a byte needs no scaling */
    }

    if (lp && rp) {
        /*
         * One pointer less another is how many elements apart they
         * are, so the byte difference is divided.  Anything else
         * between two pointers is not arithmetic and is left alone.
         */
        if (e->op != MINUS)
            return e;
        n = mkexprI(CONST, 0, inttype, (unsigned long)size, E_CONST);
        return mkbin(DIV, e, n, inttype);
    }

    /* the integer is whichever side is not the pointer */
    side = lp ? &e->right : &e->left;
    if ((*side)->flags & E_CONST) {
        (*side)->v *= size;
        return e;
    }
    n = mkexprI(CONST, 0, inttype, (unsigned long)size, E_CONST);
    scaled = mkbin(STAR, *side, n, inttype);
    *side = scaled;
    return e;
}

/*
 * Parse and evaluate a constant expression
 * Used for array sizes, case values, enum values, etc.
 */
unsigned long constVal;

void
parseConst(unsigned char token)
{
	struct expr *e;
	unsigned char save_phase;

	/* Parse expression, stop before comma (for enum values) */
	save_phase = phase;
	phase = 2;
	e = parseExpr(15);
	phase = save_phase;

	constVal = 0;
	if (!e) {
		gripe(ER_C_CE);
		return;
	}
	e = foldTree(e);	/* SECSIZE+2 style bounds */
	if (!(e->flags & E_CONST)) {
		gripe(ER_C_CE);
	} else {
		/*
		 * Into the static before the node is freed.  The local that
		 * used to carry it across the FreeExpr call cost four frame
		 * stores and four loads; memory to memory is two of each,
		 * and the return machinery is gone entirely.
		 */
		constVal = e->v;
	}
	FreeExpr(e);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
