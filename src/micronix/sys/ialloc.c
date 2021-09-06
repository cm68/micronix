/*
 * ialloc.c 
 */
#include <types.h>
#include <sys/sys.h>
#include <sys/fs.h>
#include <sys/stat.h>
#include <sys/inode.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <errno.h>

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
    register struct buf *sb;
    register struct super *sup;
    register struct inode *ip;

    sb = getsb(dev);
    sup = sb->data;

    for (;;) {
        ip = 0;              /* no I-node yet. */

        if (sup->s_ninode <= 0 && ifill(sup, dev) == 0) {
            break;              /* no more free I-nodes */
        }

        if ((ip = iget(sup->s_inode[--sup->s_ninode], dev)) == 0) {
            /*
             * this particular I-node not obtainable 
             */
            continue;
        }

        if ((ip->i_mode & IALLOC) == 0) {
            break;              /* I-node is free */
        }

        /*
         * Let go of the node and try again 
         */
        irelse(ip);
    }

    if (ip) {
        zero(&ip->i_mode, sizeof(struct dsknod));
        ip->i_mode = IALLOC;
        ip->i_rtime = seconds;
        ip->i_mtime = seconds;
        ip->i_flags |= IMOD;
        ip->i_count = 0;
        ip->i_mount = 0;
        ip->i_size = 0;
    }

    bdwrite(sb);
    return ip;
}

/*
 * Put some inumbers in the superblock's inode freelist
 */
ifill(sup, dev)
    register struct super *sup;
    int dev;
{
    register int inum;
    register n, limit;
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
    limit = sup->s_isize;         /* Max no. of blocks to search */
    itop = limit + 1;           /* Top I-block number. */

    for (; limit; limit--, iblk++) {
        if (iblk > itop) {
            iblk = 2;           /* wrap around */
        }

        /*
         * unreadable I-list block 
         */
        if ((bp = bread(iblk, dev)) == 0) {
            continue;
        }

        inum = 16 * iblk - 31;

        /*
         * Scan block
         */

        for (n = 16, dp = bp->data; n; n--, dp++, inum++)
            if ((dp->d_mode & IALLOC) == 0)
                if (sup->s_ninode < 100)
                    sup->s_inode[sup->s_ninode++] = inum;

        brelse(bp);

        if (sup->s_ninode >= 100) {
            break;              /* I-cache full. */
        }
    }

    lastiblock = iblk;          /* advice */

    if (sup->s_ninode) {
        return 1;             /* We managed to find some I-nodes */
    }

    u.error = ENOSPC;           /* Closest error I could find */

    /*
     * pr("ifill: no inodes on dev %h\n", dev); 
     */
    return 0;
}

/*
 * Free the inode if count == 0 && nlinks == 0,
 * and release it.
 */
idec(ip)
    register struct inode *ip;
{
    if (ip->i_count == 0 && ip->i_nlink == 0) {
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
    static struct super *sup;

    sb = getsb(ip->i_dev);
    sup = sb->data;

    if (sup->s_ninode < 100) {    /* Fit in super block free list ? */
        sup->s_inode[sup->s_ninode++] = ip->i_inum;
    } else {
        unsigned new;

        /*
         * record new low water mark for ifill ()
         */

        new = (ip->i_inum + 31) / 16;

        if (new < lastiblock)   /* new minimum ? */
            lastiblock = new;
    }

    zero(&ip->i_mode, sizeof(struct dsknod));
    ip->i_flags |= IMOD;
    bdwrite(sb);
}

/*
 * Free all blocks addressed by the inode
 */
itrunc(ip)
    register struct inode *ip;
{
    register int n, dev;

    if (ip->i_mode & IIO)
        return;
    dev = ip->i_dev;
    if (ip->i_mode & ILARG) {
        indfree(ip->i_addr[7], 2, dev);
        for (n = 6; n >= 0; n--)
            indfree(ip->i_addr[n], 1, dev);
    } else
        for (n = 7; n >= 0; n--)
            bfree(ip->i_addr[n], dev);
    zero(ip->i_addr, 16);
    ip->i_size = 0;
    ip->i_mode &= ~ILARG;
    ip->i_flags |= IMOD;
    ip->i_mtime = seconds;
}

/*
 * Free all blocks addressed by the level-th
 * indirect block ind on device dev. Then free ind.
 * (A 0-th level indirect block is a data block.)
 */
indfree(ind, level, dev)
    int ind, dev;
    register int level;
{
    register int *p;
    register struct buf *bp;

    if (ind == 0)
        return;

    if (level == 0) {
        bfree(ind, dev);
        return;
    }
    if ((bp = bread(ind, dev)) == 0) {

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
