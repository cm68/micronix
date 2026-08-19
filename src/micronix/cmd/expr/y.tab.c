#define A_STRING 257
#define SUBSTR 258
#define LENGTH 259
#define INDEX 260
#define NOARG 261
#define yyclearin yychar = -1
#define yyerrok yyerrflag = 0
extern int yychar, yyerrflag;
#ifndef YYMAXDEPTH
#define YYMAXDEPTH 150
#endif
#ifndef YYSTYPE
#define YYSTYPE int
#endif
#ifndef YYVCOPY
#define YYVCOPY(x,y) x=y
#endif
YYSTYPE yylval, yyval;
#define YYERRCODE 256

/*	expression command - 1.2 of 6/6/76 */
#include <stdio.h>

extern int yyinit();
extern int yyparse();
extern int write();

char	**av;
int	ac;
char	tmp[100][6];
int	tmpi;
int	argi;

main(argc, argv) char **argv; {
	ac = argc;
	argi = 1;
	av = argv;
	yyinit();
	yyparse();
}
yylex() {
register char *p;

	if(argi >= ac) return NOARG;

	p = av[argi++];

	switch(*p) {
		case '+':
		case '-':
		case '*':
		case '/':
		case '%':
		case '&':
		case '|':
		case '(':
		case ')':
			if(*(p+1) == '\0') return *p;
	}

	if(eq(p, "substr")) return SUBSTR;

	if(eq(p, "length")) return LENGTH;
	if(eq(p, "index")) return INDEX;

	yylval = p;
	return A_STRING;
}

binop(op, r1, r2) char op, *r1, *r2; {
register int i1, i2;

	i1 = atoi(r1);
	i2 = atoi(r2);

	switch(op) {
		case '+': i1 = i1 + i2; break;
		case '-': i1 = i1 - i2; break;
		case '*': i1 = i1 * i2; break;
		case '/': i1 = i1 / i2; break;
		case '%': i1 = i1 % i2; break;
		case '&': i1 = i1 & i2; break;
		case '|': i1 = i1 | i2; break;
	}
	copy(itoa(i1), tmp[tmpi]);
	return tmp[tmpi++];
}

substr(v, s, w) char *v, *s, *w; {
register int si, wi;
register char *res;

	si = atoi(s);
	wi = atoi(w);
	while(--si) if(*v) ++v;

	res = v;

	while(wi--) if(*v) ++v;

	*v = '\0';
	return res;
}

length(s) register char *s; {
register int i = 0;

	while(*s++) ++i;

	copy(itoa(i), tmp[tmpi]);
	return tmp[tmpi++];
}

index(s, t) char *s, *t; {

register int i, j;

	for(i = 0; s[i] ; ++i)
		for(j = 0; t[j] ; ++j)
			if(s[i]==t[j]) {
				copy(itoa(++i), tmp[tmpi]);
				return tmp[tmpi++];
			}
	return "0";
}

eq(a, b)
char *a, *b;
{
	register int i;

	i = 0;
l:
	if(a[i] != b[i])
		return(0);
	if(a[i++] == '\0')
		return(1);
	goto l;
}
atoi(s) register char *s; {

	register int i = 0, neg = 0;


	if(*s == '-') {
		++neg;
		++s;
	}

	while(*s) {
		if(*s < '0' || *s > '9') {
			write(2, "non-numeric argument\n", 21);
			putchar('\n');
			exit(1);
		}
		i = i * 10 + (*s - '0');
		++s;
	}
	return neg? -i: i;
}

itoa(n) register int n; {
	register char *cp;
	static char str[10];
	register int neg = 0;

	if(n < 0) {
		neg++;
		n= -n;
	}
	for(cp = &str[8]; ;--cp) {
		*cp = n % 10 + '0';
		n = n / 10;
		if (n == 0) {
			if(neg)
				*--cp = '-';
			return cp;
		}
	}
}
copy(source, sink) register char *source, *sink; {
	 while(*sink++ = *source++);
}
int yyexca[] = {
-1, 1,
	0, -1,
	-2, 0,
	};
