/*
 * rshd - the remote shell server
 *
 * cmd/rshd/rshd.c
 *
 * Listens on a tcp socket for the classic rsh request
 *
 *	<stderr-port> NUL <luser> NUL <ruser> NUL <command> NUL
 *
 * checks the caller against /etc/hosts.equiv and the remote user's
 * ~/.rhosts, puts a shell (or the command) on a pty slave, and shuttles
 * bytes between the master and the connection.  The request and the
 * authentication are rcp(1)'s and BSD rsh's; the transport is the NIC's
 * socket face (sys/NET.md), one ioctl per frame.
 *
 * One process owns the connection.  The pty master and /dev/net are
 * waited on with select(2), so there is no reader child and no polling:
 *
 *	master readable -> read a chunk, SEND it (no-wait; its ACK comes
 *			   back through the same frame stream)
 *	net readable	-> one frame: DATA to the master, ACK to the
 *			   back-pressure bookkeeping, EVENT closed/reset
 *			   ends the session
 *
 * A SEND is issued CDB_ASYNC: waiting for its ACK inside the ioctl
 * would let the driver hand an incoming DATA frame to the sender and
 * lose it.  One SEND is in flight at a time; until its ACK arrives the
 * loop does not read the master, which is the pty's own back-pressure.
 * An ACK that says no-memory resends the same chunk.
 *
 * The peer's address comes in the connected EVENT (NET.md section 5);
 * .rhosts matches the dotted quad.  There is no password and no
 * encryption: this is rsh, and it trusts the hosts the files name.
 *
 * The port defaults to 514; the simulator binds host ports for the
 * guest, so a run under d1 uses a high port and rcp -P.
 */
#include <types.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/ioctl.h>
#include <sys/signal.h>

/* the socket face, include/sys/net.h, duplicated as rcp(1) does */
#define NET_OP_RECV 0
#define N_OPEN    0x01
#define N_CLOSE   0x02
#define N_BIND    0x03
#define N_LISTEN  0x05
#define N_SEND    0x06
#define N_JOIN    0x08
#define N_ACK     0x80
#define N_DATA    0x81
#define N_EVENT   0x82
#define N_EV_CONNECTED 1
#define N_EV_CLOSED    2
#define N_EV_RESET     3
#define N_NO_MEMORY    3
#define N_MAXPAY  1024

#define CDB_ASYNC 2

#define JOBCONT  02		/* job-control op bits (sys/sys.h) */
#define JOBTTYIN 010
extern int jobctl();

#define DEVNET	"/dev/net"
#define DEVMASTER "/dev/ptyp0"
#define DEVSLV	"/dev/ttyp0"
#define RSH_PORT 514
#define REQMAX	256

static int netfd;
static int sk;			/* the connection's socket */
static int master = -1;		/* the pty master, -1 outside a session */
static int child;		/* the shell, 0 outside a session */
static int awaiting;		/* one async SEND is in flight */
static char sent[N_MAXPAY];	/* the chunk we may have to resend */
static char fbuf[N_MAXPAY];	/* a frame's payload (static: an ioctl's
				 * data buffer must be in the image, not
				 * a deep stack page - rcp's rbuf) */
static char cbuf[N_MAXPAY];	/* a command's reply */
static int sentlen;

static char req[REQMAX];
static int reqlen;
static char ipstr[18];
static char luser[32], ruser[32], command[REQMAX];

onalarm()
{
    signal(SIGALRM, onalarm);   /* V7 semantics: re-arm the handler */
}

static int bit(fd)
    int fd;
{
    int b;

    b = 1;
    while (fd-- > 0)
        b <<= 1;
    return (b);
}

/*
 * One pushed frame: its type and socket in t and s, payload in buf.
 * Returns the payload length, or -1.
 */
