
#include "files"

#ifndef TWOPASS
#include "dextern"

#define a amem
#define mem mem0
#define pa indgo
#define yypact temp1
#define greed tystate

int *ggreed = { lkst };
int *pgo = { wsets };
int *yypgo = { nontrst };

#endif

#ifdef TWOPASS

#define YYFLAG1 -1000
#define _memsize 4000
#define _actsize 2000
#define _nstates 475
#define _nnonterm 100

	/* languages */

#define C 0
#define RATFOR 1
#define EFL 2

FILE *finput;
FILE *foutput;
FILE *ftable;

int yypact[_nstates];
int yypgo[_nnonterm];
int mem[_memsize];
int nnonter;
int nstate;

int a[_actsize];				/* representation of the parser */
int pa[_nstates];				/* index into a */
int greed[_nstates];
int pgo[_nnonterm];
int ggreed[_nnonterm];
int lflag;

#endif

int maxspr;					/* maximum spread of any entry */
int maxoff;					/* maximum offset into a array */
int *pmem = {
    mem
};
int *maxa;
#define NOMORE -1000

int nxdb;
int adb;

#ifdef TWOPASS

char *ofiles[] = { FILEC, FILER, FILEE};

main(argc, argv)
char *argv[];
{

	register i, j;

	/* get the options, if any */

	foutput = NULL;

	if (argc > 1) {
		for (i = 0; j = argv[1][i]; ++i) {
			switch (j) {

			case 'c':
				lflag = C;
				continue;
			case 'r':
				lflag = RATFOR;
				continue;
			case 'e':
				lflag = EFL;
				continue;
			case 'v':
				if ((foutput = fopen(FILEU, "a")) == NULL)
					error("cannot open user output file");
				continue;

			}

		}
	}

	/* open the input and output files */

	if ((ftable = fopen(ofiles[lflag], "a")) == NULL)
		error("missing output file");

	callopt();
	exit(0);
}

#endif

callopt()
{

	register i, *p, j, k, *q;

	/* read the arrays from tempfile and set parameters */

	if ((finput = fopen(TEMPNAME, "r")) == NULL)
		error("optimizer cannot open tempfile");

	pgo[0] = 0;
	yypact[0] = 0;
	nstate = 0;
	nnonter = 0;
	for (;;) {
		switch (gtnm()) {

		case '\n':
			yypact[++nstate] = (--pmem) - mem;
		case ',':
			continue;

		case '$':
			break;

		default:
			error("bad tempfile");
		}
		break;
	}

	yypact[nstate] = yypgo[0] = (--pmem) - mem;

	for (;;) {
		switch (gtnm()) {

		case '\n':
			yypgo[++nnonter] = pmem - mem;
		case ',':
			continue;

		case EOF:
			break;

		default:
			error("bad tempfile");
		}
		break;
	}

	yypgo[nnonter--] = (--pmem) - mem;

	for (i = 0; i < nstate; ++i) {

		k = 32000;
		j = 0;
		q = mem + yypact[i + 1];
		for (p = mem + yypact[i]; p < q; p += 2) {
			if (*p > j)
				j = *p;
			if (*p < k)
				k = *p;
		}
		if (k <= j) {			/* nontrivial situation */
			/* temporarily, kill this for compatibility
			   j -= k;  /* j is now the range */
			if (k > maxoff)
				maxoff = k;
		}
		greed[i] = (yypact[i + 1] - yypact[i]) + 2 * j;
		if (j > maxspr)
			maxspr = j;
	}

	/* initialize ggreed table */

	for (i = 1; i <= nnonter; ++i) {
		ggreed[i] = 1;
		j = 0;
		/* minimum entry index is always 0 */
		q = mem + yypgo[i + 1] - 1;
		for (p = mem + yypgo[i]; p < q; p += 2) {
			ggreed[i] += 2;
			if (*p > j)
				j = *p;
		}
		ggreed[i] = ggreed[i] + 2 * j;
		if (j > maxoff)
			maxoff = j;
	}

	/* now, prepare to put the shift actions into the a array */

	for (i = 0; i < _actsize; ++i)
		a[i] = 0;
	maxa = a;

	for (i = 0; i < nstate; ++i) {
		if (greed[i] == 0 && adb > 1)
			fprintf(ftable, "State %d: null\n", i);
		pa[i] = YYFLAG1;
	}

	while ((i = nxti()) != NOMORE) {
		if (i >= 0)
			stin(i);
		else
			gin(-i);

	}

	if (adb > 2) {				/* print a array */
		for (p = a; p <= maxa; p += 10) {
			fprintf(ftable, "%4d  ", p - a);
			for (i = 0; i < 10; ++i)
				fprintf(ftable, "%4d  ", p[i]);
			fprintf(ftable, "\n");
		}
	}
	/* write out the output appropriate to the language */

	switch (lflag) {

	case C:
		coutput();
		break;

	case RATFOR:
		routput();
		break;

	case EFL:
		eoutput();
		break;

	}
	osummary();
	ZAPFILE(TEMPNAME);
}

