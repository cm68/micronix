/*
 * create.c 
 */
#include <sys.h>
#include <inode.h>
#include <proc.h>

/*
 * Creat system call.
 */
creat(name, mode)
    char *name;
    int mode;
{
    fast struct inode *ip;
    fast int fd;

    /*
     * File does not exist. Mask the mode to
     * ordinary type. Note that iname sets
     * several globals for use by imkfile.
     */
    if ((ip = iname(name)) == NULL) {
        if ((ip = imkfile(mode & 07777)) == NULL)
            return ERROR;
    } else {
        /*
         * file exists
         */
        if ((ip->mode & ITYPE) == IDIR) {
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
    fast struct inode *ip;

    if (!super())
        return;
    if ((ip = iname(name)) != NULL) {
        u.error = EEXIST;
        irelse(ip);
        return;
    }
    if ((ip = imkfile(mode, dev)) == NULL)
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
    fast struct inode *ip;

    if (u.error != ENOENT)      /* from iname() */
        return NULL;
    u.error = 0;
    if (!access(u.iparent, IWRITE))
        return NULL;
    ilock(u.iparent);
    ip = ialloc(u.iparent->dev);
    irelse(u.iparent);
    if (ip == NULL)
        return NULL;
    ilink(ip->inum);
    ip->nlinks = 1;
    ip->mode |= mode;
    ip->uid = u.uid;
    ip->gid = u.gid;
    if (mode & IIO)
        ip->addr[0] = dev;
    return (ip);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
