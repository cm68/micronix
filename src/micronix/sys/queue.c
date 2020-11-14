/*
 * queue.c 
 */

/*
 * queue protocol
 *
 *      If first or last is NULL, queue is empty.
 *
 *      first, last % 16 is in the range 2 - 15 (inclusive). (or 0)
 *      all routines must preserve this status.
 *
 *      A new char. enters the queue at "last", the end of the line.
 *      Chars. leave the queue at first, "first in line".
 *
 *      When first crosses the boundary to 0 (mod 16)
 *      the cblock it is leaving shall be freed.
 *
 *      When last crosses the edge, to 0 (mod 16)
 *      attempt to allocate a new cblock.
 *
 *      "first" always points to the char. whose turn it is next to
 *      be taken.
 *
 *      "last" points to the next free spot.
 *
 *      If at any point you fail in an alloc, drain the queue.
 *
 *      A queue shall be no longer than TTYHOG
 */

#include <sys.h>
#include <tty.h>
#include <proc.h>

#define calign(a)	 (((unsigned int) (a) - 1) & ~15)       /* PORT */

char clist[CSIZE] = { 0 };

struct cblock *cfree = NULL;    /* free list */

/*
 * Drain all queues and flush the output.
 * Called from sig().
 */
drainall(tty)
    struct tty *tty;
{
    if (tty) {
        drainques(tty);
        ustart(tty);            /* output interrupt to finish low level */
    }
}

/*
 * Drain all queues.
 * Called above and from ttyout when tty is closed
 * and output que empties.
 */
drainques(tty)
    fast struct tty *tty;
{
    drain(&tty->rawque);
    drain(&tty->cokque);
    drain(&tty->outque);

    di();
    tty->nbreak = 0;
    tty->nextc = 0;
    ei();
}

/*
 * Empty a queue, discarding its contents
 */
drain(q)
    fast struct que *q;
{
    static struct cblock *x, *y;

    di();

    x = q->first;
    y = q->last;

    if (x && y) {
        x = calign(x);
        y = calign(y);

        y->next = cfree;
        cfree = x;
    }

    qinit(q);

    ei();
}

/*
 * drain(q) fast struct que *q; { di(); while (q->count) getc(q); ei(); } 
 */

fillque(q)
    struct que *q;
{
    static char *from;
    static unsigned cleft, qspace, togo, nfill;

    /*
     * extern char resched; 
     */

    /*
     * figure out how many to copy
     *      Copy as much as you can.
     *      Copy no more that u.count
     *      Copy not so much as to make q->count exceed TTYHOG
     */

    di();

    cleft = u.count;            /* no. left to copy */

    qspace = TTYHOG - q->count; /* space in the queue */

    if (cleft > qspace)
        cleft = qspace;         /* copy only what will fit */

    from = u.base;

    while (cleft) {             /* still something to do */
        if (((int) (q->last) & 15) == 0)        /* space needed */
            if (!qalloc(q)) {
                break;
            }

        togo = 16 - ((int) (q->last) & 15);     /* whole cblock */

        if (cleft < togo)       /* take only that requested */
            togo = cleft;

        copyin(from, q->last, togo);

        from += togo;
        cleft -= togo;
        q->last += togo;
        q->count += togo;
    }

    nfill = from - u.base;

    u.count -= nfill;
    u.base = from;

    /*
     * from = &u.p->cpu; if (*from > nfill) *from -= nfill; else *from = 1; 
     */

    /*
     * resched = YES; 
     */

    ei();
}

/*
 * bulk copy from que to user space
 */

sendque(q)
    register struct que *q;
{
    static char *to;
    static unsigned cleft, nsent, togo;

    di();

    to = u.base;

    cleft = u.count;            /* as many as sys. is asking for */

    if (cleft > q->count)       /* copy only as much as is in the queue */
        cleft = q->count;

    while (cleft) {             /* still something to do */
        if (((int) (q->first) & 15) == 0)       /* need to free up */
            if (!qfree(q))
                break;

        togo = 16 - ((int) (q->first) & 15);    /* no. this block */

        if (cleft < togo)       /* no more than requested */
            togo = cleft;

        copyout(q->first, to, togo);

        q->first += togo;
        to += togo;
        cleft -= togo;
        q->count -= togo;
    }

    nsent = (to - u.base);

    u.base = to;
    u.count -= nsent;

    if (q->count == 0)
        qrelse(q);

    ei();
}

putc(q, c)
    register struct que *q;
    register char c;
{
    di();

    if ((q->count < TTYHOG)
        && (((int) (q->last) & 15) || qalloc(q))) {
        q->count++;
        *q->last++ = c;
    } else {
        c = -1;
    }

    ei();

    return c;
}

getc(q)
    register struct que *q;
{
    fast UINT c;

    di();

    if (q->count && (((int) (q->first) & 15) || qfree(q))) {
        q->count--;
        c = *q->first++;
    } else {
        c = -1;
    }

    if (q->count == 0)
        qrelse(q);

    ei();

    return c;
}

static
qalloc(q)
    register struct que *q;
{
    static struct cblock *new, *old;

    if (q->count == 0 && q->last) {
        new = q->last;
        new--;
        q->last = NULL;
    }

    else {
        if (!cfree)
            return NO;

        new = cfree;            /* allocate */
        cfree = cfree->next;
    }

    if (q->last) {              /* link */
        old = q->last;
        old--;
        old->next = new;
    } else {
        q->first = new->block;  /* first time */
    }

    q->last = new->block;
    new->next = NULL;
    return YES;
}

static
qrelse(q)
    fast struct que *q;
{
    static struct cblock *b;

    b = q->first;
    if (b) {
        b = ((int) b - 1) & ~15;
        b++;
        q->first = b;
        q->last = b;
        qfree(q);
    }
}

static
qfree(q)
    register struct que *q;
{
    static struct cblock *new, *old;

    old = q->first;             /* the cblock to be freed */

    if (!old) {
        qinit(q);
        return NO;
    }

    old--;

    new = old->next;            /* save pointer */

    old->next = cfree;          /* free cblock */
    cfree = old;

    if (new) {
        q->first = new->block;
        return YES;
    }

    else {
        qinit(q);
        return NO;
    }
}

qinit(q)
    register struct que *q;
{
    q->count = 0;
    q->first = 0;
    q->last = 0;
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
