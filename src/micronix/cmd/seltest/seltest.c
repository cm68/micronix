#include <types.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/signal.h>
#include <stdlib.h>

static void
onalarm()
{
    signal(SIGALRM, onalarm);
}

static int
bitof(fd)
    int fd;
{
    int b;

    b = 1;
    while (fd-- > 0)
        b <<= 1;
    return (b);
}

main()
{
    int m, s, n, r;
    char buf[64];

    m = open("/dev/ptyp0", 2);
    if (m < 0) {
        printf("no master\n");
        return (1);
    }
    s = open("/dev/ttyp0", 2);
    if (s < 0) {
        printf("no slave\n");
        return (1);
    }

    if (fork() == 0) {
        close(m);
        sleep(1);
        write(s, "ping\n", 5);
        sleep(5);
        return (0);
    }

    printf("selecting...\n");
    r = select(bitof(m) | bitof(s));
    printf("ready %04x\n", r);
    if (r & bitof(m)) {
        n = read(m, buf, sizeof buf);
        printf("master %d: ", n);
        write(1, buf, n);
    }

    write(m, "hi\n", 3);
    r = select(bitof(s));
    printf("slave ready %04x\n", r);
    n = read(s, buf, sizeof buf);
    printf("slave %d: ", n);
    write(1, buf, n);
    /* nothing is readable: an alarm must cut the wait short */
    signal(SIGALRM, onalarm);
    alarm(1);
    r = select(bitof(s));
    printf("eintr %d errno %d\n", r, errno);
    for (n = 0; n < 4; n++) {
        alarm(1);
        r = select(bitof(s));
        printf("again %d: %d %d\n", n, r, errno);
    }
    printf("SELTEST DONE\n");
    return (0);
}
