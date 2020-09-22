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

#include "openx.h"

int i1, o1;
int i2, o2;

#define SIG1    SIGRTMIN+5
#define SIG2    SIGRTMIN+6

sighandler_t 
mysignal(int signum, sighandler_t handler)
{
    return (signal(signum, handler));
}

void 
sig_handler(int signum)
{
    int k;
    char c;

    if (signum == SIG1) {
        ioctl(i1, FIONREAD, &k);
        if (k) {
            read(i1, &c, 1);
            write(o2, &c, 1);
        }
    } else if (signum == SIG2) {
        ioctl(i2, FIONREAD, &k);
        if (k) {
            read(i2, &c, 1);
            write(o1, &c, 1);
        }
    }
}

int
main(int argc, char **argv)
{

    mysignal(SIG1, sig_handler);
    mysignal(SIG2, sig_handler);

    open_terminal("foo1", SIG1, &i1, &o1, 0, 0);
    open_terminal("foo2", SIG2, &i2, &o2, 0, 0);

    while (1) {
        write(o1, "out 1\n", 6);
        write(o2, "out 2\n", 6);
        // the sleep gets interrupted by the signal handler
        sleep(10);
    }

    sleep(10);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
