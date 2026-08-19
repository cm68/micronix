/*
 * diff - driver and subroutines
 *
 * cmd/diff/diff.c
 *
 * Ported from 2.11BSD usr/src/bin/diff/diff.c.  The program names were
 * carried in from the 2.11 Makefile as -D strings; here they are plain
 * strings.  diffh is the halfhearted diff that -h execs, built in this
 * directory beside diff.
 */

#include "diff.h"

char	diff[] = "/bin/diff";
char	diffh[] = "/bin/diffh";
char	pr[] = "/bin/pr";

main(argc, argv)
	int argc;
	char **argv;
{
	register char *argp;

	ifdef1 = "FILE1"; ifdef2 = "FILE2";
	status = 2;
	diffargv = argv;
	argc--, argv++;
	while (argc > 2 && argv[0][0] == '-') {
		argp = &argv[0][1];
		argv++, argc--;
		while (*argp) switch(*argp++) {

#ifdef notdef
		case 'I':
			opt = D_IFDEF;
			wantelses = 0;
			continue;
		case 'E':
			opt = D_IFDEF;
			wantelses = 1;
			continue;
		case '1':
			opt = D_IFDEF;
			ifdef1 = argp;
			*--argp = 0;
			continue;
#endif
		case 'D':
			/* -Dfoo = -E -1 -2foo */
			wantelses = 1;
			ifdef1 = "";
			/* fall through */
#ifdef notdef
		case '2':
#endif
			opt = D_IFDEF;
			ifdef2 = argp;
			*--argp = 0;
			continue;
		case 'e':
			opt = D_EDIT;
			continue;
		case 'f':
			opt = D_REVERSE;
			continue;
		case 'n':
			opt = D_NREVERSE;
			continue;
		case 'b':
			bflag = 1;
			continue;
		case 'w':
			wflag = 1;
			continue;
		case 'i':
			iflag = 1;
			continue;
		case 't':
			tflag = 1;
			continue;
		case 'c':
			opt = D_CONTEXT;
			if (isdigit(*argp)) {
				context = atoi(argp);
				while (isdigit(*argp))
					argp++;
				if (*argp) {
					fprintf(stderr,
					    "diff: -c: bad count\n");
					done();
				}
				argp = "";
			} else
				context = 3;
			continue;
		case 'h':
			hflag = 1;
			continue;
		case 'S':
			if (*argp == 0) {
				fprintf(stderr, "diff: use -Sstart\n");
				done();
			}
			start = argp;
			*--argp = 0;		/* don't pass it on */
			continue;
		case 'r':
			rflag = 1;
			continue;
		case 's':
			sflag = 1;
			continue;
		case 'l':
			lflag = 1;
			continue;
		default:
			fprintf(stderr, "diff: -%s: unknown option\n",
			    --argp);
			done();
		}
	}
	if (argc != 2) {
		fprintf(stderr, "diff: two filename arguments required\n");
		done();
	}
	file1 = argv[0];
	file2 = argv[1];
	if (hflag && opt) {
		fprintf(stderr,
		    "diff: -h doesn't support -e, -f, -n, -c, or -I\n");
		done();
	}
	if (!strcmp(file1, "-"))
		stb1.st_mode = S_IFREG;
	else if (stat(file1, &stb1) < 0) {
		fprintf(stderr, "diff: ");
		perror(file1);
		done();
	}
	if (!strcmp(file2, "-"))
		stb2.st_mode = S_IFREG;
	else if (stat(file2, &stb2) < 0) {
		fprintf(stderr, "diff: ");
		perror(file2);
		done();
	}
	if ((stb1.st_mode & S_IFMT) == S_IFDIR &&
	    (stb2.st_mode & S_IFMT) == S_IFDIR) {
		diffdir(argv);
	} else
		diffreg();
	done();
}

char *
savestr(cp)
	register char *cp;
{
	register char *dp = malloc(strlen(cp)+1);

	if (dp == 0) {
		fprintf(stderr, "diff: ran out of memory\n");
		done();
	}
	strcpy(dp, cp);
	return (dp);
}

/*
 * min and max are macros in types.h, so diff has no functions under
 * those names; diffreg.c's dump_context() calls the macros.
 */
done()
{
	if (tempfile)
		unlink(tempfile);
	exit(status);
}

char *
talloc(n)
{
	register char *p;

	if ((p = malloc((unsigned)n)) != NULL)
		return(p);
	noroom();
}

char *
ralloc(p,n)
char *p;
{
	register char *q;
	char *realloc();

	if ((q = realloc(p, (unsigned)n)) == NULL)
		noroom();
	return(q);
}

noroom()
{
	fprintf(stderr, "diff: files too big, try -h\n");
	done();
}

/*
 * rindex - last occurrence of a character in a string, or 0 if there
 * is none.  Micronix's libc has neither rindex nor strrchr, so it
 * lives here; diffreg.c's splice() uses it to find the last component
 * of a pathname.
 */
char *
rindex(cp, c)
	char *cp;
	register int c;
{
	register char *p, *q;

	q = 0;
	for (p = cp; *p; p++)
		if (*p == c)
			q = p;
	return (q);
}
