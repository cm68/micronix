/*
 * fp - dump and restore files onto floppy volumes
 *
 * cmd/fp/incr.c
 *
 * The incremental dump table, /etc/dtab, one record per dumped
 * directory: its device, inode and the date of the last dump.
 */

#include "fp.h"

/*
 * tabread - read the dump table and return the dump time for the given
 * device and inode, or 0 if there is no record.
 */
long
tabread(dev, ino)
    int dev, ino;
{
    static int f;
    struct dtab b;

    f = open(TAB, READ);

    if (f < 0) {               /* no dump table file, so make one */
        close(creat(TAB, 0666));
        return 0L;
    }

    for (;;) {
        if (read(f, &b, sizeof(b)) != sizeof(b)) {
            close(f);
            return 0L;         /* didn't find it */
        }

        if (dev == b.device && ino == b.inode) {
            close(f);
            return b.date;
        }
    }
}

void
tabwrite(d)
    struct dtab *d;
{
    static int f;
    struct dtab b;

    if (!fexists(TAB))
        close(creat(TAB, 0666));

    f = open(TAB, UPDATE);

    for (;;) {
        if (read(f, &b, sizeof(b)) != sizeof(b))
            goto wr;

        if (b.device == d->device && b.inode == d->inode) {
            seek(f, -sizeof(b), 1);

        wr:
            if (write(f, d, sizeof(*d)) != sizeof(*d))
                perror(TAB);

            close(f);
            return;
        }
    }
}
