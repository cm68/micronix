#include "dextern"
#include "files"
#define IDENTIFIER 257
#define MARK 258
#define TERM 259
#define LEFT 260
#define RIGHT 261
#define BINARY 262
#define PREC 263
#define LCURLY 264
#define C_IDENTIFIER 265		/* name followed by colon */
#define NUMBER 266
#define ENDFILE 0

setup(argc, argv)
int argc;
char *argv[];
{
	int i, j, lev, t;
	int c;
	int *p;
	char actname[8];

	foutput = NULL;
	fdefine = NULL;
	i = 1;
	while (argc >= 2 && argv[1][0] == '-') {
		while (*++(argv[1])) {
			switch (*argv[1]) {
			case 'v':
			case 'V':
				foutput = fopen(FILEU, "w");
				if (foutput == NULL)
					error("cannot open y.output");
				continue;
			case 'D':
			case 'd':
				fdefine = fopen(FILED, "w");
				continue;
			case 'o':
			case 'O':
				fprintf(stderr, "`o' flag now default in yacc\n");
				continue;
			case 'r':
			case 'R':
				lflag = RATFOR;
				continue;
			case 'e':
			case 'E':
				lflag = EFL;
				continue;
			default:
				error("illegal option: %c", *argv[1]);
			}
		}
		argv++;
		argc--;
	}

	ftable = fopen(ofiles[lflag], "w");
	if (ftable == NULL)
		error("cannot open table file");

	ftemp = fopen(TEMPNAME, "w");
	faction = fopen(ACTNAME, "w");
	if (ftemp == NULL || faction == NULL)
		error("cannot open temp file");

	if (argc < 2 || ((finput = fopen(argv[1], "r")) == NULL)) {
		error("cannot open input file");
	}

	cnamp = cnames;
	defin(0, "$end");
	extval = 0400;				/* beginning of assigned values */
	defin(0, "error");
	defin(1, "$accept");
	mem = mem0;
	lev = 0;
	i = 0;

	/* sorry -- no yacc parser here.....
	   we must bootstrap somehow... */

	for (t = gettok(); t != MARK && t != ENDFILE;) {
		switch (t) {

		case ';':
			t = gettok();
			break;

		case LEFT:
		case BINARY:
		case RIGHT:
			++i;
		case TERM:
			lev = 0;

			if (t -= TERM) {
				SETASC(lev, t);
				SETPLEV(lev, i);
			}

			/* get identifiers so defined */

			t = gettok();
			for (;;) {
				switch (t) {

				case ',':
					t = gettok();
					continue;

				case ';':
					break;

				case IDENTIFIER:
					j = chfind(0, tokname);
					trmlev[j] = lev;
					if ((t = gettok()) == NUMBER) {
						trmset[j].value = numbval;
						if (j < ndefout && j > 2) {
							error
								("please define type number of %s earlier",
								 trmset[j].name);
						}
						t = gettok();
					}
					continue;

				}

				break;
			}

			continue;

		case LCURLY:
			defout();
			cpycode();
			t = gettok();
			continue;

		default:
			error("syntax error");

		}

	}

	if (t == ENDFILE) {
		error("unexpected EOF before %%");
	}

	/* t is MARK */

	defout();
	switch (lflag) {

	case EFL:
		fprintf(ftable, "\n");
		fprintf(ftable, "define yyerrok    =  yyerrf = 0\n");
		fprintf(ftable, "define yyclearin  =  yychar = -1\n");
		fprintf(ftable, "define YYSTKSIZE  =  150\n\n");
		break;

	case RATFOR:
		fprintf(ftable, "define yyerrok yyerrf = 0\n");
		fprintf(ftable, "define yyclearin yychar = -1\n");
		break;

	case C:
		fprintf(ftable, "#define yyclearin yychar = -1\n");
		fprintf(ftable, "#define yyerrok yyerrflag = 0\n");
		fprintf(ftable, "extern int yychar, yyerrflag;\n");
		fprintf(ftable,
				"#ifndef YYMAXDEPTH\n#define YYMAXDEPTH 150\n#endif\n");
		fprintf(ftable, "#ifndef YYSTYPE\n#define YYSTYPE int\n#endif\n");
		fprintf(ftable,
				"#ifndef YYVCOPY\n#define YYVCOPY(x,y) x=y\n#endif\n");
		fprintf(ftable, "YYSTYPE yylval, yyval;\n");
		break;
	}

	prdptr[0] = mem;
	/* added production */
	*mem++ = NTBASE;
	*mem++ = NTBASE + 1;
	*mem++ = 1;
	*mem++ = 0;
	prdptr[1] = mem;

	while ((t = gettok()) == LCURLY)
		cpycode();

	if (t != C_IDENTIFIER)
		error("bad syntax on first rule");

	/* read rules */

	while (t != MARK && t != ENDFILE) {

		/* process a rule */

		if (t == '|') {
			*mem++ = *prdptr[nprod - 1];
		} else if (t == C_IDENTIFIER) {
			*mem = chfind(1, tokname);
			if (*mem < NTBASE)
				error("token illegal on LHS of grammar rule");
			++mem;
		} else
			error("illegal rule: missing semicolon or | ?");

		/* read rule body */

		t = gettok();
more_rule:
		while (t == IDENTIFIER) {
			*mem = chfind(1, tokname);
			if (*mem < NTBASE)
				levprd[nprod] = trmlev[*mem];
			++mem;
			t = gettok();
		}

		if (t == PREC) {
			if (gettok() != IDENTIFIER)
				error("illegal %%prec syntax");
			j = chfind(2, tokname);
			if (j >= NTBASE)
				error("nonterminal %s illegal after %%prec",
					  nontrst[j - NTBASE].name);
			levprd[nprod] = trmlev[j];
			t = gettok();
		}

		if (t == '=') {
			levprd[nprod] |= ACTFLAG;
			fprintf(faction, acts[lflag], nprod);
			cpyact(mem - prdptr[nprod] - 1);
			fprintf(faction, acte[lflag]);
			if ((t = gettok()) == IDENTIFIER) {
				/* action within rule... */

				sprintf(actname, "$$%d", nprod);
				j = chfind(1, actname);	/* make it a nonterminal */

				/* the current rule will become rule number nprod+1 */
				/* move the contents down, and make room for the null */

				for (p = mem; p >= prdptr[nprod]; --p)
					p[2] = *p;
				mem += 2;

				/* enter null production for action */

				p = prdptr[nprod];

				*p++ = j;
				*p++ = -nprod;

				/* update the production information */

				levprd[nprod + 1] = levprd[nprod] & ~ACTFLAG;
				levprd[nprod] = ACTFLAG;
				if (++nprod >= prdlim)
					error("more than %d rules", prdlim);
				prdptr[nprod] = p;

				/* make the action appear in the original rule */
				*mem++ = j;

				/* get some more of the rule */

				goto more_rule;
			}

		}

		while (t == ';')
			t = gettok();

		*mem++ = -nprod;
		if (++nprod >= prdlim)
			error("more than %d rules", prdlim);
		prdptr[nprod] = mem;
		levprd[nprod] = 0;

	}

	/* end of all rules */

	finact();
	if (t == MARK) {
		while ((c = fgetc(finput)) != EOF)
			fputc(c, ftable);
	}
	fclose(finput);
}

