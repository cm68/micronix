/*
 * manage directory entries
 *
 * sys/link.c 
 * Changed: <2021-12-24 06:08:49 curt>
 */
 
#include <types.h>
#include <sys/sys.h>
#include <sys/fs.h>
#include <sys/stat.h>
#include <sys/inode.h>
#include <sys/proc.h>
#include <errno.h>

/*
 * Link system call
 */
link(old, new)
    char *old, *new;
{
    register struct inode *ip;
    register int inum, dev;

    if ((ip = iname(old)) == 0)
        return;
    irelse(ip);                 /* avoid deadlock */
    if ((ip->i_mode & IFMT) == IFDIR && !super())
        return;
    if (ip->i_nlink == 127) {
        u.error = EMLINK;
        return;
    }
    inum = ip->i_inum;
    dev = ip->i_dev;
    if ((ip = iname(new)) != 0) {
        irelse(ip);
        u.error = EEXIST;
        return;
    }
    if (u.error != ENOENT)
        return;
    if (u.iparent->i_dev != dev) {
        u.error = EXDEV;
        return;
    }
    u.error = 0;
    if (!access(u.iparent, IWRITE))
        return;
    if ((ip = iget(inum, dev)) == 0)
        return;
    ip->i_nlink++;
    ip->i_flags |= IMOD;
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
        if (*p == 0) {
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
    register struct inode *ip, *parent;

    if ((ip = iname(name)) == 0)
        return;
    irelse(ip);                 /* no process switching until done */
    parent = u.iparent;
    if (parent == 0) {       /* root or current dir */
        u.error = ENOENT;
        return;
    }
    if ((ip->i_mode & IFMT) == IFDIR && !super())
        return;
    if (!access(parent, IWRITE))
        return;
    if (ip->i_nlink > 0) {
        ip->i_nlink--;
        ip->i_flags |= IMOD;
    }
    parent->count++;            /* protect parent inode across idec */
    idec(ip);
    parent->count--;
    ilink(0);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
