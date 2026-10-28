/*
 * The CompuPro SS1's 2651 console, resident half.
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
 *
 * What is here is the hardware bring-up, which pic_init() selects as
 * console_minit and cus() calls before any module is placed.  The tty
 * back ends and the console open are in the driver page
 * (sys/ss1_uartleaf.c), where the line discipline that calls them lives.
 */
#include <types.h>
#include <sys/sys.h>
#include <sys/tty.h>
#include <sys/pic8259.h>

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
 * The driver page calls this through ss1_start/ss1_stop.
 */
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
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
