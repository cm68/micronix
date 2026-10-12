/*
 * swapping and scheduling
 *
 * sys/swap.c 
 * Changed: <2022-01-04 10:47:08 curt>
 */
#include <types.h>
#include <sys/sys.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/con.h>
#include <sys/signal.h>
#include <errno.h>

char memwant = 0;              /* someone wants memory, so look for a
                                 * swapout */
char swapping = 0;             /* run status of swap process */

/*
 * Swap other processes between core and disk. After
 * forking the login process, the power-up process falls
 * into this code, becoming the swapping process.
 */
int
swap()
{
    register struct proc *n;
    extern int mfree();

    /*
     * The swap map was built by swapinit() during system initialization
     * (sys/main_init.c, which says why it is not built here).
     */

    for (;;) {
        if (memwant) {
            if ((n = findout()) && swapout(n)) {
                mfree(n);       /* resets memwant */
            } else {
                goto sleep;
            }
        }

        if (n = findin()) {
            if (mget(n)) {
                swapin(n);
            } else {
                memwant = 1;
            }
        } else {
          sleep:

            swapping = 0;
            sleep(&mfree, PRISWAP);
            swapping = 1;
        }
    }
}

/*
 * Find a process that needs to be swapped in.
 */
int
findin()
{
    static struct proc *p, *n;

    di();
    n = 0;
    for (p = plist; p < plist + NPROC; p++)
        if ((p->mode & (AWAKE | SWAPPED)) == (AWAKE | SWAPPED)
            && (n == 0 || n->time < p->time)
            )
            n = p;
    ei();

    return (n);
}

/*
 * Find someone to swap out
 */
int
findout()
{
    static struct proc *p, *n;

    di();
    n = 0;
    for (p = plist; p < plist + NPROC; p++)
        if ((p->mode & (ALIVE | LOADED | LOCKED)) == (ALIVE | LOADED)
            &&
            p->time >= MINRUN
            &&
            (n == 0
                || (n->mode & AWAKE == p->mode & AWAKE && n->time < p->time)
                || n->mode & AWAKE)
            )
            n = p;
    ei();

    return (n);
}

/*
 * Swap out process p. Called from swap() above and from fork().
 */

swapout(p)
    register struct proc *p;
{
    p->mode &= ~LOADED;

    if ((p->swap = salloc(p->nsegs << 3)) != 0 && swapio(BWRITE, p)) {
        p->mode |= SWAPPED;
        pr("swapout %d: %d segs at %d\n", procid(p), p->nsegs, p->swap);
        return 1;
    } else {
        p->mode |= LOADED;
        send(p, SIGKILL);
        pr("swap error: process %i killed\n", procid(p));
        return 0;
    }
}

/*
 * Swap in process p.
 */

swapin(p)
    register struct proc *p;
{
    p->mode &= ~SWAPPED;

    if (swapio(BREAD, p)) {
        sfree(p->nsegs << 3, p->swap);
        p->mode |= LOADED;
        pr("swapin %d: %d segs at %d\n", procid(p), p->nsegs, p->swap);
        return 1;
    } else {
        mfree(p);
        pr("swap error: process %i hung\n", procid(p));
        return 0;
    }
}

/*
 * Buffer headers for swap io. These are passed to the swap
 * device strategy routine to read or write a large
 * core image at once without impacting regular buffers.
 */
struct buf swab[17] = 0, lock = 0;

/*
 * Read or write core to swap space.
 */

swapio(flag, p)
    int flag;
    register struct proc *p;
{
    static UINT8 i, j, success;
    static UINT blk;
    static struct buf *b;

    while (!(block(&lock)))
        ;
    blk = p->swap;
    success = 1;
    for (i = 0; i < 17; i++) {
        if (p->mem[i].per != FULL)
            continue;
        b = &swab[i];
        b->dev = swapdev;
        b->count = 512;
        /*
         * The core image is not in the buffer window, but the address the
         * driver wants is the same two fields either way: xmem is the 4K
         * segment the transfer starts in, and the low twelve bits of data
         * are the offset inside it (mw.c newrw, hddma.c).  So data is the
         * window's base read as an offset of zero - its low twelve bits
         * are what survives - and the segment carries the rest.  It was a
         * flat 20-bit address split across the two before the buffer
         * pool moved into segments, and at that a segment past 15 landed
         * at the wrong address entirely.
         *
         * A segment is moved one block at a time - 512 bytes, the one
         * shape every driver's strat() accepts - data stepping 512 bytes
         * a pass while xmem names the segment.
         */
        b->xmem = p->mem[i].seg;
        b->data = (char *) BUFWIN;
        b->error = 0;
        for (j = 0; j < 8; j++) {
            b->flags &= ~(BREAD | BDONE | BSYNC | BERROR);
            b->flags |= flag | (BLOCK | BBUSY);
            b->blk = blk;
            (*biosw[major(swapdev)].strat) (b);
            bwait(b);
            if (b->flags & BERROR) {
                if (b->error == ENXIO)
                    pr("Swap space full\n");
                success = 0;
            }
            blk++;
            b->data += 512;
        }
    }
    brelse(&lock);
    p->time = 0;
    return (success);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
