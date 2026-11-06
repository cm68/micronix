/*
 * The CompuPro SS1's 2651 console, driver-page half.
 *
 * sys/ss1_uartleaf.c
 *
 * The tty back ends and the console open.  They are linked into tty.mod
 * with the line discipline and the MultIO front end (sys/multio.c), so
 * the function pointers a console tty holds name functions in the same
 * page the line discipline runs in.
 *
 * The hardware bring-up stays resident (sys/ss1_uart.c): pic_init()
 * selects it as console_minit before any module can be placed, and it
 * runs from cus() with interrupts off.  This file calls it through the
 * resident entry, which is the allowed direction (module into kernel).
 */
#include <types.h>
#include <sys/sys.h>
#include <sys/tty.h>
#include <sys/pic8259.h>

extern struct tty *ttdev(INT16);     /* minor -> slot (sys/multio.c) */
extern void ttinit(struct tty *, INT16), ttyopen();
extern void ss1_txline(INT16), ss1_uart_init();

#define SS1_UART_DATA   0x7c
#define SS1_UART_STAT   0x7d
#define SS1_UART_MODE   0x7e
#define SS1_UART_CMD    0x7f

#define TXRDY   0x01
#define RXRDY   0x02
#define TXEN    0x01
#define RXEN    0x04

extern INT16 in(), out();

/*
 * Send a character.  The transmitter is the only buffering, and the
 * caller (the tty output path) already holds interrupts off, so the
 * write is the whole of it.
 */

/* prototypes for functions defined below */
int out(int, int);
void ttyopen(struct tty *);
void
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
INT16
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
INT16
ss1_stop(tty, flag)
    struct tty *tty;
    INT16 flag;
{
    ss1_txline(0);
    return (0);
}

INT16
ss1_set(tty)
    struct tty *tty;
{
    return (0);
}

/*
 * Open the 2651 console.  The 2651 has no baud rate and no modem, so the
 * open is just the tty bring-up and the uart enable.  The console tty is
 * the MultIO's (major 1 lives in that array); the 2651 is the one port.
 */
void
ss1_uart_open(dev, mode)
    register INT16 dev;
    INT16 mode;
{
    struct tty *tty;

    tty = ttdev(dev);
    if (tty->ispeed == 0) {     /* virgin */
        ttinit(tty, dev);
        ttyopen(tty);
    }
    /*
     * Output stays closed at open, as the MultIO's open leaves it: the
     * 2651's transmitter is always ready and its TxRDY stays asserted,
     * so an open IR6 with nothing queued is an interrupt storm, not an
     * idle line.  ss1_start opens it when there is output to drain and
     * ss1_stop closes it again; the line discipline calls both.
     */
    ss1_uart_init();
    ss1_txline(0);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
