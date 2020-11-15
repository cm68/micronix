/*
 * sys2.c 
 */
#include <sys/sys.h>
#include <sys/inode.h>
#include <sys/proc.h>
#include <sys/file.h>

extern long seconds;

/*
 * Stat system call
 */
stat(name, buf)
    char *name, *buf;
{
    fast struct inode *ip;

    if ((ip = iname(name)) != NULL)
        istat(ip, buf);
}

/*
 * Fstat system call
 */
fstat(fd, buf)
    int fd;
    char *buf;
{
    fast struct file *fp;

    if ((fp = ofile(fd)) == NULL)
        return;
    ilock(fp->inode);
    istat(fp->inode, buf);
}

/*
 * Common code for stat and fstat
 */
istat(ip, buf)
    struct inode *ip;
    int *buf;
{
    if (!valid(buf, 36))
        return;
    x4to3(&ip->size, &ip->size0);       /* update 3-byte size */
    copyout(ip, buf, 36);
    irelse(ip);
}

/*
 * Getpid system call
 */
getpid()
{
    return (procid(u.p));
}

/*
 * Stime system call
 */
stime(high, low)
    unsigned high, low;
{
    if (!super())
        return;
    di();
    seconds = ((long) high << 16) | low;
    ei();
}

/*
 * Seek system call
 * Seek(character-special, 0, 1 or 3) fails.
 */
seek(fd, disp, from)
    int fd;
    fast int disp;
    fast unsigned from;
{
    static struct file *fp;
    static long offset;

    if ((fp = ofile(fd)) == NULL)
        return;
    if (fp->mode & PIPE) {
        u.error = ESPIPE;
        return;
    }
    from %= 6;
    if (from == 0 || from == 3)
        offset = (unsigned) disp;
    else
        offset = disp;
    if (from >= 3)
        offset <<= 9;
    switch (from) {
    case 0:
    case 3:
        break;
    case 1:
    case 4:
        offset += fp->rwptr;
        break;
    case 2:
    case 5:
        offset += fp->inode->size;
        break;
    }
    if (offset < 0)
        u.error = EINVAL;
    else
        fp->rwptr = offset;
}

/*
 * Sleep system call (not internal sleep)
 */
snooze(secs)
    fast unsigned int secs;
{
    long waketime;
    extern int revel;

    di();
    waketime = seconds + secs;
    while (waketime > seconds) {
        secs = waketime - seconds;

        if (revel <= 0 || secs < revel) {
            revel = secs;
        }

        sleep(&revel, PRIUSER);
        di();
    }
    ei();
}

/*
 * Alarm system call.
 * Signal is sent in clock() (time.c).
 */

alarm(secs)
    UINT secs;
{
    static UINT old;

    old = u.p->alarm;
    u.p->alarm = secs;

    return old;
}

/*
 * Pause system call
 */
pause()
{
    sleep(&pause, PRIUSER);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
