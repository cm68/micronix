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

extern struct tty *ttdev();     /* minor -> slot (sys/multio.c) */
extern int ttinit(), ttyopen();
extern int ss1_txline(), ss1_uart_init();

#define SS1_UART_DATA   0x7c
#define SS1_UART_STAT   0x7d
#define SS1_UART_MODE   0x7e
#define SS1_UART_CMD    0x7f

#define TXRDY   0x01
#define RXRDY   0x02
#define TXEN    0x01
#define RXEN    0x04

extern int in(), out();

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
 * Open the 2651 console.  The 2651 has no baud rate and no modem, so the
 * open is just the tty bring-up and the uart enable.  The console tty is
 * the MultIO's (major 1 lives in that array); the 2651 is the one port.
 */
ss1_uart_open(dev, mode)
    register dev;
{
    struct tty *tty;

    tty = ttdev(dev);
    if (tty->ispeed == 0) {     /* virgin */
        ttinit(tty, dev);
        ttyopen(tty);
    }
    ss1_uart_init();
    ss1_txline(1);              /* the tty is up: let TxRDY in */
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
