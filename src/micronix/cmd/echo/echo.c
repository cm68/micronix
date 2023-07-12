/*
 * echo command from v7
 *
 * cmd/echo/echo.c
 *
 * Changed: <2023-06-23 16:51:21 curt>
 *
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
#include <stdio.h>

main(argc, argv)
int argc;
char *argv[];
{
	register int i, nflg;

	nflg = 0;
	if(argc > 1 && argv[1][0] == '-' && argv[1][1] == 'n') {
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
