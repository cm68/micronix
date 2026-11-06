/*
 * send and deliver signals
 * 
 * sys/sig.c 
 * Changed: <>
 */
#include <types.h>
#include <sys/sys.h>
#include <sys/proc.h>
#include <sys/tty.h>
#include <sys/signal.h>
#include <errno.h>
extern UINT8 super(void);
extern void killall(struct tty *, UINT8);
extern void send(struct proc *, UINT8);
extern void run(struct proc *);
extern void drainall(struct tty *);
extern void write(INT16, INT16);
extern void close(int fd);
extern UINT8 valid(UINT16, UINT16);

#define TERMIN	0               /* code for terminate */
#define IGNORE	1               /* code for ignore */

/*
 * Signal system call
 */

/* prototypes for functions defined below */
INT16 putword(int, char *);
struct proc * procptr(INT16);
void setframe(char *, char *);
void
signal(sig, func)
    INT16 sig;
    register INT16 (*func) ();
{
    register INT16 old;

    if (sig >= NSIG || sig == SIGKILL || sig == 0) {
        u.error = EINVAL;
        return;
    }
    old = u.p->slist[sig];
    if (func == TERMIN && (sig == SIGTINT || sig == SIGBACK))
        func = IGNORE;
    u.p->slist[sig] = func;
    if ((u.p->slist[0] == sig) && (func == IGNORE))
        u.p->slist[0] = 0;
    u.hl = old;
}

/*
 * Kill system call.
 */
void
kill(sig)
    INT16 sig;
{
    register struct proc *p;
    INT16 pid;

    pid = u.hl;
    if (pid == 0) {
        killall(u.p->tty, sig);
        return;
    }
    p = procptr(pid);
    if (p != 0 && p != u.p && (p->mode & ALIVE) &&
        (p->uid == u.u_uid || super()))
        send(p, sig);
    else
        u.error = ESRCH;
}

/*
 * Signal all processes with the given controlling tty.
 */
void
killall(tty, sig)
    struct tty *tty;
    UINT8 sig;
{
    register struct proc *p;

    for (p = &plist[2]; p < plist + NPROC; p++)
        if (p->tty == tty)
            send(p, sig);
}

/*
 * Send a signal to a process.
 */
void
send(p, sig)
    register struct proc *p;
    register UINT8 sig;
{
    if (!(p->mode & ALIVE))
        return;
    if (p->slist[sig] == IGNORE) {
        /*
         * if (sig == SIGBACK)
         * setback(p); 
         */

        return;
    }
    if (p->slist[0] == SIGKILL)
        return;
    p->slist[0] = sig;
    /*
     * A stopped job still has to die.  The scheduler will not look at
     * it, so the one signal the shell's stop cannot hold off lets it
     * through - otherwise ^Z would make a process unkillable.
     */
    if (sig == SIGKILL)
        jctl[p - plist] &= ~JOBSTOP;
    if (p->pri < SIGPRI && !(p->mode & AWAKE) && sig != SIGTINT)
        run(p);
}

/*
 * Set a process into background mode.
 * This means that all signals (except SIGKILL)
 * will be ignored, and an attempt to read a
 * a tty will return eof.
 */
void
setback(p)
    register struct proc *p;
{
    register INT16 i;

    p->mode |= BACK;
    for (i = 0; i < NSIG; i++)
        p->slist[i] = IGNORE;
    p->slist[SIGKILL] = TERMIN;
}

/*
 * Process a waiting signal, if any. Called from syscall, clock,
 * and abort. If the signal is to be caught, imitate an interrupt
 * by pushing the user's pc onto his stack, and setting his
 * his pc to the service routine. If the process is to terminate,
 * do the same thing, but point the pc at an exit system call.
 * The trouble with calling exit() directly from here is that
 * the clock interrupt might never return, which complicates
 * the handling of that interrupt.
 */
INT16
sig()
{
    register UINT8 s;
    register INT16 func;

    s = u.p->slist[0];

    if (!s)
        return 0;              /* no signal to process */

    func = u.p->slist[s];

    if (func == IGNORE || u.p->pri >= SIGPRI)
        return 0;

    if (func == TERMIN) {
        u.p->status = s;

        /*
         * if (s == SIGQUIT || s == SIGILL || s == SIGBAD) u.p->status |=
         * 0200; /* exit will dump core * 
         */
        func = setexit();
    }

    u.p->slist[0] = 0;

    if (s != SIGTINT)
        u.p->slist[s] = 0;

    if (s == SIGINT || s == SIGQUIT)
        drainall(u.p->tty);

    valid(--u.sp, 2);           /* allocate user memory if necessary */
    putword(u.pc, u.sp);
    u.pc = func;
    return (s);
}

#ifdef notdef
/*
 * Create a core image file in the current directory
 * Called by exit().
 */
INT16
core()
{
    register INT16 fd;

#define UADDR 9
#define K32 0x8000

    u.error = 0;
    copyout("core", UADDR, 5);
    if ((fd = creat(UADDR, 0777)) >= 0) {
        write(fd, 0, K32);
        write(fd, K32, K32);
        copyout(u.p, 0, sizeof(struct proc));
        write(fd, 0, sizeof(struct proc));
        close(fd);
    } else
        u.p->status &= ~0200;
    u.error = 0;
}
#endif  /* notdef */

/*
 * If priority is low and a signal is waiting,
 * abort the current system call. Called by sleep.
 */
void
abort(dummy)
    INT16 dummy;      /* unused: forces a 1-argument frame, see setframe */
{
    UINT8 s;
    volatile INT16 force_frame;   /* force an IY-based exit, like next() */

    if ((s = sig()) == 0 || s == SIGTINT)
        return;
    u.error = EINTR;
    setframe(u.abort, u.abort);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