gin(i)
{

	register *p, *r, *s, *q1, *q2;

	/* enter gotos on nonterminal i into array a */

	ggreed[i] = 0;

	q2 = mem + yypgo[i + 1] - 1;
	q1 = mem + yypgo[i];

	/* now, find a place for it */

	for (p = a; p < &a[_actsize]; ++p) {
		if (*p)
			continue;
		for (r = q1; r < q2; r += 2) {
			s = p + *r + 1;
			if (*s)
				goto nextgp;
			if (s > maxa) {
				if ((maxa = s) > &a[_actsize])
					error("a array overflow");
			}
		}
		/* we have found a spot */

		*p = *q2;
		if (p > maxa) {
			if ((maxa = p) > &a[_actsize])
				error("a array overflow");
		}
		for (r = q1; r < q2; r += 2) {
			s = p + *r + 1;
			*s = r[1];
		}

		pgo[i] = p - a;
		if (adb > 1)
			fprintf(ftable, "Nonterminal %d, entry at %d\n", i, pgo[i]);
		goto nextgi;

nextgp:;
	}

	error("cannot place goto %d\n", i);

nextgi:;
}

stin(i)
{
	register *r, *s, n, flag, j, *q1, *q2;

	greed[i] = 0;

	/* enter state i into the a array */

	q2 = mem + yypact[i + 1];
	q1 = mem + yypact[i];
	/* find an acceptable place */

	for (n = -maxoff; n < _actsize; ++n) {

		flag = 0;
		for (r = q1; r < q2; r += 2) {
			if ((s = *r + n + a) < a)
				goto nextn;
			if (*s == 0)
				++flag;
			else if (*s != r[1])
				goto nextn;
		}

		/* check that the position equals another only if the states are identical */

		for (j = 0; j < nstate; ++j) {
			if (pa[j] == n) {
				if (flag)
					goto nextn;	/* we have some disagreement */
				if (yypact[j + 1] + yypact[i] == yypact[j] + yypact[i + 1]) {
					/* states are equal */
					pa[i] = n;
					if (adb > 1)
						fprintf(ftable,
								"State %d: entry at %d equals state %d\n",
								i, n, j);
					return;
				}
				goto nextn;		/* we have some disagreement */
			}
		}

		for (r = q1; r < q2; r += 2) {
			if ((s = *r + n + a) >= &a[_actsize])
				error("out of space in optimizer a array");
			if (s > maxa)
				maxa = s;
			if (*s != 0 && *s != r[1])
				error("clobber of a array, pos'n %d, by %d", s - a, r[1]);
			*s = r[1];
		}
		pa[i] = n;
		if (adb > 1)
			fprintf(ftable, "State %d: entry at %d\n", i, pa[i]);
		return;

nextn:	;
	}

	error("Error; failure to place state %d\n", i);

}

#ifdef TWOPASS

#ifdef lint
%varargs
#endif
error(s, x, y)
char *s;
{

	fprintf(stderr, "Fatal error: ");
	fprintf(stderr, s, x, y);
	fprintf(stderr, "\n");
	exit(1);
}
#endif

nxti()
{								/* finds the next i */
	register i, Max, maxi;

	Max = 0;

	for (i = 1; i <= nnonter; ++i)
		if (ggreed[i] >= Max) {
			Max = ggreed[i];
			maxi = -i;
		}

	for (i = 0; i < nstate; ++i)
		if (greed[i] >= Max) {
			Max = greed[i];
			maxi = i;
		}

	if (nxdb)
		fprintf(ftable, "nxti = %d, Max = %d\n", maxi, Max);
	if (Max == 0)
		return (NOMORE);
	else
		return (maxi);
}

osummary()
{
	/* write summary */

	register i, *p;

	if (foutput == NULL)
		return;
	i = 0;
	for (p = maxa; p >= a; --p) {
		if (*p == 0)
			++i;
	}

	fprintf(foutput, "Optimizer space used: input %d/%d, output %d/%d\n",
			pmem - mem + 1, _memsize, maxa - a + 1, _actsize);
	fprintf(foutput, "%d table entries, %d zero\n", (maxa - a) + 1, i);
	fprintf(foutput, "maximum spread: %d, maximum offset: %d\n", maxspr,
			maxoff);

}

coutput()
{								/* this version is for C */

	/* write out the optimized parser */

	fprintf(ftable, "#\nextern int yychar;\n\n");
	fprintf(ftable, "int yylast %d;\n", maxa - a + 1);

	arout("yyact", a, (maxa - a) + 1);
	arout("yypact", pa, nstate);
	arout("yypgo", pgo, nnonter + 1);

}

