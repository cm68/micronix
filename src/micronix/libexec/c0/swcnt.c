/*
 * switch bookkeeping and the phase-1/phase-2 statement counters,
 * split from parse.c so no single translation unit carries the
 * statement parser and its ledgers.
 */

#include <stdlib.h>
#include "p1core.h"
#include "p1name.h"
#include "p1stmt.h"
#include "p1lex.h"
#include "p1parse.h"


void resetSwitch(void) {
    register struct swtab *sw = swList;
    unsigned char n = swCount + 1;
    /* Free all allocated switch case arrays */
    while (--n) {
        if (sw->cases)
            free(sw->cases);
        sw++;
    }
    /* Free the switch list itself */
    if (swList) {
        free(swList);
        swList = 0;
    }
    swCount = 0;
    swCapacity = 0;
    swDepth = 0;
    swEmitIdx = 0;
    swEmitDepth = 0;
    ifCount = 0;
    ifEmitIdx = 0;
}

void pushSwitch(void) {
    unsigned char idx;
    struct swtab *sw;

    if (swDepth >= MAX_SWDEPTH)
        return;  /* nesting too deep */

    /* Grow swList if needed */
    if (swCount >= swCapacity) {
        int newcap = swCapacity + SW_INIT_SWS;
        struct swtab *newlist = realloc(swList, newcap * sizeof(struct swtab));
        if (!newlist)
            fatal(ER_NOMEM);
        swList = newlist;
        swCapacity = newcap;
    }

    idx = swCount++;
    sw = &swList[idx];
    /* Allocate case array for this switch */
    sw->cases = (struct swcase *)galloc(SW_INIT_CASES * sizeof(struct swcase));
#ifdef DEBUG
#endif
    sw->count = 0;
    sw->capacity = SW_INIT_CASES;
    sw->num = idx;
    sw->base_stmts = 0;
    sw->emitIdx = 0;
    swStmtDepth[swDepth] = stmtNest;  /* record statement() depth at switch start */
    swStack[swDepth++] = idx;
}

/* Check if we're in the innermost switch's body statement() (not nested deeper) */
int atSwBodyStmt(void) {
    if (swDepth == 0) return 0;
    /* Switch body is one level deeper than where switch started */
#ifdef DEBUG
    if (VERBOSE(V_PHASE1))
    fdprintf(2, "atSwBodyStmt: stmtNest=%d swStmtDepth=%d result=%d\n",
             stmtNest, swStmtDepth[swDepth - 1],
             stmtNest == swStmtDepth[swDepth - 1] + 1);
#endif
    return stmtNest == swStmtDepth[swDepth - 1] + 1;
}

void popSwitch(void) {
    if (swDepth > 0)
        swDepth--;
}

/* Finalize previous case's stmt count before starting a new case */
void finishCase(unsigned char stmt_cnt) {
    if (swDepth > 0) {
        unsigned char idx = swStack[swDepth - 1];
        struct swtab *sw = &swList[idx];
        if (sw->count > 0) {
            sw->cases[sw->count - 1].stmts = stmt_cnt - sw->base_stmts;
#ifdef DEBUG
            if (VERBOSE(V_PHASE1))
            fdprintf(2, "finishCase: sw[%d].cases[%d].stmts = %d - %d = %d\n",
                     idx, sw->count - 1, stmt_cnt, sw->base_stmts,
                     sw->cases[sw->count - 1].stmts);
#endif
        }
        sw->base_stmts = stmt_cnt;
    }
}

void addCase(unsigned char stmt_cnt) {
    unsigned char idx;
    struct swtab *sw;
    register struct swcase *cp;

    if (swDepth == 0)
        return;

    idx = swStack[swDepth - 1];
    sw = &swList[idx];

    /* Grow cases array if needed */
    if (sw->count >= sw->capacity) {
        int newcap;
        struct swcase *newcases;

    
        newcap = sw->capacity + SW_INIT_CASES;
        if (newcap > 255)
            newcap = 255;
        if (newcap <= sw->capacity)
            fatal(ER_NOMEM);            /* 255 cases is the ceiling */
        newcases = realloc(sw->cases, newcap * sizeof(struct swcase));
        if (!newcases)
            fatal(ER_NOMEM);
        sw->cases = newcases;
        sw->capacity = newcap;
    }

    cp = &sw->cases[sw->count];
    /* Finalize previous case if any */
    if (sw->count > 0) {
        cp[-1].stmts = stmt_cnt - sw->base_stmts;
#ifdef DEBUG
        if (VERBOSE(V_PHASE1))
        fdprintf(2, "addCase: finalize sw[%d].cases[%d].stmts = %d - %d = %d\n",
                 idx, sw->count - 1, stmt_cnt, sw->base_stmts,
                 cp[-1].stmts);
#endif
    }
    sw->base_stmts = stmt_cnt;
    /* Add new case */
    cp->is_default = 0;
    cp->stmts = 0;  /* will be set by next case or popSwitch */
#ifdef DEBUG
    if (VERBOSE(V_PHASE1))
    fdprintf(2, "addCase: add sw[%d].cases[%d] base=%d\n",
             idx, sw->count, sw->base_stmts);
#endif
    sw->count++;
}

