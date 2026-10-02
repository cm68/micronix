/*
 * m4eval.c - the expression parser for m4's eval builtin
 *
 * cmd/m4/m4eval.c
 *
 * Replaces m4y.y, the v7 yacc grammar for eval (kept beside this file),
 * the way cmd/cpp replaces cpy.y: the cross build has no yacc, so the
 * grammar is written by hand, one function per precedence level.  The
 * tokenizer is yylex, copied verbatim from m4y.y's own trailing
 * section; it scans *pe (set by m4.c's doeval) and sets the global
 * evalval for a number.
 *
 * Precedence, lowest first, as m4y.y declares it:
 *
 *	|  &  !  == != > >= < <=  + -  * / %  ^  unary - +
 *
 * yyparse returns 0 and leaves the value in evalval, the way m4.c's
 * doeval expects.  yyerror is the same no-op m4y.y had.
 *
 * vim: tabstop=4 shiftwidth=4 noexpandtab:
 */

#define DIGITS 257
#define GT     258
#define GE     259
#define LT     260
#define LE     261
#define NE     262
#define EQ     263
#define POWER  264

extern long evalval;	/* a DIGITS token's value, set by yylex */
extern char *pe;	/* the expression string, scanned by yylex */

static int tok;		/* the current lookahead token */

static long oror();
static long andand();
static long bnot();
static long cmp();
static long add();
static long mul();
static long power();
static long unary();
static long primary();

static
advance()
{
	tok = yylex();
}

int
yyparse()
{
	advance();
	if (tok == '\0') {	/* s : <empty> */
		evalval = 0;
		return 0;
	}
	evalval = oror();
	return 0;
}

/* e '|' e, the logical or */
static long
oror()
{
	long l, r;

	l = andand();
	while (tok == '|') {
		advance();
		r = andand();
		l = (l != 0 || r != 0) ? 1 : 0;
	}
	return l;
}

/* e '&' e, the logical and */
static long
andand()
{
	long l, r;

	l = bnot();
	while (tok == '&') {
		advance();
		r = bnot();
		l = (l != 0 && r != 0) ? 1 : 0;
	}
	return l;
}

/* '!' e */
static long
bnot()
{
	if (tok == '!') {
		advance();
		return (bnot() == 0);
	}
	return cmp();
}

/* e == e  e != e  e > e  e >= e  e < e  e <= e, non-associative */
static long
cmp()
{
	long l, r;
	int op;

	l = add();
	op = tok;
	switch (op) {
	case EQ: case NE: case GT: case GE: case LT: case LE:
		advance();
		r = add();
		switch (op) {
		case EQ: return l == r;
		case NE: return l != r;
		case GT: return l > r;
		case GE: return l >= r;
		case LT: return l < r;
		case LE: return l <= r;
		}
	}
	return l;
}

/* e '+' e  e '-' e */
static long
add()
{
	long l;

	l = mul();
	for (;;) {
		if (tok == '+') {
			advance();
			l = l + mul();
		} else if (tok == '-') {
			advance();
			l = l - mul();
		} else
			break;
	}
	return l;
}

/* e '*' e  e '/' e  e '%' e */
static long
mul()
{
	long l;

	l = power();
	for (;;) {
		if (tok == '*') {
			advance();
			l = l * power();
		} else if (tok == '/') {
			advance();
			l = l / power();
		} else if (tok == '%') {
			advance();
			l = l % power();
		} else
			break;
	}
	return l;
}

/* e ^ e, right associative: a^b by repeated multiply, a^0 == 1 and
 * a^negative == 1, exactly as m4y.y's loop had it */
static long
power()
{
	long l, r, v;

	l = unary();
	if (tok == POWER) {
		advance();
		r = power();
		v = 1;
		for (; r-- > 0; v *= l)
			;
		return v;
	}
	return l;
}

/* '-' e  '+' e */
static long
unary()
{
	if (tok == '-') {
		advance();
		return -unary();
	}
	if (tok == '+') {
		advance();
		return unary();
	}
	return primary();
}

static long
primary()
{
	long v;

	if (tok == '(') {
		advance();
		v = oror();
		if (tok != ')') {
			yyerror("syntax error");
			return 0;
		}
		advance();
		return v;
	}
	if (tok == DIGITS) {
		v = evalval;
		advance();
		return v;
	}
	yyerror("syntax error");
	return 0;
}

/* the tokenizer, verbatim from m4y.y */
yylex()
{
	while (*pe == ' ' || *pe == '\t' || *pe == '\n')
		pe++;
	switch (*pe) {
	case '\0':
	case '+':
	case '-':
	case '/':
	case '%':
	case '(':
	case ')':
		return (*pe++);
	case '^':
		pe++;
		return (POWER);
	case '*':
		return (peek('*', POWER, '*'));
	case '>':
		return (peek('=', GE, GT));
	case '<':
		return (peek('=', LE, LT));
	case '=':
		return (peek('=', EQ, EQ));
	case '|':
		return (peek('|', '|', '|'));
	case '&':
		return (peek('&', '&', '&'));
	case '!':
		return (peek('=', NE, '!'));
	default:
		evalval = 0;
		while (*pe >= '0' && *pe <= '9')
			evalval = evalval * 10 + *pe++ - '0';
		return (DIGITS);
	}
}

peek(c, r1, r2)
{
	if (*++pe != c)
		return (r2);
	++pe;
	return (r1);
}

yyerror(s)
char *s;
{
}
