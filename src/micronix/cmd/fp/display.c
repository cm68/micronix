/*
 * fp - dump and restore files onto floppy volumes
 *
 * cmd/fp/display.c
 *
 * The verbose output: volume headers, one line per file, dates and the
 * free-space report.
 */

#include "fp.h"

void prn();

void
pvol()
{
    prs("Volume ");
    prn(super.reel + 1);
    prs("   ");
    ptime(super.time);

    prs(label.text);

    prs("\n");
}

void
prs(a)
    char *a;
{
    fputs(a, stdout);
}

void
display(a, t)
    char *a;
    struct entry *t;
{
    prs(a);
    prs(" ");
    prs(t->name);
    ppart(t);
    prs("\n");
}

static char *month[] = {
    "Jan",
    "Feb",
    "Mar",
    "Apr",
    "May",
    "Jun",
    "Jul",
    "Aug",
    "Sep",
    "Oct",
    "Nov",
    "Dec",
};

/*
 * ptime - print a date in ls's style: "Mar 27 14:03" for a recent file,
 * "Mar 27  1983" for one a year or more away.  A leading zero in the day
 * or hour is blanked, not shown.
 */
void
ptime(a)
    unsigned long a;
{
    static char buf[32], *b;
    static struct tm *t;
    static unsigned char c;

    b = buf;

    t = localtime(&a);

    /* month */
    strcpy(b, month[t->tm_mon]);
    b += strlen(b);

    *b++ = ' ';

    /* day */
    c = t->tm_mday / 10;
    *b++ = c ? c + '0' : ' ';
    *b++ = t->tm_mday % 10 + '0';

    *b++ = ' ';

    if (a < yearago || yearfromnow < a) {
        /* remote in time: show the year */
        *b++ = ' ';
        b += sprintf(b, "%d", t->tm_year + 1900);
    } else {
        /* hour */
        c = t->tm_hour / 10;
        *b++ = c ? c + '0' : ' ';
        *b++ = t->tm_hour % 10 + '0';

        *b++ = ':';

        /* minute */
        *b++ = t->tm_min / 10 + '0';
        *b++ = t->tm_min % 10 + '0';
    }

    *b++ = ' ';
    *b = 0;
    prs(buf);
}

/*
 * plength - print a length right-justified in a 10-column field.
 */
void
plength(a)
    long a;
{
    static char buf[16];
    static int n;

    n = sprintf(buf, "%ld", a);

    n = 10 - n;

    while (n-- > 0)
        prs(" ");

    prs(buf);
    prs(" ");
}

void
ppart(t)
    struct entry *t;
{
    if (ispartial(t)) {
        prs(" [part ");
        prn(t->chapter + 1);
        prs("]");
    }
}

void
prn(a)
    int a;
{
    char buf[8];

    buf[sprintf(buf, "%d", a)] = 0;
    prs(buf);
}

/*
 * space - generate a diskette space report.
 */
void
space()
{
    static int used, nfree, percent, g;
    static long l;

    nfree = freespace;
    used = ((super.vsize - super.dsize) - DIRBLOCK) - freespace;

    l = used;
    l *= 100;
    l /= (used + nfree);

    percent = l;

    prs("Volume ");
    prn(percent);
    prs("% full, ");

    l = nfree;         /* number of groups free */
    l *= BUFSIZE;      /* number of bytes free */
    l /= 1000;         /* number of K free */
    nfree = l;

    prn(nfree);
    prs("K free space left");

    if (super.flags & CONTINUED)
        prs(" [continued on next volume]");

    prs("\n");
}

void
db(a)
    char *a;
{
    if (debug)
        prs(a);
}
