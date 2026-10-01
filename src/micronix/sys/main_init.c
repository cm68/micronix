/*
 * the kernel startup
 *
 * This is init-only code: it runs once from main() in task 0 and is
 * then dead.  It is linked high - into the region between _ebss and
 * the 0xd000 scratch window - and main_data.c's expand_bufs() mints
 * buffer headers over it once the init-only functions have returned.
 * The resident data it touches (plist, ilist, flist, mlist, nbuf,
 * btop, rootdir, map0, image0) lives in main_data.c.
 *
 * sys/main_init.c
 * Changed: <2026-09-26 curt>
 */
#include <types.h>
#include <sys/fs.h>
#include <sys/sys.h>
#include <sys/stat.h>
#include <sys/inode.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/buf.h>
#include <sys/con.h>
#include <sys/signal.h>
#include <obj.h>
#include <sys/ovl.h>

#include "build.h"

/*
 * resident data, defined in main_data.c (and malloc.c / con.c)
 */
extern struct proc *swapproc;
extern struct inode *rootdir;
extern UINT nbuf;
extern UINT totmem;
extern struct buf *btop;
extern UINT8 map0[], image0[];
extern UINT8 segmap[];          /* malloc.c */
extern UINT nsegs;
extern int segalloc();          /* malloc.c */
extern int swapinit();          /* malloc.c */
extern int nodev();             /* con.c */
extern UINT kino;               /* uhdr.s's _kino: the inode we booted from */

/*
 * A module page.  setdev.c has its own copy of these two numbers for the
 * same file it reads here; the loader has to agree with it.
 */
#define PAGESHIFT 12
#define PAGESZ	(1 << PAGESHIFT)

/*
 * System initialization
 */
main()
{
    char ronly = 0;

    cus();                      /* custom hardware */
    enable();                   /* enable interrupts */
    coninit();
    plogo();
    pinit();                    /* plist, see below */
    meminit();                  /* memory */
    binit();                    /* buffers (segalloc's the boot page) */
    pcon();                     /* below */
    bopen(rootdev, WRITE);
    if (u.error) {
        u.error = 0;
        bopen(rootdev, READ);
        if (u.error)
            panic("Can't read root device");
        ronly = 1;
        pr("\nRoot device is read-only\n\n");
    }
    tmount(rootdev, 0, ronly);
    if ((rootdir = iget(1, rootdev)) == 0)
        panic("Can't get root dir");
    u.cdir = rootdir;
    rootdir->count = 2;
    irelse(rootdir);
    u.p->tty = 0;               /* not tied to any tty */
    ovlplaceall();              /* the driver modules riding in our own file */

    /*
     * The swap map, last, and here rather than in the swapper.
     *
     * sys/trap.c forks the login process and the parent becomes the
     * swapper, and fork() swaps a child out when there is a swap device
     * to put it on (sys/fork.c).  So the swapper's own loop is not the
     * first thing that can reach the map - the fork that makes the
     * swapper can - and a map still all zeros would fail the first
     * allocation and kill the child it was meant to make room for.
     *
     * Here rather than earlier because it opens the swap device, which
     * reads its label, and that is a bread: it wants the buffer cache,
     * the drivers and the overlay area, and after ovlplaceall() all
     * three are up.  It is also before expand_bufs() mints the rest of
     * the pool, which is what makes the label read a block already in
     * the cache rather than a seek (sys/malloc.c, swapinit).
     */
    swapinit();
}

/*
 * Copy one 512-byte block into the module page under construction.
 *
 * The destination is the overlay window itself, and pointedly not the
 * scratch window: this is init-only code, folded to data and parked at
 * _ebss (sys/GNUmakefile), and the park runs from 0xcbc9 up past 0xdfff -
 * so the page the scratch window shows is the page this function is
 * executing from, and holding it pulls the code out from under the
 * instruction pointer.  0x9000 is below the park and is where the page is
 * going anyway, so the page is built where it will run.  Nothing else
 * wants the frame for the moment the copy is on: the read that fills it
 * has already happened, and reading is what maps the frame back.
 *
 * image0 and not map0 all the way through, as newmap() writes it: the map
 * registers are write-only.
 */
