/*
 * name and scope management, split from type.c so no single
 * translation unit carries both halves of the symbol machinery.
 *
 * scope is handled by pushing names onto the name stack for open,
 * and popping for close.  very simple and fast
 */

#include <stdlib.h>
#include "p1core.h"
#include "p1name.h"
#include "p1stmt.h"
#include "p1lex.h"
#include "p1pblk.h"

#ifdef __GNUC__
char *kindname[] = {
    "prim", "etag", "stag", "utag", "vari", "elem", "tdef", "fdef",
    "bitf", "farg", "locl"
};
#endif

#ifdef DEBUG
int nameAllocCnt = 0;
int nameCurCnt = 0;
int nameHighWater = 0;
#endif

/*
 * basic types = 0
 * global = 1
 * inner blocks > 1
 */
unsigned char lexlevel;
/* block numbering, one entry per open lexical level - see pushScope */
static unsigned char blkIdAt[MAXBLKLVL];
static unsigned char blkIdTop;

/*
 * Locals declared in a nested block, waiting to be hoisted into the
 * function that encloses them.  popScope collects them on the way out,
 * because by the time capLocals runs they are off the lookup chain.
 */
struct local *blockLocals;

/*
 * names unlinked by popScope may still be referenced by pending AST
 * (e->var), so they cannot be freed on the spot.  they go on this
 * graveyard chain instead and cleanupParse frees them at exit.
 */
struct name *deadNames;

/*
 * Pop the current lexical scope
 *
 * Removes names at current level by unlinking from the chain.
 * Names are NOT freed (may be referenced by AST).
 */
void
popScope()
{
    struct name **pp, *n;
    struct local *grabbed, *grabtail;

#ifdef DEBUG
    if (VERBOSE(V_SCOPE)) {
        fdprintf(2, "popScope: %d -> %d (phase=%d)\n", lexlevel, lexlevel - 1, phase);
    }
#endif
    lexlevel--;

    /*
     * In phase 1, skip removal only for file scope (level 1 -> 0).
     * Local function scopes must be cleaned up so that local variables
     * from one function don't conflict with locals from another function.
     * Global names (level 1) are preserved for phase 2 lookup.
     */
    if (phase == 1 && lexlevel == 0)
        return;

    /*
     * Scan entire list and remove names at popped level.
     * Names may be interleaved due to phase 1 preserving globals.
     */
    grabbed = NULL;
    grabtail = NULL;
    pp = &names;
    while ((n = *pp) != NULL) {
        if (n->level > lexlevel) {
#ifdef DEBUG
            if (VERBOSE(V_SYM)) {
                fdprintf(2,"popScope: remove %s%s from lookup\n",
                    n->is_tag ? "tag:":"", nameOf(n->id));
            }
#endif
            /*
             * A local going out of a block inside a function has to be
             * kept: the function still needs a frame slot for it, and
             * capLocals cannot find it once it is off this chain - it
             * only ever sees the level it is called at.  Without this
             * the slot was never allocated, the variable was addressed
             * at (iy+0), and writing to it overwrote the saved IY.
             *
             * Phase 1 only.  Phase 2 re-walks the same declarations and
             * would collect them a second time.
             */
            /*
             * The same two exclusions capLocals makes, and for the
             * same reason it makes them.  A function DECLARED in a
             * block - "char *file, *s_getmsg(), msg[80];", which is
             * how K&R names a routine returning other than int - is
             * an ordinary name carrying a function type, and it was
             * captured here as a local: the frame grew a slot for it
             * and the call became an indirect jump through the slot,
             * "push iy / pop hl / add hl,de / call tramp", so it
             * called whatever the frame happened to hold.  An extern
             * names storage that exists wherever it is written, so it
             * is never a frame slot either.
             *
             * capLocals has refused both since the doprnt bug that
             * found them - it declares _pnum this way - but it only
             * sees the level it is called at.  This walk sees every
             * level below, and had no such refusal, so the same
             * declaration one block deeper still became a slot.
             */
            if (phase == 1 && lexlevel >= 2 && !n->is_tag &&
                !notaslot(n) &&
                (n->kind == kvar || n->kind == klocal)) {
                struct local *copy = mklocal(n);

                if (grabtail)
                    grabtail->next = copy;
                else
                    grabbed = copy;
                grabtail = copy;
            }
            *pp = n->chain;  /* Unlink from list */
            n->chain = deadNames;  /* park on graveyard for cleanupParse */
            deadNames = n;
        } else {
            pp = &n->chain;  /* Move to next */
        }
    }
    /*
     * In front of what is already there, not behind it.  Blocks are
     * left innermost first, so appending would put a block's own
     * locals after the ones declared inside it - and the frame
     * allocator reads the list as a walk down the scope tree, where a
     * block has to come before anything nested in it or the two share
     * space they cannot share.  Going in front restores that order.
     */
    if (grabbed) {
        grabtail->next = blockLocals;
        blockLocals = grabbed;
    }
}

