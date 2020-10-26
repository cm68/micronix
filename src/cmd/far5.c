#include "far.h"

devsize(a)
{
    char c;
    long low, high, midpoint;

    low = 0;
    high = K64;

    for (;;) {
        midpoint = (low + high) / 2;

        if (midpoint == low)
            break;

        if (seek(a, (int) midpoint, 3) >= 0 && read(a, &c, 1) == 1) {
            low = midpoint;
        }

        else {
            if (errno == ENXIO) {       /* off the disk */
                high = midpoint;
            } else {            /* on the disk but unreadable */
                perror(NULL);
                low = midpoint;
            }
        }
    }

    seek(a, 0, 0);              /* return the file offset to the beginning */

    if (high >= K64)
        high = K64 - 1;         /* max. size */

    return (int) high;
}

char *
save(a)
    char *a;
{
    char *b;

    b = alloc(lenstr(a) + 1);

    cpystr(b, a, NULL);

    return b;
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab: 
 */

