/*
 * exec.c 
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
    NORELOC,                    /* no relocation bits */
    0, 0, 0, 0, 0,              /* table, text, data, bss, heap size */
    0x100,                      /* text offset */
    0,                          /* data offset */
};

static struct inode *ip = 0;
static struct buf *bp[NBLKS] = 0;
static UINT nargs = 0;
static UINT nbytes = 0;
static UINT8 nblks = 0;
static struct obj hdr = 0;

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
        mrelse();
        putargs();
        setusr();
        readin();
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

    nargs = nbytes = nblks = 0;
    while ((s = getword(args++)) != 0) {
        nargs++;
        do {
            if ((nbytes & 511) == 0) {
                if (nblks < NBLKS) {
                    nblks++;
                    bp[nblks] = bget(nblks, NODEV);
                    d = bp[nblks]->data;
                } else {
                    u.error = E2BIG;
                    return 0;
                }
            }
            nbytes++;
        }
        while ((*d++ = getbyte(s++)) != '\0');
    }
    return 1;
}

/*
 * Set up user's stack.
 */
putargs()
{
    static UINT8 n, c, *s, *a, *d, *t, **av;
    static UINT ac, count;
    extern char usrtop;

    a = d = &usrtop - nbytes;
    av = (char **) a - nargs - 1;
    u.sp = &av[-1];
    valid(u.sp, 4 + nargs + nargs + nbytes);

    for (c = 0, ac = 0, n = 1; n <= nblks; n++) {
        s = bp[n]->data;
        t = s + 512;

        do {
            if (c == 0) {
                putword(a, &av[ac]);
                if (++ac >= nargs)
                    break;
            }
            c = *s;
            s++;
            a++;
        }
        while (s < t);

        count = min(512, nbytes);
        copyout(bp[n]->data, d, count);
        nbytes -= count;
        d += count;
    }

    copy(bp[1]->data, u.p->args, 8);    /* for ps */
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
