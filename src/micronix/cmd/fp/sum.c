/*
 * fp - dump and restore files onto floppy volumes
 *
 * cmd/fp/sum.c
 */

#include "fp.h"

void
setchecksum(t)
    struct entry *t;
{
    t->checksum = 0;
    t->checksum = (-sum(t, sizeof *t));
}

/*
 * sum - a 16-bit additive checksum over n bytes.  a is the address, n
 * the byte count.  The odd-byte case is unreachable here (every sum is
 * over a struct of even size), and is handled by clearing one word.
 */
unsigned short
sum(a, n)
    unsigned short *a;
    int n;
{
    static unsigned short b, c;

    if (n & 1)         /* odd byte count */
        a[n++] = 0;

    n /= 2;            /* switch from byte count to word count */

    c = 0;

    for (b = 0; b < n; b++)
        c += *a++;

    return c;
}

int
oksum(t)
    struct entry *t;
{
    return sum(t, sizeof *t) == 0;
}

char
xor(a, b)
    char *a;
    int b;
{
    static char c;

    c = 0;

    while (b--)
        c ^= *a++;

    return c;
}
