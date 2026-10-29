/*
 * Network card driver (NET.md).
 *
 * sys/net.c
 *
 * The socket face of the network card.  /dev/net0 is a clone node: open
 * asks the card for the next free socket and the file it returns is that
 * socket, named by the forwarded minor (slot + 1).  Everything after the
 * open - read, write, ioctl, select, close - is one socket, which is what
 * lets two sessions run at once without stealing each other's bytes.
 *
 * The card never pushes data.  It pushes an event when a socket has
 * something (a connection is waiting, bytes have arrived, a connect
 * failed); the guest pulls the bytes with RECV, only for the socket the
 * caller holds.  Commands are synchronous and the driver serializes
 * them, so the reply read belongs to the command that sent it.
 *
 * LISTEN is the blocking request: it returns when a connection is
 * waiting, and the caller then forks and ACCEPTs.  ACCEPT moves the
 * waiting connection to a slot of its own and hands that slot back; the
 * kernel swaps the child's file to it, so the parent's descriptor is
 * still the listener and the child's is the connection.
 *
 * This is its own module page (sys/nethdr.c), char major 6.  The network
 * disk on block major 6 is the ND page (sys/nd.c) and the two share
 * nothing but the resident event and lock in sys/netint.c.
 */
#include <types.h>
#include <sys/sys.h>
#include <sys/con.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <sys/ioctl.h>
#include <sys/net.h>
#include <sys/pic.h>
#include <sys/ovl.h>
#include <errno.h>

extern int in(), out();
extern int copyin(), copyout(); /* mem.s: the user side */
extern int valid();             /* malloc.c: is this user page there? */
extern int sleep(), wakeup();
extern int di(), ei();
extern UINT netline;            /* resident: select(2) reopens the line */
extern int selectchan;          /* resident: select(2)'s event */
extern int netchan;             /* resident: the command/event wait */
extern int netlock;             /* resident: one command on the wire */
extern UINT8 netready[];        /* resident: a socket has something */

#define NETPAY 128              /* the most one request or pull moves; the
                                 * module page is the budget, and a chunk
                                 * this size still keeps the wire busy */

static UINT netseg;             /* this module's segment */
static UINT8 netpresent;        /* the card answered its probe */
static UINT8 netbuf[NETPAY];    /* one frame's payload */

static UINT8 nfused[8];         /* the driver's slot allocation */
static UINT8 evcode[8];         /* 0 none, else N_EV_* */
static UINT8 evip[8][4];
static UINT16 evport[8];
static UINT8 pend[8];           /* a connection waits for this listener */
static UINT8 delivered[8];      /* a LISTEN already returned it */

/*
 * The module's init: the header hands over the segment it was placed in,
 * which is what the handler and the wait name when they look the line up
 * with intrline().  There is no hardware bring-up beyond that.
 */
int
netstart(seg)
    UINT seg;
{
    netseg = seg;
    return (0);
}

/*
 * The attach probe (sys/ovl.h).  CLEAR quiets the card, TICKLE makes it
 * push one frame and raise its line.  The card is tested for first: open
 * bus reads 0xff, where the card leaves NETSTAT's upper five bits zero.
 * The drain is the card's whole receive ring, because a Z80 reset does
 * not reset the ESP32 and a warm reset can leave the last session's
 * frames queued.  The tickle's wait is bounded for a card still booting.
 */
#define NETPROBEWAIT 60000
#define NETRING      8192

static
netcard()
{
    return ((in(NETSTAT_PORT) & 0xf8) == 0);
}

static
netput(b)
    UINT8 b;
{
    register UINT n;

    for (n = 0; n < NETPROBEWAIT; n++)
        if (in(NETSTAT_PORT) & NETSTAT_TXRDY)
            break;
    out(NETDATA_PORT, b);
}

