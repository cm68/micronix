/*
 * time.c 
 */
#include <sys/sys.h>
#include <sys/proc.h>

long seconds = 0;               /* system time */
int revel = 0;                  /* seconds to revelie for sleep system call */
char ticks = 0;

extern char resched, swapping, memwant, /* idle */ ;
extern struct proc *swapproc;

/*
 * Clock interrupt service routine
 * 8 HERTZ clock
 * We assume that HERTZ is a power of 2.
 */
clock()
{
    fast struct proc *p;
    fast int *sec;

    timein();                   /* see below */
    resched = YES;

    /*
     * Once per second stuff.
     * Bump the clock calendar
     * Do alarms & sleeps
     */

    if (!(++ticks & 7)) {       /* once a second */
        sec = &seconds;         /* avoid longs in interrupt code */

        if (++sec[1] == 0) {    /* NOT PORTABLE */
            ++sec[0];
        }

        if (--revel == 0) {     /* see snooze in sys2.c */
            wakeup(&revel);
        }

        if (memwant && !swapping && !(ticks & 31)) {
            run(swapproc);      /* every 4 seconds */
        }

        ei();

        for (p = plist; p < plist + NPROC; p++) {
            if (p->mode & ALIVE) {
                p->time++;      /* residency time */

                if (p->alarm && (--p->alarm == 0)) {
                    send(p, SIGALRM);
                }
            }
        }

        di();
    }
}

/*
 * Timeout code. This is a simplified version for use with
 * only a few timeout slots.
 */
#define NTMOUTS 5               /* max simultaneous timeouts */

struct tmout
{
    int (*func)();
    int arg;
    unsigned int ticks;
};

struct tmout tlist[NTMOUTS] = 0;        /* timeout list */

/*
 * Called from the clock interrupt HERTZ times per second
 * to update the countdowns and invoke the functions
 * that are ready.  In case the invocations take longer
 * than a clock period, the tmbusy flag prevents stumbling.
 */
timein()
{
    static char tmbusy = NO;
    fast struct tmout *t;

    if (tmbusy)
        return;

    tmbusy = YES;
    for (t = tlist; t < tlist + NTMOUTS; t++)
        if (t->func != NULL && --t->ticks == 0) {
            (*(t->func)) (t->arg);
            t->func = NULL;
        }
    tmbusy = NO;
}

/*
 * Timeout(func, arg, ticks) arranges that after >= ticks,
 * the clock interrupt will in turn call (*func)(arg).
 */
timeout(func, arg, ticks)
    int (*func)(), arg, ticks;
{
    fast struct tmout *t;

    di();
    for (t = tlist; t < tlist + NTMOUTS; t++)
        if (t->func == NULL) {
            t->func = func;
            t->arg = arg;
            t->ticks = ticks + 1;
            ei();
            return;
        }
    panic("Timeout slots are full");
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
