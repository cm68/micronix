/*
 * inetd - the internet superserver
 *
 * cmd/inetd/inetd.c
 *
 * Reads /etc/inetd.conf, opens one socket per service, binds its port,
 * listens, and selects over the descriptors.  A ready listener is
 * accepted - the connection gets its own descriptor, the listener keeps
 * its own - and the service is forked with the connection on standard
 * input, output and error.  A service that is not being used costs no
 * process, which is the point.
 *
 * A line is
 *
 *	<service> <program> [args...]
 *
 * where <service> is a name in /etc/services (tcp) or a port number.
 * '#' starts a comment.  inetd appends three arguments to the program:
 * -i, the caller's dotted quad, and inetd's pid.  The program takes the
 * connection on fd 0, calls no socket setup of its own, and kills the
 * pid when it is done - there is no SIGCHLD here, so a finished service
 * stops the parent and the next pass calls wait.
 *
 * The card has eight sockets and the listeners hold one each, so keep
 * the file to the services actually run: the rest is budget for
 * connections.
 *
 * The port numbers are the config's, not the protocol's: shell and
 * login on real hardware, anything high under d1.
 */
#include <types.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/signal.h>
#include <sys/socket.h>
#include <netdb.h>

#define CONF	"/etc/inetd.conf"
#define MAXSERV	8
#define MAXARGS	8
#define ARENA	1024

static struct serv {
    int fd;
    UINT16 port;
    char *prog;
    char *arg[MAXARGS + 1];
    int narg;
} servs[MAXSERV];
static int nserv;

static char arena[ARENA];
static int alen;

static int self;		/* our pid, handed to each service */
static char pidstr[8];
static volatile int dead;	/* children that have said they are done */

onalarm()
{
    signal(SIGALRM, onalarm);   /* V7 semantics: re-arm the handler */
}

onterm()
{
    signal(SIGTERM, onterm);
    dead++;
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
 * A name in /etc/services is a port to bind; a number is a number.  A
 * lookup that finds nothing falls back to atoi, which is zero for a
 * name, and a zero port is refused.
 */
static UINT16
portof(name)
    char *name;
{
    struct servent *sp;

    sp = getservbyname(name, "tcp");
    if (sp && sp->s_port)
        return (ntohs(sp->s_port));
    return (atoi(name));
}

/*
 * Read the service lines.  The text lives in one arena and the token
 * pointers point into it, so nothing is copied again and nothing is
 * freed at exit.
 */
static void
readconf()
{
    FILE *f;
    char line[128];
    char *p, *q, *r, *tok[2 + MAXARGS];
    int n, i;
    struct serv *s;

    f = fopen(CONF, "r");
    if (f == 0) {
        perror(CONF);
        exit(1);
    }
    while (fgets(line, sizeof line, f) != 0) {
        p = line;
        while (*p == ' ' || *p == '\t')
            p++;
        if (*p == '#' || *p == '\n' || *p == 0)
            continue;
        if (nserv >= MAXSERV) {
            fprintf(stderr, "inetd: too many services\n");
            exit(1);
        }
        if (alen + (int) strlen(p) + 1 > ARENA) {
            fprintf(stderr, "inetd: config too large\n");
            exit(1);
        }
        q = arena + alen;
        strcpy(q, p);
        alen += strlen(q) + 1;
        for (r = q; *r; r++)
            if (*r == '\n')
                *r = 0;

        n = 0;
        r = q;
        while (*r) {
            while (*r == ' ' || *r == '\t')
                r++;
            if (*r == 0)
                break;
            if (n >= 2 + MAXARGS)
                break;
            tok[n++] = r;
            while (*r && *r != ' ' && *r != '\t')
                r++;
            if (*r)
                *r++ = 0;
        }
        if (n < 2)
            continue;
        s = &servs[nserv];
        s->port = portof(tok[0]);
        if (s->port == 0) {
            fprintf(stderr, "inetd: bad service %s\n", tok[0]);
            exit(1);
        }
        s->prog = tok[1];
        s->narg = n - 2;
        for (i = 0; i < s->narg; i++)
            s->arg[i] = tok[2 + i];
        s->arg[s->narg] = 0;
        nserv++;
    }
    fclose(f);
    if (nserv == 0) {
        fprintf(stderr, "inetd: no services in %s\n", CONF);
        exit(1);
    }
}

main(argc, argv)
    int argc;
    char **argv;
{
    struct sockaddr_in sin;
    char ipstr[18], *av[3 + MAXARGS + 4];
    int i, j, r, st, cfd, pid, slen;
    char **p, *ps;
    struct serv *s;

    readconf();

    self = getpid();
    sprintf(pidstr, "%d", self);
    signal(SIGALRM, onalarm);
    signal(SIGTERM, onterm);

    for (i = 0; i < nserv; i++) {
        s = &servs[i];
        s->fd = socket(AF_INET, SOCK_STREAM, 0);
        if (s->fd < 0) {
            perror("inetd: socket");
            exit(1);
        }
        if (i == 0)
            (void) net_join(s->fd);
        sin.sin_family = AF_INET;
        sin.sin_port = htons(s->port);
        sin.sin_addr.s_addr = INADDR_ANY;
        if (bind(s->fd, (struct sockaddr *) &sin, sizeof sin) < 0) {
            fprintf(stderr, "inetd: bind %d failed\n", s->port);
            exit(1);
        }
        if (listen(s->fd, 1) < 0) {
            fprintf(stderr, "inetd: listen %d failed\n", s->port);
            exit(1);
        }
    }

    for (;;) {
        while (dead > 0) {
            dead--;
            (void) wait(&st);
        }

        r = 0;
        for (i = 0; i < nserv; i++)
            r |= bit(servs[i].fd);

        /*
         * A second's alarm bounds the wait: it reaps, and it wakes a
         * selector when the card has no interrupt line.
         */
        alarm(1);
        r = select(r);
        alarm(0);
        if (r < 0) {
            if (errno == EINTR)
                continue;
            sleep(1);
            continue;
        }

        for (i = 0; i < nserv; i++) {
            s = &servs[i];
            if (!(r & bit(s->fd)))
                continue;

            /*
             * accept() gives the connection its own descriptor; the
             * listener keeps its own, and EAGAIN is a frame for
             * another socket and costs no child.
             */
            slen = sizeof sin;
            cfd = accept(s->fd, (struct sockaddr *) &sin, &slen);
            if (cfd < 0) {
                if (errno != EAGAIN)
                    sleep(1);
                continue;
            }
            sprintf(ipstr, "%s", inet_ntoa(&sin.sin_addr));

            pid = fork();
            if (pid == 0) {
                alarm(0);
                dup2(cfd, 0);
                dup2(cfd, 1);
                dup2(cfd, 2);
                if (cfd > 2)
                    close(cfd);
                for (j = 0; j < nserv; j++)
                    if (servs[j].fd > 2)
                        close(servs[j].fd);
                p = av;
                ps = s->prog;
                *p++ = ps;
                for (j = 0; j < s->narg; j++) {
                    ps = s->arg[j];
                    *p++ = ps;
                }
                ps = "-i";
                *p++ = ps;
                ps = ipstr;
                *p++ = ps;
                ps = pidstr;
                *p++ = ps;
                *p = 0;
                execv(s->prog, av);
                fprintf(stderr, "inetd: exec %s failed\n", s->prog);
                (void) kill(self, SIGTERM);
                exit(1);
            }
            close(cfd);         /* the child has its own copy */
        }
    }
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
