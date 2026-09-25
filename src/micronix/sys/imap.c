/*
 * translate logical block of inode to disk block
 *
 * sys/imap.c 
 * Changed: <2021-12-24 06:00:58 curt>
 */
#include <types.h>
#include <sys/sys.h>
#include <sys/fs.h>
#include <sys/stat.h>
#include <sys/inode.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <errno.h>

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
    register struct inode *ip;
    register int log;
    register int *rap;
{
    int bn, index, dev, ind;
    struct buf *bp;

    *rap = 0;
    if (ip->i_mode & IIO) {       /* don't map io file */
        *rap = log + 1;
        return (log);
    }
    if (log < 0) {              /* just right for 24-bit file size */
        u.error = EFBIG;
        return (0);
    }
    dev = ip->i_dev;
    if ((ip->i_mode & ILARG) == 0) {     /* small addressing */
        if (log < 8)
            return (imapi(ip, log, rap));
        /*
         * Convert to large addressing scheme
         */
        if ((bn = balloc(dev)) == 0)
            return (0);
        bp = bget(bn, dev);
        bhold(bp);
        zero(bp->data, 512);
        copy(ip->i_addr, bp->data, 16);
        brel();
        zero(ip->i_addr, 16);
        ip->i_addr[0] = bn;
        bdwrite(bp);
        ip->i_mode |= ILARG;
        ip->i_flags |= IMOD;
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
    register struct inode *ip;
    int n, *rap;
{
    register int *p;
    UINT bn;

    *rap = 0;
    /*
     * i_addr is UINT[8] on disk - see d_addr in sys/fs.h - and this
     * walks it as ints, taking the entry and the one after it.  Same
     * width and same object; the cast says so, which ccc requires
     * between int * and unsigned short *.
     */
    p = (int *) &ip->i_addr[n];
    if (*p == 0) {
        if ((bn = plug(ip->i_dev)) == 0)
            return 0;
        *p = bn;                /* i_addr is in the (unpaged) inode */
        ip->flags |= IMOD;
    }
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
    register struct buf *bp;
    register int *p;
    UINT bn;

    *rap = 0;
    if (ind == 0)
        return (0);
    if ((bp = bread(ind, dev)) == 0)
        return (0);
    p = (int *)(bhold(bp) + n * sizeof(int));
    if (*p == 0) {
        brel();                     /* plug() reaches balloc() and sleeps */
        if ((bn = plug(dev)) == 0) {
            brelse(bp);
            return (0);
        }
        p = (int *)(bhold(bp) + n * sizeof(int));
        *p = bn;
        bdwrite(bp);
    } else
        brelse(bp);
    if (n < 255)
        *rap = *(p + 1);
    bn = *p;                        /* read it out before the window goes */
    brel();
    return (bn);
}

/*
 * Allocate a block for a file.  Returns the block number, or 0.  The
 * caller installs it: its pointer may be into a paged buffer, and
 * balloc() remaps the BUFSEG window, so nothing here may dereference a
 * buffer address across that call.
 */
UINT
plug(dev)
    int dev;
{
    return balloc(dev);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
