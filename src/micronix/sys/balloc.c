/*
 * balloc.c 
 */
#include <types.h>
#include <sys/sys.h>
#include <sys/buf.h>
#include <sys/sup.h>
#include <sys/proc.h>

/*
 * Allocate a disk block from the device freelist.
 * See Unix Programmer's Manual, section V, File System
 * for a description of the algorithm.
 */
balloc(dev)
    int dev;
{
    fast struct buf *sb;
    static struct buf *fb;
    fast struct sup *sup;
    fast int bn;

    sb = getsb(dev);
    sup = sb->data;
    if (sup->flock) {           /* mounted read-only */
        u.error = EROFS;
        return (0);
    }
    if (sup->nfree <= 0
        || (bn = sup->free[--sup->nfree]) == 0 || bcheck(bn, sup, dev) == NO)
        goto full;

    if (sup->nfree == 0) {
        if ((fb = bread(bn, dev)) == NULL)
            goto bad;
        copy(fb->data, &sup->nfree, 202);       /* PORT */
    } else {
        fb = bget(bn, dev);
    }

    zero(fb->data, 512);
    bdwrite(fb);
    bdwrite(sb);
    return (bn);
  full:
    u.error = ENOSPC;
  bad:
    sup->nfree = 0;
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
    fast struct buf *sb, *fb;
    fast struct sup *sup;

    if (bn == 0)
        return;
    sb = getsb(dev);
    sup = sb->data;
    if (bcheck(bn, sup, dev) == NO)
        goto done;
    if (sup->nfree == 0) {
        sup->free[0] = 0;
        sup->nfree = 1;
    } else if (sup->nfree >= 100) {
        fb = bget(bn, dev);
        copy(&sup->nfree, fb->data, 202);       /* PORT */
        bawrite(fb);
        sup->nfree = 0;
    }
    sup->free[sup->nfree++] = bn;
  done:
    bdwrite(sb);
}

/*
 * Check the validity of a file block number
 */
bcheck(bn, sup, dev)
    int bn, dev;
    struct sup *sup;
{
    if (sup->isize + 1 < bn && bn < sup->fsize)
        return YES;
    prdev("Out of range block number", dev);
    return NO;
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
