/*
 * Host-side test of the network card (NET.md's socket face).
 *
 * Drives nic.c the way the guest driver does - writes framed commands
 * byte-by-byte, reads framed replies back, calls nic_poll() the way the
 * d1 wiring's recurring timer does - and checks the two things the guest
 * now depends on: a listener is persistent and pauses while one
 * connection is parked, ACCEPT moves that connection to a slot of its
 * own, and a socket's data is pulled rather than pushed.
 *
 * Not in the d1 link; a standalone host check.  Build and run:
 *     cc -o nic_test nic_test.c nic.c && ./nic_test
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "nic.h"

#define TEST_PORT 23232

/* guest -> card (sys/NET.md section 4) */
#define C_OPEN    0x01
#define C_CLOSE   0x02
#define C_BIND    0x03
#define C_LISTEN  0x05
#define C_ACCEPT  0x0B
#define C_RECV    0x00
#define C_SOCKSTAT 0x0C
#define C_JOIN    0x08

/* card -> guest */
#define P_ACK     0x80
#define P_DATA    0x81
#define P_EVENT   0x82

#define E_AGAIN   7
#define EV_CONNECTED 1
#define EV_READY     4

static int failures;

/* ---- the guest driver's framing, done inline ---- */
static void
tx_frame(unsigned char type, unsigned char sock, const unsigned char *p, int len)
{
    int i;

    nic_data_out(type);
    nic_data_out(sock);
    nic_data_out(len & 0xff);
    nic_data_out((len >> 8) & 0xff);
    for (i = 0; i < len; i++)
        nic_data_out(p[i]);
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

/* a client of the card's listener; -1 if the connect is refused */
static int
client(void)
{
    int fd, r, st = 0;
    socklen_t sl = sizeof st;
    struct sockaddr_in a;

    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        return -1;
    fcntl(fd, F_SETFL, O_NONBLOCK);
    memset(&a, 0, sizeof a);
    a.sin_family = AF_INET;
    a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    a.sin_port = htons(TEST_PORT);
    r = connect(fd, (struct sockaddr *) &a, sizeof a);
    if (r < 0 && errno != EINPROGRESS) {
        close(fd);
        return -1;
    }
    usleep(200000);
    getsockopt(fd, SOL_SOCKET, SO_ERROR, &st, &sl);
    if (st != 0) {
        close(fd);
        return -1;
    }
    return fd;
}

static void
send_str(fd, s)
    int fd;
    char *s;
{
    write(fd, s, strlen(s));
}

int
main(void)
{
    unsigned char type, sock, p[1200];
    unsigned char mode = 1, port[2], cap[2];
    int len, a, b;

    nic_init();

    /* JOIN */
    tx_frame(C_JOIN, 0xff, 0, 0);
    len = rx_frame(&type, &sock, p, sizeof p);
    expect(len == 4 && type == P_ACK && sock == 0xff, "JOIN acks the ip");

    /* open slot 0, bind, listen */
    tx_frame(C_OPEN, 0, &mode, 1);
    len = rx_frame(&type, &sock, p, sizeof p);
    expect(len == 1 && type == P_ACK && p[0] == 0, "OPEN slot 0");
    port[0] = TEST_PORT >> 8;
    port[1] = TEST_PORT & 0xff;
    tx_frame(C_BIND, 0, port, 2);
    len = rx_frame(&type, &sock, p, sizeof p);
    expect(len == 1 && type == P_ACK && p[0] == 0, "BIND");
    tx_frame(C_LISTEN, 0, 0, 0);
    len = rx_frame(&type, &sock, p, sizeof p);
    expect(len == 1 && type == P_ACK && p[0] == 0, "LISTEN");

    /* A connects; the card parks it and says who */
    a = client();
    expect(a >= 0, "client A connects");
    len = rx_frame(&type, &sock, p, sizeof p);
    expect(len == 7 && type == P_EVENT && sock == 0 && p[0] == EV_CONNECTED,
        "EVENT connected A with its address and port");

    /* while it is parked, B is refused */
    b = client();
    expect(b < 0, "B refused while A is pending");

    /* ACCEPT gives A a slot and re-arms the listener */
    tx_frame(C_ACCEPT, 0, 0, 0);
    len = rx_frame(&type, &sock, p, sizeof p);
    expect(len == 2 && type == P_ACK && sock == 0 && p[0] == 0 && p[1] == 1,
        "ACCEPT A -> slot 1");

    b = client();
    expect(b >= 0, "B connects after the listener re-arms");
    len = rx_frame(&type, &sock, p, sizeof p);
    expect(len == 7 && type == P_EVENT && sock == 0 && p[0] == EV_CONNECTED,
        "EVENT connected B");

    tx_frame(C_ACCEPT, 0, 0, 0);
    len = rx_frame(&type, &sock, p, sizeof p);
    expect(len == 2 && type == P_ACK && sock == 0 && p[0] == 0 && p[1] == 2,
        "ACCEPT B -> slot 2");

    /* A's bytes are pulled from slot 1, B's from slot 2 */
    send_str(a, "hello");
    len = rx_frame(&type, &sock, p, sizeof p);
    expect(len == 1 && type == P_EVENT && sock == 1 && p[0] == EV_READY,
        "EVENT ready slot 1");
    cap[0] = 64;
    cap[1] = 0;
    tx_frame(C_RECV, 1, cap, 2);
    len = rx_frame(&type, &sock, p, sizeof p);
    expect(len == 5 && type == P_DATA && sock == 1 && memcmp(p, "hello", 5) == 0,
        "RECV slot 1 -> hello");

    tx_frame(C_RECV, 1, cap, 2);
    len = rx_frame(&type, &sock, p, sizeof p);
    expect(len == 1 && type == P_ACK && sock == 1 && p[0] == E_AGAIN,
        "RECV empty slot 1 -> E_AGAIN");

    send_str(b, "world");
    len = rx_frame(&type, &sock, p, sizeof p);
    expect(len == 1 && type == P_EVENT && sock == 2 && p[0] == EV_READY,
        "EVENT ready slot 2");
    tx_frame(C_RECV, 2, cap, 2);
    len = rx_frame(&type, &sock, p, sizeof p);
    expect(len == 5 && type == P_DATA && sock == 2 && memcmp(p, "world", 5) == 0,
        "RECV slot 2 -> world");

    /* the card's table: one listener and two connections */
    tx_frame(C_SOCKSTAT, 0xff, 0, 0);
    len = rx_frame(&type, &sock, p, sizeof p);
    expect(len == 27 && type == P_ACK && sock == 0xff,
        "SOCKSTAT reports three sockets");
    if (len == 27) {
        expect(p[0] == 3 && p[9] == 5 && p[18] == 5,
            "SOCKSTAT states: LISTEN, two OPEN");
    }

    /* close both connections; each pull then reports the close */
    tx_frame(C_CLOSE, 1, 0, 0);
    len = rx_frame(&type, &sock, p, sizeof p);
    expect(len == 1 && type == P_ACK && sock == 1, "CLOSE slot 1");
    tx_frame(C_CLOSE, 2, 0, 0);
    len = rx_frame(&type, &sock, p, sizeof p);
    expect(len == 1 && type == P_ACK && sock == 2, "CLOSE slot 2");

    close(a);
    close(b);
    if (failures == 0)
        printf("ALL PASS\n");
    else
        printf("%d FAILURES\n", failures);
    return failures != 0;
}
