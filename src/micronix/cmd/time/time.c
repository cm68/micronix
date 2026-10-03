/*
 * time - time a command
 *
 * cmd/time/time.c
 *
 * Ported from the v7 time (extra/v7/usr/src/cmd/time.c).  Only the
 * wall clock is reported: times(2) is implemented but not used here,
 * because a times(&struct) call trips a ccc code-generation bug that
 * computes the address of the struct as zero (see the man page).
 *
 * libc has no execvp - micronix has no PATH variable - so the child
 * is looked up the way make looks one up: ".", "/bin" and "/usr/bin"
 * in turn.  A name containing a slash is exec'd as-is.
 *
 * vim: tabstop=4 shiftwidth=4 noexpandtab:
 */

#include <stdio.h>
#include <sys/signal.h>

long	time();

main(argc, argv)
char *argv[];
{
	long before, after;
	int status;
	int p;

	if (argc <= 1)
		exit(0);
	time(&before);
	p = fork();
	if (p == -1) {
		fprintf(stderr, "time: cannot fork\n");
		exit(1);
	}
	if (p == 0) {
		execvp(argv[1], &argv[1]);
		fprintf(stderr, "%s: not found\n", argv[1]);
		exit(1);
	}
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	while (wait(&status) != p)
		;
	time(&after);
	if ((status & 0377) != 0)
		fprintf(stderr, "Command terminated abnormally.\n");
	fprintf(stderr, "\nreal %ldm%lds\n", (after - before) / 60,
	    (after - before) % 60);
	exit(status >> 8);
}

/*
 * Look a command up in the directories make searches, then exec it.
 */
execvp(name, argv)
char *name;
char **argv;
{
	static char *path[] = { ".", "/bin", "/usr/bin", 0 };
	char buf[128];
	int i;

	if (strchr(name, '/'))
		return execv(name, argv);
	for (i = 0; path[i]; i++) {
		strcpy(buf, path[i]);
		strcat(buf, "/");
		strcat(buf, name);
		if (access(buf, 1) == 0)
			execv(buf, argv);
	}
	return -1;
}
