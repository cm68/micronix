/*
 * fp - dump and restore files onto floppy volumes
 *
 * cmd/fp/file.c
 *
 * Helpers for turning a Micronix file into (and back out of) a tape
 * directory entry.
 */

#include "fp.h"

/*
 * bsize - the size of a file in 512-byte blocks, rounded up.
 */
int
bsize(a)
    struct stat *a;
{
    unsigned long b;

    b = fsize(a);

    return (short) ((b + 511) / 512);
}

/*
 * fsize - a file's size, reassembled from the inode's 24-bit field.
 * st_size0 is the high byte, st_size1 the low word.
 */
long
fsize(a)
    struct stat *a;
{
    unsigned long b;

    b = a->st_size0;

    b <<= 16;

    b |= a->st_size1;

    return b;
}

int
fexists(a)
    char *a;
{
    struct stat s;

    return stat(a, &s) >= 0;
}

/*
 * newfile - create the file a directory entry names, making its parent
 * directory first if the create failed because the path was not there.
 * The owner is the packed (group << 8) | user the entry carries.
 */
int
newfile(t)
    struct entry *t;
{
    char name[PATHSIZE];
    int f;

    f = creat(t->name, t->mode);

    if (f < 0 && (errno == ENOENT || errno == ENOTDIR)) {
        strcpy(name, parent(t->name));
        mkdir(name, 0, 0);
        f = creat(t->name, t->mode);
    }

    chown(t->name, (t->group << 8) | (t->user & 0377));

    return f;
}

int
isdir(a)
    char *a;
{
    struct stat s;

    if (stat(a, &s) < 0)
        return NO;

    return ftype(&s) == S_IFDIR;
}

int
ftype(a)
    struct stat *a;
{
    return a->st_mode & S_IFMT;
}