#define YYNPROD 14
extern int yychar;

int yylast = 236;
int yyact[] = {

  15,  10,   1,   3,   0,  13,  11,   3,  12,   0,
  14,  15,  10,   0,   0,  15,  13,  11,   0,  12,
  13,  14,  15,  10,   0,  14,  27,  13,  11,   0,
  12,   0,  14,  15,  10,   0,   0,   0,  13,  11,
  15,  12,   0,  14,   0,  13,  11,   2,  12,   0,
  14,  16,  17,  18,  19,   0,   0,  20,  21,  22,
  23,  24,  25,  26,   0,  28,   0,  29,   0,   0,
   0,   0,   0,   0,   0,   0,  30,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   9,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   9,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   9,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   7,   4,   5,   6,   7,   4,   5,   6,   0,   0,
   0,   0,   0,   0,   0,   8 };
int yypact[] = {

 -33,-1000, -26, -33, -33, -33, -33,-1000,-1000, -33,
 -33, -33, -33, -33, -33, -33, -15, -37,-1000, -37,
  -4,   3, -22, -22,-1000,-1000,-1000,-1000, -37,-1000,
-1000 };
int yypgo[] = {

   0,   2,  47 };
int yyr1[] = {

   0,   1,   2,   2,   2,   2,   2,   2,   2,   2,
   2,   2,   2,   2 };
int yyr2[] = {

   0,   2,   3,   3,   3,   3,   3,   3,   3,   3,
   4,   2,   3,   1 };
int yychk[] = {

-1000,  -1,  -2,  40, 258, 259, 260, 257, 261, 124,
  38,  43,  45,  42,  47,  37,  -2,  -2,  -2,  -2,
  -2,  -2,  -2,  -2,  -2,  -2,  -2,  41,  -2,  -2,
  -2 };
int yydef[] = {

   0,  -2,   0,   0,   0,   0,   0,  13,   1,   0,
   0,   0,   0,   0,   0,   0,   0,   0,  11,   0,
   3,   4,   5,   6,   7,   8,   9,   2,   0,  12,
  10 };
#define YYFLAG -1000
#define YYERROR goto yyerrlab
#define YYACCEPT return(0)
#define YYABORT return(1)

/*	parser for yacc output	*/

int yydebug = 0; /* 1 for debugging */
YYSTYPE yyv[YYMAXDEPTH]; /* where the values are stored */
int yychar = -1; /* current input token number */
int yynerrs = 0;  /* number of errors */
int yyerrflag = 0;  /* error recovery flag */

