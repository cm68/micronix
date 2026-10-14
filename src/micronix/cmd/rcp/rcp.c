/*
 * rcp - remote copy, over rsh
 *
 * cmd/rcp/rcp.c
 *
 * The guest's remote-copy client, over the NIC's socket face (sys/NET.md).
 * One connection to the remote's rshd; .rhosts on that end does the auth.
 * The syntax and function clone the real rcp:
 *
 *     rcp host:file1 host:file2 ... /local/dir     GET remote -> local dir
 *     rcp /local/file1 /local/file2 ... host:/dir  PUT local -> remote dir
 *     rcp host:/path/file  /local/name             one file, renamed
 *
 * The remote side carries the host: prefix (an ip).  The last argument is
 * the destination.  It drives /dev/net (sys/net.c), the frame-level
 * channel: this sends the rsh request, then streams the rcp files.
 *
 * The frame types below mirror include/sys/net.h, which this program
 * cannot see (it builds against lib/include, not the kernel's include).
 */

#include <stdio.h>
#include <string.h>

#define DEV      "/dev/net"
#define N_MAXPAY 1024

/* frame types (include/sys/net.h) */
#define N_OPEN    0x01
#define N_CLOSE   0x02
#define N_CONNECT 0x04
#define N_SEND    0x06
#define N_JOIN    0x08
#define N_ACK     0x80
#define N_DATA    0x81
#define N_EVENT   0x82
#define N_EV_CONNECTED 1

#define RSH_PORT  514
#define RSH_USER  "curt"        /* the host user the rshd's .rhosts names */

extern long lseek();            /* the kernel's seek, which returns the offset */

static int fd;                  /* /dev/net, the frame channel */
static int sock;                /* the one rsh socket */
static char rbuf[N_MAXPAY];     /* a frame payload scratch */
static int rtype, rsock;        /* the last frame net_rx read */

static int port = RSH_PORT;
static int recursive;           /* -r: walk directories */
static int stream_closed;       /* the remote closed; net_wait saw the event */

/* ---- frame io ---- */

static void
net_tx(type, sk, p, len)
    int type, sk, len;
    char *p;
{
    char hdr[4];

    hdr[0] = type;
    hdr[1] = sk;
    hdr[2] = len & 0xff;
    hdr[3] = (len >> 8) & 0xff;
    write(fd, hdr, 4);
    if (len)
        write(fd, p, len);
}

/* read one pushed frame into buf; return its payload length, or -1 */
static int
net_rx(typep, sockp, buf, max)
    int *typep, *sockp, max;
    char *buf;
{
    char r[4 + N_MAXPAY];
    int n, len, i;

    n = read(fd, r, sizeof r);
    if (n < 4)
        return -1;
    *typep = r[0] & 0xff;
    *sockp = r[1] & 0xff;
    len = (r[2] & 0xff) | ((r[3] & 0xff) << 8);
    if (len > n - 4)
        len = n - 4;
    if (len > max)
        len = max;
    for (i = 0; i < len; i++)
        buf[i] = r[4 + i];
    return len;
}

/* read frames until one of `want` on `sk`; return its payload length */
static int
net_wait(want, sk, buf, max)
    int want, sk, max;
    char *buf;
{
    int n;

    for (;;) {
        n = net_rx(&rtype, &rsock, buf, max);
        if (n < 0)
            return -1;
        if (rtype == want && rsock == sk)
            return n;
        if (rtype == N_EVENT && rsock == sk)
            stream_closed = 1;  /* remember it; rx_byte will see it */
    }
}

static void
net_join()
{
    net_tx(N_JOIN, 0xff, (char *)0, 0);
    net_wait(N_ACK, 0xff, (char *)0, 0);
}

