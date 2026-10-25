/*
 * per-process job control switches
 *
 * sys/job.c
 *
 * The shell decides what a job is.  It stops the pipeline it owns by
 * naming each process, resumes it the same way, and closes terminal
 * input for one that is to run in the background.  This file is the
 * whole of the kernel's part: two bits per process slot, and the call
 * that sets and clears them.
 *
 * The bits live here rather than in struct proc because that layout is
 * read directly by ps, kstat and the simulator, and none of them has
 * any business knowing about jobs.  Nothing in this file walks a job:
 * jobctl() acts on the one pid it is given, just as kill() does.
 */
#include <types.h>
#include <sys/sys.h>
#include <sys/proc.h>
#include <sys/tty.h>
#include <errno.h>

UINT8 jctl[NPROC];

/*
 * Set or clear a process' scheduling and terminal-input switches.
 *
 * jobctl(pid, op) - pid arrives in u.hl, as it does for kill(), and op
 * is the first call argument.  The caller must be the process' owner
 * (or the super-user), and it may not name itself: a process cannot
 * stop the process that is stopping things.
 */
jobctl(op)
    UINT8 op;
{
    register struct proc *p;
    register UINT8 *b;
    int pid;

    pid = u.hl;
    if ((p = procptr(pid)) == 0 || p == u.p || !(p->mode & ALIVE) ||
        (p->uid != u.u_uid && !super())) {
        u.error = ESRCH;
        return;
    }

    b = &jctl[p - plist];

    if (op & JOBSTOP)
        *b |= JOBSTOP;
    if (op & JOBCONT)
        *b &= ~JOBSTOP;
    if (op & JOBTTYOUT) {
        *b |= JOBTTYOUT;
        /*
         * A read already asleep in ttyread() waits on the raw queue
         * and would swallow the foreground's next line before it saw
         * the bit.  Wake it; the read checks the bit and returns eof.
         */
        if (p->tty)
            wakeup(&((struct tty *) p->tty)->rawque);
    } else if (op & JOBTTYIN)
        *b &= ~JOBTTYOUT;
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
