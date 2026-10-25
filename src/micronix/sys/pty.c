/*
 * pseudo-terminals
 *
 * sys/pty.c
 *
 * A pty is a pair of terminals wired back to back: the slave, which
 * looks like a terminal to the programs running on it, and the master,
 * which the program holding the pty reads and writes.  Both are ordinary
 * struct tty, and both ends go through the same tty code - ttyin() gives
 * the slave's input its line discipline, so ^C, ^Z, erase, echo and the
 * rest work; ttywrite() on the slave cooks its output; ttyread() on
 * either end takes what the other wrote.  What pty adds is only the four
 * back-end calls (start/stop/put/set) and the pairing: a put() hands the
 * character to the other end's ttyin().
 *
 * The master is a terminal in the mechanical sense only, so its mode is
 * RAW and carries no mstate: no signals, no echo, no line
 * discipline, and the job-control input gate does not apply to it (a
 * daemon holding a master is not a background terminal job).
 *
 * Devices, major 5 (ciosw, sys/consts.c):
 *
 *	minor 0 .. NPTY-1		master:  /dev/ptyp0, /dev/ptyp1
 *	minor 8 .. 8+NPTY-1		slave:   /dev/ttyp0,  /dev/ttyp1
 *
 * A slave cannot be opened before its master (ENXIO), and a master write
 * with no slave open fails EIO.  Closing the last master while a slave
 * is still open hangs the slave up: reads on both ends return end of
 * file and the processes on the slave are sent SIGHUP.
 *
 * The helpers that hold no storage - the pair decode, the back-end
 * calls, init and hangup - are in sys/pty_leaf.c, in the u page, where
 * there is room; the state below is resident because it is one object
 * shared by every process.
 */
#include <types.h>
#include <sys/sys.h>
#include <sys/tty.h>
#include <sys/proc.h>
#include <sys/signal.h>
#include <sys/con.h>
#include <sys/pty.h>
#include <errno.h>

extern int ttyread(), ttywrite(), ttymode(), ttyopen(), ttyclose();
extern int cstart(), drainques(), qinit(), sendque();
extern int pty_start(), pty_stop(), pty_put();
extern int ptymode();

/*
 * Put one end of a pair in the state a fresh terminal has.  The two
 * ends differ only in mode and device number, so both come through
 * here; the pty leaf half's init calls it too.
 */
ttyinit1(t, dev, mode, ms)
    register struct tty *t;
    int dev;
    UINT mode;
    UINT8 ms;
{
    t->ispeed = t->ospeed = 13;		/* 9600 */
    t->erase = '\b';
    t->kill = 'X' - 'A' + 1;
    t->mode = mode;
    t->dev = dev;
    t->state = 0;
    t->mstate = ms;
    t->col = 0;
    t->line = 0;
    t->count = 0;
    t->nextc = 0;
    t->nbreak = 0;

    t->start = pty_start;
    t->stop = pty_stop;
    t->put = pty_put;
    t->set = pty_stop;		/* set() is only called to change speed */

    qinit(&t->rawque);
    qinit(&t->cokque);
    qinit(&t->outque);
}

/*
 * Put the pair back to the state a fresh terminal has.
 */
int
ptyinit(i)
    int i;
{
    ttyinit1(SLAVE(i), (PTYMAJ << 8) | (PTYSLV + i),
        ECHO | MAPCR | XTABS, 0);
    ttyinit1(MASTER(i), (PTYMAJ << 8) | (PTYMST + i), RAW, 0);
    ptyinfo[i].hung = 0;
}

ptyopen(dev, mode)
    int dev;
{
    register int i;
    register struct ptyinfo *p;

    if ((i = ptyidx(dev)) < 0) {
        u.error = ENXIO;
        return;
    }
    p = &ptyinfo[i];

    if (!ISMASTER(dev)) {
        if (!p->mopen) {
            u.error = ENXIO;	/* no master: there is no terminal */
            return;
        }
        if (!p->sopen)
            ptyinit(i);
        p->sopen++;
        ttyopen(SLAVE(i));
        return;
    }

    if (!p->mopen) {
        if (p->sopen) {
            /*
             * A new master over slave descriptors that are still
             * open: the hangup is over, but the slave's open count
             * stays what it was.
             */
            p->hung = 0;
            SLAVE(i)->mstate &= ~TTYHUP;
            MASTER(i)->mstate &= ~TTYHUP;
            drainques(SLAVE(i));
            drainques(MASTER(i));
        } else
            ptyinit(i);
    }
    p->mopen++;
}

ptyclose(dev, mode)
    int dev;
{
    register int i;
    register struct ptyinfo *p;

    if ((i = ptyidx(dev)) < 0)
        return;
    p = &ptyinfo[i];

    if (ISMASTER(dev)) {
        if (p->mopen)
            p->mopen--;
        if (!p->mopen) {
            if (p->sopen)
                ptyhang(i);
            else
                ptyinit(i);
        }
        return;
    }

    if (p->sopen)
        p->sopen--;
    ttyclose(SLAVE(i));
    if (!p->sopen && !p->mopen)
        ptyinit(i);
}

/*
 * Hang the slave up.  The open descriptors stay open but stop working:
 * reads on either end return end of file and writes fail, and the
 * processes that were using the slave are told (SIGHUP) rather than
 * killed, so a shell can notice and exit on its own.  This is resident
 * rather than in the leaf half because it walks the process table and
 * drains both ends; there is no room for all of that in the u page.
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
}

/*
 * Master read: take what the slave wrote from the master's raw queue.
 * The master is not a terminal, so this is not ttyread(): the job
 * control input gate must not apply to the process holding the master.
 * When input has been taken, the slave's output queue has room to drain.
 */
ptyread(dev)
    int dev;
{
    register int i;
    register struct ptyinfo *p;
    register struct tty *m;

    if ((i = ptyidx(dev)) < 0) {
        u.error = ENXIO;
        return;
    }
    p = &ptyinfo[i];

    if (!ISMASTER(dev)) {
        ttyread(SLAVE(i));
        cstart(MASTER(i));
        return;
    }

    if (!p->sopen || p->hung)
        return;			/* end of file */
    m = MASTER(i);
    while (u.count && m->rawque.count == 0) {
        if (p->hung)
            break;
        sleep(&m->rawque, PRITTI);
    }
    if (u.count && m->rawque.count)
        sendque(&m->rawque);
    cstart(SLAVE(i));
}

/*
 * Master write: ordinary ttywrite() on the raw master; its put() hands
 * each byte to the slave's line discipline.
 */
ptywrite(dev)
    int dev;
{
    register int i;
    register struct ptyinfo *p;

    if ((i = ptyidx(dev)) < 0) {
        u.error = ENXIO;
        return;
    }
    p = &ptyinfo[i];

    if (!ISMASTER(dev)) {
        if (SLAVE(i)->mstate & TTYHUP)
            u.error = EIO;
        else
            ttywrite(SLAVE(i));
        return;
    }

    if (!p->sopen || p->hung) {
        u.error = EIO;
        return;
    }
    ttywrite(MASTER(i));
}

/*
 * The character entries the kernel copies into ovlcvec[5] when this
 * module is placed (sys/ovl.c).  The major must exist in ciosw, which
 * is why sys/consts.c carries a row of ovl trampolines there.
 */
struct ciovec ptycvec = {
    &ptyopen, &ptyclose, &ptyread, &ptywrite, &ptymode
};

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
