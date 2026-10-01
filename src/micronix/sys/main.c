/*
 * the kernel's resident data and the buffer-pool grower
 *
 * This is the data half of the old main.c.  The init-only code that
 * used to share this file moved to main_init.c, which is linked high
 * (into the region expand_bufs() below reclaims as buffer headers).
 * What stays here is resident: the process/inode/file/mount tables,
 * the kernel configuration globals (formerly con.c) and the two MMU
 * map register arrays, plus expand_bufs() itself - it cannot be
 * reclaimed, because it is the routine that mints the buffer headers
 * over the init-only code, and would overwrite itself if it lived there.
 *
 * sys/main.c
 * Changed: <2026-09-20 curt>
 */
#include <types.h>
#include <sys/fs.h>
#include <sys/sys.h>
#include <sys/stat.h>
#include <sys/inode.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/buf.h>
#include <sys/con.h>
#include <sys/signal.h>

/*
 * Kernel configuration, formerly con.c.  These are resident globals:
 * they must not move into the reclaimed highmem region.
 */
UINT rootdev = 0x0300;          /* hddma drive 0 (Seagate 5 meg) */
UINT swapdev = 0x0000;          /* m16 drive A */
UINT swapsize = 0;              /* no. of swap blocks if rootdev != swapdev */
UINT swapaddr = 18448;          /* block number of first swap block, if " */

/*
 * static initialization.
 */
struct proc plist[NPROC] = 0;
struct proc *swapproc = &plist[0];
struct proc *initproc = &plist[1];
struct inode ilist[NINODE] = 0;
struct inode *rootdir = 0;
struct file flist[NFILE] = 0;
struct mount mlist[NMOUNT] = 0;

/*
 * Buffer stuff initialized in binit
 */
UINT nbuf = 0;                  /* buffered by nothing but MAXBUFS */
UINT totmem = 0;                /* total memory in K, saved by pcon() */
struct buf *btop = 0;

UINT8 map0[], image0[];

extern int segalloc();          /* malloc.c */
extern int nodev();             /* con.c */

/*
 * The ceiling on the pool.  expand_bufs() would otherwise mint whatever
 * the resident .bss leaves between _ebss and BUFWIN.  That was past what
 * the count can carry when this was written - 282 buffers, and nbuf
 * wrapped at 256 - and it is 244 now, under the cap, the kernel having
 * grown into the headroom; the cap stays as the ceiling-independent
 * bound.  256 is also 32 of the 4K segments the blocks come from, which
 * is what the budget is really made of, and no filesystem here wants
 * more cache than that.
 *
 * DEBUGBUFS is the opt-in for a much smaller clamp, so a cache bug shows
 * up quickly instead of an hour into a recompile.  "make DEBUGBUFS=1" in
 * sys is how the host build asks for it.
 */
#ifdef DEBUGBUFS
#define MAXBUFS  32
#else
#define MAXBUFS  256
#endif

/*
 * The signon report: the machine summary, printed once the buffer cache
 * is at its final size, so the cache-block count is the one that matters.
 */
signon()
{
    char i;

    pr("%d processes\n", NPROC);
    pr("disks: ");
    for (i = 1; i < nbdev; i++)
        if (biosw[i].strat != &nodev)
            pr("%s ", devname[i]);
    pr("\nroot dev: %s/%d\n", devname[major(rootdev)], minor(rootdev));
    pr("swap dev: %s/%d\n", devname[major(swapdev)], minor(swapdev));
    pr("%dK memory\n", totmem);
    pr("%d cache blocks\n", nbuf);
}

/*
 * Grow the buffer cache to the full budget: the .bss end up to BUFWIN,
 * the base of the segment the buffers are reached through.  Mint headers
 * contiguously off &blist[8] (== _ebss) until they reach BUFWIN, pulling
 * one 4K segment (8 blocks) from segalloc() per group of 8 headers.  The
 * pool blocks themselves live in those unmapped segments, reached
 * through the BUFSEG window: block i has data = BUFWIN + (i&7)*512,
 * xmem = seg.
 *
 * The ceiling is the window's own page, not _upage above it: bwin()
 * leaves a segment mapped for the whole of a buffer access, so a header
 * parked in that page would be read back as buffer data.  Raising the
 * pool past it means borrowing the window back at every site that uses
 * it - see TODO.
 *
 * 0xd000's page used to be reserved for the superblock's window; the
 * superblock is now plain memory (getsb) and mem.s restores the page it
 * borrows for user memory, so the headers may carry on through it.
 */
expand_bufs()
{
    struct buf *b;
    int seg;
    int j;

    b = blist + 8;
    while ((int)b + sizeof(*b) <= (int)BUFWIN && nbuf < MAXBUFS) {
        seg = segalloc();
        /*
         * The inner loop carries the MAXBUFS test too.  The group is
         * eight headers, so stopping only at the top of the while would
         * overshoot the ceiling by up to seven - and one past it is a
         * counter that has wrapped.  The segment is already in hand at
         * that point, so the last group may leave some of its eight
         * blocks unused; the pool does not have to be a whole segment.
         */
        for (j = 0; j < 8 && nbuf < MAXBUFS &&
                     (int)b + sizeof(*b) <= (int)BUFWIN; j++, b++) {
            zero(b, sizeof(*b));
            b->data = (char *)BUFWIN + j * 512;
            b->xmem = seg;
            nbuf++;
        }
    }
    btop = b;
    signon();
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
