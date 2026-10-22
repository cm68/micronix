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
 * 0x20 added.  The receiver reads the next record before answering the
 * previous one, so the sender sends its lines as a burst and drains the
 * Y answers; the E record is not acknowledged.
 *
 * A line that fails its checksum is answered with a NAK (payload N) and
 * the sender resends it.  The E record's character is the next sequence
 * number, so a dropped record is caught there: the receiver NAKs and
 * discards its temporary, and the sender retries the whole transfer.
 * The destination is written to <path>.net and renamed on success.
 *
 * An R request is store-and-forward, and the recovered daemons were run
 * through it over two hops: a daemon spools the request and forwards it
 * unchanged, and the machine that holds the file execs netact, which
 * runs netcp to copy it into an S record that travels back.  This
 * reconstruction does the copy itself; the wire is the same.  netact
 * needs a `network` user in /etc/passwd, and every netmap needs the
 * return route.
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
#define HANDSHAKE 90

extern DIR *opendir();
extern struct direct *readdir();
extern int closedir();
extern int (*signal())();
extern int fread();
extern int rename(char *, char *);

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
	return -1;			/* a line, but not a valid one */
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
    char tmp[300];
    char mode[32];
    FILE *out;
    int len;
    int r;
    int seq;
    int naks;

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
    /*
     * Write a temporary and rename it at the end, so a transfer that
     * fails its last record does not leave a half file in place.
     */
    sprintf(tmp, "%s.net", path);
    out = fopen(tmp, "w");
    if (out == 0) {
	perror(tmp);
	return 0;
    }
    seq = 1;
    naks = 0;
    for (;;) {
	r = wire_read(p, line, sizeof line, HANDSHAKE);
	if (r == 0) {
	    fclose(out);
	    unlink(tmp);
	    return 0;
	}
	if (r < 0) {			/* lrec - checksum */
	    wire_write(p, "N");
	    if (++naks > 8) {
		fclose(out);
		unlink(tmp);
		return 0;
	    }
	    continue;
	}
	if (line[0] == 'D') {
	    int ds;

	    ds = ((unsigned char) line[1] - 0x20) & 0x3f;
	    if (ds != seq) {		/* a lost or out-of-order record */
		wire_write(p, "N");
		if (++naks > 8) {
		    fclose(out);
		    unlink(tmp);
		    return 0;
		}
		continue;
	    }
	    len = ((unsigned char) line[2] - 0x20) & 0x3f;
	    decode(line + 3, len, out);
	    wire_write(p, "Y");
	    seq++;
	    naks = 0;
	    continue;
	}
	if (line[0] == 'E') {
	    int es;

	    /*
	     * The end record carries the next sequence number.  If it
	     * does not match what arrived, a record was lost; NAK and
	     * leave the spool entry for a fresh transfer.
	     */
	    es = ((unsigned char) line[1] - 0x20) & 0x3f;
	    if (es != seq) {
		wire_write(p, "N");
		fclose(out);
		unlink(tmp);
		return 0;
	    }
	    break;
	}
	wire_write(p, "N");
	if (++naks > 8) {
	    fclose(out);
	    unlink(tmp);
	    return 0;
	}
    }
    fclose(out);
    rename(tmp, path);
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
 * The 1.6 spool names the next hop, not the final host.
 */
static void
queue_for(host, out)
    char *host;
    char *out;
{
    char route[64];

    if (netlookup(host, route))
	queuename(route, out);
    else
	queuename(host, out);
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
    queue_for(desthost, q);
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
    queue_for(host, q);
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
    int r, naks;

    if (!field(header, 1, host, sizeof host))
	return 0;
    queue_for(host, q);
    out = fopen(q, "w");
    if (out == 0)
	return 0;
    fprintf(out, "%s\n", header);
    naks = 0;
    for (;;) {
	r = wire_read(p, line, sizeof line, HANDSHAKE);
	if (r == 0) {
	    fclose(out);
	    return 0;
	}
	if (r < 0) {
	    wire_write(p, "N");
	    if (++naks > 8) {
		fclose(out);
		return 0;
	    }
	    continue;
	}
	if (line[0] == 'D') {
	    fprintf(out, "%s\n", line);
	    wire_write(p, "Y");
	    naks = 0;
	    continue;
	}
	if (line[0] == 'E') {
	    fprintf(out, "%s\n", line);
	    break;
	}
	wire_write(p, "N");
	if (++naks > 8) {
	    fclose(out);
	    return 0;
	}
    }
    fclose(out);
    return 1;
}

/*
 * The body of an acceptance, after the P! has been read: A!, F and its
 * Y, the S or R header and its Y, then the transfer.
 */
