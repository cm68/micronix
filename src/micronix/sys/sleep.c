/*
 * process sleep and wakeup
 *
 * sys/sleep.c 
 * Changed: <2022-01-04 10:45:08 curt>
 */
#include <types.h>
#include <sys/sys.h>
#include <sys/proc.h>

extern char memwant, swapping;
extern struct proc *swapproc;

/*
 * Rescheduling flag. If 1, switch processes at next
 * opportunity. Set by run (below), tested in trap (trap.c),
 * and reset in next (below).
 */
char resched = 0;

/*
 * Hier to the run-time throne.
 * Set in sched() below, tested in system().
 */

/*
 * Idle flag. Prevents idle time from being charged to
 * the current process. Set and reset in sched() below,
 * tested in clock() (time.c).
 */

/*
 * char idle = 0; 
 */

/*
 * Broadcast a wakeup to all processes waiting on event.
 */
wakeup(event)
    int event;
{
    struct proc *p;

    for (p = plist; p < plist + NPROC; p++)
        if (p->event == event)
            run(p);
}

/*
 * Set the process running. (The actual cpu execution
 * is managed by next()). Called by wakeup above and by
 * send() in sig.c.
 */
run(p)
    struct proc *p;
{
    p->mode |= AWAKE;
    p->event = 0;
    if (p->mode & LOADED)
        resched = 1;
    else if (!swapping)
        run(swapproc);
}

/*
 * Sleep until a wakeup for event.
 */
sleep(event, pri)
    int event, pri;
{
    u.p->event = event;
    u.p->pri = pri;
    u.p->mode &= ~AWAKE;
    if (memwant && !(u.p->mode & LOCKED))
        run(swapproc);
    next();
    abort();                    /* abort on signal and low priority */
    enable();
}

/*
 * Suspend the current process and revive some other in-core
 * process. This is presently called by sleep, exit, and trap.
 * The revived process will return from its call to next, or
 * from its call to procopy (a subroutine of fork), whichever
 * last called saveframe. In the procopy case, this return 0
 * enables fork to distinguish between parent and child.
 * Warning: this is intimately connected with C's stack framing.
 */
next()
{
    register struct proc *n;        /* force stacking of reg variables */
    static int temp[12];        /* for use while changing stacks */

    if ((n = sched()) != u.p) {
        saveframe(&u.p->frmptr, &u.p->stkptr);
        di();
        setframe(&temp[12], &temp[12]);
        newmap(n);              /* re-write the segmentation registers */
        setframe(u.p->frmptr, u.p->stkptr);
    }
    resched = 0;
    enable();                   /* unconditionally enable interrupts */
    return 0;
}

/*
 * Find the next runnable in-core process.
 */
sched()
{
    register struct proc *next, *p;
    static struct proc *circle = plist;
    static UINT8 i;

    enable();                   /* allow wakeups while idling */
    next = 0;
    while (next == 0) {
        for (p = circle, i = 0; i < NPROC; p++, i++) {
            if (p >= &plist[NPROC])
                p = plist;
            if ((p->mode & (ALIVE | AWAKE | LOADED)) !=
                (ALIVE | AWAKE | LOADED))
                continue;
            if (next == 0 || priority(p) > priority(next)) {
                next = p;
            }
        }
    }
    circle = next + 1;
    return (next);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
