#include "far.h"

/*
 * delete each named file (all extents)
 */
delete()
{
    char buf[32];
    INTERN n;
    INTERN struct fcb *f;

    if (!nfiles)
        return;                 /* don't delete everything ! */

    for (f = thedir; f < &thedir[d->epd]; f++) {
        if (f->user != userno)  /* ignore unused slots */
            continue;

        name(f, buf);

        n = isfile(buf);

        if (n < 0)
            continue;

        if (f->s2 == 0 && f->ex < d->epe) {     /* file's principal entry */
            complete[n] = YES;

            if (verbose && dflag) {
                put("d ");
                put(buf);
                put("\n");
            }
        }

        f->user = NOENT;
        ddirty = YES;           /* arrange to have the dir. written */
    }
}

/*
 * set the d structure pointer
 * a is the file descript
 */
findsize()
{
    static unsigned s;

    /*
     * measure the device
     */

    s = devsize(fd);

    /*
     * scan the table
     */

    for (d = disk; d->bpv; d++) {
        if (d->bpv == s) {      /* found it in the table */
            if (s == 400 || s == 975 || s == 330) {

                /*
                 * OK, so we've got an ambiguous size.
                 * That could be one of 2 formats
                 * The first is the default.
                 * Let's try to distinguish ...
                 */

                /*
                 * First we try reading the directory in format 1
                 * If we get positive results, then that's enough.
                 */

                readdir();

                if (iscpm() && !isempty()) {
                    return;
                }

                /*
                 * Inconclusive ... so let's try reading the alternate format.
                 */

                d++;

                readdir();

                if (iscpm() && !isempty()) {
                    return;
                }

                d--;            /* Back out. */
            }

            return;
        }
    }

    putstr(STDERR, "Device size ", itoa(s), ": Unknown format\n", NULL);

    exit(NO);
}

/*
 * replace all the named files
 */
replace()
{
    INTERN UCOUNT i;

    /*
     * make sure in advance that all the named files
     * are readable
     */

    for (i = 0; i < nfiles; i++)
        if (access(files[i], A_READ) < 0)
            perror(files[i]), exit(NO);

    /*
     * delete all the named files
     */

    delete();

    /*
     * make a map of the free groups on the diskette
     */

    findfree();

    for (i = 0; i < nfiles; i++) {
        if (verbose)
            put("r "), put(files[i]), put("\n");

        if (!repone(files[i]))
            break;

        complete[i] = YES;
    }
}

repone(a)
    FAST TEXT *a;
{
    INTERN BOOL found;
    INTERN in;
    INTERN UCOUNT ex, nex, g;   /* number of extents */
    INTERN LONG l;
    INTERN struct fcb *f;
    INTERN struct stat s;

    in = open(a, READ);

    if (in < 0)
        perror(a), exit(NO);

    /*
     * find the number of extents
     */

    if (fstat(in, &s) < 0)
        perror(a), exit(NO);

    l = s.size0;
    l <<= 16;
    l |= s.size1;               /* bytes in the file */

    l += d->bpe - 1;
    l /= d->bpe;

    nex = l;                    /* no. of dir. entries needed */

    if (nex == 0)
        nex = 1;                /* always at least one extent */

    for (ex = 0; ex < nex; ex++) {      /* write each extent */
        found = NO;             /* haven't yet found a slot for this ext. */

        for (f = thedir; f < &thedir[d->epd] && !found; f++) {
            if (f->user != NOENT)
                continue;

            cname(a, f);

            f->user = userno;

            f->ex = d->epe * ex;

            f->s2 = f->ex >> 5;

            f->ex &= 31;

            if (!exwrite(f, in, a))
                return NO;

            ddirty = found = YES;
        }

        if (!found) {           /* there wasn't a parking spot */
            put("Diskette directory full\n");
            return NO;
        }
    }

    close(in);
    return YES;
}

/*
 * write one extent
 */
BOOL
exwrite(a, b, c)
    FAST struct fcb *a;         /* make changes in this fcb */
    FAST b;                     /* read from this file desc. */
    FAST TEXT *c;               /* the unix file name if you need it */
{
    TEXT buf[GSIZE];
    INTERN n;
    INTERN UCOUNT i, nrec, new, totrec;

    totrec = 0;

    for (i = 0; i < d->npoint; i++) {   /* fill each extent pointer */
        n = read(b, buf, d->bpg);       /* read a bit of file */

        if (n < 0) {            /* error */
            perror(c);
            return NO;
        }

        if (n == 0)
            break;              /* end of file */

        if (n != d->bpg)        /* partial read */
            fill(buf + n, d->bpg - n, 26);

        /*
         * no. of records garnered this read
         */

        nrec = (n + RECSIZE - 1) / RECSIZE;

        new = galloc();         /* allocate a group */

        if (!new) {
            put("Not enough space left on diskette\n");
            fdelete(c);
            return NO;
        }

        gio(new, buf, write);

        if (d->npoint == NPOINT)        /* word-size pointers */
            a->point.word[i] = new;
        else
            a->point.byte[i] = new;

        totrec += nrec;         /* update this extents record count */
    }

    if (totrec > 128) {         /* this is goin' ta be a double ext. */
        a->rc = totrec - 128;

        if (++a->ex == 32) {
            a->ex = 0;
            a->s2++;
        }
    } else {
        a->rc = totrec;
    }

    return YES;
}

findfree()
{
    INTERN UCOUNT group, p;
    INTERN struct fcb *f;

    fill(gmap, d->ngroup, NULL);        /* all unused */

    for (f = thedir; f < &thedir[d->epd]; f++) {        /* for each dir. ent. 
                                                         */
        if (f->user == NOENT)   /* unused */
            continue;

        for (p = 0; p < d->npoint; p++) {
            if (d->npoint == NPOINT)
                group = f->point.word[p];
            else
                group = f->point.byte[p];

            if (group && group < d->ngroup)
                gmap[group] = YES;
        }
    }
}

galloc()
{
    INTERN UCOUNT g = 10000, n;

    for (n = d->ngroup - d->gdir; n; n--, g++) {
        if (d->ngroup <= g)
            g = d->gdir;

        if (!gmap[g]) {
            gmap[g] = YES;
            return g++;
        }
    }

    return NULL;                /* could't find it */
}

isfile(a)
    FAST TEXT *a;
{
    INTERN UCOUNT i;

    if (!nfiles)
        return YES;

    for (i = 0; i < nfiles; i++) {
        if (rflag) {            /* for replacement, names must match exactly */
            if (cmpstr(a, files[i])) {
                return i;
            }
        } else {
            if (match(a, files[i])) {
                return i;
            }
        }
    }

    return -1;
}

usage()
{
    put("usage: far device {dprtx} files ...\n");
    exit(NO);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab: 
 */

