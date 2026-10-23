/*
 * disk inode create and manipulate
 *
 * sys/ialloc.c 
 * Changed: <2021-12-24 06:00:17 curt>
 */
#include <types.h>
#include <sys/sys.h>
#include <sys/fs.h>
#include <sys/stat.h>
#include <sys/inode.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <errno.h>

extern char *swin();            /* uio.c: the scratch window */
extern void srel();
extern struct inode ilist[];

extern long seconds;

/*
 * Low water mark block number for the circular search 
 * algorithm in ifill ()
 */

static unsigned lastiblock = 2;

/*
 * Is this inode allocated in core?  A disk inode's image can lag what
 * the kernel has made of it: a file created since the last flush still
 * has its old, free block on disk.  ifill() would find that and put the
 * number back on the free list; when the inode is the directory the
 * allocation is being made in - imkfile() holds it busy across ialloc()
 * - the iget() that pops it blocks on the busy inode and nothing is left
 * to release it.  In core is the truth; never offer a number the kernel
 * already owns.
 */
static int
iincore(inum, dev)
    int inum, dev;
{
    register struct inode *ip;

    for (ip = ilist; ip < ilist + NINODE; ip++)
        if (ip->i_inum == inum && ip->i_dev == dev &&
            (ip->i_mode & IALLOC))
            return (1);
    return (0);
}

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
    register int inum;

    sb = getsb(dev);

    for (;;) {
        ip = 0;              /* no I-node yet. */

        sup = (struct super *)bhold(sb);
        if (sup->s_ninode <= 0) {
            brel();
            if (ifill(sb, dev) == 0)
                break;          /* no more free I-nodes */
            continue;
        }
        inum = sup->s_inode[--sup->s_ninode];
        brel();

        /*
         * The list can outlive the truth it was filled from (it is part
         * of the superblock, and a stale entry rides it across a mount).
         * If the kernel already owns this number, drop it rather than
         * let iget() block on the inode.
         */
        if (iincore(inum, dev))
            continue;

        if ((ip = iget(inum, dev)) == 0) {
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
        mlook(dev)->ifree--;
    }

    bdwrite(sb);
    return ip;
}

/*
 * Put some inumbers in the superblock's inode freelist
 */
ifill(sb, dev)
    struct buf *sb;
    int dev;
{
    register int inum;
    register n, limit;
    unsigned itop;
    register unsigned iblk;
    static struct buf *bp;
    static struct dsknod *dp;
    static struct inode *ip;
    register struct super *sup;
    char *chunk;
    UINT8 chunkseg;

    /*
     * We do a sync here so that what we find later by reading the I-list
     * bears a closer resemblance to reality.
     */

    isync();

    sup = (struct super *)bhold(sb);
    limit = sup->s_isize;         /* Max no. of blocks to search */
    brel();
    iblk = lastiblock;          /* First Inode block to search */
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

        /*
         * Scan the I-list block for free inodes: the block in the
         * scratch window and the superblock in the buffer window, both
         * up at once, so the free numbers can be added as they are found.
         */
        di();
        sup = (struct super *)bhold(sb);
        chunk = bp->data;
        chunkseg = bp->xmem;
        swin(chunkseg);
        dp = (struct dsknod *) ((char *) SCRWIN + (chunk & 0xfff));
        inum = 16 * iblk - 31;
        for (n = 16; n; n--, dp++, inum++)
            if ((dp->d_mode & IALLOC) == 0 && !iincore(inum, dev))
                if (sup->s_ninode < 100)
                    sup->s_inode[sup->s_ninode++] = inum;
        srel();
        brel();
        ei();
        brelse(bp);

        sup = (struct super *)bhold(sb);
        if (sup->s_ninode >= 100) {
            brel();
            break;              /* I-cache full. */
        }
        brel();
    }

    lastiblock = iblk;          /* advice */

    sup = (struct super *)bhold(sb);
    if (sup->s_ninode) {
        brel();
        return 1;             /* We managed to find some I-nodes */
    }
    brel();

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
    sup = (struct super *)bhold(sb);

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
    brel();

    zero(&ip->i_mode, sizeof(struct dsknod));
    ip->i_flags |= IMOD;
    mlook(ip->i_dev)->ifree++;
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
    UINT ind;
    int dev;
    register UINT8 level;
{
    register UINT *p;
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
    for (p = (UINT *)&bp->data[510]; p >= (UINT *)bp->data; p--) {
        UINT bn;

        /*
         * Read the entry under a hold and give the window back before the
         * recursive call: indfree() does its own bread() and bhold(), and
         * that one can sleep.
         */
        bhold(bp);
        bn = *p;
        brel();
        indfree(bn, level - 1, dev);
    }
    brelse(bp);
    bfree(ind, dev);
}

