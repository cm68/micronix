/*
 * translit - transliterate characters
 *
 * cmd/translit/translit.c
 *
 * A filter: each character of the standard input is mapped to the
 * character at the same position in the "to" set and written, or
 * dropped when there is no "to" set.  A set is written with ranges
 * a-z, octal @377, and a leading ^ for "everything not named".  When
 * several input characters map to one output character, runs of that
 * output character collapse to one - what the page describes and tr
 * calls -s.  translit is Micronix's tr.
 *
 * vim: tabstop=4 shiftwidth=4 noexpandtab:
 */

#include <stdio.h>

#define	digit(c) ((c) >= '0' && (c) <= '9')

/*
 * The sets and the table are file-scope, not local: a local array's
 * address is a frame offset that c1 cannot hand to a function - see
 * the ccc porting gotchas.
 */
unsigned char	fset[256], tset[256], src[256];
unsigned char	table[256];	/* char -> translated char */
unsigned char	squeeze[256];	/* collapse runs of these outputs */
unsigned char	drop[256];	/* delete these inputs */
int		tcount[256];	/* inputs mapping to each output */

/*
 * Expand a set specification into set[], returning the count.  ^ as
 * the first character sets *complement instead of adding anything.
 */
static void
parse(s, set, n, complement)
	char *s;
	unsigned char *set;
	int *n;
	int *complement;
{
	int start, end, i, v, c, k;

	*complement = 0;
	if (s[0] == '^') {
		*complement = 1;
		s++;
	}
	/*
	 * A local index, not set[(*n)++]: c1 cannot build the
	 * deref-then-postincrement shape that writes back through n.
	 */
	k = 0;
	for (; *s != '\0'; s++) {
		if (s[0] == '@' && digit(s[1])) {
			v = 0;
			s++;
			for (c = 0; digit(*s) && c < 3; c++) {
				v = v * 8 + (*s - '0');
				s++;
			}
			s--;
			set[k++] = v & 0xff;
		} else if (s[1] == '-' && s[2] != '\0') {
			start = s[0];
			end = s[2];
			if (start <= end)
				for (i = start; i <= end; i++)
					set[k++] = i;
			else
				for (i = start; i >= end; i--)
					set[k++] = i;
			s += 2;
		} else
			set[k++] = s[0];
	}
	*n = k;
}

main(argc, argv)
	int argc;
	char **argv;
{
	int nf, nt, ns, comp, dcomp;
	int i, j, c, prev;

	if (argc < 2 || argc > 3) {
		fprintf(stderr, "usage: translit from [to]\n");
		exit(1);
	}
	parse(argv[1], fset, &nf, &comp);
	if (argc == 3)
		parse(argv[2], tset, &nt, &dcomp);
	else
		nt = 0;

	for (i = 0; i < 256; i++)
		table[i] = i;
	for (i = 0; i < 256; i++)
		drop[i] = squeeze[i] = 0;

	/* the source set: what from names, or its complement */
	ns = 0;
	if (comp) {
		for (c = 0; c < 256; c++) {
			for (j = 0; j < nf; j++)
				if (fset[j] == c)
					break;
			if (j == nf)
				src[ns++] = c;
		}
	} else {
		for (j = 0; j < nf; j++)
			src[ns++] = fset[j];
	}

	if (nt == 0) {
		for (j = 0; j < ns; j++)
			drop[src[j]] = 1;
	} else {
		for (i = 0; i < 256; i++)
			tcount[i] = 0;
		for (j = 0; j < ns; j++) {
			table[src[j]] = tset[j < nt ? j : nt - 1];
			tcount[table[src[j]]]++;
		}
		for (i = 0; i < 256; i++)
			if (tcount[i] > 1)
				squeeze[i] = 1;
	}

	prev = -1;
	while ((c = getchar()) != EOF) {
		c &= 0xff;
		if (drop[c])
			continue;
		c = table[c];
		if (squeeze[c] && c == prev)
			continue;
		putchar(c);
		prev = c;
	}
	exit(0);
}