static
putblk(bp, seg, off)
    struct buf *bp;
    UINT seg, off;
{
    UINT8 save;

    save = image0[2 * OVLSEG];
    map0[2 * OVLSEG] = image0[2 * OVLSEG] = (UINT8) seg;
    copy(bhold(bp), (char *) OVLBASE + off, 512);
    brel();
    map0[2 * OVLSEG] = image0[2 * OVLSEG] = save;
}

/*
 * Place the driver modules that ride in the kernel's own file.
 *
 * The build appends one 4K page per driver past the object's own extent,
 * and setdev stamps the root driver's page into the slot the kernel is
 * already running with.  The rest are reachable only by reading the file
 * back, and that is what this does: the loader left the inode it booted
 * this kernel from in kino, the object header at the front of that file
 * gives the extent the build padded to - the same arithmetic setdev's
 * modbase() does, and it must include the symbol table, which an
 * unstripped kernel carries - and every whole page past it is a module.
 * A page's first bytes are the major it serves, so the order they were
 * appended in does not matter and there is no table of them here.
 *
 * This is init-only code, so it costs no resident space, and it runs
 * before the swapper forks (trap.c), so nothing else is in flight while
 * the frame is spoken for.  The page is built through the frame itself
 * rather than through the scratch window (putblk below).
 */
ovlplaceall()
{
    struct obj hdr;
    struct inode *ip;
    long modbase;
    UINT nmod, n, i, seg, maj;
    int ra, bad;
    struct buf *bp;
    char *s;

    if (kino == 0)              /* the loader did not say: nothing to read */
        return;
    if ((ip = iget(kino, rootdev)) == 0)
        return;

    u.offset = 0;
    u.segflg = KSEG;
    if (nread(ip, (char *) &hdr, sizeof(hdr)) < 0) {
        irelse(ip);
        return;
    }
    /*
     * Round the extent up to a page with shifts, not division: a long
     * divide would pull a member out of libc.a, and libc's objects are
     * plain ones the linker parks below the slot (cmd/ld/ld.c's
     * pass1_layout), where the text of an earlier object has already
     * been written.  The shifts are qshl/qshr, which are in the u page
     * and linked either way.
     */
    modbase = (long) sizeof(hdr) + hdr.table + hdr.text + hdr.data;
    modbase = ((modbase + PAGESZ - 1) >> PAGESHIFT) << PAGESHIFT;
    if (modbase >= (long) ip->i_size) {     /* nothing was appended */
        irelse(ip);
        return;
    }
    nmod = (UINT) ((ip->i_size - modbase) >> PAGESHIFT);

    for (n = 0; n < nmod; n++) {
        /*
         * The page's first block is the module's header, and the major in
         * it settles whether the page is worth placing.  The driver the
         * kernel was booted from is already running out of the slot
         * setdev stamped into, so its appended page is a second copy of a
         * driver that is already here - and placing it again is not the
         * harmless thing it looks like.  A second placement puts a second
         * handler on the driver's interrupt line, so every interrupt its
         * card raises is served twice and the second pass runs a chip the
         * first has already drained; it arms a second tick over the same
         * driver; and it moves the driver to a segment of its own while
         * the copy the kernel is running from stays where it was.
         *
         * So the header is read before the segment is spent, and a page
         * whose major is placed is skipped.  ovlmap answers for a driver
         * the kernel still holds as well as one in a segment, which is
         * what makes this the same test for either; its mapping of the
         * frame is the answer to a question about a driver already
         * mapped, and the next putblk saves and restores around it.
         */
        bp = bread(imap(ip, (int) (modbase >> 9) + n * 8, &ra), rootdev);
        if (bp == 0) {
            pr("ovl: page %d will not read\n", n);
            continue;
        }
        s = bhold(bp);
        maj = ((struct ovlhdr *) s)->major;
        brel();
        if (ovlmap(maj) != 0) {
            brelse(bp);
            continue;
        }
        seg = segalloc();
        putblk(bp, seg, 0);
        brelse(bp);
        bad = 0;
        for (i = 1; i < PAGESZ / 512; i++) {
            bp = bread(imap(ip, (int) (modbase >> 9) + n * 8 + i, &ra),
                       rootdev);
            if (bp == 0) {      /* a short page is not a page: place none of it */
                bad = 1;
                continue;
            }
            putblk(bp, seg, i * 512);
            brelse(bp);
        }
        if (bad) {
            pr("ovl: page %d is short\n", n);
            continue;
        }
        /*
         * ovlattach maps the page into the frame before it reads the
         * header, so afterwards OVLBASE is this module's header whether
         * the driver's init accepted it or refused.
         */
        ra = ovlattach(seg);
        if (ra)
            pr("ovl: %d not placed\n", maj);
    }
    irelse(ip);
}

