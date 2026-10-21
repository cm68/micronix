/*
 * netdaemon - the Micronix network daemon
 *
 * cmd/netdaemon/netdaemon.c
 *
 * A reconstruction of the 1983 1.6 program.  The daemon loop, the two
 * spool directories, the netmap route lookup and the ttys port table
 * are the recovered structure.  The spool records and the line protocol
 * are the real 1.6 formats, recovered by running two copies of the
 * distributed daemon under usersim with their serial ports bridged:
 *
 *	P!			proposal
 *	A!			accept
 *	F			file follows
 *	Y			yes
 *	S sales /tmp/copy doc /tmp/hello root 600
 *	D!/H5&;L]&+@X69T=W;RMF"
 *	E"
 *
 * Every line on the wire is three checksum characters, then the line,
 * then a newline.  The checksum is over the line and its newline: for
 * each byte, add it and rotate the 16-bit sum left one place.  The
 * three characters are bits 0-5, 6-11 and 12-17 of that sum, each with
 * 0x20 added.  The sender then waits for a Y after F, after the S/R
 * header, and after every D record; the E record is not acknowledged.
 *
 * An R request is store-and-forward: the machine that holds the file
 * copies it into the outbound spool as an S record, and an intermediate
 * machine copies the request, or the S record, into its own spool for
 * the next hop.  network.4's multihop network and netact's strings
 * ("netcp", "/bin/netcp", "cp fails") point this way, but the 1.6
 * daemon's own R handling was not captured; this is an inference.
 *
 * The daemon forks one reader per port in the 1.6 binary.  This
 * reconstruction keeps one process and polls each port with a one
 * second alarm, which is the same line protocol on a simpler frame.
 *
 * vim: tabstop=4 shiftwidth=4 noexpandtab:
 */

#include <types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/fs.h>
#include <sys/stat.h>
#include <sys/dir.h>
#include <dirent.h>

#define SPOOLIN	 "/usr/spool/net/in/"
#define SPOOLOUT "/usr/spool/net/out/"
#define NETMAP	 "/etc/netmap"
#define PASSWD	 "/etc/passwd"
#define TTYS	 "/etc/ttys"
#define MAXPORTS 16
#define LINEMAX	 512

extern DIR *opendir();
extern struct direct *readdir();
extern int closedir();
extern int (*signal())();
extern int fread();

struct port {
    char dev[32];
    char remote[32];
    int fd;
};

static struct port ports[MAXPORTS];
static int nports;
static int debug;
static volatile int timedout;

static int relay_s();

static void
onalarm(sig)
    int sig;
{
    timedout = 1;
}

static void
strip(line)
    char *line;
{
    int n;

    n = strlen(line);
    while (n > 0 && (line[n - 1] == '\n' || line[n - 1] == '\r'))
	line[--n] = 0;
}

/*
 * /etc/netmap: first line is this machine, each following line starts
 * with a directly connected machine and names the machines reached
 * through it.  Return the first word of the line naming the host.
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

static int
read_netmap()
{
    FILE *f;
    char line[256];

    f = fopen(NETMAP, "r");
    if (f == 0) {
	perror(NETMAP);
	return 0;
    }
    while (fgets(line, sizeof line, f))
	;
    fclose(f);
    return 1;
}

/*
 * /etc/ttys: "ttyD 1200 net-sales" makes /dev/ttyD the link to sales.
 */
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
 * The owner field of a record is the login name from /etc/passwd.
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
 * Six bits per printable character, least significant bit first; a
 * partial block is padded to three bytes.
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
	if (i + 1 < len)
	    bits |= (unsigned long) in[i + 1] << 8;
	if (i + 2 < len)
	    bits |= (unsigned long) in[i + 2] << 16;
	for (j = 0; j < 4; j++)
	    out[o++] = ((bits >> (6 * j)) & 0x3f) + 0x20;
    }
    out[o] = 0;
}

