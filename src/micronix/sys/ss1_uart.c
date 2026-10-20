/*
 * The CompuPro SS1's 2651 console.
 *
 * sys/ss1_uart.c
 *
 * The console half of the SS1 board: one 2651 USART at the board's own
 * jumper block (0x70), data at 0x7c, status at 0x7d, mode at 0x7e and
 * command at 0x7f.  TxRDY is the slave's IR6, RxRDY IR7 - the wiring is
 * the PIC's, so nothing here touches a device line.  The 2651 has no
 * modem and no programmable baud rate the way the MultIO's ACE does.
 *
 * The transmitter stays enabled at all times.  The kernel's pr() goes
 * through the monitor's polled console driver (condrv, sys/mio.s), which
 * sends through this same 2651 and waits for TxRDY; dropping TxEN stalls
 * it and the machine goes silent.  The tty queue is paced by masking and
 * unmasking the slave's IR6 instead, so only the interrupt is gated.
 */
#include <types.h>
#include <sys/sys.h>
#include <sys/tty.h>
#include <sys/pic8259.h>

extern struct tty mttys[];      /* the console tty (sys/multio.c) */
extern int ttinit(), ttyopen(), muopen();

#define SS1_UART_DATA   0x7c
#define SS1_UART_STAT   0x7d
#define SS1_UART_MODE   0x7e
#define SS1_UART_CMD    0x7f

#define TXRDY   0x01
#define RXRDY   0x02
#define TXEN    0x01
#define RXEN    0x04

#define SS1_SPIC1 0x73          /* the slave 8259's IMR (SS1 base + 3) */
#define IR6     0x40            /* the slave's TxRDY request */

extern int in(), out();

/*
 * Gate the TxRDY interrupt.  Masking the line, not the transmitter, is
 * what stops the request: the chip must keep sending for the monitor.
 */
static
ss1_txline(on)
    int on;
{
    UINT8 m;

    m = in(SS1_SPIC1);
    if (on)
        m &= ~IR6;
    else
        m |= IR6;
    out(SS1_SPIC1, m);
}

/*
 * Send a character.  The transmitter is the only buffering, and the
 * caller (the tty output path) already holds interrupts off, so the
 * write is the whole of it.
 */
ss1_putc(tty, c)
    struct tty *tty;
    UINT8 c;
{
    out(SS1_UART_DATA, c);
}

/*
 * Start the output.  The transmitter is already enabled, so TxRDY is
 * high; opening IR6 lets the PIC deliver it, and the handler (sys/mio.s)
 * sends one queued character each time, until the queue empties and
 * ttyout calls ss1_stop.
 */
ss1_start(tty)
    struct tty *tty;
{
    ss1_txline(1);
    return (0);
}

/*
 * Stop the output: close IR6 and leave the transmitter and receiver up -
 * the monitor sends polled through the same chip, and the keyboard still
 * has to be heard.  There is no baud rate to set.
 */
ss1_stop(tty, flag)
    struct tty *tty;
{
    ss1_txline(0);
    return (0);
}

ss1_set(tty)
    struct tty *tty;
{
    return (0);
}

/*
 * Bring the 2651 up: transmitter and receiver both, and IR6 closed.  The
 * transmitter is never turned off (the monitor's polled driver needs it);
 * IR6 is opened by ss1_start once there is a queue to drain, and closed
 * again by ss1_stop.  This runs from cus() with interrupts off, before
 * the console tty exists, so the line must stay closed here.
 */
ss1_uart_init()
{
    out(SS1_UART_CMD, TXEN | RXEN);
    ss1_txline(0);
}

/*
 * Open the 2651 console.  The 2651 has no baud rate and no modem, so the
 * open is just the tty bring-up and the uart enable.  The console tty is
 * the MultIO's (major 1 lives in that array); the 2651 is the one port.
 */
ss1_uart_open(dev, mode)
    register dev;
{
    struct tty *tty;

    tty = &mttys[dev & 15];
    if (tty->ispeed == 0) {     /* virgin */
        ttinit(tty, dev);
        ttyopen(tty);
    }
    ss1_uart_init();
    ss1_txline(1);              /* the tty is up: let TxRDY in */
}

/*
 * Open the console: the SS1's 2651 or the MultIO's ACE, chosen by the
 * same probe that picked the PIC and the clock.  coninit (sys/inits.s)
 * calls this with the console's device number.
 */
console_open(dev)
    register dev;
{
    if (nlines == 8)            /* the SS1 answered */
        ss1_uart_open(dev, 0);
    else
        muopen(dev, 0);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
