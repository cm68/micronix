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
 * The stateless helpers - the pair decode and the four back-end calls -
 * are in sys/pty_leaf.c, linked into the driver's own page (pty.mod)
 * where the line discipline that calls them lives.  The pair's init is
 * here; its hangup walk is resident (sys/ttyres.c) with killfg() and
 * ttynotify(), because it walks the process table and it is not on any
 * hot path.  The state is resident because it is one object shared by
 * every process.
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
extern int cstart(), ustart(), drainques(), qinit(), sendque();
extern int pty_start(), pty_stop(), pty_put();
extern int ptyioctl();
extern int ptyhang();            /* the process-table walk, resident */
extern int selectchan;          /* select(2): a reader may be watching */

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
    register struct tty *s, *m;

    if ((i = ptyidx(dev)) < 0) {
        u.error = ENXIO;
        return;
    }
    p = &ptyinfo[i];
    s = SLAVE(i);
    m = MASTER(i);

    if (!ISMASTER(dev)) {
        if (!p->mopen) {
            u.error = ENXIO;	/* no master: there is no terminal */
            return;
        }
        if (!p->sopen)
            ptyinit(i);
        p->sopen++;
        ttyopen(s);
        /*
         * A process that opens a slave means it as its terminal.  The
         * daemon that forked it already had the console as its
         * controlling tty, and ttyopen() only claims one for a process
         * that has none - so the claim is made here, over whatever was
         * there.  That is what gives the shell on an rshd's pty its
         * job control and its hangup; there is no setsid(2) to do it
         * the BSD way.
         */
        u.p->tty = s;
        return;
    }

    /*
     * The pair a session gets is the daemon's to allocate (rshd picks a
     * free one before it forks); the driver has no open count to refuse
     * with, and two masters on one pair would split a shell's bytes.
     */
    if (p->sopen) {
        /*
         * A new master over slave descriptors that are still open: the
         * hangup is over, but the slave's open count stays what it was.
         */
        p->hung = 0;
        s->mstate &= ~TTYHUP;
        m->mstate &= ~TTYHUP;
        drainques(s);
        drainques(m);
    } else
        ptyinit(i);
    p->mopen++;
}

ptyclose(dev, mode)
    int dev;
{
    register int i;
    register struct ptyinfo *p;
    register struct tty *s, *m;

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

    s = SLAVE(i);
    m = MASTER(i);
    if (p->sopen)
        p->sopen--;
    ttyclose(s);
    if (!p->sopen && !p->mopen)
        ptyinit(i);
    else if (!p->sopen) {
        /*
         * The last slave is gone while a master still holds the pair:
         * the master's read is now end of file.  Say so on both ends
         * and wake a selector; without this a session whose shell
         * exited would sit in select forever - the master has nothing
         * to read and no hangup bit to report.
         */
        s->mstate |= TTYHUP;
        m->mstate |= TTYHUP;
        wakeup(&m->rawque);         /* a master read sleeping for output */
        wakeup(&selectchan);
    }
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
    register UINT want;
    register struct ptyinfo *p;
    register struct tty *s, *m;

    if ((i = ptyidx(dev)) < 0) {
        u.error = ENXIO;
        return;
    }
    p = &ptyinfo[i];
    s = SLAVE(i);
    m = MASTER(i);

    if (!ISMASTER(dev)) {
        ttyread(s);
        cstart(m);
        return;
    }

    /*
     * The slave's output sits in its own outque until its start routine
     * moves it to the master's raw queue, and a shell that writes and
     * exits leaves it there.  Move it, take it, and only then decide
     * end of file: checking the hangup first stranded the last output
     * of every quick command.  ustart, not cstart, because the peer
     * queue having been full is exactly the stopped state a read is
     * here to clear.
     */
    want = u.count;
    for (;;) {
        ustart(s);
        if (u.count && m->rawque.count)
            sendque(&m->rawque);
        if (u.count == 0 || u.count != want)
            break;                  /* full, or something to return */
        if (!p->sopen || p->hung) {
            if (m->rawque.count == 0 && s->outque.count == 0 &&
                s->nextc == 0)
                return;         /* end of file, and nothing left */
            continue;
        }
        if (m->rawque.count == 0 && s->outque.count == 0 && s->nextc == 0)
            sleep(&m->rawque, PRITTI);
    }
    ustart(s);
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
    register struct tty *s, *m;

    if ((i = ptyidx(dev)) < 0) {
        u.error = ENXIO;
        return;
    }
    p = &ptyinfo[i];
    s = SLAVE(i);
    m = MASTER(i);

    if (!ISMASTER(dev)) {
        if (s->mstate & TTYHUP)
            u.error = EIO;
        else
            ttywrite(s);
        return;
    }

    if (!p->sopen || p->hung) {
        u.error = EIO;
        return;
    }
    ttywrite(m);
}

/*
 * The character entries the kernel copies into ovlcvec[5] when this
 * module is placed (sys/ovl.c).  The major must exist in ciosw, which
 * is why sys/consts.c carries a row of ovl trampolines there.
 */
struct ciovec ptycvec = {
    &ptyopen, &ptyclose, &ptyread, &ptywrite, &ptyioctl
};

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
