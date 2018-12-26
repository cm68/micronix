#include "sys.h"
#include "inode.h"
#include "mount.h"
#include "buf.h"
#include "proc.h"

       /*
	* Return a pointer to a locked inode.
	* Errors: bad inumber, ilist is full.
	*/
	struct inode *
iget(num, dev)
	int  num, dev;
	{
	fast struct inode *i, *f;

    loop:
	f = NULL;
	for (i = ilist; i < ilist + NINODE; i++)
		{
		if (i->inum == num && i->dev == dev)
			{
			ilock(i);
			if (i->mount)
				{
				dev = i->mount->dev;
				num = 1; /* root inumber */
				irelse(i);
				goto loop;
				}
			return (i);
			}
		if (i->count > 0 || i->flags & IBUSY)
			continue;
		if (f == NULL || f->time > i->time)
			f = i;
		}
	if ((i = f) == NULL)
		{
		u.error = ENFILE;
		return NULL;
		}
	ilock(i);
	if ((i->flags & IMOD) && !iio(IWRITE, i))
		{
		irelse(i);
		return NULL;
		}
	i->inum = num; /* mark it for other searchers */
	i->dev = dev;
	i->count = i->mount = 0;
	if (!iio(IREAD, i))
		{
		zero(i, sizeof(*i)); /* has wrong dev and inum */
		irelse(i);
		return NULL;
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
	while (ip->flags & IBUSY)
		{
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
	int  flag;
	fast struct inode *ip;
	{
	fast unsigned int inum, i;
	static struct mount *m;
	static struct buf *bp;
	static struct dsknod *dp;

	inum = ip->inum;
	i = inum + 31;
	if ((m = mlook(ip->dev)) == NULL)
		panic("iio"); /* not mounted */
	if (inum == 0 || inum > m->isize << 4)
		{
		pr("Bad inumber");
		u.error = ESYS;
		return NO;
		}
	if (m->ronly)
		ip->flags |= IRONLY;
	else
		ip->flags &= ~IRONLY;
	if ((bp = bread(i >> 4, ip->dev)) == NULL)
		return NO;
	dp = bp->data + ((i & 15) << 5);
	if (flag == IREAD)
		{
		copy(dp, &ip->mode, 32);
		brelse(bp);
		x3to4(&ip->size0, &ip->size);
		}
	else
		{
		x4to3(&ip->size, &ip->size0);
		copy(&ip->mode, dp, 32);
		bdwrite(bp);
		}
	ip->flags &= ~IMOD;
	return YES;
	}

       /*
	* Release the inode and wakeup
	* anyone waiting for it.
	*/
irelse(i)
	fast struct inode *i;
	{
	if (i == NULL)
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
	fast struct inode *ip;

	for (ip = ilist; ip < ilist + NINODE; ip++)
		if (dev == ip->dev)
			{
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
	fast struct inode *ip;

	for (ip = ilist; ip < ilist + NINODE; ip++)
		if ((ip->flags & (IMOD | IBUSY)) == IMOD)
			{
			ilock(ip);
			iio(IWRITE, ip);
			irelse(ip);
			}
	}
