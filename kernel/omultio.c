#include "sys.h"
#include "tty.h"
#include "proc.h"


#define NMIO    4      /* number of mult I/O ports */
#define MBASE 0x48
#define VREADY (1 << 3)
#define HREADY (1 << 4)
#define PREADY (1 << 5)
#define CHECK  (1 << 6)
#define POWER  (1 << 7)

#define READY (POWER | PREADY | HREADY | VREADY)
#define STATUS (POWER | CHECK | PREADY | HREADY | VREADY)

#define REVERSE        (1 << 2)
#define HALF           (1 << 3)
#define VSTROBE        (1 << 4)
#define HSTROBE        (1 << 5)
#define PSTROBE        (1 << 6)
#define RIBLIFT        (1 << 7)

#define MOTION         (1 << 7)
#define VERTICAL       (1 << 6)
#define DIR            (1 << 5)

#define CENTRONICS     (1 << 4)


#define THRESHOLD      900
#define MAXRIGHT       1572


/*
 * asyncronous communications elements
 */

/*
 * relative offsets of control registers
 */

#define INTENABLE 1		/* interrupt enable register */
#define MCONTROL 4		/* modem control register */
#define MSTATUS  6		/* modem status register */
#define MUSELECT 7		/* multio group select register */


/*
 * bits in the interrupt enable register.
 */

#define INT_RECEIVE	(1 << 0) /* recieved data avaiable */
#define INT_XMIT	(1 << 1) /* transmitter buffer empty */ 
#define INT_MODEM	(1 << 3) /* enable modem status change interrupt */

/*
 * bits in the modem control register
 */

#define MU_DTR 1		/* data terminal ready */


/*
 * bits in the modem status register
 */

#define MU_DCTS		(1 << 0)	/* delta clear to send */
#define MU_DCD		(1 << 3)	/* delta carrier detect */
#define MU_CTS 		(1 << 4)	/* clear to send */
#define MU_CD 		(1 << 7)	/* carrier detect */

/*
 * bits in the multio group select register
 */

#define PENABLE 	050		/* parallel port enable */



extern char    nmio;		/* The number of mult I/O boards present */

static unsigned char
        base = MBASE,
        hiport = MBASE,
        loport = MBASE;

static unsigned char mother = YES;

       /*
        * Hardware fiddlers in mio.s
        */

extern  int mstart(), mstop(), mputc(), mset(), minit();

       /*
        * Tty structures for multiboard ports
        */

struct tty mttys[NMIO] = {0};

       /*
        * Select a tty structure from a device number
        */
ttdev(dev)
        {
        return (&mttys[dev & 15]);
        }

       /*
        * Open a MULT I/O tty.
        * Initialize tty structure and hardware.
        */
muopen(dev, mode)
        register dev;
        {
        struct tty *tty;
        char board;


        tty = ttdev(dev);

        board = (dev & 15) >> 2;

        if ((dev & 15) >= NMIO || board >= nmio + 1)
                {
                u.error = ENXIO;        /* No such dev. */
                return;
                }

        if (tty->ispeed == 0)   /* virgin */
		{
                ttinit(tty, dev);
                mset(tty);      /* set baud rate */
		}

/*
 * a channel may be opened for dial-out only if it is presently unused.
 */

	if (dev & DIALER)
		{
					/* what about a draining out que ?*/
		if (tty->state & OPEN)		/* exclusion */
			{
			u.error = EBUSY;
			return;
			}
		}

/*
 * assert DATA TERMINAL READY
 */

	tty->mstate |= DTR;
	mumcontrol (dev);

/*
 * wait for CARRIER DETECT (if appropriate)
 */

	mumstatus (dev);	/* internalize modem status lines */

	if (dev & DIALER)
		{
		tty->mstate |= DIALER;
		}

	else
		{
		while (!(tty->mstate & CD))
			{
			di ();
			tty->mstate |= WOPEN;
			muenable (dev, INT_MODEM);
			sleep (&tty->mstate, PRITTI);
			}

		tty->mstate &= ~WOPEN;	/* no longer waiting for open */
		}

	ttyopen (tty);
	
/*
 * open for business
 */

        di();

	if (!(tty->state & OPEN) && !tty->outque.count && !tty->nextc)
                {
                mset(tty);      /* set baud rate */
                mstop(tty, 0);  /* enable input, disable output */
                }
        ei();


	}

        /*
         * Close
         */
muclose(dev)
        {
	register struct tty *t;

	t = ttdev (dev);

	if (dev & DIALER)
		{
		t->mstate &= ~DIALER;

		if ((t->mstate & (WOPEN | CD)) == (WOPEN | CD))
			{
			wakeup (&t->mstate);
			}
		}

        ttyclose(t);
        }

       /*
        * Read a multiboard tty
        */
muread(dev)
        {
        ttyread(ttdev(dev));
        }

       /*
        * Write a multiboard tty
        */
muwrite(dev)
        {
        ttywrite(ttdev(dev));
        }

       /*
        * Get/set a multiboard tty
        */
mustty(dev, flag)
        {
        ttymode(ttdev(dev), flag);
        }

       /*
        * Initialize a tty structure
	* This could be done by copying in a prototype structure.
        */
        static
