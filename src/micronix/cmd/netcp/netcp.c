/*
 * netcp - copy files, including Micronix network names
 *
 * cmd/netcp/netcp.c
 *
 * A reconstruction of the 1983 1.6 program.  The command line, the
 * source/destination dispatch, the host parsing and the local copy are
 * the recovered behaviour.  The spool record, the 45-byte data records,
 * the six-bit encoding and the '"' queue-name form are the real 1.6
 * formats, recovered by running the distributed netcp under usersim:
 *
 *	S sales /tmp/copy doc /tmp/hello root 600
 *	D!/H5&;L]&+@X69T=W;RMF"
 *	E"
 *
 * The header is kind, remote host, remote path, local host, local path,
 * owner and (for S) mode.  A D line is D + sequence + length, both
 * +0x20, then the data; one record carries at most 45 bytes.  E carries
 * the next sequence number.  The data is six bits per printable
 * character, least-significant bit first.
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

extern char *mktemp(char *);
extern int execl();
extern int fread();

static char host[HOSTMAX + 1];
static char route[64];
static char qname[256];
static int errors;

static void
usage()
{
    fprintf(stderr, "usage: netcp from to\n");
    exit(1);
}

/*
 * A network name is host%path, and the host is lower case letters.  The
 * 1.6 parser stops at the first character that is not a-z and demands
 * that it be the percent.  Return 0 when the name is not one.
 */
static int
netaddr(name, hostp)
    char *name;
    char *hostp;
{
    int i;

    i = 0;
    while (*name >= 'a' && *name <= 'z') {
	if (i >= HOSTMAX)
	    return 0;
	hostp[i++] = *name++;
    }
    hostp[i] = 0;
    if (i == 0 || *name != '%')
	return 0;
    return 1;
}

static int
islocal(name)
    char *name;
{
    return strchr(name, '%') == 0;
}

static char *
skip_host(name)
    char *name;
{
    char *p;

    if (strchr(name, '%') == 0)
	return name;
    for (p = name; *p && *p != '%'; p++)
	;
    return p + 1;
}

/*
 * Look a name up in /etc/netmap, whose first line is this machine and
 * whose following lines start with a directly connected machine followed
 * by the machines reachable through it.  The route is the first word of
 * the line that names the host.
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
    if (f == 0) {
	perror(NETMAP);
	return 0;
    }
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
 * The owner field of the spool record is the login name from /etc/passwd.
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
 * Six bits per printable character, least significant bit first.  A
 * partial group at the end is padded to three bytes with 0xff, which
 * is why a 20 byte record carries 28 characters; the length says how
 * many bytes are real.
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
 * The queue name is host%"c".  The character is a per-request id; the
 * 1.6 program varies it, and the daemon only scans the directory.
 */
static void
queuename(h, out)
    char *h;
    char *out;
{
    long now;
    int c;

    now = time((long *) 0);
    c = (now & 0x3f) + 0x20;
    sprintf(out, "%s%s%%\"%c\"", SPOOL, h, c);
}

static int
send_file(dest, src)
    char *dest;
    char *src;
{
    struct stat st;
    FILE *in, *out;
    unsigned char buf[RECMAX];
    char enc[80];
    char lname[64];
    char owner[32];
    char rpath[256];
    int mode;
    int seq;
    int n;

    if (!netaddr(dest, host) || !netlookup(host, route)) {
	fprintf(stderr, "%s: No such network address\n", dest);
	return 0;
    }
    if (stat(src, &st) < 0) {
	perror(src);
	return 0;
    }
    if ((st.st_mode & S_IFMT) != S_IFREG) {
	fprintf(stderr, "%s: Special file\n", src);
	return 0;
    }
    strcpy(rpath, skip_host(dest));
    localname(lname);
    ownername(owner);
    mode = st.st_mode & 0777;
    /*
     * The queue name is the next hop, not the final host: the 1.6
     * netcp names it with the route out of /etc/netmap.
     */
    queuename(route, qname);
    out = fopen(qname, "w");
    if (out == 0) {
	perror(qname);
	return 0;
    }
    fprintf(out, "S %s %s %s %s %s %o\n",
	host, rpath, lname, src, owner, mode);
    in = fopen(src, "r");
    if (in == 0) {
	perror(src);
	fclose(out);
	unlink(qname);
	return 0;
    }
    seq = 1;
    while ((n = fread((char *) buf, 1, RECMAX, in)) > 0) {
	encode(buf, n, enc);
	fprintf(out, "D%c%c%s\n", seq + 0x20, n + 0x20, enc);
	seq++;
    }
    fclose(in);
    fprintf(out, "E%c\n", seq + 0x20);
    fclose(out);
    return 1;
}

static int
receive_file(dest, src)
    char *dest;
    char *src;
{
    FILE *out;
    char lname[64];
    char owner[32];
    char rpath[256];

    if (!netaddr(src, host) || !netlookup(host, route)) {
	fprintf(stderr, "%s: No such network address\n", src);
	return 0;
    }
    strcpy(rpath, skip_host(src));
    localname(lname);
    ownername(owner);
    queuename(route, qname);
    out = fopen(qname, "w");
    if (out == 0) {
	perror(qname);
	return 0;
    }
    fprintf(out, "R %s %s %s %s %s\n",
	host, rpath, lname, dest, owner);
    fprintf(out, "E!\n");
    fclose(out);
    return 1;
}

static int
local_copy(dest, src)
    char *dest;
    char *src;
{
    int pid;
    int status;

    pid = fork();
    if (pid < 0) {
	perror("fork");
	return 0;
    }
    if (pid == 0) {
	execl("/bin/cp", "cp", src, dest, (char *) 0);
	perror("cp");
	exit(1);
    }
    wait(&status);
    return 1;
}

static int
copy(from, to)
    char *from;
    char *to;
{
    if (!islocal(from))
	return receive_file(to, from);
    if (!islocal(to))
	return send_file(to, from);
    return local_copy(to, from);
}

main(argc, argv)
    int argc;
    char **argv;
{
    int i;

    if (argc < 3)
	usage();
    errors = 0;
    for (i = 1; i < argc - 1; i++)
	if (!copy(argv[i], argv[argc - 1]))
	    errors++;
    return errors ? 1 : 0;
}