yyparse() {

	int yys[YYMAXDEPTH];
	int yyj, yym;
	register YYSTYPE *yypvt;
	register yystate, *yyps, yyn;
	register YYSTYPE *yypv;
	register *yyxi;
	/*
	 * ccc's c1 miscompiles the v7 spelling
	 * `if( ++yyps> &yys[YYMAXDEPTH] )`: the bound &yys[150] is
	 * computed into hl, then loading yyps clobbers it, and the
	 * compare runs against the size constant instead of the address,
	 * so the check fires on the first push.  Pre-computing the bound
	 * and splitting the increment sidesteps it (CODEGENGAPS entry 22).
	 */
	int *yyslim = &yys[YYMAXDEPTH];

	yystate = 0;
	yychar = -1;
	yynerrs = 0;
	yyerrflag = 0;
	yyps= &yys[-1];
	yypv= &yyv[-1];

 yystack:    /* put a state and value onto the stack */

	if( yydebug  ) printf( "state %d, value %d, char %d\n",yystate,yyval,yychar );
		++yyps;
		if( yyps> yyslim ) { yyerror( "yacc stack overflow" ); return(1); }
		*yyps = yystate;
		++yypv;
		YYVCOPY(*yypv,yyval);

 yynewstate:

	yyn = yypact[yystate];

	if( yyn<= YYFLAG ) goto yydefault; /* simple state */

	if( yychar<0 ) if( (yychar=yylex())<0 ) yychar=0;
	if( (yyn += yychar)<0 || yyn >= yylast ) goto yydefault;

	if( yychk[ yyn=yyact[ yyn ] ] == yychar ){ /* valid shift */
		yychar = -1;
		YYVCOPY(yyval,yylval);
		yystate = yyn;
		if( yyerrflag > 0 ) --yyerrflag;
		goto yystack;
		}

 yydefault:
	/* default state action */

	if( (yyn=yydef[yystate]) == -2 ) {
		if( yychar<0 ) if( (yychar=yylex())<0 ) yychar = 0;
		/* look through exception table */

		yyxi = yyexca;
		for(;;){
			if( (*yyxi == (-1)) && (yyxi[1] == yystate) ) break;
			yyxi += 2;
			}

		while( *(yyxi+=2) >= 0 ){
			if( *yyxi == yychar ) break;
			}
		if( (yyn = yyxi[1]) < 0 ) return(0);   /* accept */
		}

	if( yyn == 0 ){ /* error */
		/* error ... attempt to resume parsing */

		switch( yyerrflag ){

		case 0:   /* brand new error */

			yyerror( "syntax error" );
		yyerrlab:
			++yynerrs;

		case 1:
		case 2: /* incompletely recovered error ... try again */

			yyerrflag = 3;

			/* find a state where "error" is a legal shift action */

			while ( yyps >= yys ) {
			   yyn = yypact[*yyps] + YYERRCODE;
			   if( yyn>= 0 && yyn < yylast && yychk[yyact[yyn]] == YYERRCODE ){
			      yystate = yyact[yyn];  /* simulate a shift of "error" */
			      goto yystack;
			      }
			   yyn = yypact[*yyps];

			   /* the current yyps has no shift onn "error", pop stack */

			   if( yydebug ) printf( "error recovery pops state %d, uncovers %d\n", *yyps, yyps[-1] );
			   --yyps;
			   --yypv;
			   }

			/* there is no state on the stack with an error shift ... abort */

	yyabort:
			return(1);


		case 3:  /* no shift yet; clobber input char */

			if( yydebug ) printf( "error recovery discards char %d\n", yychar );

			if( yychar == 0 ) goto yyabort; /* don't discard EOF, quit */
			yychar = -1;
			goto yynewstate;   /* try again in the same state */

			}

		}

	/* reduction by production yyn */

		if( yydebug ) printf("reduce %d\n",yyn);
		yyps -= yyr2[yyn];
		yypvt = yypv;
		yypv -= yyr2[yyn];
		YYVCOPY(yyval,yypv[1]);
		yym=yyn;
			/* consult goto table to find next state */
		yyn = yyr1[yyn];
		yyj = yypgo[yyn] + *yyps + 1;
		yystate = yyact[yyj];
		if( yyj>=yylast || (yychk[yystate] + yyn) ) yystate = yyact[yypgo[yyn]];
		switch(yym){
			
case 1: { printf("%s\n", yypvt[-1]); exit(0); } break;
case 2: { yyval = yypvt[-1]; } break;
case 3: { yyval = binop('|', yypvt[-2], yypvt[-0]); } break;
case 4: { yyval = binop('&', yypvt[-2], yypvt[-0]); } break;
case 5: { yyval = binop('+', yypvt[-2], yypvt[-0]); } break;
case 6: { yyval = binop('-', yypvt[-2], yypvt[-0]); } break;
case 7: { yyval = binop('*', yypvt[-2], yypvt[-0]); } break;
case 8: { yyval = binop('/', yypvt[-2], yypvt[-0]); } break;
case 9: { yyval = binop('%', yypvt[-2], yypvt[-0]); } break;
case 10: { yyval = substr(yypvt[-2], yypvt[-1], yypvt[-0]); } break;
case 11: { yyval = length(yypvt[-0]); } break;
case 12: { yyval = index(yypvt[-1], yypvt[-0]); } break;
		}
		goto yystack;  /* stack new state and value */

	}
