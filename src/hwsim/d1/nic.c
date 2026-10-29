/*
 * The network card — NET.md's socket face, host side.
 *
 * The guest drives a byte channel: writes framed commands into NETDATA,
 * reads framed ACK/DATA/EVENT back out of it, and reads NETSTAT for
 * RXRDY/TXRDY/RDY.  This side of the channel is a small 8259-free state
 * machine that assembles the guest's frames, executes them against real
 * host sockets, and pushes the results back.  The framing and the command
 * set are sys/NET.md; this is one of its three implementations.
 *
 * The card core is self-contained on purpose: no simulator calls, only
 * POSIX sockets, so nic_test.c can drive it before d1 wires the ports on.
 * nic_poll() is the one periodic thing — the d1 wiring calls it from a
 * recurring timer, and it is where connects finish and data arrives.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>

#include "nic.h"

/* guest -> card (sys/NET.md section 4) */
#define C_OPEN    0x01
#define C_CLOSE   0x02
#define C_BIND    0x03
#define C_CONNECT 0x04
#define C_LISTEN  0x05
#define C_SEND    0x06
#define C_STATUS  0x07
#define C_JOIN    0x08
#define C_GETCONF 0x09
#define C_SETCONF 0x0A
#define C_ACCEPT  0x0B
#define C_SOCKSTAT 0x0C         /* card-level: the socket table */
#define C_RECV    0x00          /* not a wire type: pull this socket's data */

/* card -> guest (sys/NET.md section 5) */
#define P_ACK     0x80
#define P_DATA    0x81
#define P_EVENT   0x82

/* socket states (sys/NET.md section 3) */
#define S_CLOSED      0
#define S_UDP         1
#define S_TCP_IDLE    2
#define S_TCP_LISTEN  3
#define S_TCP_CONNECT 4
#define S_TCP_OPEN    5

/* ACK error codes */
#define E_NO_SOCKET 1
#define E_BAD_STATE 2
#define E_NO_MEMORY 3
#define E_REFUSED   4
#define E_TIMEOUT   5
#define E_CLOSED    6
#define E_AGAIN     7

/* EVENT codes */
#define EV_CONNECTED 1
#define EV_CLOSED    2
#define EV_RESET     3
#define EV_READY     4

#define NSOCK   8
#define MAXPAY  1024

struct sock {
    int fd;         /* host fd, -1 if none */
    int state;      /* S_* */
    int mode;       /* 0 udp, 1 tcp */
    unsigned char txbuf[MAXPAY];  /* a SEND's bytes not yet on the wire */
    int txlen;
    int lport;          /* the bound local port */
    int pfd;            /* a connection parked for ACCEPT, -1 if none */
    int notified;       /* a ready EVENT is outstanding */
    unsigned char rip[4];   /* the peer of pfd / of an open connection */
    int rport;
};

static struct sock socks[NSOCK];
static int joined;

/* the netconf pair, as SETCONF left it (the card's own copy) */
static char ncSsid[33];
static char ncPass[65];

/* the RX FIFO (card -> guest): framed bytes the guest reads out */
#define RXQ  8192
static unsigned char rxq[RXQ];
static int rxq_head, rxq_tail, rxq_n;

/* the frame being assembled from the guest's writes (guest -> card) */
static unsigned char frame[4 + MAXPAY];
static int f_n;         /* bytes assembled so far */
static int f_need;      /* total bytes in this frame */
static int f_payload;   /* 0 reading the 4-byte header, 1 reading payload */

static void
rxq_put(unsigned char b)
{
    if (rxq_n >= RXQ)
        return;                 /* drop; the tcp window is the real backpressure */
    rxq[rxq_tail] = b;
    rxq_tail = (rxq_tail + 1) % RXQ;
    rxq_n++;
}

/*
 * Push one frame into the RX FIFO.  Whole-or-nothing: if it does not fit
 * it is dropped rather than left half-written for the guest to misread.
 */
static void
push_frame(unsigned char type, unsigned char sock, const unsigned char *p, int len)
{
    int i;
    if (rxq_n + 4 + len > RXQ)
        return;
    rxq_put(type);
    rxq_put(sock);
    rxq_put(len & 0xff);
    rxq_put((len >> 8) & 0xff);
    for (i = 0; i < len; i++)
        rxq_put(p[i]);
}

