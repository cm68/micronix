/*
 * lock.c 
 */
#include <types.h>
#include <sys/sys.h>
#include <sys/inode.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/lock.h>

struct lock llist[NLOCK] = { 0 };

/*
 * lock system call
 */

reclock(fd, size)
    register int fd;
    register unsigned size;
{
    static struct file *fp;
    static struct inode *ip;
    static struct lock *l, *new;

    if ((fp = ofile(fd)) == NULL)
        return;

    /*
     * proposed new locked region
     */

    u.count = size;
    u.offset = fp->rwptr;

    ip = fp->inode;

    new = NULL;

    for (l = llist; l < llist + NLOCK; l++) {
        if (l->file) {
            if (l->file->inode == ip && lockhit(l)) {
                u.error = EBUSY;
                return;
            }
        }

        else {
            new = l;
        }
    }

    if (new == NULL) {
        u.error = ENFILE;       /* Lock table full. */
        return;
    }

    new->offset = u.offset;
    new->size = u.count;
    new->file = fp;
}

/*
 * unlock system call
 * 
 *      Remove a single lock.
 *      Unlock the region in which the reference byte lies.
 */

unlock(fd)
    register int fd;
{
    static struct file *fp;
    static struct lock *l;

    if ((fp = ofile(fd)) == NULL)
        return;

    u.offset = fp->rwptr;
    u.count = 1;

    for (l = llist; l < llist + NLOCK; l++)
        if (l->file == fp && lockhit(l))
            l->file = NULL;
}

/*
 * Check for region intersection.
 */

lockhit(l)
    register struct lock *l;
{
    return (u.offset + u.count) > l->offset
        && (l->offset + l->size) > u.offset;
}

/*
 * lclose - clear any remaining record locks
 */

lclose(fp)
    register struct file *fp;
{
    static struct lock *l;

    for (l = llist; l < llist + NLOCK; l++)
        if (l->file == fp)
            l->file = NULL;
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
