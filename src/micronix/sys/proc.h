/*
 * proc.h 
 */

/*
 * Process structure. A table of 1 proc structure
 * per process is maintained in the kernel.
 */
struct proc
{
    UCHAR args[8];              /* for ps */
    UCHAR mode;                 /* see below */
    UCHAR uid;                  /* for signal sending */
    UINT event;                 /* for sleep-wakeup */
    UINT status;                /* termination status */
    UINT *frmptr;               /* system stack frame pointer */
    UINT *stkptr;               /* system stack stack pointer */
    struct proc *parent;        /* parent proc structure */
    UINT tty;                   /* controlling terminal */
    UINT swap;                  /* disk address of swapped image */
    UCHAR nsegs;                /* no. 4K memory segments */
    struct mem
    {
        UCHAR seg;
        UCHAR per;
    } mem[17];                  /* memory map */
        UINT(*slist[NSIG]) ();  /* signal dispositions */

    /*
     * nice and pri form a short 
     */
    UCHAR nice;                 /* user decreasable priority */
    UCHAR pri;                  /* code priority (user or system) */

#ifdef notdef
    /*
     * UCHAR pad; /* pri-pad-cpu-nice form a long 
     */

    /*
     * UCHAR cpu; /* cpu priority, maintained by clock 
     */
#endif

    UINT time;                  /* residency time in core or on disk */
    UINT alarm;                 /* seconds to alarm */
    int pid;                    /* process Id number */
} plist[];

#define priority(p)	(*(unsigned short *)(&(p)->nice))

/*
 * Mode bits
 */
#define ALLOC	0001            /* this process entry is in use */
#define ALIVE	0002            /* not yet terminated */
#define AWAKE	0004            /* ready to run */
#define LOADED	0010            /* image is in core */
#define SWAPPED 0020            /* image is on disk */
#define LOCKED	0040            /* locked in core */
#define SYS	0100            /* in system phase */
#define BACK	0200            /* in background -- eof on tty reads */

/*
 * Memory map permission codes
 */
#define NONE	0
#define FULL	3
#define GROW	7

/*
 * User structure
 */
struct user
{
    UCHAR s[512];               /* this processes' system stack */

    UINT stack;                 /* see trap0() in trap.c */
    UINT ret;                   /* dummy place holder */
    UCHAR task;                 /* Registers saved by firmware */
    UCHAR mask;                 /* at trap time. */
    UCHAR *pc;
    UINT *sp;
    UINT af;
    UINT bc;
    UINT de;
    UINT hl;
    UCHAR zregs[14];
    UCHAR save;                 /* dummy for top of save area */

    struct proc *p;             /* this process */
    UCHAR error;                /* see below */
    UCHAR uid;                  /* real user id */
    UCHAR gid;                  /* real group id */
    UCHAR euid;                 /* effective user id */
    UCHAR egid;                 /* effective group id */
    char *brake;                /* end of code-data segment + 1 */
    int *abort;                 /* see docall (system.c) */
    struct inode *cdir;         /* current directory */
    struct file *olist[NOPEN];  /* open files */
    struct inode *iparent;      /* temp for create, link */
    UCHAR segflg;               /* base in KERNEL or USER */
    ULONG offset;               /* for file access */
    char *base;                 /* ditto */
    UINT count;                 /* ditto */
    struct dir
    {
        UINT inum;
        char name[14];
    } dir;                      /* for directory searches */
    UINT utime;                 /* process user time */
    UINT stime;                 /* process system time */
    ULONG cutime;               /* child user times */
    ULONG cstime;               /* child system times */
} u;

/*
 * Values for segflg
 */
#define KSEG	1
#define USEG	0

/*
 * error codes
 */
#define EPERM	1
#define ENOENT	2
#define ESRCH	3
#define EINTR	4
#define EIO	5
#define ENXIO	6
#define E2BIG	7
#define ENOEXEC 8
#define EBADF	9
#define ECHILD	10
#define EAGAIN	11
#define ENOMEM	12
#define EACCES	13
#define ESYS	14
#define ENOTBLK 15
#define EBUSY	16
#define EEXIST	17
#define EXDEV	18
#define ENODEV	19
#define ENOTDIR 20
#define EISDIR	21
#define EINVAL	22
#define ENFILE	23
#define EMFILE	24
#define ENOTTY	25
#define ETXTBSY 26
#define EFBIG	27
#define ENOSPC	28
#define ESPIPE	29
#define EROFS	30
#define EMLINK	31
#define EPIPE	32
/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
