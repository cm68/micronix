/*
 * execute a binary
 *
 * sys/exec.c 
 * Changed: <2021-12-24 05:56:43 curt>
 */
#include <types.h>
#include <sys/sys.h>
#include <sys/fs.h>
#include <sys/stat.h>
#include <sys/inode.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/signal.h>
#include <obj.h>
#include <errno.h>

#define HALT	0x76            /* system call trap */

/*
 * The argument block is built directly into the new image's initial
 * stack, in segments of their own, while the old image is still mapped.
 * Nothing goes through the buffer cache.  The block is placed flush at
 * the top of user memory, so the stack it costs is exactly the block
 * and the image gets everything below it; the strings are measured
 * first so their final addresses are known before the copy.
 */

/*
 * Objext header for cpm-format programs.
 */
struct obj cpmhdr = {
    OBJECT,                     /* standard ident byte */
    /*
     * CONF_NORELO, not NORELOC.  The name written here is not defined
     * in obj.h or anywhere else in the tree - both copies of the
     * header have only CONF_NORELO - so this has never been the
     * constant it reads as.  0x80 in the conf byte is the no
     * relocation flag, which is what the comment says it wants.
     */
    CONF_NORELO,                /* no relocation bits */
    0, 0, 0, 0, 0,              /* table, text, data, bss, heap size */
    0x100,                      /* text offset */
    0,                          /* data offset */
};

static struct inode *ip = 0;
static UINT nargs = 0;
static UINT nenv = 0;
static UINT nbytes = 0;         /* the strings, nulls included */
static UINT argsize = 0;        /* strings + vectors + argc and terminators */

/*
 * The stack pages the block lives in, by page.  A zero entry is free;
 * arginstall() hands the nonzero ones to the process and clears them.
 */
static UINT8 argseg[16];
static UINT8 argfirst;          /* the lowest page of the block */
static UINT8 argheld;           /* the buffer window has a page of ours */
static UINT8 argpage;           /* the page in the buffer window, 0xff none */

/*
 * A fake buffer header: bhold() only reads xmem and data, and using it
 * here means brel() restores the window the same way it does for real
 * buffers.  The stack page is not a cache block.
 */
static struct buf argbuf;

extern int zerouser();
extern int out();
extern int getbyte(), getword();
extern int segalloc(), segfree(), newmap(), taskload();
extern UINT nsegs;              /* free 4K segments (malloc.c) */

static struct obj hdr = 0;
char zpage[512] = 0;   /* zero page for clearing bss */

/*
 * The worker for both entries: execve(name, args, env) and exec(name,
 * args), the old call, with an empty environment.  The stack carries
 *
 *	argc, argv[0..], 0, envp[0..], 0
 *
 * and crt0 points environ at envp.  A program exec'd through call 11
 * therefore starts with no variables; through call 56, with the ones
 * its caller handed over.
 */
doexec(name, args, env)
    char *name, **args, **env;
{
    static char execing = 0;

    while (execing)
        sleep(&execing, PRIWAIT);
    execing = 1;

    if (xchk(name) && argscan(args, env) && rdhdr() && fit() &&
        argalloc() && argbuild(args, env)) {
        /*
         * Tell the tracer the user task's id, then its program name,
         * before mrelse() drops the old image's pages (after which
         * `name` is no longer readable).  Port 0xd1 carries the task
         * byte followed by the name, NUL-terminated.  The device and
         * inode of the new image follow on 0xd0 after readin().
         */
        out(0xd1, u.task);                     /* user task id */
        {
            char *p;
            for (p = name; ; p++) {
                UINT8 c = getbyte(p);
                out(0xd1, c);
                if (c == 0)
                    break;
            }
        }
        mrelse();
        arginstall();
        setusr();
        readin();
        /*
         * Tell the simulator which task, device and inode now runs, so
         * its tracer can load the new image's symbol table.  Task byte,
         * device (major then minor), then the inode number, low byte
         * first.
         */
        out(0xd0, u.task);                     /* task byte */
        out(0xd0, ip->i_major);                /* device: major */
        out(0xd0, ip->i_minor);                /* device: minor */
        out(0xd0, (UINT8) ip->i_inum);         /* inode number, low */
        out(0xd0, (UINT8) (ip->i_inum >> 8));  /* inode number, high */
        if (u.error)
            send(u.p, SIGKILL);
    } else
        argfree();
    irelse(ip);
    execing = 0;
    wakeup(&execing);
}

