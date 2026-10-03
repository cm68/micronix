/*
 * fp - dump and restore files onto floppy volumes
 *
 * cmd/fp/acct.c
 *
 * Accounting: which files and which reels have been dealt with.  Each
 * reel has a bitmap of the entries in its tape directory; a bit set
 * means that entry is done.  This is what lets extract know when every
 * chapter of a multi-volume file has arrived and when it can stop asking
 * for the next volume.
 */

#include "fp.h"

struct reeltab {
    int size;
    char *dir;        /* bit map of completed entries */
};

static int
    lastseen = NO,    /* the last reel has been seen */
    nreel = 0,        /* the high water mark for reel numbers */
    tabsize = 0;      /* number of entries in reeltab */

static struct reeltab reeltab[MAXREELS] = {0};

/*
 * missing - report the files the user asked for but never saw.
 */
void
missing()
{
    static char **f;

    if (nfiles <= 0)
        return;

    for (f = files; f < &files[nfiles]; f++) {
        if (*f) {
            prs(*f);
            prs(": file not found\n");
        }
    }
}

void
namedone(a)
    char *a;
{
    int n;

    db(a);
    db(" namedone ()\n");

    for (n = 0; n < nfiles; n++)
        if (!strcmp(a, files[n]))
            files[n] = NULL;
}

/*
 * read a bit from a bit map
 */
int
rbit(a, b)
    char *a;
    int b;
{
    int bit, byte;

    byte = b / 8;
    bit = b % 8;

    return (a[byte] >> bit) & 1;
}

/*
 * write a bit to a bit map
 */
void
wbit(a, b)
    char *a;
    int b;
{
    int bit, byte;

    byte = b / 8;
    bit = b % 8;

    a[byte] |= (1 << bit);
}

/*
 * are we all finished?
 */
int
alldone()
{
    static int n;

    db("alldone ()\n");

    if (nfiles > 0) {          /* user-specified file list */
        int nleft;             /* count how many undone */

        nleft = 0;

        for (n = 0; n < nfiles; n++)
            if (files[n])
                nleft++;

        if (nleft == 0)        /* no more */
            return YES;
    }

    if (!lastseen)
        return NO;             /* haven't yet seen the last reel, */
                               /* so we're not done yet */

    for (n = 0; n < nreel; n++) {
        if (!reeldone(n)) {
            db("reeldone NO\n");
            return NO;         /* an intermediate reel isn't done yet */
        }
    }

    return YES;
}

/*
 * tell whether an individual reel is all done.  Observe that directories
 * always have a multiple of 8 entries.
 */
int
reeldone(a)
    int a;
{
    static char *d;
    static int n;

    db("reeldone () ");

    d = reeltab[a].dir;

    if (!d) {                  /* never seen this reel */
        db("NO\n");
        return NO;
    }

    for (n = reeltab[a].size / 8 - 1; n >= 0; n--)
        if (d[n] != ~0) {
            db("NO\n");
            return NO;
        }

    db("YES\n");
    return YES;
}

/*
 * register the fact that an entry has been processed
 */
void
entdone(t)
    struct entry *t;
{
    static struct reeltab *r;

    db(t->name);
    db(" entdone ()\n");

    namedone(t->name);

    r = &reeltab[super.reel];

    if (islastreel()) {
        lastseen = YES;
        nreel = super.reel + 1;
    }

    if (!r->dir) {
        r->dir = calloc(1, dirsize / 8);
        r->size = dirsize;
    }

    /*
     * set the appropriate bit in the bit map for the current directory
     */
    wbit(r->dir, t - tapedir);
}

/*
 * sense the fact that an entry has been processed
 */
int
isentdone(t)
    struct entry *t;
{
    static struct reeltab *r;

    r = &reeltab[super.reel];

    return r->dir && rbit(r->dir, t - tapedir);
}

/*
 * is the given file name in the user specified file list?
 */
int
inlist(a)
    char *a;
{
    char **f;

    if (!a || !*a)
        return NO;

    if (nfiles <= 0)
        return YES;

    for (f = files; f < &files[nfiles]; f++) {
        if (!strcmp(a, *f))
            return YES;
    }

    return NO;
}

int
islastreel()
{
    return !(super.flags & CONTINUED);
}

int
islastent(t)
    struct entry *t;
{
    return !(t->flags & CONTINUED);
}