void addDefault(unsigned char stmt_cnt) {
    unsigned char idx;
    struct swtab *sw;
    register struct swcase *cp;

    if (swDepth == 0)
        return;

    idx = swStack[swDepth - 1];
    sw = &swList[idx];

    /* Grow cases array if needed */
    if (sw->count >= sw->capacity) {
        int newcap;
        struct swcase *newcases;

    
        newcap = sw->capacity + SW_INIT_CASES;
        if (newcap > 255)
            newcap = 255;
        if (newcap <= sw->capacity)
            fatal(ER_NOMEM);            /* 255 cases is the ceiling */
        newcases = realloc(sw->cases, newcap * sizeof(struct swcase));
        if (!newcases)
            fatal(ER_NOMEM);
        sw->cases = newcases;
        sw->capacity = newcap;
    }

    cp = &sw->cases[sw->count];
    /* Finalize previous case if any */
    if (sw->count > 0) {
        cp[-1].stmts = stmt_cnt - sw->base_stmts;
#ifdef DEBUG
        if (VERBOSE(V_PHASE1))
        fdprintf(2, "addDefault: finalize sw[%d].cases[%d].stmts = %d - %d = %d\n",
                 idx, sw->count - 1, stmt_cnt, sw->base_stmts,
                 cp[-1].stmts);
#endif
    }
    sw->base_stmts = stmt_cnt;
    /* Add default */
    cp->is_default = 1;
    cp->stmts = 0;  /* will be set by next case or popSwitch */
#ifdef DEBUG
    if (VERBOSE(V_PHASE1))
    fdprintf(2, "addDefault: add sw[%d].cases[%d] base=%d\n",
             idx, sw->count, sw->base_stmts);
#endif
    sw->count++;
}


/*
 * Function body statement count stack (separate from case counts).
 *
 * One entry per function in the file, for the same reason the if
 * bitmap is file-wide: phase 1 counts every function's statements
 * before phase 2 emits any of them.
 *
 * This held 32, and the 33rd function onward was dropped on the way in
 * and read back as a count of zero on the way out.  A zero says the
 * body is empty, so pass2 stopped reading statements for that function
 * and took the ones that followed for top-level records - it did not
 * lose one function, it lost its place, and everything after came out
 * as whatever the bytes happened to decode to.  rewrite.c has 46
 * functions and its own entry point was among the missing.
 *
 * The indices are shorts now: unsigned char ones would have wrapped at
 * 256 and desynchronised in exactly the same silent way.
 */
#define MAX_FUNCCNTS 128	/* the largest source here has 46 */
static unsigned char funcCnts[MAX_FUNCCNTS];  /* stmt counts for functions */
static unsigned short funcCntTop = 0;   /* write pointer (phase 1) */
static unsigned short funcCntIdx = 0;   /* read pointer (phase 2) */

/*
 * Block statement counts - indexed by block ENTRY order (DFS)
 * Phase 1: assign entry ID when block starts, store count at ID when block ends
 * Phase 2: read count at current entry ID (deterministic DFS order matches)
 */
/*
 * Also file-wide, also read back by position, so running out
 * desynchronises rather than truncating - see funcCnts above.  This
 * was 256 and four sources in this tree were over it, the largest
 * needing 451.  A byte apiece, so there is room to be well clear of
 * the worst rather than just past it.
 */
/*
 * 256, and it was 1024.  Measured across every source of cpp, c0 and
 * c1 - the largest C this system compiles - the high water mark is
 * 76, and declare.c, the biggest single file at 46k, reaches 39.
 *
 * A worst-case table costs its worst case on a machine with 64k and
 * no virtual memory: this array is bss, bss is image, and image is
 * heap that the program will never get back.  768 bytes of it were
 * being held against a case nothing in the tree comes within an order
 * of magnitude of.
 *
 * Overflow is fatal(ER_S_BLK) - a diagnostic, not corruption - so
 * sizing to a measured maximum with room over it is safe, and 256 is
 * three times the largest thing ever seen here.
 */
#define MAX_BLKCNTS 256
static unsigned char blkCnts[MAX_BLKCNTS];
static unsigned short blkCntTop = 0;     /* next entry ID to assign (phase 1) */
static unsigned short blkCntIdx = 0;     /* current read index (phase 2) */

/* Stack of entry IDs for active blocks (to know where to store count on exit) */
#define MAX_BLK_DEPTH 32
static unsigned short blkIdStack[MAX_BLK_DEPTH];
static unsigned char blkIdSp = 0;  /* stack pointer */

/* Called when entering a nested block in phase 1 */
void enterBlkCnt(void) {
    if (blkCntTop >= MAX_BLKCNTS || blkIdSp >= MAX_BLK_DEPTH)
        fatal(ER_S_BLK);
    {
#ifdef DEBUG
        if (VERBOSE(V_PHASE1))
            fdprintf(2, "enterBlkCnt[%d] sp=%d\n", blkCntTop, blkIdSp);
#endif
        blkIdStack[blkIdSp++] = blkCntTop++;  /* assign entry ID, push to stack */
    }
}

