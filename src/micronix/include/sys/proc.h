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
    UINT8 args[14];              /* for ps */
    UINT8 mode;                 /* see below */
    UINT8 uid;                  /* for signal sending */
    UINT16 event;                 /* for sleep-wakeup */
    UINT16 status;                /* termination status */
    UINT16 *frmptr;               /* system stack frame pointer */
    UINT16 *stkptr;               /* system stack stack pointer */
    struct proc *parent;        /* parent proc structure */
    UINT16 tty;                   /* controlling terminal */
    UINT16 swap;                  /* disk address of swapped image */
    UINT8 nsegs;                /* no. 4K memory segments */
    /*
     * The hardware task whose map ram holds this process's memory map,
     * or 0 for none.  Task 0 is the kernel's and the only supervisor
     * task, so a process that can return to user mode owns one of 1..15
     * and keeps its segmentation across a switch; see taskalloc()
     * (malloc.c), which takes a slot back from another process when all
     * fifteen are in use.
     */
    UINT8 task;
    struct mem {
        UINT8 seg;
        UINT8 per;
    }   mem[17];                /* memory map */
    UINT16 (*slist[NSIG]) ();     /* signal dispositions */

    /*
     * nice and pri form a short
     */
    UINT8 nice;                 /* user decreasable priority */
    UINT8 pri;                  /* code priority (user or system) */

#ifdef notdef
    UINT8 pad;      /* why ? */
    UINT8 cpu;      /* cpu priority, maintained by clock */
#endif

    UINT16 time;                  /* residency time in core or on disk */
    UINT16 alarm;                 /* seconds to alarm */
    INT16 pid;                    /* process Id number */
} plist[];

#define priority(p)	(*(UINT16 *)(&(p)->nice))

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
 * because the firmware pushes the trap frame down from trapstack:
 * fourteen words from &u.save land on &u.task, so offsets 4 to 31 are
 * the MPZ80's "task save area" and nothing else may live there.  The
 * word below it is where the firmware's own call into the kernel leaves
 * its return address, and below that the kernel's C stack grows down
 * out of the base of the struct and into ustack, so every other member
 * of u is out of its way.
 */
struct user {
    UINT16 stack;                 /* see task0_body() in trap.c */
    UINT16 ret;                   /* firmware return address, below the frame */
    UINT8 task;                 /* Registers saved by firmware */
    UINT8 mask;                 /* at trap time. */
    UINT8 *pc;
    UINT16 *sp;
    UINT16 af;
    UINT16 bc;
    UINT16 de;
    UINT16 hl;
    /*
     * The firmware saves more than the primary bank, and these are the
     * rest of it in the order it pushes them - the MPZ80 manual's "Task
     * Save Areas", which gotask in the monitor pops back.  Exposed by
     * name because the kernel returns a long as HL':HL, high half in
     * hl2 and low half in hl, so that DE comes back to a Whitesmiths
     * binary with its frame pointer still in it.  Only hl2 is written,
     * and only by the calls that return a long.
     */
    UINT16 ireg;                  /* interrupt register + flags */
    UINT16 ix;                    /* the user's index registers */
    UINT16 iy;
    UINT16 af2;                   /* the alternate bank */
    UINT16 bc2;
    UINT16 de2;
    UINT16 hl2;                   /* high half of a returned long */
    UINT8 save;                  /* dummy for top of save area */

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
    UINT8 *brake;                /* end of code-data segment + 1 */
    INT16 *abort;                 /* see docall (system.c) */
    struct inode *cdir;         /* current directory */
    struct file *olist[NOPEN];  /* open files */
    struct inode *iparent;      /* temp for create, link */
    UINT8 segflg;               /* base in KERNEL or USER */
    /*
     * The overlay page this process is running in, as a segment, 0 for
     * none.  The overlay window is one page for the whole system, so a
     * process parked in a driver's strategy routine - ide's idestart,
     * ncr's scsistart and dj's busplease all sleep there - would be
     * switched out, another process would map another driver, and the
     * first would wake up executing the second's bytes.  Having it here
     * makes the window per-process: ovlbmap(), ovlcmap() and ovlcall()
     * (sys/ovl.c) record the segment as they map it, and ovlremap() puts
     * that page back at every switch, because the u page is the incoming
     * process's by then.
     */
    UINT8 drv;
    UINT32 offset;              /* for file access */
    UINT8 *base;                 /* ditto */
    UINT16 count;                 /* ditto */
    UINT16 fdev;                  /* an open's forwarded device (iopen) */
    struct dir {
        UINT16 inum;
        UINT8 name[14];
    }   dir;                    /* for directory searches */
    UINT16 utime;                 /* process user time */
    UINT16 stime;                 /* process system time */
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

/*
 * The current process's memory map, as mem.s sees it: a pointer to
 * u.p->mem, set by newmap() (malloc.c) at every switch.  mem.s is hand
 * written, so it cannot name a member of struct proc - a byte added
 * anywhere ahead of mem[] would go on working and point the wrong page
 * into the window - and this is one store per switch instead.
 */
extern UINT8 *curmem;

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