arout(s, v, n)
char *s;
int *v, n;
{

	register i;

	fprintf(ftable, "%s[]{\n", s);
	for (i = 0; i < n;) {
		if (i % 10 == 0)
			fprintf(ftable, "\n");
		fprintf(ftable, "%4d", v[i]);
		if (++i == n)
			fprintf(ftable, " };\n");
		else
			fprintf(ftable, ",");
	}
}

gtnm()
{

	register s, val, c;

	/* read and convert an integer from the standard input */
	/* return the terminating character */
	/* blanks, tabs, and newlines are ignored */

	s = 1;
	val = 0;

	while ((c = fgetc(finput)) != EOF) {
		if (isdigit(c)) {
			val = val * 10 + c - '0';
		} else if (c == '-')
			s = -1;
		else
			break;
	}

	*pmem++ = s * val;
	if (pmem > &mem[_memsize])
		error("out of space");
	return (c);

}

routput()
{								/* produce ratfor output (groan! ); */

	fprintf(ftable, "define YYERROR goto 975\n");
	fprintf(ftable, "define YYABORT return(1)\n");
	fprintf(ftable, "define YYACCEPT return(0)\n");

	fprintf(ftable, "integer function yypars( yyargu )\n");
	fprintf(ftable, "define YYLAST %d\n", (maxa - a) + 1);
	fprintf(ftable, "common /yycomn/ yylval, yyval, yypv, yyvalv(150)\n");
	fprintf(ftable, "common /yylcom/ yychar, yyerrf, yydebu\n");
	fprintf(ftable,
			"integer yyval, yypv, yylval, yychar, yydebu, yynerr, yystat, yyargu\n");
	fprintf(ftable,
			"integer yylex,yyexcp,yys(150),yyerrf,yyvalv,yyn,yyj,yypvt,yym\n");

	pcom("yyact", (maxa - a) + 1);
	pcom("yypact", nstate);
	pcom("yypgo", nnonter + 1);
	fprintf(ftable, "integer yyr1(YYNPROD)\n");
	fprintf(ftable, "integer yyr2(YYNPROD)\n");
	pcom("yychk", nstate);
	pcom("yydef", nstate);

	farout("yyact", a, (maxa - a) + 1);
	farout("yypact", pa, nstate);
	farout("yypgo", pgo, nnonter + 1);

}

farout(s, v, n)
char *s;
int *v, n;
{

	/* output the array v, size n, as array s, in fortran */

	register i;

	for (i = 1; i <= n; ++i) {

		fprintf(ftable, "      data %s(%d)/%d/;\n", s, i, v[i - 1]);

	}

}

eoutput()
{								/* produce efl output (semi-groan! ); */

	fprintf(ftable, "define YYERROR = goto yyerrlab\n");
	fprintf(ftable, "define YYABORT = return(1)\n");
	fprintf(ftable, "define YYACCEPT = return(0)\n");

	fprintf(ftable, "integer procedure yypars(yyargu)\n\n");
	fprintf(ftable, "define YYLAST = %d\n\n", (maxa - a) + 1);
	fprintf(ftable,
			"common (yycomn) yylval, yyval, yypv, yyvalv(YYSTKSIZE)\n");
	fprintf(ftable, "integer         yylval, yyval, yypv, yyvalv\n\n");
	fprintf(ftable, "common (yylcom) yychar, yyerrf, yydebu\n");
	fprintf(ftable, "integer         yychar, yyerrf, yydebu\n\n");
	fprintf(ftable, "integer yynerr, yystat, yyargu\n");
	fprintf(ftable,
			"integer yylex, yyexcp, yys(YYSTKSIZE), yyn, yyj, yym\n\n");

	pcom("yyact", (maxa - a) + 1);
	pcom("yypact", nstate);
	pcom("yypgo", nnonter + 1);
	fprintf(ftable, "integer yyr1(YYNPROD)\n");
	fprintf(ftable, "integer yyr2(YYNPROD)\n");
	pcom("yychk", nstate);
	pcom("yydef", nstate);

	earout("yyact", a, (maxa - a) + 1);
	earout("yypact", pa, nstate);
	earout("yypgo", pgo, nnonter + 1);

}

earout(s, v, n)
char *s;
int *v, n;
{

	/* output the array v, size n, as array s, in efl */

	register i;

	fprintf(ftable, "\ninitial %s = (", s);

	for (i = 0; i < n;) {
		if (i % 10 == 0)
			fprintf(ftable, "\n\t");
		fprintf(ftable, "%5d", v[i]);
		if (++i == n)
			fprintf(ftable, " )\n\n");
		else
			fprintf(ftable, ",");
	}

}

pcom(s, n)
char *s;
{

	fprintf(ftable, "integer %s(%d)\n", s, n);

}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
