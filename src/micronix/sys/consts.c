/*
 * Read-only kernel tables, parked in the u page (see ../user.c).
 *
 * These are identical across processes and never written, so fork's
 * segcopy cloning them per process is harmless.  They are linked into
 * upage.o after the leaf code.
 *
 * sys/consts.c
 */

struct syscall
{
    char nbytes;                /* no. of bytes in arguments */
    int (*call)();              /* address of system call */
};

extern int indir(), exit(), r_fork(), read(), write(), open(),
    r_close(), wait(), creat(), link(), unlink(), exec(), execve(), chdir(),
    r_time(), mknod(), chmod(), chown(), brake(), stat(), r_seek(),
    getpid(), mount(), umount(), setuid(), getuid(), stime(),
    unimp(), alarm(), fstat(), r_ftruncate(), pause(), badcall(), stty(), gtty(),
    r_ioctl(),
    permission(), nice(), snooze(), sync(), reboot(), kill(), r_csw(), ssw(),
    dup(), r_pipe(), signal();

/*
 * System call branch table.  Arguments from registers, and returns to
 * registers, are handled by the workers themselves; the r_ entries left
 * here are the ones with real register or carry convention work (fork,
 * seek, ftruncate, ioctl, pipe, time) or a shared worker (close, csw).
 */
struct syscall syssw[] = {
    2, &indir,                  /* 0 */
    0, &exit,                   /* 1 */
    0, &r_fork,                 /* 2 */
    4, &read,                   /* 3 */
    4, &write,                  /* 4 */
    4, &open,                   /* 5 */
    0, &r_close,                /* 6 */
    0, &wait,                   /* 7 */
    4, &creat,                  /* 8 */
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
    0, &getpid,                 /* 20 */
    6, &mount,                  /* 21 */
    2, &umount,                 /* 22 */
    0, &setuid,                 /* 23 */
    0, &getuid,                 /* 24 */
    0, &stime,                  /* 25 */
    6, &unimp,                  /* 25 &ptrace */
    0, &alarm,                  /* 27 */
    2, &fstat,                  /* 28 */
    0, &pause,                  /* 29 */
    0, &badcall,                /* */
    2, &stty,                   /* 31 */
    2, &gtty,                   /* 32 */
    4, &permission,             /* 33 */
    0, &nice,                   /* 34 */
    0, &snooze,                 /* 35 */
    0, &sync,                   /* 36 */
    2, &kill,                   /* 37 */
    0, &r_csw,                  /* 38 */
    0, &ssw,                    /* 39 */
    0, &badcall,                /* */
    0, &dup,                    /* 41 */
    0, &r_pipe,                 /* 42 */
    2, &unimp,                  /* 43 &times */
    8, &unimp,                  /* 44 &profil */
    0, &badcall,                /* */
    4, &r_ftruncate,            /* 46 ftruncate */
    0, &badcall,                /* */
    4, &signal,                 /* 48 */
    0, &badcall,                /* 49 lock (reserved) */
    0, &badcall,                /* 50 unlock (reserved) */
    0, &badcall,                /* 51 */
    0, &badcall,                /* 52 */
    0, &badcall,                /* 53 */
    4, &r_ioctl,                /* 54 ioctl */
    2, &reboot,                 /* 55 */
    6, &execve,                 /* 56 execve(name, argv, envp) */
};

int ncalls = sizeof(syssw) / sizeof(struct syscall);

/*
 * Device switch tables, from con.c.  struct biovec/ciovec and the
 * array declarations come from sys/con.h.
 */
#include <types.h>
#include <sys/con.h>
#include <sys/ovl.h>

extern int nodev(), nulldev(), nullwrite();
extern int muopen(), muclose(), muread(), muwrite(), mustty();
extern int kread(), kwrite(), ioread(), iowrite();
extern int ovlopen(), ovlclose(), ovlstrat(), ovlioctl();
extern int ovlcopen(), ovlcclose(), ovlcread(), ovlcwrite(), ovlcmode();
extern int netopen(), netclose(), netread(), netwrite();

/* Device 0 must be nodev.  The disk rows point at the module trampolines
 * (sys/ovl.c), not at drivers: the kernel has no djopen, mwstrat or
 * idestrat to name, because the drivers are modules and are not in it.
 * Which module answers for a major is decided when that module's init()
 * registers - see sys/ovl.h.
 *
 * Every row here is a major some module claims, and the claim is made at
 * boot: a module whose card is not in the machine is never placed, and
 * its major then answers as nodev does.  A row that no module claims
 * shows up in the signon "disks:" list under whatever module was mapped
 * last, because the print maps the major and reads devname - major 5
 * reads "ide" on a machine with no SCSI card for exactly that reason. */
struct biovec biosw[] = {
    &nodev, &nulldev, &nulldev, 0,              /* 0 = no device */
    &ovlopen, &ovlclose, &ovlstrat, &ovlioctl,  /* 1 = HDCA, a module */
    &ovlopen, &ovlclose, &ovlstrat, &ovlioctl,  /* 2 = DJ-DMA, a module */
    &ovlopen, &ovlclose, &ovlstrat, &ovlioctl,  /* 3 = HD-DMA, a module */
    &ovlopen, &ovlclose, &ovlstrat, &ovlioctl,  /* 4 = IDE (ATA), a module */
    &ovlopen, &ovlclose, &ovlstrat, &ovlioctl,  /* 5 = NCR 5380 SCSI, a module */
    &ovlopen, &ovlclose, &ovlstrat, &ovlioctl,  /* 6 = ND, a module */
    &ovlopen, &ovlclose, &ovlstrat, &ovlioctl,  /* 7 = DISK 2, a module */
    &ovlopen, &ovlclose, &ovlstrat, &ovlioctl,  /* 8 = DISK 3, a module */
    &ovlopen, &ovlclose, &ovlstrat, &ovlioctl,  /* 9 = DISK 1A, a module */
};

struct ciovec ciosw[] = {
    &nulldev, &nulldev, &nulldev, &nullwrite, &nodev,
    &muopen, &muclose, &muread, &muwrite, &mustty,
    &nulldev, &nulldev, &kread, &kwrite, &nodev,
    &nulldev, &nulldev, &ioread, &iowrite, &nodev,
    &ovlcopen, &ovlcclose, &ovlcread, &ovlcwrite, &ovlcmode,
    &netopen, &netclose, &netread, &netwrite, &nodev,
};

UINT nbdev = sizeof(biosw) / sizeof(struct biovec);
UINT ncdev = sizeof(ciosw) / sizeof(struct ciovec);