/*
 * Free the tail of an indirect block: the blocks at entries start
 * through 255, and the indirect block itself if start is 0.  level is
 * indfree's - 1 for a single-indirect block, whose entries are data
 * blocks, 2 for a double-indirect block, whose entries are single-
 * indirect blocks.  Freed entries are zeroed, so a block that is kept
 * stays consistent on the disk.
 */
indfreetail(ind, level, start, dev)
    UINT ind;
    UINT8 level;
    UINT start;
    int dev;
{
    register struct buf *bp;
    register UINT *p;
    UINT bn;

    if (ind == 0)
        return;
    if ((bp = bread(ind, dev)) == 0)
        return;
    for (p = (UINT *)&bp->data[510]; p >= (UINT *)&bp->data[start * 2]; p--) {
        bhold(bp);
        bn = *p;
        *p = 0;
        brel();
        if (level == 1)
            bfree(bn, dev);
        else
            indfree(bn, level - 1, dev);
    }
    if (start == 0) {
        brelse(bp);
        bfree(ind, dev);
    } else
        bdwrite(bp);
}

/*
 * Truncate an inode to length bytes, freeing the blocks beyond it.
 * A length of zero is itrunc().
 */
itruncat(ip, length)
    register struct inode *ip;
    UINT32 length;
{
    register int n, dev;
    UINT nblk;
    int index;

    if (ip->i_mode & IIO)
        return;
    if (length == 0) {
        itrunc(ip);
        return;
    }
    dev = ip->i_dev;
    nblk = (length + 511) >> 9;     /* the first block past length */

    if ((ip->i_mode & ILARG) == 0) {
        for (n = 7; n >= nblk; n--) {
            bfree(ip->i_addr[n], dev);
            ip->i_addr[n] = 0;
        }
    } else {
        index = nblk >> 8;
        if (index >= 7) {
            /*
             * The target is inside the double-indirect block.  Free the
             * single-indirects past the containing one, the data blocks
             * past length inside it, and the double-indirect when the
             * whole of it goes.
             */
            struct buf *bp;
            UINT *p;
            UINT containing, bn;
            UINT8 entry = index - 7;
            UINT8 empty = 0;

            if ((bp = bread(ip->i_addr[7], dev)) == 0)
                return;
            p = (UINT *)bp->data;
            containing = p[entry];
            for (n = 255; n > entry; n--) {
                bhold(bp);
                bn = p[n];
                p[n] = 0;
                brel();
                indfree(bn, 1, dev);
            }
            if ((nblk & 0xff) == 0) {
                bhold(bp);
                p[entry] = 0;
                brel();
                empty = (entry == 0);
            }
            if (empty) {
                brelse(bp);
                bfree(ip->i_addr[7], dev);
                ip->i_addr[7] = 0;
            } else
                bdwrite(bp);
            if (nblk & 0xff)
                indfreetail(containing, 1, nblk & 0xff, dev);
            else
                indfree(containing, 1, dev);
        } else {
            /* free the single-indirects past the containing one */
            for (n = 6; n > index; n--) {
                indfree(ip->i_addr[n], 1, dev);
                ip->i_addr[n] = 0;
            }
            /* the data blocks past length in the containing one */
            indfreetail(ip->i_addr[index], 1, nblk & 0xff, dev);
            if ((nblk & 0xff) == 0)
                ip->i_addr[index] = 0;
            /* the whole double-indirect goes */
            indfree(ip->i_addr[7], 2, dev);
            ip->i_addr[7] = 0;
        }
    }
    ip->i_size = length;
    ip->i_flags |= IMOD;
    ip->i_mtime = seconds;
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
