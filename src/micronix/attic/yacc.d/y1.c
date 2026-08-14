#include "dextern"
#include "files"

/*     * * * *    y a c c     * * * *     */

/*      features to be fixed up ...
***  provide the ability to change table sizes dynamically
***  Make the output line numbers agree with the input line numbers
***  Engage in newspeak: production=>grammar rules, term=>token, etc.
***  handle # define, #ifdef, etc., in yacc actions, %{ %}
***  say ACCEPT, not "shift 8191", in S/R message
*/

/*      new features to be added
***  reductions by single productions ( by request )
***  follow sets for start symbol
***  option to only do slr(1)
***  easily changed array names on output
***  input controlled by a grammar
***  support multiple choices for  conflicts
***  better conflict diagnostics
*/

main(argc, argv)
int argc;
char *argv[];
{
    /* initialize and read productions */
	setup(argc, argv);
	tbitset = NWORDS(nterms);

	/* make table of which productions yield a given nonterminal */
	cpres();

	/* make a table of which nonterminals can match the empty string */
	cempty();

	/* make a table of firsts of nonterminals */
	cpfir();

	/* generate the states */
	stagen();

	/* write the states and the tables */
	output();

	go2out();
	hideprod();
	summary();
	callopt();
	others();
	exit(0);
}

others()
{								/* put out other arrays, copy the parsers */

	register c, i, j;

	/* this outputs the other arrays, after the
	   /* size of the action array is known (this is needed for
	   /* ratfor... */

	/* this routine uses prdptr, trmset, tstates, mstates, ntstates,
	   defact, levprd, cnames

	   /* the other space can have been clobbered for use of the optimizer */

	finput = fopen(parser[lflag], "r");
	if (finput == NULL)
		error("cannot find parser %s", parser[lflag]);

	(*warray[lflag]) ("yyr1", levprd, nprod);

	aryfil(temp1, nprod, 0);
	PLOOP(1, i) temp1[i] = prdptr[i + 1] - prdptr[i] - 2;
	(*warray[lflag]) ("yyr2", temp1, nprod);

	aryfil(temp1, nstate, -1000);
	TLOOP(i) {
		for (j = tstates[i]; j != 0; j = mstates[j]) {
			temp1[j] = trmset[i].value;
		}
	}
	NTLOOP(i) {
		for (j = ntstates[i]; j != 0; j = mstates[j]) {
			temp1[j] = -i;
		}
	}
	(*warray[lflag]) ("yychk", temp1, nstate);

	(*warray[lflag]) ("yydef", defact, nstate);

	/* copy parser text */

	while ((c = fgetc(finput)) != EOF) {
		if (c == '$') {
			if ((c = fgetc(finput)) != 'A') {
				fputc('$', ftable);
			} else {				/* copy actions */
				faction = fopen(ACTNAME, "r");
				if (faction == NULL)
					error("cannot reopen action tempfile");
				while ((c = fgetc(faction)) != EOF)
					fputc(c, ftable);
				fclose(faction);
				ZAPFILE(ACTNAME);
				c = fgetc(finput);
			}
		}
		fputc(c, ftable);
	}

	fclose(ftable);
}

char *
chcopy(p, q)
char *p, *q;
{
	/* copies string q into p, returning next free char ptr */
	while (*p = *q++)
		++p;
	return (p);
}

#define ISIZE 400
char *
writem(pp)
int *pp;
{								/* creates output string for item pointed to by pp */
	int i, *p;
	static char sarr[ISIZE];
	char *q;

	for (p = pp; *p > 0; ++p);
	p = prdptr[-*p];
	q = chcopy(sarr, nontrst[*p - NTBASE].name);
	q = chcopy(q, " : ");

	for (;;) {
		*q++ = ++p == pp ? '_' : ' ';
		*q = '\0';
		if ((i = *p) <= 0)
			break;
		q = chcopy(q, symnam(i));
		if (q > &sarr[ISIZE - 30])
			error("item too big");
	}

	if ((i = *pp) < 0) {		/* an item calling for a reduction */
		q = chcopy(q, "    (");
		sprintf(q, "%d)", -i);
	}

	return (sarr);
}

