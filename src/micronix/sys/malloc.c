/*
 * Allocate and free primary and secondary memory
 *
 * sys/malloc.c 
 * Changed: <>
 */

#include <types.h>
#include <sys/sys.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/fs.h>
#include <sys/con.h>
#include <sys/dlabel.h>
#include <sys/ovl.h>
#include <sys/signal.h>
#include <errno.h>

#define NONE    0               /* Decision cpu memory-access bits */
#define FULL    3
#define GROW    7

extern char memwant;

/*
 * Number of free segments
 */
UINT nsegs = 0;                 /* set in meminit(), mem.s */

/*
 * Segment map. Segment n is allocated iff segmap[n] != 0.
 */
char segmap[MAXSEG] = 0;
static UINT lastseg = 16;

/*
 * Task map.  Bit n is set while task n is assigned to a process.
 *
 * The hardware has sixteen tasks and gives each of 1..15 a 32-byte map
 * table of its own in map ram (0x600 + 32*task, selected by the low
 * nibble of the task register 0x402); task 0 is the kernel's and the
 * only supervisor task.  A process that owns a slot keeps its memory
 * map across a switch, so the map is copied when it changes (taskload)
 * rather than on every switch (newmap).
 *
 * A slot is not the process's memory.  It can be taken back and handed
 * to another process, and the loser is loaded from its mem[] again the
 * next time it is switched in - so a process's table is a cache of its
 * map and never the only copy of it.
 */
#define NTASK	16              /* tasks 1..15 are for processes */
UINT taskmap = 0;

/*
 * Allocate a segment
 */
segalloc()
{
    unsigned n; UINT8 limit;

    di();

    n = lastseg;                /* a likely place to look */

    for (limit = MAXSEG - 16; limit; limit--, n++) {
        if (MAXSEG <= n) {
            n = 16;             /* wrap around */
        }

        if (!segmap[n]) {       /* segment free ? */
            segmap[n] = 1;
            nsegs--;
            lastseg = n + 1;    /* advice for the future */
            ei();
            return n;
        }
    }

    ei();

    /*
     * The buffer cache (binit()/expand_bufs()) now also allocates from
     * here, so a segment shortage no longer means a process can be
     * swapped out to make room.  Returning silently plants a garbage
     * segment and aliases two buffers or two process pages, which shows
     * up later as a block read that is not the block asked for.
     */
    pr("segalloc: out of segments (nsegs %d)\n", nsegs);
    panic("segalloc: out of segments");
}

/*
 * Free a segment
 */
segfree(n)
    unsigned n;
{
    di();
    if (segmap[n]) {            /* allocated ? */
        nsegs++;
        segmap[n] = 0;
        lastseg = n;            /* advice */
    }
    ei();
}

/*
 * True if q is the better task-slot victim of the two: a process that
 * is not runnable beats one that is, and between equals the older - the
 * larger residency time - goes first.  findout() (swap.c) sorts the
 * same way for the same reason.  A process asleep on an event is the
 * one least likely to want its map back soon, and among those the one
 * resident longest is the one whose wakeup is furthest away: of a
 * shell, a make and a compiler all asleep in wait(), the shell is the
 * oldest and the one waiting on the whole chain.
 */
static
staler(q, n)
    struct proc *q, *n;
{
    if (!(q->mode & AWAKE) != !(n->mode & AWAKE))
        return !(q->mode & AWAKE);
    return q->time > n->time;
}

/*
 * Allocate a task slot to p.  The lowest free one, or - when all
 * fifteen are taken - the slot of the process that is least likely to
 * want its map back soon, whose task field is cleared; it is loaded
 * from mem[] again the next time it is switched in (newmap).
 */
taskalloc(p)
    struct proc *p;
{
    register struct proc *q, *n;
    UINT8 task;

    di();
    for (task = 1; task < NTASK; task++)
        if (!(taskmap & (1 << task)))
            break;

    if (task == NTASK) {        /* all taken: take one back */
        n = 0;
        for (q = plist; q < plist + NPROC; q++)
            if (q->task && (n == 0 || staler(q, n)))
                n = q;
        if (n == 0)             /* nothing holds a slot: NTASK is wrong */
            panic("taskalloc: no task slots");
        task = n->task;
        n->task = 0;
    }

    taskmap |= 1 << task;
    p->task = task;
    ei();
}

/*
 * Free a task slot.  Called with the segments themselves (mfree), so
 * what the table holds means nothing after this.
 */
taskfree(p)
    struct proc *p;
{
    if (p->task == 0)
        return;
    di();
    taskmap &= ~(1 << p->task);
    p->task = 0;
    ei();
}

/*
 * Copy p's map into the map ram of the task that holds it.
 *
 * Task 0 may write any task's table, and the hardware reads a table
 * only while that task is current, so this needs no switch.  It is what
 * a map change has to do rather than waiting for the next newmap: a
 * trap returns to user mode - and to the process's own task - without
 * one (trap.c), and a fault grows the process inside a trap.
 */
