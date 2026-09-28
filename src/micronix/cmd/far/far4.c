#include "far.h"

/*
 * interactive info gathering
 */
inter()
{
    static char devbuf[128], filebuf[128], *f;
    char buf[BUFSIZE];
    struct stat s;

    if (gtty(STDIN, buf) < 0 || gtty(STDOUT, buf) < 0) {
        put("usage: far device {d,r,x,t} files ...\n");
        exit(NO);
    }

    f = filebuf;
    files = &f;
    nfiles = 1;
    device = devbuf;
    verbose = YES;

    for (;;) {
        put("Insert diskette and close the drive door\n");

        put("Device name (such as mfa) (or <RUB-OUT> to quit): ");

        gets(device);

        if (feof(stdin))
            exit(YES);

        if (!strchr(device, '/') && strlen(device) <= 14 && !fexists(device)) {
            strcpy(buf, "/dev/");
            strcat(buf, device);
            strcpy(device, buf);
        }

        if (stat(device, &s) < 0) {
            perror(device);
            continue;
        }

        if ((s.st_mode & S_IFMT) != S_IFBLK) {
            put(device);
            put(": Not special file\n");
            continue;
        }

        break;
    }

    for (;;) {
        put("\n");
        put("\td	Delete a file.\n");
        put("\tp	Print a file.\n");
        put("\tr	Replace a file.\n");
        put("\tx	Extract a file.\n");
        put("\tt	Table of contents.\n");

        put("\nSelect a function (or <RUB-OUT> to quit): ");

        gets(buf);

        if (feof(stdin))
            exit(YES);

        *buf = tolower(*buf);

        if (*buf == 'd') {
            put("File to delete: ");
            gets(filebuf);

            if (feof(stdin))
                exit(YES);

            dflag = YES;
            break;
        }

        else if (*buf == 'p') {
            put("File to print: ");
            gets(filebuf);

            if (feof(stdin))
                exit(YES);

            pflag = YES;
            break;
        }

        else if (*buf == 'r') {
            put("File to replace: ");
            gets(filebuf);

            if (feof(stdin))
                exit(YES);

            rflag = YES;
            break;
        }

        else if (*buf == 't') {
            nfiles = 0;
            tflag = YES;
            break;
        }

        else if (*buf == 'x') {
            put("File to extract (or RETURN for all files): ");
            gets(filebuf);

            if (feof(stdin))
                exit(YES);

            if (!*filebuf)
                nfiles = 0;

            xflag = YES;
            break;
        }
    }
}

duptest()
{
    static UINT i, j;

    for (i = 0; i < nfiles; i++) {
        for (j = 0; j < nfiles; j++) {
            if (i != j && strcmp(files[i], files[j])) {
                put(files[i]);
                put(": duplicate file name\n");
                exit(NO);
            }
        }
    }
}

/*
 * is this a CP/M formatted diskette ?
 * return YES or NO
 */

iscpm()
{
    static struct fcb *f;
    static int m, printed = NO;
    static char n;

    m = 0;

    for (f = thedir; f < &thedir[d->epd]; f++) {
        if (f->user == NOENT)
            continue;

        if (15 < f->user)
            return NO;          /* heinous error */

        for (n = 0; n < sizeof(f->name); n++)
            if (!okchar(f->name[n]))
                return NO;

        for (n = 0; n < sizeof(f->type); n++)
            if (!okchar(f->type[n]))
                return NO;

        if (d->bpv == 330 && d->epe == 2) {     /* the 5 1/4 quirk */
            static unsigned i, p, rc;

            /*
             * count the number of allocated group pointers
             */

            p = 0;

            for (i = 0; i < 16; i++)
                if (f->point.byte[i])
                    p++;

            /*
             * this directory entry has p allocated pointers
             * Let's assume we have 2K groups
             */

            rc = f->rc;         /* the raw record count */

            if (f->ex & 1)      /* odd extent number */
                rc += 128;
            /*
             * now rc is the true number of records represented by this dir. entry
             */

            if ((rc + 15) / 16 != p)
                return NO;      /* must be 1.4 format */

            if (m < rc)
                m = rc;         /* find a maximum */
        }
    }

    if (d->bpv == 330 && d->epe == 2 && m <= 16 && !printed) {
        eput("Can't determine diskette format\n");
        eput("We will assume Northstar CP/M 2.x\n");
        eput("Note: a file longer that 2K is needed on the diskette\n");
        eput("to make an unambiguous CP/M format determination.\n");
        printed = YES;
    }

    return YES;
}

/*
 * is the CP/M directory empty and never used, freshly formatted.
 * return YES or NO
 */

