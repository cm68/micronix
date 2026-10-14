/*
 * The network card (NET.md), guest side.
 *
 * include/sys/net.h
 *
 * The card's two ports and the framing the guest driver (sys/net.c) moves
 * over them.  The command set, socket states and error codes are NET.md's;
 * these are the constants a caller needs to build and read frames.
 */

/* the two S-100 ports (the d1 wiring's choice, src/hwsim/d1/nicd1.c) */
#define NETDATA_PORT 0x60
#define NETSTAT_PORT 0x61

/* NETSTAT bits */
#define NETSTAT_RXRDY 0x01     /* a pushed byte waits to be read */
#define NETSTAT_TXRDY 0x02     /* a byte can be written */
#define NETSTAT_RDY   0x04     /* the card is up (JOIN finished) */

/* guest -> card commands */
#define N_OPEN    0x01
#define N_CLOSE   0x02
#define N_BIND    0x03
#define N_CONNECT 0x04
#define N_LISTEN  0x05
#define N_SEND    0x06
#define N_STATUS  0x07
#define N_JOIN    0x08

/* card -> guest */
#define N_ACK     0x80
#define N_DATA    0x81
#define N_EVENT   0x82

/* socket states (a STATUS ack's first byte) */
#define N_CLOSED      0
#define N_UDP         1
#define N_TCP_IDLE    2
#define N_TCP_LISTEN  3
#define N_TCP_CONNECT 4
#define N_TCP_OPEN    5

/* ack error codes */
#define N_NO_SOCKET 1
#define N_BAD_STATE 2
#define N_NO_MEMORY 3
#define N_REFUSED   4
#define N_TIMEOUT   5
#define N_CLOSED_E  6

/* event codes */
#define N_EV_CONNECTED 1
#define N_EV_CLOSED    2
#define N_EV_RESET     3

/* a frame's largest payload */
#define N_MAXPAY 1024
