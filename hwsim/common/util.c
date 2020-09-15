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
#include <time.h>
#include <sys/time.h>
#include "sim.h"

#define PSIZE   80          // max string containing bitdefs
#define NPATS   2           // and 2 per printf
char patspace[PSIZE * NPATS];
char patoff;

typedef unsigned long long u64;

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
 */
char *
bitdef(unsigned char v, char **defs)
{
    int i;
    char *patptr;
    int sep = 0;

    patptr = &patspace[PSIZE * patoff];
    patoff = (patoff + 1) % NPATS;
    *patptr = 0;

    if (defs) {
        for (i = 0; i < 8; i++) {
            if ((v & (1 << i)) && defs[i]) {
                if (sep++) {
                    strcat(patptr, ",");
                }
                strcat(patptr, defs[i]);
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
static unsigned char pchars[16];
static int pcol;

static void
dp()
{
    int i;
    char c;

    for (i = 0; i < pcol; i++) {
        c = pchars[i];
        if ((c <= 0x20) || (c >= 0x7f))
            c = '.';
        printf("%c", c);
        if ((i % 4) == 3) { printf(" "); }
    }
    printf("\n");
}

void
dumpmem(unsigned char (*readbyte) (unsigned short addr), unsigned short addr, int len)
{
    int i;

    pcol = 0;
 
    while (len) {
        if (pcol == 0)
            printf("%04x: ", addr);
        printf("%02x ", pchars[pcol] = (*readbyte) (addr++));
        if ((pcol % 4) == 3) { printf(" "); }
        len--;
        if (pcol++ == 15) {
            dp();
            pcol = 0;
        }
    }
    if (pcol != 0) {
        for (i = pcol; i < 16; i++)
            printf("   ");
        dp();
    }
}

static u64 lasttime;

void
ptime()
{
    u64 now, diff;

    now = now64();
    if (lasttime == 0) lasttime = now;
    diff = now - lasttime;
    lasttime = now;

    printf("%7lld ", diff);
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

// unconditionally log with timestamp  (just log)
void
log(const char *format, ...)
{
    va_list args;

    ptime();
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}

// unconditionally log with no timestamp (log line continuation)
void
logc(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}

// conditionally log with time stamp  (trace line)
void
trace(int bits, const char *format, ...)
{
    if (traceflags & bits) {
        va_list args;

        ptime();
        va_start(args, format);
        vprintf(format, args);
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
        vprintf(format, args);
        va_end(args);
    }
}

char *baseaddr;

static byte
getbyte(unsigned short addr)
{
    return baseaddr[addr];
}

/*
 * use this to dump out from our (linux) address space
 * a relative block
 */
void
hexdump(void *addr, int len)
{
    baseaddr = addr;
    dumpmem(getbyte, 0, len);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
