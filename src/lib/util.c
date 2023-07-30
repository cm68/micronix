/*
 * generally useful utility functions
 * pluggable tracing facility, hexdump, white space, bitdef formatter
 * also, a block editor
 *
 * lib/util.c
 * Changed: <2023-07-13 13:55:15 curt>
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
#include <curses.h>
#include <term.h>

#include "../include/util.h"

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
dumpmem(unsigned char (*readbyte)(unsigned short addr), unsigned short addr, int len)
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

static unsigned char *hexdump_baseaddr;

static unsigned char
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
l(const char *format, ...)
{
    va_list args;

    ptime();
    va_start(args, format);
    vdprintf(logfd, format, args);
    va_end(args);
}

// unconditionally log with no timestamp (log line continuation)
void
lc(const char *format, ...)
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
    char *s;
    int i;

    s = linkbuf;

    i = readlink(name, s, sizeof(linkbuf));
    if (i == -1) {
        return ENOENT;
    } else {
        s[i] = '\0';
    }

// printf("devnum: %s\n", s);
    if ((i = sscanf(s, "%cdev(%d,%d)", dtp, majorp, minorp)) != 3) {
// printf("major %x minor: %x dtp: %s\n", *majorp, *minorp, dtp);
        return ENOENT;
    }
    return 0;
}

int
chextoi(char c)
{
    if (c < 'a') return c - '0';
    return (c - 'a') + 0xa;
}

/*
 * a block editor
 */
void
blockedit(char *buf, int len)
{
    unsigned char *editbuf;
    int numlines = (len + 15) / 16;
    int c;
    unsigned int b;
    int y;
    int x;
    int col;
    int i;
    
    editbuf = malloc(len);
    bcopy(buf, editbuf, len);

    initscr(); 
    cbreak();
    noecho();

    for (y = 0; y < numlines; y++) {
        mvprintw(y, 0, "%03x: ", y * 16);    
        for (x = 0; x < 16; x++) {
            if ((y * 16) + x >= len) break;
            b = editbuf[(y*16)+x] & 0xff;
            mvprintw(y, 5 + (x * 3) + (x / 4), "%02x", b);
            if ((b <= 0x20) || (b >= 0x7f))
                b = '.';
            mvaddch(y, 57 + (x) + (x / 4), b);
        }
    }

    y = 0;
    x = 0;
    col = 0;

    while (1) {
        i = (y * 16) + x;
        c = mvgetch(y, 5 + (x * 3) + (x / 4) + col);
        switch (c) {
        case '0': case '1': case '2': case '3': case '4': 
        case '5': case '6': case '7': case '8': case '9': 
        case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
            b = editbuf[i] & 0xff;
            if (col) {
                b = (b & 0xf0) | chextoi(c);
            } else {
                b = (b & 0xf) | (chextoi(c) << 4);
            }
            editbuf[i] = b;
            mvprintw(y, 5 + (x * 3) + (x / 4), "%02x", b);
            if ((b <= 0x20) || (b >= 0x7f))
                b = '.';
            mvaddch(y, 57 + (x) + (x / 4), b);

            col++; 
            if (col == 2) {
                col = 0;
                x++; 
                if (x > 15) {
                    x = 0; 
                    y++;
                    if (y >= numlines) {
                        y = 0;
                    }
                }
                if (((y * 16) + x) >= len) {
                    x = 0;
                    y = 0;
                }
            }
            break;
        case 'h':   // left
            if (col) {
                col--;
            } else {
                x--; if (x < 0) x = 0;
                col = 1;
            }
            break;
        case 'j':   // down
            y++; if (y >= numlines) y = numlines - 1;
            break;
        case 'k':   // up   
            y--; if (y < 0) y = 0;
            break;
        case 'l':   // right
            if (col) {
                x++; if (x > 15) x = 15;
                if (((y * 16) + x) >= len) x = (len % 16) - 1;
                col = 0;
            } else {
                col++;
            }
            break;
        case 'w':   // write 
        case 'x':   // exit 
            goto done;
            break;
        case 'q':   // abort 
            goto abort;
        }
    }
done:
    bcopy(editbuf, buf, len);
abort:
    doupdate();
    endwin();
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