taskload(p)
    struct proc *p;
{
    if (p->task)
        copy(&p->mem[0].seg, (char *)(0x600 + (p->task << 5)), 32);
}

/*
 * Free a process' memory.
 * Called by exit(), swapout().
 */
mfree(p)
    struct proc *p;
{
    INT8 i;
    struct mem *m;

    taskfree(p);                /* the pages are going, so is the table */
    m = p->mem;
    for (i = 16; i >= 0; i--) {
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
    memwant = 0;
}

/*
 * Free memory, reset permissions, and distribute
 * a grow segment. Called by exec().
 */
mrelse()
{
    UINT8 i, grow;
    struct mem *m;

    m = u.p->mem;
    grow = m->seg;
    m->per = GROW;
    for (i = 14; i >= 1; i--) {
        m = &u.p->mem[i];
        if (m->seg != grow)
            segfree(m->seg);
        m->seg = grow;
        m->per = GROW;
    }
    u.p->nsegs = 3;             /* includes u-structure segment */
    newmap(u.p);
    taskload(u.p);              /* every page but the u segment moved */
    mwake();
}

/*
 * Restore the memory of a swapped-out process.
 * Called by swapin(), and also by fork().
 */
mget(p)
    struct proc *p;
{
    UINT8 grow, i;
    struct mem *m;

    if (p->nsegs > nsegs) {
        pr("mget: p->nsegs %d > free nsegs %d\n", p->nsegs, nsegs);
        return 0;
    }
    grow = 0;
    for (i = 0; i < 17; i++) {
        m = &p->mem[i];
        if (m->per == GROW) {
            if (grow == 0)
                grow = segalloc();
            m->seg = grow;
        } else {
            m->seg = segalloc();
        }
    }
    /*
     * A process coming back from the swapper, or a fork child, is
     * loaded but not current, so its table is only stale if it kept a
     * slot through the change (taskload checks).
     */
    taskload(p);
    return 1;
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
    char i, grow;
    struct mem *m;

#define brakeseg ((UINT)u.brake >> 12)
#define stackseg ((UINT)u.sp >> 12)

    m = &u.p->mem[seg];

    if (m->per == FULL) {
        return 1;
    }

    if (brakeseg < seg && seg < stackseg) {
        u.error = ENXIO;
        send(u.p, SIGSEGV);
        return 0;
    }

    m->per = FULL;

    if (u.p->nsegs == 17) {

        /*
         * A very old bug.  Jan 1984  Len Edmondson
         * We need to rewrite the map to the hardware registers here!.
         * taskload and not newmap: the trap that called us returns to
         * user mode without a newmap, so the permission byte this just
         * changed has to reach the installed table now.
         */

        taskload(u.p);

        return 1;
    }

    u.p->nsegs++;

    u.p->swap = 0;

    while (nsegs == 0) {
        memwant = 1;
        sleep(&mfree, PRIMEM);
    }

    if (u.p->swap)
        return 1;

    grow = segalloc();
    m = u.p->mem;

    for (i = 0; i < 16; i++)
        if (m[i].per == GROW)
            m[i].seg = grow;

    taskload(u.p);              /* the grow segment, into the live table */
    return 1;
}

/*
 * Check legality of an interval of user memory.
 * Grow the process as appropriate.
 * Called by read(), stat(), ...
 */
valid(addr, count)
    UINT count, addr;
{
    register UINT8 first, last, i;

    if (count != 0) {
        first = addr >> 12;
        last = (addr + count - 1) >> 12;
        for (i = first; i <= last; i++)
            if (u.p->mem[i].per == GROW && !grow(i))
                return 0;
    }
    return 1;
}

#define segtop(x)       ((((x) << 12) + 4095))
#define segbot(x)       (((x) << 12))

/*
 * Memory fault handler.
 * Called from trap() (trap.c).
 */
fault()
{
    register UINT8 tseg, pseg, temp;
    extern UINT8 trapad;

    tseg = trapad >> 4;         /* high nibble of trap addr */
    pseg = trapad & 0x0F;       /* low nibble */

    if (!grow(tseg)) {
        return;
    }

    if (u.p->mem[pseg].per == FULL) {
        return;
    }

    if (tseg < pseg) {
        temp = getbyte(segbot(tseg));
        grow(pseg);
        putbyte(temp, segbot(pseg));
    } else {
        temp = getbyte(segtop(tseg));
        grow(pseg);
        putbyte(temp, segtop(pseg));
    }
}

/*
 * Copy the core image of the current process to a new process.
 * Called from procopy (fork.c).
 */
bankcopy(p)
    struct proc *p;
{
    char i;

