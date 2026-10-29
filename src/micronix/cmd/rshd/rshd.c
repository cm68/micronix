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
 * ~/.rhosts.  An interactive request (no command) gets a login shell on
 * a pty slave, with bytes shuttled between the master and the
 * connection; a request with a command gets the connection itself as
 * its stdin, stdout and stderr, and no pty.
 *
 * /dev/net0 is a clone node: the open hands back one socket and its
 * descriptor is that socket.  LISTEN is a blocking ioctl - it returns
 * when a connection is waiting, and the daemon then forks.  The child
 * ACCEPTs, which keeps the same descriptor number but swaps the file
 * underneath it to the accepted connection; the parent's descriptor is
 * still the listener, so it goes straight back to LISTEN and a second
 * client is served while the first is still running.  The card refuses
 * further connections while one is parked, so two daemons (rshd and
 * telnetd, say) cannot take each other's.
 *
 * Pairs are scarce, so only an interactive session opens one; the
 * child tries them in order and the driver refuses a pair another
 * session holds (EBUSY).
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

/* the socket face, include/sys/net.h, duplicated as the other commands do */
#define NETBIND    0x7410
#define NETCONNECT 0x7411
#define NETLISTEN  0x7412
#define NETACCEPT  0x7413
#define NETCARD    0x7414

#define N_JOIN    0x08

#define JOBCONT  02		/* job-control op bits (sys/sys.h) */
#define JOBTTYIN 010
extern int jobctl();

#define DEVNET	"/dev/net0"
#define RSH_PORT 514
#define REQMAX	256
#define PAYMAX	256		/* the driver moves at most this a call */
#define NPTY	2

/* one card-level request and its reply (include/sys/net.h) */
struct netcard {
    unsigned char op;
    unsigned char len;
    unsigned char data[1 + 32 + 1 + 63];
};

static int netfd = -1;
static int master = -1;		/* the pty master, -1 outside a session */
static int child;		/* the shell, 0 outside a session */
static int parentpid;
static volatile int reap;

static char fbuf[PAYMAX];
static char req[REQMAX];
static int reqlen;
static char ipstr[18];
static char luser[32], ruser[32], command[REQMAX];

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
 * One card-level request (JOIN, the configuration pair, the socket
 * table).  Returns the reply length, or -1.
 */
static int
net_card(op, data, len)
    int op, len;
    char *data;
{
    struct netcard c;
    int i, n;

    c.op = op;
    c.len = len;
    for (i = 0; i < len; i++)
        c.data[i] = data[i];
    if (ioctl(netfd, NETCARD, &c) < 0)
        return (-1);
    n = c.len;
    for (i = 0; i < n; i++)
        data[i] = c.data[i];
    return (n);
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
 * Read the four NUL-terminated request fields.  Data may split them
 * anywhere; end of file aborts the read.
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
 * Move bytes both ways until one end is gone.  select is the wait; the
 * alarm is what makes it return when the card has no interrupt line to
 * wake it, and it bounds the wait so a vanished card cannot hang us.
 */
static void
session()
{
    int t, n, r, mask;

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
 * order is the allocation; only an interactive request comes here, so
 * a pty is spent by a login and not by a command.
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
        close(netfd);
        setuid(pw->uid);
        chdir(pw->dir);
        execl(pw->shell && pw->shell[0] ? pw->shell : "/bin/sh",
            "-sh", 0);
        perror("exec");
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
    alarm(0);
    session();
    return (0);
}

/*
 * A request with a command runs on the connection itself: the socket
 * is the command's stdin, stdout and stderr and no pty is spent.  The
 * session child forks it and waits, so the daemon still reaps one child
 * per SIGTERM.
 */
static int
run_command()
{
    int pid, st;

    pid = fork();
    if (pid == 0) {
        alarm(0);
        dup2(netfd, 0);
        dup2(netfd, 1);
        dup2(netfd, 2);
        if (netfd > 2)
            close(netfd);
        setuid(pw->uid);
        chdir(pw->dir);
        execl(pw->shell && pw->shell[0] ? pw->shell : "/bin/sh",
            "sh", "-c", command, 0);
        exit(1);
    }
    if (pid < 0)
        return (-1);
    wait(&st);
    return (0);
}

main(argc, argv)
    int argc;
    char **argv;
{
    char peer[6];
    int prt, pid, r, st, n;

    prt = RSH_PORT;
    if (argc > 1)
        prt = atoi(argv[1]);

    netfd = open(DEVNET, 2);
    if (netfd < 0) {
        perror(DEVNET);
        exit(1);
    }

    parentpid = getpid();
    signal(SIGALRM, onalarm);
    signal(SIGTERM, onterm);

    /* the card wants to be on its network; failure is the card's */
    (void) net_card(N_JOIN, peer, 0);

    peer[0] = (prt >> 8) & 0xff;
    peer[1] = prt & 0xff;
    if (ioctl(netfd, NETBIND, peer) < 0) {
        fprintf(stderr, "rshd: bind %d failed\n", prt);
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
        r = ioctl(netfd, NETLISTEN, 0);
        alarm(0);
        if (r < 0) {
            if (errno == EINTR)
                continue;
            sleep(1);
            continue;
        }

        pid = fork();
        if (pid == 0) {
            alarm(0);
            signal(SIGALRM, onchildalarm);
            n = ioctl(netfd, NETACCEPT, peer);
            if (n < 0) {
                close(netfd);
                kill(parentpid, SIGTERM);
                exit(1);
            }
            sprintf(ipstr, "%d.%d.%d.%d", peer[0] & 0xff, peer[1] & 0xff,
                peer[2] & 0xff, peer[3] & 0xff);
            if (read_request() < 0 || !allowed()) {
                close(netfd);
                kill(parentpid, SIGTERM);
                exit(1);
            }
            fprintf(stderr, "rshd: %s from %s for %s\n", ruser, ipstr, luser);
            if (command[0] != 0) {
                if (run_command() < 0) {
                    close(netfd);
                    kill(parentpid, SIGTERM);
                    exit(1);
                }
            } else if (start_session() < 0) {
                close(netfd);
                kill(parentpid, SIGTERM);
                exit(1);
            }
            close(netfd);
            kill(parentpid, SIGTERM);
            exit(0);
        }
        if (pid < 0) {
            /*
             * No child to serve it.  The pending connection blocks the
             * card's one slot, so there is no honest way to carry on:
             * stop and let the machine's init start us again.
             */
            fprintf(stderr, "rshd: fork failed\n");
            exit(1);
        }
    }
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
