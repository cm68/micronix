/*
 * ftpd - the file transfer server
 *
 * cmd/ftpd/ftpd.c
 *
 * One control connection per process.  The caller signs in with USER
 * and PASS (the machine's accounts are its own business: any password
 * is accepted here, and the file operations run as the user ftpd was
 * started as).  Data goes over a passive connection: PASV opens a
 * listener, and LIST, RETR and STOR accept the client's connection on
 * it, move the bytes, and close it.
 *
 * Commands: USER PASS QUIT SYST PWD CWD CDUP TYPE MODE STRU PASV
 * LIST NLST RETR STOR NOOP ABOR.  PORT is refused - there is no
 * listening socket the client may name.
 *
 * The port defaults to 21; the simulator binds host ports for the
 * guest, so a run under d1 uses a high port.  Under inetd the
 * connection is already on fd 0.
 */
#include <types.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/signal.h>
#include <dirent.h>
#include <sys/dir.h>
#include <sys/socket.h>

#define FTP_PORT 21
#define BUFLEN	512
#define PASV_BASE 20000
#define PASV_TRIES 32

static int netfd = -1;
static int pasvfd = -1;
static int inetd;
static int parentpid;
static volatile int reap;

static char line[BUFLEN];
static char cwd[128];
static char fbuf[BUFLEN];
static struct in_addr peeraddr;

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

static void
sendstr(s, len)
    char *s;
    int len;
{
    int n, sent;

    sent = 0;
    while (sent < len) {
        n = write(netfd, s + sent, len - sent);
        if (n > 0) {
            sent += n;
            continue;
        }
        if (n < 0 && errno == EAGAIN) {
            sleep(1);
            continue;
        }
        return;
    }
}

/*
 * One reply line.  The text is written as it is and CRLF appended.
 */
static void
reply(code, text)
    int code;
    char *text;
{
    char b[128];

    sprintf(b, "%d %s\r\n", code, text);
    sendstr(b, strlen(b));
}

/*
 * Read one control line, without its CRLF.  Returns the length, 0 at
 * end of file.
 */
static int
getline()
{
    int i, n;

    i = 0;
    for (;;) {
        n = read(netfd, &line[i], 1);
        if (n < 0) {
            if (errno == EAGAIN) {
                sleep(1);
                continue;
            }
            return (-1);
        }
        if (n == 0)
            return (0);
        if (line[i] == '\n')
            break;
        if (line[i] != '\r') {
            if (i < BUFLEN - 1)
                i++;
        }
    }
    line[i] = 0;
    return (i);
}

/*
 * 227 Entering Passive Mode (h1,h2,h3,h4,p1,p2).  The address given is
 * the caller's: clients use the control connection's address anyway,
 * and the card has no local address to ask for.
 */
static int
dopasv()
{
    struct sockaddr_in sin;
    char b[128];
    int i, fd, port;

    if (pasvfd >= 0) {
        close(pasvfd);
        pasvfd = -1;
    }
    for (i = 0; i < PASV_TRIES; i++) {
        port = PASV_BASE + (getpid() % 1000) + i;
        fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0)
            break;
        sin.sin_family = AF_INET;
        sin.sin_port = htons(port);
        sin.sin_addr.s_addr = INADDR_ANY;
        if (bind(fd, (struct sockaddr *) &sin, sizeof sin) == 0 &&
            listen(fd, 1) == 0) {
            pasvfd = fd;
            {
                UINT32 ip = ntohl(peeraddr.s_addr);

                sprintf(b, "Entering Passive Mode (%d,%d,%d,%d,%d,%d)",
                    (int) ((ip >> 24) & 0xff),
                    (int) ((ip >> 16) & 0xff),
                    (int) ((ip >> 8) & 0xff),
                    (int) (ip & 0xff),
                    (port >> 8) & 0xff, port & 0xff);
            }
            reply(227, b);
            return (0);
        }
        close(fd);
    }
    reply(425, "Can't open passive connection.");
    return (-1);
}

/*
 * Wait for the client on the passive listener.  A second's alarm
 * bounds it; the deadline is the caller's patience.
 */
static int
data_open()
{
    struct sockaddr_in sin;
    int cfd, slen, i, r;

    if (pasvfd < 0) {
        reply(425, "Use PASV first.");
        return (-1);
    }
    cfd = -1;
    for (i = 0; i < 30 && cfd < 0; i++) {
        alarm(1);
        r = select(bit(pasvfd));
        alarm(0);
        if (r > 0) {
            slen = sizeof sin;
            cfd = accept(pasvfd, (struct sockaddr *) &sin, &slen);
            if (cfd < 0 && errno == EAGAIN)
                continue;
        }
    }
    close(pasvfd);
    pasvfd = -1;
    if (cfd < 0)
        reply(425, "No data connection.");
    return (cfd);
}

static void
data_write(fd, buf, len)
    int fd;
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
        return;
    }
}

static void
dolist(verbose)
    int verbose;
{
    DIR *d;
    struct dir *e;
    char b[160];
    int cfd, n;

    cfd = data_open();
    if (cfd < 0)
        return;
    reply(150, "Here comes the directory listing.");
    d = opendir(cwd);
    if (d == 0) {
        data_write(cfd, "no such directory\r\n", 19);
    } else {
        while ((e = (struct dir *) readdir(d)) != 0) {
            if (strcmp(e->d_name, ".") == 0 ||
                strcmp(e->d_name, "..") == 0)
                continue;
            n = sprintf(b, "%s\r\n", e->d_name);
            data_write(cfd, b, n);
        }
        closedir(d);
    }
    close(cfd);
    reply(226, "Directory send OK.");
}

