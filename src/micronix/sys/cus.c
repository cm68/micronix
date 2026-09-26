/*
 * hardware initialization calls
 *
 * sys/cus.c 
 * Changed: <2021-12-24 05:55:40 curt>
 */
#include <types.h>
#include <sys/sys.h>
#include <sys/tty.h>

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
    minit();                    /* in inits.s */

    /*
     * coninit (); /* in inits.s
     */
    ovlstart();                 /* the stamped root driver, in sys/ovl.c */
    hdinit();

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

#define HDPORT	0x52

/*
 * Turn off pending interrupt from hdca (hdca need not be there)
 */
static
hdinit()
{
    in(HDPORT);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
