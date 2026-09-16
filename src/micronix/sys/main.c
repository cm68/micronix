/*
 * the kernel startup
 * this should be loaded high, so it can be overlaid by kernel data
 *
 * sys/main.c
 * Changed: <2021-12-24 06:10:54 curt>
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

#include "build.h"

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

extern UINT8 segmap[];          /* malloc.c */
extern UINT nsegs;
extern int segalloc();          /* malloc.c */
extern int nodev();             /* con.c */

/*
 * System initialization
 */
main()
{
    char ronly = 0;

    cus();                      /* custom hardware */
    enable();                   /* enable interrupts */
    coninit();
    plogo();
    pinit();                    /* plist, see below */
    meminit();                  /* memory */
    binit();                    /* buffers (segalloc's the boot page) */
    pcon();                     /* below */
    bopen(rootdev, WRITE);
    if (u.error) {
        u.error = 0;
        bopen(rootdev, READ);
        if (u.error)
            panic("Can't read root device");
        ronly = 1;
        pr("\nRoot device is read-only\n\n");
    }
    tmount(rootdev, 0, ronly);
    if ((rootdir = iget(1, rootdev)) == 0)
        panic("Can't get root dir");
    u.cdir = rootdir;
    rootdir->count = 2;
    irelse(rootdir);
    /*
     * Create the init process.
     * Note that the fork() does not
     * go thru the system call mecanism.
     */
    u.p->tty = 0;               /* not tied to any tty */

    if (fork()) {
        expand_bufs();          /* grow the cache, then start init */
        swap();                 /* no return */
    }
}

/*
 * Initialize the first proc structure
 */
pinit()
{
    UINT8 i;

    u.p = swapproc;
    u.p->mode = ALLOC | ALIVE | AWAKE | LOADED | LOCKED | SYS;
    u.p->pri = PRISWAP;
    u.p->nice = 128;
    u.p->pid = 0;
    u.p->slist[SIGTINT] = 1;    /* ignore record-available signal */
    u.p->slist[SIGBACK] = 1;    /* ignore backgrounding signal */
    copy("swap", u.p->args, 5);

    for (i = 0; i < 17; i++) {  /* memory map */
        u.p->mem[i].seg = i;
        u.p->mem[i].per = FULL;
    }

    u.p->mem[16].seg = USERSEG; /* other procs will use seperate seg */
    u.p->nsegs = 17;            /* actually 16; used by mget() */
}

/*
 * Print the logo
 */
plogo()
{
    pr("\nMicronix 1.61\n");
#ifdef BUILD_DATE
    pr(BUILD_DATE);
#else
    pr("Created 11/9/83\n");
#endif
    pr("Copyright 1983 Gary Fitts\n\n");
}

/*
 * Print configuration info
 */
pcon()
{
    char i;
    UINT kmem;

    kmem = (nsegs << 2) + 64;
    pr("%dK memory\n", kmem);
    pr("%d cache blocks\n", nbuf);
    pr("%d processes\n", NPROC);
    pr("disks: ");
    for (i = 1; i < nbdev; i++)
        if (biosw[i].strat != &nodev)
            pr("%s ", devname[i]);
    pr("\nroot dev: %s/%d\n", devname[major(rootdev)], minor(rootdev));
    pr("swap dev: %s/%d\n", devname[major(swapdev)], minor(swapdev));

    if (kmem < 128 || (kmem < 256 && swapdev == 0))
        panic("Not enough memory to run");
}

UINT8 map0[], image0[];

#define ADDR	(*(char*)0x1000)

/*
 * Find out how much memory is present.
 * Set nsegs to the number of 4K segments, and set
 * segmap[n] to 1 if the nth segment is missing.
 */
