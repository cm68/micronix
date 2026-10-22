/*
 * Network card driver (NET.md).
 *
 * sys/net.c
 *
 * The network functions of the ND module: /dev/net is block major 6,
 * minor 1, and every operation is one ioctl(2) command block.  The
 * driver builds the card's frame from the block's typed fields, sends
 * it, and reads the reply before returning.
 *
 * The code is linked into nd.mod (sys/nd.c's page), the card being the
 * same one the network disk lives on; none of it costs the u page or
 * the resident text.  netchan (sys/netint.c) is the one resident piece:
 * the event a blocked ioctl sleeps on.
 *
 * The line is not named.  netstart() registers netprobe/netint with the
 * attach loop when the module is placed; intrline() finds the line
 * again from the handler's address and this module's segment.  The
 * handler masks the line as it wakes the caller, so a frame arriving
 * with nobody waiting cannot interrupt forever; the next call opens the
 * line again.
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
extern int copy();              /* mem.s: kernel-to-kernel */
extern int copyin(), copyout(); /* mem.s: the user side */
extern int sleep(), wakeup();

static UINT netseg;             /* this module's segment, from netstart() */
static UINT8 netbuf[N_MAXPAY];  /* one frame's payload */

/*
 * The module init (sys/nd.h's header).  The ND driver has no bring-up
 * of its own; this is where the net driver's interrupt joins the attach
 * loop, with the segment the module was placed in.
 */
int
ndstart(seg)
    UINT seg;
{
    netseg = seg;
    intrset(netprobe, netint, seg);
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
 * The handler: mask the still-low line, then wake the caller.
 */
int
netint()
{
    register int l;

    l = intrline(netint, netseg);
    if (l >= 0)
        pic_mask(l);
    wakeup(&netchan);
    return (0);
}

/*
 * Wait for a byte.  The line is level-asserted while the FIFO holds
 * one, so open it, look, and sleep; the handler closes it when it wakes
 * us.  -1 means the card never attached.
 */
int
netwait()
{
    register int l;

    for (;;) {
        l = intrline(netint, netseg);
        if (l < 0)
            return (-1);
        inton(l);
        di();
        if (in(NETSTAT_PORT) & NETSTAT_RXRDY) {
            ei();
            return (0);
        }
        sleep(&netchan, PRIUSER);
    }
}

int
netattached()
{
    return (intrline(netint, netseg) >= 0);
}

/*
 * Read one pushed frame into netbuf and its header into hdr.  The card
 * pushes whole frames, so the wait above is for the first byte and the
 * rest follow without a second.
 */
static
netrecv(hdr, lenp)
    UINT8 *hdr;
    int *lenp;
{
    register int len, i;

    if (netwait() < 0)
        return (-1);
    for (i = 0; i < 4; i++)
        hdr[i] = in(NETDATA_PORT);
    len = hdr[2] | (hdr[3] << 8);
    if (len > N_MAXPAY) {       /* the card never pushes more */
        for (i = 0; i < N_MAXPAY; i++)
            in(NETDATA_PORT);
        return (-1);
    }
    for (i = 0; i < len; i++)
        netbuf[i] = in(NETDATA_PORT);
    *lenp = len;
    return (0);
}

/*
 * Build the operation's payload in netbuf.  The data phase of SEND and
 * SETCONF is the caller's buffer - the kernel's block when it fits, the
 * caller's own page when it does not - and the rest come from the
 * typed fields in cmd[].
 */
static
netbuild(op, r, b)
    int op;
    struct cdb *r;
    struct buf *b;
{
    register int i, n;
    register UINT8 *p;

    switch (op) {
    case N_OPEN:
        netbuf[0] = r->cmd[2];
        return (1);
    case N_BIND:
        netbuf[0] = r->cmd[2];
        netbuf[1] = r->cmd[3];
        return (2);
    case N_CONNECT:
        for (i = 0; i < 6; i++)
            netbuf[i] = r->cmd[2 + i];
        return (6);
    case N_SEND:
    case N_SETCONF:
        n = r->count;
        if (n < 0 || n > N_MAXPAY)
            return (-1);
        if (n == 0)
            return (0);
        if (n <= 512 && b) {
            p = (UINT8 *) bhold(b);
            copy(p, netbuf, n);
            brel();
        } else
            copyin(r->buf, netbuf, n);
        return (n);
    case N_CLOSE:
    case N_LISTEN:
    case N_STATUS:
    case N_JOIN:
    case N_GETCONF:
        return (0);
    }
    return (-1);
}

/*
 * One typed operation.  The request goes out as the card's four-byte
 * header plus the payload; the reply is the next frame, its header and
 * first byte going back in cmd[] and up to count bytes of payload in
 * the caller's buffer.  A command with no data phase gets its ACK
 * status in cmd[2] and no payload.
 */
int
netioctl(dev, cmd, r, b)
    UINT dev, cmd;
    struct cdb *r;
    struct buf *b;
{
    register int i, n, cap;
    register UINT8 *p;
    int len;
    UINT8 op, sock, hdr[4];

    if (cmd != CDBCMD || minor(dev) != NET_MINOR) {
        u.error = ENOTTY;
        return (-1);
    }
    if (!netattached()) {
        u.error = ENXIO;
        return (-1);
    }
    op = r->cmd[0] & 0xff;
    sock = r->cmd[1] & 0xff;
    cap = r->count;

    if (op != NET_OP_RECV) {
        if ((op == N_SEND || op == N_SETCONF) && (r->flags & CDB_IN)) {
            u.error = EINVAL;   /* a send's data goes to the card */
            return (-1);
        }
        len = netbuild(op, r, b);
        if (len < 0) {
            u.error = EINVAL;
            return (-1);
        }
        netput(op);
        netput(sock);
        netput(len & 0xff);
        netput((len >> 8) & 0xff);
        for (i = 0; i < len; i++)
            netput(netbuf[i]);
    }

    if (netrecv(hdr, &len) < 0) {
        u.error = EIO;
        return (-1);
    }
    r->cmd[0] = hdr[0];
    r->cmd[1] = hdr[1];
    r->cmd[2] = len ? netbuf[0] : 0;
    r->cmd[3] = 0;

    if ((r->flags & CDB_IN) && cap > 0) {
        n = len < cap ? len : cap;
        if (cap <= 512 && b) {
            p = (UINT8 *) bhold(b);
            copy(netbuf, p, n);
            brel();
        } else
            copyout(netbuf, r->buf, n);
        r->count = n;
    } else
        r->count = 0;
    return (0);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
