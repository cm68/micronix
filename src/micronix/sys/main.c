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
UINT8 nbuf = 0;
struct buf *btop = 0;

UINT8 map0[], image0[];

extern int segalloc();          /* malloc.c */

#ifdef DEBUGBUFS
/*
 * Clamp the buffer pool so a cache bug shows up quickly instead of an
 * hour into a recompile.  This is the opt-in, not the configuration:
 * the kernel ships unclamped and expand_bufs() mints to the full
 * 0xd000 budget, which is whatever the resident .bss leaves it - see
 * the bound in expand_bufs() below.  "make DEBUGBUFS=1" in sys is how
 * the host build asks for the clamp.
 */
#define MAXBUFS  32
#endif

/*
 * Grow the buffer cache to the full 0x1000-0xcfff budget.  Mint headers
 * contiguously off &blist[8] (== _ebss) until they reach 0xd000, pulling
 * one 4K segment (8 blocks) from segalloc() per group of 8 headers.  The
 * pool blocks themselves live in those unmapped segments, reached through
 * the 0xf000 window: block i has data = 0xf000 + (i&7)*512, xmem = seg.
 */
expand_bufs()
{
    struct buf *b;
    int seg;
    int j;

    b = blist + 8;
    while ((int)b + sizeof(*b) <= 0xd000
#ifdef DEBUGBUFS
           && nbuf < MAXBUFS
#endif
          ) {
        seg = segalloc();
        for (j = 0; j < 8 && (int)b + sizeof(*b) <= 0xd000; j++, b++) {
            zero(b, sizeof(*b));
            b->data = (char *)(0xf000 + j * 512);
            b->xmem = seg;
            nbuf++;
        }
    }
    btop = b;
    pr("expanded to %d buffers\n", nbuf);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
