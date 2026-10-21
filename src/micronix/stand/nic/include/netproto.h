/*
 * netproto.h - the wire between the guest and the network card.
 *
 * src/micronix/sys/NET.md is the authority; this file is its constants
 * in the form both ends of the byte channel use.  The simulator's card
 * model, src/hwsim/d1/nic.c, is the reference implementation, and the
 * guest driver is src/micronix/sys/net.c.  Any change here is a change
 * to all three.
 */
#ifndef NIC_NETPROTO_H
#define NIC_NETPROTO_H

#include <stdint.h>

/* The Z80's two ports. */
#define NIC_PORT_DATA   0x60    /* NETDATA: rx byte out, tx byte in */
#define NIC_PORT_STAT   0x61    /* NETSTAT */

#define NETSTAT_RXRDY   0x01    /* an rx byte is waiting */
#define NETSTAT_TXRDY   0x02    /* the tx fifo has room */
#define NETSTAT_RDY     0x04    /* the card is up: JOIN finished */

#define NIC_NSOCK       8
#define NIC_MAXPAY      1024    /* a frame's payload, NET.md section 2 */

/* guest -> card, NET.md section 4 */
#define C_OPEN          0x01
#define C_CLOSE         0x02
#define C_BIND          0x03
#define C_CONNECT       0x04
#define C_LISTEN        0x05
#define C_SEND          0x06
#define C_STATUS        0x07
#define C_JOIN          0x08

/* card -> guest, NET.md section 5 */
#define P_ACK           0x80
#define P_DATA          0x81
#define P_EVENT         0x82

/* socket states, NET.md section 3 */
#define S_CLOSED        0
#define S_UDP           1
#define S_TCP_IDLE      2
#define S_TCP_LISTEN    3
#define S_TCP_CONNECT   4
#define S_TCP_OPEN      5

/* ACK status codes, NET.md section 5 */
#define E_NO_SOCKET     1
#define E_BAD_STATE     2
#define E_NO_MEMORY     3
#define E_REFUSED       4
#define E_TIMEOUT       5
#define E_CLOSED        6

/* EVENT codes, NET.md section 5 */
#define EV_CONNECTED    1
#define EV_CLOSED       2
#define EV_RESET        3

#endif
