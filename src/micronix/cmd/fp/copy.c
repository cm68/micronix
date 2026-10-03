/*
 * fp - dump and restore files onto floppy volumes
 *
 * cmd/fp/copy.c
 */

#include "fp.h"

/*
 * fcopy - copy c bytes from fd a to fd b, d naming the file for error
 * messages.  When b is READONLY nothing is written and the copy is just
 * a read-through.  e selects ROUND (byte-exact) or SQUARE copies; SQUARE
 * pads the final partial block up to a block boundary before writing it.
 */
int
fcopy(a, b, c, d, e)
    int a, b, e;
    long c;
    char *d;
{
    char buf[CHUNK];
    static int togo, n;

    while (0 < c) {
        if (c < CHUNK)
            togo = c;
        else
            togo = CHUNK;

        n = read(a, buf, togo);

        if (n != togo) {
            if (n < 0)
                perror(d);
            return NO;
        }

        if (b != READONLY) {
            /*
             * last partial write changed to block boundary write
             */
            if ((e == SQUARE) && (n & 511)) {
                n = ((n + 511) / 512) * 512;
                c = n;
            }

            if (write(b, buf, n) != n) {
                perror(d);
                return NO;
            }
        }

        c -= n;
    }

    return YES;
}
