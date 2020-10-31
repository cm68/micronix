/*
 * oopen.c 
 */
#include "sys.h"
#include "inode.h"
#include "proc.h"
#include "file.h"
#include "con.h"

/*
 * Open system call.
 */
open(name, mode)
    char *name;
    unsigned int mode;
{
    fast struct inode *ip;
    fast int fd;

    if ((ip = iname(name)) == NULL)
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
    fast struct inode *ip;
    fast struct file *rf, *wf;
    int rd, wd;

    vec[0] = vec[1] = -1;
    if ((rf = falloc()) == NULL || (rd = oalloc()) == ERROR)
        return;
    rf->count = 1;
    rf->mode = IREAD | PIPE;
    u.olist[rd] = rf;
    if ((wf = falloc()) == NULL || (wd = oalloc()) == ERROR ||
        (ip = ialloc(rootdev)) == NULL) {
        rf->count = 0;
        u.olist[rd] = NULL;
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
    fast struct file *fp;
    fast int dev, fd;
    static char writ;

    writ = mode & IWRITE;
    if (writ && (ip->mode & I1WRITE))
        ip->flags |= IWRLOCK;
    dev = ip->addr[0];
    switch (ip->mode & ITYPE) {
    case IDIR:
        if (writ)
            u.error = EISDIR;
        break;
    case ICIO:
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
    case IBIO:
        bopen(dev, writ);
    }
    if (u.error || (fp = falloc()) == NULL || (fd = oalloc()) == ERROR) {
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
    fast struct file *fp;

    if ((fp = ofile(fd)) == NULL)
        return;
    u.olist[fd] = NULL;
    fclose(fp);
}

/*
 * Close a file, from close and exit.
 * Clear all region locks on last close.
 */
fclose(fp)
    fast struct file *fp;
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
    fast struct inode *ip;
    int writ;
{
    fast int dev;

    if (--ip->count <= 0) {
        ilock(ip);
        dev = ip->addr[0];
        switch (ip->mode & ITYPE) {
        case ICIO:
            cclose(dev, writ);
            break;
        case IBIO:
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
    fast struct file *fp;

    for (fp = flist; fp < flist + NFILE; fp++)
        if (fp->count <= 0) {
            zero(fp, sizeof(*fp));
            return (fp);
        }
    u.error = ENFILE;
    return NULL;
}

/*
 * Allocate an open file slot
 */
oalloc()
{
    fast struct file **op, **otop;

    for (op = u.olist, otop = op + NOPEN; op < otop; op++)
        if (*op == NULL)
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
    fast struct file *fp;

    if (fd < NOPEN && (fp = u.olist[fd]) != NULL)
        return (fp);
    u.error = EBADF;
    return NULL;
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
