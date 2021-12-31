/*
 * check for permissions on file operation
 *
 * sys/access.c
 * Changed: <2021-12-23 18:13:41 curt>
 */
#include <types.h>
#include <sys/sys.h>
#include <sys/fs.h>
#include <sys/stat.h>
#include <sys/inode.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <errno.h>

#define SUPERID 0               /* Id of superuser */

/*
 * Test whether the current user has access permission of type
 * IREAD, IWRITE, and/or IEXEC to inode ip.
 */

int
access(i, per)
    struct inode *i;
    int per;
{
    static struct inode *ip;
    static int perm;
    static int mode;

    ip = i;
    perm = per;
    mode = ip->i_mode;
    /*
     * Don't allow anyone to write
     * on devices mounted read-only
     * or locked for one-writer-only.
     */
    if (perm & IWRITE) {
        if (!(mode & IIO) && ronly(ip))
            return 0;
        if (ip->flags & IWRLOCK) {
            u.error = EBUSY;
            return 0;
        }
    }
    /*
     * Allow a super user any access except EXEC, when at
     * least one of the exec bits must be on.
     */
    if (u.u_euid == SUPERID) {
        if (!(perm & IEXEC))
            return 1;
        if (mode & (IEXEC | IEXEC << 3 | IEXEC << 6))
            return 1;
        goto bad;
    }
    /*
     * Check the anyone permissions
     */
    if ((perm & mode) == perm)
        return 1;
    /*
     * Check the group permissions
     */
    perm <<= 3;
    if (u.u_egid == ip->i_gid && (perm & mode) == perm)
        return 1;
    /*
     * Check the owner permissions
     */
    perm <<= 3;
    if (u.u_euid == ip->i_uid && (perm & mode) == perm)
        return 1;
  bad:
    u.error = EACCES;
    return 0;
}

/*
 * Test whether or not the current user is the super-user.
 * Set u.error if not.
 */
int
super()
{
    if (u.u_euid == SUPERID)
        return 1;
    u.error = EPERM;
    return 0;
}

/*
 * Access system call
 */

permission(name, mode)
    char *name;
    int mode;
{
    static struct inode *ip;
    static int euid, egid;

    if ((ip = iname(name)) == 0)
        return;
    euid = u.u_euid;
    egid = u.u_egid;
    u.u_euid = u.u_uid;
    u.u_egid = u.u_gid;
    access(ip, mode);
    u.u_euid = euid;
    u.u_egid = egid;
    irelse(ip);
}

/*
 * Check whether an inode comes from
 * a read-only file system.
 * Called above, and from chmod and chown in sys1.c.
 */

ronly(ip)
    struct inode *ip;
{
    if (ip->flags & IRONLY) {
        u.error = EROFS;
        return 1;
    }
    return 0;
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