/*
 * The two entries.  exec keeps its original two arguments and gets an
 * empty environment; execve adds the vector the shell builds.
 */
exec(name, args)
    char *name, **args;
{
    doexec(name, args, 0);
}

execve(name, args, env)
    char *name, **args, **env;
{
    doexec(name, args, env);
}

/*
 * Get the inode and check access.
 */
xchk(name)
    char *name;
{
    if ((ip = iname(name)) != 0) {
        if ((ip->i_mode & IFMT) != IFREG || ip->size == 0) {
            u.error = ENOEXEC;
            return 0;
        }
        if (access(ip, IEXEC))
            return 1;
    }
    return 0;
}

/*
 * Read the header
 */
rdhdr()
{
    u.offset = 0;
    u.segflg = KSEG;
    if (nread(ip, &hdr, sizeof(hdr)) < 0)
        return 0;
    if (hdr.ident != OBJECT) {  /* old cpm format */
        u.offset = 0;           /* rewind file */
        copy(&cpmhdr, &hdr, sizeof(hdr));
        hdr.text = ip->size;
        hdr.dataoff = hdr.text + hdr.textoff;
    }
    return 1;
}

/*
 * Check fit.  The argument block is placed at the top of user memory,
 * so the image and its heap, plus the block, must all fit.  The
 * header's sizes are the file's, so add them a step at a time: a sum
 * that wraps would look small and let an image run past the end of
 * user memory.
 */
fit()
{
    UINT brake, tbrake, sp;

    if (hdr.dataoff > MAXMEM - hdr.data) {
        u.error = ENOMEM;
        return 0;
    }
    brake = hdr.dataoff + hdr.data;
    if (hdr.bss > MAXMEM - brake) {
        u.error = ENOMEM;
        return 0;
    }
    brake += hdr.bss;

    if (hdr.textoff > MAXMEM - hdr.text) {
        u.error = ENOMEM;
        return 0;
    }
    tbrake = hdr.textoff + hdr.text;
    if (tbrake > brake)
        brake = tbrake;

    /*
     * The block uses the top byte as well as the rest of user memory,
     * so its base is one past 0xffff - argsize.  The image and heap
     * must end at or below it.
     */
    sp = 0xFFFF - argsize + 1;
    if (brake > sp || hdr.heap > sp - brake) {
        u.error = ENOMEM;
        return 0;
    }
    u.brake = (char *) brake;
    return 1;
}

/*
 * The length of a user string, bounded.  getbyte reads the caller's
 * address space through the 0xc000 window; the bound keeps an
 * unterminated string from walking the machine.
 */
static UINT
ulen(s)
    UINT s;
{
    UINT n;

    for (n = 0; n < MAXMEM; n++)
        if (getbyte(s + n) == 0)
            break;
    return n;
}

/*
 * Count the vectors and measure the strings.
 *
 * The block is placed flush at the top of user memory, so the total
 * has to be known before the copy.  This pass reads the vectors and
 * walks the strings for their lengths; argbuild() then copies them
 * straight into the new segments.  The extra read is deliberate: it
 * makes the stack cost exactly the arguments and nothing more.
 *
 * argsize is argc (2) + the two null terminators (4) + a pointer for
 * every string + the strings themselves.
 */
