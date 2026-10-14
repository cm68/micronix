/*
 * The network card — NET.md's socket face.
 *
 * An ESP32 (all of tcp/ip) behind a bus CPLD; the Z80 sees one byte
 * channel through two ports.  The core is self-contained (POSIX sockets,
 * no simulator dependencies) so nic_test.c can drive it host-side before
 * d1 wires it in.  The two ports are:
 *
 *   NETDATA   read: a byte from the RX FIFO;  write: a byte into the TX FIFO
 *   NETSTAT   read: RXRDY / TXRDY / RDY
 *
 * Framing and the command set are sys/NET.md; the port numbers are the
 * d1 wiring's (nicd1.c).
 */
#ifndef NIC_H
#define NIC_H

#define NETSTAT_RXRDY 0x01    /* the RX FIFO holds a byte to read */
#define NETSTAT_TXRDY 0x02    /* the TX FIFO has room for a byte */
#define NETSTAT_RDY   0x04    /* the card is up (JOIN finished) */

void nic_init(void);                 /* reset the card */
unsigned char nic_status(void);      /* NETSTAT: RXRDY/TXRDY/RDY */
void nic_data_out(unsigned char v);  /* NETDATA write: a TX byte */
unsigned char nic_data_in(void);     /* NETDATA read: an RX byte */
void nic_poll(void);                 /* periodic: connect/recv/send host sockets */

#endif
