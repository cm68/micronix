/********************************************************/
/* COMPARE.C - compare two files			*/
/*	adapted from "Software Tools" page 73		*/
/*	06/06/82: change to allow multiple mismatches	*/
/*	10/15/83: increase buffer size for speed	*/
/*	01/28/84: use qprintf to reduce size		*/
/********************************************************/

#include <qstdio.h>

#define MAXLINE 132
#define BUFSIZE 1024

char line1[MAXLINE], line2[MAXLINE];
main(argc,argv)
int argc;
char *argv[];
	{
	register int lineno, m1, m2;
	register FILE *fp1, *fp2;
	char *buf1, *buf2;
	FILE *fopen();
	char *sbrk();
	static	diffcnt = 0,	/* difference count */
		diffmax = 1;	/* max number of differences */

	if (argc < 3) {
		qprintf("usage: compare file1 file2 [+diffmax]");
		exit(1);
		}
	if (argc > 3)
		diffmax = atoi(argv[3]);
	if ((fp1 = fopen(argv[1], "r")) == NULL) {
		qprintf("can't open:%s\n", argv[1]);
		exit(1);
		}
	if ((fp2 = fopen(argv[2], "r")) == NULL) {
		qprintf("can't open:%s\n", argv[2]);
		exit(1);
		}
	buf1 = sbrk(BUFSIZE);
	if ((int)(buf2 = sbrk(BUFSIZE)) == -1) {
		qprintf("Not enough space for buffers\n");
		exit(1);
		}
	setbuf(fp1, buf1);		/* supply large buffers */
	setbsize(fp1, BUFSIZE);
	setbuf(fp2, buf2);
	setbsize(fp2, BUFSIZE);
	lineno = 0;
	for (;;) {
		m1 = fgets(line1, MAXLINE, fp1);
		m2 = fgets(line2, MAXLINE, fp2);
		if (m1 == NULL || m2 == NULL)
			break;
		++lineno;
		if (strcmp(line1, line2)) {
			qprintf("%d:\n%s%s", lineno, line1, line2);
			if (++diffcnt >= diffmax)
				exit(0);
			}
		}
	if (m1 == NULL && m2 != NULL)
		qprintf("eof on %s\n", argv[1]);
	else if (m1 != NULL && m2 == NULL)
		qprintf("eof on %s\n", argv[2]);
	/* else they match */
	}
