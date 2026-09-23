/*
 * block i/o 
 *
 * sys/uio.c 
 * Changed: <>
 */
#include <types.h>
#include <sys/sys.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/con.h>
#include <sys/fs.h>
#include <errno.h>

extern long seconds;            /* see clock.c */
extern UINT8 nbuf;               /* initialized in binit(), main.c */
extern struct buf *btop;            /* ditto */
extern UINT8 map0[], image0[];   /* MMU map registers (uhdr.s) */
extern int copy();               /* leaf mem.s: kernel-to-kernel copy */

/*
 * Hash the (dev, blk) pair into a bucket.  Consecutive block numbers have
 * consecutive low bits, so blk's low bits spread evenly; dev is folded in
 * so two block devices don't always collide.
 */
#define NHASH   64

static struct buf *bhash[NHASH];

static int
hash(blk, dev)
    UINT blk, dev;
{
    return ((blk ^ dev) & (NHASH - 1));
}

/*
 * Remove a buffer from its hash bucket, called before its blk/dev change.
 */
static void
unhash(b)
    struct buf *b;
{
    struct buf **pp;
    int h;

    h = hash(b->blk, b->dev);
    for (pp = &bhash[h]; *pp; pp = &(*pp)->b_hash) {
        if (*pp == b) {
            *pp = b->b_hash;
            b->b_hash = 0;
            return;
        }
    }
}

/*
 * Get a buffer for the block
 */
struct buf *
bget(blk, dev)
    UINT blk, dev;
{
    register struct buf *b, *f;
    int h;

    h = hash(blk, dev);

  loop:
    /*
     * search the hash bucket for the block
     */
    for (b = bhash[h]; b; b = b->b_hash) {
        if (b->blk == blk && b->dev == dev) {
            if (block(b))
                return (b);
            else
                goto loop;      /* busy: retry */
        }
    }
    /*
     * block not found: pick the least-recently-used free buffer
     */
    f = 0;
    for (b = blist; b < btop; b++) {
        if (b->flags & (BBUSY | BLOCK))
            continue;
        if (f == 0 || b->time < f->time)
            f = b;
    }
    if ((b = f) == 0)        /* no available buffers */
        goto loop;
    if (!block(b))           /* raced: someone else took it, retry */
        goto loop;
    if (b->flags & BDELWRI) {
        bwrite(b);
        goto loop;
    }
    unhash(b);               /* drop it from its old blk's bucket */
    b->blk = blk;
    b->dev = dev;
    b->flags &= ~BDONE;
    b->b_hash = bhash[h];
    bhash[h] = b;

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
        return 0;
    }
    b->flags |= BBUSY;
    ei();
    return 1;
}

/*
 * Read the indicated block (if necessary)
 */
struct buf *
bread(blk, dev)
    UINT blk, dev;
{
    register struct buf *b;

    b = bget(blk, dev);

    if ((b->flags & (BDONE | BERROR)) != BDONE) {
        b->flags |= BREAD | BSYNC;
        strat(b);
        bwait(b);
    }

    if (geterror(b)) {
        brelse(b);
        return 0;
    } else {
        return (b);
    }
}

/*
 * Read a block asyncronously. Used for read-ahead by iread (fio.c)
 */
