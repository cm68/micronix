/*
 * fp - dump and restore files onto floppy volumes
 *
 * cmd/fp/main.c
 */

#include "fp.h"

main(ac, av)
    int ac;
    char **av;
{
    init(ac, av);
    fp();
}

/*
 * fp - dispatch on the option flags.  Exactly one of table (t), extract
 * (x or p) and replace (r) is meant to run; the verbose flag (v) is
 * orthogonal, and its space report runs afterwards when it is set.
 */
fp()
{
    if (tflag)
        table();
    if (xflag || pflag)
        extract();
    if (rflag)
        replace();
    if (vflag)
        space();
}
