/*
 * uio.c 
 */
#include <sys/sys.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/con.h>
#include <sys/sup.h>

extern long seconds;            /* see clock.c */
UCHAR nbuf;                     /* initialized in binit(), main.c */
char (*buffer)[512];            /* ditto */
struct buf *btop;               /* ditto */

/*
 * Get a buffer for the block
 */
struct buf *
bget(blk, dev)
    uns blk, dev;
{
    fast struct buf *b, *f;

  loop:
    f = NULL;
    for (b = blist; b < btop; b++) {
        if (b->blk == blk && b->dev == dev) {
            if (block(b))
                return (b);
            else
                goto loop;
        }
        if (b->flags & (BBUSY | BLOCK))
            continue;
        if (f == NULL || b->time < f->time)
            f = b;
    }
    /*
     * block not found
     */
    if ((b = f) == NULL)        /* no available buffers */
        goto loop;
    block(b);
    if (b->flags & BDELWRI) {
        bwrite(b);
        goto loop;
    }
    b->blk = blk;
    b->dev = dev;
    b->flags &= ~BDONE;

    /*
     * zero(b->data, 512); 
     */
    return (b);
}

/*
 * Lock a block if possible.
 */
block(b)
    struct buf *b;
{
    di();
    if (b->flags & BBUSY) {
        b->flags |= BWANT;
        sleep(b, PRIBIO);
        return NO;
    }
    b->flags |= BBUSY;
    ei();
    return YES;
}

/*
 * Read the indicated block (if necessary)
 */
struct buf *
bread(blk, dev)
    uns blk, dev;
{
    fast struct buf *b;

    b = bget(blk, dev);

    if ((b->flags & (BDONE | BERROR)) != BDONE) {
        b->flags |= BREAD | BSYNC;
        strat(b);
        bwait(b);
    }

    if (geterror(b)) {
        brelse(b);
        return NULL;
    } else {
        return (b);
    }
}

/*
 * Read a block asyncronously. Used for read-ahead by iread (fio.c)
 */
aread(blk, dev)
    uns blk, dev;
{
    fast struct buf *b;

    for (b = blist; b < btop; b++)
        if (b->blk == blk && b->dev == dev)
            return;

    b = bget(blk, dev);
    b->flags &= ~BSYNC;
    b->flags |= BREAD;
    strat(b);
}

/*
 * Read the super-block (block 1) on the device.
 */
struct buf *
getsb(dev)
    uns dev;
{
    struct buf *b;

    if ((b = bread(1, dev)) == NULL)
        panic("cant get superblock");   /* should be locked in core */
    return (b);
}

/*
 * Write out the buffer synchronously and release it. Geterror
 * could be included here, but so far nothing needs it, and it
 * would complicate some routines where the current user is not
 * responsible for the write (such as bget above). The calling
 * code can still look at the error (even though the buffer has
 * been released) since process switching cannot occurr inside
 * the kernel except across sleep.
 */
bwrite(b)
    fast struct buf *b;
{
    b->flags |= BSYNC;
    b->flags &= ~BREAD;
    strat(b);
    bwait(b);
    brelse(b);
}

/*
 * Write out the buffer asynchronously.
 * There is no error reporting, except
 * device messages to console. Iodone
 * will release the buffer.
 */
bawrite(b)
    struct buf *b;
{
    b->flags &= ~(BSYNC | BREAD);
    strat(b);
}

/*
 * Mark the buffer for later writeout and release it.
 */
bdwrite(b)
    struct buf *b;
{
    b->flags |= (BDELWRI | BDONE);
    brelse(b);
}

/*
 * Release the buffer
 */
brelse(b)
    fast struct buf *b;
{
    if (b == NULL || (b->flags & BBUSY) == 0)
        return;
    if (b->flags & BWANT)
        wakeup(b);
    b->flags &= ~(BBUSY | BWANT);
    b->time = btime();
}

/*
 * Increment a count and return it as a reference "time".
 * Called from brelse. When the count recycles, reset all
 * buffer times to zero.
 */
unsigned
btime()
{
    static uns count = 0;
    static struct buf *b;

    di();
    if (++count == 0) {
        for (b = blist; b < btop; b++)
            b->time = 0;
    }
    ei();
    return (count);
}

/*
 * Report the max block io queue length. Debugging.
 */
