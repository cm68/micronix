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

    swapinit();                 /* Initialize the swap map */

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
    static UINT8 i, seg, success;
    static UINT blk;
    static struct buf *b;

    while (!(block(&lock)))
        ;
    blk = p->swap;
    for (i = 0; i < 17; i++) {
        if (p->mem[i].per != FULL)
            continue;
        seg = p->mem[i].seg;
        b = &swab[i];
        b->flags &= ~(BREAD | BDONE | BSYNC | BERROR);
        b->flags |= flag | (BLOCK | BBUSY);
        b->dev = swapdev;
        b->blk = blk;
        b->count = 4096;
        b->data = (seg & 15) << 12;
        b->xmem = seg >> 4;
        b->error = 0;
        (*biosw[major(swapdev)].strat) (b);
        blk += 8;
    }

    success = 1;
    for (i = 0; i < 17; i++)
        if (p->mem[i].per == FULL) {
            b = &swab[i];
            bwait(b);
            if (b->flags & BERROR) {
                if (b->error == ENXIO)
                    pr("Swap space full\n");
                success = 0;
            }
        }
    brelse(&lock);
    p->time = 0;
    return (success);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