finact()
{
	/* finish action routine */
	register i, j;

	if (lflag == RATFOR) {
		if (nprod <= 15) {
			fprintf(faction, "\n1000 goto(");
			PLOOP(1, i) {
				fprintf(faction, "%d,", (levprd[i] & ACTFLAG) ? i : 999);
			}
			fprintf(faction, "999),yym\n");
		} else {
			fprintf(faction, "\n1000 yyj = ((yym-1)/15)+1\ngoto(");
			for (i = 1; i < nprod; i += 15)
				fprintf(faction, "%d,", i / 15 + 800);
			fprintf(faction, "999),yyj\n");

			for (i = 1; i < nprod; i += 15) {
				fprintf(faction, "%d yyj = yym - %d\ngoto(", i / 15 + 800,
						i - 1);
				for (j = i; j < nprod && j < i + 15; ++j) {
					fprintf(faction, "%d,",
							(levprd[j] & ACTFLAG) ? j : 999);
				}
				fprintf(faction, "999),yyj\n");
			}
		}
	}

	fclose(faction);

	fprintf(ftable, ndefs[lflag], "YYERRCODE", trmset[2].value);

}

defin(t, s)
register char *s;
{
/*	define s to be a terminal if t=0
	or a nonterminal if t=1		*/

	register val;

	if (t) {
		if (++nnonter >= ntlim)
			error("too many nonterminals, limit %d", ntlim);
		nontrst[nnonter].name = cstash(s);
		return (NTBASE + nnonter);
	}
	/* must be a token */
	if (++nterms >= tlim)
		error("too many terminals, limit %d", tlim);
	trmset[nterms].name = cstash(s);

	/* establish value for token */

	if (s[0] == ' ' && s[2] == '\0')	/* single character literal */
		val = s[1];
	else if (s[0] == ' ' && s[1] == '\\') {	/* escape sequence */
		if (s[3] == '\0') {		/* single character escape sequence */
			switch (s[2]) {
				/* character which is escaped */
			case 'n':
				val = '\n';
				break;
			case 'r':
				val = '\r';
				break;
			case 'b':
				val = '\b';
				break;
			case 't':
				val = '\t';
				break;
			case 'f':
				val = '\f';
				break;
			case '\'':
				val = '\'';
				break;
			case '"':
				val = '"';
				break;
			case '\\':
				val = '\\';
				break;
			default:
				error("invalid escape");
			}
		} else if (s[2] <= '7' && s[2] >= '0') {	/* \nnn sequence */
			if (s[3] < '0' || s[3] > '7' || s[4] < '0' ||
				s[4] > '7' || s[5] != '\0')
				error("illegal \\nnn construction");
			val = 64 * s[2] + 8 * s[3] + s[4] - 73 * '0';
			if (val == 0)
				error("'\\000' is illegal");
		}
	} else {
		val = extval++;
	}
	trmset[nterms].value = val;
	trmlev[nterms] = 0;
	return (nterms);
}

