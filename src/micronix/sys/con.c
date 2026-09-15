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
 * biosw/ciosw, nbdev/ncdev and devname live in leaf/consts.c, parked
 * in the u page; they are read-only so a per-process copy is harmless.
 * con.h still declares them (the arrays and the extern globals).
 */

/*
 * Globals
 */
UINT rootdev = 0x0300;          /* hddma drive 0 (Seagate 5 meg) */
UINT swapdev = 0x0000;          /* m16 drive A */

UINT swapsize = 0;           /* no. of swap blocks if rootdev != swapdev */
UINT swapaddr = 18448;          /* block number of first swap block , if " */

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
