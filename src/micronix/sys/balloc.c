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
    sup = sb->data;
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
        copy(fb->data, &sup->s_nfree, 202);       /* PORT */
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
    sup = sb->data;
    if (bcheck(bn, sup, dev) == 0)
        goto done;
    if (sup->s_nfree == 0) {
        sup->s_free[0] = 0;
        sup->s_nfree = 1;
    } else if (sup->s_nfree >= 100) {
        fb = bget(bn, dev);
        copy(&sup->s_nfree, fb->data, 202);       /* PORT */
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
    int bn, dev;
    struct super *sup;
{
    if (sup->s_isize + 1 < bn && bn < sup->s_fsize)
        return 1;
    prdev("Out of range block number", dev);
    return 0;
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
