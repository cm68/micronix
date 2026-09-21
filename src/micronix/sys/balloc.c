/*
 * allocate/free a block from the freelist
 *
 * sys/balloc.c 
 * Changed: <2021-12-23 18:14:37 curt>
 */
#include <types.h>
#include <sys/sys.h>
#include <sys/buf.h>
#include <sys/fs.h>
#include <sys/proc.h>
#include <errno.h>

/*
 * Assert the superblock's free list holds only valid block numbers.
 * Called right after a refill (a fresh chunk was copied in) and right
 * before a spill (the in-core list is about to be written out) - the
 * two edges where a bad block number enters or leaves the list.  A bad
 * entry means the list was corrupted earlier; panic stops the machine
 * so the trace shows how the bad block got in.
 */
checkfreelist(sup)
    struct super *sup;
{
    register int i;
    register UINT bn;

    if (sup->s_nfree > 100) {
        pr("freelist: nfree %d out of range\n", sup->s_nfree);
        panic("freelist nfree out of range");
    }
    for (i = 0; i < sup->s_nfree; i++) {
        bn = sup->s_free[i];
        if (bn == 0)            /* end-of-list sentinel */
            continue;
        if (sup->s_isize + 1 < bn && bn < sup->s_fsize)
            continue;
        pr("freelist: bad block %d (0%o) at %d/%d [isize %d fsize %d]\n",
            bn, bn, i, sup->s_nfree, sup->s_isize, sup->s_fsize);
        panic("freelist bad block");
    }
}

/*
 * Dump the superblock's free list: the count and every block number.
 * Called right after a refill (the list was just filled from a chunk)
 * and right before a spill (the list is about to be written out), so
 * the two edges of the chain can be compared block by block.
 */
dumpsb(sup, what)
    struct super *sup;
    char *what;
{
    register int i;

    pr("dumpsb(%s) nfree %d:", what, sup->s_nfree);
    for (i = 0; i < sup->s_nfree; i++)
        pr(" %d", sup->s_free[i]);
    pr("\n");
}

/*
 * Dump a buffer's identity: which block it claims and which physical
 * slot (data pointer + xmem segment) it occupies.  The slot is what the
 * "same physical data" question hangs on: two buffers with the same
 * data/xmem are the same 512 bytes no matter what blk says.  flags bit
 * 0x01 (BREAD) is set only if a real disk read happened; 0x10 (BDONE)
 * alone means the buffer was served from cache.
 */
dumpbuf(b, what)
    struct buf *b;
    char *what;
{
    pr("buf(%s) blk %d dev %d data %x xmem %d flags %x idx %d\n",
        what, b->blk, b->dev, (int)b->data, b->xmem, b->flags, (int)(b - blist));
}

/*
 * Allocate a disk block from the device freelist.
 * See Unix Programmer's Manual, section V, File System
 * for a description of the algorithm.
 */
balloc(dev)
    int dev;
{
    register struct buf *sb;
    static struct buf *fb;
    register struct super *sup;
    register int bn;

    sb = getsb(dev);
    bwin(sb->xmem);
    sup = (struct super *)sb->data;
    if (sup->s_flock) {           /* mounted read-only */
        u.error = EROFS;
        return (0);
    }
    if (sup->s_nfree <= 0
        || (bn = sup->s_free[--sup->s_nfree]) == 0 || bcheck(bn, sup, dev) == 0)
        goto full;

    if (sup->s_nfree == 0) {
        if ((fb = bread(bn, dev)) == 0)
            goto bad;
#ifdef FREEDEBUG
        dumpbuf(fb, "refill");
#endif
        bcopy(fb, 0, sb, 4, 202);       /* PORT: refill free list */
        sup = (struct super *)bsup(sb); /* bcopy left fb in the 0xf000 window */
        checkfreelist(sup);
#ifdef FREEDEBUG
        dumpsb(sup, "fill");
        pr("refill blk %d: count %d e0 %d e1 %d e2 %d\n",
            bn, ((UINT *)fb->data)[0], ((UINT *)fb->data)[1],
            ((UINT *)fb->data)[2], ((UINT *)fb->data)[3]);
#endif
    } else {
        fb = bget(bn, dev);
        bwin(fb->xmem);
    }

    zero(fb->data, 512);
    bdwrite(fb);
    bdwrite(sb);
    return (bn);
  full:
    u.error = ENOSPC;
  bad:
    bwin(sb->xmem);            /* re-map: bread() above may have slept */
    sup->s_nfree = 0;
    prdev("No more space", dev);
    bdwrite(sb);
    return (0);
}

/*
 * Add block number bn to the freelist on device dev.
 */
bfree(bn, dev)
    int bn;
    int dev;
{
    register struct buf *sb, *fb;
    register struct super *sup;

    if (bn == 0)
        return;
    sb = getsb(dev);
    bwin(sb->xmem);
    sup = (struct super *)sb->data;
    if (bcheck(bn, sup, dev) == 0)
        goto done;
    if (sup->s_nfree == 0) {
        sup->s_free[0] = 0;
        sup->s_nfree = 1;
    } else if (sup->s_nfree >= 100) {
        checkfreelist(sup);
#ifdef FREEDEBUG
        dumpsb(sup, "spill");
#endif
        fb = bget(bn, dev);
#ifdef FREEDEBUG
        dumpbuf(fb, "spill");
#endif
        bcopy(sb, 4, fb, 0, 202);       /* PORT: spill free list */
        bawrite(fb);
        sup->s_nfree = 0;
    }
    sup->s_free[sup->s_nfree++] = bn;
  done:
    bdwrite(sb);
}

/*
 * Check the validity of a file block number
 */
bcheck(bn, sup, dev)
    UINT bn;
    int dev;
    struct super *sup;
{
    if (sup->s_isize + 1 < bn && bn < sup->s_fsize)
        return 1;
    pr("bcheck: bn %d (0%o) out of range [isize %d fsize %d]\n",
        bn, bn, sup->s_isize, sup->s_fsize);
    di();
    for (;;)
        ;
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
