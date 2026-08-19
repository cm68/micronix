/*
 * yylex.c - hand-written tokenizer for awk.
 *
 * Replaces the lex-generated lex.yy.c, whose DFA tables cost ~4.2 K of
 * data and whose driver ~5 K of text.  awk's lexical grammar is a plain
 * recognizer - keywords, operators, numbers, quoted strings, and inline
 * /regex/ - so a brute-force scan with a one-char pushback is a fraction
 * of the size.  The token stream and yylval semantics match awk.lx.l
 * exactly, so the yacc parser and the rest of awk are untouched.
 */

#include "stdio.h"
#include "awk.def"
#include "awk.h"

#define	CBUFLEN	150

FILE	*yyin;
extern char	*lexprog;
extern int	mustfld;
extern int	yylval;

int	lineno	= 1;
char	cbuf[CBUFLEN];
int	clen;

static int	pushc;		/* one-char pushback, 0 when empty */
static int	lexstate;	/* 0 A, 1 str, 2 chc, 3 reg, 4 comment */
static int	cflag;		/* chc negation */
static int	pendrb;		/* emit '}' before the next real token */

/* keyword: name, token, and the yylval to set (-1 for none) */
static struct kw {
	char	*name;
	int	token;
	int	val;
} kws[] = {
	"BEGIN",	XBEGIN,		-1,
	"END",		XEND,		-1,
	"PROGEND",	0,		-1,
	"while",	WHILE,		-1,
	"for",		FOR,		-1,
	"if",		IF,		-1,
	"else",		ELSE,		-1,
	"next",		NEXT,		-1,
	"exit",		EXIT,		-1,
	"break",	BREAK,		-1,
	"continue",	CONTINUE,	-1,
	"in",		IN,		-1,
	"getline",	GETLINE,	-1,
	"print",	PRINT,		PRINT,
	"printf",	PRINTF,		PRINTF,
	"sprintf",	SPRINTF,	SPRINTF,
	"split",	SPLIT,		SPLIT,
	"substr",	SUBSTR,		-1,
	"index",	INDEX,		-1,
	"length",	FNCN,		FLENGTH,
	"log",		FNCN,		FLOG,
	"int",		FNCN,		FINT,
	"exp",		FNCN,		FEXP,
	"sqrt",		FNCN,		FSQRT,
	0,		0,		-1
};

static int
getch()
{
	int	c;

	if (pushc) {
		c = pushc;
		pushc = 0;
	} else if (yyin == NULL)
		c = (lexprog && *lexprog) ? *lexprog++ : 0;
	else
		c = getc(yyin);
	if (c == '\n')
		lineno++;
	if (c == EOF)
		c = 0;
	return c;
}

static void
ungetch(c)
int	c;
{
	pushc = c;
	if (c == '\n')
		lineno--;
}

void
startreg()
{
	lexstate = 3;
}

/* read an identifier [A-Za-z_][A-Za-z0-9_]* into id, return its length */
static int
getident(id)
char	*id;
{
	int	c, n;

	c = getch();
	n = 0;
	while ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
	       (c >= '0' && c <= '9') || c == '_') {
		if (n < 30)
			id[n++] = c;
		c = getch();
	}
	ungetch(c);
	id[n] = 0;
	return n;
}

static int
isident1(c)
int	c;
{
	return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_';
}

static int
isdig(c)
int	c;
{
	return c >= '0' && c <= '9';
}

