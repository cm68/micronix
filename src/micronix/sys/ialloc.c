/*
 * ialloc.c 
 */
#include <sys/sys.h>
#include <sys/inode.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/sup.h>

extern long seconds;

/*
 * Low water mark block number for the circular search 
 * algorithm in ifill ()
 */

static unsigned lastiblock = 2;

/*
 * Allocate an inode from the inode free list.
 * The superblock contains s.ninode (up to 100)
 * free inode numbers. When these are exhausted,
 * the disk ilist is searched for more.
 * On return,    ip->mode  = IALLOC
 *               ip->flags = IBUSY | IMOD
 *               ip->dev   = dev
 *               ip->num   = new inode number.
 */
struct inode *
ialloc(dev)
    int dev;
{
    fast struct buf *sb;
    fast struct sup *sup;
    fast struct inode *ip;

    sb = getsb(dev);
    sup = sb->data;

    for (;;) {
        ip = NULL;              /* no I-node yet. */

        if (sup->ninode <= 0 && ifill(sup, dev) == NO) {
            break;              /* no more free I-nodes */
        }

        if ((ip = iget(sup->inode[--sup->ninode], dev)) == 0) {
            /*
             * this particular I-node not obtainable 
             */
            continue;
        }

        if ((ip->mode & IALLOC) == 0) {
            break;              /* I-node is free */
        }

        /*
         * Let go of the node and try again 
         */
        irelse(ip);
    }

    if (ip) {
        zero(&ip->mode, sizeof(struct dsknod));
        ip->mode = IALLOC;
        ip->rtime = seconds;
        ip->wtime = seconds;
        ip->flags |= IMOD;
        ip->count = 0;
        ip->mount = NULL;
        ip->size = 0;
    }

    bdwrite(sb);
    return ip;
}

/*
 * Put some inumbers in the superblock's inode freelist
 */
ifill(sup, dev)
    fast struct sup *sup;
    int dev;
{
    fast int inum;
    fast n, limit;
    unsigned itop;
    register unsigned iblk;
    static struct buf *bp;
    static struct dsknod *dp;
    static struct inode *ip;

    /*
     * We do a sync here so that what we find later by reading the I-list
     * bears a closer resemblance to reality.
     */

    isync();

    iblk = lastiblock;          /* First Inode block to search */
    limit = sup->isize;         /* Max no. of blocks to search */
    itop = limit + 1;           /* Top I-block number. */

    for (; limit; limit--, iblk++) {
        if (iblk > itop) {
            iblk = 2;           /* wrap around */
        }

        /*
         * unreadable I-list block 
         */
        if ((bp = bread(iblk, dev)) == NULL) {
            continue;
        }

        inum = 16 * iblk - 31;

        /*
         * Scan block
         */

        for (n = 16, dp = bp->data; n; n--, dp++, inum++)
            if ((dp->mode & IALLOC) == 0)
                if (sup->ninode < 100)
                    sup->inode[sup->ninode++] = inum;

        brelse(bp);

        if (sup->ninode >= 100) {
            break;              /* I-cache full. */
        }
    }

    lastiblock = iblk;          /* advice */

    if (sup->ninode) {
        return YES;             /* We managed to find some I-nodes */
    }

    u.error = ENOSPC;           /* Closest error I could find */

    /*
     * pr("ifill: no inodes on dev %h\n", dev); 
     */
    return NO;
}

/*
 * Free the inode if count == 0 && nlinks == 0,
 * and release it.
 */
idec(ip)
    fast struct inode *ip;
{
    if (ip->count == 0 && ip->nlinks == 0) {
        itrunc(ip);
        ifree(ip);
    }

    irelse(ip);
}

/*
 * Add an inode to the freelist
 */
ifree(ip)
    struct inode *ip;
{
    static struct buf *sb;
    static struct sup *sup;

    sb = getsb(ip->dev);
    sup = sb->data;

    if (sup->ninode < 100) {    /* Fit in super block free list ? */
        sup->inode[sup->ninode++] = ip->inum;
    }

    else {
        unsigned new;

        /*
         * record new low water mark for ifill ()
         */

        new = (ip->inum + 31) / 16;

        if (new < lastiblock)   /* new minimum ? */
            lastiblock = new;
    }

    zero(&ip->mode, sizeof(struct dsknod));
    ip->flags |= IMOD;
    bdwrite(sb);
}

/*
 * Free all blocks addressed by the inode
 */
itrunc(ip)
    fast struct inode *ip;
{
    fast int n, dev;

    if (ip->mode & IIO)
        return;
    dev = ip->dev;
    if (ip->mode & ILARGE) {
        indfree(ip->addr[7], 2, dev);
        for (n = 6; n >= 0; n--)
            indfree(ip->addr[n], 1, dev);
    } else
        for (n = 7; n >= 0; n--)
            bfree(ip->addr[n], dev);
    zero(ip->addr, 16);
    ip->size = 0;
    ip->mode &= ~ILARGE;
    ip->flags |= IMOD;
    ip->wtime = seconds;
}

/*
 * Free all blocks addressed by the level-th
 * indirect block ind on device dev. Then free ind.
 * (A 0-th level indirect block is a data block.)
 */
indfree(ind, level, dev)
    int ind, dev;
    fast int level;
{
    fast int *p;
    fast struct buf *bp;

    if (ind == 0)
        return;

    if (level == 0) {
        bfree(ind, dev);
        return;
    }
    if ((bp = bread(ind, dev)) == NULL) {

        /*
         * pr("indfree: bad freelist dev %h\n", dev); 
         */
        return;
    }
    for (p = &bp->data[510]; p >= bp->data; p--)
        indfree(*p, level - 1, dev);
    brelse(bp);
    bfree(ind, dev);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
