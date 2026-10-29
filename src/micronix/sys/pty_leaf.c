/*
 * the part of the pty driver that is linked into its own page
 *
 * sys/pty_leaf.c
 *
 * These are the helpers that hold no storage and are called from more
 * than one place: the pair/side decode and the four back-end calls the
 * tty layer makes.  pty.mod links this object beside the driver and its
 * own copy of the line discipline (sys/GNUmakefile), so the tty layer's
 * calls to start/stop/put/set and the back ends' calls to
 * ttyin/ttyout are all calls within one page rather than returns into
 * the kernel.
 *
 * The state - struct ptyinfo and the two struct tty of each pair -
 * stays in the resident kernel (sys/pty.c).  The driver window is one
 * page for the whole system, mapped only while its driver is running,
 * so state there would vanish between calls, and the queue sleepers and
 * p->tty pointer have to see one object that is always there.
 */
#include <types.h>
#include <sys/sys.h>
#include <sys/tty.h>
#include <sys/proc.h>
#include <sys/signal.h>
#include <sys/pty.h>
#include <sys/sgtty.h>
#include <errno.h>

extern int ttyout(), ttyin(), ttymode();

/*
 * Pair and side of a device number; -1 for a minor that is not one of
 * ours.  The caller has already bounded the major through ciosw.
 */
int
ptyidx(dev)
    int dev;
{
    register int m;

    m = dev & 0377;
    if (m < PTYMST + NPTY)
        return m - PTYMST;
    if (m >= PTYSLV && m < PTYSLV + NPTY)
        return m - PTYSLV;
    return -1;
}

/*
 * The slave has output to give the master.  ttyout() takes one cooked
 * character and calls put(); drain until the peer's input queue is
 * nearly full, which is what makes a writer wait instead of running the
 * pool dry.
 *
 * LOSTOP is not tested here: every caller of t->start is cstart() or
 * ustart(), and cstart() tests it and ustart() clears it (sys/ttyres.c).
 * t->start is reached no other way.
 */
int
pty_start(t)
    register struct tty *t;
{
    register struct tty *peer;

    peer = PEER(t);
    while ((t->outque.count || t->nextc) && peer->rawque.count < HIWATER)
        ttyout(t);
}

/*
 * One cooked byte from this end becomes one for the other end to read,
 * through the same ttyin() a board port's interrupt calls.
 */
int
pty_put(t, c)
    struct tty *t;
    char c;
{
    ttyin(c, PEER(t));
}

/*
 * sgtty on the slave is the terminal's; on the master there is no
 * terminal, so let stty say so.
 */
int
ptyioctl(dev, cmd, arg)
    int dev, cmd;
    char *arg;
{
    register int i;

    if ((i = ptyidx(dev)) < 0) {
        u.error = ENXIO;
        return;
    }
    if (ISMASTER(dev)) {
        u.error = ENOTTY;
        return;
    }
    if (cmd == TIOCGETA)
        ttymode(SLAVE(i), READ);
    else if (cmd == TIOCSETA)
        ttymode(SLAVE(i), WRITE);
    else
        u.error = ENOTTY;
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
