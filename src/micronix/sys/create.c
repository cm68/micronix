/*
 * make a file
 *
 * sys/create.c 
 * Changed: <2021-12-24 05:54:15 curt>
 */
#include <types.h>
#include <sys/sys.h>
#include <sys/fs.h>
#include <sys/stat.h>
#include <sys/inode.h>
#include <sys/proc.h>
#include <errno.h>

/*
 * Creat system call.
 */
creat(name, mode)
    char *name;
    int mode;
{
    register struct inode *ip;
    register int fd;

    /*
     * File does not exist. Mask the mode to
     * ordinary type. Note that iname sets
     * several globals for use by imkfile.
     */
    if ((ip = iname(name)) == 0) {
        if ((ip = imkfile(mode & 07777)) == 0)
            return ERROR;
    } else {
        /*
         * file exists
         */
        if ((ip->i_mode & IFMT) == IFDIR) {
            u.error = EISDIR;
            irelse(ip);
            return ERROR;
        }
        if (!access(ip, IWRITE)) {
            irelse(ip);
            return ERROR;
        }
        itrunc(ip);             /* itrunc ignores special files */
    }
    fd = iopen(ip, IWRITE);
    irelse(ip);
    return (fd);
}

/*
 * Mknod system call
 */
mknod(name, mode, dev)
    char *name;
    int mode, dev;
{
    register struct inode *ip;

    if (!super())
        return;
    if ((ip = iname(name)) != 0) {
        u.error = EEXIST;
        irelse(ip);
        return;
    }
    if ((ip = imkfile(mode, dev)) == 0)
        return;
    irelse(ip);
}

/*
 * Make a new file on device u.iparent->dev and make an entry
 * in u.iparent. If it is an io file, set inode.addr[0] to dev.
 * A previous call to iname is required to set things up.
 */
struct inode *
imkfile(mode, dev)
    int mode, dev;
{
    register struct inode *ip;

    if (u.error != ENOENT)      /* from iname() */
        return 0;
    u.error = 0;
    if (!access(u.iparent, IWRITE))
        return 0;
    ilock(u.iparent);
    ip = ialloc(u.iparent->i_dev);
    irelse(u.iparent);
    if (ip == 0)
        return 0;
    ilink(ip->i_inum);
    ip->i_nlink = 1;
    ip->i_mode |= mode;
    ip->i_uid = u.u_uid;
    ip->i_gid = u.u_gid;
    if (mode & IIO)
        ip->i_addr[0] = dev;
    return (ip);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