static int
read_ttys()
{
    FILE *f;
    char line[256];
    char *p;
    char *dev;
    char *net;
    char path[64];
    int i;

    f = fopen(TTYS, "r");
    if (f == 0) {
	perror(TTYS);
	return 0;
    }
    nports = 0;
    while (fgets(line, sizeof line, f)) {
	p = line;
	while (*p == ' ' || *p == '\t')
	    p++;
	if (*p == 0 || *p == '\n' || *p == '#')
	    continue;
	dev = p;
	while (*p && *p != ' ' && *p != '\t' && *p != '\n')
	    p++;
	if (*p == 0 || *p == '\n')
	    continue;
	*p++ = 0;
	/*
	 * The rest of the words are the baud rate and the net- name, in
	 * either order; find net-.
	 */
	net = 0;
	while (*p) {
	    while (*p == ' ' || *p == '\t')
		p++;
	    if (*p == 0 || *p == '\n')
		break;
	    if (strncmp(p, "net-", 4) == 0) {
		net = p + 4;
		break;
	    }
	    while (*p && *p != ' ' && *p != '\t' && *p != '\n')
		p++;
	}
	if (net == 0 || nports >= MAXPORTS)
	    continue;
	i = 0;
	while (*net && *net != ' ' && *net != '\t' && *net != '\n' &&
	    i < (int) sizeof ports[nports].remote - 1)
	    ports[nports].remote[i++] = *net++;
	ports[nports].remote[i] = 0;
	strcpy(ports[nports].dev, "/dev/");
	strcat(ports[nports].dev, dev);
	ports[nports].fd = open(ports[nports].dev, 2);
	if (ports[nports].fd < 0) {
	    perror(ports[nports].dev);
	    continue;
	}
	/*
	 * The 1.6 daemon puts a newline on the line when it opens it;
	 * both ends of the captured pair did.
	 */
	write(ports[nports].fd, "\n", 1);
	nports++;
    }
    fclose(f);
    return nports > 0;
}

static struct port *
find_port(route)
    char *route;
{
    int i;

    for (i = 0; i < nports; i++)
	if (strcmp(ports[i].remote, route) == 0)
	    return &ports[i];
    return 0;
}

/*
 * The line checksum: add each byte, including the newline, then rotate
 * the 16-bit sum left.
 */
static unsigned short
cksum(s)
    char *s;
{
    unsigned short sum;
    unsigned char c;

    sum = 0;
    for (;;) {
	c = *s++;
	if (c == 0)
	    break;
	sum += c;
	sum = (sum << 1) | (sum >> 15);
    }
    sum += '\n';
    sum = (sum << 1) | (sum >> 15);
    return sum;
}

static int
wire_write(p, s)
    struct port *p;
    char *s;
{
    unsigned short sum;
    char hdr[3];

    sum = cksum(s);
    hdr[0] = (sum & 0x3f) + 0x20;
    hdr[1] = ((sum >> 6) & 0x3f) + 0x20;
    hdr[2] = ((sum >> 12) & 0x3f) + 0x20;
    if (write(p->fd, hdr, 3) != 3)
	return 0;
    if (write(p->fd, s, strlen(s)) != strlen(s))
	return 0;
    if (write(p->fd, "\n", 1) != 1)
	return 0;
    return 1;
}

/*
 * Read one line, verify its checksum, and leave the payload in buf.
 * The alarm gives the read a timeout; a signal interrupts it.
 */
static int
wire_read(p, buf, size, secs)
    struct port *p;
    char *buf;
    int size;
    int secs;
{
    int n, r, i;
    char c;
    unsigned short sum;
    char hdr[4];

    n = 0;
    timedout = 0;
    /*
     * Old signal() resets the handler when it fires, so put it back
     * before every alarm.
     */
    signal(14, onalarm);
    alarm(secs);
    while (n < size - 1) {
	r = read(p->fd, &c, 1);
	if (r <= 0) {
	    alarm(0);
	    return 0;
	}
	buf[n++] = c;
	if (c == '\n')
	    break;
    }
    alarm(0);
    if (n < 4 || buf[n - 1] != '\n')
	return 0;
    buf[n - 1] = 0;
    sum = cksum(buf + 3);
    hdr[0] = (sum & 0x3f) + 0x20;
    hdr[1] = ((sum >> 6) & 0x3f) + 0x20;
    hdr[2] = ((sum >> 12) & 0x3f) + 0x20;
    if (buf[0] != hdr[0] || buf[1] != hdr[1] || buf[2] != hdr[2])
	return 0;
    for (i = 3; buf[i]; i++)
	buf[i - 3] = buf[i];
    buf[i - 3] = 0;
    return 1;
}

static int
expect(p, want, secs)
    struct port *p;
    char *want;
    int secs;
{
    char buf[LINEMAX];

    if (!wire_read(p, buf, sizeof buf, secs))
	return 0;
    return strcmp(buf, want) == 0;
}

/*
 * The nth whitespace-separated field of a line, 0 based.
 */
static int
field(line, n, buf, max)
    char *line;
    int n;
    char *buf;
    int max;
{
    char *p;
    char *s;
    int f;
    int i;

    p = line;
    f = 0;
    while (*p) {
	while (*p == ' ' || *p == '\t')
	    p++;
	if (*p == 0 || *p == '\n')
	    break;
	s = p;
	while (*p && *p != ' ' && *p != '\t' && *p != '\n')
	    p++;
	if (f == n) {
	    i = 0;
	    while (s < p && i < max - 1)
		buf[i++] = *s++;
	    buf[i] = 0;
	    return 1;
	}
	f++;
    }
    return 0;
}

