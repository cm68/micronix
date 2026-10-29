/*
 * tty driver - the line discipline
 *
 * sys/tty.c
 *
 * This file is linked into two pages, one copy in each: tty.mod (the
 * console and its UART boards) and pty.mod (the pseudo-terminals).  A
 * module cannot name another module, so a pty that called the console's
 * copy would be reaching out of its own page; the copy here keeps every
 * pty-to-tty call local.  What the two copies must not duplicate - the
 * process-table walks, the start/stop helpers and the cooking buffer -
 * is resident in sys/ttyres.c and called from both.
 */
#include <types.h>
#include <sys/sys.h>
#include <sys/tty.h>
#include <sys/proc.h>
#include <errno.h>
#include <sys/signal.h>
#include <sys/pic8259.h>

/*
 * The parts of the line discipline that both copies share - the process
 * table walks and the start/stop helpers - are resident (sys/ttyres.c),
 * as is the cooking scratch buffer.  This file is linked into both
 * tty.mod and pty.mod, so anything that can be written once is in the
 * resident half instead of twice in a page.
 */
extern int killfg(), ttynotify(), cstart(), ustart();
extern char ttycbuf[];
extern int selectchan;          /* select(2): a reader is watching */

/*
 * Filter the raw input que into the cooked input que
 * We can't afford to sleep here while using the buffer
 */
cookin(tty)
    register struct tty *tty;
{
    register struct que *cook, *raw;
    register c;
    register char *p, *q;

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

    for (p = ttycbuf;;) {
        if (p >= ttycbuf + TTYHOG - 2)
            p = ttycbuf;            /* overflow */

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
            p = ttycbuf;
        }

        else if (c == tty->erase) {
            if (p != ttycbuf)
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

    for (q = ttycbuf; q < p; q++)
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

    /*
     * A selector watching this tty has to be woken whether the record
     * is complete or not: the pty master is RAW and wants every byte,
     * and a cooked reader re-checks its line on the next pass.
     */
    wakeup(&selectchan);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
