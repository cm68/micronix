/*
 * job.c - the background jobs, and killing one by name
 *
 * micronix/cmd/sh/job.c
 *
 * man1/sh.1, under kill:
 *
 *	kill N
 *	kill name
 *
 *	The first version kills the process whose numerical id is N.
 *	The second version kills the process with the given command
 *	name (the name by which the process was invoked, which is also
 *	reported by ps).
 *
 * "which is also reported by ps" is a comparison and not a mechanism,
 * and the mechanism is worth writing down because the obvious reading
 * is wrong.  ps gets the name out of the KERNEL: struct proc begins
 * "char args[8];  / * for ps * /", so the process table carries it,
 * and ps opens /dev/mem to go and read it.
 *
 * THE SHELL DOES NOT DO THAT.  There is no /dev/mem anywhere in the
 * image - the only device string in it is /dev/null - so it cannot be
 * looking at the process table at all.  It matches against its own
 * record of what it started: it knows the name because it ran it, and
 * the pid because it forked it.  That is the same record the page
 * needs elsewhere, where it says the shell gives notification when an
 * asynchronous command finishes.
 *
 * The cost of it being the shell's own record rather than the
 * system's: a command started by a subshell is invisible here, since
 * the record belongs to whichever shell forked it and that one has
 * exited.  Nothing can be done about that short of reading /dev/mem,
 * which the image does not do either.
 *
 * vim: tabstop=4 shiftwidth=4 noexpandtab:
 */

#include <stdio.h>
#include "sh.h"

#define NJOB    16              /* enough background jobs to lose track of */
#define JNAME   15

struct job {
	int  pid;                   /* 0 when the slot is free */
	char name[JNAME + 1];
};

static struct job jobs[NJOB];

/*
 * Remember one.  The name is what was typed, after any alias has been
 * expanded - the name by which the process was invoked, which is what
 * the page says and what the user has to type at kill.
 */
void
jobadd(pid, name)
int pid;
char *name;
{
	int i;
	int j;

	if (pid <= 0)
		return;
	for (i = 0; i < NJOB; i++)
		if (jobs[i].pid == 0)
			break;
	if (i == NJOB)
		i = 0;                  /* the oldest goes; nothing better to do */

	jobs[i].pid = pid;
	for (j = 0; j < JNAME && name[j]; j++)
		jobs[i].name[j] = name[j];
	jobs[i].name[j] = '\0';
}

/*
 * Forget one, when it has been reaped.  A slot left behind would name
 * a pid the system is free to give to somebody else, and killing that
 * is worse than not finding it.
 */
void
jobdone(pid)
int pid;
{
	int i;

	for (i = 0; i < NJOB; i++)
		if (jobs[i].pid == pid)
			jobs[i].pid = 0;
}

/*
 * The pid we started under this name, most recent first, or -1.
 */
int
jobpid(name)
char *name;
{
	int i;

	for (i = NJOB - 1; i >= 0; i--)
		if (jobs[i].pid && strcmp(jobs[i].name, name) == 0)
			return jobs[i].pid;
	return -1;
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
