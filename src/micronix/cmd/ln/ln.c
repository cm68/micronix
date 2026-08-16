/*
 * ln
 *
 * 2.11BSD ln (ln.c 4.6, 11/15/85), ported to micronix.
 *
 * cmd/ln/ln.c
 *
 * micronix has no symbolic links, so -s is refused with a message
 * rather than silently making a hard link a script did not want,
 * and the function-pointer machinery that chose between link and
 * symlink is gone with it.  Everything else - the target-directory
 * check, the tail splice, which error to print for whom - is as
 * Berkeley wrote it.
 *
 * vim: tabstop=8 shiftwidth=8 noexpandtab:
 */

#include <types.h>
#include <stdio.h>
#include <sys/fs.h>
#include <sys/stat.h>
#include <errno.h>

struct	stat stb;
int	fflag;		/* force flag set? */
char	name[BUFSIZ];
char	*rindex();
extern	int errno;

main(argc, argv)
	int argc;
	register char **argv;
{
	register int i, r;

	argc--, argv++;
	if (argc && strcmp(argv[0], "-f") == 0) {
		fflag++;
		argv++;
		argc--;
	}
	if (argc && strcmp(argv[0], "-s") == 0) {
		fprintf(stderr, "ln: micronix has no symbolic links\n");
		exit(1);
	}
	if (argc == 0)
		goto usage;
	else if (argc == 1) {
		argv[argc] = ".";
		argc++;
	}
	if (argc > 2) {
		if (stat(argv[argc-1], &stb) < 0)
			goto usage;
		if ((stb.st_mode&S_IFMT) != S_IFDIR)
			goto usage;
	}
	r = 0;
	for(i = 0; i < argc-1; i++)
		r |= linkit(argv[i], argv[argc-1]);
	exit(r);
usage:
	fprintf(stderr, "Usage: ln f1\nor: ln f1 f2\nln f1 ... fn d2\n");
	exit(1);
}

linkit(from, to)
	char *from, *to;
{
	char *tail;

	/* is target a directory? */
	if (fflag == 0 && stat(from, &stb) >= 0
	    && (stb.st_mode&S_IFMT) == S_IFDIR) {
		printf("%s is a directory\n", from);
		return (1);
	}
	if (stat(to, &stb) >= 0 && (stb.st_mode&S_IFMT) == S_IFDIR) {
		tail = rindex(from, '/');
		if (tail == 0)
			tail = from;
		else
			tail++;
		sprintf(name, "%s/%s", to, tail);
		to = name;
	}
	if (link(from, to) < 0) {
		if (errno == EEXIST)
			perror(to);
		else if (access(from, 0) < 0)
			perror(from);
		else
			perror(to);
		return (1);
	}
	return (0);
}
