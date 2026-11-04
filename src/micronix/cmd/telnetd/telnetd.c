/*
 * telnetd - the telnet server
 *
 * cmd/telnetd/telnetd.c
 *
 * Listens on the telnet port (or takes fd 0 from inetd) and puts a
 * login on a pty: the connection is a byte stream in both directions,
 * with the telnet protocol's IAC sequences filtered out of the client's
 * stream and doubled out of the server's.  Option negotiation is
 * refused (WONT/DONT), which is enough for a terminal session.
 *
 * The login is /bin/login; it asks for the account and password over
 * the pty.  If login cannot be run, the login shell is started instead.
 *
 * The port defaults to 23; the simulator binds host ports for the
 * guest, so a run under d1 uses a high port.
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

#define TELNET_PORT 23
#define PAYMAX	256
#define NPTY	2

/* telnet protocol */
#define IAC	255
#define DONT	254
#define DO	253
#define WONT	252
#define WILL	251
#define SB	250
#define SE	240

/* the parser's states */
#define TS_DATA	0
#define TS_IAC	1
#define TS_OPT	2
#define TS_SB	3
#define TS_SBIAC	4

static int netfd = -1;
static int master = -1;
static int child;
static int inetd;
static int parentpid;
static volatile int reap;

static char fbuf[PAYMAX];
static struct in_addr peeraddr;

static struct passwd *pw;

onalarm()
{
    signal(SIGALRM, onalarm);
}

onterm()
{
    signal(SIGTERM, onterm);
    reap = 1;
}

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
 * One byte to the client, with IAC doubled.  A terminal stream is not
 * line oriented, so this is a write per byte on that side.
 */
static void
outbyte(c)
    int c;
{
    char b[2];

    b[0] = c;
    if (c == IAC) {
        b[1] = IAC;
        (void) write(netfd, b, 2);
    } else
        (void) write(netfd, b, 1);
}

/*
 * A three-byte negotiation frame: IAC <cmd> <opt>.
 */
static void
outcmd(cmd, opt)
    int cmd, opt;
{
    char b[3];

    b[0] = IAC;
    b[1] = cmd;
    b[2] = opt;
    (void) write(netfd, b, 3);
}

/*
 * The client's bytes, with the protocol taken out.  A negotiation is
 * refused: WILL gets DONT and DO gets WONT, which leaves the terminal
 * in its plain NVT mode.
 */
static void
shell_in(n)
    int n;
{
    static int st = TS_DATA;
    static int cmd;
    int i, c;

    for (i = 0; i < n; i++) {
        c = fbuf[i] & 0xff;
        switch (st) {
        case TS_DATA:
            if (c == IAC)
                st = TS_IAC;
            else
                (void) write(master, &fbuf[i], 1);
            break;
        case TS_IAC:
            if (c == IAC) {
                (void) write(master, &fbuf[i], 1);
                st = TS_DATA;
            } else if (c == WILL || c == WONT || c == DO || c == DONT) {
                cmd = c;
                st = TS_OPT;
            } else if (c == SB)
                st = TS_SB;
            else
                st = TS_DATA;
            break;
        case TS_OPT:
            if (cmd == WILL)
                outcmd(DONT, c);
            else if (cmd == DO)
                outcmd(WONT, c);
            st = TS_DATA;
            break;
        case TS_SB:
            if (c == IAC)
                st = TS_SBIAC;
            break;
        case TS_SBIAC:
            st = (c == SE) ? TS_DATA : TS_SB;
            break;
        }
    }
}

/*
 * Move bytes both ways until one end is gone.
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
                break;
            {
                int i;

                for (i = 0; i < n; i++)
                    outbyte(fbuf[i] & 0xff);
            }
        }
        if (r & bit(netfd)) {
            n = read(netfd, fbuf, PAYMAX);
            if (n < 0) {
                if (errno == EAGAIN)
                    continue;
                break;
            }
            if (n == 0)
                break;
            shell_in(n);
        }
    }

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
 * A login on a pty pair.  login reads the account and password from
 * the slave; a machine without one falls back to the login shell.
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
        if (errno != EBUSY)
            return (-1);
    }
    if (pair >= NPTY)
        return (-1);
    sprintf(sname, "/dev/ttyp%d", pair);
    child = fork();
    if (child < 0) {
        close(master);
        master = -1;
        return (-1);
    }
    if (child == 0) {
        alarm(0);
        s = open(sname, 2);
        if (s < 0)
            exit(1);
        dup2(s, 0);
        dup2(s, 1);
        dup2(s, 2);
        if (s > 2)
            close(s);
        close(master);
        if (netfd > 2)          /* inetd mode: netfd was 0, now the pty */
            close(netfd);
        execl("/bin/login", "login", 0);
        execl("/bin/sh", "-sh", 0);
        exit(1);
    }
    jobctl(child, JOBCONT | JOBTTYIN);
    alarm(0);
    session();
    return (0);
}

static void
done(status)
    int status;
{
    if (parentpid)
        kill(parentpid, SIGTERM);
    exit(status);
}

/*
 * The connection is already accepted: one login on it.  inetd mode
 * passes the caller's address and its own pid; standalone mode has
 * both in the accept.
 */
static void
serve()
{
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

    if (argc > 3 && strcmp(argv[1], "-i") == 0) {
        peeraddr.s_addr = inet_addr(argv[2]);
        if (peeraddr.s_addr == INADDR_NONE) {
            fprintf(stderr, "telnetd: bad address %s\n", argv[2]);
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

    prt = TELNET_PORT;
    if (argc > 1)
        prt = atoi(argv[1]);

    parentpid = getpid();
    signal(SIGALRM, onalarm);
    signal(SIGTERM, onterm);

    netfd = socket(AF_INET, SOCK_STREAM, 0);
    if (netfd < 0) {
        perror("telnetd: socket");
        exit(1);
    }
    (void) net_join(netfd);
    sin.sin_family = AF_INET;
    sin.sin_port = htons(prt);
    sin.sin_addr.s_addr = INADDR_ANY;
    if (bind(netfd, (struct sockaddr *) &sin, sizeof sin) < 0) {
        fprintf(stderr, "telnetd: bind %d failed\n", prt);
        exit(1);
    }
    if (listen(netfd, 1) < 0) {
        fprintf(stderr, "telnetd: listen %d failed\n", prt);
        exit(1);
    }

    for (;;) {
        if (reap) {
            reap = 0;
            (void) wait(&st);
        }
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
            close(netfd);
            netfd = cfd;
            serve();
            exit(1);
        }
        close(cfd);
        if (pid < 0) {
            fprintf(stderr, "telnetd: fork failed\n");
            exit(1);
        }
    }
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
