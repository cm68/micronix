/*
 * main.c 
 */
#include "sys.h"
#include "inode.h"
#include "mount.h"
#include "proc.h"
#include "file.h"
#include "buf.h"
#include "con.h"

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
UCHAR nbuf = 0;
struct buf *btop = 0;
char (*buffer)[512] = 0;
struct buf blist[4] = 0;        /* expanded by binit */

extern UCHAR segmap[];          /* malloc.c */
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
    char ronly = NO;

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
        ronly = YES;
        pr("\nRoot device is read-only\n\n");
    }
    tmount(rootdev, NULL, ronly);
    if ((rootdir = iget(1, rootdev)) == NULL)
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
    UCHAR i;

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
    pr("\nRedux\n");
    pr("Micronix 1.61\n");
    pr("Created 11/9/83\n");
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

UCHAR map0[], image0[];

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
