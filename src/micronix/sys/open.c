/*
 * open.c 
 */
#include <types.h>
#include <sys/sys.h>
#include <sys/fs.h>
#include <sys/stat.h>
#include <sys/inode.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/con.h>
#include <errno.h>

/*
 * Open system call.
 */
open(name, mode)
    char *name;
    unsigned int mode;
{
    register struct inode *ip;
    register int fd;

    if ((ip = iname(name)) == 0)
        return;
    mode = imode(mode);         /* translate to inode mode */
    if (!access(ip, mode)) {
        irelse(ip);
        return;
    }
    fd = iopen(ip, mode);
    irelse(ip);
    return (fd);
}

/*
 * Pipe system call.
 */
pipe(vec)
    int *vec;
{
    register struct inode *ip;
    register struct file *rf, *wf;
    int rd, wd;

    vec[0] = vec[1] = -1;
    if ((rf = falloc()) == 0 || (rd = oalloc()) == ERROR)
        return;
    rf->count = 1;
    rf->mode = IREAD | PIPE;
    u.olist[rd] = rf;
    if ((wf = falloc()) == 0 || (wd = oalloc()) == ERROR ||
        (ip = ialloc(rootdev)) == 0) {
        rf->count = 0;
        u.olist[rd] = 0;
        return;
    }
    wf->count = 1;
    wf->mode = IWRITE | PIPE;
    u.olist[wd] = wf;
    rf->inode = wf->inode = ip;
    ip->count = 2;
    ip->flags |= IPIPE;         /* for iwrite() */
    vec[0] = rd;
    vec[1] = wd;
    irelse(ip);
}

/*
 * Open a file from its inode.
 * Called from open and create.
 */
iopen(ip, mode)
    struct inode *ip;
    int mode;
{
    register struct file *fp;
    register int dev, fd;
    static char writ;

    writ = mode & IWRITE;
    if (writ && (ip->i_mode & ISVTX))
        ip->flags |= IWRLOCK;
    dev = ip->i_addr[0];
    switch (ip->i_mode & IFMT) {
    case IFDIR:
        if (writ)
            u.error = EISDIR;
        break;
    case IFCHR:
        {

            /*
             * Don't sleep with a locked inode.
             * L.W.E. March 1984
             */

            ip->count++;
            irelse(ip);
            copen(dev, writ);   /* sets u.p->tty */
            ilock(ip);
            ip->count--;
            break;
        }
    case IFBLK:
        bopen(dev, writ);
    }
    if (u.error || (fp = falloc()) == 0 || (fd = oalloc()) == ERROR) {
        ip->flags &= ~IWRLOCK;
        return ERROR;
    }
    u.olist[fd] = fp;
    fp->inode = ip;
    fp->count = 1;
    fp->mode = mode;
    ip->count++;
    return (fd);
}

/*
 * Close system call
 */
close(fd)
    int fd;
{
    register struct file *fp;

    if ((fp = ofile(fd)) == 0)
        return;
    u.olist[fd] = 0;
    fclose(fp);
}

/*
 * Close a file, from close and exit.
 * Clear all region locks on last close.
 */
fclose(fp)
    register struct file *fp;
{
    static struct inode *ip;
    static UINT writ;

    if (--fp->count > 0)
        return;

    /*
     * lclose (fp); /* inform the record locking system of file closure 
     */

    ip = fp->inode;
    writ = fp->mode & IWRITE;
    if (writ)
        ip->flags &= ~IWRLOCK;
    if (fp->mode & PIPE)
        wakeup(ip);             /* wake any pipe dreamers */
    iclose(ip, writ);
}

       /*
        * Close an inode, from fclose and umount.
        */
iclose(ip, writ)
    register struct inode *ip;
    int writ;
{
    register int dev;

    if (--ip->count <= 0) {
        ilock(ip);
        dev = ip->i_addr[0];
        switch (ip->i_mode & IFMT) {
        case IFCHR:
            ip->count++;
            irelse(ip);
            cclose(dev, writ);
            ilock(ip);
            ip->count--;
            break;
        case IFBLK:
            bflush(dev);
            bclose(dev, writ);
        }
        idec(ip);
    }
}

/*
 * Allocate a file structure
 */
struct file *
falloc()
{
    register struct file *fp;

    for (fp = flist; fp < flist + NFILE; fp++)
        if (fp->count <= 0) {
            zero(fp, sizeof(*fp));
            return (fp);
        }
    u.error = ENFILE;
    return 0;
}

/*
 * Allocate an open file slot
 */
oalloc()
{
    register struct file **op, **otop;

    for (op = u.olist, otop = op + NOPEN; op < otop; op++)
        if (*op == 0)
            return (op - u.olist);
    u.error = EMFILE;
    return ERROR;
}

/*
 * Check a file descriptor and return a pointer to
 * to its file structure.
 */
struct file *
ofile(fd)
    unsigned int fd;
{
    register struct file *fp;

    if (fd < NOPEN && (fp = u.olist[fd]) != 0)
        return (fp);
    u.error = EBADF;
    return 0;
}

/*
 * Translate a user-provided mode (from open)
 * to a mode for inodes and file structures.
 */
imode(m)
    int m;
{
    switch (m) {
    case 0:
        return (IREAD);
    case 1:
        return (IWRITE);
    default:
        return (IREAD | IWRITE);
    }
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
