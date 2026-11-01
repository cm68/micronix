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
extern unsigned char ronly();
extern unsigned char super();
extern void idec();
extern void irelse();
extern void isync();
extern void bsync();
extern void pr();
extern unsigned char valid();
extern unsigned char access();

extern char oalloc();       /* open.c: a descriptor or ERROR */
extern void close(UINT8);    /* open.c */

/*
 * Break system call
 */
void
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
void
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
void
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
void
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
void
dup()
{
    register struct file *fp;
    UINT8 fd;

    fd = u.hl;
    if ((fp = ofile(fd)) == 0)
        return;
    /*
     * oalloc() hands back a byte, and ERROR is -1 in that byte.  As
     * an unsigned char fd promotes to 0x00ff, which is not the
     * 0xffff an int -1 promotes to, so the comparison has to stay in
     * the byte too - the cast is what keeps the compiler from
     * widening both sides and testing the wrong answer.
     */
    if ((fd = oalloc()) == (UINT8) ERROR)
        return;
    u.olist[fd] = fp;
    fp->count++;
    u.hl = fd;                  /* the new descriptor is the answer */
}

/*
 * Dup2 system call.  fd2 becomes another name for the file fd names,
 * closing whatever fd2 was.  fd arrives in u.hl and fd2 inline, the
 * way every descriptor-taking call passes its first; fd2 goes back
 * the same way.  dup2(fd, fd) is a successful no-op, and an invalid
 * fd is refused before fd2 is touched.
 *
 * fd2 is a word and not the UINT8 the descriptor is: docall() reaches
 * every syscall through the table's untyped int (*)(), with the argument
 * words in arg[].  A byte first parameter makes the compiler pass it in
 * A (the .abi marker's SF_ARG8) and nothing on that path has put it
 * there - fd2 arrived in a register's worth of whatever was last in A,
 * so every dup2 failed and a login shell's Ldup fell back to stdin.
 * The descriptor is a byte from the comparison and the olist index down;
 * the call boundary is words.
 */
void
dup2(fd2)
    UINT fd2;
{
    register struct file *fp;
    UINT8 fd;

    fd = u.hl;
    if ((fp = ofile(fd)) == 0)
        return;
    if (fd2 >= NOPEN) {
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
void
sync()
{
    isync();
    syncsuper();
    bsync();
}

/*
 * Reboot system call
 */
void
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
void
getuid()
{
    u.hl = (u.u_gid << 8) | u.u_uid;
}

/*
 * Setuid system call
 */
void
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
void
nice()
{
    register char nice;

    nice = u.hl;
    if (nice < 0 && !super())
        return;
    u.p->nice = 128 - nice;
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
