/*
 * j-random utility functions
 * these are mostly prime candidates for assembly code
 */

#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include "libutil.h"

extern char *nameOf(unsigned short id);	/* lexread.c */

#ifndef PSIZE
#define PSIZE 80
#endif

#ifdef DEBUG
/* Forward declaration - only used for DEBUG builds */
int fdprintf(unsigned char fd, char *fmt, ...);
#endif

/*
 * The name a static is emitted under.
 *
 * It used to be S<n> and nothing else - a counter, carrying none of
 * what the programmer called the thing.  A symbol table full of S4
 * and S17 tells a debugger, or anyone reading an object, nothing at
 * all, and the statics are exactly what one wants to see: they are
 * the file's private state.
 *
 * The source name, qualified by the scope it was written in.  A
 * static at file scope is "_name"; one inside a function is
 * "_func.name", because the function is what makes it unique - two
 * functions in micronix's queue.c each declare "static struct cblock
 * *new, *old", and ten of its forty kernel sources repeat a static
 * name somewhere.  Two labels of one name in one assembly is a
 * duplicate the assembler refuses long before any linker could
 * uniquify it, and the lexical scope settles it without this pass
 * having to remember every static name in the file - which matters,
 * because it has to run on a Z80.
 *
 * The function is not always enough.  A static in a nested block is
 * still inside one function, and two sibling blocks may each declare
 * one of the same name - legal, and rt_statloc does it on purpose.
 * Those get the counter as well, which is what sid is; a plain
 * function-body static does not need it and does not carry it.
 *
 * The linker's job is the other half: these are local symbols, so
 * nothing resolves against them, and two objects may each have a
 * _f.n without either being wrong.
 */
char *
staticName(char *buf, unsigned short id, unsigned short fid,
    unsigned char sid)
{
	if (!fid)
		return fmtstr(buf, "_%s", nameOf(id));
	return fmtstr(fmtstr(buf, "_%s", nameOf(fid)), ".%d", sid - 1);
}

#ifdef __GNUC__
char patspace[PSIZE];

char *
bitdef(unsigned char v, char **defs)
{
	unsigned char i = 0x01;
	char *patptr;
	unsigned char sep = 0;

	patptr = patspace;
	*patptr = 0;

    while (i) {
		if ((v & i) && *defs) {
			if (sep++) {
				strcat(patptr, ",");
			}
			strcat(patptr, *defs);
		}
        i = i * 2;
        defs++;
	}
	return patptr;
}
#endif

#ifdef DEBUG
/*
 * append string s at d
 */
void
append(char *d, char *s)
{
    while (*s) {
        *d++ = *s++;
    }
    *d = 0;
}

char
iswhite(unsigned char c)
{
    switch (c) {
    case ' ': case '\t': case '\n':
        return 1;
    default:
        return 0;
    }
}

char xxbuf[200];

void
hexdump(char *tag, char *h, int l)
{
    int i;
    char *z = xxbuf;
    unsigned char c;

    strcpy(xxbuf, tag);

    for (i = 0; i < l; i++) {
        c = h[i];
        if ((i % 16) == 0) {
            fdprintf(2," %s\n%04x  ", xxbuf, i);
            z = xxbuf;
            *z = 0;
        }
        fdprintf(2,"%02x ", c);
        if ((i % 4) == 3) printf(" ");
        if ((c < ' ') || (c > 0x7e)) c = '.';
        *z++ = c;
        *z = 0;
    }
    while ((i++ % 16) != 0) {
        if ((i % 4) == 3) printf(" ");
        fdprintf(2,"   ");
    }
    printf(" %s\n", xxbuf);
}

/*
 * return the index in an array of the first occurrance of a char
 * return 0xff for miss
 */
unsigned char
lookupc(char *s, char c)
{
    unsigned char i;
    for (i = 0; s[i]; i++) {
        if (c == s[i]) {
            return i;
        }
    }
    return 0xff;
}

/*
 * fdprintf - printf-like function that writes to a Unix file descriptor
 * Uses sprintf to format to a static buffer, then writes via write() syscall
 */
int
fdprintf(unsigned char fd, char *fmt, ...)
{
    static char buf[1024];
    va_list args;
    int len;
    int result;

    va_start(args, fmt);
    len = vsprintf(buf, fmt, args);
    va_end(args);

    if (len > 0) {
        result = write(fd, buf, len);
        return result;
    }

    return len;
}

/*
 * fdputs - write a string to a Unix file descriptor
 * More efficient than fdprintf for simple string output (no formatting overhead)
 */
int
fdputs(unsigned char fd, char *s)
{
    int len;

    len = strlen(s);
    if (len > 0) {
        return write(fd, s, len);
    }
    return 0;
}
#endif

/* Binary AST emission helpers */
extern unsigned char astFd;

