#include "far.h"

/*
 * make sure the given arg does not refer to a mounted floppy
 */
mcheck(a, s)
    char *a;
    struct stat *s;
{
    char *p, buf[32], f;
    struct mtab mtab;
    struct stat st;

    /*
     * read the mount table, statting each entry
     */

    f = open(MTAB, READ);

    if (f < 0)                  /* can't open mtab */
        return;

    for (;;) {
        static n;

        n = read(f, &mtab, sizeof(mtab));

        if (n != sizeof(mtab))
            break;              /* end of file */

        if (!*mtab.special || !*mtab.directory)
            continue;           /* unused entry */

        /*
         * build full path name
         */

        cpystr(buf, "/dev/", mtab.special, NULL);

        if (stat(buf, &st) < 0)
            continue;

        if (st.addr[0] != s->addr[0])
            continue;           /* different major dev no. */

        errno = EBUSY;
        perror(mtab.special);
        exit(NO);
    }

    close(f);
}

fexists(a)
{
    struct stat s;

    return stat(a, &s) >= 0;
}

far()
{
    INTERN mode;
    struct stat s;

    if (dflag || rflag)
        mode = UPDATE;
    else
        mode = READ;

    if (stat(device, &s) < 0) {
        perror(device);
        exit(NO);
    }

    /*
     * make sure we are dealing with a block special file
     */

    if ((s.flags & S_TYPE) != S_ISBLOCK) {
        put(device);
        put(": Not block special\n");
        exit(NO);
    }

    /*
     * make sure the device or its alternate are not mounted
     */

    mcheck(device, &s);

    fd = open(device, mode);    /* open the device */

    if (fd < 0)
        fd = open(device, mode);        /* try twice */

    if (fd < 0)
        perror(device), exit(NO);

    alt = !!(s.addr[0] & ALT);

    findsize();

    readdir();

    if (mode == UPDATE && !iscpm()) {
        put("Not a CP/M diskette!\n");
        exit(NO);
    }

    /*
     * read the CP/M directory
     */

    if (dflag)
        delete();
    else if (rflag)
        replace();
    else if (tflag)
        table();
    else if (xflag || pflag)
        extract();

    /*
     * write the CP/M directory back out again if need be
     */

    if (ddirty)
        writedir();
}

table()
{
    TEXT buf[32];
    INTERN n;
    INTERN struct fcb *f;

    for (f = thedir; f < &thedir[d->epd]; f++) {
        unsigned ex;

        ex = f->ex & 31;
        ex |= f->s2 << 5;

        if (f->user != userno || ex >= d->epe)
            continue;

        name(f, buf);

        n = isfile(buf);

        if (n < 0)
            continue;           /* not asked for */

        complete[n] = YES;

        tableone(f);
    }
}

tableone(f)
    FAST struct fcb *f;
{
    TEXT buf[32];

    name(f, buf);
    put(buf);
    put("\n");
}

/*
 * convert a CP/M FCB to a UNIX file name
 */
name(a, c)
    FAST struct fcb *a;
    FAST TEXT *c;
{
    INTERN TEXT z;
    INTERN UTINY i;

    for (i = 0; i < 8; i++) {
        z = a->name[i];

        z &= 0177;              /* strip high bit */
        z = tolower(z);         /* lower case */

        if (z == ' ')           /* no more */
            break;

        if (z == '/')
            z = '%';

        *c++ = z;
    }

    for (i = 0; i < 3; i++) {
        z = a->type[i];

        z &= 0177;              /* strip high bit */
        z = tolower(z);         /* lower case */

        if (z == ' ')           /* no more */
            break;

        if (!i)
            *c++ = '.';

        if (z == '/')
            z = '%';

        *c++ = z;
    }

    *c = NULL;
}

/*
 * cname - transcribe a unix name into a fcb block at b
 *      act as the user would
 */
