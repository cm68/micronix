/*
 * kernel constant tables initialized
 *
 * sys/con.c 
 * Changed: <2021-12-24 05:53:41 curt>
 */
#include <types.h>
#include <sys/sys.h>
#include <sys/con.h>

/*
 * The driver entry points the switches below are built out of.
 *
 * Nothing declared them.  They are defined in the driver that owns
 * them, all with the implicit int of the day, and taking the address
 * of a name this file has never heard of was something the compiler
 * of the day let pass.  It is not C: &name needs a name, and the
 * error a stricter compiler gives for it - "bad op", "need lvalue",
 * pointing at the table rather than at the missing declaration - says
 * nothing about what is actually wrong.
 *
 * Written the way they are defined, which is to say returning int.
 * The switch fields are int (*)() to match.
 */
extern int nodev(), nulldev(), nullwrite();     /* cio.c */
extern int djopen(), djclose(), djstrat();      /* dj.c */
extern int djmopen(), djmclose(), djmread();
extern int djmwrite(), djstty();
extern int mwopen(), mwclose(), mwstrat();      /* mw.c */
extern int muopen(), muclose(), muread();       /* multio.c */
extern int muwrite(), mustty();
extern int kread(), kwrite(), ioread(), iowrite();  /* memdev.c */

/*
 * The block io switch is an array of
 * block io vectors. See con.h.
 * Device 0 must be nodev.
 */
struct biovec biosw[] = {
    &nodev, &nulldev, &nulldev,         /* 0 = no device */
    &nodev, &nodev, &nodev,             /* 1 = HDCA - removed, obsolete */
    &djopen, &djclose, &djstrat,        /* 2 = DJ-DMA */
    &mwopen, &mwclose, &mwstrat,        /* 3 = HD-DMA */
};

/*
 * Device names for dignostics
 */
char *devname[] = {
    "nodev", "hdca(rev4)", "djdma", "hddma",
};

/*
 * The character io switch is an array
 * of character io vectors. See con.h.
 * Device 0 is nulldev.
 */
struct ciovec ciosw[] = {
    &nulldev, &nulldev, &nulldev, &nullwrite, &nodev,
    &muopen, &muclose, &muread, &muwrite, &mustty,
    &nulldev, &nulldev, &kread, &kwrite, &nodev,
    &nulldev, &nulldev, &ioread, &iowrite, &nodev,
    &djmopen, &djmclose, &djmread, &djmwrite, &djstty,
};

/*
 * Globals
 */
UINT rootdev = 0x0208;          /* djdma alternate sectoring drive 0 */
UINT swapdev = 0x0000;          /* m16 drive A */

UINT swapsize = 0;           /* no. of swap blocks if rootdev != swapdev */
UINT swapaddr = 18448;          /* block number of first swap block , if " */

UINT nbdev = sizeof(biosw) / sizeof(struct biovec);
UINT ncdev = sizeof(ciosw) / sizeof(struct ciovec);

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
