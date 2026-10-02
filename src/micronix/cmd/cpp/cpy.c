/*
 * cpy.c - the parser for a preprocessor #if expression
 *
 * cmd/cpp/cpy.c
 *
 * Replaces cpy.y, Reiser's yacc grammar (kept beside this file), which
 * in the original was run through yacc to produce this translation
 * unit.  It is hand-written here because the cross build has no yacc
 * to run before this directory builds - the same reason cmd/lex
 * commits its y.tab.c.  The grammar is reproduced as one function per
 * precedence level, lowest first:
 *
 *	','  ?:  ||  &&  | ^  &  == !=  < > <= >=  << >>  + -  * / %  unary
 *
 * yyparse() returns 1 when the expression is true, 0 when it is not.
 * The tokenizer is yylex(), in yylex.c, included at the foot of this
 * file, which is the original's own # include "yylex.c".
 *
 * vim: tabstop=4 shiftwidth=4 noexpandtab:
 */

/* token codes, in the order cpy.y's %term lines assign them */
#define number 257
#define stop   258
#define DEFINED 259
#define EQ     260
#define NE     261
#define LE     262
#define GE     263
#define LS     264
#define RS     265
#define ANDAND 266
#define OROR   267

int yylval;		/* a number token's value, set by yylex */

static int tok;		/* the current lookahead token */

static int comma();
static int ternary();
static int oror();
static int andand();
static int bitor();
static int bitand();
static int equality();
static int relational();
static int shift();
static int additive();
static int multiplicative();
static int unary();
static int primary();

static
advance()
{
	tok = yylex();
}

int
yyparse()
{
	int v;

	advance();
	v = comma();
	if (tok != stop)
		yyerror("syntax error");
	return v;
}

/* e ',' e, returning the right operand */
static int
comma()
{
	int l;

	l = ternary();
	while (tok == ',') {
		advance();
		l = ternary();
	}
	return l;
}

/* e '?' e ':' e, right associative */
static int
ternary()
{
	int c, a, b;

	c = oror();
	if (tok != '?')
		return c;
	advance();
	a = comma();	/* the then-branch is a full expression */
	if (tok != ':') {
		yyerror("syntax error");
		return 0;
	}
	advance();
	b = ternary();	/* the else-branch is right associative */
	return c ? a : b;
}

static int
oror()
{
	int l, r;

	l = andand();
	while (tok == OROR) {
		advance();
		r = andand();
		l = l || r;
	}
	return l;
}

static int
andand()
{
	int l, r;

	l = bitor();
	while (tok == ANDAND) {
		advance();
		r = bitor();
		l = l && r;
	}
	return l;
}

/* '|' and '^' share a level */
static int
bitor()
{
	int l, r;

	l = bitand();
	for (;;) {
		if (tok == '|') {
			advance();
			r = bitand();
			l = l | r;
		} else if (tok == '^') {
			advance();
			r = bitand();
			l = l ^ r;
		} else
			break;
	}
	return l;
}

static int
bitand()
{
	int l, r;

	l = equality();
	while (tok == '&') {
		advance();
		r = equality();
		l = l & r;
	}
	return l;
}

static int
equality()
{
	int l, r;

	l = relational();
	for (;;) {
		if (tok == EQ) {
			advance();
			r = relational();
			l = l == r;
		} else if (tok == NE) {
			advance();
			r = relational();
			l = l != r;
		} else
			break;
	}
	return l;
}

static int
relational()
{
	int l, r;

	l = shift();
	for (;;) {
		switch (tok) {
		case '<':
			advance(); r = shift(); l = l < r; break;
		case '>':
			advance(); r = shift(); l = l > r; break;
		case LE:
			advance(); r = shift(); l = l <= r; break;
		case GE:
			advance(); r = shift(); l = l >= r; break;
		default:
			return l;
		}
	}
}

static int
shift()
{
	int l, r;

	l = additive();
	for (;;) {
		if (tok == LS) {
			advance();
			r = additive();
			l = l << r;
		} else if (tok == RS) {
			advance();
			r = additive();
			l = l >> r;
		} else
			break;
	}
	return l;
}

static int
additive()
{
	int l, r;

	l = multiplicative();
	for (;;) {
		if (tok == '+') {
			advance();
			r = multiplicative();
			l = l + r;
		} else if (tok == '-') {
			advance();
			r = multiplicative();
			l = l - r;
		} else
			break;
	}
	return l;
}

static int
multiplicative()
{
	int l, r;

	l = unary();
	for (;;) {
		switch (tok) {
		case '*':
			advance(); r = unary(); l = l * r; break;
		case '/':
			advance(); r = unary(); l = l / r; break;
		case '%':
			advance(); r = unary(); l = l % r; break;
		default:
			return l;
		}
	}
}

static int
unary()
{
	switch (tok) {
	case '-':
		advance();
		return -unary();
	case '!':
		advance();
		return !unary();
	case '~':
		advance();
		return ~unary();
	}
	return primary();
}

static int
primary()
{
	int v;

	switch (tok) {
	case '(':
		advance();
		v = comma();
		if (tok != ')') {
			yyerror("syntax error");
			return 0;
		}
		advance();
		return v;
	case DEFINED:
		advance();
		if (tok == '(') {
			advance();
			if (tok != number) {
				yyerror("syntax error");
				return 0;
			}
			v = yylval;
			advance();
			if (tok != ')') {
				yyerror("syntax error");
				return 0;
			}
			advance();
			return v;
		}
		if (tok != number) {
			yyerror("syntax error");
			return 0;
		}
		v = yylval;
		advance();
		return v;
	case number:
		v = yylval;
		advance();
		return v;
	}
	yyerror("syntax error");
	return 0;
}

#include "yylex.c"
