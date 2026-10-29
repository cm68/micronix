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
 * The static routines below, declared before anything calls them.
 *
 * A call the compiler has not seen declared is extern int by default,
 * and the definition further down says static: two linkages for one
 * name.  The complaint lands on the definition rather than on the
 * call that caused it.
 */
static int krw();
static int iorw();

/*
 * memread() and memwrite() comprise the memory and port devices.
 * Major 2 is memory and major 3 is the ports, and one driver serves
 * both: bit 8 of the device number is set for major 3 and clear for
 * major 2, and picks the path.  The device table names this pair for
 * both majors, which is a page's worth of functions cheaper than a
 * separate entry point for each.
 *
 * The memory majors 0 and 1 are defined.
 * minor 0 is for i/o to outer space
 * direct to the 24-bit external memory bus.
 * minor 1 is for i/o to kernel space.
 *
 * Kernel space begins with the CPU board's own page.  RAM is the
 * bottom 1K and the monitor EPROM the 1K at 0x800; the rest is
 * hardware: the front panel, task and mask registers at 0x400, the
 * write-only map RAM at 0x600, and the FPU at 0xc00, above which the
 * page is unmapped.  Only the RAM and the EPROM are memory, so only
 * they are served here.  A write to the task or mask register would
 * switch tasks or stop the machine under the kernel's feet, and the
 * map registers are how the kernel places its windows.
 */

#define LOWREG	0x400			/* front panel, task and mask */
#define LOWROM	0x800			/* monitor EPROM */
#define LOWTOP	0x1000			/* rest of the page is on board */

#define K4(x)	(4096 - ((UINT)(x) & 4095))

/*
 * The size of a Morrow MPZ80 memory map address segment.
 */

#define SEGSIZE 4096

/*
 * The zero page is 512 bytes (exec.c), and a transfer may be longer,
 * so the fill in krw() goes at that size.
 */

#define ZSIZE	512

memread(dev)
    int dev;
{
    if (dev & 0x100)            /* major 3: the ports */
        iorw(dev, READ);
    else
        krw(dev, READ);
}

memwrite(dev)
    int dev;
{
    if (dev & 0x100)            /* major 3: the ports */
        iorw(dev, WRITE);
    else
        krw(dev, WRITE);
}

/*
 * A kernel-space transfer is checked before it starts.  Only the CPU
 * board's first page holds hardware; above it is mapped memory, and
 * any transfer that does not run past the top of the 16-bit space is
 * served.  In the page the memory is the alternating 1K blocks at
 * 0x000 (RAM) and 0x800 (EPROM), so the transfer must end by the top
 * of the block it starts in.  That top is (off & 0x800) + 0x400, and
 * a start in a hardware block is already past it.  A transfer that
 * wraps at 64K is caught the same way, because the iomove pointer
 * would walk back into the page from below, where this check does not
 * look again.
 *
 * A transfer that touches hardware is answered with zeroes: a read
 * is filled from the zero page and a write is discarded, and both
 * report the whole count with no error.  The registers are not there
 * to be read or changed, and a program that walks memory should pass
 * through them rather than stop at them.
 */

extern char zpage[];            /* exec.c: a 512-byte zero page */

static
krw(dev, rw)
    UINT8 rw;
{
    UINT n, off;

    if ((UINT8) dev & 1) {      /* minor 1: kernel space, low bit */
        off = (UINT) u.offset;
        n = off + u.count;      /* below off: the transfer wraps at 64K */
        if (n < off || (off < LOWTOP && n > (off & LOWROM) + LOWREG)) {
            if (rw == READ)
                do
                    iomove(READ, zpage, u.count >= ZSIZE ? ZSIZE : u.count);
                while (u.count);
            u.count = 0;
            return;
        }
        iomove(rw, (char *) off, u.count);
    }

    else {                      /* outer space */
        while (u.count) {
            n = min(u.count, K4(u.base));
            n = min(n, K4(u.offset));

            memrw(u.base, u.offset, rw, n);

            u.offset += n;
            u.base += n;
            u.count -= n;
        }
    }
}

/*
 * The character entries these two majors register (sys/ovl.c copies
 * them into ovlcvec when the module is placed).  One driver serves both
 * major 2 and major 3, so the same vector serves both headers; each
 * module page gets its own copy because memdev.o is linked into each.
 */
extern int nulldev(), nodev(), noioctl();

struct ciovec memcvec = {
    &nulldev, &nulldev, &memread, &memwrite, &noioctl
};

static
iorw(dev, rw)
    int dev;
    UINT8 rw;
{
    UINT8 port;

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
