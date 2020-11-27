/*
 * sys/signal.h
 *
 * all the signal names
 */
#define	SIG_DFL		0
#define	SIG_IGN	 	1

#define	SIGHUP		1
#define	SIGINT		2
#define	SIGQUIT		3
#define	SIGILL		4
#define	SIGTRAP		5
#define	SIGBACK		6       /* control - b typed */
#define	SIGTINT		7       /* tty input record available */
#define	SIGFPE		8       /* floating point exception */
#define	SIGKILL		9
#define	SIGBUS		10
#define	SIGSEGV		11
#define	SIGSYS		12
#define	SIGPIPE		13
#define	SIGALRM		14
#define	SIGTERM		15

/*
 * other parts of the kernel use SIGMEM = 11 SIGBAD = 12
 */

#define	NSIG		16

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
