/*
 * netconf - read or write the network card's wifi configuration
 *
 * cmd/netconf/netconf.c
 *
 * The third provisioning path in sys/NET.md section 4: GETCONF and
 * SETCONF are card-level operations on /dev/net0, the card's socket
 * clone, through one NETCARD ioctl (include/sys/net.h).
 *
 *	netconf			print the stored ssid and password
 *	netconf <ssid> [<pass>]	store a pair (no pass: an open network)
 *
 * The request numbers and payload layout mirror include/sys/net.h,
 * which this program cannot see (it builds against lib/include, not the
 * kernel's include).
 */

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define DEV	  "/dev/net0"

#define NETCARD   0x7414

#define N_GETCONF 0x09
#define N_SETCONF 0x0A

#define N_SSIDMAX 32
#define N_PASSMAX 63

#define N_CONFMAX (1 + N_SSIDMAX + 1 + N_PASSMAX)

/* one card-level request and its reply (include/sys/net.h) */
struct netcard {
    unsigned char op;
    unsigned char len;
    unsigned char data[N_CONFMAX];
};

static int fd;

static int
net_card(op, data, len)
    int op, len;
    unsigned char *data;
{
    struct netcard c;
    int i, n;

    c.op = op;
    c.len = len;
    for (i = 0; i < len; i++)
	c.data[i] = data[i];
    if (ioctl(fd, NETCARD, &c) < 0)
	return -1;
    n = c.len;
    for (i = 0; i < n; i++)
	data[i] = c.data[i];
    return n;
}

static int
getconf()
{
    unsigned char p[N_CONFMAX];
    int n, sl, pl;

    n = net_card(N_GETCONF, p, 0);
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
    unsigned char p[N_CONFMAX];
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

    n = net_card(N_SETCONF, p, n);
    if (n < 1) {
	fprintf(stderr, "netconf: no answer from the card\n");
	return 1;
    }
    st = p[0] & 0xff;
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

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
