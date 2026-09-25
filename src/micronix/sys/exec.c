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

#define NBLKS	4               /* max argument blocks */
#define NODEV	-1              /* see con.c */
#define HALT	0x76            /* system call trap */

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
static struct buf *bp[NBLKS] = 0;
static UINT nargs = 0;
static UINT nbytes = 0;
static UINT8 nblks = 0;
extern int zerouser();
extern int out();

static struct obj hdr = 0;
char zpage[512] = 0;   /* zero page for clearing bss */

/*
 * Exec system call.
 */
exec(name, args)
    char *name, **args;
{
    static char execing = 0;

    while (execing)
        sleep(&exec, PRIWAIT);
    execing = 1;

    if (xchk(name) && getargs(args) && rdhdr() && fit()) {
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
        putargs();
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
    }
    irelse(ip);
    arelse();
    execing = 0;
    wakeup(&exec);
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
 * Check fit
 */
fit()
{
    static char *brake, *tbrake;

    brake = hdr.dataoff + hdr.data + hdr.bss;
    tbrake = hdr.textoff + hdr.text;
    brake = max(brake, tbrake);

    if (brake + hdr.heap + 4 + nargs + nargs + nbytes > MAXMEM) {
        u.error = ENOMEM;
        return 0;
    }
    u.brake = brake;
    return 1;
}

/*
 * Copy arguments to buffers
 */
getargs(args)
    char **args;
{
    static char *s, *d;
    int held;

    held = 0;
    nargs = nbytes = nblks = 0;
    while ((s = getword(args++)) != 0) {
        nargs++;
        do {
            if ((nbytes & 511) == 0) {
                if (nblks < NBLKS) {
                    if (held)           /* the last block is full */
                        brel();
                    nblks++;
                    bp[nblks] = bget(nblks, NODEV);
                    d = bhold(bp[nblks]);
                    held = 1;
                } else {
                    if (held)
                        brel();
                    u.error = E2BIG;
                    return 0;
                }
            }
            nbytes++;
        }
        while ((*d++ = getbyte(s++)) != '\0');
    }
    if (held)
        brel();
    return 1;
}

/*
 * Set up user's stack.
 */
putargs()
{
    static UINT8 n, c, *s, *a, *d, **av;
    static UINT ac, count, rem;
    extern char usrtop;

    /*
     * s, a, d, t and av are UINT8; usrtop is a plain char, bp->data is
     * a char *, and u.sp is a UINT *.  The bytes are the same bytes -
     * this is building the argument block at the top of user memory
     * and the types are only saying how each line means to read it -
     * but ccc will not convert between them without being told, so
     * each crossing is spelled out.
     */
    a = d = (UINT8 *) (&usrtop - nbytes);
    av = (UINT8 **) ((char **) a - nargs - 1);
    u.sp = (UINT *) &av[-1];
    valid(u.sp, 4 + nargs + nargs + nbytes);

    for (c = 0, ac = 0, n = 1; n <= nblks; n++) {
        s = (UINT8 *) bhold(bp[n]);
        rem = 512;

        while (rem) {
            if (c == 0) {
                putword(a, &av[ac]);
                if (++ac >= nargs)
                    break;
            }
            c = *s;
            s++;
            a++;
            rem--;
        }

        count = min(512, nbytes);
        copyout(bp[n]->data, d, count);
        nbytes -= count;
        d += count;
        brel();
    }

    s = (UINT8 *) bhold(bp[1]);
    copy(s, u.p->args, 8);      /* for ps */
    brel();
    putword(-1, &av[nargs]);    /* as per unix specs */
    putword(nargs, u.sp);
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
 * Release the argument buffers
 */
arelse()
{
    static int n;

    for (n = 1; n <= nblks; n++)
        brelse(bp[n]);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
