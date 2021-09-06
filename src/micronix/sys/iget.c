/*
 * iget.c 
 */
#include <types.h>
#include <sys/sys.h>
#include <sys/fs.h>
#include <sys/stat.h>
#include <sys/inode.h>
#include <sys/mount.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <errno.h>

struct inode ilist[];

/*
 * Return a pointer to a locked inode.
 * Errors: bad inumber, ilist is full.
 */
struct inode *
iget(num, dev)
    int num, dev;
{
    register struct inode *i, *f;

  loop:
    f = 0;
    for (i = ilist; i < ilist + NINODE; i++) {
        if (i->i_inum == num && i->i_dev == dev) {
            ilock(i);
            if (i->i_mount) {
                dev = i->i_mount->dev;
                num = 1;        /* root inumber */
                irelse(i);
                goto loop;
            }
            return (i);
        }
        if (i->count > 0 || i->flags & IBUSY)
            continue;
        if (f == 0 || f->time > i->time)
            f = i;
    }
    if ((i = f) == 0) {
        u.error = ENFILE;
        return 0;
    }
    ilock(i);
    if ((i->flags & IMOD) && !iio(IWRITE, i)) {
        irelse(i);
        return 0;
    }
    i->i_inum = num;              /* mark it for other searchers */
    i->i_dev = dev;
    i->i_count = i->i_mount = 0;
    if (!iio(IREAD, i)) {
        zero(i, sizeof(*i));    /* has wrong dev and inum */
        irelse(i);
        return 0;
    }
    return (i);
}

/*
 * Lock an inode
 */
ilock(ip)
    struct inode *ip;
{
    ip->count++;
    while (ip->flags & IBUSY) {
        ip->flags |= IWANT;
        sleep(ip, PRINOD);
    }
    ip->flags |= IBUSY;
    ip->count--;
}

/*
 * Read or write the inode.
 * Inode should be busy.
 */
iio(flag, ip)
    int flag;
    register struct inode *ip;
{
    register unsigned int inum, i;
    static struct mount *m;
    static struct buf *bp;
    static struct dsknod *dp;

    inum = ip->i_inum;
    i = inum + 31;
    if ((m = mlook(ip->i_dev)) == 0)
        panic("iio");           /* not mounted */
    if (inum == 0 || inum > m->isize << 4) {
        pr("Bad inumber");
        u.error = EIO;
        return 0;
    }
    if (m->ronly)
        ip->flags |= IRONLY;
    else
        ip->flags &= ~IRONLY;
    if ((bp = bread(i >> 4, ip->i_dev)) == 0)
        return 0;
    dp = bp->data + ((i & 15) << 5);
    if (flag == IREAD) {
        copy(dp, &ip->i_mode, 32);
        brelse(bp);
        x3to4(&ip->i_size0, &ip->i_size);
    } else {
        x4to3(&ip->i_size, &ip->i_size0);
        copy(&ip->i_mode, dp, 32);
        bdwrite(bp);
    }
    ip->flags &= ~IMOD;
    return 1;
}

/*
 * Release the inode and wakeup
 * anyone waiting for it.
 */
irelse(i)
    register struct inode *i;
{
    if (i == 0)
        return;
    if (i->flags & IWANT)
        wakeup(i);
    i->flags &= ~IBUSY & ~IWANT;
    i->time = itime();
}

/*
 * Maintain a reference "time" for inodes
 */
itime()
{
    static UINT count = 0;
    static struct inode *i;

    di();
    if (++count == 0)
        for (i = ilist; i < ilist + NINODE; i++)
            i->time = 0;
    ei();
    return (count);
}

/*
 * Flush the ilist. Used by umount to flush dev's inodes
 * and remove them from the cache.
 */
iflush(dev)
    int dev;
{
    register struct inode *ip;

    for (ip = ilist; ip < ilist + NINODE; ip++)
        if (dev == ip->i_dev) {
            ilock(ip);
            if (ip->flags & IMOD)
                iio(IWRITE, ip);
            irelse(ip);
            zero(ip, sizeof(*ip));
        }
}

/*
 * Sync the ilist. Called from sync().
 */
isync()
{
    register struct inode *ip;

    for (ip = ilist; ip < ilist + NINODE; ip++)
        if ((ip->flags & (IMOD | IBUSY)) == IMOD) {
            ilock(ip);
            iio(IWRITE, ip);
            irelse(ip);
        }
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
