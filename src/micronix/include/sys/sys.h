/*
 * sort of a kitchen sink, really
 *
 * include/sys/sys.h
 * Changed: <2021-12-23 14:35:26 curt>
 */

/*
 * parameters
 */
#define NPROC	20
#define NINODE	50
#define NMOUNT	4
#define NFILE	60
#define NOPEN	16
#define NSIG	16
#define MINRUN	2               /* minimum run time before swapout */
#define MAXMEM	0xffff          /* maximum process size */
#define MAXSEG	256             /* max 4K memory segments, not inc. kernel */
#define USERSEG 15              /* u page segment; the buffer window is 14 */
#define BUFSEG  14              /* the buffer-cache window (bwin, uio.c) */
#define BUFWIN  (BUFSEG * 0x1000)   /* ... its page base, and the pool's top */

/*
 * The scratch window, and the only page left that a driver may hold.
 *
 * It is the page the firmware has always remapped to reach another
 * segment: mem.s's getbyte/putbyte/memrw borrow it and put it back, which
 * is what the superblock's window used to do before getsb() moved the
 * superblock into a slot of the kernel's own memory (uio.c).  A driver
 * that needs a segment of its own visible *beside* a buffer cannot have
 * BUFSEG for it, because that is where the buffer is, and this is the one
 * other page there is.
 *
 * swin()/srel() (uio.c) hold it the way bhold()/brel() hold the buffer
 * window, and carry the same two rules: never sleep holding one, and call
 * nothing that maps this window while one is held.  Sharing it with mem.s
 * is safe because mem.s holds interrupts off across its own borrow, so no
 * handler can ever see the register half borrowed.
 */
#define SCRSEG  13              /* the scratch window (swin, uio.c) */
#define SCRWIN  (SCRSEG * 0x1000)   /* ... its page base, 0xd000 */

/*
 * Priorities
 */
#define PRIMEM	100
#define PRISWAP 90
#define PRIBIO	90
#define PRINOD	80
#define SIGPRI	80              /* below here, signal can wake a sleeper */
#define PRIPIPE 70              /* rdwri.c */
#define PRITTI	60              /* tty.c */
#define PRITTO	60              /* tty.c */
#define PRIWAIT 55              /* fork.c */
#define PRISYS	50              /* system.c */
#define PRIUSER 40              /* sleep.c, system.c */

/*
 * constants
 */
#define READ	0
#define WRITE	1
#define UPDATE	2
#define ERROR	-1
#define HERTZ	8

/*
 * the 8080 carry flag is used to signal an error in the
 * system call interface.   this is it's bit position in AF.
 */
#define ERRBIT	1

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
