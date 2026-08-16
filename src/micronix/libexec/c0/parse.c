/*
 * statement parsing
 */

#include <stdlib.h>
#include "p1core.h"
#include "p1expr.h"
#include "p1name.h"
#include "p1stmt.h"
#include "p1lex.h"
#include "p1swcnt.h"
#include "p1pblk.h"

char *blockname();

/* Switch statement table tracking (phase 1) - dynamically allocated */
struct swtab *swList = 0;           /* dynamically allocated switch array */
unsigned char swCount = 0;          /* number of switches in function */
unsigned char swCapacity = 0;       /* allocated size of swList */
unsigned char swStack[MAX_SWDEPTH]; /* nesting stack (indices into swList) */
unsigned char swStmtDepth[MAX_SWDEPTH]; /* statement() depth at which each switch started */
unsigned char swDepth = 0;          /* nesting depth */
unsigned char stmtNest = 0;
int spanStop;			/* set when a function definition ends */  /* current statement() nesting depth */

/* Phase 2 switch emission tracking */
unsigned char swEmitIdx = 0;                /* next switch to emit */
unsigned char swEmitStack[MAX_SWDEPTH];     /* stack of switch indices */
unsigned char swEmitDepth = 0;              /* emit stack depth */

/*
 * If/else tracking: bitmap of has_else flags, one bit per if statement.
 *
 * The count runs for the whole file, not for one function.  Phase 1
 * walks every function before phase 2 walks any of them, so the bit
 * phase 1 records for an if has to still be there when phase 2 reaches
 * it, and a per-function reset would have the next function's phase 1
 * overwrite it.
 *
 * A bit apiece, so the limit is cheap to be generous with, and cpp
 * lowers every while and for into an if - a source spends them much
 * faster than its ifs alone suggest.  256 was not enough for a
 * 2900-line one.
 */
/*
 * 512, and it was 4096.  This is a bitmap, one bit per if, so 4096
 * cost 512 bytes of bss - and the most ifs in any source of cpp, c0
 * or c1 is 66.  512 leaves it seven times the room it has ever
 * needed and costs 64 bytes.  Overflow is fatal(ER_S_IF).
 */
#define MAX_IFS 512
unsigned char ifHasElse[MAX_IFS / 8]; /* bit N set: if #N has else */
unsigned short ifCount = 0;          /* phase 1: count of if statements */
unsigned short ifEmitIdx = 0;        /* phase 2: next if to emit */

#ifdef DEBUG
#endif

