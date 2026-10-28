/*
 * the tty driver's resident half
 *
 * sys/ttyres.c
 *
 * The line discipline and the board front ends are a module now
 * (tty.mod), so that the bytes they used to take out of the resident
 * kernel go back to the buffer pool.  These are the pieces that cannot
 * go with them:
 *
 *   - the state, because p->tty and the queue sleepers name it and a
 *     module page is only mapped while its driver is running;
 *   - the helpers that walk the process table and the start/stop
 *     pointers both tty copies use, because the copies are two pages
 *     and a shared helper is cheaper once than twice;
 *   - the three entries the resident interrupt stubs call (ttyin,
 *     ttyout, mumint), which have to map the driver's page around the
 *     call - an interrupt runs with whatever page the interrupted
 *     process had, never the driver's.
 *
 * The driver's init publishes its segment and those three entry points
 * here (ttypub).  Nothing else in the kernel calls into the tty page;
 * the char-device entries reach it through the ovlc trampolines.
 */
#include <types.h>
#include <sys/sys.h>
#include <sys/tty.h>
#include <sys/proc.h>
#include <sys/signal.h>
#include <sys/con.h>
#include <sys/pty.h>

extern UINT8 map0[];
extern int drainques();
extern int selectchan;          /* select(2): a hung-up tty reads EOF */

/*
 * The terminals on the MultIO/SS1 boards.  One object for the whole
 * machine: the tty layer, the process's p->tty, and the sleepers on the
 * queues all name this storage, so it cannot live in a page that is
 * mapped only while the driver runs.
 */
struct tty mttys[TTY_SLOTS] = { 0 };

/*
 * cookin()'s partial-line scratch.  It was a static in sys/tty.c; both
 * the console copy and the pty copy of the line discipline need one, and
 * the console and a pty can be cooking at the same time, so it is shared
 * here rather than duplicated in each page.  cookin() never sleeps while
 * holding it, so one buffer serves every tty, exactly as the static did.
 */
char ttycbuf[TTYHOG];

/*
 * Signal the processes on a terminal that are in its foreground job.
 *
 * The terminal's own signals - interrupt, quit and input available -
 * belong to the foreground job.  A process the shell has stopped, or
 * one whose terminal input it has closed, is not in it, so ^C typed at
 * the shell must not reach it.  An explicit kill still does; this only
 * filters what the tty generates.
 */
killfg(tty, sig)
    register struct tty *tty;
    UINT8 sig;
{
    register struct proc *p;
    register UINT8 j;

    for (p = plist; p < plist + NPROC; p++) {
        j = jctl[p - plist];
        if (p->tty == tty && (p->mode & ALIVE) &&
            !(j & (JOBSTOP | JOBTTYOUT)))
            send(p, sig);
    }
}

/*
 * Tell the shell that ^Z was typed.
 *
 * The terminal does not know which processes form the foreground job,
 * and must not guess: stopping every process on the tty would stop the
 * shell with them, and stopping none would leave the job running.  So
 * the kernel only notifies the shell - the process asleep in wait()
 * that has asked for SIGBACK - and the shell stops the pipeline it
 * recorded.  It is the handler, not the default, that identifies the
 * shell here: a program that calls wait() but never asked for the
 * signal is left alone.
 */
ttynotify(tty)
    register struct tty *tty;
{
    register struct proc *p;

    for (p = plist; p < plist + NPROC; p++)
        if (p->tty == tty && (p->mode & ALIVE) && !(p->mode & AWAKE) &&
            p->event == (UINT) p && p->slist[SIGBACK] > 1)
            break;

    if (p >= plist + NPROC)
        return;

    send(p, SIGBACK);
}

/*
 * Hang the pty slave up.  The open descriptors stay open but stop
 * working: reads on either end return end of file and writes fail, and
 * the processes that were using the slave are told (SIGHUP) rather than
 * killed, so a shell can notice and exit on its own.
 *
 * It is here rather than with the rest of the pty driver for the same
 * reason killfg() and ttynotify() are: it is a walk of the process
 * table over resident state, it is not on any hot path (the last master
 * close with a slave still open), and the pty page has no bytes to
 * spare for it.
 */
