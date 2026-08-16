/*
 * parse.c - the statement parser's shared state
 *
 * What parse.c owns and phase 2 reads back.  These were written out
 * as extern declarations by hand in pblock.c, pstmt2.c and swcnt.c -
 * three copies of the same contract, none of them checked against
 * the definitions in parse.c.  One header instead.
 */
#ifndef _P1PARSE_H
#define _P1PARSE_H

#include "p1base.h"

/* if-statement bookkeeping: a bit per if, so phase 2 knows which
 * had an else without keeping the tree */
extern unsigned char ifHasElse[];
extern unsigned short ifCount;
extern unsigned short ifEmitIdx;	/* phase 2: next if to emit */

/* statement nesting, for the switch-body test */
extern unsigned char stmtNest;
extern unsigned char swStmtDepth[];

#endif
