#include "sys.h"
#include "inode.h"
#include "mount.h"
#include "proc.h"
#include "buf.h"
#include "sup.h"

extern int rootdev;
extern long seconds;

       /*
	* Mount system call
	*/
mount(ioname, dname, ronly)
	char *ioname, *dname;
	int ronly;
	{
	fast struct inode *io, *id;
	struct mount *m;
	int dev, idev, inum;

	if (!super())
		return;
	if ((io = iname(ioname)) == NULL)
		return;
	irelse(io);	/* prevent deadlock with iname below */
	if ((io->mode & ITYPE) != IBIO)
		{
		u.error = ENOTBLK;
		return;
		}
	dev = io->addr[0];
	idev = io->dev;		/* save for iget below */
	inum = io->inum;	/* ditto */
	if ((id = iname(dname)) == NULL)
		return;
	if ((id->mode & ITYPE) != IDIR)
		{
		u.error = ENOTDIR;
		goto done;
		}
	if (mlook(dev) || id->mount)
		{
		u.error = EBUSY;
		goto done;
		}
	bopen(dev, !ronly);
	if (u.error)
		goto done;
	if ((m = tmount(dev, id, ronly)) == 0)
		goto done;
	id->mount = m;
	id->count++;
	if ((io = iget(inum, idev)) != NULL)
		{
		io->count++;
		irelse(io);
		}
   done:
	irelse(id);
	}

       /*
	* Search the mount table for device dev.
	* Return a pointer to the mount structure, or NULL.
	*/
	struct mount *
mlook(dev)
	int dev;
	{
	fast struct mount *m;

	for (m = mlist; m < mlist + NMOUNT; m++)
		if (m->dev == dev)
			return (m);
	return NULL;
	}

       /*
	* Make an entry in the mount table.
	*/
tmount(dev, ip, ronly)
	int dev, ronly;
	struct inode *ip;
	{
	fast struct mount *m;
	fast struct buf *sb;
	fast struct sup *sp;

	if ((m = mlook(0)) == NULL)
		{
		u.error = EBUSY;
		return NO;
		}
	sb = bread(1, dev);
	if (u.error)
		{
		brelse(sb);
		return NO;
		}
	sb->flags |= BLOCK;
	sp = sb->data;
	sp->flock = ronly;	/* flock means read-only */
	if (dev == rootdev)	/* part of power-up */
		{
		di();
		seconds = sp->time;
		ei();
		}
	m->isize = sp->isize;
	m->fsize = sp->fsize;
	m->dev = dev;
	m->inode = ip;
	m->ronly = ronly;
	brelse(sb);
	return m;
	}

       /*
	* Unmount a file system from its inode
	*/
umount(ioname)
	char * ioname;
	{
	fast struct inode *io, *ip;
	struct mount *m;
	struct buf *sb;
	int dev;

	if (!super())
		return;
	if ((io = iname(ioname)) == NULL)
		return;
	if (io->mode & ITYPE != IBIO)
		{
		u.error = ENOTBLK;
		goto bad;
		}
	dev = io->addr[0];
	if ((m = mlook(dev)) == NULL || dev == rootdev)
		{
		u.error = EINVAL;
		goto bad;
		}
	isync();
	for (ip = ilist; ip < ilist + NINODE; ip++)
		if (ip->dev == dev)
			if (ip->flags & (IBUSY | IMOD) || ip->count > 0)
				{
				u.error = EBUSY;
				goto bad;
				}
	iflush(dev);
	sb = getsb(dev);
	sb->flags &= ~BLOCK;
	brelse(sb);
	irelse(io);		/* avoid deadlock */
	iclose(io, !m->ronly);
	ip = m->inode;
	ip->mount = 0;
	ip->count--;
	idec(ip);
	zero(m, sizeof(*m));
	return;
    bad:
	irelse(io);
	}
