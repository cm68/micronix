/*
 * fp - dump and restore files onto floppy volumes
 *
 * cmd/fp/extract.c
 *
 * The x (and p) option: pull files back off the volume set.  A file that
 * spans volumes is reassembled from its chapters, which are tied
 * together by their stored modification time.
 */

#include "fp.h"

void exreel();
struct entry *exone();
struct entry *exsplit();
int exfile();

int
extract()
{
    if (!readdir())
        return NO;

    for (;;) {
        if (!reeldone(super.reel))
            exreel();

        if (alldone())
            break;

        change(STILLMORE);
    }

    missing();
}

void
exreel()
{
    char name[PATHSIZE];
    static struct entry *t, *u;

    db("exreel ()\n");

    /*
     * scan the directory
     */
    for (t = tapedir; t < &tapedir[dirsize]; t++) {
        if (isentdone(t))
            continue;              /* already done */

        if (!inlist(t->name)) {    /* wasn't asked for */
            entdone(t);
            continue;
        }

        if (!istapedir(t)) {
            t = exone(t);          /* not a directory */
            continue;
        }

        /* directory */
        if (vflag)
            display("x", t);

        mkdir(t->name, t->mode, (t->group << 8) | t->user);
        entdone(t);

        if (nfiles > 0) {
            strcpy(name, t->name);

            for (u = tapedir; u < &tapedir[dirsize]; u++) {
                /* a descendant of the directory just made */
                if (strncmp(u->name, name, strlen(name)) == 0 && *u->name)
                    u = exone(u);
            }
        }
    }
}

struct entry *
exone(t)
    struct entry *t;
{
    db(t->name);
    db(" exone ()\n");

    if (ispartial(t))
        t = exsplit(t);
    else
        exfile(t);

    return t;
}

/*
 * exsplit - extract a file whose chapters are spread over more than one
 * volume.  It is passed the directory entry to start with and returns
 * the entry it finishes with (possibly on a different volume).
 */
struct entry *
exsplit(t)
    struct entry *t;
{
    char nchapter, nfinished, lastseen;
    char name[PATHSIZE];
    long time;

    db(t->name);
    db(" exsplit ()\n");

    strcpy(name, t->name);         /* save the file name */
    nfinished = 0;
    lastseen = NO;

    /*
     * make sure the extraction starts with chapter zero
     */
    while (t && t->chapter != 0) {
        change(ONEFIRST);
        t = tapestat(name);
    }

    /*
     * save the mod time.  This is what ties the chapters of the file
     * together.
     */
    time = t->time;

    for (;;) {
        if (!isentdone(t)) {
            exfile(t);
            nfinished++;
        }

        if (islastent(t)) {
            lastseen = YES;
            nchapter = t->chapter + 1;
        }

        if (lastseen && nfinished == nchapter)
            break;

        for (;;) {
            change(STILLMORE);

            t = tapestat(name);

            if (t) {
                if (t->time == time)
                    break;
                else
                    prs("This part of the file is from the wrong set.\n");
            }
        }
    }

    return t;
}

/*
 * exfile - extract a single chapter of a file.  It is guaranteed not to
 * change the volume.
 */
int
exfile(t)
    struct entry *t;
{
    char *linkname;
    static int f, type;
    struct stat s;

    entdone(t);

    db(t->name);
    db(" exfile ()\n");

    if (vflag)
        display("x", t);

    if (t->links > 1 && t->chapter == 0) {    /* candidate for linking */
        s.st_dev = (t->major << 8) | t->minor;
        s.st_ino = t->inumber;

        linkname = lookup(t->name, &s);
    } else {
        linkname = NULL;
    }

    /*
     * yes there was a previous name
     */
    if (linkname)
        if (link(linkname, t->name) < 0)
            perror(t->name);

    /*
     * special file extraction (easy)
     */
    type = t->mode & S_IFMT;

    if (type == S_IFCHR || type == S_IFBLK) {
        if (linkname == NULL) {    /* first or only file */
            if (mknod(t->name, t->mode, t->device) < 0) {
                perror(t->name);
                return NO;
            }
        }

        return YES;
    }

    /*
     * Come up with an output file descriptor.  This section assumes that
     * chapter zero is encountered first, which is checked for above.
     */

    /*
     * if pflag is set we aren't even creating files, so write to stdout.
     */
    if (pflag) {
        f = STDOUT;
    }

    /*
     * A plain file that carries no data: if it is the first instance,
     * create an empty file; otherwise the link above has done the work.
     */
    else if (t->flags & LINK) {
        if (linkname == NULL)      /* first instance */
            close(newfile(t));     /* empty file */

        return YES;
    }

    else if (t->chapter == 0 || !fexists(t->name)) {
        f = newfile(t);
    }

    else {
        f = open(t->name, WRITE);
    }

    if (f < 0) {
        perror(t->name);
        return NO;
    }

    seek(fd, t->block, BYBLOCK);
    seek(f, t->offset, BYBLOCK);

    fcopy(fd, f, t->length, t->name, ROUND);

    if (f != STDOUT)
        close(f);

    return YES;
}
