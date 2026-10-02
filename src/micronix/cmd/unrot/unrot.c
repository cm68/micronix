/*
 * unrot - un-rotate kwic output for display
 *
 * cmd/unrot/unrot.c
 *
 * The Software Tools unrot (Kernighan & Plauger, "Software Tools",
 * 1976), reconstructed from the distribution binary (usr/bin/unrot,
 * dated 3/18/82).  It is the display companion to kwic: kwic emits one
 * line per keyword, rotated so the keyword leads and a $ marks the
 * fold; unrot reads those lines and folds each back at the $, printing
 * the preceding context (after the $) right-justified in columns 1-39,
 * the keyword with its following context (before the $) in columns 41
 * on, and column 40 blank - the "blank column down the middle" the page
 * names.  A line with no $ folds at the end, the whole line the head.
 *
 * The fold is circular, the way the original's fixed 79-column buffer
 * behaved: a half longer than 39 columns wraps around to the other end
 * of the line.  Trailing blanks are not printed.
 *
 * vim: tabstop=4 shiftwidth=4 noexpandtab:
 */
#include <stdio.h>

#define	WIDTH	79		/* output line, columns 1..79 */
#define	HALF	39		/* left field, columns 1..39 */
#define	BLANK	1		/* column 40 */

main()
{
	char line[1024];
	char out[WIDTH];
	char *dollar, *end, *tail;
	int i, j, lh, lt, right;

	while (fgets(line, sizeof line, stdin) != NULL) {
		dollar = line;
		while (*dollar && *dollar != '$' && *dollar != '\n')
			dollar++;
		end = dollar;
		if (*dollar == '$')
			end = dollar + 1;
		while (*end && *end != '\n')
			end++;
		/* head is line..dollar, tail is dollar+1..end (or empty) */
		lh = dollar - line;
		tail = (*dollar == '$') ? dollar + 1 : end;
		lt = end - tail;

		for (i = 0; i < WIDTH; i++)
			out[i] = ' ';

		/* head left-justified, its first char in column HALF+BLANK+1 */
		j = HALF + BLANK;
		for (i = 0; i < lh; i++) {
			int k = j + i;

			while (k >= WIDTH)
				k -= WIDTH;
			out[k] = line[i];
		}

		/* tail right-justified, its last char in column HALF (39);
		 * placed after head so it wins where the two halves wrap
		 * over each other */
		j = HALF - lt;
		for (i = 0; i < lt; i++) {
			int k = j + i;

			while (k < 0)
				k += WIDTH;
			out[k] = tail[i];
		}

		/* print up to the rightmost non-blank */
		for (right = WIDTH - 1; right >= 0 && out[right] == ' '; right--)
			;
		for (i = 0; i <= right; i++)
			putchar(out[i]);
		putchar('\n');
	}
	return 0;
}
