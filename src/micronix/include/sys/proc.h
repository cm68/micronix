/*
 * sys/proc.h
 */

/*
 * Process structure. A table of 1 proc structure per process is maintained
 * in the kernel.
 */
struct proc {
    char args[8];               /* for ps */
    UINT8 mode;                 /* see below */
    UINT8 uid;                  /* for signal sending */
    UINT event;                 /* for sleep-wakeup */
    UINT status;                /* termination status */
    UINT *frmptr;               /* system stack frame pointer */
    UINT *stkptr;               /* system stack stack pointer */
    struct proc *parent;        /* parent proc structure */
    UINT tty;                   /* controlling terminal */
    UINT swap;                  /* disk address of swapped image */
    UINT8 nsegs;                /* no. 4K memory segments */
    struct mem {
        UINT8 seg;
        UINT8 per;
    }   mem[17];                /* memory map */
    UINT (*slist[NSIG]) ();     /* signal dispositions */

    /*
     * nice and pri form a short
     */
    UINT8 nice;                 /* user decreasable priority */
    UINT8 pri;                  /* code priority (user or system) */

#ifdef notdef
    UCHAR pad;      /* why ? */
    UCHAR cpu;      /* cpu priority, maintained by clock */
#endif

    UINT time;                  /* residency time in core or on disk */
    UINT alarm;                 /* seconds to alarm */
    UINT8 pid;                  /* process Id number */
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
#define SYS     0100            /* in system phase */
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
struct user {
    char s[512];                /* this processes' system stack */

    UINT stack;                 /* see trap0() in trap.c */
    UINT ret;                   /* dummy place holder */
    UINT8 task;                 /* Registers saved by firmware */
    UINT8 mask;                 /* at trap time. */
    char *pc;
    UINT *sp;
    UINT af;
    UINT bc;
    UINT de;
    UINT hl;
    char zregs[14];
    char save;                  /* dummy for top of save area */

    struct proc *p;             /* this process */
    UINT8 error;                /* see below */
    union bytepair real;       /* real user id */
    union bytepair effective;  /* effective user id */
#ifdef notdef
    UINT8 uid;                  
    UINT8 gid;                  /* real group id */
    UINT8 euid;                 
    UINT8 egid;                 /* effective group id */
#endif
    char *brake;                /* end of code-data segment + 1 */
    int *abort;                 /* see docall (system.c) */
    struct inode *cdir;         /* current directory */
    struct file *olist[NOPEN];  /* open files */
    struct inode *iparent;      /* temp for create, link */
    UINT8 segflg;               /* base in KERNEL or USER */
    UINT32 offset;              /* for file access */
    char *base;                 /* ditto */
    UINT count;                 /* ditto */
    struct dir {
        UINT inum;
        char name[14];
    }   dir;                    /* for directory searches */
    UINT utime;                 /* process user time */
    UINT stime;                 /* process system time */
    UINT32 cutime;              /* child user times */
    UINT32 cstime;              /* child system times */
} u;

#define u_uid   real.bytes.low
#define u_gid   real.bytes.low
#define u_euid   effective.bytes.low
#define u_egid   effective.bytes.low

/*
 * Values for segflg
 */
#define KSEG	1
#define USEG	0

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
