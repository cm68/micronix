/*
 * rlogind - the remote login server
 *
 * cmd/rlogind/rlogind.c
 *
 * Listens on a tcp socket for the rlogin request
 *
 *	<empty> NUL <luser> NUL <ruser> NUL <term>/<speed> NUL
 *
 * and answers it with the protocol's reply byte - NUL when the caller
 * is trusted, 1 and a message when it is not.  Everything after that
 * byte is the terminal: a login shell on a pty slave, with bytes
 * shuttled between the master and the connection.  There is no command
 * form here.  It is rsh (1)'s other face: with no command it execs
 * rlogin, so a stock rsh never reaches rshd's pty at all - rshd's
 * commandless request serves clients that speak that variant.
 *
 * The caller is checked against /etc/hosts.equiv and the remote
 * user's ~/.rhosts, as rshd does, by address: the card's connected
 * event carries the peer's dotted quad.  Root is never trusted through
 * hosts.equiv, only through its own .rhosts.  There is no password and
 * no encryption.
 *
 * /dev/net0 is a clone node: the open hands back one socket and its
 * descriptor is that socket.  LISTEN is a blocking ioctl - it returns
 * when a connection is waiting, and the daemon then forks.  The child
 * ACCEPTs, which keeps the same descriptor number but swaps the file
 * underneath it to the accepted connection; the parent's descriptor is
 * still the listener, so it goes straight back to LISTEN and a second
 * client is served while the first is still running.  The card refuses
 * further connections while one is parked, so two daemons (rshd and
 * rlogind, say) cannot take each other's.
 *
 * Pairs are scarce, so a session opens one; the child tries them in
 * order and the driver refuses a pair another session holds (EBUSY).
 *
 * The port defaults to 513; the simulator binds host ports for the
 * guest, so a run under d1 uses a high port and rlogin -p.
 */
#include <types.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/signal.h>
#include <sys/socket.h>

#define JOBCONT  02		/* job-control op bits (sys/sys.h) */
#define JOBTTYIN 010
extern int jobctl();

#define RLOGIN_PORT 513
#define REQMAX	256
#define PAYMAX	256		/* the driver moves at most this a call */
#define NPTY	2

static int netfd = -1;
static int master = -1;		/* the pty master, -1 outside a session */
static int child;		/* the shell, 0 outside a session */
static int inetd;		/* run by inetd: the connection is fd 0 */
static int parentpid;
static volatile int reap;

static char fbuf[PAYMAX];
static char req[REQMAX];
static int reqlen;
static char ipstr[18];
static struct in_addr peeraddr;	/* the caller's address */
static char luser[32], ruser[32];

static struct passwd *pw;

onalarm()
{
    signal(SIGALRM, onalarm);   /* V7 semantics: re-arm the handler */
}

onterm()
{
    signal(SIGTERM, onterm);
    reap = 1;
}

/* the child's alarm, for its own select deadlines */
onchildalarm()
{
    signal(SIGALRM, onchildalarm);
}

static int
bit(fd)
    int fd;
{
    int b;

    b = 1;
    while (fd-- > 0)
        b <<= 1;
    return (b);
}

/*
 * One readable chunk.  The driver is non-blocking after select, so an
 * EAGAIN is the selector's answer to "another socket's event"; it is
 * not end of file.
 */
static int
net_read(buf, max)
    char *buf;
    int max;
{
    int r, n;

    for (;;) {
        alarm(1);
        r = select(bit(netfd));
        if (r < 0) {
            if (errno == EINTR) {
                alarm(0);
                continue;
            }
            alarm(0);
            return (-1);
        }
        if (r & bit(netfd)) {
            n = read(netfd, buf, max);
            alarm(0);
            if (n < 0 && errno == EAGAIN)
                continue;
            return (n);
        }
    }
}

/*
 * All of it.  A write moves a chunk and EAGAIN means the card's
 * transmit buffer is full; it drains a hundred times a second.
 */
