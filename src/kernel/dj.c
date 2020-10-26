/*
 * dj.c 
 */

/*
 * Micronix driver for Disk Jockey DMA
 * 
 *      Len Edmondson
 *      Morrow Designs
 *          1982
 *
 * 
 * The minor device number is interpreted as follows:
 *
 *      bit(s)          Meaning
 *
 *      0,1             drive number    (0 - 3)
 *      2               drive size      (0 = 8", 1 = 5 1/4")
 *      3               alt. sectors    (if set)
 *      4,5             drive type
 *      6               double sided soft 5
 *      7               internal use (should be zero)
 *
 *                      5 1/4" types
 *                      -------------
 *                      0 - Tandon
 *                      1 - SA200
 *                      2 - SA400
 *                      3 - Tandon (reserved)
 *
 *
 *                      8" types
 *                      ----------
 *                      0 - SA800
 *                      1 - SA850
 *                      2 - SA800 (reserved)
 *                      3 - SA800 (reserved)
 *
 */

#include "sys.h"
#include "buf.h"
#include "con.h"
#include "proc.h"

#define	DJINTERVAL	(10 * HERTZ)
#define	DJTHRESHHOLD	2

#define	NONE		255
#define	DEFSPECS	8
#define	FIVEBASE	16
#define	MAXTRACK	80

/*
 * per - drive flags bits 
 */

#define 	F_WP		(1 << 2)
#define	ALT		(1 << 3)        /* do alternate sectoring */

#define	DOUBSIDE	(1 << 6)
#define	SINGSIDE	0

#define	GETSTAT		(1 << 7)
#define	DISCARD		0xff
#define	TYPE		((1 << 4) | (1 << 5))
#define	D		djcomm
#define	IOSTAT		djcomm[8]
#define	INTSTAT		djcomm[10]
#define	SERIAL		0x2C
#define	DISABLE		0
#define	LOGICAL		0x2E
#define	EIGHTFIRST	0
#define	END		10
#define	PIC1		(0x4D)
#define	VI		(1<<1)  /* use int. line 1 */
#define	DJMIN		0x1030
#define	DJMAX		0x127f
#define	DJPRIORITY	PRIBIO

#define	DJINT		1
#define	TIME		 (~0)
#define	MAX		((unsigned) 3000)       /* biggest floppy boundary */
#define	CHAN		((unsigned)(djcomm))
#define	OKSTAT		0x40
#define 	NOSTAT		0
#define	BADSTAT		1
#define	COMMSIZE	9
#define	ISOPEN		(1 << 0)
#define	ORG1		(1 << 1)        /* sector numbers start from 1 */
#define	MAP		((char *) (0x602))
#define	IMAGE		((char *) (0x202))

/*
 * DJDMA command codes
 */

#define	SREAD		0x20
#define	SWRITE		0x21
#define	STATUS		0x22    /* Get drive status */
#define	SETDMA		0x23    /* Set DMA address */
#define	SETINT		0x24    /* Set interrupt */
#define	HALT		0x25    /* Controller halt */
#define	DJHALT		0x25    /* Controller halt */
#define	SETCHANNEL	0x27    /* Set channel address */
#define	SETTRACK	0x2D    /* Set max. track */
#define	MEMREAD		0xA0    /* read controller memory */
#define	MEMWRITE	0xA1    /* write controller memory */
#define	DJEXEC		0xA2

#define	VERSO		0200    /* other side */

#define	STATRET		5

/*
 * Status byte 1
 */

#define	DOUBLE		0x10    /* 1 if double density */
#define	FIVE		(1 << 2)        /* 1 if 5 1/4 inch drive */
#define	HARD		0x02    /* 1 if hard sectored */

/*
 * Status byte 2
 */

#define	S128		0       /* 128 byte sectors */
#define	S256		1       /* 256 byte sectors */
#define	S512		2       /* 512 byte sectors */
#define	S1024		3       /* 1024 byte sectors */
#define	SHARD		4       /* Hard sectored 256 or 512 byte */

/*
 * Status byte 3
 */

#define	READY		0x80    /* Drive ready bit */
#define	S_WP		(1 << 6)        /* Write protected */
#define	TRACK0		0x20    /* Drive at track zero */
#define	SIDE2		0x04    /* Double sided indicator */

#define	START		0xEF    /* DJ DMA attention port */
#define	DEFCHAN		0x50    /* default chan. addr. */

