/*
 * chg - the media changer
 *
 * cmd/chg/chg.c
 *
 * Drives /dev/mediachg, the out-of-band media changer port
 * (sys/MEDIACHG.md).  chg joins its arguments into one command line,
 * writes it to /dev/mediachg with the '@' leadin and newline the port
 * requires, and prints the response line.  It exits 0 on ok and 1 on
 * error, so a script can tell a load that failed.
 *
 * vim: tabstop=4 shiftwidth=4 noexpandtab:
 */

#include <stdio.h>

#define DEV	"/dev/mediachg"
#define MAXL	128

main(argc, argv)
	int argc;
	char **argv;
{
	char	line[MAXL];
	char	rbuf[MAXL];
	int	fd, i, len, rn;
	char	*a;

	if (argc < 2) {
		fprintf(stderr,
		    "usage: chg load <set>:<vol> onto <drive> | unload <drive> |\n");
		fprintf(stderr, "       query <set|drive> | next <drive>\n");
		exit(1);
	}

	/* the command line: '@' leadin, argv joined by spaces, '\n' end */
	len = 0;
	line[len++] = '@';
	for (i = 1; i < argc; i++) {
		if (i > 1)
			line[len++] = ' ';
		for (a = argv[i]; *a; a++)
			line[len++] = *a;
	}
	line[len++] = '\n';

	fd = open(DEV, 2);
	if (fd < 0) {
		fprintf(stderr, "chg: no %s\n", DEV);
		exit(1);
	}
	write(fd, line, len);

	/* the response is one newline-terminated line */
	rn = 0;
	for (;;) {
		if (rn >= MAXL - 1)
			break;
		if (read(fd, &rbuf[rn], 1) != 1)
			break;
		if (rbuf[rn] == '\n')
			break;
		rn++;
	}
	rbuf[rn] = '\0';
	close(fd);

	printf("%s\n", rbuf);
	if (rn >= 2 && rbuf[0] == 'o' && rbuf[1] == 'k')
		exit(0);
	exit(1);
}
