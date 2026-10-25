/*
 * the part of the pty driver that lives in the u page
 *
 * sys/pty_leaf.c
 *
 * These are the helpers that hold no storage and are called from more
 * than one place: the pair/side decode, the four back-end calls the tty
 * layer makes, and the pair's own init and hangup.  The u page is where
 * stateless code that every process may run lives (LEAF_C, see
 * sys/GNUmakefile), and it has room where the resident text does not.
 *
 * The state - struct ptyinfo and the two struct tty of each pair -
 * stays in the resident kernel (sys/pty.c).  The u page is cloned per
 * process, so state in it would be a copy per process, and the queue
 * sleepers and p->tty pointer have to see one object.
 */
#include <types.h>
#include <sys/sys.h>
#include <sys/tty.h>
#include <sys/proc.h>
#include <sys/signal.h>
#include <sys/pty.h>
#include <errno.h>

extern int ttyout(), ttyin(), ttyread(), ttywrite(), ttymode(),
    ttyopen(), ttyclose();
extern int qinit(), drainques(), cstart();

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
 */
int
pty_start(t)
    register struct tty *t;
{
    register struct tty *peer;

    peer = PEER(t);
    while (!(t->state & LOSTOP) && (t->outque.count || t->nextc) &&
        peer->rawque.count < HIWATER)
        ttyout(t);
}

int
pty_stop(t, cold)
    struct tty *t;
    int cold;
{
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
ptymode(dev, flag)
    int dev;
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
    ttymode(SLAVE(i), flag);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