argscan(args, env)
    char **args, **env;
{
    UINT i, s, len, need;

    nargs = nenv = nbytes = 0;
    argsize = 6;                /* argc, and the argv/envp terminators */

    if (args) {
        for (i = 0; (s = getword(args + i)) != 0; i++) {
            len = ulen(s);
            if (len > MAXMEM - 3) {
                u.error = E2BIG;
                return 0;
            }
            need = len + 3;             /* string, null, pointer */
            if (argsize > MAXMEM - need) {
                u.error = E2BIG;
                return 0;
            }
            argsize += need;
            nbytes += len + 1;
            nargs++;
        }
    }
    if (env) {
        for (i = 0; (s = getword(env + i)) != 0; i++) {
            len = ulen(s);
            if (len > MAXMEM - 3) {
                u.error = E2BIG;
                return 0;
            }
            need = len + 3;
            if (argsize > MAXMEM - need) {
                u.error = E2BIG;
                return 0;
            }
            argsize += need;
            nbytes += len + 1;
            nenv++;
        }
    }

    /*
     * Keep page 0 out of the stack: a block that reaches below 0x1000
     * would overwrite the grow segment mrelse() leaves in mem[0].
     */
    if (argsize > 0xEFFF) {
        u.error = E2BIG;
        return 0;
    }
    return 1;
}

/*
 * Map a new stack page in the buffer window.  bhold() on a fake header
 * stacks the segment it replaces and brel() puts it back, so the window
 * is the kernel's again after the build.  getbyte() reads the caller
 * through the other window (0xc000), which is what lets the old image
 * and the new stack be touched at the same time.
 */
static char *
argwin(page)
    UINT page;
{
    argbuf.xmem = argseg[page];
    argbuf.data = (char *) BUFWIN;
    argheld = 1;
    return (bhold(&argbuf));
}

static void
argrel()
{
    if (argheld) {
        brel();
        argheld = 0;
    }
    argpage = 0xff;
}

static void
argmap(page)
    UINT page;
{
    if (argheld && page == argpage)
        return;
    argrel();
    argwin(page);
    argpage = page;
}

static void
argput(addr, c)
    UINT addr; UINT8 c;
{
    argmap(addr >> 12);
    ((UINT8 *) BUFWIN)[addr & 0xFFF] = c;
}

static UINT8
argget(addr)
    UINT addr;
{
    argmap(addr >> 12);
    return (((UINT8 *) BUFWIN)[addr & 0xFFF]);
}

static void
argputw(addr, v)
    UINT addr, v;
{
    argput(addr, (UINT8) v);
    argput(addr + 1, (UINT8) (v >> 8));
}

/*
 * Copy one user string to the stack page.  *room is what is left of the
 * page; running out before the null means the list does not fit, which
 * is E2BIG rather than a write off the end.
 */
static int
argcopy(s, dp, roomp)
    UINT s; UINT *dp, *roomp;
{
    UINT8 c;
    UINT d, room;

    d = *dp;
    room = *roomp;
    while (room) {
        c = getbyte(s++);
        argput(d, c);
        d++;
        room--;
        if (c == 0) {
            *dp = d;
            *roomp = room;
            return 1;
        }
    }
    return 0;
}

/*
 * Reserve the segments for the block's pages.  This happens before
 * mrelse(), while the old image still owns its pages, so it can fail:
 * the old path allocated the stack only after the old pages were free.
 * Check the free count first and answer ENOMEM rather than panicking in
 * segalloc().
 */
argalloc()
{
    UINT p, n;

    argfirst = (0xFFFF - argsize + 1) >> 12;
    n = 16 - argfirst;
    if (nsegs < n) {
        u.error = ENOMEM;
        return 0;
    }
    for (p = 0; p < 16; p++)
        argseg[p] = 0;
    for (p = argfirst; p < 16; p++)
        argseg[p] = segalloc();
    return 1;
}

/*
 * Build the block flush at the top of user memory: the strings end at
 * 0xffff and the vectors sit below them, so the stack costs exactly
 * argsize and everything below it is left to the image.  The strings
 * are copied once here; argscan() has already measured them.
 */
