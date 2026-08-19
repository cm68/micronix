/*
 * prefix and postfix expression parsing.
 *
 * Split from expr.c for the same reason fold.c was: the biggest
 * source in the tree has to fit through the compiler ON the target,
 * and cpp's tables for one translation unit are paid per unit.
 */
#include <stdlib.h>
#include "p1core.h"
#include "p1expr.h"
#include "p1type.h"
#include "p1name.h"
#include "p1lex.h"

/* string literal: refer to the strN emitted in phase 1 */
struct expr *
pfxString(void)
{
    struct name *np;
    struct expr *e;

    np = (struct name *)galloc(sizeof(struct name));
    np->id = SYNTH + globalStrCtr++;
    np->type = getType(TF_POINTER, chartype, 0);
    np->kind = kvar;
    np->level = 1;
    e = mkexprI(STRING, 0, np->type, 0, E_CONST);
    e->var = (struct var *)np;
    gettoken();
    return e;
}

/* Symbol reference - SYM = address
 * For variables: wrap in DEREF to get value
 * For functions: return address (decay to function pointer)
 */
struct expr *
pfxSym(void)
{
    struct expr *e, *e1;
    struct type *tp;
    struct name *np;
    unsigned int uofs;
    unsigned short symid;

    /* Save the id before gettoken() overwrites cur.v.id */
    symid = cur.v.id;

    np = findName(symid, 0);

    /* Peek at next token to enable implicit function declarations */
    gettoken();

    if (!np) {
        /* Undefined symbol */
        /* K&R extension: if followed by '(', implicitly declare as
         * function returning int */
        if (cur.type == LPAR) {
            /* Create implicit function declaration: int name() */
            tp = (struct type *)permalloc(sizeof(struct type));
            tp->flags = TF_FUNC;
            tp->sub = inttype;  /* Return type: int */
            tp->elem = NULL;    /* No parameter info */

            np = (struct name *)galloc(sizeof(struct name));
            /* Initialize in struct field order */
            np->id = symid;
            np->type = tp;
            /* chain set by addName */
            np->kind = kvar;
            np->level = 1;  /* Global scope */
            /* is_tag = 0 from calloc */
            np->sclass = SC_EXTERN;

            np = addName(np);
            /*
             * At file scope, which is what the level above asks for
             * and what addName overwrote with the level we happen to
             * be standing in.  An implicit declaration is of an
             * external name; it does not belong to the block that
             * first mentioned it, and being left there meant it died
             * at the end of that function.
             *
             * addName only reaches namesAdd when the name is new, and
             * it is: this arm runs because findName found nothing at
             * any level.  So this is the entry just made.
             */
            np->level = 1;

#ifdef DEBUG
            if (VERBOSE(V_SYM)) {
                fdprintf(2, "Implicit declaration: int %s()\n", nameOf(symid));
            }
#endif
        } else {
            /* Not a function call - report error */
#ifdef DEBUG
            fdprintf(2, "bad op (not fn): %d sym=%s\n", cur.type,
                     nameOf(symid));
#endif
            gripe(ER_E_UD);
            return mkexprI(CONST, 0, inttype, 0, 0);
        }
    }

    if (np->kind == kelem) {
        /* Enum constant: treat as integer constant.
         * uofs intermediate: zc3 miscompiles the direct
         * uchar -> ulong argument promotion (clobbers the
         * loaded byte), yielding 0 for every enum. */
        uofs = np->w.m.offset;
        e = mkexprI(CONST, 0, inttype, uofs, E_CONST);
    } else {
        tp = np->type;
        e1 = mkexprI(SYM, 0, tp, 0, 0);
        e1->var = (struct var *)np;

        // Functions and arrays decay to pointers (addresses)
        // Only wrap non-functions in DEREF to get their value
        if (tp->flags & (TF_FUNC | TF_ARRAY))
            e = e1;  // address forms decay to the address
        else
            e = mkexprI(DEREF, e1, tp, 0, 0);  // Variable: wrap in DEREF
    }
    /* Note: gettoken() already called above for lookahead */
    return e;
}

