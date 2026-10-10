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
extern int rtcinit();
extern int copyin();
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
    if ((m = tmount(dev, id, ronly, dname)) == 0)
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
 * Copy each mount entry's live counts into its superblock, so the
 * sync() that follows writes them out.  The allocators keep mlist's
 * bfree and ifree real (a static array, so it costs nothing to touch);
 * the superblock is their on-disk backing store, and is only updated
 * here and at mount.
 */
void
syncsuper()
{
    register struct mount *m;
    struct buf *sb;
    struct super *sp;

    for (m = mlist; m < mlist + NMOUNT; m++) {
        if (m->dev == 0)
            continue;
        sb = getsb(m->dev);
        sp = (struct super *)bhold(sb);
        sp->s_bfree = m->bfree;
        sp->s_ifree = m->ifree;
        brel();
        bdwrite(sb);
    }
}

/*
 * Make an entry in the mount table.
 */
tmount(dev, ip, ronly, dname)
    int dev, ronly;
    struct inode *ip;
    char *dname;
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
    /*
     * read(1, dev) directly rather than getsb(): a device that will not
     * mount is an error here, not a panic, so the superblock is still in
     * the buffer's own segment and needs the window.  getsb() attaches a
     * slot; if it already has one, this mapping is harmless - sb->data is
     * the slot either way.
     */
    sp = (struct super *)bhold(sb);
    sp->s_flock = ronly;        /* flock means read-only */

    /*
     * Record the mount point and mark the superblock dirty so the write
     * out carries it.  copyin() maps the caller's page into its own
     * window (MAP0[2]/[4]), not the buffer window the hold above owns,
     * and touches no header, so it is safe between bhold and brel.
     * 63 bytes and the terminator below cap the path at the field's 64.
     */
    copyin(dname, sp->s_mount, 63);
    sp->s_mount[63] = 0;
    sp->s_fmod = 1;

    if (dev == rootdev)         /* part of power-up */
        rtcinit(sp->s_time);
    m->isize = sp->s_isize;
    m->fsize = sp->s_fsize;
    m->dev = dev;
    m->inode = ip;
    m->ronly = ronly;
    m->bfree = sp->s_bfree;     /* the live counts start at the seeded ones */
    m->ifree = sp->s_ifree;
    brel();
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
    struct super *sp;
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
    sp = (struct super *)bhold(sb);
    sp->s_bfree = m->bfree;    /* persist the live counts */
    sp->s_ifree = m->ifree;
    brel();
    bdwrite(sb);
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