static int
octal(s)
    char *s;
{
    int v;

    v = 0;
    while (*s >= '0' && *s <= '7')
	v = v * 8 + (*s++ - '0');
    return v;
}

/*
 * Decode one D record's payload into the output file.
 */
static int
decode(s, len, out)
    char *s;
    int len;
    FILE *out;
{
    int i, j, n;
    int v[4];
    unsigned long bits;

    n = 0;
    for (i = 0; s[i] && s[i + 1] && s[i + 2] && s[i + 3] && n < len; i += 4) {
	for (j = 0; j < 4; j++)
	    v[j] = ((unsigned char) s[i + j] - 0x20) & 0x3f;
	bits = v[0] | ((unsigned long) v[1] << 6) |
	    ((unsigned long) v[2] << 12) | ((unsigned long) v[3] << 18);
	for (j = 0; j < 3 && n < len; j++) {
	    fputc((bits >> (8 * j)) & 0xff, out);
	    n++;
	}
    }
    return n == len;
}

/*
 * Receive an S transfer after its header has been read and acked.
 */
static int
receive_s(p, header)
    struct port *p;
    char *header;
{
    char line[LINEMAX];
    char host[64];
    char ourhost[64];
    char path[256];
    char mode[32];
    FILE *out;
    int len;

    /*
     * The first field is the destination machine.  If it is not this
     * one, copy the record into the spool and let scan_out forward it.
     */
    localname(ourhost);
    if (!field(header, 1, host, sizeof host))
	return 0;
    if (strcmp(host, ourhost) != 0)
	return relay_s(p, header);

    /*
     * The remote path is the destination on this machine; the local
     * path is where it was on the sender.
     */
    if (!field(header, 2, path, sizeof path))
	return 0;
    out = fopen(path, "w");
    if (out == 0) {
	perror(path);
	return 0;
    }
    for (;;) {
	if (!wire_read(p, line, sizeof line, 5)) {
	    fclose(out);
	    return 0;
	}
	if (line[0] == 'D') {
	    len = ((unsigned char) line[2] - 0x20) & 0x3f;
	    decode(line + 3, len, out);
	    wire_write(p, "Y");
	    continue;
	}
	if (line[0] == 'E')
	    break;
    }
    fclose(out);
    if (field(header, 6, mode, sizeof mode))
	chmod(path, octal(mode));
    if (debug)
	fprintf(stderr, "received %s\n", path);
    return 1;
}

/*
 * The queue name in the outbound spool is host%"c", the same shape
 * netcp writes.
 */
static void
queuename(h, out)
    char *h;
    char *out;
{
    int now;
    int c;

    now = time((long *) 0);
    c = (now & 0x3f) + 0x20;
    sprintf(out, "%s%s%%\"%c\"", SPOOLOUT, h, c);
}

/*
 * An R request at the machine that holds the file: copy it into the
 * outbound spool as an S record for the requester, and let scan_out
 * forward it.  The requester's host and destination path are fields 3
 * and 4; the file is field 2 on this machine.
 */
static int
copy_r_to_s(header)
    char *header;
{
    char reqfile[256];
    char desthost[64];
    char dest[256];
    char owner[32];
    char ourhost[64];
    char q[256];
    char sline[LINEMAX];
    char dline[LINEMAX];
    char enc[80];
    unsigned char buf[45];
    struct stat st;
    FILE *in, *out;
    int seq, n, mode;

    if (!field(header, 2, reqfile, sizeof reqfile))
	return 0;
    if (!field(header, 3, desthost, sizeof desthost))
	return 0;
    if (!field(header, 4, dest, sizeof dest))
	return 0;
    if (stat(reqfile, &st) < 0 || (st.st_mode & S_IFMT) != S_IFREG)
	return 0;
    in = fopen(reqfile, "r");
    if (in == 0)
	return 0;
    localname(ourhost);
    ownername(owner);
    mode = st.st_mode & 0777;
    queuename(desthost, q);
    out = fopen(q, "w");
    if (out == 0) {
	fclose(in);
	return 0;
    }
    sprintf(sline, "S %s %s %s %s %s %o",
	desthost, dest, ourhost, reqfile, owner, mode);
    fprintf(out, "%s\n", sline);
    seq = 1;
    while ((n = fread((char *) buf, 1, 45, in)) > 0) {
	encode(buf, n, enc);
	fprintf(out, "D%c%c%s\n", seq + 0x20, n + 0x20, enc);
	seq++;
    }
    fprintf(out, "E%c\n", seq + 0x20);
    fclose(in);
    fclose(out);
    return 1;
}

/*
 * An R request at a machine that does not hold the file: copy the
 * request into the outbound spool so scan_out forwards it toward the
 * host named in its first field.
 */
