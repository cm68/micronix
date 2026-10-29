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
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/signal.h>
#include <pwd.h>

#define DEV      "/dev/net0"
#define N_MAXPAY 256

/* the socket face (include/sys/net.h) */
#define NETCONNECT 0x7411
#define NETCARD    0x7414
#define N_JOIN     0x08

#define RSH_PORT  514

extern long lseek();            /* the kernel's seek, which returns the offset */

/* one card-level request and its reply (include/sys/net.h) */
struct netcard {
    unsigned char op;
    unsigned char len;
    unsigned char data[1 + 32 + 1 + 63];
};

static int fd;                  /* the connection, after connect */
static int port = RSH_PORT;
static int recursive;           /* -r: walk directories */
static char rsh_user[32];       /* who this end says it is, from getpwuid */

static int
bit(n)
    int n;
{
    int b;

    b = 1;
    while (n-- > 0)
        b <<= 1;
    return (b);
}

onalarm()
{
    signal(SIGALRM, onalarm);   /* V7 semantics: re-arm the handler */
}

/*
 * One readable chunk.  select is the wait; the alarm makes it return
 * when the card has no interrupt line, and EAGAIN means the wakeup was
 * another socket's event, not end of file.
 */
static int
net_read(buf, max)
    char *buf;
    int max;
{
    int r, n;

    for (;;) {
        alarm(1);
        r = select(bit(fd));
        if (r < 0) {
            if (errno == EINTR) {
                alarm(0);
                continue;
            }
            alarm(0);
            return (-1);
        }
        if (r & bit(fd)) {
            n = read(fd, buf, max);
            alarm(0);
            if (n < 0 && errno == EAGAIN)
                continue;
            return (n);
        }
    }
}

/* all of it; the card's transmit buffer fills and drains under us */
static int
send_bytes(buf, len)
    char *buf;
    int len;
{
    int n, sent;

    sent = 0;
    while (sent < len) {
        n = write(fd, buf + sent, len - sent);
        if (n > 0) {
            sent += n;
            continue;
        }
        if (n < 0 && errno == EAGAIN) {
            sleep(1);
            continue;
        }
        return (-1);
    }
    return (0);
}

/* the rcp byte stream reader: serves the remote's bytes one at a time */
static char rxacc[N_MAXPAY];
static int rxoff, rxlen;

/* a local scratch for the file protocol's one-byte acks and data */
static char rbuf[N_MAXPAY];

static int
rx_byte()
{
    for (;;) {
        if (rxoff < rxlen)
            return rxacc[rxoff++] & 0xff;
        rxoff = 0;
        rxlen = net_read(rxacc, N_MAXPAY);
        if (rxlen <= 0)
            return -1;
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
    for (u = rsh_user; *u; u++) /* local user */
        buf[n++] = *u;
    buf[n++] = '\0';
    for (u = rsh_user; *u; u++) /* remote user */
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

    {
        struct passwd *pw = getpwuid(getuid());

        if (pw && pw->name[0])
            strncpy(rsh_user, pw->name, sizeof rsh_user - 1);
        else
            strcpy(rsh_user, "root");
        rsh_user[sizeof rsh_user - 1] = 0;
    }

    fd = open(DEV, 2);
    if (fd < 0) {
        fprintf(stderr, "rcp: no %s\n", DEV);
        exit(1);
    }
    signal(SIGALRM, onalarm);
    {
        struct netcard c;

        c.op = N_JOIN;
        c.len = 0;
        (void) ioctl(fd, NETCARD, &c);
    }
    {
        unsigned char a[6];
        int i;

        for (i = 0; i < 4; i++)
            a[i] = ip[i];
        a[4] = (port >> 8) & 0xff;
        a[5] = port & 0xff;
        if (ioctl(fd, NETCONNECT, a) < 0) {
            fprintf(stderr, "rcp: connect to %s failed\n", host);
            exit(1);
        }
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

    close(fd);
    exit(0);
}
