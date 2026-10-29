/*
 * echo command
 *
 * cmd/echo/echo.c
 *
 * 2.11BSD echo (echo.c 4.2, Berkeley 5/27/85), ported to micronix.
 * The only change is the header: the program is the 2.11 source, so
 * -n is recognised only as exactly "-n" and not as a prefix.
 *
 * vim: tabstop=4 shiftwidth=4 noexpandtab:
 */
#include <stdio.h>

main(argc, argv)
int argc;
char *argv[];
{
	register int i, nflg;

	nflg = 0;
	if(argc > 1 && argv[1][0] == '-' && argv[1][1] == 'n' && !argv[1][2]) {
		nflg++;
		argc--;
		argv++;
	}
	for(i=1; i<argc; i++) {
		fputs(argv[i], stdout);
		if (i < argc-1)
			putchar(' ');
	}
	if(nflg == 0)
		putchar('\n');
	exit(0);
}
