/*
 * fp - dump and restore files onto floppy volumes
 *
 * cmd/fp/init.c
 */

#include "fp.h"

void arglist();
void doflag();

void
init(ac, av)
    int ac;
    char **av;
{
    stderr->_flag &= ~_IONBF;      /* make stderr buffered */

    time(&dumptime);

    time(&yearago);
    yearago -= YEAR;
    time(&yearfromnow);
    yearfromnow += YEAR;

    serial = dumptime;

    if (sizeof(struct entry) != 64)
        fatal("entry structure wrong size\n");

    arglist(ac, av);

    if (!devopen())
        exit(NO);
}

void
arglist(ac, av)
    int ac;
    char **av;
{
    int n = 2;

    if (ac > 1)
        doflag(av[1]);

    if (rflag == NO && tflag == NO && xflag == NO)
        tflag = YES;

    if (ac > 2) {
        nfiles = ac - 2;
        files = av + 2;
    } else {
        nfiles = 0;
    }

    if (fflag) {
        device = av[n++];
        nfiles--;
        files++;
    }

    if (eflag) {
        formatprog = av[n++];
        nfiles--;
        files++;
    }
}

void
doflag(a)
    char *a;
{
    for (; *a; a++) {
        switch (*a) {
        case 'e':
            eflag = YES;
            break;

        case 'l':
            lflag = YES;
            break;

        case 'p':
            pflag = YES;
            break;

        case 'v':
            vflag = YES;
            break;

        case 'c':
            cflag = YES;
            break;

        case 'f':
            fflag = YES;
            break;

        case 'i':                 /* incremental */
            iflag = YES;
            break;

        case 's':                 /* silent */
            sflag = YES;
            vflag = NO;
            break;

        case 't':                 /* table */
            tflag = YES;
            break;

        case 'q':                 /* suppress error checking */
            qflag = YES;
            break;

        case 'r':
            rflag = YES;
            break;

        case 'x':
            xflag = YES;
            break;

        case '?':
            debug = YES;
            break;

        case '-':
            break;

        default:
            usage();
            break;
        }
    }

    if (rflag + tflag + pflag + xflag > YES)
        fatal("Only one of p, r, t, and x may be specified\n");

    if (cflag && !rflag)
        cflag = NO;
}