defout()
{								/* write out the defines (at the end of the declaration section) */

	register int i, c;
	register char *cp, *format;

	format = ndefs[lflag];
	for (i = ndefout; i <= nterms; ++i) {

		cp = trmset[i].name;
		if (*cp == ' ')
			++cp;				/* literals */

		for (; (c = *cp) != '\0'; ++cp) {

			if (islower(c) || isupper(c) || isdigit(c) || c == '_');	/* VOID */
			else
				goto nodef;
		}

		fprintf(ftable, format, trmset[i].name, trmset[i].value);
		if (fdefine != NULL)
			fprintf(fdefine, format, trmset[i].name, trmset[i].value);

nodef:	;
	}

	ndefout = nterms + 1;

}

char *
cstash(s)
register char *s;
{
	char *temp;

	temp = cnamp;
	do {
		if (cnamp >= &cnames[cnamsz])
			error("too many characters in id's and literals");
		else
			*cnamp++ = *s;
	} while (*s++);
	return (temp);
}

gettok()
{
	register i, base;
	static int peekline;		/* number of '\n' seen in lookahead */
	register c, match, reserve;

begin:
	reserve = 0;
	lineno += peekline;
	peekline = 0;
	c = fgetc(finput);
	while (c == ' ' || c == '\n' || c == '\t' || c == '\f') {
		if (c == '\n')
			++lineno;
		c = fgetc(finput);
	}
	if (c == '/') {
		if (fgetc(finput) != '*')
			error("illegal /");
		c = fgetc(finput);
		while (c >= 0) {
			if (c == '\n')
				++lineno;
			if (c == '*') {
				if ((c = fgetc(finput)) == '/')
					break;
			} else
				c = fgetc(finput);
		}
		if (c == EOF)
			return (0);
		goto begin;
	}

	switch (c) {

	case EOF:
		return (ENDFILE);
	case '{':
		ungetc(c, finput);
		return ('=');			/* action ... */
	case '"':
	case '\'':
		match = c;
		tokname[0] = ' ';
		i = 1;
		while (1) {
			c = fgetc(finput);
			if (c == '\n' || c == EOF)
				error("illegal or missing ' or \"");
			if (c == '\\') {
				c = fgetc(finput);
				tokname[i] = '\\';
				if (++i >= _namesize)
					--i;
			} else if (c == match)
				break;
			tokname[i] = c;
			if (++i >= _namesize)
				--i;
		}
		break;

	case '%':
	case '\\':

		switch (c = fgetc(finput)) {

		case '0':
			return (TERM);
		case '<':
			return (LEFT);
		case '2':
			return (BINARY);
		case '>':
			return (RIGHT);
		case '%':
		case '\\':
			return (MARK);
		case '=':
			return (PREC);
		case '{':
			return (LCURLY);
		default:
			reserve = 1;
		}

	default:

		if (isdigit(c)) {		/* number */
			numbval = c - '0';
			base = (c == '0') ? 8 : 10;
			for (c = fgetc(finput); isdigit(c); c = fgetc(finput)) {
				numbval = numbval * base + c - '0';
			}
			ungetc(c, finput);
			return (NUMBER);
		} else if (islower(c) || isupper(c) || c == '_' || c == '.'
				   || c == '$') {
			while (islower(c) || isupper(c) || isdigit(c) || c == '_'
				   || c == '.' || c == '$') {
				tokname[i] = c;
				if (reserve && isupper(c))
					tokname[i] += 'a' - 'A';
				if (++i >= _namesize)
					--i;
				c = fgetc(finput);
			}
		} else
			return (c);

		ungetc(c, finput);
	}

	tokname[i] = '\0';

	if (reserve) {				/* find a reserved word */
		if (!strcmp(tokname, "term"))
			return (TERM);
		if (!strcmp(tokname, "token"))
			return (TERM);
		if (!strcmp(tokname, "left"))
			return (LEFT);
		if (!strcmp(tokname, "nonassoc"))
			return (BINARY);
		if (!strcmp(tokname, "binary"))
			return (BINARY);
		if (!strcmp(tokname, "right"))
			return (RIGHT);
		if (!strcmp(tokname, "prec"))
			return (PREC);
		error("invalid escape, or illegal reserved word: %s", tokname);
	}

	/* look ahead to distinguish IDENTIFIER from C_IDENTIFIER */

	c = fgetc(finput);
	while (c == ' ' || c == '\t' || c == '\n' || c == '\f') {
		if (c == '\n')
			++peekline;
		c = fgetc(finput);
	}
	if (c == ':')
		return (C_IDENTIFIER);
	ungetc(c, finput);
	return (IDENTIFIER);
}

