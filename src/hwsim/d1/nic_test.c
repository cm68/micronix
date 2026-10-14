/*
 * Host-side test of the network card (NET.md's socket face).
 *
 * Drives nic.c the way the guest driver will - writes framed commands
 * byte-by-byte, reads framed replies back, calls nic_poll() the way the
 * d1 wiring's recurring timer does - against an echo server this file
 * forks.  Not in the d1 link; a standalone host check.
 *
 * Build and run:
 *     cc -o nic_test nic_test.c nic.c && ./nic_test
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "nic.h"

#define TEST_PORT 23232

static int failures;

/* ---- the echo server, in a child ---- */
static void
echo_server(void)
{
    int lfd, c, on = 1;
    struct sockaddr_in a;
    char buf[512];
    int r;

    lfd = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof on);
    memset(&a, 0, sizeof a);
    a.sin_family = AF_INET;
    a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    a.sin_port = htons(TEST_PORT);
    if (bind(lfd, (struct sockaddr *)&a, sizeof a) < 0 ||
        listen(lfd, 1) < 0)
        _exit(1);
    c = accept(lfd, NULL, NULL);
    if (c < 0)
        _exit(1);
    while ((r = read(c, buf, sizeof buf)) > 0)
        write(c, buf, r);
    close(c);
    close(lfd);
    _exit(0);
}

/* ---- the guest driver's framing, done inline ---- */
static void
tx_byte(unsigned char b)
{
    nic_data_out(b);
}

static void
tx_frame(unsigned char type, unsigned char sock, const unsigned char *p, int len)
{
    int i;

    tx_byte(type);
    tx_byte(sock);
    tx_byte(len & 0xff);
    tx_byte((len >> 8) & 0xff);
    for (i = 0; i < len; i++)
        tx_byte(p[i]);
}

static int
rx_byte(void)
{
    int tries;

    for (tries = 0; tries < 10000; tries++) {
        nic_poll();
        if (nic_status() & NETSTAT_RXRDY)
            return nic_data_in();
        usleep(1000);
    }
    return -1;
}

/* read one frame; returns the payload length, or -1 on timeout */
static int
rx_frame(unsigned char *type, unsigned char *sock, unsigned char *p, int pmax)
{
    int a, b, lo, hi, len, i;

    a = rx_byte();
    if (a < 0) return -1;
    b = rx_byte();
    if (b < 0) return -1;
    lo = rx_byte();
    if (lo < 0) return -1;
    hi = rx_byte();
    if (hi < 0) return -1;
    len = lo | (hi << 8);
    if (len > pmax) return -1;
    for (i = 0; i < len; i++) {
        int c = rx_byte();
        if (c < 0) return -1;
        p[i] = c;
    }
    *type = a;
    *sock = b;
    return len;
}

static void
expect(int ok, const char *what)
{
    if (ok)
        printf("  ok   %s\n", what);
    else {
        printf("  FAIL %s\n", what);
        failures++;
    }
}

int
main(void)
{
    pid_t child;
    unsigned char type, sock, p[1200];
    int len;

    child = fork();
    if (child == 0)
        echo_server();

    sleep(1);                   /* let the server bind */
    nic_init();

    /* JOIN */
    tx_frame(0x08, 0xff, 0, 0);
    len = rx_frame(&type, &sock, p, sizeof p);
    expect(len == 4 && type == 0x80 && sock == 0xff, "JOIN acks the ip");
    printf("       ip %d.%d.%d.%d\n", p[0], p[1], p[2], p[3]);

    /* OPEN tcp */
    {
        unsigned char mode = 1;
        tx_frame(0x01, 0, &mode, 1);
    }
    len = rx_frame(&type, &sock, p, sizeof p);
    expect(len == 1 && type == 0x80 && p[0] == 0, "OPEN tcp");

    /* CONNECT */
    {
        unsigned char c[6] = { 127, 0, 0, 1, TEST_PORT >> 8, TEST_PORT & 0xff };
        tx_frame(0x04, 0, c, 6);
    }
    len = rx_frame(&type, &sock, p, sizeof p);
    expect(len == 1 && type == 0x80 && p[0] == 0, "CONNECT issued");

    /* EVENT connected */
    len = rx_frame(&type, &sock, p, sizeof p);
    expect(len == 1 && type == 0x82 && p[0] == 1, "EVENT connected");

    /* SEND "hello" */
    tx_frame(0x06, 0, (const unsigned char *)"hello", 5);
    len = rx_frame(&type, &sock, p, sizeof p);
    expect(len == 1 && type == 0x80 && p[0] == 0, "SEND accepted");

    /* DATA: the echo */
    len = rx_frame(&type, &sock, p, sizeof p);
    expect(len == 5 && type == 0x81 && memcmp(p, "hello", 5) == 0,
           "DATA echoes hello");

    /* CLOSE */
    tx_frame(0x02, 0, 0, 0);
    len = rx_frame(&type, &sock, p, sizeof p);
    expect(len == 1 && type == 0x80 && p[0] == 0, "CLOSE");

    kill(child, SIGKILL);

    if (failures == 0)
        printf("ALL PASS\n");
    else
        printf("%d FAILURES\n", failures);
    return failures != 0;
}
