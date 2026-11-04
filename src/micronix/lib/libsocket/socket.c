/*
 * libsocket - BSD sockets over the network card
 *
 * lib/libsocket/socket.c
 *
 * The kernel side is the descriptor and nothing else: the card's clone
 * node (sys/NET.md) is one socket per open, and each call here is one
 * of the driver's ioctls or a read or write on that descriptor.  What
 * the kernel has to hold is only what a wakeup needs - the per-socket
 * event and ready bits select(2) reads - and accept() gets its BSD
 * "new descriptor, listener survives" from dup: the duplicate is what
 * NETACCEPT turns into the connection, and the listener keeps its own.
 *
 * Everything a socket library usually keeps beside the descriptor -
 * the names bind(), connect() and accept() learned - is in the tables
 * below, in this process, not in the kernel: the machine's kernel
 * memory is its tightest budget and none of this needs a wakeup.
 */
#include <types.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

/* the card's requests, include/sys/net.h */
#define NETBIND    0x7410
#define NETCONNECT 0x7411
#define NETLISTEN  0x7412
#define NETACCEPT  0x7413
#define NETCARD    0x7414

#define N_JOIN    0x08

#define DEVNET	"/dev/net0"
#define MAXFD	16			/* NOPEN: the descriptors one can name */

/* one card-level request and its reply (include/sys/net.h) */
struct netcard {
    unsigned char op;
    unsigned char len;
    unsigned char data[1 + 32 + 1 + 63];
};

static UINT8 haspeer[MAXFD];
static UINT8 haslocal[MAXFD];
static UINT32 peeraddr[MAXFD];
static UINT16 peerport[MAXFD];
static UINT16 localport[MAXFD];

/*
 * The card's network association.  Not a socket call: one call puts the
 * card on its network for every socket this process holds, so a daemon
 * does it once before binding.
 */
int
net_join(fd)
    int fd;
{
    struct netcard c;

    c.op = N_JOIN;
    c.len = 0;
    return (ioctl(fd, NETCARD, &c));
}

int
socket(domain, type, protocol)
    int domain, type, protocol;
{
    int fd;

    (void) protocol;
    if (domain != AF_INET || type != SOCK_STREAM) {
        errno = EINVAL;		/* the clone node is IPv4 TCP only */
        return (-1);
    }
    fd = open(DEVNET, 2);
    if (fd < 0)
        return (-1);
    if (fd < MAXFD) {
        haspeer[fd] = 0;
        haslocal[fd] = 0;
        peeraddr[fd] = 0;
        peerport[fd] = 0;
        localport[fd] = 0;
    }
    return (fd);
}

int
bind(s, addr, addrlen)
    int s, addrlen;
    struct sockaddr *addr;
{
    struct sockaddr_in *sin;
    unsigned char p[2];
    UINT16 port;

    (void) addrlen;
    if (addr == 0 || addr->sa_family != AF_INET) {
        errno = EINVAL;
        return (-1);
    }
    sin = (struct sockaddr_in *) addr;
    port = ntohs(sin->sin_port);
    p[0] = (port >> 8) & 0xff;
    p[1] = port & 0xff;
    if (ioctl(s, NETBIND, p) < 0)
        return (-1);
    if (s >= 0 && s < MAXFD) {
        haslocal[s] = 1;
        localport[s] = sin->sin_port;
    }
    return (0);
}

int
listen(s, backlog)
    int s, backlog;
{
    (void) backlog;		/* the card parks one connection per listener */
    return (ioctl(s, NETLISTEN, 0));
}

/*
 * The duplicate becomes the connection: NETACCEPT swaps the descriptor
 * it is given, so the listener the caller passed in is untouched and
 * the accepted socket is a descriptor of its own, as BSD has it.
 */
int
accept(s, addr, addrlen)
    int s, *addrlen;
    struct sockaddr *addr;
{
    struct sockaddr_in *sin;
    unsigned char peer[6];
    int fd, e;
    UINT32 ip;
    UINT16 port;

    fd = dup(s);
    if (fd < 0)
        return (-1);
    if (ioctl(fd, NETACCEPT, peer) < 0) {
        e = errno;
        close(fd);
        errno = e;
        return (-1);
    }
    ip = ((UINT32) peer[0] << 24) | ((UINT32) peer[1] << 16) |
        ((UINT32) peer[2] << 8) | (UINT32) peer[3];
    port = ((UINT16) peer[4] << 8) | peer[5];
    if (fd < MAXFD) {
        haspeer[fd] = 1;
        peeraddr[fd] = htonl(ip);
        peerport[fd] = htons(port);
    }
    if (addr && addrlen) {
        sin = (struct sockaddr_in *) addr;
        memset(sin, 0, sizeof *sin);
        sin->sin_family = AF_INET;
        sin->sin_port = htons(port);
        sin->sin_addr.s_addr = htonl(ip);
        *addrlen = sizeof *sin;
    }
    return (fd);
}

