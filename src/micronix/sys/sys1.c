/*
 * sys1.c 
 */
#include <types.h>
#include <sys/sys.h>
#include <sys/inode.h>
#include <sys/file.h>
#include <sys/proc.h>

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
    fast struct inode *i;

    if ((i = iname(name)) == NULL)
        return;
    if ((i->mode & ITYPE) != IDIR) {
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
    fast struct inode *i;

#define MBITS	07777           /* user changable mode bits */

    if ((i = iname(name)) == NULL)
        return;
    if (!ronly(i) && (u.euid == i->uid || super())) {
        i->mode &= ~MBITS;
        i->mode |= (mode & MBITS);
        i->flags |= IMOD;
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
    fast struct inode *ip;

    if (!super())
        return;
    if ((ip = iname(name)) == NULL)
        return;
    if (!ronly(ip)) {
        ip->uid = owner;
        ip->gid = owner >> 8;
        ip->flags |= IMOD;
    }
    irelse(ip);
}

/*
 * Dup system call
 */
dup(fd)
    int fd;
{
    fast struct file *fp;

    if ((fp = ofile(fd)) == NULL)
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
    return ((u.gid << 8) | u.uid);
}

/*
 * Setuid system call
 */
setuid(id)
    unsigned int id;
{
    fast char uid, gid;

    uid = (char) id;
    gid = id >> 8;
    if ((uid == u.uid && gid == u.gid) || super()) {
        u.p->uid = uid;
        u.uid = uid;
        u.gid = gid;
        u.euid = uid;
        u.egid = gid;
    }
}

/*
 * Nice system call
 */
nice(n)
    int n;
{
    fast char nice;

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
