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
 * /dev/net0 is a character device, major 6, and a clone node: open asks
 * the card for the next free socket and the file it returns is that
 * socket, named by the forwarded minor (slot + 1).  read and write move
 * that socket's bytes; ioctl(2) carries the requests below.  Each is its
 * own request code, so the driver copies only the little each needs:
 *
 *	ioctl(fd, NETBIND, &port)	port[2] network order
 *	ioctl(fd, NETCONNECT, &addr)	ip[4], port[2] network order
 *	ioctl(fd, NETLISTEN, 0)		blocks until a connection waits
 *	ioctl(fd, NETACCEPT, 0)		takes it; the descriptor becomes it
 *	ioctl(fd, NETCARD, &card)	one card-level request (JOIN,
 *					GETCONF, SETCONF, the socket table)
 *
 * The card never pushes data.  It pushes an event when a socket has
 * something and the guest pulls the bytes with RECV - the driver asks
 * only for the socket the caller holds, so two readers cannot lose one
 * another's frames.
 *
 * A connected EVENT carries the peer's ip[4] and port[2] in network
 * order behind it, so a server can authenticate the caller and netstat
 * can show the connection.
 */
#define NET_MAJOR 6
#define NET_MINOR 1             /* the old block face, now unused */
#define NET_OP_RECV 0           /* not a wire type: pull this socket */

/* the two S-100 ports (the d1 wiring's choice, src/hwsim/d1/nicd1.c) */
#define NETDATA_PORT 0x60
#define NETSTAT_PORT 0x61

/*
 * The ioctl request codes.  They are in the terminal's range's
 * neighbourhood (whoever else wants a char device gets its own).
 */
#define NETBIND    0x7410
#define NETCONNECT 0x7411
#define NETLISTEN  0x7412
#define NETACCEPT  0x7413
#define NETCARD    0x7414

/*
 * One card-level request and its reply.  op is an N_* card operation
 * (JOIN, GETCONF, SETCONF, SOCKSTAT); len is the payload the caller
 * sends, and on return the payload the card answered with.  data is
 * wide enough for the netconf pair and for the socket table.
 */
struct netcard {
    UINT8 op;
    UINT8 len;
    UINT8 data[1 + 32 + 1 + 63];
};


/*
 * The driver is its own module page (sys/nethdr.c).  netchan, netlock and
 * netready are the resident pieces in sys/netint.c; the line is
 * discovered, not named.
 */
extern int netprobe(), netint(), netattached();
extern int netchan, netlock;
extern UINT8 netready[];

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
#define N_ACCEPT  0x0B
#define N_SOCKSTAT 0x0C         /* card-level: the socket table */

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
#define N_AGAIN     7

/* event codes */
#define N_EV_CONNECTED 1        /* code, peer ip[4], peer port[2] */
#define N_EV_CLOSED    2
#define N_EV_RESET     3
#define N_EV_READY     4        /* data waits; pull it with RECV */

/*
 * One N_SOCKSTAT record, nine bytes: state, local port[2], peer ip[4],
 * peer port[2], each port in network order.  netstat(1) reads these.
 */
#define NETSTATSZ 9

/* a frame's largest payload */
#define N_MAXPAY 1024

/* the netconf payload's bounds (NET.md section 4) */
#define N_SSIDMAX 32
#define N_PASSMAX 63
#define N_CONFMAX (1 + N_SSIDMAX + 1 + N_PASSMAX)
