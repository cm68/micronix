       /*
        * Allocate and free primary and secondary memory
        */

#include "sys.h"
#include "proc.h"
#include "buf.h"
#include "sup.h"
#include "con.h"

#define NONE    0       /* Decision cpu memory-access bits */
#define FULL    3
#define GROW    7

extern char memwant;

        /*
         * Number of free segments
         */
UINT    nsegs = 0;      /* set in meminit(), mem.s */

        /*
         * Segment map. Segment n is allocated iff segmap[n] != 0.
         */
char    segmap[MAXSEG] = 0;
static UINT lastseg = 16;

        /*
         * Allocate a segment
         */
segalloc()
        {
	unsigned n, limit;

        di();

	n = lastseg;		/* a likely place to look */

	for (limit = MAXSEG - 16; limit; limit--, n++)
		{
		if (MAXSEG <= n)
			{
			n = 16;		/* wrap around */
			}

		if (!segmap [n])	/* segment free ? */
                        {
			segmap [n] = 1;
                        nsegs--;
			lastseg = n + 1;	/* advice for the future */
                        ei();
                        return n;
                        }
		}

        ei();
/*
	panic("Mem all.");
*/
        }

        /*
         * Free a segment
         */
segfree(n)
        unsigned n;
        {
        di();
        if (segmap[n])		/* allocated ? */
		{
                nsegs++;
	        segmap[n] = 0;
		lastseg = n;		/* advice */
		}
        ei();
        }

       /*
        * Free a process' memory.
        * Called by exit(), swapout().
        */
mfree(p)
        struct proc *p;
        {
	static i;
        static struct mem * m;

        m = p->mem;
	for (i = 16; i >= 0; i--)
                {
                segfree(m[i].seg);
                m[i].seg = 0;
                }
        mwake();
        }

        /*
         * Wake up memory sleepers
         */
mwake()
        {
        if (memwant)
                wakeup(&mfree);
        memwant = NO;
        }

       /*
        * Free memory, reset permissions, and distribute
        * a grow segment. Called by exec().
        */
mrelse()
        {
        static UCHAR i, grow;
        static struct mem * m;

        m = u.p->mem;
        grow = m->seg;
        m->per = GROW;
	for (i = 14; i >= 1; i--)
                {
                m = &u.p->mem[i];
                if (m->seg != grow)
                        segfree(m->seg);
                m->seg = grow;
                m->per = GROW;
                }
        u.p->nsegs = 3;         /* includes u-structure segment */
        newmap(u.p);
        mwake();
        }

       /*
        * Restore the memory of a swapped-out process.
        * Called by swapin(), and also by fork().
        */
mget(p)
        struct proc *p;
        {
        static UCHAR grow, i;
        static struct mem *m;

        if (p->nsegs > nsegs)
                return NO;
        grow = 0;
        for (i = 0; i < 17; i++)
                {
                m = &p->mem[i];
                if (m->per == GROW)
                        {
                        if (grow == 0)
                                grow = segalloc();
                        m->seg = grow;
                        }
                else
			{
                        m->seg = segalloc();
			}
                }
        return YES;
        }

       /*
        * Grow the current process in response to a memory fault.
        * Nail down the fault segment, and allocate another
        * grow segment if necessary. Sleep for memory.
        * Called by fault() and valid() (below).
        */
grow(seg)
        UINT seg;
        {
        static char i, grow;
        static struct mem *m;
        #define brakeseg ((UINT)u.brake >> 12)
        #define stackseg ((UINT)u.sp >> 12)


        m = &u.p->mem[seg];

        if (m->per == FULL)
		{
                return YES;
		}

        if (brakeseg < seg && seg < stackseg)
                {
                u.error = ENXIO;
                send(u.p, SIGMEM);
                return NO;
                }

        m->per = FULL;

        if (u.p->nsegs == 17)
		{

/*
 * A very old bug.  Jan 1984  Len Edmondson
 * We need to rewrite the map to the hardware registers here!.
 */

		newmap(u.p);

                return YES;
		}

	u.p->nsegs++;

        u.p->swap = 0;

        while (nsegs == 0)
                {
                memwant = YES;
                sleep(&mfree, PRIMEM);
                }

        if (u.p->swap)
                return YES;

        grow = segalloc();
        m = u.p->mem;

        for (i = 0; i < 16; i++)
                if (m[i].per == GROW)
                        m[i].seg = grow;

        newmap(u.p);
        return YES;
        }

       /*
        * Check legality of an interval of user memory.
        * Grow the process as appropriate.
        * Called by read(), stat(), ...
        */
