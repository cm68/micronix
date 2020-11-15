/*
 * mset.c 
 */
#include <types.h>
#include <sys/tty.h>

#define lowbyte(a) (a)
#define highbyte(a) ((a) >> 8)

/*
 * mset - Set the baud rate on serial ports
 *      Set the parity.
 *      Make all settings for the ace
 *      to reflect the internal state.
 */

mset(tty)                       /* called during stty system call and at open 
                                 */
    struct tty *tty;
{
    char lc;
    int baud;

    if ((tty->dev & 3) == 0)
        return;

    di();

    muselect(tty);

    baud = rates[tty->ispeed & 15];

    /*
     * Read the line control status.
     */

    r = in(MBASE + LCONTROL);

    /*
     * Set the baud rate.
     */

    if (baud) {
        r |= BAUDREG;
        out(MBASE + LCONTROL, r);
        out(MBASE + LOBAUD, lowbyte(baud));
        out(MBASE + HIBAUD, highbyte(baud));
    }

    r = MU_WORD8 | MU_STOP2;    /* 8 data bits, 2 stop bits */

    /*
     * parity
     */

    if (tty->mode & (ODD | EVEN)) {
        r |= MU_PARITY;
        if (tty->mode & EVEN) {
            r |= MU_EVEN;
        }
    }

    out(MBASE + LCONTROL, r);

    r = 0;

    if (tty->mstate & DTR)
        r |= MU_DTR | MU_RTS;

    out(MBASE + MCONTROL, r);

    ei();
}

/*
 * The constants needed by the ace divider circuits to achieve the 
 */

/*
 * stty rate requests 0 to 15. 
 */

int rates[] = {
    0,                          /* 0 = HANGUP */
    2304,                       /* 1 = 50 */
    1536,                       /* 2 = 75 */
    1047,                       /* 3 = 110 */
    857,                        /* 4 = 134.5 */
    768,                        /* 5 = 150 */
    576,                        /* 6 = 200 */
    384,                        /* 7 = 300 */
    192,                        /* 8 = 600 */
    96,                         /* 9 = 1200 */
    64,                         /* 10 = 1800 */
    48,                         /* 11 = 2400 */
    24,                         /* 12 = 4800 */
    12,                         /* 13 = 9600 */
    6,                          /* 14 = 19200 */
    3,                          /* 15 = 38400 */
};

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