int
netprobe(mode)
    int mode;
{
    register UINT n;

    if (!netcard())
        return (0);
    netpresent = 1;

    if (mode == PROBE_CLEAR) {
        for (n = 0; n < NETRING && (in(NETSTAT_PORT) & NETSTAT_RXRDY); n++)
            in(NETDATA_PORT);
        return (0);
    }
    netput(N_STATUS);
    netput(0);                  /* socket 0 */
    netput(0);                  /* payload length 0, little-endian */
    netput(0);
    for (n = 0; n < NETPROBEWAIT; n++)
        if (in(NETSTAT_PORT) & NETSTAT_RXRDY)
            break;
    for (n = 0; n < 32; n++)    /* let the line follow the ring index */
        in(NETSTAT_PORT);
    return (0);
}

/*
 * The handler: mask the still-low line, then wake the callers.
 */
int
netint()
{
    register int l;

    l = intrline(netint, netseg);
    if (l >= 0) {
        netline = l;            /* select(): the line to reopen */
        pic_mask(l);
    }
    wakeup(&netchan);
    wakeup(&selectchan);
    return (0);
}

/*
 * A card with no interrupt line still has to wake a listener.  The tick
 * is the clock for that: once a second, whether or not anything is
 * waiting, and the woken request looks again.
 */
int
nettick()
{
    wakeup(&netchan);
    wakeup(&selectchan);
    return (0);
}

/*
 * Wait for a byte.  The line is level-asserted while the FIFO holds
 * one, so open it, look, and sleep; the handler closes it when it wakes
 * us.  -1 means the card never attached.
 */
static
netwait()
{
    register int l;
    register UINT n;

    l = intrline(netint, netseg);
    if (l < 0) {
        /*
         * No line to sleep on.  A command's reply is pushed inside the
         * write that sent it, and a pushed frame waits in the card, so
         * this is a look, not a wait; the blocking requests are woken by
         * nettick().
         */
        for (n = 0; n < NETPROBEWAIT; n++)
            if (in(NETSTAT_PORT) & NETSTAT_RXRDY)
                return (0);
        return (-1);
    }
    for (;;) {
        netline = l;
        inton(l);
        di();
        if (in(NETSTAT_PORT) & NETSTAT_RXRDY) {
            ei();
            return (0);
        }
        sleep(&netchan, PRIUSER);
    }
}

/*
 * Is the card there?  Not "is its line wired": a machine can have the
 * card without a working interrupt route, and the driver moves frames
 * perfectly well by polling NETSTAT.
 */
int
netattached()
{
    return (netpresent);
}

/*
 * One command on the wire.  The card serves one at a time, so every
 * operation takes netlock first and the reply read belongs to it.
 */
static
netlockon()
{
    di();
    while (netlock) {
        ei();
        sleep(&netlock, PRIUSER);
        di();
    }
    netlock = 1;
    ei();
}

static
netlockoff()
{
    if (netlock) {
        netlock = 0;
        wakeup(&netlock);
    }
}

static
netframe(type, sock, p, len)
    int type, sock, len;
    UINT8 *p;
{
    register int i;

    netput((UINT8) type);
    netput((UINT8) sock);
    netput((UINT8) len);
    netput((UINT8) (len >> 8));
    for (i = 0; i < len; i++)
        netput(p[i]);
}

/*
 * Read one pushed frame into hdr and up to cap payload bytes into buf.
 * Returns the payload length; bytes past cap are discarded.
 */
static
netrecv(hdr, buf, cap)
    UINT8 *hdr;
    UINT8 *buf;
    int cap;
{
    register int len, i, n;

    if (netwait() < 0)
        return (-1);
    for (i = 0; i < 4; i++)
        hdr[i] = in(NETDATA_PORT);
    len = hdr[2] | (hdr[3] << 8);
    if (len > N_MAXPAY) {
        for (i = 0; i < N_MAXPAY; i++)
            in(NETDATA_PORT);
        return (-1);
    }
    n = len < cap ? len : cap;
    for (i = 0; i < n; i++)
        buf[i] = in(NETDATA_PORT);
    for (; i < len; i++)
        in(NETDATA_PORT);
    return (len);
}

/*
 * A pushed event is kept for the socket it names.  A connected event
 * also parks the connection for LISTEN; any event makes the socket
 * readable for select.
 */
