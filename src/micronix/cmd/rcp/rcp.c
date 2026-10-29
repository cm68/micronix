/*
 * rcp - remote copy, over rsh
 *
 * cmd/rcp/rcp.c
 *
 * The guest's remote-copy client, and the server half rshd runs on the
 * far end, over the NIC's socket face (sys/NET.md).  One connection to
 * the remote's rshd; .rhosts on that end does the auth.  The client
 * invokes the server as "rcp -f" (fetch: send the named files) or
 * "rcp -t" (sink: receive into the destination) over the rsh session;
 * those modes run on the pty's stdin/stdout and never touch the card.
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
 * The request codes below mirror include/sys/net.h, which this program
 * cannot see (it builds against lib/include, not the kernel's include).
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <types.h>
#include <sys/ioctl.h>
#include <sys/signal.h>
#include <sys/fs.h>
#include <sys/stat.h>
#include <sys/sgtty.h>
#include <sys/dir.h>
#include <dirent.h>
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
static char cmd[512], req[512]; /* the rsh command and request lines */

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

/* send one local file: C header, basename, bytes, NUL, then read the ack */
static int
put_one(path)
    char *path;
{
    int f, n;
    long size;
    char hdr[64], *bp;

    f = open(path, 0);
    if (f < 0) {
        fprintf(stderr, "rcp: cannot read %s\n", path);
        return (-1);
    }
    size = lseek(f, 0L, 2);     /* seek to end: the size */
    lseek(f, 0L, 0);            /* rewind */

    bp = basename(path);
    sprintf(hdr, "C0644 %ld %s\n", size, bp);
    send_bytes(hdr, strlen(hdr));
    rx_exact(rbuf, 1);          /* the sink's ack */

    while ((n = read(f, rbuf, N_MAXPAY)) > 0)
        send_bytes(rbuf, n);

    send_bytes("\0", 1);        /* end of file */
    rx_exact(rbuf, 1);          /* the sink's ack for the data and NUL */
    close(f);
    return (0);
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
        if (depth > 0 || usedir)
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

/*
 * The last path component, with any trailing slashes ignored, copied
 * into buf so the result is stable.
 */
static char *
basepath(path, buf)
    char *path, *buf;
{
    int n = strlen(path);

    while (n > 1 && path[n - 1] == '/')
        n--;
    memcpy(buf, path, n);
    buf[n] = 0;
    return (basename(buf));
}

static int
isdirpath(path)
    char *path;
{
    struct stat st;

    if (stat(path, &st) < 0)
        return (0);
    return ((st.st_mode & S_IFMT) == S_IFDIR);
}

/*
 * Send a file, or under -r a directory and everything below it.  A
 * directory is a D header, its entries in readdir order, and an E; each
 * piece waits for the sink's one-byte acknowledgement.
 */
static int
send_tree(path)
    char *path;
{
    struct stat st;
    struct dir *dp;
    DIR *d;
    char child[512], pbuf[512], hdr[160], *name;

    if (stat(path, &st) < 0) {
        fprintf(stderr, "rcp: cannot read %s\n", path);
        return (-1);
    }
    if ((st.st_mode & S_IFMT) != S_IFDIR) {
        if ((st.st_mode & S_IFMT) != S_IFREG) {
            fprintf(stderr, "rcp: %s is not a regular file\n", path);
            return (-1);
        }
        return (put_one(path));
    }
    if (!recursive) {
        fprintf(stderr, "rcp: %s is a directory (use -r)\n", path);
        return (-1);
    }

    name = basepath(path, pbuf);
    sprintf(hdr, "D%04o 0 %s\n", st.st_mode & 0777, name);
    if (send_bytes(hdr, strlen(hdr)) < 0 || rx_exact(rbuf, 1) < 0)
        return (-1);
    if ((d = opendir(path)) == 0) {
        fprintf(stderr, "rcp: cannot read %s\n", path);
        return (-1);
    }
    while ((dp = (struct dir *) readdir(d)) != 0) {
        if (dp->name[0] == '.' &&
            (dp->name[1] == 0 || (dp->name[1] == '.' && dp->name[2] == 0)))
            continue;
        if (path[strlen(path) - 1] == '/')
            sprintf(child, "%s%s", path, dp->name);
        else
            sprintf(child, "%s/%s", path, dp->name);
        if (send_tree(child) < 0) {
            closedir(d);
            return (-1);
        }
    }
    closedir(d);
    if (send_bytes("E\n", 2) < 0 || rx_exact(rbuf, 1) < 0)
        return (-1);
    return (0);
}

/*
 * Receive file and directory pieces until the source closes.  usedir
 * says the destination is an existing directory, so a top-level D
 * appends its name there rather than renaming the destination.
 */
static int
recv_all(dest, usedir)
    char *dest;
    int usedir;
{
    int r;

    strcpy(curpath, dest);
    depth = 0;
    for (;;) {
        r = get_one(dest, usedir);
        if (r == 2) {           /* "E": up a directory */
            if (depth > 1 || usedir) {
                char *slash = strrchr(curpath, '/');

                if (slash == curpath)
                    curpath[1] = 0;     /* stay at the root */
                else if (slash)
                    *slash = 0;
            }
            depth--;
        } else if (r <= 0)
            break;
    }
    return (0);
}

/*
 * The protocol is 8-bit and unlined: a one-byte ack must come back
 * before the next piece, and a NUL is a NUL.  rshd gives a command the
 * connection itself, so this normally does nothing (gtty on a net fd is
 * ENOTTY); a server run from an interactive shell has a cooked pty, and
 * this puts it in raw mode before the first byte.
 */
static void
rawmode()
{
    struct sgtty sg;

    if (gtty(0, &sg) >= 0) {
        sg.mode = RAW;          /* no echo, no line, no cr/lf or tab maps */
        (void) stty(0, &sg);
    }
}

/*
 * The two server halves, run by rshd's shell on the pty.  fd 0 is the
 * terminal and is open read/write, so it is both streams.
 */
static int
server_fetch(args, nargs)
    char **args;
    int nargs;
{
    int i;

    fd = 0;
    rawmode();
    signal(SIGALRM, onalarm);
    if (send_bytes("\0", 1) < 0 || rx_exact(rbuf, 1) < 0)
        return (1);
    for (i = 0; i < nargs; i++)
        if (send_tree(args[i]) < 0)
            return (1);
    return (0);
}

static int
server_sink(dest)
    char *dest;
{
    fd = 0;
    rawmode();
    signal(SIGALRM, onalarm);
    if (send_bytes("\0", 1) < 0)
        return (1);
    recv_all(dest, isdirpath(dest));
    return (0);
}

static void
usage()
{
    fprintf(stderr, "usage: rcp [-P port] host:file ... dir\n");
    fprintf(stderr, "       rcp [-P port] file ... host:dir\n");
    exit(1);
}

/*
 * Flags and operands.  -f and -t are the server modes; the client
 * builds them into the rsh command line, so the parser takes them in
 * the same place it takes -r.
 */
static int
parse_args(argc, argv, args, nargp)
    int argc;
    char **argv;
    char **args;
    int *nargp;
{
    int i, nargs, server;

    nargs = 0;
    server = 0;
    for (i = 1; i < argc; i++) {
        char *a = argv[i];

        if (a[0] == '-' && a[1]) {
            if (a[1] == 'P')
                port = atoi(argv[++i]);
            else if (a[1] == 'r')
                recursive = 1;
            else if (a[1] == 'f')
                server |= 1;
            else if (a[1] == 't')
                server |= 2;
            else
                usage();
        } else if (nargs < 64)
            args[nargs++] = a;
    }
    if (server) {
        if (server == 3 || nargs < 1)
            usage();
        if (server == 1)
            exit(server_fetch(args, nargs));
        exit(server_sink(args[0]));
    }
    if (nargs < 2)
        usage();
    *nargp = nargs;
    return (0);
}

/* the client: resolve the remote, connect, and run the transfer */
static void
client(args, nargs)
    char **args;
    int nargs;
{
    char *dest, *colon, *host, *rdest, hostbuf[64];
    int ip[4], nsrc, j, k, n;
    int get, isdir;

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
    if (get && isdirpath(dest))
        isdir = 1;

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
        recv_all(dest, isdir);
    } else {
        /* the sink sends a NUL first; the remote -t places by basename */
        rx_exact(rbuf, 1);
        for (j = 0; j < nsrc; j++)
            if (send_tree(args[j]) < 0)
                break;
    }

    close(fd);
    exit(0);
}

main(argc, argv)
    int argc;
    char **argv;
{
    char *args[64];
    int nargs;

    parse_args(argc, argv, args, &nargs);
    client(args, nargs);
    return (0);
}