/* parenthesized expression or type cast, the LPAR consumed */
struct expr *
pfxCast(void)
{
    struct expr *e, *e1;
    struct type *tp;

    /* Check if this is a type cast: (type)expr */
    if (isCastStart()) {
        /* Parse the type name */
        tp = parseTypeName();
        expect(RPAR, ER_E_SP);

        /* Parse the expression being cast */
        /* Cast has unary precedence */
        e1 = parseExpr(OP_PRI_MULT - 1);

        /*
         * A cast that only renames the type can change it in
         * place, and most do.  One that makes the value narrower
         * cannot: the value is somewhere, and where its low half
         * sits depends on how wide it was.  A long lives in HL:DE
         * with the low word in DE, so "(int)f()" that just
         * relabelled the call as an int left pass2 reading HL -
         * the high word - and there was nothing left in the tree
         * to say otherwise.
         *
         * NARROW says it explicitly.  It is unary and has been in
         * the opcode table and the pretty-printer all along; only
         * nobody emitted it.
         */
        if (e1) {
            int srcval, plain;

            /*
             * A cast is a width change only when the source is a
             * plain value: an address or an aggregate is not shrunk
             * the same way.  The target may still be a pointer -
             * sixteen bits on this machine - so "(struct cell *)
             * (long)" narrows rather than relabels.  Relabelling it
             * typed the AND underneath as a pointer, the folder's
             * "x & 0xffff -> x" dropped the mask as word-width, and
             * objptr() handed the high word over as the pointer.
             */
            srcval = e1->type && tp &&
                !(e1->type->flags & (TF_POINTER | TF_ARRAY | TF_FUNC |
                                     TF_AGGREGATE)) &&
                !(e1->flags & E_CONST);
            plain = srcval &&
                !(tp->flags & (TF_POINTER | TF_ARRAY | TF_FUNC |
                               TF_AGGREGATE));

            if (srcval && tp->size < e1->type->size) {
                e = mkexpr(NARROW, e1);
                e->type = tp;
            } else if (plain && tp->size > e1->type->size) {
                /*
                 * And the other direction, which is the same
                 * mistake read the other way: relabelling a byte
                 * as a long does not put anything in the three
                 * bytes above it.  Which of the two conversions it
                 * is depends on the *source*: a signed value
                 * sign-extends and an unsigned one zero-extends,
                 * and the instructions differ.
                 */
                e = mkexpr((e1->type->flags & TF_UNSIGNED) ?
                    WIDEN : SEXT, e1);
                e->type = tp;
            } else {
                /*
                 * A constant is relabelled, not wrapped - but a
                 * narrowing relabel has to truncate the VALUE too,
                 * here and now, or every later fold happily
                 * compares the wide value under the narrow name:
                 * (unsigned short)BIG != BIG&0xffff folded true.
                 * Signed targets sign-extend from the new width so
                 * the value still means what the type says.
                 */
                if ((e1->flags & E_CONST) && tp &&
                    !(tp->flags & (TF_POINTER | TF_ARRAY | TF_FUNC |
                                   TF_AGGREGATE))) {
                    if (tp->size == 1) {
                        e1->v &= 0xff;
                        if (!(tp->flags & TF_UNSIGNED) && (e1->v & 0x80))
                            e1->v |= 0xffffff00L;
                    } else if (tp->size == 2) {
                        e1->v &= 0xffff;
                        if (!(tp->flags & TF_UNSIGNED) && (e1->v & 0x8000L))
                            e1->v |= 0xffff0000L;
                    }
                }
                e1->type = tp;
                e = e1;
            }
        } else {
            e = mkexprI(CONST, 0, tp, 0, 0);
        }
    } else {
        /*
         * Parenthesized expression: (expr)
         * parse inner expression with lowest precedence
         */
        e = parseExpr(0);
        expect(RPAR, ER_E_SP);
    }
    return e;
}