#define	RECSIZE		128
#define	BUFSIZE		512
#define 	K		1024
#define	K4		4096
#define	UCHAR		unsigned char

#define	NDRIVES		8

#define	TABADDR		0x1340  /* base address of tables in controller
                                 * memory */

/*
 * drive manufacturer codes
 *     5 1/4" drives
 */

#define TANDON		0
#define SA200		1

#define mS		* 341 / 10

#define STEP		 4      /* offset for step delay */
#define SETTLE		10      /* offset for head settle */

/*
 * delays
 */

struct delay
{
    int step, settle;
};

/*
 * the structure of the internal tables of
 * the DJDMA
 */

struct djtable
{
    char tracks,                /* no. of tracks on the drive */
         curtrack,              /* current track */
         pattern, number;

    int steprate,               /* stepping rate */
        rampup, rampdown, settle,       /* head settle delay 34.1 to the mS */
        image;

    char config, code;
};

struct status
{
    unsigned char other[8], spt, config, dev,   /* device number */
         dchar,                 /* drive characteristics */
         slength,               /* sector length */
         dstat,                 /* drive status */
         retstat;               /* command return status */
};

struct specs
{
    unsigned char config,       /* configuration byte */
         ds,                    /* double sided (boolean) */
         spt,                   /* sectors per track */
         cylinder,              /* secs. per cyl. */
         spb,                   /* sectors per block */
         ncyl,                  /* number of cylinders */
         toff;                  /* track offset */

    unsigned bps,               /* bytes per sector */
             volume;            /* secs per disk */
};

/*
 * Each drive has one of the following structures
 * associated with it. The structure holds infomation
 * about configuration and current activity.
 */

struct dm
{
    struct specs *specs;
    UCHAR flags;                /* see below */
};

static unsigned char kw = NO,   /* 1K write operation */
*haltstat = 0, djtimer = 0, djtaken = NO, kbuf[K] = { 0 };

unsigned char djcomm[26] = { 0 };

static kdev = ~0,               /* cached sector's device */
    ksec = ~0;                  /* cached sector's number */

static struct dm dm[NDRIVES] = { 0 };

static struct buf *curbuf = 0,  /* buffer currently under scrutiny */
    *qhead = 0,                 /* New requests added head. */
    *qtail = 0;                 /* Next request found here. */

static struct specs specs[] = {
    {0, NO, 26, 26, 4, 77, 2, 128, 1950},
    {0, NO, 26, 26, 2, 77, 2, 256, 1950},
    {0, NO, 15, 15, 1, 77, 2, 512, 1125},
    {0, NO, 8, 8, 1, 77, 2, 1024, 600},

    {0, YES, 26, 52, 4, 77, 2, 128, 3900},
    {0, YES, 26, 52, 2, 77, 2, 256, 3900},
    {0, YES, 15, 30, 1, 77, 2, 512, 2250},
    {0, YES, 8, 16, 1, 77, 2, 1024, 1200},

    {0x90, NO, 10, 10, 1, 35, 2, 512, 330},     /* 5 1/4 default */
    {0xF0, YES, 10, 20, 1, 35, 2, 512, 660},
    {0xB0, NO, 10, 10, 1, 35, 2, 512, 330},
    {0x10, NO, 10, 10, 2, 35, 3, 256, 320},

    {0xA0, NO, 10, 10, 1, 40, 2, 512, 380},
    {0xC0, NO, 10, 10, 1, 80, 2, 512, 780},
    {0xD0, YES, 10, 20, 1, 40, 2, 512, 760},
    {0xE0, YES, 10, 20, 1, 80, 2, 512, 1560},

    {0, NO, 18, 18, 4, 40, 0, 128, 720},        /* Xerox */
    {0, NO, 10, 10, 2, 40, 0, 256, 400},        /* Osborne */
    {0, NO, 8, 8, 1, 40, 0, 512, 320},  /* IBM dos 1 single */
    {0, NO, 5, 5, 1, 40, 0, 1024, 200}, /* single sided Morrow */
    {0, NO, 9, 9, 1, 40, 0, 512, 360},  /* IBM dos 2, single */

