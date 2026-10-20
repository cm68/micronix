/*
 * hardware initialization calls
 *
 * sys/cus.c 
 * Changed: <2021-12-24 05:55:40 curt>
 */
#include <types.h>
#include <sys/sys.h>
#include <sys/tty.h>
#include <sys/ints.h>
#include <sys/pic8259.h>

/*
 * Initialize custom hardware. Called from main().
 * Compile with -x0 and load after main, so that
 * this code sits in the buffer space.
 */

/*
 * The initialisers below, declared before cus() calls them.
 *
 * Without this the call is the first the compiler hears of the name,
 * which makes it extern int by default, and the definition further
 * down says static - two different linkages for one name.  The error
 * lands on the definition, a page away from the call that caused it.
 */
static int cinit(), hdinit();
extern int pic_init();          /* sys/pic_init.c: probe the PICs at boot */
extern int intr_attach();       /* sys/probe_init.c */

cus()
{

    /*
     * Initialize
     *      clist
     *      multio 
     *      console
     *      djdma
     *      hdca
     *      votrax
     */

    di();
    cinit();
    pic_init();                 /* probe the PICs, picks the clock + console */
    (*clockinit)();             /* the heartbeat, chosen by pic_init */
    (*console_minit)();         /* the uart bring-up, chosen by pic_init */

    /*
     * coninit (); /* in inits.s
     */
    ovlstart();                 /* the stamped root driver, in sys/ovl.c */
    hdinit();                   /* clear the boot's stuck op-done/seek-done */
    /*
     * Wire that driver's interrupt now, before main() goes on to open the
     * root device.  The rest of the modules cannot be placed until the
     * root device can be read - the kernel reads them out of its own file
     * through it - so main() calls intr_attach() again after ovlplaceall()
     * for those.  It is the first open, not the last placement, that the
     * wiring has to beat.
     */
    intr_attach();

    /*
     * vtinit ();
     */
    ei();
}

static
cinit()
{
    extern char clist[];
    extern struct cblock *cfree;

    static char *p, *top;
    static struct cblock *b;

    p = clist;
    p = (int) p & ~15;

    if (p < clist)
        p += 16;

    top = &clist[CSIZE] - 16;

    for (; p <= top; p += 16) {
        /*
         * clist is a char array being carved into cblocks, so the
         * walking pointer is a char * and this is the point where a
         * sixteen byte lump of it becomes a struct.  ccc will not
         * convert between unrelated pointer types on its own; the
         * cast says what the loop above has already arranged, which
         * is that p is aligned on a cblock boundary.
         */
        b = (struct cblock *) p;

        b->next = cfree;        /* free it */
        cfree = b;
    }

}

/*
 * 
 * 
 * # define VTBASE 0xc4 # define NVTPORT 4 # define NVT 4 # define QUIET (1
 * << 4) # define estate col # define istate nextc
 * 
 * static vtinit () { extern char nvt; extern vtstart (), vtstop (), vtputc
 * (), nulldev (); static unsigned char p, i; struct tty *t; extern struct
 * tty vt[];
 * 
 * /* * Turn off vt boards (they need not be preset) *
 * 
 * p = VTBASE; t = vt;
 * 
 * for (i = 0; i < NVT; i++, p += NVTPORT, t++) { if (in (p) == 0xff) break;
 * 
 * nvt++;
 * 
 * out (p, QUIET); out (p + 1, 0);
 * 
 * t->mode = RAW; t->start = vtstart; t->stop = vtstop; t->put = vtputc;
 * t->set = nulldev; t->estate = QUIET; t->istate = 0; t->dev = i; } }
 * 
 */

#define HDCMD	0x51
#define HDPORT	0x52

/*
 * Turn off the pending interrupts from the hdca the boot left behind.  The
 * level 1/2/3 boot read this controller to get the kernel here and never
 * acknowledged the op-done, so the line is stuck high; the attach loop's
 * probe needs a fresh edge and finds nothing while the line is already up.
 * A RAM pointer command acknowledges the op-done, and reading the
 * interrupt-clear port acknowledges the seek-done.  (The hdca need not be
 * there - a missing card takes the writes harmlessly.)
 */
static
hdinit()
{
    out(HDCMD, 0);              /* CMD_RAMDATA: acknowledge op-done */
    in(HDPORT);                 /* acknowledge seek-done */
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