meminit()
{
    int n, any;

    di();
    nsegs = MAXSEG - 16;
    for (n = 16; n < MAXSEG; n++) {
        map0[2] = n;
        ADDR = 0;
        if (ADDR != 0 || --ADDR != -1) {
            segmap[n] = 1;
            nsegs--;
        }
    }
    map0[2] = image0[2];
    ei();

    /*
     * Report the absent pages here, before segalloc() reuses segmap[n]
     * to mean "allocated", so the list is genuinely the bad memory.
     */
    any = 0;
    for (n = 16; n < MAXSEG; n++)
        if (segmap[n])
            any = 1;
    if (any) {
        pr("\nBad memory in the following 4K segments (in hex):\n");
        for (n = 16; n < MAXSEG; n++)
            if (segmap[n])
                pr("%x ", n);
        pr("(These segments will not be used by the system)\n\n");
    }
}

/*
 * Initialize the blist. Called once from main().
 */
binit()
{
    struct buf *b;
    int seg;

    /*
     * Seed the 8 boot buffers from one 4K segment.  Buffer data now lives
     * in the unmapped physical segments, reached through the 0xe000 window:
     * block i has data = 0xe000 + (i&7)*512 and xmem = its segment.  blist
     * holds exactly the 8 boot headers (the last .bss object, see
     * textpad.s); expand_bufs() mints the rest contiguously off &blist[8]
     * once the init-only functions have returned.
     */
    seg = segalloc();
    nbuf = 8;
    btop = blist + nbuf;
    for (b = blist; b < btop; b++) {
        zero(b, sizeof(*b));
        b->data = (char *)(0xe000 + ((b - blist) & 7) * 512);
        b->xmem = seg;
    }
}

/*
 * Grow the buffer cache to the full 0x1000-0xcfff budget.  Mint headers
 * contiguously off &blist[8] (== _ebss) until they reach 0xd000, pulling
 * one 4K segment (8 blocks) from segalloc() per group of 8 headers.  The
 * pool blocks themselves live in those unmapped segments, reached through
 * the 0xe000 window: block i has data = 0xe000 + (i&7)*512, xmem = seg.
 */
expand_bufs()
{
    struct buf *b;
    int seg;
    int j;

    b = blist + 8;
    while ((int)b + sizeof(*b) <= 0xd000) {
        seg = segalloc();
        for (j = 0; j < 8 && (int)b + sizeof(*b) <= 0xd000; j++, b++) {
            zero(b, sizeof(*b));
            b->data = (char *)(0xe000 + j * 512);
            b->xmem = seg;
            nbuf++;
        }
    }
    btop = b;
    pr("expanded to %d buffers\n", nbuf);
}

/*
 * Map a buffer's segment into the 0xe000 window so its data (0xe000 +
 * offset) becomes reachable.  b->xmem carries the segment; segment 14 is
 * the window, so its map register is map0[2*14] == map0[28].
 */
bwin(seg)
    int seg;
{
    map0[28] = image0[28] = seg;
}

/*
 * Map a buffer into the 0xd000 window (the copyin/copyout page, free while
 * we are not touching process address space) and return its address there.
 * Used for the superblock so it stays reachable alongside another buffer in
 * 0xe000 (e.g. ifill's inode-block scan).
 */
bsup(sb)
    struct buf *sb;
{
    map0[26] = image0[26] = sb->xmem;
    return (0xd000 + ((int)sb->data & 0xfff));
}

/*
 * Copy between two buffers in (possibly) different segments.  Maps the
 * source into 0xe000 and the destination into 0xd000 - the copyin/copyout
 * page, which is free here because we are not touching process address
 * space during superblock/freelist handling - then ldir's between the two
 * windows.  soff/doff are byte offsets within the buffers.
 */
bcopy(sb, soff, db, doff, count)
    struct buf *sb, *db;
    int soff, doff, count;
{
    di();
    map0[28] = image0[28] = sb->xmem;
    map0[26] = image0[26] = db->xmem;
    copy(sb->data + soff,
        (char *)(0xd000 + ((int)db->data & 0xfff) + doff), count);
    ei();
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