aread(blk, dev)
    UINT blk, dev;
{
    register struct buf *b;

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
    UINT dev;
{
    struct buf *b;

    if ((b = bread(1, dev)) == 0)
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
    register struct buf *b;
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
    register struct buf *b;
{
    if (b == 0 || (b->flags & BBUSY) == 0)
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
    static UINT count = 0;
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
    b->forw = 0;
    b->back = 0;
    (*biosw[bmajor(b->dev)].strat) (b);
}

/*
 * Access a block device open routine.
 */
bopen(dev, mode)
    UINT dev, mode;
{
    (*biosw[bmajor(dev)].open) (dev, mode);
}

/*
 * Access a block device close routine.
 */
bclose(dev, mode)
    UINT dev, mode;
{
    (*biosw[bmajor(dev)].close) (dev, mode);
}

/*
 * Return the major device number.
 */
unsigned
bmajor(dev)
    UINT dev;
{
    static UINT maj;

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
    UINT dev;
{
    register struct buf *b;

  loop:
    for (b = blist; b < btop; b++)
        if (dev == b->dev) {
            if (!block(b))
                goto loop;
            if (b->flags & BDELWRI)
                bwrite(b);
            else
                brelse(b);
            unhash(b);
            bzero(b);
        }
}

/*
 * Zero a buffer
 */
bzero(b)
    struct buf *b;
{
    char *data;
    UINT8 xmem;

    /* data/xmem are the buffer's fixed window address and segment, set
     * once by binit()/expand_bufs(); preserve them across the zero. */
    data = b->data;
    xmem = b->xmem;
    zero(b, sizeof(*b));
    b->data = data;
    b->xmem = xmem;
}

/*
 * Sync the blist. Called from sync().
 */
bsync()
{
    register struct buf *b;
    register struct super *s;

    b = getsb(rootdev);
    bwin(b->xmem);
    s = (struct super *)b->data;

    if (!s->s_flock) {            /* not read-only */
        di();
        s->s_time = seconds;
        ei();
        bdwrite(b);
    } else
        brelse(b);

    for (b = blist; b < btop; b++)
        if ((b->flags & (BBUSY | BDELWRI)) == BDELWRI)
            bawrite(b);
}

/*
 * Count the buffers still carrying a delayed write - the length of the
 * dirty list.  A busy buffer is being written, not waiting, so only the
 * not-busy delayed-write buffers count, as in bsync().
 */
int
ndirty()
{
    register struct buf *b;
    register int n = 0;

    for (b = blist; b < btop; b++)
        if ((b->flags & (BBUSY | BDELWRI)) == BDELWRI)
            n++;
    return n;
}

/*
 * Write every delayed-write buffer out synchronously.  bwrite waits for
 * the write to finish, so across the reboot() loop the count really does
 * drain to zero rather than just being issued.
 */
int
bdrain()
{
    register struct buf *b;
    register int n = 0;

    for (b = blist; b < btop; b++)
        if ((b->flags & (BBUSY | BDELWRI)) == BDELWRI) {
            bwrite(b);
            n++;
        }
    return n;
}

/*
 * Map a buffer's segment into the 0xf000 window so its data (0xf000 +
 * offset) becomes reachable.  b->xmem carries the segment; segment 15 is
 * the window, so its map register is map0[2*15] == map0[30].
 */
bwin(seg)
    int seg;
{
    map0[30] = image0[30] = seg;
}

/*
 * Map a buffer into the 0xd000 window (the copyin/copyout page, free while
 * we are not touching process address space) and return its address there.
 * Used for the superblock so it stays reachable alongside another buffer in
 * 0xf000 (e.g. ifill's inode-block scan).
 */
bsup(sb)
    struct buf *sb;
{
    map0[26] = image0[26] = sb->xmem;
    return (0xd000 + ((int)sb->data & 0xfff));
}

/*
 * Copy between two buffers in (possibly) different segments.  Maps the
 * source into 0xf000 and the destination into 0xd000 - the copyin/copyout
 * page, which is free here because we are not touching process address
 * space during superblock/freelist handling - then ldir's between the two
 * windows.  soff/doff are byte offsets within the buffers.
 */
bcopy(sb, soff, db, doff, count)
    struct buf *sb, *db;
    int soff, doff, count;
{
    di();
    map0[30] = image0[30] = sb->xmem;
    map0[26] = image0[26] = db->xmem;
    copy(sb->data + soff,
        (char *)(0xd000 + ((int)db->data & 0xfff) + doff), count);
    ei();
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