static
netstash(sock, p, len)
    int sock, len;
    UINT8 *p;
{
    register int i;

    if (sock >= 8 || len < 1)
        return;
    evcode[sock] = p[0];
    netready[sock] = 1;
    if (p[0] == N_EV_CONNECTED && len >= 7) {
        for (i = 0; i < 4; i++)
            evip[sock][i] = p[1 + i];
        evport[sock] = (p[5] << 8) | p[6];
        pend[sock] = 1;
        delivered[sock] = 0;
    }
    wakeup(&selectchan);        /* a selector may be asleep on this one */
}

/*
 * Take every frame the card has already pushed.  A command reply cannot
 * be in the FIFO here: netlock is held and every command reads its own
 * reply before releasing it.
 */
static
netdrain()
{
    UINT8 hdr[4];
    int n;

    while (in(NETSTAT_PORT) & NETSTAT_RXRDY) {
        n = netrecv(hdr, netbuf, NETPAY);
        if (n < 0)
            break;
        if (hdr[0] == N_EVENT)
            netstash(hdr[1], netbuf, n);
    }
}

/*
 * Send a command and read its reply, stashing events that arrive first.
 * *rtype and *rsock get the reply's type and socket, *rlen its payload
 * length, and the payload lands in rbuf.  Returns 0 or -1.
 */
static
netcmd(op, sock, p, len, rtype, rsock, rbuf, rcap, rlen)
    int op, sock, len;
    UINT8 *p;
    int *rtype, *rsock;
    UINT8 *rbuf;
    int rcap, *rlen;
{
    UINT8 hdr[4];
    int n;

    netframe(op, sock, p, len);
    for (;;) {
        n = netrecv(hdr, rbuf, rcap);
        if (n < 0)
            return (-1);
        if (hdr[0] == N_EVENT) {
            netstash(hdr[1], rbuf, n);
            continue;
        }
        *rtype = hdr[0];
        *rsock = hdr[1];
        *rlen = n;
        return (0);
    }
}

/*
 * A command whose reply is its status byte.  Returns that byte, or -1
 * with u.error set.
 */
static
netcall(op, sock, p, len)
    int op, sock, len;
    UINT8 *p;
{
    int rt, rs, n, st;

    netlockon();
    if (netcmd(op, sock, p, len, &rt, &rs, netbuf, NETPAY, &n) < 0) {
        netlockoff();
        u.error = EIO;
        return (-1);
    }
    st = (rt == N_ACK) ? netbuf[0] : N_CLOSED_E;
    netlockoff();
    return (st);
}

static
netfree()
{
    register int i;

    for (i = 0; i < 8; i++)
        if (!nfused[i])
            return (i);
    return (-1);
}

/*
 * Clone open: claim the next free socket and name it by the forwarded
 * minor.  iopen() copies u.fdev into the file, and every path after that
 * reaches the driver with this dev, not the node's.
 */
int
netopen(dev, mode)
    UINT dev, mode;
{
    int slot, st;
    UINT8 tcp;

    if (minor(dev) != 0) {
        u.error = ENXIO;
        return;
    }
    if (!netattached()) {
        u.error = ENODEV;       /* the card never answered its probe */
        return;
    }
    if ((slot = netfree()) < 0) {
        u.error = ENFILE;
        return;
    }
    tcp = 1;                    /* the clone node is a tcp socket */
    if ((st = netcall(N_OPEN, slot, &tcp, 1)) != 0) {
        u.error = EIO;
        return;
    }
    nfused[slot] = 1;
    evcode[slot] = 0;
    pend[slot] = 0;
    delivered[slot] = 0;
    netready[slot] = 0;
    u.fdev = (NET_MAJOR << 8) | (slot + 1);
}

int
netclose(dev, mode)
    UINT dev, mode;
{
    int slot;

    slot = minor(dev) - 1;
    if (slot < 0 || slot >= 8)
        return;
    (void) netcall(N_CLOSE, slot, (UINT8 *) 0, 0);
    nfused[slot] = 0;
    evcode[slot] = 0;
    pend[slot] = 0;
    delivered[slot] = 0;
    netready[slot] = 0;
}

