/*
 * process table
 *
 * include/sys/proc.h
 * Changed: <2021-12-23 14:29:50 curt>
 */

/*
 * A table of 1 proc structure per process is maintained in the kernel.
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
 *
 * The system stack is not a member here: it is the 512-byte object
 * ustack (user.c), which the linker places immediately below u in
 * USERSEG.  The register save area has to come first in the struct
 * because the firmware pushes the trap frame down from trapstack and
 * leaves SP at &u.stack (see trap.c): the kernel's C stack grows down
 * out of the base of the struct and into ustack, so every other member
 * of u is out of its way.
 */
struct user {
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
    /*
     * The firmware saves more than the primary bank, and these are the
     * rest of it in the order it pushes them - the MPZ80 manual's "Task
     * Save Areas", which gotask in the monitor pops back.  Exposed by
     * name because the kernel returns a long as HL':HL, high half in
     * hl2 and low half in hl, so that DE comes back to a Whitesmiths
     * binary with its frame pointer still in it.  Only hl2 is written,
     * and only by the calls that return a long.
     */
    UINT ireg;                  /* interrupt register + flags */
    UINT ix;                    /* the user's index registers */
    UINT iy;
    UINT af2;                   /* the alternate bank */
    UINT bc2;
    UINT de2;
    UINT hl2;                   /* high half of a returned long */
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
};

/*
 * user.c initializes it, so this is a declaration and not another
 * definition: a header that says "} u;" gives every file that
 * includes it its own u in bss, and the one file with the
 * initializer puts its copy in data.  Those do not merge and must
 * not - the initialized one is the object.
 */
extern struct user u;

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