char *
symnam(i)
{								/* return a pointer to the name of symbol i */
	char *cp;

	cp = (i >= NTBASE) ? nontrst[i - NTBASE].name : trmset[i].name;
	if (*cp == ' ')
		++cp;
	return (cp);
}

summary()
{								/* output the summary on the tty */

	if (foutput != NULL) {
		fprintf(foutput, "\n%d/%d terminals, %d/%d nonterminals\n", nterms,
				tlim, nnonter, ntlim);
		fprintf(foutput, "%d/%d grammar rules, %d/%d states\n", nprod,
				prdlim, nstate, stsize);
		fprintf(foutput,
				"%d shift/reduce, %d reduce/reduce conflicts reported\n",
				zzsrconf, zzrrconf);
		fprintf(foutput, "%d/%d working sets used\n", zzcwp - wsets,
				wssize);
		fprintf(foutput, "memory: states,etc. %d/%d, parser %d/%d\n",
				zzmemsz, memsiz, memp - amem, actsiz);
		fprintf(foutput, "%d/%d distinct lookahead sets\n", nlset, lsetsz);
		fprintf(foutput, "%d extra closures\n", zzclose - 2 * nstate);
		fprintf(foutput, "%d shift entries, %d exceptions\n", zzacent,
				zzexcp);
		fprintf(foutput, "%d goto entries\n", zzgoent);
		fprintf(foutput, "%d entries saved by goto default\n", zzgobest);
	}
	if (zzsrconf != 0 || zzrrconf != 0) {
		fprintf(stdout, "\nconflicts: ");
		if (zzsrconf)
			fprintf(stdout, "%d shift/reduce", zzsrconf);
		if (zzsrconf && zzrrconf)
			fprintf(stdout, ", ");
		if (zzrrconf)
			fprintf(stdout, "%d reduce/reduce", zzrrconf);
		fprintf(stdout, "\n");
	}

	fclose(ftemp);
	if (fdefine != NULL)
		fclose(fdefine);
}

/* VARARGS */
error(s, a1)
{								/* write out error comment */

	++nerrors;
	fprintf(stderr, "\n fatal error: ");
	fprintf(stderr, s, a1);
	fprintf(stderr, ", line %d\n", lineno);
	if (!fatfl)
		return;
	summary();
	exit(1);
}

int *
yalloc(n)
{								/* allocate n+1 words from vector mem */
	int *omem;
	omem = mem;
	mem += n + 1;
	if (mem - mem0 >= memsiz)
		error("memory overflow");
	return (omem);
}

aryfil(v, n, c)
int *v, n, c;
{								/* set elements 0 through n-1 to c */
	int i;
	for (i = 0; i < n; ++i)
		v[i] = c;
}

setunion(a, b)
register *a, *b;
{
	/* set a to the union of a and b */
	/* return 1 if b is not a subset of a, 0 otherwise */
	register i, x, sub;

	sub = 0;
	SETLOOP(i) {
		*a = (x = *a) | *b++;
		if (*a++ != x)
			sub = 1;
	}
	return (sub);
}

prlook(lp)
struct looksets *lp;
{
    int *pp;
	int j;
	pp = lp->lset;
	if (pp == 0) {
		fprintf(foutput, "\tNULL");
	} else {
		fprintf(foutput, " { ");
		TLOOP(j) {
			if (BIT(pp, j))
				fprintf(foutput, "%s ", symnam(j));
		}
		fprintf(foutput, "}");
	}
}

concat(s, t, u)
char *s, *t, *u;
{
	/* copies the concatenation of t and u into s */
	while (*t)
		*s++ = *t++;
	while (*s++ = *u++);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */

