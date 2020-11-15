/*
 * sys.h 
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
#define NULL	0
#define YES	1
#define NO	0
#define READ	0
#define WRITE	1
#define UPDATE	2
#define ERROR	-1
#define OK	0
#define ERRBIT	1               /* 8080 carry flag */
#define BYTMASK 0377
#define HERTZ	8

/*
 * signals
 */
#define SIGHUP  1
#define SIGINT	2               /* tty.c */
#define SIGQUIT 3               /* tty.c */
#define SIGILL	4               /* trap.c */
#define SIGBACK 6               /* tty.c */
#define SIGTINT 7               /* tty.c */
#define SIGKILL 9               /* sig.c */
#define SIGMEM	11              /* malloc.c */
#define SIGBAD	12              /* system.c */
#define SIGPIPE 13              /* rdwri.c */
#define SIGALRM 14              /* time.c */

/*
 * macros
 */
#define min(x, y)	(((x) < (y))? (x) : (y))
#define max(x, y)	(((x) < (y))? (y) : (x))
#define until(x)	while(!(x))
#define forever		for(;;)

/*
 * Types
 */
#define fast register

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