static int
net_frame(t, s, buf, max)
    int *t, *s;
    char *buf;
    int max;
{
    struct cdb r;

    memset((char *) &r, 0, sizeof r);
    r.cmd[0] = NET_OP_RECV;
    r.cmd[1] = 0xff;
    r.len = 4;
    r.count = max;
    r.flags = CDB_IN;
    r.buf = buf;
    r.hole = -1;
    if (ioctl(netfd, CDBCMD, &r) < 0)
        return (-1);
    *t = r.cmd[0] & 0xff;
    *s = r.cmd[1] & 0xff;
    return (r.count);
}

/*
 * Wait for one frame.  select() is the wait; the alarm is what makes it
 * return when the card has no interrupt line to wake it (the driver
 * polls in that case), and it bounds the wait so a vanished card cannot
 * hang the daemon.
 */
static int
wait_frame(t, s, buf, max)
    int *t, *s;
    char *buf;
    int max;
{
    int r;

    for (;;) {
        alarm(1);
        r = select(bit(netfd));
        if (r < 0) {
            if (errno == EINTR)
                continue;
            return (-1);
        }
        if (r & bit(netfd))
            return (net_frame(t, s, buf, max));
    }
}

static int
net_send_async(buf, len)
    char *buf;
    int len;
{
    struct cdb r;

    memset((char *) &r, 0, sizeof r);
    r.cmd[0] = N_SEND;
    r.cmd[1] = sk;
    r.len = 4;
    r.count = len;
    r.flags = CDB_ASYNC;
    r.buf = buf;
    r.hole = -1;
    return (ioctl(netfd, CDBCMD, &r));
}

/*
 * A synchronous command: issue it and wait for its ACK.  A frame that
 * is not the ACK is dropped; the commands here are setup and teardown,
 * where nothing else should be in the stream.
 */
static int
net_cmd(sock, op, p, len)
    int sock, op, len;
    char *p;
{
    struct cdb r;
    char *rep;
    int t, s, n;

    memset((char *) &r, 0, sizeof r);
    r.cmd[0] = op;
    r.cmd[1] = sock;
    r.len = 4;
    r.hole = -1;
    switch (op) {
    case N_OPEN:
        r.cmd[2] = p[0];
        break;
    case N_BIND:
        r.cmd[2] = p[0];
        r.cmd[3] = p[1];
        break;
    }
    rep = cbuf;
    r.count = sizeof cbuf;
    r.flags = CDB_IN;
    r.buf = rep;
    if (ioctl(netfd, CDBCMD, &r) < 0) {
        return (-1);
    }
    t = r.cmd[0] & 0xff;
    s = r.cmd[1] & 0xff;
    for (;;) {
        if (t == N_ACK && s == sock) {
            return (rep[0] & 0xff);
        }
        if (net_frame(&t, &s, rep, N_MAXPAY) < 0)
            return (-1);
    }
}

/*
 * Does path, read line by line, name this host - and, when needuser,
 * this local user after it?  A blank line and one starting with # are
 * not entries; the fields are whitespace separated.
 */
static int
trust(path, needuser)
    char *path;
    int needuser;
{
    FILE *f;
    char line[128];
    char *p, *q;

    f = fopen(path, "r");
    if (f == 0)
        return (0);
    while (fgets(line, sizeof line, f) != 0) {
        p = line;
        while (*p == ' ' || *p == '\t')
            p++;
        if (*p == '#' || *p == '\n' || *p == '\0')
            continue;
        q = p;
        while (*q && *q != ' ' && *q != '\t' && *q != '\n')
            q++;
        if (*q)
            *q++ = 0;
        if (strcmp(p, ipstr) != 0)
            continue;
        if (!needuser) {
            fclose(f);
            return (1);
        }
        while (*q == ' ' || *q == '\t')
            q++;
        if (strncmp(q, luser, strlen(luser)) == 0 &&
            (q[strlen(luser)] == 0 || q[strlen(luser)] == '\n' ||
             q[strlen(luser)] == ' ' || q[strlen(luser)] == '\t')) {
            fclose(f);
            return (1);
        }
    }
    fclose(f);
    return (0);
}

static struct passwd *pw;