static void
push_ack(unsigned char sock, unsigned char status)
{
    unsigned char p[1];

    p[0] = status;
    push_frame(P_ACK, sock, p, 1);
}

static void
push_event(unsigned char sock, unsigned char code)
{
    unsigned char p[1];

    p[0] = code;
    push_frame(P_EVENT, sock, p, 1);
}

/*
 * A connected event carries the peer's address: code, then ip[4] in
 * network order and the peer's port[2] in network order.  rshd needs the
 * address for .rhosts, and the port is what netstat shows; the guest has
 * no other way to ask who called.  Closed, reset and ready stay one byte.
 */
static void
push_event_ip(unsigned char sock, unsigned char code, int fd)
{
    struct sockaddr_in a;
    socklen_t al;
    unsigned char p[7];

    p[0] = code;
    al = sizeof a;
    if (getpeername(fd, (struct sockaddr *) &a, &al) == 0) {
        memcpy(p + 1, &a.sin_addr.s_addr, 4);
        p[5] = (ntohs(a.sin_port) >> 8) & 0xff;
        p[6] = ntohs(a.sin_port) & 0xff;
    } else {
        memset(p + 1, 0, 6);
    }
    push_frame(P_EVENT, sock, p, 7);
}

/* ---- the commands (guest -> card) ---- */

/*
 * Put the listener's host socket back in LISTEN.  A connection arriving
 * is parked on the listener and the host listener is closed, so a second
 * client gets a refused connection until ACCEPT has taken the first.
 * This is what reopens that door.
 */
static int
arm_listen(unsigned char s)
{
    struct sockaddr_in a;
    int fd, on = 1;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
        return (-1);
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof on);
    memset(&a, 0, sizeof a);
    a.sin_family = AF_INET;
    a.sin_addr.s_addr = INADDR_ANY;
    a.sin_port = htons(socks[s].lport);
    if (bind(fd, (struct sockaddr *)&a, sizeof a) < 0 ||
        listen(fd, 1) < 0) {
        close(fd);
        return (-1);
    }
    fcntl(fd, F_SETFL, O_NONBLOCK);
    socks[s].fd = fd;
    socks[s].state = S_TCP_LISTEN;
    return (0);
}

static int
free_slot(void)
{
    int i;

    for (i = 0; i < NSOCK; i++)
        if (socks[i].fd < 0 && socks[i].pfd < 0 &&
            socks[i].state == S_CLOSED)
            return (i);
    return (-1);
}

static void
nic_open(unsigned char s, unsigned char mode)
{
    if (s >= NSOCK || socks[s].state != S_CLOSED) {
        push_ack(s, E_BAD_STATE);
        return;
    }
    socks[s].mode = mode ? 1 : 0;
    socks[s].fd = -1;
    socks[s].pfd = -1;
    socks[s].txlen = 0;
    socks[s].notified = 0;
    socks[s].lport = 0;
    socks[s].rport = 0;
    if (!mode) {
        int fd = socket(AF_INET, SOCK_DGRAM, 0);

        if (fd < 0) {
            push_ack(s, E_NO_MEMORY);
            return;
        }
        fcntl(fd, F_SETFL, O_NONBLOCK);
        socks[s].fd = fd;
    }
    socks[s].state = mode ? S_TCP_IDLE : S_UDP;
    push_ack(s, 0);
}

static void
nic_close(unsigned char s)
{
    if (s >= NSOCK)
        return;
    if (socks[s].pfd >= 0) {
        close(socks[s].pfd);
        socks[s].pfd = -1;
    }
    if (socks[s].fd >= 0) {
        /*
         * A SEND is acknowledged when it is queued here, not when it is
         * on the wire: nic_poll writes the backlog out.  A close that
         * arrives between the two must not drop the last output of a
         * session, so give the backlog a short bounded drain first.
         */
        int tries;

        for (tries = 0; socks[s].txlen > 0 && tries < 20; tries++) {
            struct pollfd pf;
            int r;

            r = send(socks[s].fd, socks[s].txbuf, socks[s].txlen,
                     MSG_DONTWAIT);
            if (r > 0) {
                if (r == socks[s].txlen)
                    socks[s].txlen = 0;
                else {
                    memmove(socks[s].txbuf, socks[s].txbuf + r,
                            socks[s].txlen - r);
                    socks[s].txlen -= r;
                }
                continue;
            }
            if (r < 0 && errno != EAGAIN && errno != EWOULDBLOCK)
                break;
            pf.fd = socks[s].fd;
            pf.events = POLLOUT;
            (void) poll(&pf, 1, 5);     /* 5 ms; the loop bounds the wait */
        }
        close(socks[s].fd);
        socks[s].fd = -1;
    }
    socks[s].state = S_CLOSED;
    socks[s].mode = 0;
    socks[s].txlen = 0;
    socks[s].notified = 0;
    socks[s].lport = 0;
    socks[s].rport = 0;
    push_ack(s, 0);
}

