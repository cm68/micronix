/*
 * one version of the multio driver
 *
 * sys/multio.c 
 * Changed: <>
 */
#include <types.h>
#include <sys/sys.h>
#include <sys/tty.h>
#include <sys/sgtty.h>
#include <sys/proc.h>
#include <sys/ovl.h>
#include <errno.h>
#include <sys/pic8259.h>
extern void mumcontrol();
extern void mumstatus();
extern void muenable();
extern void sleep();
extern void ttyopen();
extern void wakeup();
extern void ttyclose();
extern void ttywrite();
extern void ttymode();
extern void ttyhangup();

/*
 * The static routines below, declared before anything calls them.
 *
 * A call the compiler has not seen declared is extern int by default,
 * and the definition further down says static: two linkages for one
 * name.  The complaint lands on the definition rather than on the
 * call that caused it.
 */
extern void ttinit();

#define NMIO    TTY_SLOTS       /* struct tty slots, not minors (see tty.h) */
#define MBASE 0x48
#define VREADY (1 << 3)
#define HREADY (1 << 4)
#define PREADY (1 << 5)
#define CHECK  (1 << 6)
#define POWER  (1 << 7)

#define READY (POWER | PREADY | HREADY | VREADY)
#define STATUS (POWER | CHECK | PREADY | HREADY | VREADY)

#define REVERSE        (1 << 2)
#define HALF           (1 << 3)
#define VSTROBE        (1 << 4)
#define HSTROBE        (1 << 5)
#define PSTROBE        (1 << 6)
#define RIBLIFT        (1 << 7)

#define MOTION         (1 << 7)
#define VERTICAL       (1 << 6)
#define DIR            (1 << 5)

#define CENTRONICS     (1 << 4)

#define THRESHOLD      900
#define MAXRIGHT       1572

/*
 * asyncronous communications elements
 */

/*
 * relative offsets of control registers
 */

#define INTENABLE 1             /* interrupt enable register */
#define MCONTROL 4              /* modem control register */
#define MSTATUS  6              /* modem status register */
#define MUSELECT 7              /* multio group select register */

/*
 * bits in the interrupt enable register.
 */

#define INT_RECEIVE	(1 << 0)        /* recieved data avaiable */
#define INT_XMIT	(1 << 1)        /* transmitter buffer empty */
#define INT_MODEM	(1 << 3)        /* enable modem status change
                                         * interrupt */

/*
 * bits in the modem control register
 */

#define MU_DTR 1                /* data terminal ready */

/*
 * bits in the modem status register
 */

#define MU_DCTS		(1 << 0)        /* delta clear to send */
#define MU_DCD		(1 << 3)        /* delta carrier detect */
#define MU_CTS 		(1 << 4)        /* clear to send */
#define MU_CD 		(1 << 7)        /* carrier detect */

/*
 * bits in the multio group select register
 */

#define PENABLE 	050     /* parallel port enable */

/*
 * Hardware fiddlers in mio.s, and the SS1's 2651 in ss1_uart.c
 */
extern INT16 ace_start(), ace_stop(), ace_putc(), ace_set();
extern INT16 ss1_start(), ss1_stop(), ss1_set();
extern void ss1_putc();
extern void ss1_uart_open();
extern UINT8 ttypub();            /* publish the page's doors (sys/ttyres.c) */
extern void ttyconnect(), cstart();  /* shared helpers, sys/ttyres.c */
extern void ttyin(UINT8, struct tty *), ttyout(struct tty *);   /* the line discipline, same page (sys/tty.c) */
extern void outwait(struct tty *, INT16);  /* drain the output queue (sys/ttysys.c) */
extern void mumint();                   /* below, and an entry point the resident half calls */

/*
 * Tty structures for the four ports: the SS1's 2651 and the three
 * Wunderbus ACEs.  The minor numbers are not slot numbers (tty.h): the
 * console is minor 0 and aliases a board, the SS1 is minor 1, and the
 * ACEs are 4-6.  The slot a minor maps to is fixed, so the assembly
 * interrupt handlers can name their tty by constant offset.
 *
 *   minor  TTY_CONSOLE -> the board console_minor names
 *   minor  TTY_SS1     -> mttys[0]
 *   minor  TTY_ACE0    -> mttys[1]
 *   minor  TTY_ACE1    -> mttys[2]
 *   minor  TTY_ACE2    -> mttys[3]
 */
extern struct tty mttys[];

/*
 * Select a tty structure from a device number.  A NULL return means the
 * minor names no port.  The console is resolved through console_minor
 * every time, so /dev/console follows the routing without a second
 * structure and without a stale dev in the opened file.
 */
