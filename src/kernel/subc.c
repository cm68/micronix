/*
 * subc.c 
 */
#include "sys.h"
#include "con.h"

/*
 * panic stop
 */
int

panic(s)
    char *s;
{
    sync();
    pr("Something's wrong: %s\n", s);
    pr("To prevent damage, the system is going down.\n");
    pr("Please reboot, check your file system, and try again.\n");
    pr("If the problem recurs, please contact your dealer\n");
    forever;
}

/*
 * Print " on device <devname>/minor"
 */

prdev(s, dev)
    char *s;
    UINT dev;
{
    pr("%s on disk %s/%d\n", s, devname[major(dev)], minor(dev));
}

/*
 * Limited formatted print for error diagnostics. Recognizes %i or %d as base
 * 10, %o as octal, %x or %h as hex (all unsigned ints), and %s as string.
 */

pr(fmt, args)
    fast char *fmt;
    int args;                   /* may have any number of args */
{
    int *argp;
    char c;

    if (csw() & 1)              /* supress messages if switch 1 is set */
        return;

    argp = fmt;
    argp++;
    while ((c = *fmt++) != '\0') {
        if (c != '%') {
            console(c);
            continue;
        }
        switch (c = *fmt++) {
        case 'i':
        case 'd':
            prn(*argp++, 10);
            break;
        case 'o':
            prn(*argp++, 8);
            break;
        case 'h':
        case 'x':
            prn(*argp++, 16);
            break;
        case 's':
            prs(*argp++);
            break;
        case '\0':
            return;
        default:
            console(c);
        }                       /* end switch */
    }                           /* end while */
}                               /* end function */

/*
 * Print an unsigned integer in base b
 */

prn(n, b)
    fast unsigned n, b;
{
    static UCHAR c;

    if (n >= b)
        prn(n / b, b);
    if ((c = n % b) < 10)
        console(c + '0');
    else
        console(c - 10 + 'a');
}

/*
 * Print a string
 */

prs(s)
    char *s;
{
    char c;

    while ((c = *s++) != 0)
        console(c);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
