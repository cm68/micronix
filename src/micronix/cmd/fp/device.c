/*
 * fp - dump and restore files onto floppy volumes
 *
 * cmd/fp/device.c
 */

#include "fp.h"

/*
 * devopen - open the volume and read its superblock.  The device is
 * opened read-write for a replace (r) and read-only otherwise.
 */
int
devopen()
{
    int mode;

    if (rflag)
        mode = UPDATE;
    else
        mode = READ;

    fd = open(device, mode);

    if (fd < 0) {
        perror(device);
        return NO;
    }

    return readsuper();
}

/*
 * devsize - measure a device, returning its size in blocks.  It binary
 * searches for the highest block that will read; a read at a block past
 * the end comes back ENXIO.
 */
int
devsize(a)
    int a;
{
    char c;
    int n;
    long low, high, mid;

    low = 0;
    high = MAXDEVSIZE;

    for (;;) {
        mid = (low + high) / 2;

        if (mid == low)
            break;

        seek(a, (int) mid, 3);

        n = read(a, &c, 1);

        if (n == 1 || errno != ENXIO)
            low = mid;
        else
            high = mid;
    }

    if (high >= MAXDEVSIZE)
        high = MAXDEVSIZE - 1;     /* max. size */

    return (int) high;
}

void
closedev()
{
    close(fd);
    fd = (-1);
}