    {0, YES, 18, 36, 4, 40, 0, 128, 1440},
    {0, YES, 10, 20, 2, 40, 0, 256, 800},
    {0, YES, 8, 16, 1, 40, 0, 512, 640},
    {0, YES, 5, 10, 1, 40, 0, 1024, 400},       /* Dual Morrow */
    {0, YES, 9, 18, 1, 40, 0, 512, 720},        /* IBM dos 2, double */

    /*
     * {0, NO, 10, 10, 1, 40, 0, 512, 400}, /* radio shack * 
     */

    {END, NO, 0, 0, 0, 0}
};

struct delay delay[] = {

    /*
     * 8" types
     */

    8 mS, 8 mS,                 /* SA 800 */
    3 mS, 15 mS,                /* SA 850 */
    8 mS, 8 mS,                 /* SA 800 (reserved) */
    8 mS, 8 mS,                 /* SA 800 (reserved) */

    /*
     * 5 " types
     */

    5 mS, 15 mS,                /* Tandon (reserved) */
    20 mS, 20 mS,               /* SA 200 */
    40 mS, 10 mS,               /* SA 400 */
    5 mS, 15 mS,                /* Tandon */
};

djopen(dev, mode)
    register dev, mode;
{
    static struct status *s;
    static struct dm *d;
    static struct specs *sp, *S;
    static struct buf *b;

    if (djtaken)
        return djbusy();

    d = &dm[dev & 7];

    if (d->flags & ISOPEN) {    /* already open */
        /*
         * a drive may be opened as only one type of drive at once
         */
        if ((dev & (ALT | TYPE | DOUBSIDE)) !=
            (d->flags & (ALT | TYPE | DOUBSIDE)))
            return djbusy();
    }

    /*
     * fill in information about the device
     * Do a DJDMA getstatus command.
     */

    b = bread(0, dev | GETSTAT);

    if (b == NULL) {
      err:
        u.error = EIO;
        return;
    }

    b->dev |= DISCARD;
    s = b->data;

    /*
     * deal with the Write Protect bit
     */

    if (s->dstat & S_WP) {      /* note write enable/disable status */
        d->flags |= F_WP;

        if (mode != READ)
            goto err;           /* read only */
    } else {
        d->flags &= ~F_WP;
    }

    /*
     * find sp, the specs for this drive and diskette
     */

    if (s->dchar & HARD) {      /* hard sectored, table lookup */
        sp = &specs[DEFSPECS];

        if (s->config)          /* reasonable */
            for (S = specs; S->config != END; S++)
                if (S->config == s->config)
                    sp = S;     /* change our minds */
    }

    else {                      /* soft sectored */
        /*
         * All soft sectored formats have one origin sector numbers.
         */
        d->flags |= ORG1;

        /*
         * select a format based on sector length
         */

        sp = &specs[s->slength & 3];

        /*
         * offset based on sidedness
         */

        if (dev & FIVE) {
            sp += FIVEBASE;

            if ((dev & DOUBSIDE) || s->config == DOUBSIDE)
                sp += 5;
        }

        else {                  /* eight inch */
            if (s->dstat & SIDE2)
                sp += 4;
        }

        /*
         * Adjustment for IBM 9 spt
         */

        if (s->spt == 9 && s->slength == S512) {
            sp += 2;
        }
    }

    brelse(b);

    if ((d->flags & ISOPEN) && sp != d->specs) {
        djclose(dev);           /* diskette has been changed */
        goto err;
    }

    d->specs = sp;

    d->flags |= ISOPEN | (dev & (ALT | TYPE | DOUBSIDE));       /* set the
                                                                 * alternation 
                                                                 */
}

/*
 * Close the drive, disallowing further access.
 */

djclose(dev)
    register dev;
{
    dm[dev & 7].flags = 0;

    if (dev == kdev)
        kdev = -1;              /* de-cache */
}

/*
 * link the buffer into the queue
 */

djstrat(b)
    register struct buf *b;
{
    b->forw = NULL;

    di();

    if (qtail) {
        qhead->forw = b;
        qhead = b;
    } else {                    /* No request yet. */
        qhead = qtail = b;
    }

    djint();

    ei();
}

