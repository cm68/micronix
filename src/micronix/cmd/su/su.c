/*
 * su [name]
 *
 * v7 su (usr/src/cmd/su.c), ported to micronix.
 *
 * cmd/su/su.c
 *
 * WHICH su THIS IS, and why.  Two were available: the v7 one, fifty
 * lines, and the 2.11BSD one (extra/2.11/pdp11/usr/src/bin/su.c),
 * a hundred and eighty-three.  This follows v7.
 *
 * The 2.11 su is not a longer version of the same program, it is a
 * Berkeley one with a policy the seventh edition did not have and a
 * library this machine does not have.  It wants syslog, initgroups,
 * getgrgid, setpriority, ttyname, getenv and setenv, and this tree
 * has none of them: syslog and the process-priority calls are not
 * system calls here at all, ttyname is written out by hand where it
 * is wanted (cmd/login/login.c, cmd/ps), and there is no process
 * environment to get from or set into.  What it would buy is a
 * group-zero check before su to root, -f and - full logins, and
 * HOME/SHELL/USER export - none of which the 1982 page for this
 * command mentions.  su.1 here says "Non super users will be asked to
 * give passwords" and stops, which is v7's program.  So the port is
 * v7's, and what 2.11 adds is named here rather than half-built.
 *
 * What the port had to say differently:
 *
 *	setgid then setuid	became one setuid.  There is no setgid
 *			call on this machine: the kernel takes the pair
 *			as a single word, uid | gid << 8, and that is
 *			the same packing chown takes - see owner (1).
 *			The unchecked calls are the original's; neither
 *			version of the original tested them.
 *
 *	execl		is not in this tree's libraries.  execv is, and
 *			login uses it, so the shell is exec'd with a
 *			two-entry argv: "su" and a null.
 *
 *	environ		does not exist at a program's start here, and
 *			so the loop that rewrote PS1 to "# " is gone
 *			with it.  getenv (3) builds an environment array
 *			by reading a file called ENVIRON on its first
 *			call and nothing fills one in before that, so
 *			the loop would have walked a null pointer.  The
 *			shell here has no PS1 either - nothing in the
 *			tree mentions it - so the loop had nothing to
 *			aim at besides.
 *
 *	getpass and crypt are libc here, declared by <crypt.h>, and are
 * the same two routines passwd (1) uses; the salt comes from the
 * stored string, so the comparison is the one the original made.
 *
 * vim: tabstop=8 shiftwidth=8 noexpandtab:
 */

#include <types.h>
#include <stdio.h>
#include <pwd.h>
#include <crypt.h>

struct	passwd *pwd;
char	*argvec[2];

main(argc, argv)
	int argc;
	char **argv;
{
	char *nptr;
	char *password;
	char *shell = "/bin/sh";

	if (argc > 1)
		nptr = argv[1];
	else
		nptr = "root";
	if ((pwd = getpwnam(nptr)) == NULL) {
		printf("Unknown id: %s\n", nptr);
		exit(1);
	}
	if (pwd->passwd[0] == '\0' || getuid() == 0)
		goto ok;
	password = getpass("Password:");
	if (strcmp(pwd->passwd, crypt(password, pwd->passwd)) != 0) {
		printf("Sorry\n");
		exit(2);
	}

ok:
	endpwent();
	setuid(pwd->uid | (pwd->gid << 8));
	if (pwd->shell && *pwd->shell)
		shell = pwd->shell;
	argvec[0] = "su";
	argvec[1] = 0;
	execv(shell, argvec);
	printf("No shell\n");
	exit(3);
}
