/*
 * fp - dump and restore files onto floppy volumes
 *
 * cmd/fp/format.c
 */

#include "fp.h"

/*
 * doformat - run the format program (formatprog, /bin/fdj) and wait for
 * it to finish.
 */
void
doformat()
{
    int s, p, w;

    closedev();

    p = fork();

    if (p == 0) {              /* child */
        execl(formatprog, formatprog, NULL);
        exit(NO);
    }

    for (;;) {
        w = wait(&s);

        if (w == (-1) || w == p)
            break;
    }
}