static int
net_write(buf, len)
    char *buf;
    int len;
{
    int n, sent;

    sent = 0;
    while (sent < len) {
        n = write(netfd, buf + sent, len - sent);
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

/*
 * BSD's rule: hosts.equiv for an ordinary user, the user's own .rhosts
 * always, and never hosts.equiv for root.  The host is the dotted quad
 * the accept returned, so the files name addresses and not names.
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
 * The request's answer (RFC 1282): a NUL byte once the caller is
 * trusted, or a byte of value 1 and a message when it is not.  The
 * client blocks for this byte before it turns its terminal raw, so a
 * server that stays silent is a client that dies with "didn't receive
 * NULL byte from server".
 */
static int
reply_ok()
{
    return (net_write("\0", 1));
}

static int
reply_error(msg)
    char *msg;
{
    char buf[128];
    int len;

    buf[0] = 1;
    len = strlen(msg);
    if (len > (int) sizeof buf - 1)
        len = sizeof buf - 1;
    memcpy(buf + 1, msg, len);
    return (net_write(buf, len + 1));
}

/*
 * Read the four NUL-terminated request fields.  The first is the
 * client's terminal type only when it sends a second one; an empty
 * field is the usual case, and the terminal type and speed ride in the
 * fourth as "<term>/<speed>" with any arguments after further slashes.
 * Data may split the fields anywhere; end of file aborts the read.
 */
static int
read_request()
{
    int n, i, done;

    reqlen = 0;
    done = 0;
    while (done < 4) {
        n = net_read(fbuf, PAYMAX);
        if (n <= 0)
            return (-1);
        for (i = 0; i < n; i++) {
            if (reqlen >= REQMAX)
                return (-1);
            req[reqlen++] = fbuf[i];
            if (fbuf[i] == 0)
                done++;
        }
    }
    {
        char *p;
        int f;

        p = req;
        for (f = 0; f < 4; f++) {
            if (f == 1)
                strncpy(luser, p, sizeof luser - 1);
            else if (f == 2)
                strncpy(ruser, p, sizeof ruser - 1);
            p += strlen(p) + 1;
        }
        luser[sizeof luser - 1] = 0;
        ruser[sizeof ruser - 1] = 0;
    }
    return (0);
}

/*
 * Move bytes both ways until one end is gone.  select is the wait; the
 * alarm is what makes it return when the card has no interrupt line to
 * wake it, and it bounds the wait so a vanished card cannot hang us.
 */
static void
session()
{
    int n, r, mask;

    for (;;) {
        alarm(1);
        mask = bit(netfd) | bit(master);
        r = select(mask);
        if (r < 0) {
            if (errno == EINTR)
                continue;
            break;
        }
        if (r & bit(master)) {
            n = read(master, fbuf, PAYMAX);
            if (n <= 0)
                break;		/* the shell closed the slave */
            if (net_write(fbuf, n) < 0)
                break;
        }
        if (r & bit(netfd)) {
            n = read(netfd, fbuf, PAYMAX);
            if (n < 0) {
                if (errno == EAGAIN)
                    continue;
                break;
            }
            if (n == 0)
                break;		/* the client closed */
            if (write(master, fbuf, n) != n)
                break;		/* the slave is gone */
        }
    }

    /*
     * The client is gone or the shell is.  Closing the master hangs the
     * slave up (SIGHUP to the shell); then the child is killed and
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
    alarm(0);
}

/*
 * Put a login shell on a pty pair and shuttle for it.  The driver
 * refuses a pair another session holds (EBUSY), so trying them in
 * order is the allocation.
 */
static int
start_session()
{
    char mname[16], sname[16];
    int s, pair;

    for (pair = 0; pair < NPTY; pair++) {
        sprintf(mname, "/dev/ptyp%d", pair);
        master = open(mname, 2);
        if (master >= 0)
            break;
        if (errno != EBUSY) {
            perror(mname);
            return (-1);
        }
    }
    if (pair >= NPTY)
        return (-1);
    sprintf(sname, "/dev/ttyp%d", pair);
    child = fork();
    if (child < 0) {
        perror("fork");
        close(master);
        master = -1;
        return (-1);
    }
    if (child == 0) {
        alarm(0);
        s = open(sname, 2);
        if (s < 0) {
            perror(sname);
            exit(1);
        }
        dup2(s, 0);
        dup2(s, 1);
        dup2(s, 2);
        if (s != 0 && s != 1 && s != 2)
            close(s);
        close(master);
        if (netfd > 2)          /* inetd mode: netfd was 0, now the pty */
            close(netfd);
        setuid(pw->uid);
        chdir(pw->dir);
        execl(pw->shell && pw->shell[0] ? pw->shell : "/bin/sh",
            "-sh", 0);
        perror("exec");
        exit(1);
    }

    /*
     * A daemon started from the shell with & is a background job, and
     * its child inherits the no-terminal-input bit: the shell on the
     * slave would read end of file at once.  The parent owns the child
     * (and is root), so it can put it back in the foreground.
     */
    jobctl(child, JOBCONT | JOBTTYIN);
    alarm(0);
    session();
    return (0);
}

/*
 * Exit, telling whoever listens for us.  The standalone daemon forks a
 * session child and the child stops the parent so the next pass reaps
 * it; under inetd the parent is inetd, which reaps the same way.
 */
static void
done(status)
    int status;
{
    if (parentpid)
        kill(parentpid, SIGTERM);
    exit(status);
}

/*
 * The one request this descriptor carries.  The forked child of the
 * standalone daemon and the process inetd runs both come here; the
 * connection is on netfd and peer is the caller's ip[4] and port[2].
 */
static void
serve()
{
    sprintf(ipstr, "%s", inet_ntoa(&peeraddr));
    if (read_request() < 0) {
        close(netfd);
        done(1);
    }
    if (!allowed()) {
        (void) reply_error("Permission denied.\n");
        close(netfd);
        done(1);
    }
    (void) reply_ok();
    if (!inetd)
        fprintf(stderr, "rlogind: %s from %s for %s\n", ruser, ipstr,
            luser);
    if (start_session() < 0) {
        close(netfd);
        done(1);
    }
    close(netfd);
    done(0);
}

main(argc, argv)
    int argc;
    char **argv;
{
    struct sockaddr_in sin;
    int prt, pid, r, st, cfd, slen;

    /*
     * inetd mode: the connection is already on fd 0, and the arguments
     * inetd appended carry the caller's address and its own pid.
     */
    if (argc > 3 && strcmp(argv[1], "-i") == 0) {
        peeraddr.s_addr = inet_addr(argv[2]);
        if (peeraddr.s_addr == INADDR_NONE) {
            fprintf(stderr, "rlogind: bad address %s\n", argv[2]);
            exit(1);
        }
        parentpid = atoi(argv[3]);
        inetd = 1;
        netfd = 0;
        signal(SIGALRM, onalarm);
        signal(SIGTERM, onterm);
        serve();
        exit(0);
    }

    prt = RLOGIN_PORT;
    if (argc > 1)
        prt = atoi(argv[1]);

    parentpid = getpid();
    signal(SIGALRM, onalarm);
    signal(SIGTERM, onterm);

    netfd = socket(AF_INET, SOCK_STREAM, 0);
    if (netfd < 0) {
        perror("rlogind: socket");
        exit(1);
    }
    /* the card wants to be on its network; failure is the card's */
    (void) net_join(netfd);
    sin.sin_family = AF_INET;
    sin.sin_port = htons(prt);
    sin.sin_addr.s_addr = INADDR_ANY;
    if (bind(netfd, (struct sockaddr *) &sin, sizeof sin) < 0) {
        fprintf(stderr, "rlogind: bind %d failed\n", prt);
        exit(1);
    }
    if (listen(netfd, 1) < 0) {
        fprintf(stderr, "rlogind: listen %d failed\n", prt);
        exit(1);
    }

    for (;;) {
        if (reap) {
            reap = 0;
            (void) wait(&st);
        }

        /*
         * A second's alarm bounds the wait: it is what reaps a finished
         * session, and what makes a card without an interrupt line
         * wake the listener.
         */
        alarm(1);
        r = select(bit(netfd));
        alarm(0);
        if (r < 0) {
            if (errno == EINTR)
                continue;
            sleep(1);
            continue;
        }
        if (!(r & bit(netfd)))
            continue;

        /*
         * accept() gives the connection its own descriptor while the
         * listener keeps the one it had; EAGAIN is a frame for another
         * socket and costs no child.
         */
        slen = sizeof sin;
        cfd = accept(netfd, (struct sockaddr *) &sin, &slen);
        if (cfd < 0) {
            if (errno != EAGAIN)
                sleep(1);
            continue;
        }
        peeraddr.s_addr = sin.sin_addr.s_addr;

        pid = fork();
        if (pid == 0) {
            alarm(0);
            signal(SIGALRM, onchildalarm);
            close(netfd);       /* the listener; the parent keeps it */
            netfd = cfd;
            serve();
            exit(1);
        }
        close(cfd);             /* the child has its own copy */
        if (pid < 0) {
            fprintf(stderr, "rlogind: fork failed\n");
            exit(1);
        }
    }
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
