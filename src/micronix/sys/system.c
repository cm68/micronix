/*
 * System call table
 *
 * sys/system.c 
 * Changed: <2022-01-04 10:53:40 curt>
 */
#include <types.h>
#include <sys/sys.h>
#include <sys/proc.h>
#include <sys/signal.h>
#include <errno.h>
extern void brake();
extern void chdir();
extern void chmod();
extern void chown();
extern void sync();
extern void dup();
extern void dup2();
extern void getuid();
extern void nice();
extern void setuid();
extern void alarm();
extern void getpid();
extern void pause();
extern void stat();
extern void stime();
extern void fstat();
extern void snooze();
extern void pr();
extern void send();

extern char resched;

/*
 * extern struct proc * hier; 
 */
extern void nulldev();

char trace = 0;                 /* debugging switch */

/*
 * System call branch table structure.
 */
struct syscall
{
    char nbytes;                /* no. of bytes in arguments */
    void (*call)();             /* address of system call */
};

#define BADCALL 47              /* index of a bad call entry in syssw */
#define SYSCALL 0317            /* 8080 rst 1 instruction */
#define INDIR	0               /* index of a indirect call in syssw */

/*
 * Everything the table below takes the address of.
 *
 * Written the way con.c writes the same thing for its device
 * switches: as functions returning nothing, grouped by the file that
 * defines them, because that is the fact a reader wants.  None of
 * this was declared at all before; Whitesmith's took an unknown name
 * for an external int and an "&" of one for its address, which is why
 * a table of forty-eight function pointers compiled with no
 * declarations in sight.  ccc asks, so here they are.
 *
 * indir, unimp and badcall are in this file, below the table.
 */
extern void indir(), unimp(), badcall();

extern void brake(), chdir(), chmod(), chown(), sync(), dup(), dup2(), getuid(), setuid(), nice();            /* sys1.c */
extern void pause(), stat(), alarm(), getpid(), stime(), fstat(), snooze();                                      /* sys2.c */
extern void permission();                                    /* access.c */
extern void mount(), umount();                               /* mount.c */
extern void link(), unlink();                                /* link.c */
extern void exec();                                          /* exec.c */
extern void mknod();
extern void creat();                               /* create.c */
extern void open();                                          /* open.c */
extern void read(), write();                                 /* rdwri.c */
extern void exit(), wait();                                  /* fork.c */
extern void kill(), signal();                                /* sig.c */
extern void s_stty(), s_gtty();                              /* cio.c */

extern void r_close(), r_fork(), r_ioctl();         /* reg.c */
extern void r_pipe(), r_seek(), r_ftruncate(), r_time();

/*
 * These two are entries 49 and 50 and NOTHING DEFINES THEM.  reg.c
 * has both, commented out, at lines 173 and 175:
 *
 *      r_lock () { reclock (u.hl, arg [0]); }
 *      r_unlock () { unlock (u.hl); }
 *
 * and the reclock() and unlock() they would call are not in the tree
 * either: they lived in lock.c, which has been removed (see git
 * history).  So the kernel does not link as it stands, and has not for
 * as long as those two lines have been comments.  Declaring them here
 * is what lets this file compile; it does not make them exist, and the
 * link will say so.
 */
extern int r_lock(), r_unlock();

       /*
        * System call branch table. Arguments from registers,
        * and returns to registers, are handled by r_ functions.
        */
/*
 * syssw and ncalls live in consts.c, parked in the u page; the
 * table is read-only so a per-process copy is harmless.
 */
extern struct syscall syssw[];
extern int ncalls;
extern unsigned char valid();             /* sys/malloc.c: make the page grown */

/*
 * Entry point for system calls. A typical call:
 * "rst1; func; arg0; arg1". At the rst1 location:
 * "hlt".
 * See trap() for the stack setup.
 */
void
system()
{
    u.p->mode |= SYS;
    u.p->pri = PRISYS;
    /*
     * The rst1 trap pushes its return address on the user stack, and
     * that push may be the first touch of a page that is still GROW.
     * The hardware writes it without a fault, so nothing has marked
     * the page FULL - and fork() copies only FULL pages, while the
     * kernel reads this address through the window.  A stack that
     * crossed the boundary on the trap then reads back as garbage,
     * docall() sees a call number that is not one, and a good syscall
     * comes back as SIGSYS with EINVAL.  Grow the page before reading
     * it: the word is already in the segment, so this only promotes
     * it to FULL, which is what the copy and the window want.
     */
    valid(u.sp, 2);
    u.pc = getword(u.sp++);     /* return from rst1 */
    enable();
    u.segflg = USEG;
    u.error = 0;
    docall(u.pc, 1);          /* note: fork may change u */
    u.p->pri = PRIUSER;

    /*
     * if (hier && priority(hier) > priority(u.p)) resched = 1; 
     */
    u.p->mode &= ~SYS;
}

/*
 * Arguments for use by register-passing functions
 * in reg.c
 */
int arg[4] = 0;

/*
 * Get the arguments and do the call. On a direct call,
 * update the pc and save the frame pointer so that a
 * signal can abort from this level.
 */
docall(addr, direct)
    register char *addr;
    UINT8 direct;
{
    register UINT8 call, nbytes;
    int dummy;

    if (direct)
        call = getbyte(addr++);
    else if (getbyte(addr++) != SYSCALL)
        call = BADCALL;
    else if ((call = getbyte(addr++)) == INDIR)
        return;                 /* an double indirect is a no op */
    if (call >= ncalls)
        call = BADCALL;
    nbytes = syssw[call].nbytes;
    copyin(addr, arg, nbytes);
    if (direct) {
        u.pc += nbytes + 1;
        saveframe(&u.abort, &dummy);
    }
    if (trace && call)
        pr(" %d:%d ", procid(u.p), call);
    /*
     * The table's pointers are untyped, so every entry is called with
     * word arguments.  A syscall must therefore not declare an 8-bit
     * first parameter: c1 marks such a definition SF_ARG8 and passes it
     * in A, and nothing on this path puts it there.  dup2 did, and every
     * dup2 got a register's worth of garbage - the login shell's Ldup
     * onto fd 0 failed and it read stdin instead of .login.  Declare the
     * parameter UINT and narrow it inside the call.
     */
    (*syssw[call].call) (arg[0], arg[1], arg[2]);
}

/*
 * Indirect system call
 */
void
indir(addr)
    char *addr;
{
    docall(addr, 0);
}

/*
 * Unimplemented call
 */
void
unimp()
{
    u.error = EINVAL;
}

/*
 * Bad call
 */
void
badcall()
{
    u.error = EINVAL;
    send(u.p, SIGSYS);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
