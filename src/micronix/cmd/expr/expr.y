/*
 * expr command from PWB
 *
 * cmd/expr/expr.y
 *
 * The PWB expression evaluator - the yacc grammar plus its lexer and
 * the arithmetic/string primitives, in one file as v6 wrote it.
 * Source is extra/pwb/spencer/sys/source/s1/expr.y, version 1.2 of
 * 6/6/76.
 *
 * yacc emits y.tab.c from this grammar; that file is committed beside
 * it because the cross build has no yacc to run before this directory
 * builds (see GNUmakefile).  The C here was normalised to what ccc
 * accepts, the same set of v6-isms the msh port fixed:
 *
 *   - the `=`-less initializers: `int yyline 0`, `register i 0`,
 *     `register neg 0` gain `=`; implicit-int registers gain int.
 *   - `int yyline` is dropped, and yyinit() and yyerror() are defined
 *     at the foot of the file.  The v6 expr linked them out of the
 *     yacc runtime, but that would make expr depend on liby.a, which
 *     cmd does not build before this directory - so they are inlined
 *     and expr is self-contained, the way cmd/make carries its own
 *     yyerror.
 *   - itoa()'s buffer was `static char *str[10]`, a pointer array
 *     where a char array was wanted; it is `char str[10]` now.
 *
 * vim: tabstop=4 shiftwidth=4 noexpandtab:
 */

%token A_STRING SUBSTR LENGTH INDEX NOARG

/* operators listed below in increasing precedence: */
%left '|'
%left '&'
%left '+' '-'
%left '*' '/' '%'
%left SUBSTR
%left LENGTH INDEX
%%

/* a single `expression' is evaluated and printed: */

expression:	expr NOARG = { printf("%s\n", $1); exit(0); }
	;


expr:	'(' expr ')' = { $$ = $2; }
	| expr '|' expr   = { $$ = binop('|', $1, $3); }
	| expr '&' expr   = { $$ = binop('&', $1, $3); }
	| expr '+' expr   = { $$ = binop('+', $1, $3); }
	| expr '-' expr   = { $$ = binop('-', $1, $3); }
	| expr '*' expr   = { $$ = binop('*', $1, $3); }
	| expr '/' expr   = { $$ = binop('/', $1, $3); }
	| expr '%' expr   = { $$ = binop('%', $1, $3); }
	| SUBSTR expr expr expr = { $$ = substr($2, $3, $4); }
	| LENGTH expr       = { $$ = length($2); }
	| INDEX expr expr = { $$ = index($2, $3); }
	| A_STRING
	;
%%
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

/* The two yacc-runtime entry points, inlined so expr does not have to
 * link liby.a (see the header).  yyinit is the v6 no-op the parser's
 * main calls before yyparse; yyerror is what yyparse reports through.
 */
yyinit()
{
}

yyerror(s)
char *s;
{
	printf("%s\n", s);
}