int
yylex()
{
	int	c, c2, n, i;
	char	id[40];
	struct kw *kp;

	/* the '}' after a '}' -> ';' pair */
	if (pendrb) {
		pendrb = 0;
		return '}';
	}

	if (lexstate == 3)		/* regex mode */
		return yylexreg();

	for (;;) {
		c = getch();
		switch (c) {
		case ' ':
		case '\t':
			continue;	/* skip whitespace */

		case 0:			/* EOF */
			return 0;

		case '#':		/* comment to end of line */
			while ((c = getch()) && c != '\n')
				;
			if (c == '\n')
				return NL;
			return 0;

		case '\n':
			return NL;

		case ';':
			if ((c = getch()) == '\n')	/* eat the newline */
				;
			else
				ungetch(c);
			return ';';

		case '}':
			/* consume trailing blanks and, if there, a newline */
			do {
				c2 = getch();
			} while (c2 == ' ' || c2 == '\t');
			if (c2 != '\n')
				ungetch(c2);
			pendrb = 1;
			return ';';

		case '"':		/* string literal */
			lexstate = 1;
			clen = 0;
			return yylexstr();

		case '$':		/* field or indirect */
			c2 = getch();
			if (isdig(c2)) {
				n = 0;
				while (isdig(c2)) {
					n = n * 10 + (c2 - '0');
					c2 = getch();
				}
				ungetch(c2);
				if (n == 0) {
					yylval = (hack)lookup("$record", symtab, 0);
					return STRING;
				}
				yylval = (hack)fieldadr(n);
				return FIELD;
			}
			if (c2 == ' ' || c2 == '\t' || c2 == 0 || c2 == '\n') {
				ungetch(c2);
				return INDIRECT;
			}
			ungetch(c2);	/* fall through as a bare '$'? */
			return '$';

		case '\\':		/* line continuation */
			if ((c = getch()) == '\n')
				continue;	/* join lines */
			ungetch(c);
			return '\\';

		case '|':
			if ((c = getch()) == '|')
				return BOR;
			ungetch(c);
			return '|';

		case '&':
			if ((c = getch()) == '&')
				return AND;
			ungetch(c);
			return '&';

		case '!':
			if ((c = getch()) == '=') {
				yylval = NE;
				return RELOP;
			}
			if (c == '~') {
				yylval = NOTMATCH;
				return MATCHOP;
			}
			ungetch(c);
			return NOT;

		case '~':
			yylval = MATCH;
			return MATCHOP;

		case '<':
			if ((c = getch()) == '=') {
				yylval = LE;
				return RELOP;
			}
			ungetch(c);
			yylval = LT;
			return RELOP;

		case '>':
			if ((c = getch()) == '=') {
				yylval = GE;
				return RELOP;
			}
			if (c == '>') {
				yylval = APPEND;
				return RELOP;
			}
			ungetch(c);
			yylval = GT;
			return RELOP;

		case '=':
			if ((c = getch()) == '=') {
				yylval = EQ;
				return RELOP;
			}
			ungetch(c);
			yylval = ASSIGN;
			return ASGNOP;

		case '+':
			if ((c = getch()) == '+') {
				yylval = INCR;
				return INCR;
			}
			if (c == '=') {
				yylval = ADDEQ;
				return ASGNOP;
			}
			ungetch(c);
			return '+';

		case '-':
			if ((c = getch()) == '-') {
				yylval = DECR;
				return DECR;
			}
			if (c == '=') {
				yylval = SUBEQ;
				return ASGNOP;
			}
			ungetch(c);
			return '-';

		case '*':
			if ((c = getch()) == '=') {
				yylval = MULTEQ;
				return ASGNOP;
			}
			ungetch(c);
			return '*';

		case '/':
			if ((c = getch()) == '=') {
				yylval = DIVEQ;
				return ASGNOP;
			}
			ungetch(c);
			return '/';

		case '%':
			if ((c = getch()) == '=') {
				yylval = MODEQ;
				return ASGNOP;
			}
			ungetch(c);
			return '%';

		default:
			if (isdig(c)) {
				/* number: digits, optional .digits, optional exp */
				char num[40];
				n = 0;
				while (isdig(c)) {
					if (n < 30)
						num[n++] = c;
					c = getch();
				}
				if (c == '.') {
					if (n < 30)
						num[n++] = c;
					c = getch();
					while (isdig(c)) {
						if (n < 30)
							num[n++] = c;
						c = getch();
					}
				}
				if (c == 'e' || c == 'E') {
					if (n < 30)
						num[n++] = c;
					c = getch();
					if (c == '+' || c == '-') {
						if (n < 30)
							num[n++] = c;
						c = getch();
					}
					while (isdig(c)) {
						if (n < 30)
							num[n++] = c;
						c = getch();
					}
				}
				ungetch(c);
				num[n] = 0;
				yylval = (hack)setsymtab(num, EMPTY, fatof(num),
				    CON|NUM, symtab);
				return NUMBER;
			}
			if (c == '.') {
				c2 = getch();
				if (isdig(c2)) {
					char num[40];
					n = 0;
					num[n++] = '.';
					while (isdig(c2)) {
						if (n < 30)
							num[n++] = c2;
						c2 = getch();
					}
					ungetch(c2);
					num[n] = 0;
					yylval = (hack)setsymtab(num, EMPTY,
					    fatof(num), CON|NUM, symtab);
					return NUMBER;
				}
				ungetch(c2);
				return '.';
			}
			if (isident1(c)) {
				id[0] = c;
				getident(id + 1);
				for (kp = kws; kp->name; kp++)
					if (strcmp(id, kp->name) == 0) {
						if (kp->token == FNCN)
							yylval = kp->val;
						else if (kp->val != -1)
							yylval = kp->val;
						if (kp->token == VAR) {
							mustfld = 1;
							yylval = (hack)setsymtab(id,
							    EMPTY, 0L, NUM, symtab);
						}
						return kp->token;
					}
				yylval = (hack)setsymtab(id, tostring(""), 0L,
				    STR|NUM, symtab);
				return VAR;
			}
			yylval = c;
			return c;
		}
	}
}

