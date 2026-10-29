/*
 * character i/o top levels
 *
 * sys/cio.c 
 * Changed: <2021-12-24 05:52:38 curt>
 */
#include <types.h>
#include <sys/sys.h>
#include <sys/con.h>
#include <sys/file.h>
#include <sys/fs.h>
#include <sys/stat.h>
#include <sys/sgtty.h>
#include <sys/inode.h>
#include <sys/proc.h>
#include <errno.h>

extern struct file *falloc();   /* open.c */

/*
 * Access ciosw to open a character device
 */
copen(dev, mode)
    int dev;
    UINT8 mode;
{
    (*ciosw[cmajor(dev)].open) (dev, mode);
}

/*
 * Access ciosw to close a character device
 */
cclose(dev, mode)
    int dev;
    UINT8 mode;
{
    (*ciosw[cmajor(dev)].close) (dev, mode);
}

/*
 * Access ciosw to read a character device
 */
cread(dev)
    int dev;
{
    (*ciosw[cmajor(dev)].read) (dev);
}

/*
 * Access ciosw to write a character device
 */
cwrite(dev)
    int dev;
{
    (*ciosw[cmajor(dev)].write) (dev);
}

/*
 * The descriptor keeps its number and becomes the device the driver
 * named: this is what ACCEPT does, giving the child the connection while
 * the parent's descriptor is still the listener.
 */
static
cioswap(fd, fp, dev)
    int fd, dev;
    register struct file *fp;
{
    register struct file *nf;

    if ((nf = falloc()) == 0) {
        u.error = ENFILE;
        return (-1);
    }
    nf->inode = fp->inode;
    nf->count = 1;
    nf->mode = fp->mode;
    nf->f_dev = dev;
    u.olist[fd] = nf;
    if (--fp->count <= 0)
        cclose(fp->f_dev, fp->mode & IWRITE);
    return (0);
}

/*
 * One request to a character device.  A terminal's status is just two
 * of them - TIOCGETA and TIOCSETA - so stty(3) and gtty(3) are library
 * wrappers over ioctl(2), not system calls of their own.
 *
 * A driver that changes u.fdev is handing the descriptor a different
 * device - a clone's accept - and cioswap()s this process's file to it.
 */
cioctl(fd, cmd, arg)
    int fd, cmd;
    char *arg;
{
    struct file *fp;
    struct inode *ip;
    int dev;

    if ((fp = ofile(fd)) == 0)
        return (-1);            /* ofile set u.error */
    ip = fp->inode;
    if ((ip->i_mode & IFMT) != IFCHR) {
        u.error = ENOTTY;
        return (-1);
    }
    dev = fp->f_dev;
    u.fdev = dev;
    if (cmd == TIOCGETA || cmd == TIOCSETA) {
        if (!valid(arg, 6))     /* malloc.c */
            return (-1);
        u.base = arg;
        u.segflg = USEG;
    }
    (*ciosw[cmajor(dev)].ioctl) (dev, cmd, arg);
    if (u.error)
        return (-1);
    if (u.fdev != dev)
        return (cioswap(fd, fp, u.fdev));
    return (0);
}

/*
 * The old system calls, kept for binaries built before the library
 * moved them onto ioctl.  They are the same two requests with the
 * system call's register and carry convention; new programs use the
 * library wrappers (lib/libc/gtty.c, stty.c).
 */
gtty(addr)
    char *addr;
{
    if (u.error) {
        u.af |= ERRBIT;
        u.hl = u.error;
    } else
        u.af &= ~ERRBIT;
}

stty(addr)
    char *addr;
{
    if (u.error) {
        u.af |= ERRBIT;
        u.hl = u.error;
    } else
        u.af &= ~ERRBIT;
}

/*
 * Extract the major device number
 */
cmajor(dev)
    unsigned dev;
{
    if ((dev >>= 8) >= ncdev)
        return (0);             /* nodev sets u.error = ENXIO */
    return (dev);
}

/*
 * Do-nothing entry for biosw or ciosw
 */
nulldev()
{
}

/*
 * Error entry for biosw or ciosw
 */
nodev()
{
    u.error = ENXIO;
}

/*
 * An ioctl the device does not have.
 */
noioctl()
{
    u.error = ENOTTY;
}

/*
 * Null write. Pretend to write all characters.
 */
nullwrite()
{
    u.count = 0;
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
