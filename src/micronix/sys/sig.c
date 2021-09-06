/*
 * sig.c 
 */
#include <types.h>
#include <sys/sys.h>
#include <sys/proc.h>
#include <sys/tty.h>
#include <sys/signal.h>
#include <errno.h>

#define TERMIN	0               /* code for terminate */
#define IGNORE	1               /* code for ignore */

/*
 * Signal system call
 */
signal(sig, func)
    register unsigned int sig, (*func) ();
{
    register int old;

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
    return (old);
}

/*
 * Kill system call.
 */
kill(pid, sig)
    int pid, sig;
{
    register struct proc *p;

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
killall(tty, sig)
    struct tty *tty;
    int sig;
{
    register struct proc *p;

    for (p = &plist[2]; p < plist + NPROC; p++)
        if (p->tty == tty)
            send(p, sig);
}

/*
 * Send a signal to a process.
 */
send(p, sig)
    register struct proc *p;
    register int sig;
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
    if (p->pri < SIGPRI && !(p->mode & AWAKE) && sig != SIGTINT)
        run(p);
}

/*
 * Set a process into background mode.
 * This means that all signals (except SIGKILL)
 * will be ignored, and an attempt to read a
 * a tty will return eof.
 */
setback(p)
    register struct proc *p;
{
    register int i;

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
sig()
{
    register char s;
    register int func;

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
core()
{
    register int fd;

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
abort()
{
    register int s;

    if ((s = sig()) == 0 || s == SIGTINT)
        return;
    u.error = EINTR;
    setframe(u.abort, u.abort);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
