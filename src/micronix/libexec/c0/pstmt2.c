/*
 * the phase-2 statement arms
 *
 * Split out of pblock.c, which was block structure, locals and these
 * in one file and so needed every header pass1 has.  These want the
 * switch tables and the emitters; the block half wants the locals.
 * Neither wants the other's.
 */
/*
 * No <string.h>: nothing here calls it.  NULL was the only thing
 * wanted and a plain 0 is a null pointer constant - cpp holds every
 * macro and identifier a header brings, and this was the last source
 * in the tree that would not fit a 62K TPA.
 */
#include "p1core.h"
#include "p1expr.h"
#include "p1type.h"
#include "p1name.h"
#include "p1stmt.h"
#include "p1lex.h"
#include "p1swcnt.h"
#include "p1pblk.h"
#include "p1parse.h"

/*
 * The phase-2 statement arms, one worker each.  statement() carried
 * a bank of "hoisted locals - shared across cases to reduce stack
 * frame", which is the opposite of what this compiler wants: it does
 * no lifetime analysis, by design, so sharing a frame is sharing two
 * registers among locals that never coexist.  The function boundary
 * is the lifetime analysis.
 */

/* the case the current switch is up to, shared by CASE and DEFAULT */
struct swcase *
nextCase(void)
{
    register struct swtab *sw;

    sw = &swList[swEmitStack[swEmitDepth - 1]];
    return &sw->cases[sw->emitIdx++];
}

/* if <condition> <statement> [else ...], cur on the IF */
void
stIf2(void)
{
    unsigned char hasElse;
    struct expr *e1;

    for (;;) {
        hasElse = (ifHasElse[ifEmitIdx >> 3] >> (ifEmitIdx & 7)) & 1;
        ifEmitIdx++;
        gettoken();
        expect(LPAR, ER_S_NP);
        e1 = parseExpr(PRI_ALL);
        expect(RPAR, ER_S_NP);
        /* Emit: IF nlabels cond then has_else [else] */
        /* fold first: emitExpr folds internally and may replace the
         * root node, leaving our e1 dangling for FreeExpr */
        e1 = foldTree(e1);
        emit1(IF);
        emit1(cntCondLbls(e1));
        emitExpr(e1);
        FreeExpr(e1);
        parseBlock();
        emit1(hasElse);  /* has_else comes after then block */
        if (cur.type == ELSE) {
            gettoken();
            if (cur.type == IF)
                continue;       /* else if: run the arm again */
            parseBlock();
        }
        return;
    }
}

void
stRet2(void)
{
    struct expr *e1;

    e1 = 0;
    gettoken();
    if (cur.type != SEMI)
        e1 = parseExpr(PRI_ALL);
    expect(SEMI, ER_S_SN);
    /* a struct has no value to return - return its address */
    if (e1 && e1->type && (e1->type->flags & TF_AGGREGATE) &&
        !(e1->type->flags & (TF_POINTER | TF_ARRAY | TF_FUNC)))
        gripe(ER_E_AG);
    /* Emit: RETURN has_value [expr] */
    e1 = foldTree(e1);
    emit1(RETURN);
    emit1(e1 ? 1 : 0);
    if (e1) {
        /*
         * The value has to arrive at the width the function
         * was declared to return, and only the tree knows
         * whether to sign- or zero-extend.  Without this,
         * "long f() { return 7; }" loaded HL alone - which is
         * the high half of a long, so it returned 458752 and
         * whatever DE happened to hold.
         */
        emitOperand(e1, curFunc ? curFunc->type->sub : 0);
        FreeExpr(e1);
    }
}

void
stSwitch2(void)
{
    struct expr *e1;
    unsigned char sw_idx;

    gettoken();
    expect(LPAR, ER_S_NP);
    e1 = parseExpr(PRI_ALL);
    expect(RPAR, ER_S_NP);
    expect(BEGIN, ER_S_SB);
    /* Get this switch's index and push onto emit stack */
    sw_idx = swEmitIdx++;
    swEmitStack[swEmitDepth++] = sw_idx;
    swList[sw_idx].emitIdx = 0;
    /* Emit switch header: SWITCH has_label case_count expr */
    /* has_label=0 since cpp handles break lowering */
    e1 = foldTree(e1);
    emit1(SWITCH);
    emit1(0);  /* no label - cpp lowered break to goto */
    emit1(popCount());
    /*
     * The control is promoted to int, like any other operand that
     * has to meet int-sized company - and the case labels are ints.
     *
     * It went out at its own width, so a char control was left in A
     * and pass2 widened it the only way it could without knowing the
     * type: "ld l,a / ld h,0", high half zero, with a comment saying
     * anything narrower than its use had been widened before it got
     * there.  For a signed char it had not been.
     *
     *	char opt;  ...  switch (opt) { case -1: ... }
     *
     * put 0x00ff up against the 0xffff in the table and matched
     * nothing, so diff -e dropped every one of its ed command lines
     * and printed only the text.  emitOperand puts a SEXT there when
     * the type is signed and a WIDEN when it is not.
     */
    emitOperand(e1, inttype);
    FreeExpr(e1);
    /* Parse body - CASE/DEFAULT emit themselves */
    statement();
    swEmitDepth--;
    expect(END, ER_S_CC);
}

void
stExpr2(void)
{
    struct expr *e;

    e = parseExpr(PRI_ALL);
    expect(SEMI, ER_S_SN);
    /* Convert postinc/postdec to preinc/predec since result unused */
    if (e && (e->op == INCR || e->op == DECR) && (e->flags & E_POSTFIX))
        e->flags &= ~E_POSTFIX;
    /* Emit expression statement directly (no EXPR wrapper) */
    e = foldTree(e);
    emitExpr(e);
    FreeExpr(e);
}

void
stGoto2(void)
{
    unsigned short lblid;

    gettoken();
    if (cur.type != SYM) {
        recover(ER_S_GL, SEMI);
        return;
    }
    /* Copy the label before gettoken overwrites cur.v.id */
    lblid = cur.v.id;
    gettoken();
    expect(SEMI, ER_S_SN);
    /* Emit: GOTO label */
    emit1(GOTO);
    emitS(nameOf(lblid));
}