valid(addr, count)
        UINT count, addr;
        {
        fast UCHAR first, last, i;

        if (count != 0)
                {
                first = addr >> 12;
                last = (addr + count - 1) >> 12;
                for (i = first; i <= last; i++)
                        if (u.p->mem[i].per == GROW && !grow(i))
                                return NO;
                }
        return YES;
        }

#define segtop(x)       ((((x) << 12) + 4095))
#define segbot(x)       (((x) << 12))

       /*
        * Memory fault handler.
        * Called from trap() (trap.c).
        */
fault()
        {
        fast UCHAR tseg, pseg, temp;
        extern UCHAR trapad;


        tseg = trapad >> 4;             /* high nibble of trap addr */
        pseg = trapad & 0x0F;           /* low nibble */


        if (!grow(tseg))
		{
                return;
		}

        if (u.p->mem[pseg].per == FULL)
		{
                return;
		}

        if (tseg < pseg)
                {
                temp = getbyte (segbot(tseg));
                grow(pseg);
                putbyte (temp, segbot (pseg));
                }
        else
                {
                temp = getbyte (segtop(tseg));
                grow(pseg);
                putbyte (temp, segtop (pseg));
                }
        }

        /*
         * Copy the core image of the current process to a new process.
         * Called from procopy (fork.c).
         */
bankcopy(p)
        struct proc * p;
        {
        static char i;

        for (i = 0; i < 17; i++)
                if (u.p->mem[i].per == FULL)
                        segcopy(u.p->mem[i].seg, p->mem[i].seg); /* mem.s */
        }

        /*
         * Write the current processes' memory map
         * to task1's segment registers.
         */
newmap(n)
        struct proc *n;
        {
        extern UCHAR image0[], image1[], map0[], map1[];

        di();
        map0[2*USERSEG] = image0[2*USERSEG] = n->mem[16].seg;
        u.p = n;        /* fork() did not set this */
        copy(&u.p->mem, &image1, 32);
        copy(&u.p->mem, &map1, 32);
        ei();
        }


       /*
        * The swap map lists the free segments of swap space
        * in increasing-address order. The size of each segment
        * is an integral number of 512 byte blocks. The end of
        * the map is indicated by a 0 size.
        */
#define MAPSIZE (NPROC + 2)
struct map
        {
        unsigned size;
        unsigned addr;
        }
swapmap[MAPSIZE] = 0;

       /*
        * Initialize the swapmap
        */
swapinit()
        {
        fast struct buf *b;
        fast struct sup *s;

        if (swapdev == rootdev)
                {
                b = getsb(rootdev);
                s = b->data;
                swapaddr = s->fsize;
                brelse(b);
                }
        swapmap[0].size = swapsize;
        swapmap[0].addr = swapaddr;
        swapmap[1].size = 0;
        }

       /*
        * Allocate size (!= 0) blocks of swap space.
        */
salloc(size)
        fast int size;
        {
        fast struct map *mp;
        fast int blk;

        for (mp = swapmap; mp->size != 0; mp++)
                {
                blk = mp->addr;
                if (mp->size == size)
                        {
                        lshift(mp);
                        return (blk);
                        }
                if (mp->size > size)
                        {
                        mp->size -= size;
                        mp->addr += size;
                        return (blk);
                        }
                }
        pr("Out of swap space");
        return NO;
        }

       /*
        * Add a segment of swap space to the swap map
        */
sfree(size, blk)
        int  size, blk;
        {
        fast struct map *mp;

        for (mp = swapmap; mp->addr < blk && mp->size != 0; mp++)
                ;
        rshift(mp);
        mp->size = size;
        mp->addr = blk;
        if (mp > swapmap)
                mp--;
        while (mp->addr + mp->size == (mp + 1)->addr)
                {
                mp->size += (mp + 1)->size;
                lshift(mp + 1);
                }
        }

       /*
        * Shift the map left, overwriting *mp
        */
lshift(mp)
        fast struct map *mp;
        {
        for (; mp->size != 0; mp++)
                {
                mp->size = (mp + 1)->size;
                mp->addr = (mp + 1)->addr;
                }
        }

       /*
        * Shift the map right, duplicating *mp
        */
rshift(mp)
        struct map *mp;
        {
        fast struct map *np;

        for (np = mp; np->size != 0; np++) /* find the end */
                ;
        for (np++; np != mp; np--)
                {
                np->size = (np - 1)->size;
                np->addr = (np - 1)->addr;
                }
        }
