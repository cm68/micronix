/*
 * Memory device driver
 *
 * sys/memdev.c 
 * Changed: <2022-01-04 10:34:45 curt>
 */
#include <types.h>
#include <sys/sys.h>
#include <sys/proc.h>
#include <sys/con.h>

/*
 * kread() and kwrite() comprise the memory device.
 * minor device numbers 0 and 1 are defined.
 * minor 0 is for i/o to outer space
 * direct to the 24-bit external memory bus.
 * minor 1 is for i/o to kernel space.
 */

#define K4(x)	(4096 - ((UINT)(x) & 4095))

/*
 * The size of a Morrow MPZ80 memory map address segment.
 */

#define SEGSIZE 4096

kread(dev)
{
    krw(dev, READ);
}

kwrite(dev)
{
    krw(dev, WRITE);
}

/*
 * Note that iomove can take 4K as a maximum byte count.
 */

static
krw(dev, rw)
    int rw;
{
    static UINT n;

    if (dev & 1) {              /* kernel space */
        while (u.count) {
            iomove(rw, (char *) (u.offset), min(u.count, SEGSIZE));
        }
    }

    else {                      /* outer space */
        while (u.count) {
            n = u.count;
            n = min(n, K4(u.base));
            n = min(n, K4(u.offset));

            memrw(u.base, u.offset, rw, n);

            u.offset += n;
            u.base += n;
            u.count -= n;
        }
    }
}

ioread(dev)
    int dev;
{
    iorw(dev, READ);
}

iowrite(dev)
    int dev;
{
    iorw(dev, WRITE);
}

iorw(dev, rw)
    int dev, rw;
{
    static UINT8 port;

    port = minor(dev);
    if (port == 0)
        port = u.offset;
    while (u.count) {
        if (rw == READ)
            putbyte(in(port), u.base);
        else
            out(port, getbyte(u.base));
        u.base++;
        u.count--;
    }
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
