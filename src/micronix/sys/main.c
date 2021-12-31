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
char (*buffer)[512] = 0;
struct buf blist[4] = 0;        /* expanded by binit */

extern UINT8 segmap[];          /* malloc.c */
extern UINT nsegs;
extern int nodev();             /* con.c */

/*
 * This module must be compiled with the -x0 option and
 * loaded last of all. The buffer initialization routine
 * binit will use all space above the blist for buffer
 * area, including the main() code below. Since this code
 * may need up to 4 buffers (2 for bopens, 1 for tmount,
 * and 1 for iget), the following padding is necessary
 * to allow initial use of the blist and still protect main().
 */
static char pad[5 * 512] = 0;

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
    binit();                    /* buffers */
    meminit();                  /* memory */
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

    if (fork())
        swap();                 /* no return */
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

    if (kmem % 64 != 0) {
        pr("\nBad memory in the following 4K segments (in hex):\n");
        for (i = 16; i < nsegs; i++)
            if (segmap[i])
                pr("%h ", i);
        pr("(These segments will not be used by the system)\n\n");
    }
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
    int n;

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
}

/*
 * Initialize the blist. Called once from main().
 */
binit()
{
    struct buf *b;
    UINT space;
    extern char usrtop;

    space = (UINT) (&usrtop) - (UINT) (blist);
    nbuf = space / (512 + sizeof(struct buf));
    buffer = &blist[nbuf];
    btop = blist + nbuf;
    for (b = blist; b < btop; b++) {
        zero(b, sizeof(*b));
        b->data = buffer[b - blist];
        b->xmem = KERNEL;
    }
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