/* open tcp socket sk, connect it, wait for it to establish */
static int
net_connect(sk, ip, prt)
    int sk, prt;
    int *ip;
{
    char c[6];
    char mode = 1;              /* tcp */

    net_tx(N_OPEN, sk, &mode, 1);
    if (net_wait(N_ACK, sk, (char *)0, 0) < 0)
        return -1;

    c[0] = ip[0];
    c[1] = ip[1];
    c[2] = ip[2];
    c[3] = ip[3];
    c[4] = (prt >> 8) & 0xff;
    c[5] = prt & 0xff;
    net_tx(N_CONNECT, sk, c, 6);
    if (net_wait(N_ACK, sk, (char *)0, 0) < 0)
        return -1;
    if (net_wait(N_EVENT, sk, rbuf, 1) < 0)
        return -1;
    return (rbuf[0] == N_EV_CONNECTED) ? 0 : -1;
}

static void
net_close(sk)
    int sk;
{
    net_tx(N_CLOSE, sk, (char *)0, 0);
    net_wait(N_ACK, sk, (char *)0, 0);
}

/* send a chunk to the remote and wait for the card's ack */
static void
send_bytes(buf, len)
    char *buf;
    int len;
{
    net_tx(N_SEND, sock, buf, len);
    net_wait(N_ACK, sock, (char *)0, 0);
}

/* the rcp byte stream reader: serves the remote's bytes one at a time */
static char rxacc[N_MAXPAY];
static int rxoff, rxlen;

static int
rx_byte()
{
    int n;

    for (;;) {
        if (stream_closed)
            return -1;      /* the remote closed: end of stream */
        if (rxoff < rxlen)
            return rxacc[rxoff++] & 0xff;
        rxoff = 0;
        n = net_rx(&rtype, &rsock, rxacc, N_MAXPAY);
        if (n < 0)
            return -1;
        if (rtype == N_EVENT && rsock == sock)
            return -1;      /* the remote closed or reset: end of stream */
        rxlen = (rtype == N_DATA && rsock == sock) ? n : 0;
    }
}

/* read exactly n bytes from the remote */
static int
rx_exact(buf, n)
    char *buf;
    int n;
{
    int i;

    for (i = 0; i < n; i++) {
        int b = rx_byte();

        if (b < 0)
            return -1;
        buf[i] = b;
    }
    return 0;
}

/* read exactly n bytes and write them to fd */
static int
rx_to_fd(out, n)
    int out;
    long n;
{
    static char dbuf[N_MAXPAY];
    int chunk;

    while (n > 0) {
        chunk = (n > N_MAXPAY) ? N_MAXPAY : (int)n;
        if (rx_exact(dbuf, chunk) < 0)
            return -1;
        write(out, dbuf, chunk);
        n -= chunk;
    }
    return 0;
}

/* the last path component, after the final '/' */
static char *
basename(path)
    char *path;
{
    char *b = path;

    while (*path) {
        if (*path == '/')
            b = path + 1;
        path++;
    }
    return b;
}

/* parse a dotted-quad into ip[4] */
static int
parse_ip(host, ip)
    char *host;
    int *ip;
{
    int n, oct, i;

    n = 0;
    oct = 0;
    for (i = 0; host[i]; i++) {
        if (host[i] >= '0' && host[i] <= '9')
            n = n * 10 + (host[i] - '0');
        else if (host[i] == '.') {
            if (oct >= 3 || n > 255)
                return -1;
            ip[oct++] = n;
            n = 0;
        } else
            return -1;
    }
    if (oct != 3 || n > 255)
        return -1;
    ip[3] = n;
    return 0;
}

/* resolve a host - an ip, or a name the card's DNS would - to ip[4] */
static int
resolve_host(host, ip)
    char *host;
    int *ip;
{
    static char *names[] = { "nook", "127.0.0.1" };

    if (parse_ip(host, ip) == 0)
        return 0;
    if (strcmp(host, names[0]) == 0)
        return parse_ip(names[1], ip);
    return -1;
}