static int
proposal(p)
    struct port *p;
{
    char line[LINEMAX];
    char rheader[LINEMAX];
    int r, tries;

    wire_write(p, "A!");
    for (tries = 0; tries < 6; tries++) {
	r = wire_read(p, line, sizeof line, HANDSHAKE);
	if (r == 0)
	    return 0;
	if (r > 0 && strcmp(line, "F") == 0)
	    break;
	wire_write(p, "N");
    }
    if (tries == 6)
	return 0;
    wire_write(p, "Y");
    for (tries = 0; tries < 6; tries++) {
	r = wire_read(p, line, sizeof line, HANDSHAKE);
	if (r == 0)
	    return 0;
	if (r > 0 && (line[0] == 'S' || line[0] == 'R'))
	    break;
	wire_write(p, "N");
    }
    if (tries == 6)
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
	for (tries = 0; tries < 6; tries++) {
	    r = wire_read(p, line, sizeof line, HANDSHAKE);
	    if (r == 0)
		return 0;
	    if (r > 0 && line[0] == 'E')
		break;
	    wire_write(p, "N");
	}
	if (tries == 6)
	    return 0;
	localname(ourhost);
	if (field(rheader, 1, reqhost, sizeof reqhost) &&
	    strcmp(reqhost, ourhost) == 0)
	    return copy_r_to_s(rheader);
	return relay_r(rheader);
    }
    return 0;
}

/*
 * Accept a transfer: a P! proposal, then the body.
 */
static int
accept_link(p)
    struct port *p;
{
    char line[LINEMAX];
    int r;

    r = wire_read(p, line, sizeof line, 1);
    if (r == 0)
	return 0;
    if (r < 0) {
	wire_write(p, "N");		/* lrec - checksum: send a NAK */
	return 0;
    }
    if (strcmp(line, "P!"))
	return 0;
    return proposal(p);
}

/*
 * Send one line and watch for a NAK.  The 1.6 receiver defers its Y
 * until it has read the next line, so a timeout is not a failure; a NAK
 * means resend this line.
 */
static int
send_line(p, line)
    struct port *p;
    char *line;
{
    char buf[LINEMAX];
    int tries, r, naked;

    naked = 0;
    for (tries = 0; tries < 6; tries++) {
	if (!wire_write(p, line))
	    return 0;
	r = wire_read(p, buf, sizeof buf, 1);
	if (r == 0) {
	    /* No answer: a receiver that reads ahead, or one that has
	     * given up after a NAK.  The latter is a failure. */
	    if (naked)
		return 0;
	    return 1;
	}
	if (r < 0) {			/* a bad line from the peer */
	    naked = 1;
	    continue;
	}
	if (buf[0] == 'N') {		/* NAK: send this line again */
	    naked = 1;
	    continue;
	}
	if (strcmp(buf, "P!") == 0) {	/* the R reply arrived */
	    proposal(p);
	    return 1;
	}
	return 1;			/* Y, or another acknowledgement */
    }
    return 0;
}

/*
 * Send one spool entry: P! and its A!, then F, the S or R header, every
 * D record and the E.  Between lines the sender watches for a NAK; it
 * does not wait for a Y, because the receiver reads ahead.
 */
static int
send_spool(p, path)
    struct port *p;
    char *path;
{
    FILE *f;
    char line[LINEMAX];
    int ok;
    int tries, r, got;

    f = fopen(path, "r");
    if (f == 0)
	return 0;
    got = 0;
    for (tries = 0; tries < 6; tries++) {
	if (!wire_write(p, "P!"))
	    break;
	r = wire_read(p, line, sizeof line, HANDSHAKE);
	if (r == 1 && strcmp(line, "A!") == 0) {
	    got = 1;
	    break;
	}
	if (r == 0)
	    break;
    }
    if (!got) {
	fclose(f);
	return 0;
    }
    if (!send_line(p, "F")) {
	fclose(f);
	return 0;
    }
    while (fgets(line, sizeof line, f)) {
	strip(line);
	if (line[0] == 0)
	    continue;
	if (!send_line(p, line)) {
	    fclose(f);
	    return 0;
	}
	if (line[0] == 'E')
	    break;
    }
    fclose(f);
    ok = 1;
    while (wire_read(p, line, sizeof line, 2)) {
	if (line[0] == 'N') {		/* a NAK that arrived late */
	    ok = 0;
	    break;
	}
	if (strcmp(line, "P!") == 0) {
	    /*
	     * The peer turned the transfer around - an R reply, or a
	     * fresh proposal - while the acknowledgements were being
	     * drained.  Handle it here, or it is lost.
	     */
	    proposal(p);
	    break;
	}
    }
    return ok;
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