/*
 * String literal body.  On entry cbuf[0..clen-1] is empty; scan until
 * the closing quote.  Lexed from lexstate == 1.
 */
int
yylexstr()
{
	int	c;

	for (;;) {
		c = getch();
		if (c == '"') {
			char *s;
			lexstate = 0;
			cbuf[clen] = 0;
			s = tostring(cbuf);
			cbuf[clen] = ' ';
			cbuf[++clen] = 0;
			yylval = (hack)setsymtab(cbuf, s, 0L, CON|STR, symtab);
			return STRING;
		}
		if (c == '\n' || c == 0) {
			yyerror("newline in string");
			lexstate = 0;
			return 0;
		}
		if (c == '\\') {
			c = getch();
			if (c == '"')
				cbuf[clen++] = '"';
			else if (c == 'n')
				cbuf[clen++] = '\n';
			else if (c == 't')
				cbuf[clen++] = '\t';
			else if (c == '\\')
				cbuf[clen++] = '\\';
			else
				cbuf[clen++] = c;
		} else
			cbuf[clen++] = c;
	}
}

/*
 * Regex body.  Lexed from lexstate == 3.  Returns the regex token
 * stream (CHAR CCL NCCL DOT STAR PLUS QUEST OR and ^ $ ( )), ending
 * when the closing '/' is seen (pushed back for the parser).
 */
int
yylexreg()
{
	int	c, c2, v;

	c = getch();
	switch (c) {
	case '[':
		lexstate = 2;
		clen = 0;
		cflag = 0;
		return yylexchc();
	case '^':
		c2 = getch();
		if (c2 == '[') {
			lexstate = 2;
			clen = 0;
			cflag = 1;
			return yylexchc();
		}
		ungetch(c2);
		return '^';
	case '?':
		return QUEST;
	case '+':
		return PLUS;
	case '*':
		return STAR;
	case '|':
		return OR;
	case '.':
		return DOT;
	case '(':
	case ')':
	case '$':
		return c;
	case '\\':
		c2 = getch();
		if (isdig(c2)) {	/* \ddd octal */
			v = c2 - '0';
			c2 = getch();
			if (isdig(c2)) {
				v = v * 8 + (c2 - '0');
				c2 = getch();
				if (isdig(c2)) {
					v = v * 8 + (c2 - '0');
					c2 = getch();
				}
			}
			ungetch(c2);
			yylval = v;
			return CHAR;
		}
		if (c2 == 'n')
			yylval = '\n';
		else if (c2 == 't')
			yylval = '\t';
		else
			yylval = c2;
		return CHAR;
	case '/':
		lexstate = 0;
		ungetch('/');
		return 0;	/* end of regex; parser sees the '/' next */
	case '\n':
		yyerror("newline in regular expression");
		lexstate = 0;
		return 0;
	default:
		yylval = c;
		return CHAR;
	}
}

/*
 * Character class body ([...] or [^...]).  Lexed from lexstate == 2.
 */
int
yylexchc()
{
	int	c;

	for (;;) {
		c = getch();
		if (c == '\\') {
			c = getch();
			if (c == ']')
				cbuf[clen++] = ']';
			else if (c == 'n')
				cbuf[clen++] = '\n';
			else if (c == 't')
				cbuf[clen++] = '\t';
			else
				cbuf[clen++] = c;
		} else if (c == ']') {
			lexstate = 3;
			cbuf[clen] = 0;
			yylval = (hack)tostring(cbuf);
			return cflag ? NCCL : CCL;
		} else if (c == '\n' || c == 0) {
			yyerror("newline in character class");
			lexstate = 0;
			return 0;
		} else
			cbuf[clen++] = c;
	}
}
