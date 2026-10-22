/*
 * The network card (NET.md), guest side.
 *
 * include/sys/net.h
 *
 * The card's two ports and the framing the guest driver (sys/net.c) moves
 * over them.  The command set, socket states and error codes are NET.md's;
 * these are the constants a caller needs to build and read frames.
 */

/*
 * /dev/net is a block device, major 6 minor 1 (dev/devlist), and every
 * operation is one ioctl(2) command block (include/sys/ioctl.h):
 *
 *	ioctl(fd, CDBCMD, &cdb)
 *
 * cdb.cmd[0] is the operation below and cdb.cmd[1] the socket; the
 * operation's own fields follow at cdb.cmd[2..].  cdb.buf/cdb.count are
 * its data phase, if it has one.  On return cdb.cmd[0] is the reply type
 * (N_ACK/N_DATA/N_EVENT), cdb.cmd[1] the socket, cdb.cmd[2] the first
 * reply byte - the ACK status, a STATUS's state, or an EVENT's code -
 * and cdb.count the reply bytes copied back.
 *
 *	N_OPEN      cmd[2] mode (0 udp, 1 tcp)
 *	N_CLOSE     -
 *	N_BIND      cmd[2],cmd[3] port, network order
 *	N_CONNECT   cmd[2..5] ip, cmd[6],cmd[7] port, network order
 *	N_LISTEN    -
 *	N_SEND      buf/count payload; flags clear
 *	N_STATUS    reply payload: state, rx[2], tx[2]
 *	N_JOIN      reply payload: ip[4]
 *	N_GETCONF   reply payload: netconf
 *	N_SETCONF   buf/count payload; flags clear
 *	NET_OP_RECV send nothing; return the next frame
 *
 * A command's reply is read before the ioctl returns, so its ACK is
 * already there; a command with a reply payload needs CDB_IN and count
 * set to the buffer's size.
 */
#define NET_MAJOR 6
#define NET_MINOR 1
#define NET_OP_RECV 0           /* not a wire type: just receive */

/* the two S-100 ports (the d1 wiring's choice, src/hwsim/d1/nicd1.c) */
#define NETDATA_PORT 0x60
#define NETSTAT_PORT 0x61

/*
 * The driver is part of the ND module (sys/nd.c's page); netchan is the
 * one resident piece, the event a blocked ioctl sleeps on.  The line is
 * discovered, not named.
 */
extern int netprobe(), netint(), netwait(), netattached();
extern int netioctl(), ndstart();
extern int netchan;

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
#define N_GETCONF 0x09
#define N_SETCONF 0x0A

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

/* the netconf payload's bounds (NET.md section 4) */
#define N_SSIDMAX 32
#define N_PASSMAX 63
#define N_CONFMAX (1 + N_SSIDMAX + 1 + N_PASSMAX)
