/*
 * astio.c - AST file I/O
 */
#include "pass2.h"
#include "lexeme.h"
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>

#ifdef DEBUG
#include "debug.h"
#endif

int infd;
int outfd;
static int pushback = -1;

/*
 * The .n sidecar, from cpp -j: identifiers crossed the front of the
 * compiler as 2-byte ids and their spellings live here - a 2-byte
 * count, a table of 2-byte offsets, then the names in id order,
 * NUL-terminated.  Ids are 1-based; 0 is reserved.  Two seeks fetch
 * any name, so nothing gets loaded.  c1 is where ids become symbols
 * again: expansion happens as names are READ, so everything
 * downstream - codegen, the peephole, our own diagnostics - sees
 * real spellings and never knows the ids existed.
 */
static int nidfd = -1;

/*
 * Sidecar seeks bypass lseek's _fdpos tracking on the Z80: this fd
 * is only ever read at seeked positions, nothing asks where it is,
 * and lseek would drag 600 bytes of position machinery into c1.
 */
#ifdef CCC
extern int seekraw();
#define NSEEK(fd, off) seekraw(fd, (int)(off), 0)
#else
#define NSEEK(fd, off) lseek(fd, (long)(off), 0)
#endif

void
nidopen(char *f1)
{
	/*
	 * Static, not a frame local: a 256-byte auto was the first
	 * frame local past the IY displacement range anywhere in this
	 * pass, and the layout put it on top of the caller's frame -
	 * strcpy wrote the path across main's return address, and c1
	 * under the simulator "returned" into two characters of the
	 * filename.  The big-frame layout bug is pass1's to fix; this
	 * buffer has no reason to be on the stack at all.
	 */
	static char nf[256];
	char *dot;

	/*
	 * base.ast -> base.nam.  Cut at the LAST dot rather than
	 * assuming what follows it is one character long: this read
	 * "nf[n - 2] == '.'" when the tree file was called base.1, and
	 * silently did nothing the moment it was not.  The sidecar was
	 * then never opened, every identifier stayed the two-byte id it
	 * arrived as, and c1 emitted symbols with the raw id in them -
	 * assembler that the assembler rejected, naming a line thirty
	 * further down than the pass that wrote it.
	 */
	strcpy(nf, f1);
	dot = strrchr(nf, '.');
	if (dot)
		strcpy(dot, ".nam");
	nidfd = open(nf, O_RDONLY);
}

/*
 * Fetch a spelling.  The read overshoots into the following names;
 * the first NUL marks the end.
 */
void
nidname(unsigned short id, char *buf, int size)
{
	unsigned char two[2];
	int n, i;

	NSEEK(nidfd, 2 + 2 * (id - 1));
	read(nidfd, (char *)two, 2);
	NSEEK(nidfd, two[0] | (two[1] << 8));
	n = read(nidfd, buf, size - 1);
	for (i = 0; i < n; i++)
		if (!buf[i])
			return;
	buf[i] = 0;
}

/*
 * Replace every @id with its spelling, in place.  Anything else
 * passes through untouched: synthetics (L%d, str%d) and plain-mode
 * names never contain '@'.
 *
 * EVERY one.  This used to expand the first and stop, which was
 * enough while a name held at most one - "_@4" for a global, "@6" for
 * a local.  A static is "_@4.@6" now, the function it is in and then
 * its own name, and expanding the first wrote the spelling straight
 * over the rest: "_counter.n" came out as "_counter", the reference
 * pointed at the function instead of at the static, and the code read
 * and wrote the first two bytes of its own entry point.
 *
 * The rebuild goes through a buffer rather than in place, because a
 * spelling is longer than the "@6" it replaces and would otherwise
 * overwrite the text still to be read.  Static, not a frame local -
 * see the note on nidopen above.
 */
void
nidxp(char *buf, int size)
{
	static char xbuf[128];
	char *s, *d, *end;
	unsigned short id;

	if (nidfd < 0)
		return;
	for (s = buf; *s && *s != '@'; s++)
		;
	if (!*s)
		return;			/* nothing to expand */

	s = buf;
	d = xbuf;
	end = xbuf + sizeof(xbuf) - 1;
	while (*s && d < end) {
		if (*s == '@' && s[1] >= '0' && s[1] <= '9') {
			id = 0;
			for (s++; *s >= '0' && *s <= '9'; s++)
				id = id * 10 + (*s - '0');
			nidname(id, d, end - d);
			while (*d)
				d++;
		} else {
			*d++ = *s++;
		}
	}
	*d = 0;
	for (s = xbuf, d = buf; *s && size > 1; size--)
		*d++ = *s++;
	*d = 0;
}