/*
 * Read one chunk.  The card is asked only for this socket, so another
 * reader cannot lose a frame; EAGAIN means the selector should wait.
 */
int
netread(dev)
    UINT dev;
{
    UINT8 p[2];
    int slot, rt, rs, n, cap, st;

    slot = minor(dev) - 1;
    if (slot < 0 || slot >= 8 || !nfused[slot]) {
        u.error = EBADF;
        return;
    }
    if (evcode[slot] == N_EV_CLOSED || evcode[slot] == N_EV_RESET) {
        evcode[slot] = 0;
        netready[slot] = 0;
        u.count = 0;            /* end of file */
        return;
    }
    if (u.count == 0)
        return;
    cap = u.count > NETPAY ? NETPAY : u.count;
    p[0] = cap;
    p[1] = cap >> 8;

    netlockon();
    netdrain();
    if (evcode[slot] == N_EV_CLOSED || evcode[slot] == N_EV_RESET) {
        netlockoff();
        evcode[slot] = 0;
        netready[slot] = 0;
        u.count = 0;
        return;
    }
    if (netcmd(NET_OP_RECV, slot, p, 2, &rt, &rs, netbuf, cap, &n) < 0) {
        netlockoff();
        u.error = EIO;
        return;
    }
    if (rt == N_DATA) {
        copyout(netbuf, u.base, n);
        u.count -= n;
        netready[slot] = (n == cap);    /* maybe more where that came from */
    } else {
        st = netbuf[0];
        if (st == N_AGAIN) {
            u.error = EAGAIN;
            netready[slot] = 0;
        } else if (st == N_CLOSED_E) {
            netready[slot] = 0;
            u.count = 0;        /* end of file */
        } else {
            u.error = EIO;
        }
    }
    netlockoff();
}

/*
 * Write one chunk.  The card's ACK means accepted for transmission; a
 * full transmit buffer is EAGAIN, which the caller retries.
 */
int
netwrite(dev)
    UINT dev;
{
    int slot, st, len;

    slot = minor(dev) - 1;
    if (slot < 0 || slot >= 8 || !nfused[slot]) {
        u.error = EBADF;
        return;
    }
    if (u.count == 0)
        return;
    len = u.count > NETPAY ? NETPAY : u.count;
    copyin(u.base, netbuf, len);
    st = netcall(N_SEND, slot, netbuf, len);
    if (st == N_NO_MEMORY) {
        u.error = EAGAIN;
        return;
    }
    if (st != 0) {
        u.error = EIO;
        return;
    }
    u.count -= len;
}

/*
 * Walk a blocking request that needs events: drain, check, sleep.  The
 * tick wakes it when the card has no interrupt line.
 */
static
netawait(fn, slot)
    int (*fn) ();
    int slot;
{
    register int r;

    for (;;) {
        netlockon();
        netdrain();
        r = (*fn) (slot);
        netlockoff();
        if (r)
            return (r);
        sleep(&netchan, PRIUSER);
    }
}

static
waitconnect(slot)
    int slot;
{
    if (evcode[slot] == N_EV_CONNECTED) {
        evcode[slot] = 0;
        netready[slot] = 0;
        return (1);
    }
    if (evcode[slot] == N_EV_RESET) {
        evcode[slot] = 0;
        netready[slot] = 0;
        return (-1);
    }
    return (0);
}

static
waitlisten(slot)
    int slot;
{
    if (pend[slot] && !delivered[slot]) {
        delivered[slot] = 1;
        netready[slot] = 0;
        return (1);
    }
    return (0);
}

/*
 * Requests on the socket this descriptor names.  The descriptor is the
 * socket, so no request carries a socket number; the request code says
 * the operation and the argument is only what that operation needs.
 */
