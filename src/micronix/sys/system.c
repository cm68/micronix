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

extern char resched;

/*
 * extern struct proc * hier; 
 */
extern int nulldev();

char trace = 0;                 /* debugging switch */

/*
 * System call branch table structure.
 */
struct syscall
{
    char nbytes;                /* no. of bytes in arguments */
    int (*call)();              /* address of system call */
};

#define BADCALL 47              /* index of a bad call entry in syssw */
#define SYSCALL 0317            /* 8080 rst 1 instruction */
#define INDIR	0               /* index of a indirect call in syssw */

/*
 * Everything the table below takes the address of.
 *
 * Written the way con.c writes the same thing for its device
 * switches: as int-returning functions, grouped by the file that
 * defines them, because that is the fact a reader wants.  None of
 * this was declared at all before; Whitesmith's took an unknown name
 * for an external int and an "&" of one for its address, which is why
 * a table of forty-eight function pointers compiled with no
 * declarations in sight.  ccc asks, so here they are.
 *
 * indir, unimp and badcall are in this file, below the table.
 */
extern int indir(), unimp(), badcall();

extern int brake(), chdir(), chmod(), chown(), sync();      /* sys1.c */
extern int pause(), stat();                                 /* sys2.c */
extern int permission();                                    /* access.c */
extern int mount(), umount();                               /* mount.c */
extern int link(), unlink();                                /* link.c */
extern int exec();                                          /* exec.c */
extern int mknod();                                         /* create.c */

extern int r_alarm(), r_close(), r_creat(), r_csw();        /* reg.c */
extern int r_dup(), r_exit(), r_fork(), r_fstat();
extern int r_getpid(), r_getuid(), r_gtty(), r_kill();
extern int r_nice(), r_open(), r_pipe(), r_read();
extern int r_seek(), r_setuid(), r_signal(), r_sleep();
extern int r_ssw(), r_stime(), r_stty(), r_time();
extern int r_wait(), r_write();

/*
 * These two are entries 49 and 50 and NOTHING DEFINES THEM.  reg.c
 * has both, commented out, at lines 173 and 175:
 *
 *      r_lock () { reclock (u.hl, arg [0]); }
 *      r_unlock () { unlock (u.hl); }
 *
 * and lock.c has the reclock() and unlock() they would call.  So the
 * kernel does not link as it stands, and has not for as long as those
 * two lines have been comments.  Declaring them here is what lets this
 * file compile; it does not make them exist, and the link will say so.
 */
extern int r_lock(), r_unlock();

       /*
        * System call branch table. Arguments from registers,
        * and returns to registers, are handled by r_ functions.
        */
/*
 * syssw and ncalls live in leaf/consts.c, parked in the u page; the
 * table is read-only so a per-process copy is harmless.
 */
extern struct syscall syssw[];
extern int ncalls;

/*
 * Entry point for system calls. A typical call:
 * "rst1; func; arg0; arg1". At the rst1 location:
 * "hlt".
 * See trap() for the stack setup.
 */
system()
{
    u.p->mode |= SYS;
    u.p->pri = PRISYS;
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
    int direct;
{
    register int call, nbytes;
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
    (*syssw[call].call) (arg[0], arg[1], arg[2]);
}

/*
 * Indirect system call
 */
indir(addr)
    char *addr;
{
    docall(addr, 0);
}

/*
 * Unimplemented call
 */
unimp()
{
    u.error = EINVAL;
}

/*
 * Bad call
 */
badcall()
{
    u.error = EINVAL;
    send(u.p, SIGSYS);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
