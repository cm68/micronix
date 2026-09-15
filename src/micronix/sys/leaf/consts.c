/*
 * Read-only kernel tables, parked in the u page (see ../user.c).
 *
 * These are identical across processes and never written, so fork's
 * segcopy cloning them per process is harmless.  They are linked into
 * user.rel after the leaf code.
 *
 * sys/leaf/consts.c
 */

struct syscall
{
    char nbytes;                /* no. of bytes in arguments */
    int (*call)();              /* address of system call */
};

extern int indir(), r_exit(), r_fork(), r_read(), r_write(), r_open(),
    r_close(), r_wait(), r_creat(), link(), unlink(), exec(), chdir(),
    r_time(), mknod(), chmod(), chown(), brake(), stat(), r_seek(),
    r_getpid(), mount(), umount(), r_setuid(), r_getuid(), r_stime(),
    unimp(), r_alarm(), r_fstat(), pause(), badcall(), r_stty(), r_gtty(),
    permission(), r_nice(), r_sleep(), sync(), r_kill(), r_csw(), r_ssw(),
    r_dup(), r_pipe(), r_signal();

/*
 * System call branch table.  Arguments from registers, and returns to
 * registers, are handled by the r_ functions.
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
    0, &r_time,                 /* 13 */
    6, &mknod,                  /* 14 */
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
};

int ncalls = sizeof(syssw) / sizeof(struct syscall);

/*
 * Device switch tables, from con.c.  struct biovec/ciovec and the
 * array declarations come from sys/con.h.
 */
#include <types.h>
#include <sys/con.h>

extern int nodev(), nulldev(), nullwrite();
extern int djopen(), djclose(), djstrat();
extern int mwopen(), mwclose(), mwstrat();
extern int muopen(), muclose(), muread(), muwrite(), mustty();
extern int kread(), kwrite(), ioread(), iowrite();
extern int djmopen(), djmclose(), djmread(), djmwrite(), djstty();

/* Device names for diagnostics.  String literals ride the u page
 * now that ccc parks them in .data. */
char *devname[] = {
    "nodev", "hdca(rev4)", "djdma", "hddma",
};

/* Device 0 must be nodev. */
struct biovec biosw[] = {
    &nodev, &nulldev, &nulldev,         /* 0 = no device */
    &nodev, &nodev, &nodev,             /* 1 = HDCA - removed, obsolete */
    &djopen, &djclose, &djstrat,        /* 2 = DJ-DMA */
    &mwopen, &mwclose, &mwstrat,        /* 3 = HD-DMA */
};

struct ciovec ciosw[] = {
    &nulldev, &nulldev, &nulldev, &nullwrite, &nodev,
    &muopen, &muclose, &muread, &muwrite, &mustty,
    &nulldev, &nulldev, &kread, &kwrite, &nodev,
    &nulldev, &nulldev, &ioread, &iowrite, &nodev,
    &djmopen, &djmclose, &djmread, &djmwrite, &djstty,
};

UINT nbdev = sizeof(biosw) / sizeof(struct biovec);
UINT ncdev = sizeof(ciosw) / sizeof(struct ciovec);
