/* Copyright (c) 1979 Regents of the University of California */

/*
 * Strings - extract strings from an object file for whatever
 *
 * Bill Joy UCB
 * April 22, 1978
 *
 * The algorithm is to look for sequences of "non-junk" characters
 * The variable "minlen" is the minimum length string printed.
 * This helps get rid of garbage.
 * Default minimum string length is 4 characters.
 *
 * 2BSD strings, ported to micronix.
 *
 * cmd/strings/strings.c
 *
 * The original read a VAX a.out header and, unless told -a, skipped
 * the text and searched only the initialized data, because that is
 * where THAT compiler kept a program's strings.  This one keeps its
 * literals in text, so the assumption arrives inverted: the segment
 * worth searching is text and data together, and what deserves
 * skipping is the header in front and the symbol table behind -
 * which is still the same idea, "read the program, not the
 * bookkeeping".  -a searches the whole file, symbols and all,
 * exactly as it always did.
 *
 * A dead global came out (offset, declared and never touched), and
 * the flags are char by the range audit's law: tested and stored,
 * never done arithmetic on.  minlength stays int - it is the
 * user's -# digits, unclamped.
 *
 * vim: tabstop=8 shiftwidth=8 noexpandtab:
 */

#include <stdio.h>
#include <obj.h>
#include <ctype.h>

long	ftell();

struct	obj header;

char	*infile = "Standard input";
char	oflg;
char	asdata;
int	minlength = 4;

main(argc, argv)
	int argc;
	char *argv[];
{

	argc--, argv++;
	while (argc > 0 && argv[0][0] == '-') {
		register int i;
		if (argv[0][1] == 0)
			asdata++;
		else for (i = 1; argv[0][i] != 0; i++) switch (argv[0][i]) {

		case 'o':
			oflg++;
			break;

		case 'a':
			asdata++;
			break;

		default:
			if (!isdigit(argv[0][i])) {
				fprintf(stderr, "Usage: strings [ - ] [ -o ] [ -# ] [ file ... ]\n");
				exit(1);
			}
			minlength = argv[0][i] - '0';
			for (i++; isdigit(argv[0][i]); i++)
				minlength = minlength * 10 + argv[0][i] - '0';
			i--;
			break;
		}
		argc--, argv++;
	}
	do {
		if (argc > 0) {
			if (freopen(argv[0], "r", stdin) == NULL) {
				perror(argv[0]);
				exit(1);
			}
			infile = argv[0];
			argc--, argv++;
		}
		fseek(stdin, (long) 0, 0);
		if (asdata ||
		    fread((char *)&header, sizeof header, 1, stdin) != 1 ||
		    header.ident != OBJECT) {
			fseek(stdin, (long) 0, 0);
			find((long) 100000000L);
			continue;
		}
		fseek(stdin, (long) sizeof header, 0);
		find((long) header.text + header.data);
	} while (argc > 0);
}

find(cnt)
	long cnt;
{
	static char buf[BUFSIZ];
	register char *cp;
	register int c, cc;

	cp = buf, cc = 0;
	for (; cnt != 0; cnt--) {
		c = getc(stdin);
		if (c == '\n' || dirt(c) || cnt == 0) {
			if (cp > buf && cp[-1] == '\n')
				--cp;
			*cp++ = 0;
			if (cp > &buf[minlength]) {
				if (oflg)
					printf("%7D ", ftell(stdin) - cc - 1);
				printf("%s\n", buf);
			}
			cp = buf, cc = 0;
		} else {
			if (cp < &buf[sizeof buf - 2])
				*cp++ = c;
			cc++;
		}
		if (ferror(stdin) || feof(stdin))
			break;
	}
}

dirt(c)
	int c;
{

	switch (c) {

	case '\n':
	case '\f':
		return (0);

	case 0177:
		return (1);

	default:
		return (c > 0200 || c < ' ');
	}
}