argbuild(args, env)
    char **args, **env;
{
    UINT i, s, d, room, sp, av, a;

    sp = 0xFFFF - argsize + 1;
    av = sp + 2;
    a = av + 2 * (nargs + nenv + 2);   /* the string area; 0 if no strings */
    u.sp = (UINT *) sp;

    argheld = 0;
    argpage = 0xff;
    room = nbytes;

    argputw(sp, nargs);
    d = a;
    for (i = 0; i < nargs; i++) {
        s = getword(args + i);
        argputw(av + 2 * i, d);
        if (!argcopy(s, &d, &room))
            goto big;
    }
    argputw(av + 2 * nargs, 0xFFFF);        /* argv terminator, -1 (exec(2)) */
    for (i = 0; i < nenv; i++) {
        s = getword(env + i);
        argputw(av + 2 * (nargs + 1 + i), d);
        if (!argcopy(s, &d, &room))
            goto big;
    }
    argputw(av + 2 * (nargs + 1 + nenv), 0);    /* envp terminator */

    /*
     * The first bytes of argv[0], for ps, stopping at the null.  The
     * table field is fourteen so /bin/rshd survives; ps prints it with
     * the same width.
     */
    zero(u.p->args, 14);
    i = nbytes < 14 ? nbytes : 14;
    for (d = 0; d < i; d++) {
        ((UINT8 *) u.p->args)[d] = argget(a + d);
        if (((UINT8 *) u.p->args)[d] == 0)
            break;
    }

    argrel();
    return 1;

big:
    argrel();
    u.error = E2BIG;
    return 0;
}

/*
 * Hand the block's pages to the process after mrelse() has cleared the
 * old image.  mrelse() keeps page 15 - the old stack - so free it here,
 * now that nothing reads it, and install the pages the block uses.
 */
arginstall()
{
    UINT p, n;
    UINT8 old, grow;

    old = u.p->mem[15].seg;
    grow = u.p->mem[0].seg;
    n = 0;
    for (p = argfirst; p < 16; p++) {
        u.p->mem[p].seg = argseg[p];
        u.p->mem[p].per = FULL;
        argseg[p] = 0;              /* the process owns it now */
        n++;
    }
    if (old != grow)
        segfree(old);
    u.p->nsegs = 2 + n;             /* grow, the block's pages, and u */
    newmap(u.p);
    taskload(u.p);
}

/*
 * Release whatever a failed exec reserved.  On success the entries were
 * cleared by arginstall().
 */
argfree()
{
    UINT p;

    argrel();
    for (p = 0; p < 16; p++) {
        if (argseg[p]) {
            segfree(argseg[p]);
            argseg[p] = 0;
        }
    }
    argfirst = 0;
}

/*
 * Set up user's pc, id's, and signals.
 */
setusr()
{
    static int *sp, *st;

    u.pc = hdr.textoff;
    u.u_euid = (ip->i_mode & ISUID) ? (ip->i_uid) : (u.u_uid);
    u.u_egid = (ip->i_mode & ISGID) ? (ip->i_gid) : (u.u_gid);
    for (sp = u.p->slist, st = sp + NSIG; sp < st; sp++)
        if (*sp > 1)
            *sp = 0;
}

/*
 * Read in the text and data segments
 */
readin()
{
    extern char rst1[];

    u.segflg = USEG;
    u.p->mode |= LOCKED;
    valid(rst1, 1);
    putbyte(HALT, rst1);
    valid(hdr.textoff, hdr.text);
    nread(ip, hdr.textoff, hdr.text);
    valid(hdr.dataoff, hdr.data);
    nread(ip, hdr.dataoff, hdr.data);

    /*
     * Zero the bss.  The Whitesmith's compiler never emitted a bss
     * (it put everything in data), so no code has ever done this;
     * ccc does, and a segment handed back by segalloc() still holds
     * whatever its previous owner left.  zerouser() ldir's a shared
     * zero page into the region.
     */
    valid(hdr.dataoff + hdr.data, hdr.bss);
    zerouser((char *) (hdr.dataoff + hdr.data), hdr.bss);

    u.p->mode &= ~LOCKED;       /* no core lock across exec */
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