void
out(char *s)
{
	write(outfd, s, strlen(s));
}

/*
 * Formatted emission.  One call where a chain of out/outd/outc calls
 * stood: each call site in the code generator costs code.  Not a
 * printf - the templates only ever splice a number, a name or a
 * register letter into literal text, so %d, %s and %c are the whole
 * vocabulary.
 */
void
outf(char *fmt, ...)
{
	va_list ap;
	register char *p = fmt;
	char *q;

	va_start(ap, fmt);
	q = p;
	while (*p) {
		if (*p != '%') {
			p++;
			continue;
		}
		if (p > q)
			write(outfd, q, p - q);
		p++;
		if (*p == 's')
			out(va_arg(ap, char *));
		else if (*p == 'c')
			outc(va_arg(ap, int));
		else
			outd(va_arg(ap, int));
		p++;
		q = p;
	}
	if (p > q)
		write(outfd, q, p - q);
	va_end(ap);
}

void
outc(char c)
{
	write(outfd, &c, 1);
}

/*
 * Negating before converting cannot represent the most negative int,
 * which stayed negative and turned every digit into whatever character
 * sits that far below '0'.  Build the digits from the negative side
 * instead, where every value is representable.
 */
void
outd(int n)
{
	char buf[12];
	register char *p = buf + 11;
	int neg;

	/*
	 * Both builds must SPELL a word the same way, or the self-built
	 * compiler's output text never matches the host's: 0xffff is
	 * -1 in a 16-bit int and 65535 in a 32-bit one.  The Z80 form
	 * is the canonical one - the host narrows to match.
	 */
	n = (short)n;
	neg = n < 0;
	if (!neg) n = -n;
	*p = 0;
	do { *--p = '0' - n % 10; n /= 10; } while (n);
	if (neg) *--p = '-';
	out(p);
}

/*
 * The same word, spelled without a sign.
 *
 * outd() narrows to a signed short so both builds spell a word alike,
 * and for an ADDRESS that canonical choice is the wrong one: pass1
 * hands 0xf000 down as -4096, and asz answers "invalid operand" to
 *
 *	ld (-4096),hl
 *
 * which is every memory mapped device on the machine.  Unsigned is
 * just as canonical - 16 bits either way, and the host narrows to
 * the same 16 - so this is the same guarantee with the sign dropped.
 * A displacement like (ix-5) still goes through outd, where the sign
 * is the point.
 */
void
outu(int n)
{
	char buf[8];
	register char *p = buf + 7;
	unsigned int v = (unsigned short)n;

	*p = 0;
	do { *--p = '0' + v % 10; v /= 10; } while (v);
	out(p);
}

/*
 * Copy pass1's assembly output (globals, string literals, file-scope
 * asm) through to our output, then select .text for the code we are
 * about to generate.  The .2 stream starts in .text and emits its own
 * segment directives as it goes, so it needs no preamble.
 */
/*
 * The id-expanding copy: @id becomes its spelling except inside
 * string data, where an @ followed by digits is just text.  A
 * backslash escapes the next character within quotes.  Output is
 * batched - a write per character is a syscall per character, and
 * under the simulator those are the whole bill.
 */
static char cxbuf[128];
static int cxn;

void
cxput(char c)
{
	cxbuf[cxn++] = c;
	if (cxn == sizeof(cxbuf)) {
		write(outfd, cxbuf, cxn);
		cxn = 0;
	}
}

void
copyxp(void)
{
	char buf[64];
	char nam[20];
	char *s;
	int n, i;
	char c;
	char inq = 0, esc = 0;
	char at = 0;		/* digits collected after '@', +1 */
	unsigned short id = 0;

	while ((n = read(in2fd, buf, sizeof(buf))) > 0) {
		for (i = 0; i < n; i++) {
			c = buf[i];
			if (at) {
				if (c >= '0' && c <= '9') {
					id = id * 10 + (c - '0');
					at = 2;
					continue;
				}
				if (at > 1) {	/* a bare '@' is just text */
					nidname(id, nam, sizeof(nam));
					for (s = nam; *s; s++)
						cxput(*s);
				} else {
					cxput('@');
				}
				at = 0;
			}
			if (inq) {
				if (esc)
					esc = 0;
				else if (c == '\\')
					esc = 1;
				else if (c == '"')
					inq = 0;
			} else if (c == '"') {
				inq = 1;
			} else if (c == '@') {
				at = 1;
				id = 0;
				continue;
			}
			cxput(c);
		}
	}
	if (at > 1) {
		nidname(id, nam, sizeof(nam));
		for (s = nam; *s; s++)
			cxput(*s);
	} else if (at) {
		cxput('@');
	}
	if (cxn)
		write(outfd, cxbuf, cxn);
}

