/*
 * system - hand a command line to the shell
 *
 * lib/libc/system.c
 *
 * Fork, let the child exec the shell with -c, and wait for it.  The
 * shell is what makes this worth having: anything with a pipe, a
 * redirection or a wildcard in it cannot be exec'd directly, and this
 * is how a program gets those without parsing them itself.
 *
 * Returns the child's wait status, or -1 if the fork failed.  A shell
 * that could not be exec'd shows up as 127 in the status, which is
 * the convention everywhere and lets the caller tell "the command
 * failed" from "there was no shell".
 *
 * Written because Make wanted it and nothing provided it; it lived in
 * make.c for a while, which was the wrong place for a libc function.
 *
 * vim: tabstop=4 shiftwidth=4 noexpandtab:
 */

#define SHELL	"/bin/sh"

int
system(cmd)
char *cmd;
{
	int child;
	int w;
	int status;

	if (cmd == 0)
		return 0;

	status = 0;
	child = fork();
	if (child < 0)
		return -1;

	if (child == 0) {
		execl(SHELL, "sh", "-c", cmd, 0);
		/*
		 * Only reached if the shell is not there.  _exit rather
		 * than exit: the child inherited the parent's stdio
		 * buffers and must not flush them a second time.
		 */
		_exit(127);
	}

	/*
	 * Wait for our own child.  Anything else that comes back is
	 * someone else's and is discarded, which is what this call has
	 * always done - a program that cares about other children
	 * should not be using system().
	 */
	while ((w = wait(&status)) != -1 && w != child)
		;

	if (w == -1)
		return -1;

	return status;
}
