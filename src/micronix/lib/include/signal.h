/*
 *	Signal definitions for CP/M
 */
#ifdef	unix
#define NSIG 17

#define	SIGHUP	1	/* hangup    (not used by terminal driver) */
#define	SIGINT	2	/* interrupt (^C or BREAK) */
#define	SIGQUIT	3	/* quit      (^\) */
#define	SIGILL	4	/* illegal instruction (not reset when caught) */
#define	SIGTRAP	5	/* trace trap (not reset when caught) */
#define	SIGIOT	6	/* IOT instruction */
#define	SIGEMT	7	/* EMT instruction */
#define	SIGFPE	8	/* floating point exception */
#define	SIGKILL	9	/* kill (cannot be caught or ignored) */
#define	SIGBUS	10	/* bus error */
#define	SIGSEGV	11	/* segmentation violation */
#define	SIGSYS	12	/* bad argument to system call */
#define	SIGPIPE	13	/* write on a pipe with no one to read it */
#define	SIGALRM	14	/* alarm clock */
#define	SIGTERM	15	/* software termination signal from kill */
#else	unix
#define	SIGINT	1		/* control-C */
#endif	unix

#define	SIG_DFL	0		/* default action is to exit */
#define	SIG_IGN	1		/* ignore them */

/*
 * The handler here is an int, not a function pointer.  libunix's
 * signal() keeps the address in a short and the trampolines call it;
 * a function name in an argument is an address, and that is the
 * calling convention this system has.  The POSIX spelling was a
 * fiction that never matched the body.
 */
extern short	signal(int, short);

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