/*
 * BSD's rule: hosts.equiv for an ordinary user, the user's own .rhosts
 * always, and never hosts.equiv for root.  The host is the dotted quad
 * from the connected event, so the files name addresses and not names.
 */
static int
allowed()
{
    char path[128];

    pw = getpwnam(ruser);
    if (pw == 0)
        return (0);
    if (pw->uid != 0 && trust("/etc/hosts.equiv", 0))
        return (1);
    if (strcmp(pw->dir, "/") == 0)
        sprintf(path, "/.rhosts");
    else
        sprintf(path, "%s/.rhosts", pw->dir);
    return (trust(path, 1));
}

/*
 * Read the four NUL-terminated request fields.  Data frames may split
 * them anywhere; the event that ends the connection aborts the read.
 */
static int
read_request()
{
    char *buf;
    int t, s, n, i, done;

    buf = fbuf;

    reqlen = 0;
    done = 0;
    while (done < 4) {
        n = wait_frame(&t, &s, buf, N_MAXPAY);
        if (n < 0)
            return (-1);
        if (s != sk || t != N_DATA)
            return (-1);		/* closed, or not a data frame */
        for (i = 0; i < n; i++) {
            if (reqlen >= REQMAX)
                return (-1);
            req[reqlen++] = buf[i];
            if (buf[i] == 0)
                done++;
        }
    }
    /*
     * field 0 is the stderr port, which this server does not open; the
     * child's stderr is the same pty, so a client that reads one stream
     * sees both.
     */
    {
        char *p;
        int f;

        p = req;
        for (f = 0; f < 4; f++) {
            if (f == 1)
                strncpy(luser, p, sizeof luser - 1);
            else if (f == 2)
                strncpy(ruser, p, sizeof ruser - 1);
            else if (f == 3)
                strncpy(command, p, sizeof command - 1);
            p += strlen(p) + 1;
        }
        luser[sizeof luser - 1] = 0;
        ruser[sizeof ruser - 1] = 0;
        command[sizeof command - 1] = 0;
    }
    return (0);
}

/*
 * Put a shell on the slave and hand back the master.  The child opens
 * the slave before it lets go of the master, so the pair exists for the
 * moment that matters; opening the slave is what claims it as the
 * child's controlling tty (sys/pty.c).
 */
static int
start_session()
{
    int s;

    /*
     * The select loop's alarm is per-process and fork copies it: the
     * shell would be killed by SIGALRM a second after it started.
     */
    alarm(0);
    master = open(DEVMASTER, 2);
    if (master < 0)
        return (-1);
    child = fork();
    if (child == 0) {
        alarm(0);
        s = open(DEVSLV, 2);
        if (s < 0)
            exit(1);
        dup2(s, 0);
        dup2(s, 1);
        dup2(s, 2);
        if (s != 0 && s != 1 && s != 2)
            close(s);
        close(master);
        close(netfd);
        setuid(pw->uid);
        chdir(pw->dir);
        if (command[0] == 0)
            execl(pw->shell && pw->shell[0] ? pw->shell : "/bin/sh",
                "-sh", 0);
        else
            execl(pw->shell && pw->shell[0] ? pw->shell : "/bin/sh",
                "sh", "-c", command, 0);
        exit(1);
    }
    if (child < 0) {
        close(master);
        master = -1;
        return (-1);
    }

    /*
     * A daemon started from the shell with & is a background job, and
     * its child inherits the no-terminal-input bit: the shell on the
     * slave would read end of file at once.  The parent owns the child
     * (and is root), so it can put it back in the foreground.
     */
    jobctl(child, JOBCONT | JOBTTYIN);
    return (0);
}

/*
 * Move bytes both ways until one end is gone.  One frame per select
 * pass: the driver's RECV blocks, so reading a second would sleep with
 * the descriptor set already answered.
 */
