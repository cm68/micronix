/*
 * sys1.c 
 */
#include <types.h>
#include <sys/sys.h>
#include <sys/fs.h>
#include <sys/stat.h>
#include <sys/inode.h>
#include <sys/file.h>
#include <sys/proc.h>
#include <errno.h>

/*
 * Break system call
 */
brake(addr)
    int addr;
{
    if (addr > u.sp - 12 && u.brake < u.sp) {
        u.error = ENOMEM;
        return;
    }
    u.brake = addr;
}

/*
 * Chdir system call
 */
chdir(name)
    char *name;
{
    register struct inode *i;

    if ((i = iname(name)) == 0)
        return;
    if ((i->i_mode & IFMT) != IFDIR) {
        u.error = ENOTDIR;
        goto out;
    }
    if (!access(i, IEXEC))
        goto out;
    u.cdir->count--;
    idec(u.cdir);
    u.cdir = i;
    i->count++;
  out:
    irelse(i);
}

/*
 * Chmod system call
 */
chmod(name, mode)
    char *name;
    int mode;
{
    register struct inode *i;

#define MBITS	07777           /* user changable mode bits */

    if ((i = iname(name)) == 0)
        return;
    if (!ronly(i) && (u.u_euid == i->i_uid || super())) {
        i->i_mode &= ~MBITS;
        i->i_mode |= (mode & MBITS);
        i->i_flags |= IMOD;
    }
    irelse(i);
}

/*
 * Chown system call
 */
chown(name, owner)
    char *name;
    UINT owner;
{
    register struct inode *ip;

    if (!super())
        return;
    if ((ip = iname(name)) == 0)
        return;
    if (!ronly(ip)) {
        ip->i_uid = owner;
        ip->i_gid = owner >> 8;
        ip->i_flags |= IMOD;
    }
    irelse(ip);
}

/*
 * Dup system call
 */
dup(fd)
    int fd;
{
    register struct file *fp;

    if ((fp = ofile(fd)) == 0)
        return;
    if ((fd = oalloc()) == ERROR)
        return;
    u.olist[fd] = fp;
    fp->count++;
    return (fd);
}

/*
 * Sync system call
 */
sync()
{
    isync();
    bsync();
}

/*
 * Getuid system call
 */
getuid()
{
    return ((u.u_gid << 8) | u.u_uid);
}

/*
 * Setuid system call
 */
setuid(id)
    unsigned int id;
{
    register char uid, gid;

    uid = (char) id;
    gid = id >> 8;
    if ((uid == u.u_uid && gid == u.u_gid) || super()) {
        u.p->uid = uid;
        u.u_uid = uid;
        u.u_gid = gid;
        u.u_euid = uid;
        u.u_egid = gid;
    }
}

/*
 * Nice system call
 */
nice(n)
    int n;
{
    register char nice;

    nice = n;
    if (nice < 0 && !super())
        return;
    u.p->nice = 128 - nice;
}

/*
 * Make up for the Decision's lack of console switches
 */
unsigned switches = 0;

/*
 * Set the switches
 */
ssw(arg)
    int arg;
{
    switches = arg;
}

/*
 * Read the switches
 */
csw()
{
    return (switches);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
