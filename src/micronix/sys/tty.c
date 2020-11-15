/*
 * otty.c 
 */
#include <types.h>
#include <sys/sys.h>
#include <sys/tty.h>
#include <sys/proc.h>

/*
 * Open a tty. Called by the device open routine.
 * Currently, this simply sets the process'
 * controlling tty.
 */
ttyopen(tty)
    struct tty *tty;
{
    if (u.p->tty == NULL)
        u.p->tty = tty;

    tty->count++;
    tty->state |= OPEN;
}

/*
 * Close a tty. Called by device close routine.
 */
ttyclose(tty)
    fast struct tty *tty;
{
    if (!--tty->count) {
        tty->state &= ~OPEN;
    }
}

/*
 * High level output. Write characters to the terminal.
 */
ttywrite(tty)
    fast struct tty *tty;
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
    fast struct tty *tty;
{
    fast struct que *cok;

    if (tty->state & ERROR) {
        tty->state &= ~ERROR;   /* clear the error condition */
        drain(&tty->rawque);    /* discard input */
        u.error = EIO;          /* cause the read to fail */
        return;
    }

    /*
     *      if (u.p->mode & BACK)
     *              return;         /* eof for read in background *
     */

    cok = &tty->cokque;

    if (cok->count == 0) {
        cookin(tty);
    }

    sendque(cok);

    if (cok->count != 0) {
        killall(tty, SIGTINT);  /* term input available */
    }
}

/*
 * Filter the raw input que into the cooked input que
 * We can't afford to sleep here while using the buffer
 */
cookin(tty)
    fast struct tty *tty;
{
    fast struct que *cook, *raw;
    fast c;
    static char buf[TTYHOG], *p, *q;

    cook = &tty->cokque;
    raw = &tty->rawque;

    if (tty->mode & (RAW | CBREAK)) {
        c = cwait(tty);         /* at least one char */

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
    fast struct que *raw;

    raw = &tty->rawque;

    while (di(), raw->count == 0) {
        tty->state |= HISLEEP;
        sleep(raw, PRIUSER);
    }

    ei();
    return (getc(raw));
}

/*
 * Get/set tty status (for an sgtty routine)
 */
ttymode(tty, flag)
    fast struct tty *tty;
    int flag;
{

    /*
     * outwait(tty, 0); 
     */
    iomove(flag, tty, 6);
    if (flag == WRITE)
        (*tty->set) (tty);
}

/*
 * Low level output, called from the terminal's
 * ready-to-print interrupt.
 */
ttyout(tty)
    fast struct tty *tty;
{

    /*
     * fast struct que *que; 
     */
    fast char *state;
    char cold;

    state = &tty->state;

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
            mputc(tty, cookout(tty));
        } else {
            /*
             * Shortcut for the easy case. 
             */
            mputc(tty, getc(&tty->outque));
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
    fast struct tty *tty;
{
    struct que *que;
    fast int mode;
    fast char c;
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
 * Low level input, called from the keyboard interrupt.
 * Put the character into the raw input queue, and echo
 * it if necessary.
 */
ttyin(c, tty)
    fast int c;
    fast struct tty *tty;
{
    struct que *raw;
    fast struct que *out;
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

        case QUIT:
            killall(tty, SIGQUIT);
            ustart(tty);
            return;

        case RUB:
            killall(tty, SIGINT);
            ustart(tty);
            return;

            /*
             * case STOP: if (state & LOSTOP) break; 
             */
        case XOFF:
            tty->state |= LOSTOP;
            (*tty->stop) (tty, NO);
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
        killall(tty, SIGTINT);
        tty->line = 0;
    }
}

/*
 * Conditionally start output
 */
cstart(tty)
    fast struct tty *tty;
{
    if (!(tty->state & LOSTOP))
        (*tty->start) (tty);
}

/*
 * Unconditionally start output
 */
ustart(tty)
    fast struct tty *tty;
{
    tty->state &= ~LOSTOP;
    (*tty->start) (tty);
}

/*
 * Wait for output queue to drain.
 * Called by ttyclose, ttymode, and ttywrite().
 */
outwait(tty, count)
    fast struct tty *tty;
    fast int count;
{
    fast struct que *out;

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