static int
relay_r(header)
    char *header;
{
    char host[64];
    char q[256];
    FILE *out;

    if (!field(header, 1, host, sizeof host))
	return 0;
    queuename(host, q);
    out = fopen(q, "w");
    if (out == 0)
	return 0;
    fprintf(out, "%s\nE!\n", header);
    fclose(out);
    return 1;
}

/*
 * An S record at a machine that is not its destination: copy the
 * header and every D record into the outbound spool for the next hop.
 */
static int
relay_s(p, header)
    struct port *p;
    char *header;
{
    char host[64];
    char q[256];
    char line[LINEMAX];
    FILE *out;

    if (!field(header, 1, host, sizeof host))
	return 0;
    queuename(host, q);
    out = fopen(q, "w");
    if (out == 0)
	return 0;
    fprintf(out, "%s\n", header);
    for (;;) {
	if (!wire_read(p, line, sizeof line, 5)) {
	    fclose(out);
	    return 0;
	}
	if (line[0] == 'D') {
	    fprintf(out, "%s\n", line);
	    wire_write(p, "Y");
	    continue;
	}
	if (line[0] == 'E') {
	    fprintf(out, "%s\n", line);
	    break;
	}
    }
    fclose(out);
    return 1;
}

/*
 * Accept a transfer: P! then F then the S or R header.  The sender waits
 * for Y after F and after the header, and after each D record.
 */
static int
accept_link(p)
    struct port *p;
{
    char line[LINEMAX];
    char rheader[LINEMAX];

    if (!wire_read(p, line, sizeof line, 1))
	return 0;
    if (strcmp(line, "P!"))
	return 0;
    wire_write(p, "A!");
    if (!expect(p, "F", 5))
	return 0;
    wire_write(p, "Y");
    if (!wire_read(p, line, sizeof line, 5))
	return 0;
    wire_write(p, "Y");
    if (line[0] == 'S')
	return receive_s(p, line);
    if (line[0] == 'R') {
	char reqhost[64];
	char ourhost[64];

	/*
	 * The request names the machine that holds the file in its
	 * first field.  Here, copy the file into an S entry; anywhere
	 * else, copy the request into the spool and forward it.
	 */
	strcpy(rheader, line);
	wire_read(p, line, sizeof line, 5);	/* E, no ack */
	localname(ourhost);
	if (field(rheader, 1, reqhost, sizeof reqhost) &&
	    strcmp(reqhost, ourhost) == 0)
	    return copy_r_to_s(rheader);
	return relay_r(rheader);
    }
    return 0;
}

/*
 * Send one spool entry: P!, A!, F, Y, then the S or R header, its Y,
 * every D with a Y, and the E alone.
 */
static int
send_spool(p, path)
    struct port *p;
    char *path;
{
    FILE *f;
    char line[LINEMAX];

    f = fopen(path, "r");
    if (f == 0)
	return 0;
    if (!wire_write(p, "P!") || !expect(p, "A!", 5)) {
	fclose(f);
	return 0;
    }
    if (!wire_write(p, "F") || !expect(p, "Y", 5)) {
	fclose(f);
	return 0;
    }
    while (fgets(line, sizeof line, f)) {
	strip(line);
	if (line[0] == 0)
	    continue;
	if (line[0] == 'E') {
	    wire_write(p, line);
	    break;
	}
	if (!wire_write(p, line) || !expect(p, "Y", 5)) {
	    fclose(f);
	    return 0;
	}
    }
    fclose(f);
    return 1;
}

static int
scan_out()
{
    DIR *d;
    struct direct *dp;
    struct port *p;
    char path[256];
    char header[256];
    char host[64];
    char route[64];
    FILE *f;

    d = opendir(SPOOLOUT);
    if (d == 0)
	return 0;
    while ((dp = readdir(d)) != 0) {
	if (dp->name[0] == '.')
	    continue;
	strcpy(path, SPOOLOUT);
	strcat(path, dp->name);
	f = fopen(path, "r");
	if (f == 0)
	    continue;
	if (fgets(header, sizeof header, f) == 0) {
	    fclose(f);
	    continue;
	}
	fclose(f);
	if (!field(header, 1, host, sizeof host))
	    continue;
	if (!netlookup(host, route))
	    continue;
	p = find_port(route);
	if (p == 0)
	    continue;
	if (send_spool(p, path))
	    unlink(path);
    }
    closedir(d);
    return 1;
}

static void
daemon()
{
    int i;

    for (;;) {
	for (i = 0; i < nports; i++)
	    accept_link(&ports[i]);
	scan_out();
    }
}

main(argc, argv)
    int argc;
    char **argv;
{
    if (argc != 1)
	debug = 1;
    if (!read_netmap())
	return 0;
    if (!read_ttys())
	return 0;
    signal(14, onalarm);		/* SIGALRM */
    daemon();
    return 0;
}
