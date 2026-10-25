/*
 * tty driver
 *
 * sys/tty.c 
 * Changed: <>
 */
#include <types.h>
#include <sys/sys.h>
#include <sys/tty.h>
#include <sys/proc.h>
#include <errno.h>
#include <sys/signal.h>
#include <sys/pic8259.h>

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
 * Filter the raw input que into the cooked input que
 * We can't afford to sleep here while using the buffer
 */
cookin(tty)
    register struct tty *tty;
{
    register struct que *cook, *raw;
    register c;
    static char buf[TTYHOG], *p, *q;

    cook = &tty->cokque;
    raw = &tty->rawque;

    if (tty->mode & (RAW | CBREAK)) {
        c = cwait(tty);         /* at least one char */
        if (c < 0)
            return;             /* input closed while we slept */

        do {
            di();

            if (tty->nbreak && (c == '\n' || c == EOT))
                tty->nbreak--;
            ei();

            putc(cook, c);
        }
        while ((c = getc(raw)) != -1);

        /*
         * goto turnon; 
         */
        return;
    }

    /*
     * Wait until at least one break char. is in raw queue
     */
  tryagain:

    for (;;) {
        di();
        if (tty->nbreak)
            break;
        if (jctl[u.p - plist] & JOBTTYOUT) {
            ei();
            return;             /* input closed while we slept */
        }
        if (tty->mstate & TTYHUP) {
            ei();
            return;             /* hung up while we slept */
        }
        tty->state |= HISLEEP;
        sleep(raw, PRIUSER);
    }

    ei();

    for (p = buf;;) {
        if (p >= buf + TTYHOG - 2)
            p = buf;            /* overflow */

        di();
        c = getc(raw);

        if (c < 0) {            /* we're out of sync */
            tty->nbreak = 0;
            ei();
            goto tryagain;
        }

        ei();

        if (c == EOT) {
            break;
        }

        else if (c == '\n') {
            *p++ = c;
            break;
        }

        else if (c == tty->kill) {
            p = buf;
        }

        else if (c == tty->erase) {
            if (p != buf)
                p--;
        }

        else if (c == BSLASH) {
            c = getc(raw);

            if (c == tty->erase || c == tty->kill) {
                *p++ = c;
            } else {
                *p++ = BSLASH;
                *p++ = c;
            }

            if (c == '\n' || c == EOT) {
                break;
            }
        }

        else {
            *p++ = c;
        }
    }

    di();
    if (tty->nbreak)
        tty->nbreak--;
    ei();

    for (q = buf; q < p; q++)
        putc(cook, *q);

    /*
     * turnon: if (tty->state & INSTOP) { tty->state &= ~INSTOP; tty->state
     * |= STARTIN; (*tty->start)(tty); } 
     */
}

/*
 * Get a character from the tty's raw queue,
 * sleeping if necessary.
 * See ttyin for the wakeup.
 */
cwait(tty)
    struct tty *tty;
{
    register struct que *raw;

    raw = &tty->rawque;

    while (di(), raw->count == 0) {
        if (jctl[u.p - plist] & JOBTTYOUT) {
            ei();
            return (-1);        /* input closed while we slept */
        }
        if (tty->mstate & TTYHUP) {
            ei();
            return (-1);        /* hung up while we slept */
        }
        tty->state |= HISLEEP;
        sleep(raw, PRIUSER);
    }

    ei();
    return (getc(raw));
}

/*
 * Low level output, called from the terminal's
 * ready-to-print interrupt.
 */
ttyout(tty)
    register struct tty *tty;
{

    /*
     * fast struct que *que; 
     */
    register char *state;
    char cold;

    /*
     * state is a UINT8 in struct tty and this walks it as a char.
     * Same object either way on this machine - the bits below are
     * tested and set, never compared for order - so the cast is safe
     * and ccc wants it said.
     */
    state = (char *) &tty->state;

    /*
     * if (*state & STOPIN) { *state &= ~STOPIN; (*tty->put)(tty, XOFF);
     * *state |= INSTOP; return; } 
     */

    /*
     * if (*state & STARTIN) { *state &= ~STARTIN; (*tty->put)(tty, XON);
     * return; } 
     */

    /*
     * que = &tty->outque; 
     */

    if (!(*state & LOSTOP) && (tty->outque.count || tty->nextc)) {
        if ((tty->mode & (MORE | XTABS | MAPCR)) || tty->nextc) {
            (*tty->put)(tty, cookout(tty));
        } else {
            /*
             * Shortcut for the easy case.
             */
            (*tty->put)(tty, getc(&tty->outque));
        }

        /*
         * (*tty->put)(tty, cookout(tty)); 
         */
    }

    else {
        cold = !(*state & OPEN) && !tty->outque.count && !tty->nextc;

        (*tty->stop) (tty, cold);

        if (cold) {
            drainques(tty);
            *state = 0;
        }
    }

    if ((*state & HOSLEEP) && tty->outque.count <= LOWATER) {
        *state &= ~HOSLEEP;
        wakeup(&tty->outque);
    }
}

