/*
 * sys/sys.h
 *
 * sort of a kitchen sink, really
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
#define USERSEG 3               /* segment 3. See user.c and malloc.c */

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