/*
 * Take a name back off the lookup chain and free it.
 *
 * Unlike popScope's graveyard this frees on the spot, because the
 * only caller drops a declaration that nothing refers to - cpp scored
 * the name as appearing once in the whole stream, and that once is
 * the declaration being dropped.  There is no AST pointing at it
 * because there is no mention of it anywhere to have built one.
 */
void
dropName(struct name *n)
{
    struct name **pp;

    for (pp = &names; *pp; pp = &(*pp)->chain) {
        if (*pp == n) {
            *pp = n->chain;
            free(n);
#ifdef DEBUG
            nameCurCnt--;
#endif
            return;
        }
    }
}

/*
 * Push a new lexical scope
 *
 * Increments the lexical level counter to begin a new scope. Names added
 * after this call will be at the new level and will shadow any outer names
 * with the same identifier.
 *
 * Parameters:
 *   n - Scope name for debugging (currently unused)
 */
void
pushScope(char *n)
{
#ifdef DEBUG
    if (VERBOSE(V_SCOPE)) {
        fdprintf(2, "pushScope(%s): %d -> %d (phase=%d)\n", n ? n : "?", lexlevel, lexlevel + 1, phase);
    }
#endif
    lexlevel++;
    /*
     * Number the blocks within a function as they are entered, so a
     * local can say which one declared it.  Level alone cannot: two
     * sibling blocks are both at the same level, and their locals have
     * to share frame space rather than follow one another.
     *
     * Kept per level so that leaving a block returns to the enclosing
     * block's number rather than to the deepest one reached.  The
     * function body is 0 and the count restarts with each function.
     */
    if (lexlevel < MAXBLKLVL)
        blkIdAt[lexlevel] = (lexlevel <= 2) ? (blkIdTop = 0) : ++blkIdTop;
}

/*
 * The block a declaration made now belongs to.
 */
unsigned char
curblk(void)
{
    return lexlevel < MAXBLKLVL ? blkIdAt[lexlevel] : 0;
}

/*
 * Hand the pending block locals back, so the next function starts
 * empty.  Called by capLocals once it has taken the list.
 */
void
clrblklocs(void)
{
    blockLocals = NULL;
}

/*
 * Look up a name in the symbol table chain
 * Traverses from most recent to oldest, first match wins (shadowing)
 */
struct name *
findName(unsigned short id, unsigned char is_tag)
{
    struct name *n;

    for (n = names; n; n = n->chain) {
        if (n->is_tag == is_tag && n->id == id) {
            return n;
        }
    }
    return 0;
}

/*
 * Find duplicate name at current scope level
 */
struct name *
findDup(unsigned short id, unsigned char is_tag)
{
    struct name *n = findName(id, is_tag);
    return (n && n->level == lexlevel) ? n : 0;
}

/*
 * Link name at head of symbol table chain
 */
void
namesAdd(struct name *n)
{
    n->chain = names;
    names = n;
#ifdef DEBUG
    if (VERBOSE(V_SYM)) {
        fdprintf(2,"addName: level:%d %s %s%s\n",
            lexlevel,
            n->kind < sizeof(kindname)/sizeof(kindname[0]) ?
                kindname[n->kind] : "unkn",
            n->is_tag ? "tag:":"", nameOf(n->id));
    }
#endif
}

/*
 * Create and add a new name entry to the symbol table
 */
struct name *
newName(unsigned short id, kind k, struct type *t, unsigned char is_tag)
{
    struct name *n;

    n = findDup(id, is_tag);
    if (n) {
        /* Phase 2: Reuse existing names from phase 1 */
        if (phase == 2)
            return n;
        if (n->sclass & SC_EXTERN)
            return n;
        gripe(ER_D_DN);
        return 0;
    }

    n = (struct name *)galloc(sizeof(*n));
    /* Initialize in struct field order */
    n->id = id;
    n->type = t;
    /* chain set by namesAdd */
    n->kind = k;
    n->level = lexlevel;
    n->is_tag = is_tag;
    namesAdd(n);
#ifdef DEBUG
    nameAllocCnt++;
    nameCurCnt++;
    if (nameCurCnt > nameHighWater)
        nameHighWater = nameCurCnt;
#endif
    return n;
}

/*
 * Add an existing name entry to the symbol table.
 * Returns the canonical entry: when a duplicate already exists, n is
 * freed and the duplicate returned - the caller must use the returned
 * pointer, never n (the AST holds these pointers and usage tracking
 * writes through them; a stale n would scribble on freed memory).
 */
struct name *
addName(struct name *n)
{
    struct name *dup;

    dup = findDup(n->id, n->is_tag);
    if (dup) {
        /* Phase 2: Name already exists from phase 1 */
        if (phase == 2) {
            free(n);
            return dup;
        }
        if (dup->sclass & SC_EXTERN) {
            dup->sclass = n->sclass;
            if (n->u.init)
                dup->u.init = n->u.init;
            free(n);
            return dup;
        }
        gripe(ER_D_DN);
        return n;
    }

    n->level = lexlevel;
    namesAdd(n);
    return n;
}
