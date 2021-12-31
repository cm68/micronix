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
    djinit();
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
        b = p;

        b->next = cfree;        /* free it */
        cfree = b;
    }

}

#define SERIAL		0x2c
#define DISABLE	0
#define DJPORT		0xef
#define DJINT		1
#define LOGICAL	0x2e
#define EIGHTFIRST	0
#define NOSTAT		0
#define SETCHANNEL	0x27
#define HALT		0x25
#define SETTRACK	0x2d
#define MAXTRACK	80
#define NDJDRIVE	8

static
djinit()
{
    extern unsigned char djcomm[];
    extern char map0[], image0[];
    static char *p, i;

    p = 0x1050;

    map0[2] = 0;                /* contort the map */

    *p++ = SERIAL;              /* disable the serial port */
    *p++ = DISABLE;

    *p++ = LOGICAL;             /* set to 8" drives first */
    *p++ = EIGHTFIRST;
    *p++ = NOSTAT;

    *p++ = SETCHANNEL;          /* set the command address */

    *p++ = ((unsigned) djcomm >> 0);
    *p++ = ((unsigned) djcomm >> 8);
    *p++ = ((unsigned) djcomm >> 16);

    for (i = 0; i < NDJDRIVE; i++) {
        *p++ = SETTRACK;
        *p++ = i;
        *p++ = MAXTRACK;
        *p++ = NOSTAT;
    }

    *p++ = HALT;
    *p++ = NOSTAT;

    map0[2] = image0[2];        /* restore the map */

    out(DJPORT, 0);

    inton(DJINT);

    djgoose();                  /* start up dj watchdog */
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
