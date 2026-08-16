/*
 * swcnt.c - switch and statement-count bookkeeping
 *
 * What swcnt.c exports.  The AST is written in two phases and the
 * second needs to know how many statements each block held, which is
 * what all the counting is for.
 *
 * One source's contract in one header, so a caller takes this and not
 * everything else that used to share p1stmt.h with it.
 */
#ifndef _P1SWCNT_H
#define _P1SWCNT_H

#include "p1base.h"
struct expr;
struct type;
struct name;
struct local;

int atSwBodyStmt(void);
int reserveCount(void);
void patchCount(int slot, char c);
void resetSwitch(void);             /* reset for new function */
void pushSwitch(void);              /* enter switch statement */
void popSwitch(void);               /* exit switch statement */
void addCase(unsigned char stmt_cnt);  /* add case to current switch */
void addDefault(unsigned char stmt_cnt);           /* add default to current switch */
void finishCase(unsigned char stmt_cnt);           /* finalize current case stmt count */
void pushCount(char c);
char popCount(void);
void resetCounts(void);
void resetCountIdx(void);
void resetSpanCnts(void);  /* Reset read pointer for phase 2 */
void enterBlkCnt(void);  /* call when entering block in phase 1 */
void pushBlkCnt(unsigned char n);
unsigned char popBlkCnt(void);
void flipBlkCnts(void);  /* prepare for phase 2 */
void resetBlkCnts(void);
void pushFuncCnt(unsigned char n);
unsigned char popFuncCnt(void);
void resetFuncIdx(void);

#endif
