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
 * Note that a "hole" can exist in the middle of a file if no
 * reads or writes have taken place to that particular block.
 * On a write (rw == IWRITE) a zero pointer is filled in by
 * allocating a zero-filled block; on a read (rw == IREAD) a
 * zero pointer maps to 0 and the caller zero-fills the hole, so
 * a read never allocates.  Disk space runs out only on a write.
 * Bugs: should check validity of block numbers,
 * in case the disk goes bad.
 */
UINT
imap(ip, log, rap, rw)
    register struct inode *ip;
    register UINT log;
    register UINT *rap;
    int rw;
{
    UINT bn, ind;
    UINT8 index; int dev;
    struct buf *bp;

    *rap = 0;
    if (ip->i_mode & IIO) {       /* don't map io file */
        *rap = log + 1;
        return (log);
    }
    if (log >= 0x8000) {         /* past a 24-bit file size, 16 MB of blocks */
        u.error = EFBIG;
        return (0);
    }
    dev = ip->i_dev;
    if ((ip->i_mode & ILARG) == 0) {     /* small addressing */
        if (log < 8)
            return (imapi(ip, log, rap, rw));
        if (rw == IREAD)         /* past the direct blocks: a hole */
            return (0);
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
    ind = imapi(ip, min(index, 7), rap, rw);
    if (index >= 7)             /* intermediate fetch for huge file */
        ind = imapb(ind, index - 7, dev, rap, rw);
    return (imapb(ind, log & 0377, dev, rap, rw));
}

/*
 * Get the block number from ip->addr[n]. If it is zero, a write
 * allocates a new block and installs its number; a read returns 0
 * (a hole).
 */
UINT
imapi(ip, n, rap, rw)
    register struct inode *ip;
    int n;
    UINT *rap;
    int rw;
{
    register UINT *p;
    UINT bn;

    *rap = 0;
    /*
     * i_addr is UINT[8] on disk - see d_addr in sys/fs.h - and this
     * walks it as its own type.  *p is the block number; the entry
     * after it is the read-ahead hint.
     */
    p = &ip->i_addr[n];
    if (*p == 0) {
        if (rw == IREAD)        /* a hole reads as zeros, no alloc */
            return (0);
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
UINT
imapb(ind, n, dev, rap, rw)
    UINT ind;
    int n, dev;
    UINT *rap;
    int rw;
{
    register struct buf *bp;
    register UINT *p;
    UINT bn;

    *rap = 0;
    if (ind == 0)
        return (0);
    if ((bp = bread(ind, dev)) == 0)
        return (0);
    p = (UINT *)(bhold(bp) + n * sizeof(UINT));
    if (*p == 0) {
        if (rw == IREAD) {      /* a hole reads as zeros, no alloc */
            brel();
            brelse(bp);
            return (0);
        }
        brel();                     /* plug() reaches balloc() and sleeps */
        if ((bn = plug(dev)) == 0) {
            brelse(bp);
            return (0);
        }
        p = (UINT *)(bhold(bp) + n * sizeof(UINT));
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
