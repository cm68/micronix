/*
 * util.c
 *
 * generally useful utility functions
 * pluggable tracing facility, hexdump, white space, bitdef formatter
 */

#define	_GNU_SOURCE

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "sim.h"

#define PSIZE   80          // max string containing bitdefs
#define NPATS   2           // and 2 per printf
char patspace[PSIZE * NPATS];
char patoff;

/*
 * byte bitoff formatter.
 * usage:  char *foo = { "0", "1", "2", "3", 0, "5", "6", "7" };
 *  printf("%s %s\n", bitdef(0xa, foo), bitdef(0x83, foo));
 */
char *
bitdef(unsigned char v, char**defs)
{
    int i;
    char *patptr;
    int sep = 0;

    patptr = &patspace[PSIZE * patoff];
    patoff = (patoff + 1) % NPATS;
    *patptr = 0;

    for (i = 0; i < 8; i++) {
        if ((v & (1 << i)) && defs[i]) {
            if (sep++) {
                strcat(patptr, ",");
            }
            strcat(patptr, defs[i]);
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
