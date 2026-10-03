/*
 * fp - dump and restore files onto floppy volumes
 *
 * cmd/fp/super.c
 *
 * The volume superblock, at block SUPER, and block allocation for the
 * replace path.
 */

#include "fp.h"

int
readsuper()
{
    seek(fd, SUPER, BYBLOCK);

    if (read(fd, &super, sizeof super) != sizeof super) {
        prs("Volume not readable\n");
        return NO;
    }

    /*
     * see if we should build a fresh super block
     */
    if (cflag
        || super.vsize == 0
        || super.free > super.vsize     /* ridiculous values */
        || super.dsize > super.vsize
        || sum(&super, sizeof super) != 0
        || super.dsize == FORMPATTERN
        || super.reel >= MAXREELS) {

        if (!rflag) {
            prs("Volume not in fp format.\n");
            return NO;
        }

        /*
         * set up a new super block
         */
        newvolume = YES;

        memset(&super, 0, sizeof super);

        super.vsize = devsize(fd);
        super.dsize = super.vsize / 80;

        if (super.dsize < MINDSIZE)
            super.dsize = MINDSIZE;

        super.free = DIRBLOCK + super.dsize;
        superdirty = YES;
    }

    dirsize = 8 * super.dsize;
    freespace = super.vsize - super.free;

    if (tapedir)
        free(tapedir);

    if (super.dsize > MAXDSIZE) {
        prs("Device too large\n");
        return NO;
    }

    tapedir = malloc(BUFSIZE * super.dsize);

    if (tapedir == NULL) {
        perror("directory buffer");
        exit(NO);
    }

    readlabel();

    return YES;
}

/*
 * balloc - allocate a space of at least a blocks, returning its starting
 * block, or 0 when the volume does not have that much free.
 */
unsigned
balloc(a)
    unsigned a;
{
    unsigned b;

    if (a > freespace)
        return 0;

    b = super.free;

    super.free += a;
    freespace -= a;
    superdirty = YES;

    return b;
}

/*
 * writesuper - write the super block if need be, then the volume label.
 */
void
writesuper()
{
    if (superdirty) {
        super.time = dumptime;
        super.reel = reel++;
        super.serial = ++serial;
        super.check = 0;
        super.check = -sum(&super, sizeof super);

        seek(fd, SUPER, BYBLOCK);
        write(fd, &super, sizeof super);

        superdirty = NO;
    }

    writelabel();
}