static
sio(d, addr, xaddr, op, sec, dev)
    register unsigned sec;
    register struct dm *d;
{
    static unsigned char spt;
    static struct specs *sp;

    sp = d->specs;

    spt = sp->spt;

    D[0] = SETDMA;              /* set DMA command */

    /*
     * D[1] = addr; D[2] = addr >> 8; 
     */
    *(char **) (D + 1) = addr;

    D[3] = xaddr;

    D[4] = op;                  /* I/O command */

    D[5] = (sec / sp->cylinder) + sp->toff,     /* track */
        D[6] = sec % spt;       /* basic 0-origin sector number */

    if (dev & ALT) {            /* alternate sectors */
        D[6] <<= 1;             /* D[6] *= 2; */

        if (!(spt & 1) && D[6] >= spt)
            D[6]++;

        D[6] %= spt;
    }

    if (d->flags & ORG1)
        D[6]++;                 /* 1-org. sectors correction */

    if (sp->ds && (sec % sp->cylinder) >= spt)
        D[6] |= VERSO;          /* flip side (head) */

    D[7] = dev & 7;             /* drive */

    D[8] = NOSTAT;              /* io status will be returned here */

    D[9] = SETINT;
    D[10] = NOSTAT;

    D[11] = HALT;

    /*
     * D[12] = NOSTAT; 
     */

    haltstat = &D[12];

    /*
     * a command is 13 bytes long
     */

}

/*
 * wait a little while to see if the given location changes
 */

static
djwait(a)
    char *a;
{
    unsigned int i;

    for (i = TIME; i && *a == NOSTAT; i--);
}

static unsigned char inservice = NO;

djint()
{
    static unsigned char op, spb;
    static unsigned addr, xaddr, sec, off, blk, bps, count, togo, dev;
    static struct dm *d;
    static struct specs *sp;

    pr("");                     /* timing delay kludge */

    /*
     * make sure only one enters at a time
     * at the beginning of djint, curbuf determines
     * whether this int. came from the contoller or the strat routine
     */

    if (djtaken) {              /* a djexec command */
        djack();
        wakeup(djcomm);
        return;
    }

    di();

    if (inservice               /* already working on it */
        || (curbuf && INTSTAT == NOSTAT)        /* not our int. */
        ||(!curbuf && !qtail)   /* not our int. */
        ||(busget(&djint) == NO)        /* can't get bus yet. See mw.c */
        ) {
        ei();
        return;
    }

    else {
        inservice = YES;
        ei();
    }

    djtimer = 0;                /* reset the deadman switch */

    /*
     * first, check up on the current operation
     */

    if (curbuf) {               /* something happening */
        /*
         * Yes, a DJDMA int. command has been executed
         */
        /*
         * check to see if the operation(s) failed
         */
        if (IOSTAT != OKSTAT) { /* failed */
          fail:
            curbuf->error = EIO;
          berr:
            curbuf->flags |= BERROR;

          finished:

            /*
             * placate the system, send wakeups, etc.
             * note that nothing really exciting will happen just now
             * In particular, an interrupt won't happen on our V.I. line
             */

            iodone(curbuf);
            curbuf = 0;

            /*
             * push the controller past the interrupt command
             * this is the interrupt acknowledge section
             * we must first tell the DJDMA board to turn off its
             * interrupt line, then tell the PIC chip that this int.
             * is serviced
             */

            djack();

            if (!qtail)
                goto request;

            if (busgive(&djint) || !busget(&djint)) {
                inservice = NO;
                ei();
                return;
            }

            goto request;       /* set up next req. */
        }

        /*
         * yes, the previous op(s) worked
         * if bps <= count, then the previous command
         * has already send the data directly to its destination
         */

        if (!off && bps <= count)       /* small sectors */
            goto secdone;

        /*
         * partial request
         */

        if (!kw) {              /* the cache has just been filled */
            kdev = dev;         /* mark the cache valid */
            ksec = sec;
        }

        if (curbuf->flags & BREAD) {    /* partial read operation worked */
            /*
             * copy the information from the 1K cache out to the block
             */
          kread:

            copy(kbuf + off, addr, togo);

          secdone:
            count -= togo;

            if (count) {
                addr += togo;
                sec++;
                off = 0;
                djack();
                goto nextsec;
            }

            else {
                goto finished;
            }
        }

        /*
         * partial write request
         */

        if (kw) {               /* the write op. must be complete */
            goto finished;
        }

        /*
         * partial, between operations
         */

      kwrite:
        copy(curbuf->data, kbuf + off, togo);

        djack();                  /* acknowledge old int. */

        /*
         * an int. on our line could happen now
         * but would be discarded as spurious because inservice is set.
         */

        sio(d, kbuf, KERNEL, SWRITE, ksec, dev);        /* setup new comm. */

        kw = YES;               /* doing the write portion of a 1K req. */

        goto go;

    }

    /*
     * nohing happening, pluck a buffer
     */

  request:

    di();

    if (!qtail) {               /* all done , No more requests */
        inservice = NO;
        ei();
        busgive(NULL);
        return;
    }

    curbuf = qtail;
    qtail = qtail->forw;

    ei();

    /*
     * set up the next command
     */

    dev = curbuf->dev;

    if (dev & GETSTAT) {        /* stat req. */
        if (getstat(dev)) {
            copy(djcomm, curbuf->data, sizeof(struct status));
            goto finished;
        } else {
            goto fail;
        }
    }

    d = &dm[dev & 7];

    if ((d->flags & ISOPEN) == 0)       /* not open */
        goto fail;

    sp = d->specs;

    count = curbuf->count;
    blk = curbuf->blk;

    if (blk > MAX) {            /* outrageous */
      range:
        curbuf->error = ENXIO;
        goto berr;
    }

    bps = sp->bps;
    spb = sp->spb;

    /*
     * find the sector number of the beginning of the transfer
     */

    off = 0;

    togo = bps;                 /* transfer 1 whole sector */

    if (count < togo)           /* unless less is requested */
        togo = count;

    if (bps == K) {             /* 1K blocks */
        sec = blk >> 1;         /* sec = blk / 2 */

        if (blk & 1) {          /* odd numbered block */
            off = BUFSIZE;

            if (togo > BUFSIZE)
                togo = BUFSIZE; /* truncate for K bounds. */
        }
    } else {
        sec = blk * spb;        /* sector number of the volume */
    }

    op = (curbuf->flags & BREAD) ? SREAD : SWRITE;

    if (op == SWRITE && (d->flags & F_WP))
        goto fail;

    addr = curbuf->data;
    xaddr = curbuf->xmem;

  nextsec:

    if (sec >= sp->volume)      /* off the end */
        goto range;

    /*
     * when the sector fits entirely within the count,
     * things are simple
     * we merely transfer directly
     * to x-mem space
     */

    if (!off && bps <= count) { /* small sector sizes and alignment */
        sio(d, addr, xaddr, op, sec, dev);

      go:
        di();
        djstart();
        inservice = NO;
        ei();
        return;
    }

    /*
     * partial request
     *
     * The first step is to bring the desired sector into the cache
     * If its not already there.
     *
     */

    if (ksec != sec || kdev != dev) {   /* not in cache */
        kdev = -1;              /* cache invalid */
        sio(d, kbuf, KERNEL, SREAD, sec, dev);
        kw = NO;
    }

    else if (op == SREAD)       /* partial cached read */
        goto kread;

    else                        /* cached write */
        goto kwrite;

    goto go;
}

