/*
 * netstat - what the network card's sockets are doing
 *
 * cmd/netstat/netstat.c
 *
 * Opens the card's clone node (/dev/net0) and asks it for its socket
 * table: one nine-byte record per listener or connection - state, local
 * port, peer address, peer port.  The card is the only place that knows
 * every socket, which is why this is one card-level request
 * (include/sys/net.h) and not something the guest driver keeps.
 *
 *	tcp  *.514  *.*  LISTEN
 *	tcp  *.514  127.0.0.1.40001  ESTABLISHED
 *
 * The local address is the port and a star for the interface: the card
 * owns the address and this asks it only about sockets.
 */

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define DEV	  "/dev/net0"

#define NETCARD    0x7414
#define N_SOCKSTAT 0x0C

#define N_TCP_LISTEN 3
#define N_TCP_OPEN   5

#define NETSTATSZ 9

/* one card-level request and its reply (include/sys/net.h) */
struct netcard {
    unsigned char op;
    unsigned char len;
    unsigned char data[1 + 32 + 1 + 63];
};

static void
addr(buf, ip, port)
    char *buf;
    unsigned char *ip;
    int port;
{
    if (port == 0)
	sprintf(buf, "*.*");
    else
	sprintf(buf, "%d.%d.%d.%d.%u", ip[0] & 0xff, ip[1] & 0xff,
	    ip[2] & 0xff, ip[3] & 0xff, port);
}

main()
{
    struct netcard c;
    unsigned char *p;
    char loc[24], rem[24];
    int fd, i, lport, rport;

    fd = open(DEV, 2);
    if (fd < 0) {
	perror(DEV);
	return 1;
    }
    c.op = N_SOCKSTAT;
    c.len = 0;
    if (ioctl(fd, NETCARD, &c) < 0) {
	fprintf(stderr, "netstat: no answer from the card\n");
	close(fd);
	return 1;
    }
    close(fd);

    printf("Proto Local          Foreign         State\n");
    for (i = 0; i + NETSTATSZ <= c.len; i += NETSTATSZ) {
	unsigned char *r = c.data + i;

	if (r[0] != N_TCP_LISTEN && r[0] != N_TCP_OPEN)
	    continue;
	lport = (r[1] << 8) | r[2];
	rport = (r[7] << 8) | r[8];
	sprintf(loc, "*.%u", lport);
	addr(rem, r + 3, rport);
	printf("tcp   %-14s %-15s %s\n", loc, rem,
	    r[0] == N_TCP_LISTEN ? "LISTEN" : "ESTABLISHED");
    }
    return 0;
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