ttinit(tty, dev)
        fast struct tty *tty;
        {
        tty->ispeed = tty->ospeed = 13;		/* 9600 baud */
        tty->erase  = '\b';
        tty->kill   = 'X' - 'A' + 1;

        tty->mode = ECHO | MAPCR | XTABS;

        tty->col    = 0;
        tty->dev    = dev;
        tty->start  = &mstart;
        tty->stop   = &mstop;
        tty->put    = &mputc;
        tty->set    = &mset;
        }

muselect (t)
	struct tty *t;
	{
	out (MBASE + MUSELECT, (t->dev & 3) | PENABLE);	/* group select */
	}

/*
 * mumint - multio modem status change interrupt service routine
 */


mumint (t)
	struct tty *t;
	{
	unsigned char s;


/*
 * read the modem status register
 */


	di ();

	muselect (t);
	s = in (MBASE + MSTATUS);

	ei ();


	if (s & MU_DCTS)		/* Clear to send changed. */
		{
		if (s & MU_CTS)		/* Clear to send. */
			{
			cstart (t);
			}
		}


	if (s & MU_DCD)		/* carrier indication changed */
		{
		if (s & MU_CD)
			{
			ttyconnect (t);	/* carrier appeared */
			}
		else
			{
			ttyhangup (t);	/* carrier lost */
			}
		}
	}

/*
 * set or clear DATA TERMINAL READY on the multio
 * Assert modem control lines as per t->mstate
 * (master only)
 */


/*
 * Update the modem control register in the ACE
 * from information found in the tty structure.
 * Currently, the only line we play with here is DTR.
 */

mumcontrol (dev)
	{
	unsigned char a;
	struct tty *t;

	t = ttdev (dev);

	di ();

	muselect (t);
	a = in (MBASE + MCONTROL);

	if (t->mstate & DTR)
		{
		a |= MU_DTR;
		}
	else
		{
		a &= ~MU_DTR;
		}

	out (MBASE + MCONTROL, a);

	ei ();
	}

/*
 * Sense the modem status lines
 * Make internal notations as to their state.
 * (master only)
 * This would need to be modified to work
 * on other than the first mult I/O board.
 */


mumstatus (dev)
	{
	unsigned char s;
	struct tty *t;

	t = ttdev (dev);

	if (dev)	/* ACE */
		{
		di ();

		muselect (t);

		s = in (MBASE + MSTATUS);		/* read status */

		ei ();
		}

	else		/* centronics parallel port */
		{
		s = MU_CD;	/* simulate carrier */
		}

/* 
 * make an internal note of the hardware status
 */

	if (s & MU_CD)
		{
		t->mstate |= CD;
		}
	else
		{
		t->mstate  &= ~CD;
		}
	}

/*
 * Centronics Parallel printer interrupt
 */

ppint (tty)
        register struct tty *tty;
        {
/*
 * Find the I/O port base address for this interrupt.
 * Bits 2 and 3 of the minor device number give the board desigation.
 * The drivers supports at most 4 boards.
 * The port addresses progress 16 per board.
 * The following magic formula computes an I/O base address from a 
 * device number.
 */

	base = MBASE + ((tty->dev & 014) << 2);

/*
 * see if the printer is ready
 */

        out (base + MUSELECT, PENABLE); /* select group zero */

        if ((in (base) & PREADY) == 0)
		{
                return;                 /* not ready */
		}

/*
 * wake up the high level if it needs it
 */

        if ((tty->state & HOSLEEP) && tty->outque.count <= LOWATER)
                {
                tty->state &= ~HOSLEEP;
                wakeup(&tty->outque);
                }

/*
 * turn off the interrupts if the char. que is drained
 */

        if (tty->outque.count == 0)
                {
                mstop (tty);		/* No more characters to send. */
                return;
                }

/* Determine which version of the multio we are dealing with. */
/* mother is set for wb I/O mother board style */
/* table top mom or mult I/O ? */

        mother = (in (base + 1) != 0xff); 

        hiport = loport = base;

        if (mother)
                loport++;
        else
                hiport++;

#define HIGH (~0)

        out (loport, cookout (tty));	/* set the character */

        out (hiport, HIGH);		/* strobe it out */
        out (hiport, HIGH ^ PSTROBE);
        out (hiport, HIGH);
        }

#define MBASE (0x48)
#define PIC1 (MBASE + 5)

/*
 * Enable interrupts on PIC line a
 */

inton (a)
        {
        di ();
        out (MBASE + MUSELECT, PENABLE);
        out (PIC1, in (PIC1) & ~(1 << a));
        ei ();
        }

/*
 * Disable interrupts on PIC line a
 */

intoff (a)
        {
        di ();
        out (MBASE + MUSELECT, PENABLE);
        out (PIC1, in (PIC1) | (1 << a));
        ei ();
        }

/*
 * muenable -
 * set the multio interrupt lines according to mode
 * Lines we play with are transmit, receive, and modem status change.
 * Write a value to the interrupt enable register.
 */

muenable (dev, mode)
	{
	di ();
	out (MBASE + MUSELECT, (dev & 3) | PENABLE);	/* group select */
	out (MBASE + INTENABLE, mode);
	ei ();
	}
