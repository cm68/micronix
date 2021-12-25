/*
 * child terminal window for the simulator
 *
 * lib/openx.c
 * Changed: <2021-12-23 15:44:00 curt>
 */
#define	_GNU_SOURCE

#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <stdlib.h>
#include <string.h>

static int ntermpids;
static int *termpids;

static void
kill_terminal_children()
{
    int i;

    for (i = 0; i < ntermpids ; i++) {
        kill(termpids[i], SIGTERM);
    }
}

/*
 * SIGIO seems to be busted with pty's, so I am going to cheat outrageously:
 * i fork 2 child processes., one for the xterm and 1 for a poller process
 */
void
open_terminal(char *name, int signum, int *in_p, int *out_p, int cooked, char *logfile)
{
    char *cmd = malloc(100);
    int infd;
    int outfd;
    int pipe_up[2];
    int pipe_down[2];
    int pid;
    char *ptyname;
    int mypid = getpid();
    int i;
    struct termios tio;
    int in;
    int out;
    char c;
    fd_set readfds;
    int fds;
    char *args[30];
    char **argp;
    char *fontsize=getenv("TERMINAL_FONTSIZE");
    char *geometry=getenv("TERMINAL_GEOMETRY");
    if (!geometry) {
        geometry = "120x60";
    }
    if (!fontsize) {
        fontsize = "12";
    }
    pipe(pipe_up);

    // we need to capture the tty name so we can send output to it
    sprintf(cmd,
        "bash -c 'tty > /proc/%d/fd/%d ; while test -d /proc/%d ; do sleep 1 ; done ; sleep 60'",
        mypid, pipe_up[1], mypid);
    pid = fork();
    if (!pid) {     // xterm child
#ifdef notdef
        // try terminals in order of preference
        execlp("xfce4-terminal", "xfce4-terminal", 
            "-T", name,
            "--disable-server",
            "--command", cmd, 
            (char *) 0);

        execlp("mate-terminal", "mate-terminal", 
            "-t", name,
            "--sm-client-disable",
            "--disable-factory",
            "--command", cmd, 
            (char *) 0);
#endif
        if (logfile) {
            unlink(logfile);
        }
        
        argp = args;
        *argp++ = "xterm";
        *argp++ = "-T";
        *argp++ = name;
        *argp++ = "-geometry"; 
        *argp++ = geometry; 
        *argp++ = "-fa"; 
        *argp++ = "Monospace"; 
        *argp++ = "-fs"; 
        *argp++ = fontsize;
        if (logfile) {
            *argp++ = "-l"; 
            *argp++ = "-lf"; 
            *argp++ = logfile;
        }
        *argp++ = "-e"; 
        *argp++ = "bash"; 
        *argp++ = "-c"; 
        *argp++ = cmd;
        *argp++ = (char *) 0;
        execvp("xterm", args);
    }
    ptyname = malloc(100);
    i = read(pipe_up[0], ptyname, 100);
    if (i == -1) {
        perror("pipe");
    }
    close(pipe_up[0]);
    close(pipe_up[1]);

    // remember to kill the xterm
    termpids = realloc(termpids, sizeof(int) * ++ntermpids);
    termpids[ntermpids - 1] = pid;

    pipe(pipe_up);
    pipe(pipe_down);

    pid = fork();
    if (!pid) {     // polling child
        // build a filename, null terminated at the newline
        ptyname[i] = 0;
        for (i = 0; i < strlen(ptyname); i++) {
            if (ptyname[i] == '\n') {
                ptyname[i] = 0;
                break;
            }
        }
        in = open(ptyname, O_RDWR);
        if (!cooked) {
            tcgetattr(in, &tio);
            cfmakeraw(&tio);
            tcsetattr(in, TCSANOW, &tio);
        }
        fds = pipe_down[0];
        if (fds < in) fds = in;
        fds++;

        while (1) {
            FD_ZERO(&readfds);
            FD_SET(in, &readfds);
            FD_SET(pipe_down[0], &readfds);
            select(fds, &readfds, 0, 0, 0);
            ioctl(in, FIONREAD, &i);
            if (i) {
                read(in, &c, 1);
                write(pipe_up[1], &c, 1);
                if (signum) {
                    kill(mypid, signum);
                }
            }
            ioctl(pipe_down[0], FIONREAD, &i);
            if (i) {
                read(pipe_down[0], &c, 1);
                write(in, &c, 1);
            }
        }
    }

    // remember to kill the poller
    termpids = realloc(termpids, sizeof(int) * ++ntermpids);
    termpids[ntermpids - 1] = pid;

    *in_p = pipe_up[0];
    *out_p = pipe_down[1];
}

__attribute__((constructor))
void
terminal_cleanup_setup()
{
    atexit(kill_terminal_children);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
