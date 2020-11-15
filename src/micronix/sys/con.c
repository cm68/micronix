/*
 * con.c 
 */
#include <types.h>
#include <sys/sys.h>
#include <sys/con.h>

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
