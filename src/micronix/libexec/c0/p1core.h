/*
 * what every pass1 source needs, and no more
 *
 * The system headers, the generated ones, the shared typedefs and the
 * error codes.  Everything past this is a part of the compiler rather
 * than its furniture, and lives in a p1*.h a source includes only if
 * it uses it - see p1expr.h for why that matters.
 */
#ifndef _P1CORE_H
#define _P1CORE_H

/*
 * The system headers are NOT here.
 *
 * c0 keeps every name it has ever seen for the whole translation
 * unit, so what this header pulls in is charged to all twenty-four
 * sources whether they use it or not - <string.h> is 47 names,
 * <stdio.h> 36, <stdlib.h> 35.  Six sources use stdio outside DEBUG,
 * eleven use string.h, fifteen use stdlib.h, and parse.c - the source
 * that needs the most room in cpp - uses one function from one of
 * them.  So a source includes what it uses.
 *
 * <unixio.h> stays: three names, and it is the read/write/open the
 * whole pass is built on.
 *
 * DEBUG builds print, so they get stdio here rather than in twenty
 * separate guards.
 */
#ifdef CCC
#include <unixio.h>
#endif
#ifdef DEBUG
#include <stdio.h>
#endif

/*
 * Compiler-wide control.
 *
 * These were in p1stmt.h, beside the switch tables, and every source
 * that wanted to know which parsing phase it was in had to take the
 * statement machinery with it - which is most of them, and is what
 * made p1stmt.h the expensive include.  They are not statements;
 * they are the state the whole pass runs under.
 *
 * They used to be written out twice, once in each arm of a
 * CCC/not-CCC pair that existed only to choose between <stdio.h> and
 * <unixio.h>.  Both copies had to be edited together and nothing said
 * so.
 */
struct name;			/* a pointer to one is all this needs */

/* Global context for static variable name mangling */
extern struct name *curFunc;
extern unsigned char staticCtr;  // file-global counter for static variable names
extern unsigned char shadowCtr;  // counter for shadowed locals

/* AST output control */
extern unsigned char astFd;         // where to write AST output
extern unsigned char asmFd;         // where to write global data assembly

/* Two-phase parsing control */
extern unsigned char phase;         // 1 = build symbol table, 2 = emit AST

/*
 * generated files
 */
#include "debug.h"
#include "token.h"

#include "p1base.h"

/*
 * we just want the error symbols
 * error.h is generated, and contains actual error strings if DEF_ERRMSG.
 */
#undef DEF_ERRMSG
#include "error.h"

/*
 * Allocation counters for tracking memory usage
 */
#ifdef DEBUG
extern int nameAllocCnt;
extern int nameCurCnt;
extern int nameHighWater;
extern int exprAllocCnt;
extern int exprCurCnt;
extern int exprHighWater;
/*
 * Compiler-wide control.
 *
 * These were in p1stmt.h, beside the switch tables, and every source
 * that wanted to know which parsing phase it was in had to take the
 * statement machinery with it - which is most of them, and is what
 * made p1stmt.h the expensive include.  They are not statements;
 * they are the state the whole pass runs under.
 */
struct name;			/* a pointer to one is all this needs */

/* Global context for static variable name mangling */
extern struct name *curFunc;
extern unsigned char staticCtr;  // file-global counter for static variable names
extern unsigned char shadowCtr;  // counter for shadowed locals

/* AST output control */
extern unsigned char astFd;         // where to write AST output
extern unsigned char asmFd;         // where to write global data assembly

/* Two-phase parsing control */
extern unsigned char phase;         // 1 = build symbol table, 2 = emit AST

#endif

/*
 * Compiler-wide control.
 *
 * These were in p1stmt.h, beside the switch tables, and every source
 * that wanted to know which parsing phase it was in had to take the
 * statement machinery with it - which is most of them, and is what
 * made p1stmt.h the expensive include.  They are not statements;
 * they are the state the whole pass runs under.
 */
struct name;			/* a pointer to one is all this needs */

/* Global context for static variable name mangling */
extern struct name *curFunc;
extern unsigned char staticCtr;  // file-global counter for static variable names
extern unsigned char shadowCtr;  // counter for shadowed locals

/* AST output control */
extern unsigned char astFd;         // where to write AST output
extern unsigned char asmFd;         // where to write global data assembly

/* Two-phase parsing control */
extern unsigned char phase;         // 1 = build symbol table, 2 = emit AST

#endif