/*
 * Initialize the first proc structure
 */
pinit()
{
    UINT8 i;

    u.p = swapproc;
    u.p->mode = ALLOC | ALIVE | AWAKE | LOADED | LOCKED | SYS;
    u.p->pri = PRISWAP;
    u.p->nice = 128;
    u.p->pid = 0;
    u.p->slist[SIGTINT] = 1;    /* ignore record-available signal */
    u.p->slist[SIGBACK] = 1;    /* ignore backgrounding signal */
    copy("swap", u.p->args, 5);

    for (i = 0; i < 17; i++) {  /* memory map */
        u.p->mem[i].seg = i;
        u.p->mem[i].per = FULL;
    }

    u.p->mem[16].seg = USERSEG; /* other procs will use seperate seg */
    u.p->nsegs = 17;            /* actually 16; used by mget() */
}

/*
 * Print the logo
 */
plogo()
{
    pr("\nMicronix 2.0\n");
#ifdef BUILD_DATE
    pr(BUILD_DATE);
#else
    pr("Created 11/9/83\n");
#endif
    pr("Copyright 1983 Gary Fitts\n\n");
}

/*
 * Check that there is enough memory to run.
 */
pcon()
{
    if (totmem < 128 || (totmem < 256 && swapdev == 0))
        panic("Not enough memory to run");
}

#define ADDR	(*(char*)0x1000)

/*
 * Find out how much memory is present.
 *
 * Memory runs up from segment 16 with no gap in it, so the first page
 * that is not there is where the machine ends and there is nothing above
 * it to ask about.  The probe stops on that page and the rest of the
 * window is marked used, not left free: segalloc() reads segmap[n] as
 * "in use" (the map is reused for allocation once this is done), and a
 * segment past the end of memory left looking free is one it can hand
 * out - which aliases real memory onto open bus, 0xff reads, and a
 * system that behaves as though the block it read was the block it
 * asked for.
 *
 * Set nsegs to the number of 4K segments, and set segmap[n] to 1 if the
 * nth segment is missing.
 */
meminit()
{
    int n;

    di();
    nsegs = MAXSEG - 16;
    for (n = 16; n < MAXSEG; n++) {
        map0[2] = n;
        ADDR = 0;
        if (ADDR != 0 || --ADDR != -1)
            break;
    }
    for (; n < MAXSEG; n++) {
        segmap[n] = 1;
        nsegs--;
    }
    map0[2] = image0[2];
    ei();

    totmem = (nsegs << 2) + 64;     /* full memory, saved for signon() */
}

/*
 * Initialize the blist. Called once from main().
 */
binit()
{
    struct buf *b;
    int seg;

    /*
     * Seed the 8 boot buffers from one 4K segment.  Buffer data now lives
     * in the unmapped physical segments, reached through the BUFSEG window:
     * block i has data = BUFWIN + (i&7)*512 and xmem = its segment.  blist
     * holds exactly the 8 boot headers (the last .bss object, see
     * textpad.s); expand_bufs() mints the rest contiguously off &blist[8]
     * once the init-only functions have returned.
     */
    seg = segalloc();
    nbuf = 8;
    btop = blist + nbuf;
    for (b = blist; b < btop; b++) {
        zero(b, sizeof(*b));
        b->data = (char *)BUFWIN + ((b - blist) & 7) * 512;
        b->xmem = seg;
    }
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
