#include "sys.h"
#include "inode.h"
#include "proc.h"

       /*
	* Link system call
	*/
link(old, new)
	char *old, *new;
	{
	fast struct inode *ip;
	fast int inum, dev;

	if ((ip = iname(old)) == NULL)
		return;
	irelse(ip);	/* avoid deadlock */
	if ((ip->mode & ITYPE) == IDIR && !super())
		return;
	if (ip->nlinks == 127)
		{
		u.error = EMLINK;
		return;
		}
	inum = ip->inum;
	dev = ip->dev;
	if ((ip = iname(new)) != NULL)
		{
		irelse(ip);
		u.error = EEXIST;
		return;
		}
	if (u.error != ENOENT)
		return;
	if (u.iparent->dev != dev)
		{
		u.error = EXDEV;
		return;
		}
	u.error = 0;
	if (!access(u.iparent, IWRITE))
		return;
	if ((ip = iget(inum, dev)) == NULL)
		return;
	ip->nlinks++;
	ip->flags |= IMOD;
	irelse(ip);
	ilink(inum);
	}

       /*
	* Write a link to inum in directory u.iparent
	* using the name in u.dir.name.
	* U->offset has already been set by iname.
	* Called from link above and from imkfile (create.c).
	*/
ilink(inum)
	int inum;
	{
	static char *p;

	for (p = u.dir.name; p < &u.dir.name[14]; p++)
		if (*p == 0)
			{
			zero(p, &u.dir.name[14] - p);
			break;
			}
	u.dir.inum = inum;
	u.segflg = KSEG;
	ilock(u.iparent);
	nwrite(u.iparent, &u.dir, 16);
	irelse(u.iparent);
	}

       /*
	* Unlink system call.
	*/
unlink(name)
	char *name;
	{
	fast struct inode *ip, *parent;

	if ((ip = iname(name)) == NULL)
		return;
	irelse(ip);	/* no process switching until done */
	parent = u.iparent;
	if (parent == NULL)	    /* root or current dir */
		{
		u.error = ENOENT;
		return;
		}
	if ((ip->mode & ITYPE) == IDIR && !super())
		return;
	if (!access(parent, IWRITE))
		return;
	if (ip->nlinks > 0)
		{
		ip->nlinks--;
		ip->flags |= IMOD;
		}
	parent->count++;	/* protect parent inode across idec */
	idec(ip);
	parent->count--;
	ilink(0);
	}