int
netioctl(dev, cmd, arg)
    UINT dev, cmd;
    char *arg;
{
    UINT8 p[8];
    int slot, rt, rs, n, st, i;

    slot = minor(dev) - 1;
    if (slot < 0 || slot >= 8 || !nfused[slot]) {
        u.error = EBADF;
        return (-1);
    }

    switch (cmd) {
    case NETBIND:
        if (!valid(arg, 2)) {
            u.error = EINVAL;
            return (-1);
        }
        copyin(arg, p, 2);
        if (netcall(N_BIND, slot, p, 2) != 0) {
            u.error = EINVAL;
            return (-1);
        }
        return (0);

    case NETCONNECT:
        if (!valid(arg, 6)) {
            u.error = EINVAL;
            return (-1);
        }
        copyin(arg, p, 6);
        if (netcall(N_CONNECT, slot, p, 6) != 0) {
            u.error = EIO;
            return (-1);
        }
        if (netawait(waitconnect, slot) < 0) {
            u.error = EIO;
            return (-1);
        }
        return (0);

    case NETLISTEN:
        if (netcall(N_LISTEN, slot, (UINT8 *) 0, 0) != 0) {
            u.error = EIO;
            return (-1);
        }
        netawait(waitlisten, slot);
        return (0);

    case NETACCEPT:
        netlockon();
        if (netcmd(N_ACCEPT, slot, (UINT8 *) 0, 0,
                &rt, &rs, netbuf, NETPAY, &n) < 0) {
            netlockoff();
            u.error = EIO;
            return (-1);
        }
        st = (rt == N_ACK) ? netbuf[0] : N_CLOSED_E;
        if (st == N_AGAIN) {
            netlockoff();
            u.error = EAGAIN;
            return (-1);
        }
        if (st != 0 || n < 2) {
            netlockoff();
            u.error = EIO;
            return (-1);
        }
        i = netbuf[1];          /* the new slot */
        if (i >= 8 || nfused[i]) {
            netlockoff();
            u.error = EIO;
            return (-1);
        }
        nfused[i] = 1;
        /*
         * Hand the peer back to the caller: it is who connected, the one
         * thing accept has to say besides the new socket itself.
         */
        if (arg && valid(arg, 6)) {
            copyout(evip[slot], arg, 4);
            p[0] = evport[slot] >> 8;
            p[1] = evport[slot];
            copyout(p, arg + 4, 2);
        }
        evcode[slot] = 0;
        pend[slot] = 0;
        delivered[slot] = 0;
        netready[slot] = 0;
        netlockoff();
        /*
         * The descriptor keeps its number and becomes the connection:
         * cioctl() sees the change and swaps the caller's file.
         */
        u.fdev = (NET_MAJOR << 8) | (i + 1);
        return (0);

    case NETCARD:
        /*
         * One card-level request, whatever it is: JOIN, GETCONF,
         * SETCONF, the socket table.  The payload is opaque here - the
         * operation and its shape are the caller's and the card's - so
         * this is one path and one copy in and out.
         */
        if (!valid(arg, sizeof(struct netcard))) {
            u.error = EINVAL;
            return (-1);
        }
        copyin(arg, p, 2);
        n = p[1];
        if (n > sizeof(struct netcard) - 2) {
            u.error = EINVAL;
            return (-1);
        }
        if (n > 0)
            copyin(arg + 2, netbuf, n);
        netlockon();
        if (netcmd(p[0], 0xff, netbuf, n, &rt, &rs, netbuf, NETPAY, &n) < 0) {
            netlockoff();
            u.error = EIO;
            return (-1);
        }
        netlockoff();
        if (rt != N_ACK) {
            u.error = EIO;
            return (-1);
        }
        if (n > sizeof(struct netcard) - 2)
            n = sizeof(struct netcard) - 2;
        p[1] = n;
        copyout(p + 1, arg + 1, 1);
        if (n)
            copyout(netbuf, arg + 2, n);
        return (0);
    }
    u.error = ENOTTY;
    return (-1);
}

struct ciovec netcvec = {
    &netopen, &netclose, &netread, &netwrite, &netioctl
};

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
