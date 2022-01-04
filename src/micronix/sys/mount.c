/*
 * manage the mount table
 * sys/mount.c 
 * Changed: <>
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

extern int rootdev;
extern long seconds;
extern struct inode ilist[];

/*
 * Mount system call
 */
mount(ioname, dname, ronly)
    char *ioname, *dname;
    int ronly;
{
    register struct inode *io, *id;
    struct mount *m;
    int dev, idev, inum;

    if (!super())
        return;
    if ((io = iname(ioname)) == 0)
        return;
    irelse(io);                 /* prevent deadlock with iname below */
    if ((io->i_mode & IFMT) != IFBLK) {
        u.error = ENOTBLK;
        return;
    }
    dev = io->i_addr[0];
    idev = io->i_dev;             /* save for iget below */
    inum = io->i_inum;            /* ditto */
    if ((id = iname(dname)) == 0)
        return;
    if ((id->i_mode & IFMT) != IFDIR) {
        u.error = ENOTDIR;
        goto done;
    }
    if (mlook(dev) || id->mount) {
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
    if ((io = iget(inum, idev)) != 0) {
        io->count++;
        irelse(io);
    }
  done:
    irelse(id);
}

/*
 * Search the mount table for device dev.
 * Return a pointer to the mount structure, or 0.
 */
struct mount *
mlook(dev)
    int dev;
{
    register struct mount *m;

    for (m = mlist; m < mlist + NMOUNT; m++)
        if (m->dev == dev)
            return (m);
    return 0;
}

/*
 * Make an entry in the mount table.
 */
tmount(dev, ip, ronly)
    int dev, ronly;
    struct inode *ip;
{
    register struct mount *m;
    register struct buf *sb;
    register struct super *sp;

    if ((m = mlook(0)) == 0) {
        u.error = EBUSY;
        return 0;
    }
    sb = bread(1, dev);
    if (u.error) {
        brelse(sb);
        return 0;
    }
    sb->flags |= BLOCK;
    sp = sb->data;
    sp->s_flock = ronly;        /* flock means read-only */
    if (dev == rootdev) {       /* part of power-up */
        di();
        seconds = sp->s_time;
        ei();
    }
    m->isize = sp->s_isize;
    m->fsize = sp->s_fsize;
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
    char *ioname;
{
    register struct inode *io, *ip;
    struct mount *m;
    struct buf *sb;
    int dev;

    if (!super())
        return;
    if ((io = iname(ioname)) == 0)
        return;
    if (io->i_mode & IFMT != IFBLK) {
        u.error = ENOTBLK;
        goto bad;
    }
    dev = io->i_addr[0];
    if ((m = mlook(dev)) == 0 || dev == rootdev) {
        u.error = EINVAL;
        goto bad;
    }
    isync();
    for (ip = ilist; ip < ilist + NINODE; ip++)
        if (ip->i_dev == dev)
            if (ip->flags & (IBUSY | IMOD) || ip->count > 0) {
                u.error = EBUSY;
                goto bad;
            }
    iflush(dev);
    sb = getsb(dev);
    sb->flags &= ~BLOCK;
    brelse(sb);
    irelse(io);                 /* avoid deadlock */
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

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