struct tty *
ttdev(dev)
    INT16 dev;
{
    register INT16 m;

    m = dev & 15;
    if (m == TTY_CONSOLE)
        m = console_minor;
    switch (m) {
    case TTY_SS1:
        return (&mttys[0]);
    case TTY_ACE0:
        return (&mttys[1]);
    case TTY_ACE1:
        return (&mttys[2]);
    case TTY_ACE2:
        return (&mttys[3]);
    }
    return (0);
}

/*
 * Open a MULT I/O tty.
 * Initialize tty structure and hardware.
 */
void
muopen(dev, mode)
    register INT16 dev;
    INT16 mode;
{
    struct tty *tty;
    register INT16 minor, port;

    /*
     * Minor 0 is the console alias; resolve it once to the board the
     * boot routed the console to.  What the tty records (and what
     * selects the ACE group) is the board's own minor, not the alias.
     * The DIALER bit rides above the port number and is kept.
     */
    minor = dev & 0377;
    port = minor & 15;
    if (port == TTY_CONSOLE)
        port = console_minor;
    dev = (dev & 0xff00) | (minor & ~15) | port;

    tty = ttdev(dev);
    if (tty == 0) {
        u.error = ENXIO;        /* No such dev. */
        return;
    }

    /*
     * The SS1's 2651 is not a banked ACE: it has fixed ports and its own
     * open.  Minor 1 is that board even when the console was routed
     * elsewhere.
     */
    if (port == TTY_SS1) {
        ss1_uart_open(dev, 0);
        return;
    }

    if (tty->ispeed == 0) {     /* virgin */
        ttinit(tty, dev);
        (*tty->set)(tty);               /* set baud rate */
    }

    /*
     * a channel may be opened for dial-out only if it is presently unused.
     */

    if (dev & DIALER) {
        /*
         * what about a draining out que ?
         */
        if (tty->state & OPEN) {        /* exclusion */
            u.error = EBUSY;
            return;
        }
    }

    /*
     * assert DATA TERMINAL READY
     */

    tty->mstate |= DTR;
    mumcontrol(dev);

    /*
     * wait for CARRIER DETECT (if appropriate)
     */

    mumstatus(dev);             /* internalize modem status lines */

    if (dev & DIALER) {
        tty->mstate |= DIALER;
    }

    else {
        while (!(tty->mstate & CD)) {
            di();
            tty->mstate |= WOPEN;
            muenable(dev, INT_MODEM);
            sleep(&tty->mstate, PRITTI);
        }

        tty->mstate &= ~WOPEN;  /* no longer waiting for open */
    }

    ttyopen(tty);

    /*
     * open for business
     */

    di();

    if (!(tty->state & OPEN) && !tty->outque.count && !tty->nextc) {
        (*tty->set)(tty);               /* set baud rate */
        (*tty->stop)(tty, 0);           /* enable input, disable output */
    }
    ei();

}

/*
 * Close
 */
void
muclose(dev)
    INT16 dev;
{
    register struct tty *t;

    t = ttdev(dev);

    if (dev & DIALER) {
        t->mstate &= ~DIALER;

        if ((t->mstate & (WOPEN | CD)) == (WOPEN | CD)) {
            wakeup(&t->mstate);
        }
    }

    /*
     * Let the transmitter finish before the tty goes.  The output queue is
     * drained by the transmit-ready interrupt, and a close that returned
     * with bytes still in it left them to be sent while the caller went
     * on.  At shutdown that is exactly what happened: init's "Micronix is
     * down." was still shifting out when reboot() printed the halt line
     * straight to the console, and the two writers interleaved - and once
     * raced, losing a character.  A stopped line is not waited for: its
     * output will never drain until it is started again, and a close must
     * not hang on one.
     */
    if (!(t->state & LOSTOP))
        outwait(t, 0);
    ttyclose(t);
}

/*
 * Read a multiboard tty
 */
void
muread(dev)
    INT16 dev;
{
    ttyread(ttdev(dev));
}

/*
 * Write a multiboard tty
 */
void
muwrite(dev)
    INT16 dev;
{
    ttywrite(ttdev(dev));
}

/*
 * A request to a multiboard tty.  Getting and setting its status are
 * the two the library's stty(3) and gtty(3) send.
 */
void
muioctl(dev, cmd, arg)
    INT16 dev, cmd;
    UINT8 *arg;
{
    if (cmd == TIOCGETA)
        ttymode(ttdev(dev), READ);
    else if (cmd == TIOCSETA)
        ttymode(ttdev(dev), WRITE);
    else
        u.error = ENOTTY;
}

/*
 * Initialize a tty structure
 * This could be done by copying in a prototype structure.
 */