/* unary minus, complement, logical not - the operator still in cur */
struct expr *
pfxUnary(void)
{
    struct expr *e, *e1;
    unsigned char uop;
    unsigned long uval;

    /*
     * The lexeme and the AST node are not the same thing: unary
     * minus becomes NEG, and "~" has to become NOT the same way.
     * Leaving it as the TWIDDLE lexeme meant pass2 never saw an
     * operator it recognised, so "~x" reduced to nothing at all -
     * no code, at either width, for the life of the compiler.
     */
    uop = (cur.type == MINUS) ? NEG :
          (cur.type == TWIDDLE) ? NOT : cur.type;
    gettoken();
    e1 = parseExpr(OP_PRI_MULT - 1);
    if (!e1)
        return 0;
    /* Fold unary ops on constants */
    if (e1->flags & E_CONST) {
        uval = e1->v;
        if (uop == NEG) uval = -uval;
        else if (uop == NOT) uval = ~uval;
        else if (uop == BANG) uval = !uval;
        e1->v = uval;
        return e1;
    }
    e = mkexpr(uop, e1);
    /*
     * Negation and complement give back what they were handed;
     * "!" gives an int whatever it was applied to.  Taking the
     * operand's type made "!lv" a long, so "!lv != 0" looked
     * like 32-bit work and went looking for a helper to
     * compare a truth value nothing had widened.
     */
    e->type = (uop == BANG) ? inttype : e1->type;
    /*
     * And these promote as the binary operators do: "-c" on a
     * char is the negation of an int, not of a byte.  Taking
     * the operand's width made it a byte negation widened
     * afterwards, so "-(unsigned char)5" came out as 251.
     */
    if ((uop == NEG || uop == NOT) && e->type &&
        e->type->size > 0 && e->type->size < inttype->size &&
        !(e->type->flags & (TF_POINTER | TF_ARRAY)))
        e->type = inttype;
    return e;
}

/* unary dereference, the STAR consumed */
struct expr *
pfxStar(void)
{
    struct expr *e, *e1;

    e1 = parseExpr(OP_PRI_MULT - 1);
    if (!e1)
        return 0;
    /*
     * Dereferencing a pointer to a function is a no-op: what it
     * yields is the function, and what you can do with that is
     * call it, which needs the address the pointer already holds.
     * Wrapping it in a load meant "(*fp)()" tried to fetch
     * through the pointer and call whatever it found.
     */
    if (e1->type && (e1->type->flags & TF_POINTER) &&
        e1->type->sub && (e1->type->sub->flags & TF_FUNC))
        return e1;
    e = mkexpr(DEREF, e1);
    if ((e1->type->flags & TF_POINTER) && e1->type->sub)
        e->type = e1->type->sub;
    else
        e->type = e1->type;
    return e;
}

/* address-of, the AND consumed */
struct expr *
pfxAddr(void)
{
    struct expr *e, *e1;

    e = parseExpr(OP_PRI_MULT - 1);
    if (!e)
        return 0;
    /*
     * Mark variable as address-taken (can't use register).
     *
     * Phase 1 has already done this, in skipExpr, and that is the
     * one that counts - by the time this runs the registers have
     * been handed out.  It is kept because the flag is also read
     * for the parameters, and it complains about nothing: taking
     * the address of a register variable is refused in phase 1,
     * which still has the right line number for it.
     */
    if (e->op == DEREF && e->left->op == SYM)
        ((struct name *)e->left->var)->w.r.addr_taken = 1;
    /* Optimize: &(DEREF x) = x, since SYM already gives address */
    if (e->op == DEREF) {
        e1 = e;
        e = e->left;
        if (e->op == SYM)
            e->type = getType(TF_POINTER, e->type, 0);
        e1->left = NULL;
        freeNode(e1);
    } else if (e->type->flags & (TF_ARRAY | TF_FUNC)) {
        /*
         * An array and a FUNCTION are both already their own address,
         * so &a is a, with the type made a pointer.  The function half
         * of this was missing: a function designator is not a DEREF
         * and was not an array, so "&order" fell into the else below
         * and was handed back as a one-child AND.
         *
         * AND is BINARY in astops.h - the one table pass1 writes by,
         * pass2 reads by and astpp prints by - so a node with a left
         * and no right desynchronises the stream at that point.  pass2
         * read the next node as the missing operand, ran off the end,
         * and died.  That is the c1 SIGSEGV on Morrow's formatmw.c:
         * report() passes &order and &ex to sort(), and c1 wrote 95K
         * of assembler and then crashed with no diagnostic from
         * anything.  A bare "order" always worked, which is why it
         * looked like a code generation fault rather than a parse one.
         */
        e->type = getType(TF_POINTER, e->type, 0);
    } else {
        /*
         * Not an lvalue, so there is no address to take - "&(a + 1)".
         * This used to build the same malformed AND and desynchronise
         * the stream exactly as above, turning a source error into a
         * crash in the next pass.  Say so here, where the line number
         * is still right, and hand back the operand so parsing can go
         * on and find any further errors.
         */
        gripe(ER_E_LV);
    }
    return e;
}

