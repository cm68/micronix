/*
 * netmail - write a network mail record
 *
 * cmd/netmail/netmail.c
 *
 * A reconstruction of the 1.6 program.  mail(1) finds a network name
 * (machine%user), execs this program with that address, and leaves the
 * message on standard input.
 *
 * A reconstruction of the 1.6 program.  The record is
 *
 *	M <rhost> <ruser> <lhost> <luser>
 *	D<seq><len><data>
 *	...
 *	E<next>
 *
 * with the same six-bit encoding and 45 byte records as netcp.  The
 * header is record zero; the character after the record letter is the
 * number.  The file is written under a TEMP name in the outbound spool
 * and linked to <route>%<codes> when it is complete, so the daemon
 * never sees a partial record.
 *
 * vim: tabstop=4 shiftwidth=4 noexpandtab:
 */

#include <types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/fs.h>
#include <sys/stat.h>

#define SPOOL	"/usr/spool/net/out/"
#define NETMAP	"/etc/netmap"
#define PASSWD	"/etc/passwd"
#define HOSTMAX	31
#define RECMAX	45

static char host[HOSTMAX + 1];
static char route[64];
static int names;

static void
usage()
{
    fprintf(stderr, "usage: netmail \"machine%%user\"\n");
    exit(1);
}

/*
 * The address is host%user.  The host is lower case letters, as in
 * netcp; return a pointer to the user part.
 */
static char *
netaddr(name)
    char *name;
{
    int i;

    i = 0;
    while (*name >= 'a' && *name <= 'z') {
	if (i >= HOSTMAX)
	    return 0;
	host[i++] = *name++;
    }
    host[i] = 0;
    if (i == 0 || *name != '%')
	return 0;
    return name + 1;
}

/*
 * The first line of /etc/netmap is this machine's name.
 */
static void
localname(buf)
    char *buf;
{
    FILE *f;
    char *p;

    strcpy(buf, "local");
    f = fopen(NETMAP, "r");
    if (f == 0)
	return;
    if (fgets(buf, 64, f)) {
	p = buf;
	while (*p && *p != ' ' && *p != '\t' && *p != '\n')
	    p++;
	*p = 0;
    }
    fclose(f);
}

/*
 * The user's login name, from /etc/passwd.
 */
static void
ownername(buf)
    char *buf;
{
    FILE *f;
    char line[256];
    char *p;
    char *name;
    char *uidp;
    int uid;

    strcpy(buf, "root");
    uid = getuid();
    f = fopen(PASSWD, "r");
    if (f == 0)
	return;
    while (fgets(line, sizeof line, f)) {
	p = line;
	name = p;
	while (*p && *p != ':')
	    p++;
	if (*p == 0)
	    continue;
	*p++ = 0;
	while (*p && *p != ':')
	    p++;
	if (*p == 0)
	    continue;
	*p++ = 0;
	uidp = p;
	while (*p && *p != ':')
	    p++;
	*p = 0;
	if (atoi(uidp) == uid) {
	    strcpy(buf, name);
	    break;
	}
    }
    fclose(f);
}

/*
 * The route to a machine: the first word of the /etc/netmap line that
 * names it.
 */
static int
netlookup(name, routep)
    char *name;
    char *routep;
{
    FILE *f;
    char line[256];
    char *p;
    char *rt;
    char *tok;
    int found;

    f = fopen(NETMAP, "r");
    if (f == 0)
	return 0;
    found = 0;
    while (!found && fgets(line, sizeof line, f)) {
	p = line;
	while (*p == ' ' || *p == '\t')
	    p++;
	if (*p == 0 || *p == '\n')
	    continue;
	rt = p;
	while (*p && *p != ' ' && *p != '\t' && *p != '\n')
	    p++;
	if (*p)
	    *p++ = 0;
	if (strcmp(rt, name) == 0) {
	    strcpy(routep, rt);
	    found = 1;
	    break;
	}
	while (*p) {
	    while (*p == ' ' || *p == '\t')
		p++;
	    if (*p == 0 || *p == '\n')
		break;
	    tok = p;
	    while (*p && *p != ' ' && *p != '\t' && *p != '\n')
		p++;
	    if (*p)
		*p++ = 0;
	    if (strcmp(tok, name) == 0) {
		strcpy(routep, rt);
		found = 1;
		break;
	    }
	}
    }
    fclose(f);
    return found;
}

/*
 * Six bits per printable character, least significant bit first.  A
 * partial group at the end is padded with 0xff bytes, as the stock
 * programs do; the length field says how many bytes are real.
 */
static void
encode(in, len, out)
    unsigned char *in;
    int len;
    char *out;
{
    int i, j, o;
    unsigned long bits;

    o = 0;
    for (i = 0; i < len; i += 3) {
	bits = in[i];
	bits |= (unsigned long) (i + 1 < len ? in[i + 1] : 0xff) << 8;
	bits |= (unsigned long) (i + 2 < len ? in[i + 2] : 0xff) << 16;
	for (j = 0; j < 4; j++)
	    out[o++] = ((bits >> (6 * j)) & 0x3f) + 0x20;
    }
    out[o] = 0;
}

/*
 * A spool name is dir+base, a percent sign, and three unique six-bit
 * characters: the process, the time, and a counter.  A slash is
 * changed to 0 so the name cannot leave the directory.
 */
static void
spoolname(dir, base, out)
    char *dir;
    char *base;
    char *out;
{
    int c0, c1, c2;
    long now;

    now = time((long *) 0);
    c0 = (getpid() & 0x3f) + 0x20;
    c1 = (now & 0x3f) + 0x20;
    c2 = ((++names) & 0x3f) + 0x20;
    if (c0 == '/')
	c0 = '0';
    if (c1 == '/')
	c1 = '0';
    if (c2 == '/')
	c2 = '0';
    sprintf(out, "%s%s%%%c%c%c", dir, base, c0, c1, c2);
}

static int
send(addr)
    char *addr;
{
    FILE *out;
    char temp[256];
    char final[256];
    char ruser[64];
    char lname[64];
    char owner[32];
    char enc[80];
    unsigned char buf[RECMAX];
    char *user;
    int seq;
    int n;

    user = netaddr(addr);
    if (user == 0 || *user == 0)
	usage();
    if (!netlookup(host, route)) {
	fprintf(stderr, "%s: No such network address\n", addr);
	return 0;
    }
    if (strlen(user) >= sizeof ruser)
	return 0;
    strcpy(ruser, user);
    localname(lname);
    ownername(owner);

    spoolname(SPOOL, "TEMP", temp);
    out = fopen(temp, "w");
    if (out == 0) {
	perror(temp);
	return 0;
    }
    fprintf(out, "M %s %s %s %s\n", host, ruser, lname, owner);
    seq = 1;
    while ((n = fread((char *) buf, 1, RECMAX, stdin)) > 0) {
	encode(buf, n, enc);
	fprintf(out, "D%c%c%s\n", seq + 0x20, n + 0x20, enc);
	seq++;
    }
    fprintf(out, "E%c\n", seq + 0x20);
    fclose(out);

    spoolname(SPOOL, route, final);
    if (link(temp, final) < 0) {
	perror(final);
	unlink(temp);
	return 0;
    }
    unlink(temp);
    return 1;
}

main(argc, argv)
    int argc;
    char **argv;
{
    if (argc != 2)
	usage();
    return send(argv[1]) ? 0 : 1;
}
