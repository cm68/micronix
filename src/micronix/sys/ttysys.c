/*
 * tty syscall half
 *
 * sys/ttysys.c
 *
 * The parts of the tty driver a process reaches through a system call -
 * open, close, read, write, sgtty and the output drain that waits on a
 * writer - as opposed to the interrupt half (ttyin/ttyout/cstart and the
 * line discipline) in sys/tty.c.  The two are linked together into each
 * tty page: tty.mod (the console and its boards) and pty.mod (its own
 * copy, so a pty never calls into another module).  Both copies are page
 * images, so what is shared between them - the helpers and state in
 * sys/ttyres.c - is resident instead.
 *
 * It sleeps: ttyread waits for a line in cookin()/cwait(), and
 * ttywrite's outwait() waits for the transmitter.  A tty page can hold
 * that - the driver page is mapped for the whole call, sleeping included,
 * and ovlremap() puts it back at a switch because u.drv names it.
 */
#include <types.h>
#include <sys/sys.h>
#include <sys/tty.h>
#include <sys/proc.h>
#include <errno.h>
#include <sys/signal.h>

extern int fillque(), cstart(), cookin(), sendque(), killfg(), drain(),
    iomove(), sleep(), wakeup();
extern int selectchan;          /* select(2)'s event, sys/select.c */

/*
 * Open a tty. Called by the device open routine.
 * Currently, this simply sets the process'
 * controlling tty.
 */
ttyopen(tty)
    struct tty *tty;
{
    if (u.p->tty == 0)
        u.p->tty = tty;

    tty->count++;
    tty->state |= OPEN;
}

/*
 * Close a tty. Called by device close routine.
 */
ttyclose(tty)
    register struct tty *tty;
{
    /*
     * count is a UINT8.  The old "if (!--count)" decremented before it
     * tested, so a close that arrived with the count already at zero
     * wrapped it to 255, and OPEN stayed set forever.  Decrement only a
     * positive count, and drop OPEN when the last open goes away.
     */
    if (tty->count != 0 && --tty->count == 0)
        tty->state &= ~OPEN;
}

/*
 * High level output. Write characters to the terminal.
 */
ttywrite(tty)
    register struct tty *tty;
{
    for (;;) {
        fillque(&tty->outque);
        cstart(tty);

        if (u.count == 0)
            break;

        outwait(tty, LOWATER);
    }
}

/*
 * High level input.
 * Read characters from the terminal
 */
ttyread(tty)
    register struct tty *tty;
{
    register struct que *cok;

    if (tty->state & ERROR) {
        tty->state &= ~ERROR;   /* clear the error condition */
        drain(&tty->rawque);    /* discard input */
        u.error = EIO;          /* cause the read to fail */
        return;
    }

    if (jctl[u.p - plist] & JOBTTYOUT)
        return;                 /* eof for read in background */

    if (tty->mstate & TTYHUP)
        return;                 /* the line is gone */

    cok = &tty->cokque;

    if (cok->count == 0) {
        cookin(tty);
    }

    /*
     * The shell may have closed this job's input while cookin() was
     * asleep.  The line it took is in the cooked queue and belongs to
     * the foreground, so return eof and leave it there.
     */
    if (jctl[u.p - plist] & JOBTTYOUT)
        return;

    if (tty->mstate & TTYHUP)
        return;                 /* hung up while we slept */

    sendque(cok);

    if (cok->count != 0) {
        killfg(tty, SIGTINT);   /* term input available */
    }
    wakeup(&selectchan);        /* select(2): cooked input is here */
}

/*
 * Get/set tty status (for an sgtty routine)
 */
ttymode(tty, flag)
    register struct tty *tty;
    UINT8 flag;
{

    /*
     * outwait(tty, 0);
     */
    iomove(flag, tty, 6);
    if (flag == WRITE)
        (*tty->set) (tty);
}

/*
 * Wait for output queue to drain.
 * Called by ttyclose, ttymode, and ttywrite().
 */
outwait(tty, count)
    register struct tty *tty;
    register int count;
{
    register struct que *out;

    out = &tty->outque;

    while (di(), out->count > count) {
        tty->state |= HOSLEEP;
        sleep(out, PRIUSER);    /* wakeup in ttyout */
    }

    ei();
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
