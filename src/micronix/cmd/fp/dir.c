/*
 * fp - dump and restore files onto floppy volumes
 *
 * cmd/fp/dir.c
 *
 * The tape directory: one struct entry per file, on block DIRBLOCK of
 * the volume, super.dsize blocks long.
 */

#include "fp.h"

#define ALLDIR (BUFSIZE * super.dsize)

int
readdir()
{
    struct entry *t;

    seek(fd, DIRBLOCK, BYBLOCK);

    if (read(fd, tapedir, ALLDIR) != ALLDIR) {
        prs("Volume unreadable.\n");
        return NO;
    }

    dirdirty = NO;

    if (cflag || newvolume) {
        for (t = tapedir; t < &tapedir[dirsize]; t++)
            *t->name = 0;

        newvolume = NO;
        super.free = DIRBLOCK + super.dsize;
        freespace = super.vsize - super.free;

        dirdirty = YES;
        superdirty = YES;
    }

    /*
     * mark all the empty entries "done" right at first
     */
    for (t = tapedir; t < &tapedir[dirsize]; t++)
        if (!*t->name)
            entdone(t);

    return YES;
}

/*
 * writedir - write out the tape directory
 */
void
writedir()
{
    struct entry *t;

    if (dirdirty) {
        seek(fd, DIRBLOCK, BYBLOCK);
        write(fd, tapedir, ALLDIR);

        dirdirty = NO;
    }

    writesuper();
}

int
ispartial(t)
    struct entry *t;
{
    return t->chapter != 0 || (t->flags & CONTINUED);
}

struct entry *
tapestat(a)
    char *a;
{
    struct entry *t;

    if (a == 0 || *a == 0)     /* trivial cases */
        return NULL;

    for (t = &tapedir[dirsize - 1]; tapedir <= t; t--)
        if (!strcmp(a, t->name))
            return t;

    return NULL;
}

int
istapedir(t)
    struct entry *t;
{
    return (t->mode & S_IFMT) == S_IFDIR;
}

/*
 * allocate a directory entry
 */
struct entry *
ealloc()
{
    static struct entry *t;

    for (t = tapedir; t < &tapedir[dirsize]; t++)
        if (*t->name == 0)
            return t;

    return NULL;
}