static
djstart()
{
    di();

    if (haltstat)
        *haltstat = NOSTAT;

    out(START, 0);
    ei();
}

#define S ((struct status *) djcomm)

static
getstat(a)
    register a;
{
    static unsigned char type;
    static unsigned astep, asettle;
    static struct delay *d;
    static struct status *s;

    kdev = -1;                  /* cache invalid */

    /*
     * Set the step and settle times
     *
     * structure pointer into delay table
     */

    type = (a >> 4) & 3;        /* drive type # (bits 4 & 5 ) */

    if (a & FIVE)
        type += 4;              /* use last half of tab for 5" */

    d = delay + type;           /* structure pointer into table */

    /*
     * calculate internal controller memory addresses
     * 16 bytes per table entry
     */

    a &= 7;

    astep = TABADDR + (a << 4); /* ADDR + 16 * drive */
    asettle = astep;

    astep += STEP;              /* address of the step value */
    asettle += SETTLE;          /* address of the settle value */

    memio(WRITE, &d->step, astep, sizeof d->step);
    memio(WRITE, &d->settle, asettle, sizeof d->settle);

    /*
     * set the dma address to kbuf
     */

    D[0] = SETDMA;
    *(char **) (D + 1) = kbuf;
    D[3] = KERNEL;

    D[4] = SREAD;               /* move to cyl 3 */
    D[5] = 3;                   /* cylinder */
    D[6] = 1;                   /* sector */
    D[7] = a;                   /* drive */
    D[8] = NOSTAT;

    D[9] = STATUS;              /* get stat */
    D[10] = a;

    /*
     * D[11] = NOSTAT; 
     */

    /*
     * D[12] = NOSTAT; 
     */

    /*
     * D[13] = NOSTAT; 
     */
    D[14] = NOSTAT;

    D[15] = HALT;
    D[16] = NOSTAT;

    djstart();
    ei();
    djwait(&D[16]);
    di();

    if (D[14] != OKSTAT)
        return NO;

    /*
     * read sector zero of 5 inch hard diskettes to find the config byte.
     * read sector 9 of soft 5's to differentiate IBM's
     * Sometimes, the dos config byte will eliminate the need
     * for performing the attemped read of sec 9.
     */

    S->spt = 8;

    D[0] = SREAD;
    D[1] = 0;                   /* cylinder */
    D[2] = 0;                   /* sector */
    D[3] = a;                   /* drive */

    /*
     * D[ 4] = NOSTAT; 
     */

    D[5] = HALT;

    /*
     * D[ 6] = NOSTAT; 
     */

    if (S->dchar & HARD) {

        /*
         * D[ 0] = SREAD; /* move to cyl 3 
         */

        /*
         * D[ 1] = 0; /* cylinder 
         */

        /*
         * D[ 2] = 0; /* sector 
         */

        /*
         * D[ 3] = a; /* drive 
         */

        /*
         * D[ 4] = NOSTAT; 
         */

        /*
         * D[ 5] = HALT; 
         */

        /*
         * D[ 6] = NOSTAT; 
         */

        readone();

        if (D[4] != OKSTAT)
            return NO;

        S->config = kbuf[0x5C]; /* copy the configuration byte */
    }

    else if (S->slength == S512 && (a & FIVE)) {        /* IBM maybe */
        /*
         * First we'll try to take the short cut by reading the configuration
         * which may be available as a result of an IBM format.
         * This information is found in bytes 0, 1, and 2 of
         * sector 2, cyl, 0, head, 0
         *
         * And now a short quote from IBM PC DOS 2.0 manual 
         * Appendix C, page 7
         *
         * The FAT begins in the second sector of the volume.
         *
         *     The second and third bytes always contain hex FFFF.
         *     
         *     The first byte is used as follows:
         *
         *     FF      Dual sided, 8 spt
         *     FE      Single sided, 8 spt
         *     FD      Dual sided, 9 spt
         *     FC      Single sided, 9 spt
         *
         * It doesn't say so in the manual, but we interpret this to mean
         * bit 0 is the dual sided bit and bit 1 is the 8spt bit.
         *
         */
        /*
         * D[ 0] = SREAD;      
         */
        /*
         * D[ 1] = 0;  /* cylinder 
         */
        D[2] = 2;               /* sector */

        /*
         * D[ 3] = a; /* drive 
         */

        /*
         * D[ 4] = NOSTAT; 
         */

        /*
         * D[ 5] = HALT; 
         */

        /*
         * D[ 6] = NOSTAT; 
         */

        readone();

        if (D[4] == OKSTAT      /* read worked */
            && kbuf[1] == 0xff  /* valid dos fat */
            && kbuf[2] == 0xff && (*kbuf & 0xfc) == 0xfc        /* valid
                                                                 * config.
                                                                 * byte */
            ) {
            if (*kbuf & 1)
                S->config = DOUBSIDE;

            if (!(*kbuf & 2))
                S->spt = 9;

            return YES;         /* short cut */
        }

        /*
         * Attempt reading sector 9 of track 0 to discriminate
         * between 8 and 9 sector per track formats.
         */

        /*
         * D[ 0] = SREAD;      
         */
        /*
         * D[ 1] = 0;  /* cylinder 
         */
        D[2] = 9;               /* sector */

        /*
         * D[ 3] = a; /* drive 
         */

        /*
         * D[ 4] = NOSTAT; 
         */

        /*
         * D[ 5] = HALT; 
         */

        /*
         * D[ 6] = NOSTAT; 
         */

        readone();

        if (D[4] == OKSTAT) {   /* read of sec 9 was successful */
            S->spt = 9;         /* therefore 9 sec. per trk. */
        }
    }

    else if (S->slength == S1024 && (a & FIVE)) {

        /*
         * D[ 0] = SREAD; 
         */

        /*
         * D[ 1] = 0; /* cylinder 
         */
        D[2] = 1;               /* sector */

        /*
         * D[ 3] = a; /* drive 
         */

        /*
         * D[ 4] = NOSTAT; 
         */

        /*
         * D[ 5] = HALT; 
         */

        /*
         * D[ 6] = NOSTAT; 
         */

        readone();

        if (kbuf[0x81] & 4) {   /* Morrow double bit set */
            static char *p, n, s;

            p = kbuf + 80;
            n = 25;             /* 25 bytes to xor */
            s = 0;              /* preset sum */

            while (n--)         /* xor checksum */
                s ^= *p++;

            if (s == 0)
                S->config = DOUBSIDE;
        }
    }

    return YES;
}