static void
session()
{
    char *buf;
    int t, s, n, r, mask;

    buf = fbuf;

    awaiting = 0;
    for (;;) {
        alarm(1);			/* a polled card has no other wakeup */
        mask = bit(netfd);
        if (!awaiting)
            mask |= bit(master);
        r = select(mask);
        if (r < 0) {
            if (errno == EINTR)
                continue;
            break;
        }
        if ((r & bit(master)) && !awaiting) {
            n = read(master, buf, N_MAXPAY);
            if (n <= 0)
                break;		/* the shell closed the slave */
            memcpy(sent, buf, n);
            sentlen = n;
            if (net_send_async(sent, sentlen) < 0)
                break;
            awaiting = 1;
        }
        if (r & bit(netfd)) {
            n = wait_frame(&t, &s, buf, N_MAXPAY);
            if (n < 0)
                break;
            if (s != sk)
                continue;
            if (t == N_DATA) {
                if (write(master, buf, n) != n)
                    break;	/* the slave is gone */
                continue;
            }
            if (t == N_ACK) {
                if (!awaiting)
                    continue;
                if (n > 0 && (buf[0] & 0xff) == N_NO_MEMORY) {
                    if (net_send_async(sent, sentlen) < 0)
                        break;
                } else
                    awaiting = 0;
                continue;
            }
            if (t == N_EVENT && (buf[0] == N_EV_CLOSED ||
                                 buf[0] == N_EV_RESET))
                break;
        }
    }

    /*
     * The client is gone or the shell is.  Closing the master hangs
     * the slave up (SIGHUP to the shell); then the child is killed and
     * reaped rather than waited on forever.
     */
    if (master >= 0) {
        close(master);
        master = -1;
    }
    if (child) {
        int st;

        kill(child, SIGKILL);
        wait(&st);
        child = 0;
    }
    net_cmd(sk, N_CLOSE, (char *) 0, 0);
    awaiting = 0;
}

main(argc, argv)
    int argc;
    char **argv;
{
    char *buf;
    char b[2];
    char mode;
    int t, s, n, prt, st;

    buf = fbuf;
    prt = RSH_PORT;
    if (argc > 1)
        prt = atoi(argv[1]);

    netfd = open(DEVNET, 2);
    if (netfd < 0) {
        fprintf(stderr, "rshd: no %s\n", DEVNET);
        exit(1);
    }

    signal(SIGALRM, onalarm);
    /* the card wants to be on its network; failure is the card's */
    (void) net_cmd(0xff, N_JOIN, (char *) 0, 0);

    for (;;) {
        sk = 0;
        mode = 1;			/* tcp */
        st = net_cmd(sk, N_OPEN, &mode, 1);
        if (st) {
            sleep(1);
            continue;
        }
        b[0] = (prt >> 8) & 0xff;
        b[1] = prt & 0xff;
        if (net_cmd(sk, N_BIND, b, 2) || net_cmd(sk, N_LISTEN, (char *) 0, 0)) {
            net_cmd(sk, N_CLOSE, (char *) 0, 0);
            sleep(1);
            continue;
        }

        /*
         * Wait for a connection.  The connected event carries the peer
         * ip[4]; anything else on this socket is not a request yet.
         */
        for (;;) {
            n = wait_frame(&t, &s, buf, N_MAXPAY);
            if (n < 0)
                break;
            if (s != sk)
                continue;
            if (t == N_EVENT) {
                if (buf[0] == N_EV_CONNECTED) {
                    sprintf(ipstr, "%d.%d.%d.%d", buf[1] & 0xff,
                        buf[2] & 0xff, buf[3] & 0xff, buf[4] & 0xff);
                    break;
                }
                n = -1;		/* closed or reset before a request */
                break;
            }
        }
        if (n < 0 || read_request() < 0 || !allowed()) {
            net_cmd(sk, N_CLOSE, (char *) 0, 0);
            continue;
        }

        fprintf(stderr, "rshd: %s from %s for %s\n", ruser, ipstr, luser);
        if (start_session() < 0) {
            net_cmd(sk, N_CLOSE, (char *) 0, 0);
            continue;
        }
        session();
    }
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