int
connect(s, addr, addrlen)
    int s, addrlen;
    struct sockaddr *addr;
{
    struct sockaddr_in *sin;
    unsigned char a[6];
    UINT32 ip;

    (void) addrlen;
    if (addr == 0 || addr->sa_family != AF_INET) {
        errno = EINVAL;
        return (-1);
    }
    sin = (struct sockaddr_in *) addr;
    ip = ntohl(sin->sin_addr.s_addr);
    a[0] = (ip >> 24) & 0xff;
    a[1] = (ip >> 16) & 0xff;
    a[2] = (ip >> 8) & 0xff;
    a[3] = ip & 0xff;
    a[4] = (ntohs(sin->sin_port) >> 8) & 0xff;
    a[5] = ntohs(sin->sin_port) & 0xff;
    if (ioctl(s, NETCONNECT, a) < 0)
        return (-1);
    if (s >= 0 && s < MAXFD) {
        haspeer[s] = 1;
        peeraddr[s] = sin->sin_addr.s_addr;
        peerport[s] = sin->sin_port;
    }
    return (0);
}

/*
 * What bind(), connect() and accept() remembered.  The card has no
 * name store to ask, and these are only ever the answers those calls
 * gave.
 */
int
getpeername(s, addr, addrlen)
    int s, *addrlen;
    struct sockaddr *addr;
{
    struct sockaddr_in *sin;

    if (s < 0 || s >= MAXFD || !haspeer[s]) {
        errno = EINVAL;		/* v6 has no ENOTCONN */
        return (-1);
    }
    if (addr && addrlen) {
        sin = (struct sockaddr_in *) addr;
        memset(sin, 0, sizeof *sin);
        sin->sin_family = AF_INET;
        sin->sin_port = peerport[s];
        sin->sin_addr.s_addr = peeraddr[s];
        *addrlen = sizeof *sin;
    }
    return (0);
}

int
getsockname(s, addr, addrlen)
    int s, *addrlen;
    struct sockaddr *addr;
{
    struct sockaddr_in *sin;

    if (s < 0 || s >= MAXFD || !haslocal[s]) {
        errno = EINVAL;
        return (-1);
    }
    if (addr && addrlen) {
        sin = (struct sockaddr_in *) addr;
        memset(sin, 0, sizeof *sin);
        sin->sin_family = AF_INET;
        sin->sin_port = localport[s];
        sin->sin_addr.s_addr = INADDR_ANY;
        *addrlen = sizeof *sin;
    }
    return (0);
}

/* no options and no half-close on the card: these are not errors */
int
setsockopt(s, level, name, val, len)
    int s, level, name, len;
    char *val;
{
    return (0);
}

int
shutdown(s, how)
    int s, how;
{
    (void) s;
    (void) how;
    return (0);
}

UINT16
htons(x)
    UINT16 x;
{
    return (((x >> 8) & 0xff) | ((x & 0xff) << 8));
}

UINT16
ntohs(x)
    UINT16 x;
{
    return (htons(x));
}

UINT32
htonl(x)
    UINT32 x;
{
    return (((UINT32) ((x >> 24) & 0xff)) |
        ((UINT32) ((x >> 16) & 0xff) << 8) |
        ((UINT32) ((x >> 8) & 0xff) << 16) |
        ((UINT32) (x & 0xff) << 24));
}

UINT32
ntohl(x)
    UINT32 x;
{
    return (htonl(x));
}

/*
 * "a.b.c.d" to network order, INADDR_NONE for anything else - the BSD
 * answer, and the same one inet_addr has always had for a bad address.
 */
UINT32
inet_addr(cp)
    char *cp;
{
    UINT32 a;
    int i, v;

    a = 0;
    for (i = 0; i < 4; i++) {
        if (*cp < '0' || *cp > '9') {
            errno = EINVAL;
            return (INADDR_NONE);
        }
        v = 0;
        while (*cp >= '0' && *cp <= '9')
            v = v * 10 + (*cp++ - '0');
        if (v > 255) {
            errno = EINVAL;
            return (INADDR_NONE);
        }
        a = (a << 8) | (UINT32) v;
        if (i < 3) {
            if (*cp != '.') {
                errno = EINVAL;
                return (INADDR_NONE);
            }
            cp++;
        }
    }
    return (htonl(a));
}

char *
inet_ntoa(in)
    struct in_addr *in;
{
    static char buf[18];
    UINT32 a;

    a = ntohl(in->s_addr);
    sprintf(buf, "%d.%d.%d.%d",
        (int) ((a >> 24) & 0xff),
        (int) ((a >> 16) & 0xff),
        (int) ((a >> 8) & 0xff),
        (int) (a & 0xff));
    return (buf);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
