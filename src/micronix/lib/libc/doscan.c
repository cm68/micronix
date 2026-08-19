/*
 * _doscan - implement scanf, fscanf, sscanf
 *
 */

#include	<types.h>
#include 	<ctype.h>

static int	(*gch)();
static int	(*ungch)();
static int	eofflag;
extern int	_atof();	/* stub in atof.c - see math.h */
extern int	atoi(char *);

/*
 * getch wraps the caller's get: EOF is remembered, so the return-value
 * logic can tell "nothing matched" from "end of input" without asking
 * the stream.
 */
static
getch()
{
	int	c;

	if ((c = (*gch)()) == EOF)
		eofflag = 1;
	return c;
}

static
range(c, base)
int	c;
uchar	base;
{
	if(isdigit(c))
		c -= '0';
	else
		{
		if (isupper(c))
			c = tolower(c) ;
		if (isalpha(c))
			c = c - 'a' + 10 ;
		else
			return -1 ;
		}
	if (c >= base)
		return -1 ;
	return c ;
}

static
wspace()
{
	int	c;

	while(isspace(c = getch()))
		continue;
	if(c != EOF)
		(*ungch)(c);
}

_doscan(get, unget, fmt, args)
int	(*get)();
int	(*unget)();
register char *	fmt;
int **		args;
{
	uchar	c, sign, base, n, noass,len;
	char	width ;
	char *	sptr;
	int	ch;
	long	val;
	char	buf[60];

	gch = get;
	ungch = unget;
	eofflag = 0;
	n = 0;
	while(c = *fmt++) {

		len = 0 ;
		if(isspace(c)) {
			wspace();
			continue;
		}
		if(c == '%') {
			noass = 0;
			width = 0;
loop:
			switch(c = *fmt++) {

			case '\0':
				return n ? n : eofflag ? EOF : 0;

			case '*':
				noass++;
				goto loop;

			case 'F':
				len++;
			case 'f':
				wspace();
				sptr = buf;
				if(width == 0)
					width = sizeof buf - 1;
				sign = 0;	/* really decimal point seen */
				ch = getch();
				if(ch == '-') {
					*sptr++ = ch;
					ch = getch();
					width--;
				}
				while(width && isdigit(ch) || !sign && ch == '.') {
					*sptr++ = ch;
					if(ch == '.')
						sign++;
					ch = getch();
					width--;
				}
				if(width && (ch == 'e' || ch == 'E')) {
					*sptr++ = ch;
					ch = getch();
					width--;
					if(width && (ch == '-' || ch == '+')) {
						*sptr++ = ch;
						ch = getch();
						width--;
					}
					while(width && isdigit(ch)) {
						*sptr++ = ch;
						ch = getch();
						width--;
					}
				}
				*sptr = 0;
				if(ch != EOF)
					(*ungch)(ch);
				if(sptr == buf)
					return n ? n : eofflag ? EOF : 0;
				n++;
				/*
				 * The digits were consumed and counted, but
				 * nothing is stored: there is no float type
				 * and so no _atof to convert to.  A program
				 * with its own representation (see math.h)
				 * links its own _atof taking the destination
				 * by address.
				 */
				if(!noass)
					_atof(buf, *args++);
				continue;

			case 'l':
				len++;
				goto loop;

			case 'D':
				len++;
			case 'd':
				base = 10;
				break;

			case 'O':
				len++;
			case 'o':
				base = 8;
				break;

			case 'X':
				len++;
			case 'x':
				base = 16;
				break ;

			case 's':
				wspace();
				if ( !noass )
					sptr = (char *)*args++;
				if ((ch = getch()) == EOF )
					return n ? n : EOF;
				while(ch && ch != EOF && !isspace(ch)) {
					if(ch == *fmt) {
						fmt++;
						break;
					}
					if ( !noass ) 
						*sptr++ = ch;
					if(--width == 0)
						break;
					ch = getch();
				}
				n++;
				if(!noass)
					*sptr = 0;
				continue;

			case 'c':
				if ( !noass )
					sptr = (char *)*args++;
				do {
					if ((ch = getch()) == EOF) 
						return n ? n : EOF;
					if ( !noass )
						*sptr++ = ch;
				} while(--width > 0);
				n++;
				continue;
			default:
				if(isdigit(c)) {
					width = atoi(fmt-1);
					while(isdigit(*fmt))
						fmt++;
					goto loop;
				}
				if(c != (ch = getch()))
					if(ch == EOF)
						return n ? n : EOF;
					else {
						(*ungch)(ch);
						return n;
					}
				continue;
			}
			wspace();
			val = 0;
			sign = 0;
			ch = getch();
			if(ch == '-') {
				sign++;
				ch = getch();
			}
			if(range(ch, base) == -1) {
				(*ungch)(ch);
				return n ? n : eofflag ? EOF : 0;
			}
			do {
				val = val * base + range(ch, base);
			} while (( --width != 0 ) && ( range(ch = getch(),base) != -1 )) ;
			n++;
			if (range(ch,base) == -1)
				(*ungch)(ch);
			if(sign)
				val = -val;
			if ( !noass )
				if(len)
					*(long *)*args++ = val;
				else
					**args++ = val;
			continue;
		} else if(c != (ch = getch())) {
			if(ch != EOF) {
				(*ungch)(ch);
				return n;
			} else
				return n ? n : EOF;
		}
	}
	return n;
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