/* sizeof(type), sizeof(expr), or sizeof expr - the keyword consumed */
struct expr *
pfxSizeof(void)
{
    struct expr *e1;
    struct type *t;

    if (cur.type == LPAR) {
        gettoken();  /* consume '(' */
        if (isCastStart()) {
            /* sizeof(type) */
            t = parseTypeName();
            expect(RPAR, ER_E_SP);
        } else {
            /* sizeof(expr) */
            e1 = parseExpr(0);
            t = e1 ? e1->type : (struct type *)0;
            FreeExpr(e1);
            expect(RPAR, ER_E_SP);
        }
    } else {
        /* sizeof expr (no parens) */
        e1 = parseExpr(OP_PRI_MULT - 1);
        t = e1 ? e1->type : (struct type *)0;
        FreeExpr(e1);
    }
    /*
     * typesize, not t->size: the type node keeps its size in a
     * byte, so an array of 256 or more records zero.  "unsigned
     * char buf[512]" answered 0, and pass1's own
     * read(fd, lexBuf, sizeof lexBuf) asked the kernel for no
     * bytes and took the zero back for end of file.
     */
#ifdef DEBUG
    if (!t) fdprintf(2, "bad op (sizeof): no type\n");
#endif
    if (!t) gripe(ER_E_UO);
    /*
     * An array whose extent nothing has settled has no size to ask
     * for.  It is indeterminate until a later declaration says how
     * big it is, and until then sizeof cannot be answered - it used
     * to come back 2, the size of the pointer an array decays to,
     * which is a wrong answer rather than a refused one.
     *
     * So the declaration has to precede the use.  That is what C
     * requires in any case, and it is what lets the two phases run a
     * function at a time: nothing may depend on a declaration further
     * down the file.
     */
    if (incomplete(t))
        gripe(ER_D_IA);
    return mkexprI(CONST, 0, inttype, t ? typesize(t) : 0, E_CONST);
}

struct expr *
parsePrefix(void)
{
    unsigned char inc_op;
    struct expr *e = 0;
    struct type *t;
    long sval;

    switch (cur.type) {   // prefix

    case LNUMBER:
        /* the source said L, so it is a long however small it is */
        e = mkexprI(CONST, 0, longtype, (unsigned long)cur.v.numeric,
                    E_CONST);
        gettoken();
        break;

    case INUMBER:
        /* folded upstream from an int-typed construct */
        e = mkexprI(CONST, 0, inttype, (unsigned long)cur.v.numeric,
                    E_CONST);
        gettoken();
        break;

    case NUMBER:
        sval = cur.v.numeric;
        /* Inline: determine smallest type for constant */
        if (sval < 0)
            t = (sval >= -128) ? chartype : (sval >= -32768) ? inttype : longtype;
        else if (sval <= 255)
            t = uchartype;
        else if (sval <= 65535)
            t = ushorttype;
        else
            /* sval >= 0 always fits in long here; spelling this as
             * sval <= 2147483647L trips a zc3 bug (rewritten to
             * < LONG_MIN, always false) */
            t = longtype;
        e = mkexprI(CONST, 0, t, (unsigned long)sval, E_CONST);
        gettoken();
        break;

    case STRING:
        e = pfxString();
        break;

    case SYM:
        e = pfxSym();
        break;

    case LPAR:
        gettoken();
        e = pfxCast();
        break;

    case MINUS:
    case TWIDDLE:
    case BANG:
        e = pfxUnary();
        break;

    case STAR:
        gettoken();
        e = pfxStar();
        break;

    case AND:
        gettoken();
        e = pfxAddr();
        break;

    case SIZEOF:
        gettoken();
        e = pfxSizeof();
        break;

    case INCR:      // prefix increment: ++i
    case DECR:      // prefix decrement: --i
        inc_op = cur.type;
        gettoken();
        e = mkIncDec(parseExpr(OP_PRI_MULT - 1), inc_op, 0);
        break;

	default:
#ifdef DEBUG
		fdprintf(2, "bad op (expr): %d\n", cur.type);
#endif
		gripe(ER_E_UO);
		return 0;
    }

    return e;
}
