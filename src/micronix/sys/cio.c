/*
 * cio.c 
 */
#include <types.h>
#include <sys/sys.h>
#include <sys/con.h>
#include <sys/file.h>
#include <sys/inode.h>
#include <sys/proc.h>

/*
 * Access ciosw to open a character device
 */
copen(dev, mode)
    int dev, mode;
{
    (*ciosw[cmajor(dev)].open) (dev, mode);
}

/*
 * Access ciosw to close a character device
 */
cclose(dev, mode)
    int dev, mode;
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
 * Access ciosw to sgtty a character device
 */
cmode(fd, addr, flag)
    int fd, flag;
    char *addr;
{
    static struct file *fp;
    static struct inode *ip;
    static int dev;

    if ((fp = ofile(fd)) == NULL)
        return;
    ip = fp->inode;
    if ((ip->mode & ITYPE) != ICIO) {
        u.error = ENOTTY;
        return;
    }
    if (!valid(addr, 6))        /* malloc.c */
        return;
    u.base = addr;
    u.segflg = USEG;
    dev = ip->addr[0];
    (*ciosw[cmajor(dev)].mode) (dev, flag);
}

/*
 * Gtty system call. Notice that the calling conventions
 * for the devive sgtty routine are not unix standard.
 */
gtty(fd, addr)
    int fd;
    char *addr;
{
    cmode(fd, addr, READ);
}

/*
 * Stty system call. Notice that the calling conventions
 * for the devive sgtty routine are not unix standard.
 */
stty(fd, addr)
    int fd;
    char *addr;
{
    cmode(fd, addr, WRITE);
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
 * Null write. Pretend to write all characters.
 */
nullwrite()
{
    u.count = 0;
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