int
ptyhang(i)
    int i;
{
    register struct tty *s, *m;
    register struct proc *pr;

    s = SLAVE(i);
    m = MASTER(i);
    ptyinfo[i].hung = 1;

    s->mstate |= TTYHUP;
    m->mstate |= TTYHUP;

    for (pr = plist; pr < plist + NPROC; pr++)
        if (pr->tty == s && (pr->mode & ALIVE))
            send(pr, SIGHUP);

    drainques(s);
    drainques(m);

    wakeup(&s->rawque);
    wakeup(&m->rawque);
    wakeup(&s->outque);
    wakeup(&m->outque);
    wakeup(&selectchan);        /* the end of file is readable */
}

/*
 * Called on wrong baud rate detection.  The resident interrupt stubs
 * call this; it touches nothing in the driver's page.
 */
ttyerror(t)
    struct tty *t;
{
    t->state |= ERROR;
}

/*
 * Called on carrier present detection.
 */
ttyconnect(t)
    struct tty *t;
{
    t->mstate |= CD;

    if ((t->mstate & (WOPEN | DIALER)) == WOPEN) {
        wakeup(&t->mstate);
    }
}

/*
 * Conditionally start output.  The tty->start pointer it reaches belongs
 * to whichever page the caller is running in, which is the page this
 * helper was called from.
 */
cstart(tty)
    register struct tty *tty;
{
    if (!(tty->state & LOSTOP))
        (*tty->start) (tty);
}

/*
 * Unconditionally start output.
 */
ustart(tty)
    register struct tty *tty;
{
    tty->state &= ~LOSTOP;
    (*tty->start) (tty);
}

/*
 * The driver page: its segment, and the entries the resident interrupt
 * stubs call.  The init publishes them (sys/multio.c); the pointers are
 * OVLBASE-relative addresses in the module, valid while its page is
 * mapped, which the wrappers below arrange.
 */
static UINT ttyseg;
static int (*ttyin_m) ();
static int (*ttyout_m) ();
static int (*mumint_m) ();
static UINT ttyready;           /* the console tty is initialized and open */

ttypub(seg, in, out, mum)
    UINT seg;
    int (*in) ();
    int (*out) ();
    int (*mum) ();
{
    ttyseg = seg;
    ttyin_m = in;
    ttyout_m = out;
    mumint_m = mum;
    ttyready = 0;
    return (0);
}

/*
 * The three doors from the resident interrupt stubs into the driver.
 * Each maps the driver's page for the call and puts the interrupted
 * process's page back afterwards - the same save and restore ovlcall
 * does, with arguments.  They stay shut until the console has been
 * opened (console_open below): the page is placed before it is opened,
 * and an input interrupt in that window would run the line discipline on
 * an mttys[] that ttinit() has not filled in.  A shut door drops the
 * character, which is what a port nobody has opened hears.
 */
int
ttyin(c, tty)
    register UINT8 c;
    register struct tty *tty;
{
    UINT8 drv;
    int r;

    if (!ttyready || ttyseg == 0 || ttyin_m == 0)
        return (0);
    drv = u.drv;
    u.drv = (UINT8) ttyseg;
    map0[2 * OVLSEG] = (UINT8) ttyseg;
    r = (*ttyin_m) (c, tty);
    u.drv = drv;
    map0[2 * OVLSEG] = drv;
    return (r);
}

int
ttyout(tty)
    register struct tty *tty;
{
    UINT8 drv;

    if (!ttyready || ttyseg == 0 || ttyout_m == 0)
        return (0);
    drv = u.drv;
    u.drv = (UINT8) ttyseg;
    map0[2 * OVLSEG] = (UINT8) ttyseg;
    (*ttyout_m) (tty);
    u.drv = drv;
    map0[2 * OVLSEG] = drv;
    return (0);
}

int
mumint(tty)
    register struct tty *tty;
{
    UINT8 drv;

    if (!ttyready || ttyseg == 0 || mumint_m == 0)
        return (0);
    drv = u.drv;
    u.drv = (UINT8) ttyseg;
    map0[2 * OVLSEG] = (UINT8) ttyseg;
    (*mumint_m) (tty);
    u.drv = drv;
    map0[2 * OVLSEG] = drv;
    return (0);
}

/*
 * Open /dev/console at boot (sys/inits.s calls this).  The driver is a
 * module now, so this is the device table's own door rather than a
 * direct call into it: ciosw[1].open is the char trampoline, which maps
 * the module and runs its open.
 */
console_open(dev)
    register dev;
{
    int r;

    r = (*ciosw[1].open) (dev, 0);
    ttyready = 1;
    return (r);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