isempty()
{
    static unsigned char *p;
    static int n;

    p = (unsigned char *) thedir;
    n = d->epd * sizeof(*thedir);

    for (; n; n--, p++)
        if (*p != NOENT)
            return NO;

    return YES;
}

tail(a)
{
    static char buf[BUFSIZE], *p;

    strcpy(buf, a);

    p = buf + strlen(buf) - 1;

    while (p > buf && *p == '/')
        *p-- = NULL;

    p = buf;

    while (strchr(p, '/'))
        p++;

    return p;
}

/*
 * fdelete - delete the given file (all extents)
 */

fdelete(a)
{
    char buf[32];
    static struct fcb *f;

    for (f = thedir; f < &thedir[d->epd]; f++) {
        if (f->user == userno) {        /* dir slot in use */
            name(f, buf);

            if (strcmp(a, buf)) {       /* right file */
                f->user = NOENT;        /* delete it */
                ddirty = YES;   /* mark dir. dirty */
            }
        }
    }
}

/*
 * generate a diskette space report
 */

space()
{
    static int used, free, percent, g;
    static long l;

    findfree();

    used = 0;
    free = 0;

    for (g = d->gdir; g < d->ngroup; g++) {
        if (gmap[g])
            used++;
        else
            free++;
    }

    l = used;
    l *= 100;
    l /= (used + free);

    percent = l;

    put("Diskette ");
    putnum(percent), put("% full, ");

    l = free;                   /* number of groups free */
    l *= d->bpg;                /* number of bytes free */
    l /= 1000;                  /* number of K free */
    free = l;

    putnum(free), put("K free space left\n");
}

char *
itoa(a)
{
    static char buf[16];

    sprintf(buf, "%d", a);
    return buf;
}

putnum(a)
{
    put(itoa(a));
}

readdir()
{
    static int g;
    static struct fcb *f;

    f = thedir;

    for (g = 0; g < d->gdir; g++) {
        if (cflag)
            memset(f, NOENT, d->bpg);

        else
            gio(g, f, READ);

        f += d->epg;            /* advance to the next group */
    }
}

writedir()
{
    static int g;
    static struct fcb *f;

    f = thedir;

    for (g = 0; g < d->gdir; g++) {
        gio(g, f, WRITE);

        f += d->epg;            /* advance to the next group */
    }
}

/*
 *      a is the file name
 *      b is the matching expression
 */

int
match(a, b)
    char *a, *b;
{
    if (*a == NULL && *b == NULL)       /* end simultaneously */
        return YES;

    if (*b == NULL)             /* pattern ends */
        return NO;

    if (*b == '*') {
        char *p;

        for (p = a + strlen(a); a <= p; p--)
            if (match(p, b + 1))
                return YES;

        return NO;
    }

    if (*a == NULL)             /* text ends */
        return NO;

    if (omatch(a, b))
        return match(a + 1, nextpat(b));

    else
        return NO;
}

int
omatch(a, b)
    char *a, *b;
{
    int sense;
    char *start;

    if (*a != NULL && *b == '?')
        return YES;

    else if (*b == ESCAPE && b[1])
        return *a == b[1];

    else if (*b != '[')
        return *a == *b;

    b++;                        /* [ .... ] form */

    if (*b == NEGATE) {
        sense = NO;
        b++;
    } else {
        sense = YES;
    }

    start = b;                  /* mark the starting pos. */

    for (;;) {
        if (*b == ']' || !*b) {
            return !sense;
        }

        else if (*b == ESCAPE && b[1]) {
            if (*a == b[1])
                return sense;
            else
                b += 2;
        }

        else if (*b == '-' && b != start && alphanum(b[1])
            && alphanum(b[-1])
            ) {
            if (b[-1] <= *a && *a <= b[1])
                return sense;

            else
                b += 2;
        }

        else {
            if (*a == *b)
                return sense;
            else
                b++;
        }
    }

    return NO;
}

char *
nextpat(a)
    char *a;
{
    if (*a == ESCAPE && a[1])
        return a + 2;

    if (*a == '[') {
        a++;

        while (*a) {
            if (*a == ESCAPE && a[1])
                a += 2;

            else if (*a == ']')
                return a + 1;

            else
                a++;
        }

        return a;
    }

    return a + 1;
}

int
alphanum(a)
{
    return isdigit(a) || isalpha(a);
}

/*
 * okchar - test to determine whether the given character
 *              is valid in a CP/M directory name.
 */
#define HIGHBIT (1 << 7)

okchar(a)
    unsigned char a;
{
    a &= ~HIGHBIT;              /* ignore the high order bit */

    return a == ' ' || (isblack(a) && !islower(a));
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab: 
 */