static void
nic_bind(unsigned char s, const unsigned char *p)
{
    struct sockaddr_in a;

    if (s >= NSOCK || socks[s].state == S_CLOSED) {
        push_ack(s, E_NO_SOCKET);
        return;
    }
    socks[s].lport = (p[0] << 8) | p[1];      /* port[2] network order */
    if (socks[s].mode != 0) {
        push_ack(s, 0);         /* tcp: the listener binds when armed */
        return;
    }
    memset(&a, 0, sizeof a);
    a.sin_family = AF_INET;
    a.sin_addr.s_addr = INADDR_ANY;
    a.sin_port = htons(socks[s].lport);
    if (bind(socks[s].fd, (struct sockaddr *)&a, sizeof a) < 0) {
        push_ack(s, E_BAD_STATE);
        return;
    }
    push_ack(s, 0);
}

static void
nic_connect(unsigned char s, const unsigned char *p)
{
    struct sockaddr_in a;
    int r, fd, on = 1;

    if (s >= NSOCK || socks[s].state == S_CLOSED) {
        push_ack(s, E_NO_SOCKET);
        return;
    }
    memset(&a, 0, sizeof a);
    a.sin_family = AF_INET;
    memcpy(&a.sin_addr.s_addr, p, 4);
    a.sin_port = htons((p[4] << 8) | p[5]);   /* port[2] network order */

    if (socks[s].mode == 0) {
        /* udp: connect fixes the default peer, and that is all */
        if (connect(socks[s].fd, (struct sockaddr *)&a, sizeof a) < 0) {
            push_ack(s, E_BAD_STATE);
            return;
        }
        socks[s].state = S_UDP;
        push_ack(s, 0);
        return;
    }

    if (socks[s].fd >= 0) {
        push_ack(s, E_BAD_STATE);
        return;
    }
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        push_ack(s, E_NO_MEMORY);
        return;
    }
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof on);
    fcntl(fd, F_SETFL, O_NONBLOCK);
    socks[s].fd = fd;
    r = connect(fd, (struct sockaddr *)&a, sizeof a);
    if (r < 0 && errno != EINPROGRESS) {
        close(fd);
        socks[s].fd = -1;
        push_ack(s, E_REFUSED);
        return;
    }
    socks[s].state = S_TCP_CONNECT;
    push_ack(s, 0);
}

static void
nic_listen(unsigned char s)
{
    if (s >= NSOCK || socks[s].state == S_CLOSED) {
        push_ack(s, E_NO_SOCKET);
        return;
    }
    if (socks[s].mode != 1 || socks[s].lport == 0) {
        push_ack(s, E_BAD_STATE);
        return;
    }
    if (socks[s].pfd >= 0) {
        push_ack(s, 0);         /* a connection already waits: the door
                                 * reopens when ACCEPT takes it */
        return;
    }
    if (socks[s].state == S_TCP_LISTEN && socks[s].fd >= 0) {
        push_ack(s, 0);         /* already armed */
        return;
    }
    push_ack(s, arm_listen(s) < 0 ? E_NO_MEMORY : 0);
}

/*
 * Take the parked connection.  The listener keeps its port and its state;
 * the accepted connection gets a slot of its own and the listener is
 * armed again, so the parent can go straight back to LISTEN.
 */