void
statement(void)
{
    unsigned char block = 1;
    char stmt_count = 0;       /* Count statements for streaming (phase 1) */
    char *text;                /* Text pointer for ASM */
    struct swcase *sc;         /* CASE/DEFAULT: the case up next */
    struct expr *e1;           /* CASE: the value expression */

    stmtNest++;  /* Track statement() nesting depth */

    while (block) {
#ifdef DEBUG
        if ((VERBOSE(V_PHASE1) && phase == 1) || (VERBOSE(V_PHASE2) && phase == 2))
            fdprintf(2, "stmt() P%d lev=%d cur.type=%d (%c)\n",
                     phase, lexlevel, cur.type, cur.type > 31 && cur.type < 127 ? cur.type : '?');
#endif

        /*
         * Phase 1: Skip statements, only track scope and declarations
         * This discovers local variables without building trees.
         * Also counts statements for streaming emission in phase 2.
         */
        if (phase == 1) {
#ifdef DEBUG
            if (VERBOSE(V_PHASE1) && cur.type == SWITCH)
                fdprintf(2, "P1 SWITCH at lexlevel=%d\n", lexlevel);
#endif
            switch (cur.type) {
            case END:
            case E_O_F:
                /* Push statement count for blocks */
#ifdef DEBUG
                if (VERBOSE(V_PHASE1))
                    fdprintf(2, "P1 END: lev=%d cnt=%d sw=%d\n",
                             lexlevel, stmt_count, swDepth);
#endif
                /*
                 * Store stmt_count for the innermost switch body.
                 * Only do this when we're at the switch body level (stmtNest),
                 * not when nested blocks inside cases end.
                 */
                if (atSwBodyStmt())
                    swList[swStack[swDepth - 1]].final_cnt = stmt_count;
                /* Function body uses separate mechanism */
                if (lexlevel == 2 && swDepth == 0)
                    pushFuncCnt(stmt_count);
                /* Nested blocks (lexlevel > 2) store to block counts */
                /* Note: this can happen while inside a switch (swDepth > 0) */
                if (lexlevel > 2 && !atSwBodyStmt())
                    pushBlkCnt(stmt_count);
                block = 0;
                break;
            case BEGIN:
                gettoken();
                pushScope(blockname());
                /* Register block entry for nested blocks */
                if (lexlevel > 2)
                    enterBlkCnt();
                statement();  /* recurse for nested block */
                popScope();
                expect(END, ER_S_CC);
                stmt_count++;  /* BEGIN counts as one statement */
                break;
            /* Declarations - process for symbol table but don't count */
            case INT: case CHAR: case SHORT: case LONG:
            case VOID:
            case STRUCT: case UNION:
            case UNSIGNED:
            case STATIC: case REGISTER: case AUTO:
            case EXTERN:
                declaration();
                break;
            case SYM:
                /* Fall through to expression statement */
            case NUMBER: case INUMBER: case LNUMBER:
            case STRING: case LPAR:
            case STAR: case INCR: case DECR:
                parseExpr(PRI_ALL);
                expect(SEMI, ER_S_SN);
                stmt_count++;
                break;
            case LABEL:
                hlClose();      /* a loop's top is not a straight line */
                gettoken();
                stmt_count++;
                break;
            case IF:
            handle_if: {
                unsigned short thisIf = ifCount++;
                if (thisIf >= MAX_IFS)
                    fatal(ER_S_IF);
                gettoken();
                expect(LPAR, ER_S_NP);
                parseExpr(PRI_ALL);
                expect(RPAR, ER_S_NP);
                parseBlock();
                if (cur.type == ELSE) {
                    ifHasElse[thisIf >> 3] |= 1 << (thisIf & 7);
                    gettoken();
                    if (cur.type == IF)
                        goto handle_if;  /* else if */
                    parseBlock();
                } else {
                    ifHasElse[thisIf >> 3] &= ~(1 << (thisIf & 7));
                }
                stmt_count++;
                break;
            }
            /* WHILE/DO/FOR handled by cpp loop lowering */
            case SWITCH: {
                struct swtab *sw;
#ifdef DEBUG
                if (VERBOSE(V_PHASE1))
                    fdprintf(2, "P1 SWITCH: cur=%d line=%d\n", cur.type, lineno);
#endif
                gettoken();
                expect(LPAR, ER_S_NP);
                parseExpr(PRI_ALL);
                expect(RPAR, ER_S_NP);
                expect(BEGIN, ER_S_SB);
                pushSwitch();  /* start new switch table */
                /* the slot is claimed at ENTRY, matching phase 2's
                 * read order; the count is patched in at the end */
                swList[swStack[swDepth - 1]].cslot =
                    (unsigned char)reserveCount();
                statement();  /* switch body - adds cases to table */
                /* Finalize last case using stmt_count stored by END handler */
                sw = &swList[swStack[swDepth - 1]];
                finishCase(sw->final_cnt);
                patchCount(sw->cslot, sw->count);
                popSwitch();
                expect(END, ER_S_CC);
                stmt_count++;
                break;
            }
            case CASE: {
                gettoken();
                parseConst(COLON);
                addCase(stmt_count);  /* add to current switch table */
                expect(COLON, ER_S_NL);
                break;
            }
            case DEFAULT:
                gettoken();
                addDefault(stmt_count);  /* add default to current switch table */
                expect(COLON, ER_S_NL);
                break;
            case BREAK:  /* only in switch - loop breaks handled by cpp */
                gettoken();
                expect(SEMI, ER_S_SN);
                stmt_count++;
                break;
            case RETURN:
                gettoken();
                if (cur.type != SEMI)
                    parseExpr(PRI_ALL);
                expect(SEMI, ER_S_SN);
                stmt_count++;
                break;
            case GOTO:
                gettoken();
                if (cur.type == SYM)
                    gettoken();
                expect(SEMI, ER_S_SN);
                stmt_count++;
                break;
            case ASM:
                text = getAsmText();
                free(text);
                stmt_count++;
                break;
            case SEMI:
                gettoken();
                stmt_count++;
                break;
            default:
                gettoken();  /* skip unknown token */
                break;
            }
            continue;  /* phase 1: don't build statement tree */
        }

        /*
         * Phase 2: Normal statement parsing with tree building
         */
    	switch (cur.type) {

    	case END:   // end a block
    	case E_O_F: // end of file
            block = 0;
            break;

        case BEGIN:  // begin a block
            gettoken();
            pushScope(blockname());
            /* Emit block header: AST_BLOCK 0 stmt_count */
            emit1(AST_BLOCK);
            emit1(0);  /* no decls - hoisted to function */
            emit1(popBlkCnt());
            /* Stream body statements */
            statement();
            popScope();
            expect(END, ER_S_CC);
            break;

        case IF:   /* if <condition> <statement> */
            stIf2();
            break;

        /* BREAK/CONTINUE handled by cpp - lowered to goto */

        case RETURN:
            stRet2();
            break;

        /* Local declarations - type keywords */
        case INT:
        case CHAR:
        case SHORT:
        case LONG:
        case VOID:
        case STRUCT:
        case UNION:
        case UNSIGNED:
        case STATIC:
        case REGISTER:
        case AUTO:
        case EXTERN:
            declaration();
            break;

        case LABEL:
            /* Label (from cpp) */
            emit1(LABEL);
            emitS(nameOf(cur.v.id));
            gettoken();
            break;

        case SYM:
            /* fall through to expression */
        case NUMBER: case INUMBER:    // numeric literals can start expression statements
        case LNUMBER:
        case STRING:    // string literals can start expression statements
        case LPAR:
        case STAR:
        case INCR:
        case DECR:
            stExpr2();
            break;

        /* FOR/WHILE/DO handled by cpp loop lowering - should not appear here */

        case ELSE:
            recover(SEMI, ER_S_OE);
            break;

        case SWITCH:  /* switch (<expr>) <block> ; */
            stSwitch2();
            break;

        case CASE:
            gettoken();
            e1 = parseExpr(13);  /* parse case value expression */
            expect(COLON, ER_S_NL);
            sc = nextCase();
            /* Emit: CASE stmt_count value_expr */
            e1 = foldTree(e1);
            emit1(CASE);
            emit1(sc->stmts);
            emitExpr(e1);
            FreeExpr(e1);
            break;

        case GOTO:
            stGoto2();
            break;

        case DEFAULT:
            gettoken();
            expect(COLON, ER_S_NL);
            sc = nextCase();
            /* Emit: DEFAULT stmt_count */
            emit1(DEFAULT);
            emit1(sc->stmts);
            break;

        case SEMI:
            gettoken();
            emit1(SEMI);
            break;

        case ASM:
            /* Get asm text and stream it as a statement */
            text = getAsmText();
            emitAsmStmt(text);
            free(text);
            break;

        default:
#ifdef DEBUG
            fdprintf(2, "bad op: %d\n", cur.type);
#endif
            gripe(ER_E_UO);
            break;
        }
    }

    stmtNest--;  /* Restore statement() nesting depth */
}

/*
 * Generate a unique name for a block scope (dummy for debugging)
 */
char*
blockname()
{
	return "blk";
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