/*
 * Get the next character for output.
 * Expand the output que if necessary.
 */
cookout(tty)
    register struct tty *tty;
{
    struct que *que;
    register int mode;
    register char c;
    int inc;
    static char *more = "--more--\b\b\b\b\b\b\b\b        \b\b\b\b\b\b\b\b";

  loop:
    que = &tty->outque;
    mode = tty->mode;

    if (mode & MORE && tty->line > 21) {
        tty->line = 0;
        tty->nextc = 32;
    }

    if (tty->nextc) {
        c = tty->nextc;
        if (c == '\n') {
            tty->nextc = 0;
            tty->line++;
            return ('\n');
        } else if (c < 8) {
            tty->nextc--;
            return (' ');
        } else {
            c -= 32;
            if (c == 7)
                tty->state |= LOSTOP;
            if (c < 31)
                tty->nextc++;
            else
                tty->nextc = 0;
            return (more[c]);
        }
    }

    c = getc(que);

    if (' ' <= c && c <= 0177) {
        tty->col++;
        return (c);
    }

    switch (c) {

    case '\n':
        if (mode & MAPCR) {
            tty->nextc = '\n';
            tty->col = 0;
            return ('\r');
        } else {
            tty->line++;
            return ('\n');
        }

    case '\t':
        inc = 8 - (tty->col & 7);
        tty->col += inc;
        if (mode & XTABS) {
            tty->nextc = inc - 1;
            return (' ');
        } else
            return ('\t');

    case '\b':
        if (tty->col != 0)
            tty->col--;
        return ('\b');

    default:
        return (c);
    }
}

/*
 * Called on wrong baud rate detection.
 */

ttyerror(t)
    struct tty *t;
{
    t->state |= ERROR;
}

/*
 * called on carrier present detection
 * Don't disturb the sleepers waiting for carrier
 * if the tty is being used as a dialer.
 * We dial for ourselves.
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
 * called on carrier loss
 */

ttyhangup(t)
    struct tty *t;
{
    t->mstate &= ~CD;

    if (!(t->mstate & DIALER)) {
        drainques(t);
        killall(t, SIGKILL);
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
 * Low level input, called from the keyboard interrupt.
 * Put the character into the raw input queue, and echo
 * it if necessary.
 */
ttyin(c, tty)
    register UINT8 c;
    register struct tty *tty;
{
    struct que *raw;
    register struct que *out;
    char state;
    int mode;

    raw = &tty->rawque;
    out = &tty->outque;

    mode = tty->mode;
    state = tty->state;

    if (raw->count >= TTYHOG) { /* input queue overflow */
        di();
        drain(raw);
        tty->nbreak = 0;
        ei();
        return;
    }

    /*
     * else if (raw->count >= HIWATER)     /* Tandem mode input stop *
     * {
     * tty->state |= STOPIN;
     * (*tty->start)(tty);
     * }
     */

    if ((mode & ALL8) == 0)
        c &= 0177;              /* strip parity */

    if (c == '\r' && (mode & MAPCR))
        c = '\n';

    if ((mode & RAW) == 0) {    /* cooked or cbreak mode */
        switch (c) {

            /*
             * case BACKGRND:
             * killall(tty, SIGBACK);
             * ustart(tty);
             * return;
             */

        case SUSP:
            ttynotify(tty);
            ustart(tty);
            return;

        case QUIT:
            killfg(tty, SIGQUIT);
            ustart(tty);
            return;

        case RUB:
            killfg(tty, SIGINT);
            ustart(tty);
            return;

            /*
             * case STOP: if (state & LOSTOP) break; 
             */
        case XOFF:
            tty->state |= LOSTOP;
            (*tty->stop) (tty, 0);
            return;

        case XON:
            if (state & LOSTOP) {
                ustart(tty);
            }

            return;

        }
    }

    /*
     * if (state & LOSTOP)
     * {
     * ustart(tty);
     * return;
     * }
     */

    /*
     * Deposit the character in the input queue.
     */

    di();
    if (c == EOT || c == '\n')
        tty->nbreak++;
    putc(raw, c);
    ei();

    if (mode & ECHO) {
        if (mode & RAW) {
            putc(out, c);
        }

        else if (c == tty->kill) {
            /*
             * putc(out, c); 
             */
            putc(out, '\n');
        }

        else if (c == tty->erase) {
            putc(out, '\b');
            putc(out, ' ');
            putc(out, '\b');
        }

        else {
            putc(out, c);
        }

        cstart(tty);
    }

    if (mode & RAW || mode & CBREAK || c == '\n' || c == EOT) {
        if (state & HISLEEP) {
            tty->state &= ~HISLEEP;
            wakeup(raw);
        }
        killfg(tty, SIGTINT);
        tty->line = 0;
    }
}

/*
 * Conditionally start output
 */
cstart(tty)
    register struct tty *tty;
{
    if (!(tty->state & LOSTOP))
        (*tty->start) (tty);
}

/*
 * Unconditionally start output
 */
ustart(tty)
    register struct tty *tty;
{
    tty->state &= ~LOSTOP;
    (*tty->start) (tty);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