static void
nic_accept(unsigned char s)
{
    unsigned char p[2];
    unsigned char rip[4];
    struct sockaddr_in a;
    socklen_t al;
    int n, rport;

    if (s >= NSOCK || socks[s].state != S_TCP_LISTEN) {
        push_ack(s, E_BAD_STATE);
        return;
    }
    if (socks[s].pfd < 0) {
        push_ack(s, E_AGAIN);
        return;
    }
    if ((n = free_slot()) < 0) {
        push_ack(s, E_NO_MEMORY);
        return;
    }

    memset(rip, 0, sizeof rip);
    rport = 0;
    al = sizeof a;
    if (getpeername(socks[s].pfd, (struct sockaddr *) &a, &al) == 0) {
        memcpy(rip, &a.sin_addr.s_addr, 4);
        rport = ntohs(a.sin_port);
    }

    socks[n].fd = socks[s].pfd;
    socks[n].pfd = -1;
    socks[n].mode = 1;
    socks[n].state = S_TCP_OPEN;
    socks[n].txlen = 0;
    socks[n].notified = 0;
    socks[n].lport = socks[s].lport;
    memcpy(socks[n].rip, rip, 4);
    socks[n].rport = rport;

    socks[s].pfd = -1;
    p[0] = 0;                   /* status: accepted */
    p[1] = n;                   /* the socket it became */
    push_frame(P_ACK, s, p, 2);
    (void) arm_listen(s);       /* the door reopens; a failure is retried
                                 * by the next LISTEN */
}

static void
nic_send(unsigned char s, const unsigned char *p, int len)
{
    int r;

    if (s >= NSOCK || socks[s].fd < 0) {
        push_ack(s, E_NO_SOCKET);
        return;
    }
    if (socks[s].state != S_TCP_OPEN && socks[s].state != S_UDP) {
        push_ack(s, E_BAD_STATE);
        return;
    }
    if (socks[s].txlen > 0) {
        push_ack(s, E_NO_MEMORY);   /* a backlog is still draining */
        return;
    }
    r = send(socks[s].fd, p, len, MSG_DONTWAIT);
    if (r == len) {
        push_ack(s, 0);
        return;
    }
    if (r >= 0) {                   /* partial: buffer the rest */
        p += r;
        len -= r;
    } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
        push_ack(s, E_CLOSED);
        return;
    }
    /* accepted for transmission; the poll drains the backlog */
    memcpy(socks[s].txbuf, p, len);
    socks[s].txlen = len;
    push_ack(s, 0);
}

/*
 * Pull this socket's data.  Nothing is ever pushed: a caller that wants
 * bytes asks for them, which is what makes two readers on two fds safe.
 * E_AGAIN means "nothing yet, try again"; E_CLOSED is end of file.
 */
static void
nic_recv(unsigned char s, int cap)
{
    unsigned char buf[MAXPAY];
    int r;

    if (s >= NSOCK || socks[s].state == S_CLOSED) {
        push_ack(s, E_NO_SOCKET);
        return;
    }
    if (socks[s].state != S_TCP_OPEN && socks[s].state != S_UDP) {
        push_ack(s, E_AGAIN);
        return;
    }
    if (cap <= 0 || cap > MAXPAY)
        cap = MAXPAY;
    r = recv(socks[s].fd, buf, cap, MSG_DONTWAIT);
    if (r > 0) {
        socks[s].notified = 0;
        push_frame(P_DATA, s, buf, r);
        return;
    }
    if (r == 0) {                       /* the peer closed cleanly */
        close(socks[s].fd);
        socks[s].fd = -1;
        socks[s].state = S_CLOSED;
        push_ack(s, E_CLOSED);
        return;
    }
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
        push_ack(s, E_AGAIN);
        return;
    }
    close(socks[s].fd);
    socks[s].fd = -1;
    socks[s].state = S_CLOSED;
    push_ack(s, E_CLOSED);
}

static void
nic_status_cmd(unsigned char s)
{
    unsigned char p[5];

    if (s >= NSOCK)
        return;
    /* rx/tx are 0: data is pushed and sent immediately, nothing buffers
     * per socket, so the flow control is the SEND's no-memory ACK. */
    p[0] = socks[s].state;
    p[1] = 0;
    p[2] = 0;
    p[3] = socks[s].txlen & 0xff;
    p[4] = (socks[s].txlen >> 8) & 0xff;
    push_frame(P_ACK, s, p, 5);
}

static void
nic_join(void)
{
    unsigned char ip[4] = { 127, 0, 0, 1 }; /* the host, for the sim */

    joined = 1;
    push_frame(P_ACK, 0xff, ip, 4);
}

/*
 * netconf: the third way to give the card a network, over this
 * interface.  GETCONF returns the pair; SETCONF takes it.  The real card
 * keeps it in NVS and reconnects; the sim keeps it here, which is enough
 * for the guest command and the contract.
 */