void
ttinit(tty, dev)
    register struct tty *tty;
    INT16 dev;
{
    tty->ispeed = tty->ospeed = 13;     /* 9600 baud */
    tty->erase = '\b';
    tty->kill = 'X' - 'A' + 1;

    tty->mode = ECHO | MAPCR | XTABS;

    tty->col = 0;
    tty->dev = dev;

    /*
     * The back end belongs to the port, not to the machine.  The console
     * globals name the console's board; a tty opened on the other board
     * has to reach its own ports, and ttys are no longer one board's.
     */
    if ((dev & 15) == TTY_SS1) {
        tty->start = ss1_start;
        tty->stop = ss1_stop;
        tty->put = ss1_putc;
        tty->set = ss1_set;
    } else {
        tty->start = ace_start;
        tty->stop = ace_stop;
        tty->put = ace_putc;
        tty->set = ace_set;
    }
}

void
muselect(t)
    struct tty *t;
{
    /*
     * The ACEs are minors 4-6 and their port groups are 1-3, so the
     * group is the minor minus TTY_ACE0-1, not the minor itself.
     */
    out(MBASE + MUSELECT, ((t->dev & 15) - (TTY_ACE0 - 1)) | PENABLE);
}

/*
 * mumint - multio modem status change interrupt service routine
 */

void
mumint(t)
    struct tty *t;
{
    UINT8 s;

    /*
     * read the modem status register
     */

    di();

    muselect(t);
    s = in(MBASE + MSTATUS);

    ei();

    if (s & MU_DCTS) {          /* Clear to send changed. */
        if (s & MU_CTS) {       /* Clear to send. */
            cstart(t);
        }
    }

    if (s & MU_DCD) {           /* carrier indication changed */
        if (s & MU_CD) {
            ttyconnect(t);      /* carrier appeared */
        } else {
            ttyhangup(t);       /* carrier lost */
        }
    }
}

/*
 * set or clear DATA TERMINAL READY on the multio
 * Assert modem control lines as per t->mstate
 * (master only)
 */

/*
 * Update the modem control register in the ACE
 * from information found in the tty structure.
 * Currently, the only line we play with here is DTR.
 */

void
mumcontrol(dev)
    INT16 dev;
{
    UINT8 a;
    struct tty *t;

    t = ttdev(dev);

    di();

    muselect(t);
    a = in(MBASE + MCONTROL);

    if (t->mstate & DTR) {
        a |= MU_DTR;
    } else {
        a &= ~MU_DTR;
    }

    out(MBASE + MCONTROL, a);

    ei();
}

/*
 * Sense the modem status lines
 * Make internal notations as to their state.
 * (master only)
 * This would need to be modified to work
 * on other than the first mult I/O board.
 */

void
mumstatus(dev)
    INT16 dev;
{
    UINT8 s;
    struct tty *t;

    t = ttdev(dev);

    if (dev) {                  /* ACE */
        di();

        muselect(t);

        s = in(MBASE + MSTATUS);        /* read status */

        ei();
    }

    else {                      /* centronics parallel port */
        s = MU_CD;              /* simulate carrier */
    }

    /*
     * make an internal note of the hardware status
     */

    if (s & MU_CD) {
        t->mstate |= CD;
    } else {
        t->mstate &= ~CD;
    }
}

#define MBASE (0x48)
#define PIC1 (MBASE + 5)

/*
 * muenable -
 * set the multio interrupt lines according to mode
 * Lines we play with are transmit, receive, and modem status change.
 * Write a value to the interrupt enable register.
 */

void
muenable(dev, mode)
    INT16 dev, mode;
{
    struct tty *t;

    t = ttdev(dev);

    di();
    out(MBASE + MUSELECT, ((t->dev & 15) - (TTY_ACE0 - 1)) | PENABLE);
    out(MBASE + INTENABLE, mode);
    ei();
}

/*
 * The character entries the kernel copies into ovlcvec[1] when this
 * module is placed (sys/ovl.c).  They are the resident interrupt stubs'
 * counterparts: a process reaches them through the ovlc trampolines,
 * the stubs through ttyin/ttyout/mumint in sys/ttyres.c.
 */
struct ciovec ttycvec = {
    &muopen, &muclose, &muread, &muwrite, &muioctl
};

/*
 * The module's init, run once by ovlplace before the page is reachable.
 * It tells the resident half where the page is and which of its
 * addresses the interrupt stubs should call; the state it points at
 * (mttys, sys/ttyres.c) is resident and needs nothing from this page.
 */
INT16
ttyinit(seg)
    UINT16 seg;
{
    ttypub(seg, ttyin, ttyout, mumint);
    return (0);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
