/*
 * generally useful utility functions
 */

#define	_GNU_SOURCE

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

char patspace[100];

char *
bitdef(unsigned char v, char**defs)
{
    int i;

    patspace[0] = 0;

    for (i = 0; i < 8; i++) {
        if ((v & (1 << i)) && defs[i]) {
            strcat(patspace, defs[i]);
            strcat(patspace, " ");
        }
    }
    return patspace;
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
dumpmem(unsigned char (*readbyte) (long addr), long addr, int len)
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
 * if sizeof(long) != sizeof(char *) lose big
 */
static unsigned char
getbyte(long addr)
{
    return *(unsigned char *)addr;
}

void
hexdump(unsigned char *addr, unsigned short len)
{
    if (sizeof(long) != sizeof(unsigned char *)) {
        printf("the horror, the horror.\n");
        exit(1);
    }
    dumpmem(getbyte, (long)addr, len);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