static void
nic_getconf(void)
{
    unsigned char p[1 + 32 + 1 + 63];
    int n = 0, l;

    l = strlen(ncSsid);
    p[n++] = l;
    memcpy(p + n, ncSsid, l);
    n += l;
    l = strlen(ncPass);
    p[n++] = l;
    memcpy(p + n, ncPass, l);
    n += l;
    push_frame(P_ACK, 0xff, p, n);
}

static void
nic_setconf(const unsigned char *p, int len){
    int sl, pl;

    if (len < 2) {
        push_ack(0xff, E_BAD_STATE);
        return;
    }
    sl = p[0];
    if (sl < 1 || sl > 32 || len < 1 + sl + 1) {
        push_ack(0xff, E_BAD_STATE);
        return;
    }
    pl = p[1 + sl];
    if (pl > 63 || len < 1 + sl + 1 + pl) {
        push_ack(0xff, E_BAD_STATE);
        return;
    }
    memcpy(ncSsid, p + 1, sl);
    ncSsid[sl] = '\0';
    memcpy(ncPass, p + 1 + sl + 1, pl);
    ncPass[pl] = '\0';
    push_ack(0xff, 0);
}

/*
 * The socket table netstat prints: one nine-byte record per socket that
 * is a listener or an open connection - state, local port, peer address,
 * peer port, ports in network order.  A parked connection's peer is the
 * one that connected; the listener keeps its own port and no peer.
 */
static void
nic_sockstat(void)
{
    unsigned char tab[NSOCK * 9];
    int n = 0, i;

    for (i = 0; i < NSOCK; i++) {
        struct sockaddr_in a;
        socklen_t al;
        int lport, rport;

        if (socks[i].state != S_TCP_LISTEN && socks[i].state != S_TCP_OPEN)
            continue;
        lport = socks[i].lport;
        rport = socks[i].rport;
        memset(tab + n + 3, 0, 4);
        if (socks[i].state == S_TCP_OPEN && socks[i].fd >= 0) {
            al = sizeof a;
            if (getsockname(socks[i].fd, (struct sockaddr *) &a, &al) == 0)
                lport = ntohs(a.sin_port);
            al = sizeof a;
            if (getpeername(socks[i].fd, (struct sockaddr *) &a, &al) == 0) {
                memcpy(tab + n + 3, &a.sin_addr.s_addr, 4);
                rport = ntohs(a.sin_port);
            }
        }
        tab[n] = socks[i].state;
        tab[n + 1] = (lport >> 8) & 0xff;
        tab[n + 2] = lport & 0xff;
        tab[n + 7] = (rport >> 8) & 0xff;
        tab[n + 8] = rport & 0xff;
        n += 9;
    }
    push_frame(P_ACK, 0xff, tab, n);
}

/* ---- the frame dispatcher ---- */

static void
nic_dispatch(void)
{
    unsigned char type = frame[0];
    unsigned char sock = frame[1];
    unsigned char *p = frame + 4;
    int len = f_need - 4;

    switch (type) {
    case C_OPEN:
        nic_open(sock, p[0]);
        break;
    case C_CLOSE:
        nic_close(sock);
        break;
    case C_BIND:
        nic_bind(sock, p);
        break;
    case C_CONNECT:
        nic_connect(sock, p);
        break;
    case C_LISTEN:
        nic_listen(sock);
        break;
    case C_ACCEPT:
        nic_accept(sock);
        break;
    case C_RECV:
        nic_recv(sock, len ? (p[0] | (p[1] << 8)) : MAXPAY);
        break;
    case C_SEND:
        nic_send(sock, p, len);
        break;
    case C_STATUS:
        nic_status_cmd(sock);
        break;
    case C_JOIN:
        nic_join();
        break;
    case C_GETCONF:
        nic_getconf();
        break;
    case C_SETCONF:
        nic_setconf(p, len);
        break;
    case C_SOCKSTAT:
        nic_sockstat();
        break;
    default:
        push_ack(sock, E_BAD_STATE);
        break;
    }
}

/* ---- the register face ---- */

void
nic_init(void)
{
    int s;

    for (s = 0; s < NSOCK; s++) {
        socks[s].fd = -1;
        socks[s].pfd = -1;
        socks[s].state = S_CLOSED;
        socks[s].mode = 0;
        socks[s].txlen = 0;
        socks[s].notified = 0;
        socks[s].lport = 0;
        socks[s].rport = 0;
    }
    joined = 0;
    ncSsid[0] = ncPass[0] = '\0';
    rxq_head = rxq_tail = rxq_n = 0;
    f_n = f_need = f_payload = 0;
}