void
copyinit(void)
{
	char buf[64];
	int n;

	if (nidfd < 0) {
		while ((n = read(in2fd, buf, sizeof(buf))) > 0)
			write(outfd, buf, n);
	} else {
		copyxp();
	}
	out("\t.text\n");
}

unsigned char
read1(void)
{
	unsigned char c;
	if (pushback >= 0) {
		c = pushback;
		pushback = -1;
	} else {
		c = read(infd, &c, 1) == 1 ? c : E_O_F;
	}
#ifdef DEBUG
	if (VERBOSE(V_IO))
		fprintf(stderr, "read1: %d (0x%02x) '%c'\n", c, c,
			(c >= 32 && c < 127) ? c : '.');
#endif
	return c;
}

void
unread1(unsigned char c)
{
	pushback = c;
}

unsigned short
read2(void)
{
	unsigned char buf[2];
	unsigned short v;
	read(infd, buf, 2);
	v = buf[0] | (buf[1] << 8);
#ifdef DEBUG
	if (VERBOSE(V_IO))
		fprintf(stderr, "read2: %u (0x%04x)\n", v, v);
#endif
	return v;
}

/*
 * The four bytes land in val4 and the function returns nothing.  A
 * long return travels in HL:DE, and the one caller stored it into a
 * local just to push it again; longs are expensive here and rare in
 * the source, so the value goes to memory once and stays there.
 *
 * The file is little-endian by definition - write4 on the other side
 * puts the low byte first - and this used to read the four bytes
 * straight into val4 because the machine laid a long down the same
 * way.  It does not any more: the HIGH word is at the lower address
 * now (QLONG.md, NUXI), so reading the file into the value would land
 * the halves the wrong way round.  The bytes are assembled instead,
 * which says what the file means without depending on either layout.
 */
unsigned long val4;

void
read4(void)
{
#ifdef CCC
	/*
	 * The four bytes are a long the way this machine lays one down,
	 * so they are read into the value and that is the whole of it.
	 */
	read(infd, (char *)&val4, 4);
#else
	/*
	 * The host lays a long down the other way and is not short of
	 * cycles, so it does the work: this is emit4 backwards.  Widened
	 * before each shift, not after - these are unsigned char, so the
	 * shift would be done at int width, and an int is 16 bits on the
	 * machine this same source has to compile for, where any byte
	 * with its top bit set would shift into the sign and then
	 * sign-extend on the way into the long.
	 */
	unsigned char buf[4];

	read(infd, (char *)buf, 4);
	val4 = ((unsigned long)buf[1] << 24) | ((unsigned long)buf[0] << 16) |
	       ((unsigned long)buf[3] << 8) | (unsigned long)buf[2];
#endif
#ifdef DEBUG
	if (VERBOSE(V_IO))
		fprintf(stderr, "read4: %lu (0x%08lx)\n", val4, val4);
#endif
}

/*
 * Read a counted string.  The size is not optional: the length comes
 * off the file as a byte, so it can say up to 255, and a name that
 * exactly filled its buffer put the terminator one past the end.  What
 * followed became part of the name - a 16-character label came out with
 * a stray byte in the middle of it and the assembler stopped on a
 * symbol nobody wrote.
 */
void
readS(char *buf, int size)
{
	unsigned char len = read1();
	int keep = len < size - 1 ? len : size - 1;
	int over = len - keep;
	char waste;

	read(infd, buf, keep);
	buf[keep] = 0;
	while (over-- > 0)		/* the rest still has to come off */
		read(infd, &waste, 1);
	nidxp(buf, size);
#ifdef DEBUG
	if (VERBOSE(V_IO))
		fprintf(stderr, "readS: \"%s\" (len=%d)\n", buf, len);
#endif
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
