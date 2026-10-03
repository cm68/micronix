/*
 * time - time a command
 *
 * cmd/time/time.c
 *
 * Ported from the v7 time (extra/v7/usr/src/cmd/time.c).  It reports
 * the wall clock, the user CPU time and the system CPU time, the last
 * two from times(2).
 *
 * The units are what the simulator actually counts: real time is in
 * seconds from time(2), and the two CPU figures are the cycle counts
 * times(2) hands back, differenced across the child's run.  The v7
 * original printed all three in hh:mm:ss.cc; there is no such clock
 * here, so the raw numbers are shown.
 *
 * times(2) is not implemented on earlier kernels - it is still unimp in
 * the syscall table there - so the user/sys line is printed only when
 * times() succeeds, and the command otherwise reports the wall clock
 * alone.
 *
 * libc has no execvp - micronix has no PATH variable - so the child
 * is looked up the way make looks one up: ".", "/bin" and "/usr/bin"
 * in turn.  A name containing a slash is exec'd as-is.
 *
 * vim: tabstop=4 shiftwidth=4 noexpandtab:
 */

#include <stdio.h>
#include <sys/signal.h>
#include <sys/times.h>

long	time();
int	times();

main(argc, argv)
char *argv[];
{
	struct tms buffer, obuffer;
	long before, after;
	int status;
	int p;
	int times_ok;

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
	times_ok = (times(&obuffer) == 0);
	while (wait(&status) != p)
		;
	if (times_ok)
		times(&buffer);
	time(&after);
	if ((status & 0377) != 0)
		fprintf(stderr, "Command terminated abnormally.\n");
	fprintf(stderr, "\nreal %ldm%lds\n", (after - before) / 60,
	    (after - before) % 60);
	if (times_ok)
		fprintf(stderr, "user %ld sys %ld\n",
		    buffer.tms_cutime - obuffer.tms_cutime,
		    buffer.tms_cstime - obuffer.tms_cstime);
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
