/*
 * kill - send a signal to a process
 *
 * cmd/kill/kill.c
 *
 * There is no kill(1) in the distribution to port.  /bin holds 118
 * entries and kill is not one of them, and no floppy carries a source
 * for one: extra/ has v6's and v7's, which is where the interface
 * below comes from, and the system call it needs has been here all
 * along in lib/libu/kill.s.  What the system had instead was the
 * shell's own kill builtin, which reaches only the children that shell
 * forked and only under the name it remembers them by.  This is the
 * command.
 *
 *	kill -h			list the signals and stop
 *	kill pid ...		send SIGTERM (15)
 *	kill -9 pid ...		by number
 *	kill -HUP pid ...	by name
 *
 * v7's kill took the signal in the first argument only, and as a
 * number only.  The names are added here, and so is -h.  A leading SIG
 * is optional, so HUP and SIGHUP are the same word, and case does not
 * matter.
 *
 * TWO THINGS THIS REFUSES THAT kill(2) WOULD DO IF ASKED, both because
 * the kernel does not check what its own page says it checks:
 *
 *   signal 0 and 16 up.  Neither kill() in sys/sig.c nor send() range-
 *			checks the signal, and send() indexes slist[] with
 *			whatever it is handed.  slist is the receiving
 *			process's own dispositions, NSIG = 16 of them, a
 *			field of its proc entry rather than a table of its
 *			own, so kill(pid, 20) reads off the end of that
 *			array and into the rest of the entry and the next
 *			one along, and what it finds there decides whether
 *			the signal is delivered at all.  If it is, sig()
 *			indexes the same array with the same number a second
 *			time, reading a disposition nobody set and clearing
 *			it, so the process is either terminated on that
 *			number or jumped to whatever address the word held.
 *			Only 1..15 is passed on.
 *
 *   pid 0.		The page says pid 0 signals "all other processes
 *			with the same controlling tty", but killall() does
 *			not skip the caller - it sends to every process
 *			holding that tty, the shell that ran this command
 *			included, and there is no way back from that.
 *			Refused rather than documented.
 *
 * The names are the kernel's, from include/sys/signal.h, which is what
 * <sys/signal.h> is here - the table that is really raised, and the one
 * ed and tar read for the same reason.  Note that the C library's
 * <signal.h> still carries the two CP/M names for 6 and 7, IOT and EMT,
 * which this kernel does not raise: 6 is the terminal's ^B and 7 its
 * "input record available", and they are listed under those names.
 *
 * vim: tabstop=8 shiftwidth=8 noexpandtab:
 */

#include <types.h>
#include <stdio.h>
#include <sys/signal.h>

int	kill();			/* lib/libu/kill.s */

struct	sig {
	char	*name;
	char	*what;
};

/*
 * Indexed by number, so entry 0 is the hole where signal 0 would be.
 * NSIG is 16 and the last signal is 15, which is what the kernel's
 * kill() will accept and no more.
 */
struct	sig sigs[NSIG] = {
	{ "",     "" },
	{ "HUP",  "hangup" },
	{ "INT",  "interrupt" },
	{ "QUIT", "quit" },
	{ "ILL",  "illegal instruction" },
	{ "TRAP", "trace trap" },
	{ "BACK", "control-B typed" },
	{ "TINT", "tty input record available" },
	{ "FPE",  "floating point exception" },
	{ "KILL", "kill (cannot be caught or ignored)" },
	{ "BUS",  "bus error" },
	{ "SEGV", "segmentation violation" },
	{ "SYS",  "bad argument to system call" },
	{ "PIPE", "write on a pipe with no one to read it" },
	{ "ALRM", "alarm clock" },
	{ "TERM", "software termination signal" },
};

int	status;

main(argc, argv)
	int argc;
	char *argv[];
{
	register int i;
	int signo;

	if (argc > 1 && argv[1][0] == '-' && argv[1][1] != '\0') {
		if (argv[1][1] == 'h' && argv[1][2] == '\0')
			listsigs();
		signo = signame(&argv[1][1]);
		if (signo <= 0 || signo >= NSIG) {
			fprintf(stderr, "kill: %s: no such signal\n", argv[1]);
			exit(2);
		}
		argc--;
		argv++;
	} else
		signo = SIGTERM;

	if (argc < 2)
		usage();

	for (i = 1; i < argc; i++)
		onesig(argv[i], signo);

	exit(status);
}

/*
 * The signal the option names: a number as it stands, a name to be
 * looked up with or without its SIG on the front.  Case is folded, so
 * -hup, -Hup and -SIGHUP all answer 1.  0 means there is no such
 * signal, and 0 is never a signal that can be sent - see the head.
 */
signame(s)
	register char *s;
{
	register int i;
	register int j;
	register char *p;

	if (*s >= '0' && *s <= '9')
		return (atoi(s));

	if (upper(s[0]) == 'S' && upper(s[1]) == 'I' && upper(s[2]) == 'G')
		s += 3;

	for (i = 1; i < NSIG; i++) {
		p = sigs[i].name;
		for (j = 0; p[j] != '\0' && s[j] != '\0'; j++)
			if (p[j] != upper(s[j]))
				break;
		if (p[j] == '\0' && s[j] == '\0')
			return (i);
	}
	return (0);
}

upper(c)
	register int c;
{

	if (c >= 'a' && c <= 'z')
		return (c - 'a' + 'A');
	return (c);
}

/*
 * One process id.  A bad one is reported and the rest still go: v7
 * walked the whole list this way, and "kill 12 34 56" should not be
 * called off because 34 was reaped a moment ago.
 */
onesig(arg, signo)
	char *arg;
	int signo;
{
	register char *p;
	int pid;

	for (p = arg; *p != '\0'; p++)
		if (*p < '0' || *p > '9') {
			fprintf(stderr, "kill: %s: not a process id\n", arg);
			status = 1;
			return;
		}
	pid = atoi(arg);
	if (pid == 0) {
		/*
		 * 0 is killall(), every process on the terminal and this
		 * shell with them, so it is refused before the call.
		 * Nothing else has to be: procptr() searches the process
		 * table for a matching pid and answers ESRCH when there
		 * is none, so a number too large to be a pid - which
		 * atoi() wraps, "99999" arriving here negative - is
		 * reported as the missing process it is rather than as a
		 * malformed argument.
		 */
		fprintf(stderr, "kill: %s: not a process id\n", arg);
		status = 1;
		return;
	}
	if (kill(pid, signo) < 0) {
		/*
		 * errno is the kernel's: r_system() puts u.error in hl
		 * and sets the carry, so kill(2) returns -1 with errno
		 * set.  The message is perror's, the way owner.c asks
		 * for one.
		 */
		fprintf(stderr, "kill: ");
		perror(arg);
		status = 1;
	}
}

usage()
{

	fprintf(stderr, "usage: kill [ -sig ] pid ...\n");
	fprintf(stderr, "       kill -h\n");
	exit(2);
}

listsigs()
{
	register int i;

	for (i = 1; i < NSIG; i++)
		printf("%2d  %-5s %s\n", i, sigs[i].name, sigs[i].what);
	exit(0);
}