/* build the rsh request: "0\0root\0root\0<command>\0"; return its length */
static int
make_request(buf, command)
    char *buf, *command;
{
    int n = 0, i;
    char *u;

    buf[n++] = '0';             /* no stderr back-channel */
    buf[n++] = '\0';
    for (u = RSH_USER; *u; u++) /* local user */
        buf[n++] = *u;
    buf[n++] = '\0';
    for (u = RSH_USER; *u; u++) /* remote user */
        buf[n++] = *u;
    buf[n++] = '\0';
    for (i = 0; command[i]; i++)
        buf[n++] = command[i];
    buf[n++] = '\0';
    return n;
}

/* send one local file: T header, basename, bytes, NUL, then read the ack */
static void
put_one(path)
    char *path;
{
    int f, n;
    long size;
    char hdr[64], *bp;

    f = open(path, 0);
    if (f < 0) {
        fprintf(stderr, "rcp: cannot read %s\n", path);
        return;
    }
    size = lseek(f, 0L, 2);     /* seek to end: the size */
    lseek(f, 0L, 0);            /* rewind */

    bp = basename(path);
    sprintf(hdr, "C0644 %ld %s\n", size, bp);
    send_bytes(hdr, strlen(hdr));
    rx_exact(rbuf, 1);          /* the sink's ack */

    while ((n = read(f, rbuf, N_MAXPAY)) > 0)
        send_bytes(rbuf, n);
    rx_exact(rbuf, 1);          /* the sink's ack */

    send_bytes("\0", 1);        /* end of file */
    rx_exact(rbuf, 1);          /* the sink's ack */
    close(f);
}

/* the -r directory walk: the current directory, and its depth below dest */
static char curpath[512];
static int depth;

/* receive one piece: "D" (a directory), "C" (a file), "E" (up), or close */
static int
get_one(path, usedir)
    char *path;
    int usedir;
{
    char hdr[128], name[128], lpath[512], *p;
    int mode, i, c, f;
    long size;

    /* the header line: "D/C<mode> <size> <name>\n" or "E\n".  The source
     * closes the connection at the end of the transfer, so EOF here is
     * the end. */
    for (i = 0; i < sizeof hdr - 1; i++) {
        c = rx_byte();
        if (c < 0)
            return 0;
        if (c == '\n')
            break;
        hdr[i] = c;
    }
    hdr[i] = '\0';

    if (hdr[0] == 'E') {
        send_bytes("\0", 1);    /* ack the E */
        return 2;               /* up a directory */
    }
    if (hdr[0] != 'D' && hdr[0] != 'C')
        return 0;

    p = hdr + 1;
    mode = 0;
    for (i = 0; i < 4 && *p; i++)
        mode = mode * 8 + (*p++ - '0');
    while (*p == ' ')
        p++;
    size = 0;
    while (*p >= '0' && *p <= '9')
        size = size * 10 + (*p++ - '0');
    while (*p == ' ')
        p++;
    strcpy(name, p);

    if (hdr[0] == 'D') {
        /* a directory: the top-level one is the dest itself; deeper ones
         * append their name */
        if (depth > 0)
            sprintf(curpath + strlen(curpath), "/%s", name);
        mknod(curpath, 040000 | (mode & 0777), 0);
        depth++;
        send_bytes("\0", 1);    /* ack the D */
        return 1;
    }

    /* "C": a file.  The source sends the header, then the bytes and the
     * end-of-file NUL back to back, and waits once after them - so ack the
     * header, take the data, take the NUL, ack the pair. */
    if (usedir || depth > 0)
        sprintf(lpath, "%s/%s", curpath, name);
    else
        strcpy(lpath, path);    /* a single-file rename, ignore the name */
    send_bytes("\0", 1);        /* ack the C header */
    f = creat(lpath, 0666);
    if (f < 0) {
        fprintf(stderr, "rcp: cannot create %s\n", lpath);
        return -1;
    }
    rx_to_fd(f, size);
    close(f);
    rx_exact(rbuf, 1);          /* the end-of-file NUL */
    send_bytes("\0", 1);        /* ack the data + NUL */
    return 1;
}