char quelen = 0;

/*
 * Sort a queue of block I/O requests into a reasonable order.
 * For use by strategy routines. Must be di()'ed.
 */
bsort(h, b)
    struct buf *h, *b;
{
    static struct buf *p, *f;
    static UINT bc, pc, fc;
    static char n;

    bc = b->cyl;
    n = 1;
    for (p = h; f = p->forw; p = f) {
        n++;
        pc = p->cyl;
        fc = f->cyl;
        if ((pc <= bc && bc <= fc) || (pc >= bc && bc >= fc))
            break;
    }
    if (n > quelen)
        quelen = n;
    b->forw = f;
    p->forw = b;
}

/*
 * Access the strategy routine to read or write a buffer
 */
strat(bp)
    struct buf *bp;
{
    static struct buf *b;

    b = bp;
    b->flags |= BBUSY;
    b->flags &= ~(BDONE | BERROR);
    b->error = 0;
    b->count = 512;
    b->forw = NULL;
    b->back = NULL;
    (*biosw[bmajor(b->dev)].strat) (b);
}

/*
 * Access a block device open routine.
 */
bopen(dev, mode)
    uns dev, mode;
{
    (*biosw[bmajor(dev)].open) (dev, mode);
}

/*
 * Access a block device close routine.
 */
bclose(dev, mode)
    uns dev, mode;
{
    (*biosw[bmajor(dev)].close) (dev, mode);
}

/*
 * Return the major device number.
 */
unsigned
bmajor(dev)
    uns dev;
{
    static uns maj;

    if ((maj = dev >> 8) >= nbdev)
        maj = 0;                /* nodev, sets b->error = ENXIO */
    return (maj);
}

/*
 * Called at io completion time. Adjust flags.
 * If io was synchronous, issue wakeup.
 * If io was asynchronous, release buffer.
 */
iodone(bp)
    struct buf *bp;
{
    static struct buf *b;
    static UINT dev;

    b = bp;
    dev = b->dev;
    b->flags |= BDONE;
    b->flags &= ~BDELWRI;
    if (b->flags & BERROR)
        perror(b);
    if (b->flags & BSYNC)
        wakeup(b);
    else
        brelse(b);              /* brelse handles wakeup on BWANT */
}

/*
 * Print an I/O error message at the console
 */
perror(b)
    struct buf *b;
{
    if (b->error == ENXIO || (b->flags & (BREAD | BSYNC)) == BREAD)
        return;
    pr((b->flags & BREAD) ? "Read" : "Write");
    pr(" error block %i", b->blk);
    prdev("", b->dev);
}

/*
 * Wait for io completion on buffer
 */
bwait(b)
    struct buf *b;
{
    for (;;) {
        di();
        if (b->flags & BDONE)
            break;
        b->flags |= BWANT;
        sleep(b, PRIBIO);
    }
    ei();
}

/*
 * Set u.error from the buffer
 */
geterror(b)
    struct buf *b;
{
    return (u.error = (b->flags & BERROR) ? ((b->error) ? b->error : EIO)
        : 0);
}

/*
 * Flush dev's blocks from the blist. Called from iclose.
 * Note that iclose holds the inode, so there is no
 * danger that anyone else will access the device while
 * this is going on (as long as there is only one inode).
 */
bflush(dev)
    uns dev;
{
    fast struct buf *b;

  loop:
    for (b = blist; b < btop; b++)
        if (dev == b->dev) {
            if (!block(b))
                goto loop;
            if (b->flags & BDELWRI)
                bwrite(b);
            else
                brelse(b);
            bzero(b);
        }
}

/*
 * Zero a buffer
 */
bzero(b)
    struct buf *b;
{
    zero(b, sizeof(*b));
    b->data = buffer[b - blist];
    b->xmem = KERNEL;
}

/*
 * Sync the blist. Called from sync().
 */
bsync()
{
    fast struct buf *b;
    fast struct sup *s;

    b = getsb(rootdev);
    s = b->data;

    if (!s->flock) {            /* not read-only */
        di();
        s->time = seconds;
        ei();
        bdwrite(b);
    } else
        brelse(b);

    for (b = blist; b < btop; b++)
        if ((b->flags & (BBUSY | BDELWRI)) == BDELWRI)
            bawrite(b);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
