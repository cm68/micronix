/*
 * kernel print
 * 
 * sys/subc.c 
 * Changed: <>
 */
#include <types.h>
#include <sys/sys.h>
#include <sys/con.h>
#include <sys/ovl.h>
extern void sync();
extern void pr();
extern void prn();
extern void prs();

/*
 * panic stop
 */
void

panic(s)
    UINT8 *s;
{
    sync();
    pr("panic: %s\n", s);
    for (;;)
        hlt();
}

/*
 * Print " on device <devname>/minor"
 */

void
prdev(s, dev)
    UINT8 *s;
    UINT16 dev;
{
    pr("%s on disk %s/%d\n", s, devname, minor(dev));
}

/*
 * Limited formatted print for error diagnostics. Recognizes %i or %d as base
 * 10, %o as octal, %x or %h as hex (all unsigned ints), and %s as string.
 */

void
pr(fmt, args)
    UINT8 *fmt;
    INT16 args;                   /* may have any number of args */
{
    INT16 *argp;
    UINT8 c;

    argp = &args;
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

void
prn(n, b)
    register UINT16 n, b;
{
    static UINT8 c;

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

void
prs(s)
    UINT8 *s;
{
    UINT8 c;

    while ((c = *s++) != 0)
        console(c);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