#undef S

static
readone()
{
    D[6] = NOSTAT;
    djstart();
    ei();
    djwait(&D[6]);
    di();
}

/*
 * tell the PIC we're done with the current interrupt
 */

djack()
{
    if (!haltstat)
        return;

    djstart();                  /* get past the int. command */
    djwait(haltstat);
    haltstat = 0;
}

djgoose()
{
    if (haltstat && ++djtimer >= DJTHRESHHOLD) {
        IOSTAT = BADSTAT;
        INTSTAT = OKSTAT;
        djint();
    }

    timeout(djgoose, 0, DJINTERVAL);
}

djmclose()
{
    djtaken = NO;
}

djmopen()
{
    static char i;

    if (curbuf)
        return djbusy();

    for (i = 0; i < 8; i++)
        if (dm[i].flags & ISOPEN)
            return djbusy();

    djtaken = YES;
}

static
djbusy()
{
    u.error = EBUSY;
}

/*
 * djexec () - execute contoller routine 
 *
 */

djstty(dev, flag)
{
    static char *a;

    busplease();

    iomove(WRITE, &a, sizeof a);        /* copy in argument */

    if (flag == READ) {         /* status command */
        D[0] = STATUS;
        D[1] = a;

        /*
         * D[2] = NOSTAT;       
         */
        /*
         * D[3] = NOSTAT;       
         */
        /*
         * D[4] = NOSTAT;       
         */

        /*
         * D[5] = NOSTAT;       
         */

        D[6] = SETINT;
        /*
         * D[7] = NOSTAT;       
         */

        D[8] = HALT;
        /*
         * D[9] = NOSTAT;       
         */

        a = D + 2;
        haltstat = D + 9;
    }

    else {
        D[0] = DJEXEC;

        *(char **) (D + 1) = a; /* controller mem. address */

        /*
         * D[3] = NOSTAT;               
         */

        D[4] = SETINT;
        /*
         * D[5] = NOSTAT;               
         */

        D[6] = DJHALT;
        /*
         * D[7] = NOSTAT;               
         */

        a = D + 3;
        haltstat = D + 7;
    }

    di();

    djtimer = 0;
    djstart();

    sleep(djcomm, DJPRIORITY);

    busthanks();

    iomove(READ, a, 4);         /* return status */
}

