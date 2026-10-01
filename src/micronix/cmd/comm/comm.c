/*
 * comm [-123] file1 [file2]
 *
 * v7 comm (usr/src/cmd/comm.c), ported to micronix.
 *
 * cmd/comm/comm.c
 *
 * The comparison is v7's, unchanged: two line buffers of 256 bytes,
 * compare() so long as the lines agree, and one line read ahead of
 * each file, so the columns come out in order with no buffering of
 * a whole file.  The column labels in ldr are shortened as the flags
 * come in, which is why -1 prints column 2 without its leading tab.
 *
 * What the port had to say differently is only the second file.  v7
 * wanted both files named - "comm: arg count" was what it said for
 * one - and the micronix command comm (1) documents file2 as
 * optional, with the standard input read in its place, so a missing
 * second argument becomes stdin below.  A file named - was already
 * the standard input in v7's openfil, and still is.
 *
 * The 1982 micronix /bin/comm on the 1.6 disk is not this program.
 * It calls itself common, takes a -wN flag beside the three column
 * flags, and its output does not match comm (1): it drops the lines
 * unique to the second file, which for "comm empty file" is the whole
 * of the answer.  The page, and so this port, is v7's behaviour.
 *
 * vim: tabstop=8 shiftwidth=8 noexpandtab:
 */

#include <types.h>
#include <stdio.h>

#define LB 256
int	one;
int	two;
int	three;

char	*ldr[3];

FILE *ib1;
FILE *ib2;
FILE *openfil();

main(argc, argv)
	int argc;
	char *argv[];
{
	int	l;
	char	lb1[LB],lb2[LB];

	ldr[0] = "";
	ldr[1] = "\t";
	ldr[2] = "\t\t";
	if(argc > 1)  {
		if(*argv[1] == '-' && argv[1][1] != 0) {
			l = 1;
			while(*++argv[1]) {
				switch(*argv[1]) {
				case'1':
					if(!one) {
						one = 1;
						ldr[1][0] =
						ldr[2][l--] = '\0';
					}
					break;
				case '2':
					if(!two) {
						two = 1;
						ldr[2][l--] = '\0';
					}
					break;
				case '3':
					three = 1;
					break;
				default:
				fprintf(stderr,"comm: illegal flag\n");
				exit(1);
				}
			}
			argv++;
			argc--;
		}
	}

	if(argc < 2) {
		fprintf(stderr,"comm: arg count\n");
		exit(1);
	}

	ib1 = openfil(argv[1]);
	ib2 = argc > 2 ? openfil(argv[2]) : stdin;


	if(rd(ib1,lb1) < 0) {
		if(rd(ib2,lb2) < 0)	exit(0);
		copy(ib2,lb2,2);
	}
	if(rd(ib2,lb2) < 0)	copy(ib1,lb1,1);

	while(1) {

		switch(compare(lb1,lb2)) {

			case 0:
				wr(lb1,3);
				if(rd(ib1,lb1) < 0) {
					if(rd(ib2,lb2) < 0)	exit(0);
					copy(ib2,lb2,2);
				}
				if(rd(ib2,lb2) < 0)	copy(ib1,lb1,1);
				continue;

			case 1:
				wr(lb1,1);
				if(rd(ib1,lb1) < 0)	copy(ib2,lb2,2);
				continue;

			case 2:
				wr(lb2,2);
				if(rd(ib2,lb2) < 0)	copy(ib1,lb1,1);
				continue;
		}
	}
}

rd(file,buf)
	FILE *file;
	char *buf;
{

	register int i, c;
	i = 0;
	while((c = getc(file)) != EOF) {
		*buf = c;
		if(c == '\n' || i > LB-2) {
			*buf = '\0';
			return(0);
		}
		i++;
		buf++;
	}
	return(-1);
}

wr(str,n)
	char	*str;
{

	switch(n) {

		case 1:
			if(one)	return;
			break;

		case 2:
			if(two)	return;
			break;

		case 3:
			if(three)	return;
	}
	printf("%s%s\n",ldr[n-1],str);
}

copy(ibuf,lbuf,n)
	FILE *ibuf;
	char *lbuf;
{
	do {
		wr(lbuf,n);
	} while(rd(ibuf,lbuf) >= 0);

	exit(0);
}

compare(a,b)
	char	*a,*b;
{
	register char *ra,*rb;

	ra = --a;
	rb = --b;
	while(*++ra == *++rb)
		if(*ra == '\0')	return(0);
	if(*ra < *rb)	return(1);
	return(2);
}

FILE *openfil(s)
	char *s;
{
	FILE *b;
	if(s[0]=='-' && s[1]==0)
		b = stdin;
	else if((b=fopen(s,"r")) == NULL) {
		fprintf(stderr,"comm: cannot open %s\n",s);
		exit(1);
	}
	return(b);
}