static void
usage()
{
    fprintf(stderr, "usage: rcp [-P port] host:file ... dir\n");
    fprintf(stderr, "       rcp [-P port] file ... host:dir\n");
    exit(1);
}

main(argc, argv)
    int argc;
    char **argv;
{
    char *args[64], *dest, *colon, *host, *rdest, cmd[512], req[512], hostbuf[64];
    int ip[4], nargs, nsrc, i, j, k, n;
    int get, isdir;

    nargs = 0;
    for (i = 1; i < argc; i++) {
        char *a = argv[i];

        if (a[0] == '-' && a[1]) {
            if (a[1] == 'P')
                port = atoi(argv[++i]);
            else if (a[1] == 'r')
                recursive = 1;
            else
                usage();
        } else if (nargs < 64)
            args[nargs++] = a;
    }
    if (nargs < 2)
        usage();

    dest = args[nargs - 1];
    nsrc = nargs - 1;
    get = (strchr(args[0], ':') != 0);  /* remote sources -> GET */

    if (get) {
        colon = strchr(args[0], ':');
        for (k = 0; args[0][k] && args[0][k] != ':'; k++)
            hostbuf[k] = args[0][k];
        hostbuf[k] = '\0';
        host = hostbuf;
        rdest = (char *)0;
    } else {
        colon = strchr(dest, ':');
        if (!colon)
            usage();
        for (k = 0; dest[k] && dest[k] != ':'; k++)
            hostbuf[k] = dest[k];
        hostbuf[k] = '\0';
        host = hostbuf;
        rdest = colon + 1;
    }
    if (resolve_host(host, ip) < 0) {
        fprintf(stderr, "rcp: %s is not an ip address\n", host);
        exit(1);
    }

    /* is the destination a directory? (matters for GET's local writes) */
    isdir = (nsrc > 1);
    if (!isdir) {
        char *d = get ? dest : rdest;
        int n2 = strlen(d);
        if (n2 > 0 && d[n2 - 1] == '/')
            isdir = 1;
    }

    fd = open(DEV, 2);
    if (fd < 0) {
        fprintf(stderr, "rcp: no %s\n", DEV);
        exit(1);
    }
    sock = 0;
    net_join();
    if (net_connect(sock, ip, port) < 0) {
        fprintf(stderr, "rcp: connect to %s failed\n", host);
        exit(1);
    }

    /* the rsh request: the command is rcp -t (send) or rcp -f (fetch) */
    if (get) {
        /* rcp [-r] -f <src1> <src2> ... */
        strcpy(cmd, recursive ? "rcp -r -f" : "rcp -f");
        for (j = 0; j < nsrc; j++) {
            strcat(cmd, " ");
            strcat(cmd, strchr(args[j], ':') + 1);
        }
    } else {
        /* rcp [-r] -t <dest> */
        strcpy(cmd, recursive ? "rcp -r -t " : "rcp -t ");
        strcat(cmd, rdest);
    }
    n = make_request(req, cmd);
    send_bytes(req, n);

    if (get) {
        /* the source sends a NUL first, then D/C/E pieces, and closes at
         * the end; ack each piece, the C header and its data too */
        rx_exact(rbuf, 1);
        send_bytes("\0", 1);    /* ack the opening NUL */
        strcpy(curpath, dest);
        depth = 0;
        for (;;) {
            i = get_one(dest, isdir);
            if (i == 2) {       /* "E": up a directory */
                if (depth > 1) {
                    char *slash = strrchr(curpath, '/');

                    if (slash)
                        *slash = '\0';
                }
                depth--;
            } else if (i <= 0)
                break;
        }
    } else {
        /* the sink sends a NUL first; the remote -t places by basename */
        rx_exact(rbuf, 1);
        for (j = 0; j < nsrc; j++)
            put_one(args[j]);
    }

    net_close(sock);
    close(fd);
    exit(0);
}
