/*
 * imap.c 
 */
#include <sys/sys.h>
#include <sys/inode.h>
#include <sys/buf.h>
#include <sys/proc.h>

/*
 * Return the physical block number for logical block log,
 * using the addressing encoded in the inode. Return 0 on
 * error. Also, set *rap to the number of the next block.
 * Note that a "hole" can exist in the middle of a
 * file if no reads or writes have taken place to that
 * particular block. If necessary, this routine attempts
 * to fill the hole by allocating a zero-filled block.
 * Thus disk space can run out during a read.
 * Bugs: should check validity of block numbers,
 * in case the disk goes bad.
 */
imap(ip, log, rap)
    fast struct inode *ip;
    fast int log;
    fast int *rap;
{
    int bn, index, dev, ind;
    struct buf *bp;

    *rap = 0;
    if (ip->mode & IIO) {       /* don't map io file */
        *rap = log + 1;
        return (log);
    }
    if (log < 0) {              /* just right for 24-bit file size */
        u.error = EFBIG;
        return (0);
    }
    dev = ip->dev;
    if ((ip->mode & ILARGE) == 0) {     /* small addressing */
        if (log < 8)
            return (imapi(ip, log, rap));
        /*
         * Convert to large addressing scheme
         */
        if ((bn = balloc(dev)) == 0)
            return (0);
        bp = bget(bn, dev);
        zero(bp->data, 512);
        copy(ip->addr, bp->data, 16);
        zero(ip->addr, 16);
        ip->addr[0] = bn;
        bdwrite(bp);
        ip->mode |= ILARGE;
        ip->flags |= IMOD;
    }
    index = log >> 8;
    ind = imapi(ip, min(index, 7), rap);
    if (index >= 7)             /* intermediate fetch for huge file */
        ind = imapb(ind, index - 7, dev, rap);
    return (imapb(ind, log & 0377, dev, rap));
}

/*
 * Get the block number from ip->addr[n]. If it is zero,
 * allocate a new block and install its number.
 */
imapi(ip, n, rap)
    fast struct inode *ip;
    int n, *rap;
{
    fast int *p;
    fast int bn;

    *rap = 0;
    p = &ip->addr[n];
    if (*p == 0)
        if (plug(p, ip->dev))
            ip->flags |= IMOD;
    if (n < 7)
        *rap = *(p + 1);
    return (*p);
}

/*
 * Ind is the number of an indirect block. Fetch the block
 * and return the number at block[n]. If necessary, install
 * a new number.
 */
imapb(ind, n, dev, rap)
    int ind, n, dev, *rap;
{
    fast struct buf *bp;
    fast int *p;
    fast int bn;

    *rap = 0;
    if (ind == 0)
        return (0);
    if ((bp = bread(ind, dev)) == NULL)
        return (0);
    p = bp->data + n * sizeof(int);
    if (*p == 0 && plug(p, dev))
        bdwrite(bp);
    else
        brelse(bp);
    if (n < 255)
        *rap = *(p + 1);
    return (*p);
}

/*
 * Allocate a block for a file
 */
plug(p, dev)
    int *p, dev;
{
    static UINT bn;

    if ((bn = balloc(dev)) != 0) {
        if (*p == 0) {
            *p = bn;
            return YES;
        }
        bfree(bn);
    }
    return NO;
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
