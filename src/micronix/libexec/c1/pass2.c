/*
 * pass2.c - Code generator main
 */
#include "pass2.h"

/* counted in lower.c, one per shape the rules could not match */
extern int nincomplete;
extern int nbadcase;
#include "libutil.h"
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#ifdef DEBUG
#include <stdio.h>
#endif

#ifdef DEBUG
#include "dbgtags.c"
#endif

int in2fd;

#ifdef DEBUG
short verbose;
#endif

void
errout(char *buf)
{
	write(2, buf, strlen(buf));
}

static char *progname;

void
usage(char *complaint)
{
	errout(complaint);
	errout("usage: c1 [options] <.ast> <.dat> <out.s>\n");
#ifdef DEBUG
	errout("  -v <mask>    Set verbosity (hex bitmask)\n");
#ifdef __GNUC__
	{
		int i;
		for (i = 0; vopts[i]; i++) {
			char buf[64];
			sprintf(buf, "\t%x %s\n", 1 << i, vopts[i]);
			errout(buf);
		}
	}
#endif
#endif
	exit(1);
}

#ifdef CCC
/*
 * pass2 does all io on raw file descriptors too - see the note in
 * pass1.c for what the stdio flush machinery costs when exit() drags
 * it in.
 */
void
_cleanup(void)
{
}
#endif

int
main(int argc, char **argv)
{
	register char *s;

	progname = *argv++;
	argc--;


	/* Parse options */
	while (argc) {
		s = *argv;
		if (*s++ != '-')
			break;
		argv++;
		argc--;

		while (*s) {
			switch (*s++) {
			case 'h':
				usage("");
				break;
#ifdef DEBUG
			case 'v':
				if (!argc--)
					usage("verbosity not specified\n");
				verbose = strtol(*argv++, 0, 0);
				break;
#endif
			default:
				{
					char buf[64];
					fmtstr(buf, "bad flag %c\n", s[-1]);
					errout(buf);
				}
				break;
			}
		}
	}

	if (argc != 3) {
		usage("requires 3 file arguments\n");
	}

#ifdef DEBUG
#ifdef __GNUC__
	if (verbose) {
		int i, j = 0;
		char buf[128];

		for (i = 0; i < 32; i++) {
			if (!vopts[i])
				break;
			if (verbose & (1 << i))
				j |= (1 << i);
		}

		sprintf(buf, "verbose: %x (", j);
		errout(buf);
		for (i = 0; vopts[i]; i++) {
			if (j & (1 << i)) {
				errout(vopts[i]);
				j ^= (1 << i);
				if (j) {
					errout(" ");
				}
			}
		}
		errout(")\n");
	}
#endif
#endif

	infd = open(argv[0], O_RDONLY);
	if (infd < 0) {
		errout("cannot open AST file\n");
		exit(1);
	}

	nidopen(argv[0]);
	in2fd = open(argv[1], O_RDONLY);
	if (in2fd < 0) {
		errout("cannot open init file\n");
		exit(1);
	}

#ifdef linux
	outfd = creat(argv[2], 0600);	/* host: no group bits - the ls gitignore marker */
#else
	outfd = creat(argv[2], 0644);
#endif
	if (outfd < 0) {
		errout("cannot create output file\n");
		exit(1);
	}

	copyinit();
	parse();

#ifdef DEBUG
	dumphits();
#endif
	close(infd);
	close(in2fd);
	close(outfd);

	/*
	 * A shape the rules could not match left a comment in the
	 * output and nothing else - no instruction, and until now no
	 * complaint either.  Fail, so a build stops here rather than
	 * linking a program with a statement missing from it.
	 */
	/*
	 * A case value outside 0..255 cannot be dispatched at all: every
	 * shape the dispatch has compares eight bits, and a word control
	 * goes to the no-match label before the table is consulted, so
	 * the arm is unreachable.  Said here, while the source being
	 * compiled is still named, rather than left to the .error in the
	 * output - that only surfaces if the assembler runs, and a -s
	 * run never gets that far.
	 */
	if (nbadcase) {
		char nbuf[12];
		char *q = nbuf + 11;
		int n = nbadcase;

		*q = 0;
		do { *--q = '0' + n % 10; n /= 10; } while (n);
		errout(argv[2]);
		errout(": ");
		errout(q);
		errout(" switch case value(s) outside 0..255 - the dispatch"
		    " compares a byte, so those arms can never be reached\n");
		return 1;
	}
	if (nincomplete) {
		char nbuf[12];
		char *q = nbuf + 11;
		int n = nincomplete;

		*q = 0;
		do { *--q = '0' + n % 10; n /= 10; } while (n);
		errout(argv[2]);
		errout(": ");
		errout(q);
		errout(" expression(s) that no rule could build code for -"
		    " the output is missing them\n");
		return 1;
	}
	return 0;
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