/* Called when exiting a nested block in phase 1 - store count at entry ID */
void storeBlkCnt(unsigned char n) {
    if (blkIdSp > 0) {
        unsigned short id = blkIdStack[--blkIdSp];
#ifdef DEBUG
        if (VERBOSE(V_PHASE1))
            fdprintf(2, "storeBlkCnt[%d] = %d\n", id, n);
#endif
        blkCnts[id] = n;
    }
}

void pushFuncCnt(unsigned char n) {
    if (funcCntTop >= MAX_FUNCCNTS)
        fatal(ER_S_FN);
    funcCnts[funcCntTop++] = n;
}

unsigned char popFuncCnt(void) {
    if (funcCntIdx < funcCntTop)
        return funcCnts[funcCntIdx++];
    return 0;
}

/* Reset function stmt count read pointer for phase 2 */
void resetFuncIdx(void) {
    funcCntIdx = 0;
}

/* Legacy names for compatibility - now use entry-order indexing */
void pushBlkCnt(unsigned char n) {
    storeBlkCnt(n);  /* store at entry ID from stack */
}

unsigned char popBlkCnt(void) {
    unsigned char r = 0;
    if (blkCntIdx < blkCntTop)
        r = blkCnts[blkCntIdx++];
#ifdef DEBUG
    if (VERBOSE(V_PHASE2))
        fdprintf(2, "popBlkCnt[%d] = %d\n", blkCntIdx-1, r);
#endif
    return r;
}

/* Prepare block counts for phase 2 - just reset read index */
void flipBlkCnts(void) {
    blkCntIdx = 0;
}

void resetBlkCnts(void) {
    blkCntTop = 0;
    blkCntIdx = 0;
    blkIdSp = 0;
}

/*
 * Count storage for streaming AST emission
 * Phase 1 records counts (arg counts, case counts, stmt counts).
 * Phase 2 retrieves them in FIFO order (same order they were pushed).
 */
static unsigned char countBuf[MAX_COUNTS];
static unsigned char countTop = 0;   /* write pointer for pushing */
static unsigned char countIdx = 0;   /* read pointer for popping (FIFO) */

void
pushCount(char c)
{
	if (countTop < MAX_COUNTS) {
		countBuf[countTop] = (unsigned char)c;
#ifdef DEBUG
		if (VERBOSE(V_PHASE1))
			fdprintf(2, "pushCount[%d] = %d %p (verify=%d)\n", countTop, c,
			         &countBuf[countTop], countBuf[countTop]);
#endif
		countTop++;
	}
}

/*
 * Reserve a slot now, fill it later.  Phase 2 reads the queue at
 * switch ENTRY - pre-order - but a switch's case count is only
 * known at its END.  Pushing at the end wrote nested functions'
 * counts in post-order and every nested switch read its
 * neighbour's: the outer table was built with the inner's count,
 * and the inner's leftover cases spilled into the outer, which is
 * where "duplicate case" came from - or a silent misdispatch when
 * the values happened not to collide.  Blocks solved this with
 * enterBlkCnt/storeBlkCnt; switches now do the same.
 */
int
reserveCount(void)
{
	if (countTop < MAX_COUNTS)
		return countTop++;
	return MAX_COUNTS - 1;
}

void
patchCount(int slot, char c)
{
	countBuf[slot] = (unsigned char)c;
}

char
popCount(void)
{
	if (countIdx < countTop) {
		char c = countBuf[countIdx++];
#ifdef DEBUG
		if (VERBOSE(V_PHASE2))
			fdprintf(2, "popCount[%d] = %d %p (top=%d)\n", countIdx - 1, c, &countBuf[countIdx-1], countTop);
#endif
		return c;
	}
#ifdef DEBUG
	fdprintf(2, "popCount: UNDERFLOW! (idx=%d top=%d)\n", countIdx, countTop);
#endif
	return 0;
}

void
resetCounts(void)
{
	countTop = 0;
	countIdx = 0;
}

/*
 * Everything the two phases hand between them, emptied.
 *
 * Phase 1 fills these and phase 2 reads them back by entry order, so
 * the read pointers restart while the write pointers stay - which is
 * right when the phases run over the whole file once each.  Run a
 * span at a time and the write pointers have to go back too, or the
 * next span's phase 2 reads the last span's counts: a block that held
 * sixteen statements was emitted as nine, the rest of it dropped.
 */
void
resetSpanCnts(void)
{
	funcCntTop = 0;
	funcCntIdx = 0;
	resetBlkCnts();
	resetCounts();
	resetSwitch();
}

/* Reset read pointer for phase 2 (preserves pushed values) */
void
resetCountIdx(void)
{
	countIdx = 0;
}

/* Forward declarations */
char *blockname(void);

