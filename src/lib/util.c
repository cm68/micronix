/*
 * util.c
 *
 * generally useful utility functions
 * pluggable tracing facility, hexdump, white space, bitdef formatter
 */

#define	_GNU_SOURCE

#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/errno.h>

#include "util.h"

int logfd = 1;

u64
now64()
{
    struct timeval tv;
    u64 u64useconds;

    gettimeofday(&tv,NULL);
    u64useconds = (1000000L * tv.tv_sec) + tv.tv_usec;
    return u64useconds;
}

/*
 * byte bitoff formatter.
 * usage:  char *foo = { "0", "1", "2", "3", 0, "5", "6", "7" };
 *  printf("%s %s\n", bitdef(0xa, foo), bitdef(0x83, foo));
 * null pointer for defs is fine.
 *
 * we return a pointer to a static formatted buffer. since we
 * might want multiple bitdefs in a printf, we rotate instances
 * of them
 */
char *
bitdef(unsigned char v, char **defs)
{
    int i;
    char *patptr;
    int sep = 0;
    char *s, *d;

#define PSIZE   80          // max string containing bitdefs
#define NPATS   2           // and 2 per printf
    static char patspace[PSIZE * NPATS];
    static char patoff;

    patptr = &patspace[PSIZE * patoff];
    patoff = (patoff + 1) % NPATS;
    *patptr = 0;
    d = patptr;

    if (defs) {
        for (i = 0; i < 8; i++) {
            if ((v & (1 << i)) && defs[i]) {
                if (sep++) {
                    *d++ = ',';
                }
                s = defs[i];
                while ((d < &patptr[PSIZE-1]) && *s) {
                    *d++ = *s++;
                }
                *d = '\0';
            }
        }
    }
    return patptr;
}

/*
 * jump over white space
 */
void
skipwhite(char **s)
{
    while (**s && (**s == ' ' || **s == '\t')) {
        (*s)++;
    }
}

/*
 * formatted memory dumper subroutines
 * we are exclusively interested in 16 bit offsets
 */
static char pchars[16];
static int pcol;

/*
 * dump out the sanitized ascii
 */
static void
dp()
{
    int i;
    char c;

    for (i = 0; i < pcol; i++) {
        c = pchars[i];
        if ((c <= 0x20) || (c >= 0x7f))
            c = '.';
        dprintf(logfd, "%c", c);
        if ((i % 4) == 3) { dprintf(logfd, " "); }
    }
    dprintf(logfd, "\n");
}

void
dumpmem(char (*readbyte)(unsigned short addr), unsigned short addr, int len)
{
    int i;
    char c;

    pcol = 0;
 
    while (len) {
        if (pcol == 0)
            dprintf(logfd, "%04x: ", addr);
        c = (*readbyte)(addr++);
        pchars[pcol] = c;
        dprintf(logfd, "%02x ", c & 0xff);
        if ((pcol % 4) == 3) { dprintf(logfd, " "); }
        len--;
        if (pcol++ == 15) {
            dp();
            pcol = 0;
        }
    }
    // fill the line with pad
    if (pcol != 0) {
        for (i = pcol; i < 16; i++)
            dprintf(logfd, "   ");
        dp();
    }
}

static char *hexdump_baseaddr;

static char
getbyte(unsigned short addr)
{
    return hexdump_baseaddr[addr];
}

/*
 * use this to dump out from our (linux) address space
 * a relative block
 */
void
hexdump(void *addr, int len)
{
    hexdump_baseaddr = addr;
    dumpmem(getbyte, 0, len);
}

/*
 * timestamp logging
 */
static u64 lasttime;

/*
 * print a relative time
 */
void
ptime()
{
    u64 now, diff;

    now = now64();
    if (lasttime == 0) lasttime = now;
    diff = now - lasttime;
    lasttime = now;

    dprintf(logfd, "%7lld ", diff);
}

// unconditionally log with timestamp  (just log)
void
log(const char *format, ...)
{
    va_list args;

    ptime();
    va_start(args, format);
    vdprintf(logfd, format, args);
    va_end(args);
}

// unconditionally log with no timestamp (log line continuation)
void
logc(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    vdprintf(logfd, format, args);
    va_end(args);
}

/*
 * a named trace level facility
 */
char *tracenames[32];
int traces;

int
register_trace(char *name)
{
    tracenames[traces] = name;
    return 1 << traces++;
}

// conditionally log with time stamp  (trace line)
void
trace(int bits, const char *format, ...)
{
    if (traceflags & bits) {
        va_list args;

        ptime();
        va_start(args, format);
        vdprintf(logfd, format, args);
        va_end(args);
    }
}

// conditionally log with no time stamp (trace line continuation)
void
tracec(int bits, const char *format, ...)
{
    if (traceflags & bits) {
        va_list args;

        va_start(args, format);
        vdprintf(logfd, format, args);
        va_end(args);
    }
}

int
devnum(char *name, char *dtp, int *majorp, int *minorp)
{
    char linkbuf[80];

    int i;
    i = readlink(name, linkbuf, sizeof(linkbuf));
    if (i == -1) {
        return ENOENT;
    } else {
        linkbuf[i] = '\0';
    }
    if ((i = sscanf(linkbuf, "%cdev(%d,%d)", dtp, majorp, minorp)) != 3) {
        return ENOENT;
    }
    return 0;
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
