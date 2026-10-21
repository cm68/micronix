/*
 * random system calls
 *
 * sys/sys1.c 
 * Changed: <>
 */
#include <types.h>
#include <sys/sys.h>
#include <sys/fs.h>
#include <sys/stat.h>
#include <sys/inode.h>
#include <sys/file.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/reboot.h>
#include <errno.h>

/*
 * Break system call
 */
brake(addr)
    int addr;
{
    UINT old;

    if (addr > u.sp - 12 && u.brake < u.sp) {
        u.error = ENOMEM;
        return;
    }
    old = u.brake;
    u.brake = addr;
    valid(old, addr - old);         /* nail down the pages the break just claimed */
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
 * Dup system call.  The descriptor arrives in u.hl, and the new one
 * goes back the same way.
 */
dup()
{
    register struct file *fp;
    int fd;

    fd = u.hl;
    if ((fp = ofile(fd)) == 0)
        return;
    if ((fd = oalloc()) == ERROR)
        return;
    u.olist[fd] = fp;
    fp->count++;
    u.hl = fd;
}

/*
 * Dup2 system call.  fd2 becomes another name for the file fd names,
 * closing whatever fd2 was.  fd arrives in u.hl and fd2 inline, the
 * way every descriptor-taking call passes its first; fd2 goes back
 * the same way.  dup2(fd, fd) is a successful no-op, and an invalid
 * fd is refused before fd2 is touched.
 */
dup2(fd2)
    int fd2;
{
    register struct file *fp;
    int fd;

    fd = u.hl;
    if ((fp = ofile(fd)) == 0)
        return;
    if (fd2 < 0 || fd2 >= NOPEN) {
        u.error = EBADF;
        return;
    }
    if (fd != fd2) {
        if (u.olist[fd2])
            close(fd2);
        u.olist[fd2] = fp;
        fp->count++;
    }
    u.hl = fd2;
}

/*
 * Sync system call
 */
sync()
{
    isync();
    syncsuper();
    bsync();
}

/*
 * Reboot system call
 */
reboot(howto)
    int howto;
{
    register int n;

    if (!super())
        return;
    if ((howto & RB_HALT) == 0)
        pr("reboot: autoboot not implemented, halting\n");

    /*
     * Flush the inodes and write the dirty buffers out, reporting how many
     * remain, until the cache is clean - then halt.  isync() runs each
     * pass, so inode changes are caught too.  Processes are left alone:
     * the graceful path (init's shutdown) has already stopped them, and a
     * forced halt wants to die now, not to wait for them.
     */
    for (;;) {
        isync();
        n = ndirty();
        pr("halt: %d dirty buffers\n", n);
        if (n == 0)
            break;
        bdrain();
    }
    for (;;)
        hlt();
}

/*
 * Getuid system call
 */
getuid()
{
    u.hl = (u.u_gid << 8) | u.u_uid;
}

/*
 * Setuid system call
 */
setuid()
{
    register char uid, gid;
    unsigned int id;

    id = u.hl;
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
nice()
{
    register char nice;

    nice = u.hl;
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
ssw()
{
    switches = u.hl;
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
