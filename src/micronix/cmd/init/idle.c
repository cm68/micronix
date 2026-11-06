/*
 * The single user shell's idle check, split out of init.c.
 *
 * cmd/init/idle.c
 *
 * Two questions about the shell, answered from the kernel's process
 * table through /dev/mem, the hook and the layout ps uses:
 *
 *	waiting at the tty?	the shell sleeps on its tty's raw input
 *				queue (cwait in sys/tty.c), so its event
 *				is &tty->rawque, not its own proc entry
 *
 *	has it run a command?	a command is a live child of the shell's
 *				proc entry
 *
 * init calls this once a second after the shell starts.  The first
 * ttywait is where the login script has finished and the shell is
 * asking for input; a child after that is somebody typing.  A shell
 * that reaches ttywait and then has no child for thirty seconds is
 * unattended, and init ends single user.
 *
 * This file exists because <sys/proc.h> defines a struct dir of its
 * own; including it in init.c would shadow sys/dir.h's directory entry
 * and the directory walk there stops compiling.
 */
#include <types.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/signal.h>
#include <sys/sys.h>
#include <sys/proc.h>
#include <sys/tty.h>
#include <signal.h>

extern int errno;

struct proc plist[NPROC] = 0;	/* as ps declares it; the read fills it */
static struct proc *ptab_base;	/* the physical base the table came from */

/*
 * &((struct tty *)0)->rawque.  The struct has no padding - it is the
 * address the compiler puts the member at, read out of the object for
 * the layout in sys/tty.h.
 */
#define RAWQOFF	21

/*
 * Read _plist into plist[].  1 on success, 0 when /dev/mem cannot be
 * read: the caller must not act on a table it did not get.
 */
static int
readtable()
{
	int mem;
	struct proc *ptab;

	mem = open("/dev/mem", O_RDONLY);
	if (mem < 0)
		return (0);
	if (seek(mem, 0x1003, 0) < 0 ||
	    read(mem, &ptab, sizeof ptab) != sizeof ptab ||
	    seek(mem, ptab, 0) < 0 ||
	    read(mem, plist, sizeof plist) != sizeof plist) {
		close(mem);
		return (0);
	}
	close(mem);
	ptab_base = ptab;
	return (1);
}

/*
 * Bit 0: the shell is waiting at the tty input queue.
 * Bit 1: the shell has a live child, i.e. it is running a command.
 * Bit 2: the table could not be read; neither claim is safe, so init
 * skips this sample.
 */
shellflags(shellpid)
int shellpid;
{
	int i, shell, flags;

	if (!readtable())
		return (4);

	for (i = 0; i < NPROC; i++)
		if ((plist[i].mode & ALIVE) && plist[i].pid == shellpid) {
			shell = i;
			break;
		}
	if (shell < 0)
		return (4);

	flags = 0;
	if ((int) plist[shell].event == (int) plist[shell].tty + RAWQOFF)
		flags |= 1;
	for (i = 0; i < NPROC; i++)
		if (i != shell && (plist[i].mode & ALIVE) &&
		    plist[i].parent == ptab_base + shell) {
			flags |= 2;
			break;
		}
	return (flags);
}

/*
 * SIGKILL every live process but init.  Used by init's return to single
 * user: iterate the process table rather than a pid range, because pids
 * are allocated up to 30000 and a missed one leaves init's wait unable
 * to finish.  Zombies are left for the wait loops that follow.
 */
killothers()
{
	int i;

	if (!readtable())
		return;
	for (i = 0; i < NPROC; i++)
		if ((plist[i].mode & ALIVE) && plist[i].pid > 1)
			kill(plist[i].pid, SIGKILL);
}

/*
 * Run shellflags() in a child so the table read never runs on init's
 * stack, and hand the bits back in the high half of the exit status
 * (libc exit keeps the code in the high byte).  -1 means the shell
 * itself exited while the check ran.
 */
idlecheck(shellpid)
int shellpid;
{
	int c, st, r;

	c = fork();
	if (c < 0)
		return (0);
	if (c == 0) {
		alarm(0);
		signal(SIGALRM, SIG_IGN);
		exit(shellflags(shellpid));
	}
	for (;;) {
		r = wait(&st);
		if (r == c)
			return ((st >> 8) & 0377);
		if (r == shellpid)
			return (-1);
		if (r == -1 && errno == EINTR)
			continue;
		if (r == -1)
			return (0);
	}
}

/* vim: tabstop=4 shiftwidth=4 expandtab: */
