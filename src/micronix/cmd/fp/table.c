/*
 * fp - dump and restore files onto floppy volumes
 *
 * cmd/fp/table.c
 *
 * The t (table) option: list the contents of the volume.
 */

#include "fp.h"

void tableone();

int
table()
{
    char **f;
    static struct entry *t;

    if (!readdir())
        return NO;

    if (vflag)         /* announce the reel number */
        pvol();

    for (t = tapedir; t < &tapedir[dirsize]; t++)
        if (inlist(t->name))
            tableone(t);

    missing();
}

void
tableone(t)
    struct entry *t;
{
    int type;

    type = t->mode & S_IFMT;

    if (vflag) {
        plength(t->length);
        ptime(t->time);
    }

    prs(t->name);
    ppart(t);

    if (t->flags & LINK)
        prs(" [link]");

    if (type == S_IFCHR || type == S_IFBLK)
        prs(" [special]");

    if (type == S_IFDIR)
        prs(" [directory]");

    prs("\n");
}
