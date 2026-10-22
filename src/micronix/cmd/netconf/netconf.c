/*
 * netconf - read or write the network card's wifi configuration
 *
 * cmd/netconf/netconf.c
 *
 * The third provisioning path in sys/NET.md section 4: GETCONF and
 * SETCONF are card-level operations, socket 0xff, on /dev/net - the
 * network face of the ND module, one ioctl command block per operation
 * (include/sys/ioctl.h and sys/net.h).
 *
 *	netconf			print the stored ssid and password
 *	netconf <ssid> [<pass>]	store a pair (no pass: an open network)
 *
 * The operation numbers and reply types mirror include/sys/net.h, which
 * this program cannot see (it builds against lib/include, not the
 * kernel's include).
 */

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define DEV	  "/dev/net"
#define N_MAXPAY  1024

#define N_GETCONF 0x09
#define N_SETCONF 0x0A
#define N_ACK	  0x80

#define N_SSIDMAX 32
#define N_PASSMAX 63

static int fd;

/* read frames until the card-level ACK, keeping the last one in r */
static int
net_ack(r, buf, max)
    struct cdb *r;
    char *buf;
    int max;
{
    struct cdb q;

    while ((r->cmd[0] & 0xff) != N_ACK || (r->cmd[1] & 0xff) != 0xff) {
	memset((char *) &q, 0, sizeof q);
	q.cmd[0] = 0;			/* NET_OP_RECV */
	q.cmd[1] = 0xff;
	q.len = 4;
	q.count = max;
	q.flags = max ? CDB_IN : 0;
	q.buf = buf;
	q.hole = -1;
	if (ioctl(fd, CDBCMD, &q) < 0)
	    return -1;
	memcpy((char *) r, (char *) &q, sizeof q);
    }
    return 0;
}

static int
getconf()
{
    struct cdb r;
    char p[N_MAXPAY];
    int n, sl, pl;

    memset((char *) &r, 0, sizeof r);
    r.cmd[0] = N_GETCONF;
    r.cmd[1] = 0xff;
    r.len = 4;
    r.count = sizeof p;
    r.flags = CDB_IN;
    r.buf = p;
    r.hole = -1;
    if (ioctl(fd, CDBCMD, &r) < 0)
	return -1;
    if (net_ack(&r, p, sizeof p) < 0)
	return -1;
    n = r.count;
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
    struct cdb r;
    char p[1 + N_SSIDMAX + 1 + N_PASSMAX];
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

    memset((char *) &r, 0, sizeof r);
    r.cmd[0] = N_SETCONF;
    r.cmd[1] = 0xff;
    r.len = 4;
    r.count = n;
    r.flags = 0;
    r.buf = p;
    r.hole = -1;
    if (ioctl(fd, CDBCMD, &r) < 0 || net_ack(&r, (char *)0, 0) < 0) {
	fprintf(stderr, "netconf: no answer from the card\n");
	return 1;
    }
    st = r.cmd[2] & 0xff;
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
