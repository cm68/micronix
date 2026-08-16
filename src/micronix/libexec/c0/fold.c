/*
 * constant folding.
 *
 * Split from expr.c: the biggest source in the tree was the one
 * source cpp could no longer fit next to on the 64K machine, and
 * this is the half with a scheduled exit - folding moves into cpp
 * wholesale once sizeof lives there, and then this file simply
 * goes away.
 */
#include <stdlib.h>
#include <string.h>
#include "p1core.h"
#include "p1expr.h"
#include "p1type.h"
#include "p1name.h"

extern void freeNode(struct expr *e);

/*
 * Fold a single node if both operands are constants.
 * Returns folded CONST expr, or original expr if not foldable.
 */
struct expr *
foldNode(struct expr *e)
{
    unsigned lv, rv;
    int rel = 0, sgn;
    unsigned char op;
    struct expr *left, *right, *result;

    if (!e)
        return e;

    /*
     * A unary operator on a constant.  Only the parser folded these,
     * and it looks at the operand before the operand's own subtree
     * has been folded - so "~7" became a number where "~(A|B|C)" did
     * not, the three macros still being an OR tree at that moment.
     * pass2 has no form for the complement of a constant, so
     *
     *	f->_flag &= ~(_IOREAD|_IOWRT|_IONBF);
     *
     * in fclose emitted no code at all and every stream kept the
     * flags it was closed with.  The width discipline is the binary
     * fold's below: wrap to the result type, and leave longs alone.
     *
     * A width change is one of these.  The cast code relabels a
     * constant rather than wrapping it, but it asks the same question
     * at the same too-early moment: "(char)(799 & 0x7f)" was still an
     * AND tree when the cast looked, so it got a NARROW - and by the
     * time the AND folded, the NARROW was around a literal, which no
     * rule in pass2 names.  Nothing was emitted and the destination
     * kept whatever it held.  The wrap below IS the conversion, so
     * these need no arithmetic of their own.
     */
    if (e->left && !e->right && e->left->op == CONST &&
        (e->left->flags & E_CONST) &&
        (e->op == NEG || e->op == NOT || e->op == BANG ||
         e->op == NARROW || e->op == WIDEN || e->op == SEXT) &&
        e->type->size > 0 && e->type->size <= 2 &&
        !(e->type->flags & (TF_POINTER | TF_ARRAY | TF_FUNC))) {
        unsigned long uv;

        left = e->left;
        uv = left->v;
        if (e->op == NEG) uv = -uv;
        else if (e->op == NOT) uv = ~uv;
        else if (e->op == BANG) uv = !uv;
        left->type = e->type;
        left->v = uv;
        if (left->type->size == 1) {
            left->v &= 0xff;
            if (!(left->type->flags & TF_UNSIGNED) && (left->v & 0x80))
                left->v |= 0xffffff00L;
        } else {
            left->v &= 0xffff;
            if (!(left->type->flags & TF_UNSIGNED) && (left->v & 0x8000L))
                left->v |= 0xffff0000L;
        }
        left->flags = E_CONST | (e->flags & E_FUNARG);
        left->next = e->next;
        left->left = NULL;
        left->right = NULL;
        e->left = NULL;
        e->next = NULL;
        freeNode(e);
        return left;
    }

    if (!e->left || !e->right)
        return e;

    left = e->left;
    right = e->right;
    op = e->op;

    /*
     * x + (-y) is x - y, and saying so here saves pass2 from ever
     * meeting the other shape.  It has rules for subtracting a
     * register from another and none for adding a negated one, so
     *
     *	(-x) + (-y)
     *
     * was an expression it could not build - and in a longer sum,
     * where the association put a plain term on the left, it built
     * PART of it and dropped the negated term without a word:
     * "(-x) + (-y) + (-(x+y))" compiled clean and came out three
     * short.  Turning the add into a subtract costs nothing, removes
     * the negation entirely, and is what the machine would want in
     * any case: hl - de is one instruction sequence, hl + (0 - de)
     * is two.
     */
    if (op == PLUS && right && right->op == NEG && right->left) {
        struct expr *neg = right;

        e->op = MINUS;
        e->right = right = neg->left;
        neg->left = NULL;
        freeNode(neg);
        op = MINUS;
    }

    /*
     * (x + c1) - c2, and the rest of that family: put the constants
     * together so what reaches pass2 is one offset from x.  Written
     * out, "ybuf+256-5" arrived as a subtract of five from an add of
     * two hundred and fifty six, and no rule spells an address
     * computed that way against a register variable - while
     * "ybuf+251", the same address, compiles.  grep's -y guard is
     * written the long way.
     *
     * Only when the inner constant is on the right, which is where
     * the parser puts it for these, and only for the two operators
     * that reassociate: x - (y - c) is not x - y - c.
     */
    if ((op == PLUS || op == MINUS) && right && (right->flags & E_CONST) &&
        left && (left->op == PLUS || left->op == MINUS) &&
        left->right && (left->right->flags & E_CONST) &&
        left->left && left->left->type) {
        long c1 = (long)left->right->v;
        long c2 = (long)right->v;
        struct expr *inner = left;

        if (inner->op == MINUS)
            c1 = -c1;
        if (op == MINUS)
            c2 = -c2;
        e->op = PLUS;
        e->left = inner->left;
        right->v = (unsigned long)(c1 + c2);
        inner->left = NULL;
        FreeExpr(inner->right);
        inner->right = NULL;
        freeNode(inner);
        left = e->left;
        op = PLUS;
    }

    /*
     * A conditional whose test is constant is one of its arms.  This
     * was missing entirely, and nothing said so: a static initializer
     * is required to be constant, so an unfolded ?: left the whole
     * expression non-constant and the initializer quietly emitted
     * zero.  The preprocessor's folder hides it for short spellings -
     * it answers them before pass1 ever sees them - so it only showed
     * when an expression grew past the folder's save buffer and came
     * back here to be folded:
     *
     *	(l) | ((r) << 2) | ((d) << 4) | ((rlo) ? RP_SUBR : 0)
     *
     * in pass2's rule table, where every packed path byte came out
     * nought and the self-built compiler matched rules that were not
     * there.  The arms are already folded - foldTree works upwards -
     * so the answer is whichever one the test picks.
     */
    if (op == QUES && (left->flags & E_CONST) && right->op == COLON &&
        right->left && right->right) {
        struct expr *dead;

        if (left->v) {
            result = right->left;
            dead = right->right;
        } else {
            result = right->right;
            dead = right->left;
        }
        right->left = right->right = NULL;
        result->next = e->next;
        result->flags |= (e->flags & E_FUNARG);
        e->left = e->right = NULL;
        e->next = NULL;
        FreeExpr(left);
        FreeExpr(dead);
        freeNode(right);
        freeNode(e);
        return result;
    }

    /* Identity folding: x + 0 -> x, x * 1 -> x, etc. */
    if (right->flags & E_CONST) {
        rv = right->v;
        result = NULL;
        switch (op) {
        case PLUS:
        case MINUS:
        case OR:
        case XOR:
        case LSHIFT:
        case RSHIFT:
            if (rv == 0) result = left;  /* x +/- 0 -> x */
            break;
        case STAR:
        case DIV:
            if (rv == 1) result = left;  /* x * 1 -> x */
            break;
        case AND:
        case AMPER:
            /* identity only at word width: a LONG keeps its high
             * half, and dropping the mask handed it back whole -
             * "(long)v & 0xffff" simply vanished */
            if (rv == 0xffff && e->type->size <= 2)
                result = left;
            break;
        }
        if (result) {
            /*
             * The node being dropped may be one of a chain.  next is
             * how an argument list is threaded, and E_FUNARG is what
             * says a node is on one, so both have to come across to
             * whatever replaces it:
             *
             *	strncpy(n->name, name, 15);
             *
             * n->name is the first member, so its offset is zero,
             * "base + 0" folds to base - and the arguments after it
             * went out with the node that had been holding them.  The
             * call was emitted with one argument and the other two
             * were never pushed, so strncpy read its length from
             * whatever was on the stack and wrote that many bytes.
             */
            result->next = e->next;
            result->flags |= (e->flags & E_FUNARG);
            e->left = NULL;
            e->next = NULL;
            FreeExpr(right);
            e->right = NULL;
            freeNode(e);
            return result;
        }
    }

    /*
     * A string literal is not a constant of the kind this folds.
     *
     * It carries E_CONST because it is a constant address, but the
     * address is a label emitted later - strN - and kept in e->var;
     * e->v is zero and always was.  So folding arithmetic on one read
     * a number that was never the address, and the label went with it:
     *
     *	"MWDL"[1]	ld a,(1)	an absolute address of one
     *	"MWDL"[0]	ld a,(str1)	right, and only because adding
     *					zero returns the operand above
     *
     * Indexing at a variable, or through a pointer, never came here
     * and was always right, which is what kept this rare.  The boot
     * loader comparing a disk label against DL_MAGIC is where it was
     * found.
     */
    if (left->op == STRING || right->op == STRING)
        return e;

    /* Both operands must be constants for full folding */
    if (!(left->flags & E_CONST) || !(right->flags & E_CONST))
        return e;

    /*
     * Never fold longs.  The arithmetic here runs in int width, so
     * a long fold silently loses its high half on the Z80 - and
     * doing it long-wide instead put six hundred bytes of helper
     * calls into this hot path and three sources back over the
     * heap line.  The two builds agreeing on doing nothing beats
     * either doing something different.  Longs are rare, and pass2
     * places a long constant operand directly.
     */
    if (e->type && e->type->size > 2)
        return e;

    lv = left->v;
    rv = right->v;
    sgn = !(left->type->flags & TF_UNSIGNED) &&
          !(right->type->flags & TF_UNSIGNED);

    switch (op) {
    case PLUS:   lv += rv; break;
    case MINUS:  lv -= rv; break;
    case STAR:   lv *= rv; break;
    case DIV:    if (rv) lv /= rv; break;
    case MOD:    if (rv) lv %= rv; break;
    case LSHIFT: lv <<= rv; break;
    case RSHIFT: lv >>= rv; break;
    case AND:
    case AMPER:  lv &= rv; break;
    case OR:     lv |= rv; break;
    case XOR:    lv ^= rv; break;
    /*
     * Relations fold too, and have to: a comparison of two constants
     * reaches pass2 as a comparison of two constants, which matches no
     * rule and emits nothing.  The result is an int, not the type of
     * what was compared, so the type is fixed up below.
     *
     * lv and rv are unsigned, so the signed relations need casting
     * back.  Both operands being signed is what decides it: if either
     * is unsigned the usual conversions make the comparison unsigned.
     */
    case EQ:     lv = (lv == rv); rel = 1; break;
    case NEQ:    lv = (lv != rv); rel = 1; break;
    case LT:     lv = sgn ? ((long)lv <  (long)rv) : (lv <  rv); rel = 1; break;
    case GT:     lv = sgn ? ((long)lv >  (long)rv) : (lv >  rv); rel = 1; break;
    case LE:     lv = sgn ? ((long)lv <= (long)rv) : (lv <= rv); rel = 1; break;
    case GE:     lv = sgn ? ((long)lv >= (long)rv) : (lv >= rv); rel = 1; break;
    /*
     * && and || of two constants.  Folding them is worth a branch and
     * two labels, but the reason they are here is that pass2 could not
     * do anything with them: an operand that is still a NUMBER sets no
     * flags, so the short-circuit branched on whatever the previous
     * instruction left.  Like the relations, the answer is an int
     * whatever was tested.
     */
    case LAND:   lv = (lv && rv); rel = 1; break;
    case LOR:    lv = (lv || rv); rel = 1; break;
    default:
        return e;
    }

    /* Reuse left node as constant - carrying the chain, as above */
    left->op = CONST;
    left->v = lv;
    left->type = rel ? inttype : e->type;
    /*
     * Wrap the folded value to the width of its type.  Folding runs
     * in the host's arithmetic, which is wider than the target's:
     * without this, 3115*31 kept bits sixteen and up that no Z80
     * int ever holds, and a later cast or comparison read them as
     * real.  Wrapping here is also what makes a host-built c0 and
     * a self-hosted one fold identically.
     */
    if (left->type &&
        !(left->type->flags & (TF_POINTER | TF_ARRAY | TF_FUNC))) {
        if (left->type->size == 1) {
            left->v &= 0xff;
            if (!(left->type->flags & TF_UNSIGNED) && (left->v & 0x80))
                left->v |= 0xffffff00L;
        } else if (left->type->size == 2) {
            left->v &= 0xffff;
            if (!(left->type->flags & TF_UNSIGNED) && (left->v & 0x8000L))
                left->v |= 0xffff0000L;
        }
    }
    left->flags = E_CONST | (e->flags & E_FUNARG);
    left->next = e->next;
    left->left = NULL;
    left->right = NULL;
    FreeExpr(right);
    e->left = NULL;
    e->next = NULL;
    e->right = NULL;
    freeNode(e);
    return left;
}

/*
 * Walk expression tree bottom-up and fold constants.
 * Called before emitting to AST.
 */
struct expr *
foldTree(struct expr *e)
{
    struct expr **pp;

    if (!e)
        return NULL;
    e->left = foldTree(e->left);
    /* Walk right child and any next chain (function arguments) */
    for (pp = &e->right; *pp; pp = &(*pp)->next)
        *pp = foldTree(*pp);
    return foldNode(e);
}