/*
 * write to DJ controller memory
 */

djmwrite()
{
    djmio(WRITE);
}

/*
 * read from controller memory
 */

djmread()
{
    djmio(READ);
}

djmio(flag)
{
    static char *a;
    static unsigned b;

    a = u.offset;               /* djcontroller mem address */
    b = u.count;                /* number of bytes to transfer */

    /*
     * memory address range checking
     */

    if (b > K)
        return u.error = EINVAL;

    if (flag == WRITE)
        iomove(WRITE, kbuf, b);

    busplease();

    memio(flag, kbuf, a, b);

    busthanks();

    if (flag == READ)
        iomove(READ, kbuf, b);
}

static
memio(flag, memaddr, djaddr, count)
{
    D[0] = (flag == READ) ? MEMREAD : MEMWRITE;

    *(char **) (D + 1) = memaddr;

    D[3] = KERNEL;

    *(int *) (D + 4) = count;

    *(char **) (D + 6) = djaddr;

    D[8] = HALT;
    D[9] = NOSTAT;

    djstart();
    djwait(&D[9]);
}

static
busready()
{
    wakeup(busready);
}

/*
 * get the bus - in line
 */

static
busplease()
{
    di();

    if (!busget(busready))
        sleep(busready, DJPRIORITY);
    else
        ei();
}

static
busthanks()
{
    busgive(NULL);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
