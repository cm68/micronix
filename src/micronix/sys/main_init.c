/*
 * the kernel startup
 *
 * This is init-only code: it runs once from main() in task 0 and is
 * then dead.  It is linked high - into the region between _ebss and
 * the 0xd000 scratch window - and main_data.c's expand_bufs() mints
 * buffer headers over it once the init-only functions have returned.
 * The resident data it touches (plist, ilist, flist, mlist, nbuf,
 * btop, rootdir, map0, image0) lives in main_data.c.
 *
 * sys/main_init.c
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

#include "build.h"

/*
 * resident data, defined in main_data.c (and malloc.c / con.c)
 */
extern struct proc *swapproc;
extern struct inode *rootdir;
extern UINT8 nbuf;
extern struct buf *btop;
extern UINT8 map0[], image0[];
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
    u.p->tty = 0;               /* not tied to any tty */
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
     * in the unmapped physical segments, reached through the 0xf000 window:
     * block i has data = 0xf000 + (i&7)*512 and xmem = its segment.  blist
     * holds exactly the 8 boot headers (the last .bss object, see
     * textpad.s); expand_bufs() mints the rest contiguously off &blist[8]
     * once the init-only functions have returned.
     */
    seg = segalloc();
    nbuf = 8;
    btop = blist + nbuf;
    for (b = blist; b < btop; b++) {
        zero(b, sizeof(*b));
        b->data = (char *)(0xf000 + ((b - blist) & 7) * 512);
        b->xmem = seg;
    }
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