static void
doretr(name)
    char *name;
{
    int fd, cfd, n;

    cfd = data_open();
    if (cfd < 0)
        return;
    fd = open(name, 0);
    if (fd < 0) {
        reply(550, "No such file.");
        close(cfd);
        return;
    }
    reply(150, "Opening data connection.");
    while ((n = read(fd, fbuf, BUFLEN)) > 0)
        data_write(cfd, fbuf, n);
    close(fd);
    close(cfd);
    reply(226, "Transfer complete.");
}

static void
dostor(name)
    char *name;
{
    int fd, cfd, n;

    cfd = data_open();
    if (cfd < 0)
        return;
    fd = creat(name, 0644);
    if (fd < 0) {
        reply(550, "Can't create file.");
        close(cfd);
        return;
    }
    reply(150, "Ok to send data.");
    for (;;) {
        n = read(cfd, fbuf, BUFLEN);
        if (n == 0)
            break;
        if (n < 0) {
            if (errno == EAGAIN) {
                sleep(1);
                continue;
            }
            break;
        }
        if (write(fd, fbuf, n) != n)
            break;
    }
    close(fd);
    close(cfd);
    reply(226, "Transfer complete.");
}

/*
 * The command line: an uppercase verb and the rest of the line.
 */
static int
verb()
{
    int i;

    for (i = 0; line[i]; i++) {
        if (line[i] == ' ') {
            line[i] = 0;
            return (i + 1);
        }
    }
    return (i);
}

static void
serve()
{
    char *arg;
    int n, i;

    strcpy(cwd, "/");
    reply(220, "micronix ftp ready.");
    for (;;) {
        n = getline();
        if (n <= 0)
            break;
        arg = line + verb();
        for (i = 0; line[i]; i++)
            if (line[i] >= 'a' && line[i] <= 'z')
                line[i] -= 32;

        if (strcmp(line, "USER") == 0) {
            reply(331, "Password required.");
        } else if (strcmp(line, "PASS") == 0) {
            reply(230, "Logged in.");
        } else if (strcmp(line, "QUIT") == 0) {
            reply(221, "Bye.");
            break;
        } else if (strcmp(line, "SYST") == 0) {
            reply(215, "UNIX Type: L8");
        } else if (strcmp(line, "NOOP") == 0) {
            reply(200, "NOOP ok.");
        } else if (strcmp(line, "PWD") == 0 || strcmp(line, "XPWD") == 0) {
            char b[160];

            sprintf(b, "\"%s\"", cwd);
            reply(257, b);
        } else if (strcmp(line, "CWD") == 0 || strcmp(line, "XCWD") == 0) {
            if (*arg == 0)
                reply(550, "CWD needs a path.");
            else if (chdir(arg) < 0)
                reply(550, "No such directory.");
            else {
                if (*arg == '/')
                    strncpy(cwd, arg, sizeof cwd - 1);
                else {
                    if (cwd[strlen(cwd) - 1] != '/')
                        strcat(cwd, "/");
                    strncat(cwd, arg, sizeof cwd - strlen(cwd) - 1);
                }
                cwd[sizeof cwd - 1] = 0;
                reply(250, "CWD ok.");
            }
        } else if (strcmp(line, "CDUP") == 0) {
            chdir("..");
            strcpy(cwd, "/");
            reply(250, "CDUP ok.");
        } else if (strcmp(line, "TYPE") == 0 || strcmp(line, "MODE") == 0 ||
                   strcmp(line, "STRU") == 0) {
            reply(200, "Ok.");
        } else if (strcmp(line, "PASV") == 0) {
            dopasv();
        } else if (strcmp(line, "LIST") == 0) {
            dolist(1);
        } else if (strcmp(line, "NLST") == 0) {
            dolist(0);
        } else if (strcmp(line, "RETR") == 0) {
            if (*arg == 0)
                reply(550, "RETR needs a file.");
            else
                doretr(arg);
        } else if (strcmp(line, "STOR") == 0) {
            if (*arg == 0)
                reply(550, "STOR needs a file.");
            else
                dostor(arg);
        } else if (strcmp(line, "ABOR") == 0) {
            reply(226, "Abort ok.");
        } else if (strcmp(line, "PORT") == 0) {
            reply(502, "PORT is not supported; use PASV.");
        } else {
            reply(500, "Command not understood.");
        }
    }
    close(netfd);
    if (parentpid)
        kill(parentpid, SIGTERM);
    exit(0);
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
            fprintf(stderr, "ftpd: bad address %s\n", argv[2]);
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

    prt = FTP_PORT;
    if (argc > 1)
        prt = atoi(argv[1]);

    parentpid = getpid();
    signal(SIGALRM, onalarm);
    signal(SIGTERM, onterm);

    netfd = socket(AF_INET, SOCK_STREAM, 0);
    if (netfd < 0) {
        perror("ftpd: socket");
        exit(1);
    }
    (void) net_join(netfd);
    sin.sin_family = AF_INET;
    sin.sin_port = htons(prt);
    sin.sin_addr.s_addr = INADDR_ANY;
    if (bind(netfd, (struct sockaddr *) &sin, sizeof sin) < 0) {
        fprintf(stderr, "ftpd: bind %d failed\n", prt);
        exit(1);
    }
    if (listen(netfd, 1) < 0) {
        fprintf(stderr, "ftpd: listen %d failed\n", prt);
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
            fprintf(stderr, "ftpd: fork failed\n");
            exit(1);
        }
    }
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
