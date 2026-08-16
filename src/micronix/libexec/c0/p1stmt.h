/*
 * statements: blocks, locals and switches
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
#ifndef _P1STMT_H
#define _P1STMT_H

#include "p1base.h"

/* swcnt.c - switch bookkeeping and statement counters */
struct swcase *nextCase(void);
void stIf2(void);
void stRet2(void);
void stSwitch2(void);
void stExpr2(void);
void stGoto2(void);
/* outh.c - AST-writer helpers */
struct name *findMemberOff(struct name *members, int offset);

/*
 * Switch statement table tracking (phase 1)
 * Accumulates case values and labels for each switch statement.
 * Nested switches use swStack to track which switch is current.
 * Dynamically allocated - no fixed limits on switch count.
 */
#define MAX_SWDEPTH 8       /* max switch nesting */
#define SW_INIT_CASES 8     /* initial cases per switch, and the step it grows by */
#define SW_INIT_SWS   8     /* initial switches per function, likewise */

/*
 * A case, as phase 1 records it.  Not its value: phase 2 re-parses
 * that from the token stream as an expression, so the four bytes this
 * used to carry were written twice and read never.  A big switch runs
 * to a couple of hundred cases and the array is live for the whole
 * function.
 */
/*
 * A local as register allocation and emission need it.
 *
 * Phase 1 used to hand phase 2 a whole struct name per local - forty
 * bytes on the host, twenty-three on the Z80 - of which thirteen
 * fields are ever read.  The symbol-table chain, the tag and emitted
 * flags and the initialiser union are all dead the moment the copy is
 * taken.
 *
 * And these do not come and go with each function: phase 1 captures
 * every function's before phase 2 frees any, so the whole file's worth
 * is live at the turn between them.  Six bytes apiece is worth having.
 */
struct local {
	unsigned short id;
	struct type *type;
	struct local *next;
	kind kind;
	unsigned char level;
	unsigned char sclass;
	unsigned char static_id;
	unsigned char ref_count;  /* reference count (capped at 255) */
	unsigned char agg_refs;   /* struct member accesses, for IX */
	unsigned char reg;        /* 0=none, 1=B, 2=C, 3=BC, 4=IX */
	unsigned char addr_taken;
	unsigned char blkid;      /* declaring block, with level walks scopes */
	short frm_off;            /* params positive, locals negative */
};

struct swcase {
    unsigned char is_default; /* 1 if default, 0 if case */
    unsigned char stmts;    /* statement count for this case section */
};

struct swtab {
    struct swcase *cases;   /* allocated case array for this switch */
    unsigned char count;    /* number of cases */
    unsigned char capacity; /* allocated size of cases array */
    unsigned char num;      /* switch number (for labels) */
    unsigned char base_stmts; /* stmt_count at start of current case */
    unsigned char final_cnt;  /* stmt_count when switch body ends */
    unsigned char emitIdx;  /* phase 2: current case being emitted */
    unsigned char cslot;    /* reserved count-queue slot (phase 1) */
};

/* Dynamic switch list */
extern struct swtab *swList;
extern unsigned char swCount;       /* number of switches in function */
extern unsigned char swCapacity;    /* allocated size of swList */
extern unsigned char swStack[];     /* nesting stack (indices into swList) */
extern unsigned char swDepth;       /* nesting depth */
extern unsigned char swEmitIdx;     /* phase 2: next switch to emit */
extern unsigned char swEmitStack[]; /* phase 2: stack of switch indices */
extern unsigned char swEmitDepth;   /* phase 2: emit stack depth */



/*
 * Count storage for streaming AST emission
 * Phase 1 computes counts (args, cases, stmts), phase 2 retrieves them.
 * Reset between functions, flip after phase 1 for LIFO retrieval.
 */

/* Block statement counts (phase 1 -> phase 2) */

#endif
