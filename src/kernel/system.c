/*
 * system.c 
 */
#include "sys.h"
#include "proc.h"

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
        * System call branch table. Arguments from registers,
        * and returns to registers, are handled by r_ functions.
        */
struct syscall syssw[] = {
    2, &indir,                  /* 0 */
    0, &r_exit,                 /* 1 */
    0, &r_fork,                 /* 2 */
    4, &r_read,                 /* 3 */
    4, &r_write,                /* 4 */
    4, &r_open,                 /* 5 */
    0, &r_close,                /* 6 */
    0, &r_wait,                 /* 7 */
    4, &r_creat,                /* 8 */
    4, &link,                   /* 9 */
    2, &unlink,                 /* 10 */
    4, &exec,                   /* 11 */
    2, &chdir,                  /* 12 */
    0, &r_time                  /* 13 */
        6, &mknod,              /* 14 */
    4, &chmod,                  /* 15 */
    4, &chown,                  /* 16 */
    2, &brake,                  /* 17 */
    4, &stat,                   /* 18 */
    4, &r_seek,                 /* 19 */
    0, &r_getpid,               /* 20 */
    6, &mount,                  /* 21 */
    2, &umount,                 /* 22 */
    0, &r_setuid,               /* 23 */
    0, &r_getuid,               /* 24 */
    0, &r_stime,                /* 25 */
    6, &unimp,                  /* 25 &ptrace */
    0, &r_alarm,                /* 27 */
    2, &r_fstat,                /* 28 */
    0, &pause,                  /* 29 */
    0, &badcall,                /* */
    2, &r_stty,                 /* 31 */
    2, &r_gtty,                 /* 32 */
    4, &permission,             /* 33 */
    0, &r_nice,                 /* 34 */
    0, &r_sleep,                /* 35 */
    0, &sync,                   /* 36 */
    2, &r_kill,                 /* 37 */
    0, &r_csw,                  /* 38 */
    0, &r_ssw,                  /* 39 */
    0, &badcall,                /* */
    0, &r_dup,                  /* 41 */
    0, &r_pipe,                 /* 42 */
    2, &unimp,                  /* 43 &times */
    8, &unimp,                  /* 44 &profil */
    0, &badcall,                /* */
    0, &badcall,                /* */
    0, &badcall,                /* */
    4, &r_signal,               /* 48 */

#ifdef notdef
    2, &r_lock,                 /* 49 */
    0, &r_unlock,               /* 50 */
#endif
};

int ncalls = sizeof(syssw) / sizeof(struct syscall);

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
    docall(u.pc, YES);          /* note: fork may change u */
    u.p->pri = PRIUSER;

    /*
     * if (hier && priority(hier) > priority(u.p)) resched = YES; 
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
    fast char *addr;
    int direct;
{
    fast int call, nbytes;
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
    docall(addr, NO);
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
    send(u.p, SIGBAD);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
