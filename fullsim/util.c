/*
 * generally useful utility functions
 */

#define	_GNU_SOURCE

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "sim.h"

#define PSIZE   50
char patspace[PSIZE * 2];
char patoff;

char *
bitdef(unsigned char v, char**defs)
{
    int i;
    char *patptr;
    int sep = 0;

    patptr = &patspace[PSIZE * patoff];
    patoff ^= 1;
    *patptr = 0;

    for (i = 0; i < 8; i++) {
        if ((v & (1 << i)) && defs[i]) {
            if (sep++) 
                strcat(patptr, ",");
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
unsigned char pchars[16];
int pcol;

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
dumpmem(unsigned char (*readbyte) (vaddr addr), vaddr addr, int len)
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

char *baseaddr;

static byte
getbyte(vaddr addr)
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