unsigned char
nic_status(void)
{
    unsigned char s = 0;

    if (rxq_n > 0)
        s |= NETSTAT_RXRDY;
    s |= NETSTAT_TXRDY;         /* the FIFO always has room */
    if (joined)
        s |= NETSTAT_RDY;
    return s;
}

void
nic_data_out(unsigned char v)
{
    frame[f_n++] = v;
    if (!f_payload && f_n == 4) {
        f_need = 4 + (frame[2] | (frame[3] << 8));
        f_payload = 1;
    }
    if (f_payload && f_n >= f_need) {
        nic_dispatch();
        f_n = 0;
        f_payload = 0;
    }
}

unsigned char
nic_data_in(void)
{
    unsigned char b;
    if (rxq_n == 0)
        return 0;
    b = rxq[rxq_head];
    rxq_head = (rxq_head + 1) % RXQ;
    rxq_n--;
    return b;
}

/*
 * The periodic pass: finish connects, take accepted connections, and
 * drain arrived data.  Called from a recurring timer in the d1 wiring,
 * not from the port handlers, so a blocking host call never stalls the
 * guest — everything here is non-blocking.
 */
void
nic_poll(void)
{
    struct pollfd pfds[NSOCK];
    int idx[NSOCK];
    int n, i;

    n = 0;
    for (i = 0; i < NSOCK; i++) {
        if (socks[i].fd < 0)
            continue;
        idx[n] = i;
        pfds[n].fd = socks[i].fd;
        pfds[n].events = 0;
        if (socks[i].state == S_TCP_CONNECT)
            pfds[n].events = POLLOUT;
        else if (socks[i].state == S_TCP_LISTEN ||
                 socks[i].state == S_TCP_OPEN || socks[i].state == S_UDP) {
            pfds[n].events = POLLIN;
            if (socks[i].txlen > 0)
                pfds[n].events |= POLLOUT;
        }
        pfds[n].revents = 0;
        n++;
    }
    if (n == 0)
        return;
    if (poll(pfds, n, 0) <= 0)
        return;

    for (i = 0; i < n; i++) {
        int s = idx[i];
        short rev = pfds[i].revents;

        if (rev == 0)
            continue;

        if (socks[s].state == S_TCP_CONNECT) {
            int err = 0;
            socklen_t el = sizeof err;

            getsockopt(socks[s].fd, SOL_SOCKET, SO_ERROR, &err, &el);
            if (err == 0) {
                socks[s].state = S_TCP_OPEN;
                push_event_ip(s, EV_CONNECTED, socks[s].fd);
            } else {
                close(socks[s].fd);
                socks[s].fd = -1;
                socks[s].state = S_CLOSED;
                push_event(s, EV_RESET);
            }
        } else if (socks[s].state == S_TCP_LISTEN) {
            int c;

            if (socks[s].pfd >= 0)
                continue;       /* a connection is already parked */
            c = accept(socks[s].fd, NULL, NULL);
            if (c >= 0) {
                fcntl(c, F_SETFL, O_NONBLOCK);
                socks[s].pfd = c;
                close(socks[s].fd);
                socks[s].fd = -1;
                push_event_ip(s, EV_CONNECTED, c);
            }
        } else if (socks[s].state == S_TCP_OPEN || socks[s].state == S_UDP) {
            if (rev & POLLOUT && socks[s].txlen > 0) {
                int r = send(socks[s].fd, socks[s].txbuf, socks[s].txlen,
                             MSG_DONTWAIT);

                if (r > 0) {
                    if (r == socks[s].txlen)
                        socks[s].txlen = 0;
                    else {
                        memmove(socks[s].txbuf, socks[s].txbuf + r,
                                socks[s].txlen - r);
                        socks[s].txlen -= r;
                    }
                }
            }
            /*
             * Readiness only: the data stays in the host socket until the
             * guest pulls it, so two readers on two fds cannot steal one
             * another's frames.  One notice is enough until it is pulled.
             */
            if (!socks[s].notified &&
                (rev & (POLLIN | POLLHUP | POLLERR))) {
                socks[s].notified = 1;
                push_event(s, EV_READY);
            }
        }
    }
}