chfind(t, s)
register char *s;
{
	int i;

	if (s[0] == ' ')
		t = 0;
	TLOOP(i) {
		if (!strcmp(s, trmset[i].name)) {
			return (i);
		}
	}
	NTLOOP(i) {
		if (!strcmp(s, nontrst[i].name)) {
			return (i + NTBASE);
		}
	}
	/* cannot find name */
	if (t > 1)
		error("%s should have been defined earlier", s);
	return (defin(t, s));
}

cpycode()
{								/* copies code between \{ and \} */

	int c;
	c = fgetc(finput);
	if (c == '\n') {
		c = fgetc(finput);
		lineno++;
	}
	while (c >= 0) {
		if (c == '\\')
			if ((c = fgetc(finput)) == '}')
				return;
			else
				fputc('\\', ftable);
		if (c == '%')
			if ((c = fgetc(finput)) == '}')
				return;
			else
				fputc('%', ftable);
		fputc(c, ftable);
		if (c == '\n')
			++lineno;
		c = fgetc(finput);
	}
	error("eof before %%}");
}

cpyact(offset)
{								/* copy C action to the next ; or closing } */
	int brac, c, match, j, s;

	brac = 0;

loop:
	c = fgetc(finput);
swt:
	switch (c) {

	case ';':
		if (brac == 0) {
			fputc(c, faction);
			return;
		}
		goto lcopy;

	case '{':
		if (brac++ || lflag != EFL)
			goto lcopy;
		else
			goto loop;

	case '$':
		s = 1;
		c = fgetc(finput);
		if (c == '$') {
			fprintf(faction, "yyval");
			goto loop;
		}
		if (c == '-') {
			s = -s;
			c = fgetc(finput);
		}
		if (isdigit(c)) {
			j = 0;
			while (isdigit(c)) {
				j = j * 10 + c - '0';
				c = fgetc(finput);
			}

			j = j * s - offset;
			if (j > 0) {
				error("Illegal use of $%d", j + offset);
			}

			fprintf(faction, dollar[lflag], -j);
			goto swt;
		}
		fputc('$', faction);
		if (s < 0)
			fputc('-', faction);
		goto swt;

	case '}':
		if (--brac)
			goto lcopy;
		if (lflag != EFL)
			fputc(c, faction);
		return;

	case '#':					/* look for RATFOR or EFL comments */
		if (lflag == C)
			goto lcopy;
		fputc(c, faction);
		while (c = fgetc(finput)) {
			if (c == '\n')
				goto swt;
			if (c == EOF)
				goto swt;
			fputc(c, faction);
		}

	case '/':					/* look for comments */
		if (lflag != C)
			goto lcopy;
		fputc(c, faction);
		c = fgetc(finput);
		if (c != '*')
			goto swt;

		/* it really is a comment */

		fputc(c, faction);
		while (c = fgetc(finput)) {
			while (c == '*') {
				fputc(c, faction);
				if ((c = fgetc(finput)) == '/')
					goto lcopy;
			}
			fputc(c, faction);
			if (c == '\n')
				++lineno;
		}
		error("EOF inside comment");

	case '\'':					/* character constant */
		match = '\'';
		goto string;

	case '\"':					/* character string */
		match = '\"';

string:

		fputc(c, faction);
		while (c = fgetc(finput)) {

			if (c == '\\') {
				fputc(c, faction);
				c = fgetc(finput);
				if (c == '\n')
					++lineno;
			} else if (c == match)
				goto lcopy;
			else if (c == '\n')
				error("newline in string or char. const.");
			fputc(c, faction);
		}
		error("EOF in string or character constant");

	case EOF:
		error("action does not terminate");

	case '\n':
		++lineno;
		goto lcopy;

	}

lcopy:
	fputc(c, faction);
	goto loop;
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
