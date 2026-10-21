/*
 * netconf - read or write the network card's wifi configuration
 *
 * cmd/netconf/netconf.c
 *
 * The third provisioning path in sys/NET.md section 4: GETCONF and
 * SETCONF are card-level frames, socket 0xff, on the same /dev/net byte
 * channel rcp drives.  The pair lives in the card's non-volatile memory;
 * SETCONF makes it reconnect with the new network at once.
 *
 *	netconf			print the stored ssid and password
 *	netconf <ssid> [<pass>]	store a pair (no pass: an open network)
 *
 * The frame types mirror include/sys/net.h, which this program cannot
 * see (it builds against lib/include, not the kernel's include).
 */

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#define DEV	  "/dev/net"
#define N_MAXPAY  1024

#define N_GETCONF 0x09
#define N_SETCONF 0x0A
#define N_ACK	  0x80

#define N_SSIDMAX 32
#define N_PASSMAX 63

static int fd;
static int rtype, rsock;

/* ---- frame io, the same shape as rcp's ---- */

static void
net_tx(type, sk, p, len)
    int type, sk, len;
    char *p;
{
    char hdr[4];

    hdr[0] = type;
    hdr[1] = sk;
    hdr[2] = len & 0xff;
    hdr[3] = (len >> 8) & 0xff;
    write(fd, hdr, 4);
    if (len)
	write(fd, p, len);
}

static int
net_rx(typep, sockp, buf, max)
    int *typep, *sockp, max;
    char *buf;
{
    char r[4 + N_MAXPAY];
    int n, len, i;

    n = read(fd, r, sizeof r);
    if (n < 4)
	return -1;
    *typep = r[0] & 0xff;
    *sockp = r[1] & 0xff;
    len = (r[2] & 0xff) | ((r[3] & 0xff) << 8);
    if (len > n - 4)
	len = n - 4;
    if (len > max)
	len = max;
    for (i = 0; i < len; i++)
	buf[i] = r[4 + i];
    return len;
}

/* read frames until the card-level ACK */
static int
net_ack(buf, max)
    char *buf;
    int max;
{
    int n;

    for (;;) {
	n = net_rx(&rtype, &rsock, buf, max);
	if (n < 0)
	    return -1;
	if (rtype == N_ACK && rsock == 0xff)
	    return n;
    }
}

static int
getconf()
{
    char p[N_MAXPAY];
    int n, sl, pl;

    net_tx(N_GETCONF, 0xff, (char *)0, 0);
    n = net_ack(p, sizeof p);
    if (n < 2)
	return -1;

    sl = p[0] & 0xff;
    if (sl > N_SSIDMAX || n < 1 + sl + 1)
	return -1;
    pl = p[1 + sl] & 0xff;
    if (pl > N_PASSMAX || n < 1 + sl + 1 + pl)
	return -1;

    printf("ssid: ");
    if (sl)
	fwrite(p + 1, 1, sl, stdout);
    else
	printf("(none)");
    printf("\npassword: ");
    fwrite(p + 2 + sl, 1, pl, stdout);
    printf("\n");
    return 0;
}

static int
setconf(ssid, pass)
    char *ssid, *pass;
{
    char p[1 + N_SSIDMAX + 1 + N_PASSMAX];
    char a[N_MAXPAY];
    int sl, pl, n, st;

    sl = strlen(ssid);
    pl = strlen(pass);
    if (sl < 1 || sl > N_SSIDMAX) {
	fprintf(stderr, "netconf: ssid must be 1-%d characters\n", N_SSIDMAX);
	return 1;
    }
    if (pl > N_PASSMAX) {
	fprintf(stderr, "netconf: password is over %d characters\n", N_PASSMAX);
	return 1;
    }

    n = 0;
    p[n++] = sl;
    memcpy(p + n, ssid, sl);
    n += sl;
    p[n++] = pl;
    memcpy(p + n, pass, pl);
    n += pl;

    net_tx(N_SETCONF, 0xff, p, n);
    n = net_ack(a, sizeof a);
    if (n < 1) {
	fprintf(stderr, "netconf: no answer from the card\n");
	return 1;
    }
    st = a[0] & 0xff;
    if (st != 0) {
	fprintf(stderr, "netconf: the card refused, status %d\n", st);
	return 1;
    }
    printf("netconf: saved\n");
    return 0;
}

static void
usage()
{
    fprintf(stderr, "usage: netconf\n");
    fprintf(stderr, "       netconf <ssid> [<pass>]\n");
    exit(1);
}

main(argc, argv)
    int argc;
    char **argv;
{
    int r;

    if (argc > 3)
	usage();
    fd = open(DEV, 2);
    if (fd < 0) {
	perror(DEV);
	return 1;
    }
    if (argc == 1)
	r = getconf();
    else
	r = setconf(argv[1], argc == 3 ? argv[2] : "");
    close(fd);
    if (r < 0) {
	fprintf(stderr, "netconf: the card did not answer properly\n");
	return 1;
    }
    return r;
}

/* vim: tabstop=4 shiftwidth=4 noexpandtab: */