cname(a, b)
    FAST TEXT *a;
    FAST struct fcb *b;
{
    FAST TEXT *per;
    INTERN TEXT z;
    INTERN UTINY i;

    a = tail(a);

    fill(b, sizeof(*b), NULL);

    /*
     * find the last period
     */

    for (per = a + lenstr(a) - 1; per >= a; per--)
        if (*per == '.')
            break;

    if (per < a)
        per = NULL;

    fill(b->name, sizeof(b->name), ' ');
    fill(b->type, sizeof(b->type), ' ');

    for (i = 0; i < sizeof(b->name); i++) {
        if (!*a || a == per)
            break;

        z = *a++;
        z &= 0377;
        z = toupper(z);

        b->name[i] = z;

    }

    if (per) {
        a = per + 1;
        for (i = 0; i < sizeof(b->type); i++) {
            if (!*a)
                break;

            z = *a++;
            z &= 0377;
            z = toupper(z);

            b->type[i] = z;
        }
    }
}

/*
 * extract each matching file
 */
extract()
{
    TEXT buf[32];
    INTERN n;
    struct fcb *f;
    INTERN UTINY g;

    for (f = thedir; f < &thedir[d->epd]; f++) {
        unsigned ex;

        ex = f->ex & 31;
        ex |= f->s2 << 5;

        if (f->user != userno || ex >= d->epe)
            continue;

        name(f, buf);

        n = isfile(buf);

        if (n >= 0) {
            complete[n] = YES;
            exone(f, buf);
        }
    }
}

/*
 * extract one file
 */
exone(a, b)
    FAST struct fcb *a;
    FAST TEXT *b;
{
    INTERN BOOL found;
    INTERN out;
    INTERN UCOUNT ex, g;
    struct fcb *f;

    if (verbose)
        put("x "), put(b), put("\n");

    if (xflag) {
        out = creat(b, 0666);

        if (out < 0) {
            perror(b);
            return;
        }

    }

    for (ex = 0;; ex += d->epe) {
        found = NO;

        for (f = thedir; f < &thedir[d->epd]; f++) {
            if (f->user != userno)
                continue;

            if (!cmpbuf(f->name, a->name, 11))
                continue;

            if (d->epe == 1 && ex != (f->ex | (f->s2 << 5)))
                continue;

            if (d->epe == 2 && ex != (f->ex | (f->s2 << 5))
                && ex + 1 != (f->ex | (f->s2 << 5))
                ) {
                continue;
            }

            found = YES;

            if (pflag)
                readex(f, STDOUT, b);

            if (xflag)
                readex(f, out, b);

            break;
        }

        if (!found)
            break;              /* no more extents */
    }

    if (xflag)
        close(out);
}

readex(a, b, c)
    FAST struct fcb *a;
    FAST b;                     /* file desc. */
    FAST TEXT *c;               /* file name */
{
    TEXT buf[GSIZE];
    INTERN UCOUNT g, nrecs, nbytes, togo, p, rpg;

    rpg = (d->bps / 128) * d->spg;      /* recs. per grp. */
    nrecs = a->rc;              /* no. of records left in this extent */

    if (d->epe == 2 && (a->ex & 1))     /* odd numbered extent */
        nrecs += 128;

    for (p = 0; p < d->npoint; p++) {   /* for each group pointer */
        if (!nrecs)
            break;              /* no more to read */

        if (d->npoint == NPOINT)        /* word-size pointers */
            g = a->point.word[p];       /* group no. */
        else
            g = a->point.byte[p];

        if (g == 0)             /* unallocated group pointer */
            break;

        gio(g, buf, read);      /* read the group */

        togo = nrecs;           /* no. of records to transfer this time */

        if (togo > rpg)         /* at most one group at a time */
            togo = rpg;

        nbytes = togo * 128;    /* no. of bytes to write */

        if (write(b, buf, nbytes) != nbytes) {
            perror(c);
            return NO;
        }

        nrecs -= togo;          /* accounting */
    }

    return YES;
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab: 
 */