void emit1(unsigned char b)
{
	write(astFd, &b, 1);
}

void emit2(unsigned short w)
{
	unsigned char buf[2];
	buf[0] = w & 0xff;
	buf[1] = (w >> 8) & 0xff;
	write(astFd, buf, 2);
}

/*
 * A long goes into the AST laid out the way the machine lays one down
 * - high word first, each word little-endian within itself - so the
 * Z80 build stores it and pass2 loads it and neither shifts anything.
 * See QLONG.md and NUXI; read4 on the other side is this backwards.
 */
void emit4(unsigned long l)
{
	unsigned char buf[4];

#ifdef CCC
	*(unsigned long *)buf = l;
#else
	buf[0] = (l >> 16) & 0xff;
	buf[1] = (l >> 24) & 0xff;
	buf[2] = l & 0xff;
	buf[3] = (l >> 8) & 0xff;
#endif
	write(astFd, buf, 4);
}

void emitN(char *s, unsigned char len)
{
	emit1(len);
	if (len > 0)
		write(astFd, s, len);
}

void emitS(char *s)
{
	emitN(s, strlen(s));
}

/* raw bytes, no length prefix - caller emits the count */
void emitRaw(char *s, unsigned short len)
{
	if (len > 0)
		write(astFd, s, len);
}

/* Assembly output helpers - write to asmFd */
extern unsigned char asmFd;

void asmStr(char *s)
{
	write(asmFd, s, strlen(s));
}

void asmLine(char *s)
{
	asmStr(s);
	write(asmFd, "\n", 1);
}

void asmLabel(char *name)
{
	asmStr(name);
	asmLine(":");
}

void asmDb(int val)
{
	char buf[16];
	fmtstr(buf, "\t.db %d", val & 0xff);
	asmLine(buf);
}

/*
 * Check if a character is printable ASCII for string literals.
 * Excludes the double quote that ends an asz string and the
 * backslash that starts an escape inside one.
 */
int
isPrintable(unsigned char c)
{
	return c >= 0x20 && c <= 0x7e && c != '"' && c != '\\';
}

char asciibuf[128];
/*
 * Emit a string as .db with ASCII literals and hex for non-printable.
 * Groups printable chars up to 60, breaks on non-printable.
 * Format: .db "text", 0x##, "more text", 0x0
 * asz takes strings in double quotes; single quotes are char literals.
 */
void asmDbStr(unsigned char *data, int len)
{
	char *p;
	int i, start, runlen;
	unsigned char c;
	int needcomma;

	asmStr("\t.db ");
	needcomma = 0;

	i = 0;
	while (i <= len) {  /* <= to include null terminator */
		c = (i < len) ? data[i] : 0;

		/* Check for printable run */
		if (isPrintable(c)) {
			start = i;
			runlen = 0;
			while (i <= len && runlen < 60) {
				c = (i < len) ? data[i] : 0;
				if (!isPrintable(c))
					break;
				runlen++;
				i++;
			}
			/* Emit the printable run as "string" */
			if (needcomma)
				asmStr(", ");
			asmStr("\"");
			p = asciibuf;
			while (start < i) {
				*p++ = data[start++];
			}
			*p = 0;
			asmStr(asciibuf);
			asmStr("\"");
			needcomma = 1;
		} else {
			/* Emit non-printable as hex */
			if (needcomma)
				asmStr(", ");
			fmtstr(asciibuf, "0x%c%c",
			    "0123456789abcdef"[(c >> 4) & 0xf],
			    "0123456789abcdef"[c & 0xf]);
			asmStr(asciibuf);
			needcomma = 1;
			i++;
		}
	}
	asmStr("\n");
}

void asmDw(int val)
{
	char buf[16];
	/* both builds must SPELL a word the same way; the 16-bit form
	 * is canonical, exactly as pass2's outd narrows.  The long
	 * masking this replaces printed 65535 on the host and -1 from
	 * the self-hosted build anyway - the mask vanished somewhere in
	 * the long path - and the assembler reads both the same. */
	fmtstr(buf, "\t.dw %d", (int)(short)val);
	asmLine(buf);
}

void asmDwSym(char *name)
{
	asmStr("\t.dw ");
	asmLine(name);
}

void asmDs(int size)
{
	char buf[16];
	fmtstr(buf, "\t.ds %d", size);
	asmLine(buf);
}

/* Segment tracking - emit .text/.data/.bss only when segment changes */
static unsigned char curSeg = 0;  /* SEG_TEXT */

void setSeg(unsigned char seg)
{
	if (seg == curSeg)
		return;
	curSeg = seg;
	switch (seg) {
	case 0: asmLine("\t.text"); break;  /* SEG_TEXT */
	case 1: asmLine("\t.data"); break;  /* SEG_DATA */
	case 2: asmLine("\t.bss"); break;   /* SEG_BSS */
	}
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