    for (i = 0; i < 17; i++)
        if (u.p->mem[i].per == FULL)
            segcopy(u.p->mem[i].seg, p->mem[i].seg);    /* mem.s */
}

/*
 * Switch to process n: give it a task of its own, and point the
 * kernel's u window at it.
 *
 * The map ram of a task already holds the process's map - it was copied
 * there when the map last changed (taskload) - so there is no map copy
 * here, only the u page.  That page is the one thing a switch must
 * remap: the kernel stack, this process's struct user and the trap
 * frame all live in it, and there is one of it per process.  Task 0 is
 * the only supervisor task and is what the kernel runs in, so this is
 * task 0's entry for the page whichever task the process returns to.
 */
newmap(n)
    struct proc *n;
{
    extern UINT8 map0[];

    di();
    if (n->task == 0) {         /* evicted, swapped in, or never had one */
        taskalloc(n);
        taskload(n);
    }
    /*
     * The register only: the image at 0x200 is the monitor's record of
     * task 0's map and nothing reads its user-page entry, so there is
     * nothing here to keep in step.
     */
    map0[2 * USERSEG] = n->mem[16].seg;
    u.p = n;                    /* fork() did not set this */
    u.task = n->task;           /* what gotask writes to the task register */
    curmem = (char *) n->mem;   /* where mem.s looks up a user page */
    ovlremap();                 /* and the driver this process is inside */
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
 * Initialize the swapmap.  Called from main(), before the swapper forks
 * (sys/main_init.c): the first fork can reach swapout against this map,
 * and a map with a size in it that nobody computed is a map that has the
 * first child killed.
 */
swapinit()
{
    struct dlgeom g;

    if (swapdev) {
        /*
         * Swap is a slice of its own: the minor names a drive and the
         * slice that holds the swap area.  Its geometry comes off the
         * drive's label, which is read through the drive's 'c' - block 0
         * of any other slice is that slice's first block, not the label.
         * The 'c' device is built here from the swap minor, because only
         * the driver knows which low bits are the drive and this is the
         * outside of it (each driver's open builds the same number).
         *
         * One cylinder is left at the front.  The first block of a swap
         * area must not be zero: salloc hands back the map's own address
         * and swapout reads a zero as "could not allocate", which kills
         * the process it was trying to save (sys/swap.c).
         *
         * The count is rounded down to whole 8-block groups.  salloc
         * asks for 8 blocks per segment (p->nsegs << 3), and a request
         * of more than one block is bounded only by its first block
         * (mwstrat), so a part group at the end would be swap space
         * whose last blocks are past the end of the drive.
         */
        bopen(swapdev, WRITE);
        zero(&g, sizeof g);
        if (u.error ||
            dllabel((swapdev & ~(7 << 5)) | (DL_WHOLE << 5),
                    devslice(swapdev), &g, 0, 0) != 1) {
            pr("swap: %s/%d: no usable label, no swap\n",
                devname, minor(swapdev));
            u.error = 0;
            swapdev = 0;
            swapsize = 0;
            swapaddr = 0;
        } else {
            swapaddr = g.spc;
            swapsize = (g.maxblk + 1 - swapaddr) & ~7;
            /*
             * What the map came out holding, which is the one number
             * that says whether this machine can swap at all and how
             * far.  Blocks, because that is what the map is kept in and
             * what a swap address means; kilobytes beside it because a
             * block count is not a size anyone reads at a glance.  A
             * half-kilobyte block, so the shift is the conversion.
             */
            pr("swap map: %d blocks (%dK) at %d on %s/%d\n", swapsize,
                swapsize >> 1, swapaddr, devname,
                minor(swapdev));
        }
    }
    swapmap[0].size = swapsize;
    swapmap[0].addr = swapaddr;
    swapmap[1].size = 0;
}

/*
 * Allocate size (!= 0) blocks of swap space.
 */
unsigned
salloc(size)
    register int size;
{
    register struct map *mp;
    register unsigned blk;

    for (mp = swapmap; mp->size != 0; mp++) {
        blk = mp->addr;
        if (mp->size == size) {
            lshift(mp);
            return (blk);
        }
        if (mp->size > size) {
            mp->size -= size;
            mp->addr += size;
            return (blk);
        }
    }
    pr("Out of swap space");
    return 0;
}

/*
 * Add a segment of swap space to the swap map
 */
sfree(size, blk)
    int size;
    unsigned blk;
{
    register struct map *mp;

    for (mp = swapmap; mp->addr < blk && mp->size != 0; mp++);
    rshift(mp);
    mp->size = size;
    mp->addr = blk;
    if (mp > swapmap)
        mp--;
    while (mp->addr + mp->size == (mp + 1)->addr) {
        mp->size += (mp + 1)->size;
        lshift(mp + 1);
    }
}

/*
 * Shift the map left, overwriting *mp
 */
lshift(mp)
    register struct map *mp;
{
    for (; mp->size != 0; mp++) {
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
    register struct map *np;

    for (np = mp; np->size != 0; np++)  /* find the end */
        ;
    for (np++; np != mp; np--) {
        np->size = (np - 1)->size;
        np->addr = (np - 1)->addr;
    }
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
