#include "sys.h"
#include "inode.h"
#include "mount.h"
#include "proc.h"

#define SUPERID 0	/* Id of superuser */

#ifndef KNR
int ronly(struct inode *ip);
#endif

/*
* Test whether the current user has access permission of type
* IREAD, IWRITE, and/or IEXEC to inode ip.
*/
#ifdef KNR
int
access(i, per)
	struct inode *i;
	int per;
#else
access(struct inode *i, int per)
#endif
{
	static struct inode *ip;
	static int perm;
	static int mode;

	ip = i;
	perm = per;
	mode = ip->mode;
	/*
	 * Don't allow anyone to write
	 * on devices mounted read-only
	 * or locked for one-writer-only.
	 */
	if (perm & IWRITE) {
		if (!(mode & IIO) && ronly(ip))
			return NO;
		if (ip->flags & IWRLOCK) {
			u.error = EBUSY;
			return NO;
		}
	}
	/*
	 * Allow a super user any access except EXEC, when at
	 * least one of the exec bits must be on.
	 */
	if (u.euid == SUPERID) {
		if (!(perm & IEXEC))
			return YES;
		if (mode & (IEXEC | IEXEC << 3 | IEXEC << 6))
			return YES;
		goto bad;
	}
	/*
	 * Check the anyone permissions
	 */
	if ((perm & mode) == perm)
		return YES;
	/*
	 * Check the group permissions
	 */
	perm <<= 3;
	if (u.egid == ip->gid && (perm & mode) == perm)
		return YES;
	/*
	 * Check the owner permissions
	 */
	perm <<= 3;
	if (u.euid == ip->uid && (perm & mode) == perm)
		return YES;
   bad:
	u.error = EACCES;
	return NO;
}

/*
* Test whether or not the current user is the super-user.
* Set u.error if not.
*/
int
super()
{
	if (u.euid == SUPERID)
		return YES;
	u.error = EPERM;
	return NO;
}

/*
* Access system call
*/
#ifdef KNR
permission(name, mode)
	char *name;
	int mode;
#else
int
permission(char *name, int mode)
#endif
{
	static struct inode *ip;
	static int euid, egid;

	if ((ip = iname(name)) == NULL)
		return;
	euid = u.euid;
	egid = u.egid;
	u.euid = u.uid;
	u.egid = u.gid;
	access(ip, mode);
	u.euid = euid;
	u.egid = egid;
	irelse(ip);
}

/*
 * Check whether an inode comes from
 * a read-only file system.
 * Called above, and from chmod and chown in sys1.c.
 */
#ifdef KNR
ronly(ip)
	struct inode *ip;
#else
int
ronly(struct inode *ip)
#endif
{
	if (ip->flags & IRONLY) {
		u.error = EROFS;
		return YES;
	}
	return NO;
}
