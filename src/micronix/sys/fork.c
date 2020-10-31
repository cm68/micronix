/*
 * fork.c 
 */
#include "sys.h"
#include "inode.h"
#include "proc.h"
#include "file.h"
#include "con.h"

extern struct proc *initproc, *swapproc;

/*
 * Fork system call.
 */
fork()
{
    fast struct proc *child;
    fast struct file **o;

    /*
     * Allocate a process slot.
     */

    for (child = plist; child < plist + NPROC; child++)
        if (!(child->mode & ALLOC))
            goto found;

    /*
     * pr("Process table is full!"); 
     */

    u.error = EAGAIN;
    return;

  found:

    /*
     * Bump the reference count on the current directory
     * and all the open files.
     */

    u.cdir->count++;

    for (o = u.olist; o < u.olist + NOPEN; o++)
        if (*o != NULL)
            (*o)->count++;

    if (procopy(child))
        return (procid(child)); /* in parent */
    else
        return (0);             /* in child */
}

/*
 * Copy the current proc structure to child, update
 * the proc structures as necessary, and copy the
 * current core image to another bank or to disk.
 * Then return YES. Later, the copy will be activated
 * by next() and will return from this level with NO.
 */
procopy(child)
    fast struct proc *child;    /* see comments to next (sleep.c) */
{
    static char bank;
    fast int s;
    fast char mode;

    saveframe(&u.p->frmptr, &u.p->stkptr);
    copy(u.p, child, sizeof(struct proc));
    child->time = 0;
    child->parent = u.p;
    child->pid = newpid();

    if (mget(child)) {          /* try to get primary memory */
        bankcopy(child);        /* malloc.c */
    }

    else {
        mode = u.p->mode;
        u.p->mode |= LOCKED;    /* prevent swap */

        if (swapdev && swapout(child)) {
            run(swapproc);
        } else {
            zero(child, sizeof(struct proc));
            u.error = EAGAIN;
        }

        u.p->mode = mode;
    }

    return YES;
}

/*
 * Turn a process pointer into a process id
 */
procid(p)
    struct proc *p;
{
    return p->pid;
}

/*
 * Turn a process id into a process pointer
 */
struct proc *
procptr(id)
{
    struct proc *p;

    for (p = plist; p < plist + NPROC; p++)
        if ((p->pid == id) && (p->mode & ALLOC))
            return p;

    return NULL;                /* No such process ID */
}

/*
 * Generate a new process ID number 
 */

newpid(a)
{
    unsigned new;
    static pid = 1;

    for (;;) {
        new = pid;              /* Pick a new number. */

        if (++pid > 30000) {    /* Bump the serial number. */
            pid = 1;
        }

        if (!procptr(new)) {    /* Return it if not in use */
            return new;
        }
    }
}

/*
 * Exit from a process. Save the status for the parent,
 * close all open files, decrement the current directory,
 * and give any children to the initialization process.
 * For uniformity, this is called only as a user system
 * call. When setsig (sig.c) wants to force an exit, it
 * writes an exit system call on the user's stack and
 * forces a return to it.
 * If the exit was caused by a core-dump signal,
 * then core is dumped.
 */
exit(stat)
    int stat;
{
    fast int fd;
    fast struct proc *p;

    u.p->status |= (stat << 8);

    /*
     * if (u.p->status & 0200) core(); 
     */
    for (fd = 0; fd < NOPEN; fd++) {
        close(fd);
        u.error = 0;
    }
    for (p = plist; p < plist + NPROC; p++) {
        if (p->parent == u.p)
            if (!(p->mode & ALIVE))
                zero(p, sizeof(*p));
            else
                p->parent = initproc;
    }
    u.cdir->count--;
    u.p->mode &= ~(ALIVE | AWAKE);
    wakeup(u.p->parent);
    mfree(u.p);
    next();
}

/*
 * Wait system call
 */
wait(addr)
    int *addr;                  /* where to return the status */
{
    char child;                 /* children detection flag */
    unsigned pid;               /* process id */
    struct proc *p;

    for (;;) {
        child = NO;             /* no child found yet */

        /*
         * Search for child.
         */

        for (p = plist; p < plist + NPROC; p++) {
            if (p->parent == u.p) {
                child = YES;

                /*
                 * If the child is a zombie,
                 * return immediately.
                 */

                if (!(p->mode & ALIVE)) {       /* dead */
                    *addr = p->status;  /* set r1 */
                    pid = procid(p);
                    zero(p, sizeof(*p));        /* free it */
                    return pid;
                }
            }
        }

        if (child == NO) {
            u.error = ECHILD;   /* No kids. */
            return;
        }

        sleep(u.p, PRIWAIT);    /* wait for kids to die */
    }

    u.error = ECHILD;
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
