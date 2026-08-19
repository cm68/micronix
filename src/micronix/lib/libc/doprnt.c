/*
 * printf worker function
 *
 */

#include	<stdio.h>
#include	<ctype.h>

static uchar	ival;
static char *	x;
static FILE *	ffile;
extern int	atoi(char *);
extern int	strlen(char *);

static
pputc(c)
int	c;
{
	putc(c, ffile);
}

static char *
icvt(cp)
register char *	cp;
{
	ival = atoi(cp);
	while(isdigit((unsigned)*cp))
		cp++;
	return cp;
}

/*
 * %e %f %g hand the argument to _fnum by address, not by value.
 *
 * There is no float type here, and no one Z80 float format to pick:
 * the machines that shipped with a floating point unit disagreed, and
 * so did the BASIC ROMs.  A program that wants these conversions
 * declares its own representation (see math.h) and links its own
 * _fnum; libc's is a stub that prints '?'.  Such a program sets
 * FLTSIZE to the width of what varargs actually pushed.
 */
#ifndef FLTSIZE
#define FLTSIZE 4
#endif

_doprnt(file, f, a)
FILE *	file;
register char *		f;
int *		a;
{
	char	c, prec;
	uchar	fill, left, plus;
	int	i;
	uchar	base, width, sign, len;
	uchar	ftype;
	extern	short _pnum(), _fnum();

	ffile = file;
	while(c = *f++)
		if(c != '%')
			pputc(c);
		else {
			base = 10;
			width = 0;
			sign = 0;
			left = 0;
			ftype = 0;
			plus = 0;
			len = sizeof(int)/sizeof *a;
			/*
			 * Flags, in any order.  "+" was not among them, and
			 * an unknown one is worse here than being ignored:
			 * the switch below took the "+" as the conversion
			 * character, printed it, CONSUMED NO ARGUMENT, and
			 * left the "d" to come out as an ordinary letter.
			 * So "%+d" printed "+d" and every argument after it
			 * in the call was off by one - "[%+d] [%d] [%x]" of
			 * 4, 4, 255 gave "[+d] [4] [4]".  nm's disassembler
			 * writes "(iy%+d)" and got "(iy+d)".
			 */
			for (;;) {
				if(*f == '-') {
					f++;
					left++;
					continue;
				}
				if(*f == '+') {
					f++;
					plus++;
					continue;
				}
				break;
			}
			fill = *f == '0';
			if(isdigit((unsigned)*f)) {
				f = icvt(f);
				width = ival;
			} else if(*f == '*') {
				width = *a++;
				f++;
			}
			if(*f == '.')
				if(*++f == '*') {
					prec = *a++;
					f++;
				} else {
					f = icvt(f);
					prec = ival;
				}
			else
				prec = fill ? width : -1;
			if(*f == 'l') {
				f++;
				len = sizeof(long)/sizeof *a;
			}
			switch(c = *f++) {

			case 0:
				return;
			case 'o':
			case 'O':
				base = 8;
				break;
			case 'd':
			case 'D':
				/*
				 * 2 asks _pnum for a "+" on a value that is
				 * not negative.  Only the signed conversions
				 * take the flag, which is what C says.
				 */
				sign = plus ? 2 : 1;
				break;

			case 'x':
			case 'X':
				base = 16;
				break;

			case 's':
				x = *(char **)a;
				a += sizeof(char *)/sizeof *a;
				if(!x)
					x = "(null)";
				i = strlen(x);
dostring:
				if(prec < 0)
					prec = 0;
				if(prec && prec < i)
					i = prec;
				if(width > i)
					width -= i;
				else
					width = 0;
				if(!left)
					while(width--)
						pputc(' ');
				while(i--)
					pputc(*x++);
				if(left)
					while(width--)
						pputc(' ');
				continue;
			case 'c':
				c = *a++;
			default:
				x = &c;
				i = 1;
				goto dostring;

			case 'u':
			case 'U':
				break;

			case 'e':
			case 'E':
				sign++;

			case 'g':
			case 'G':
				sign++;

			case 'f':
			case 'F':
				if(prec < 0)
					prec = 6;
				ftype = 1;
				break;
			}
			if(left) {
				left = width;
				width = 0;
			}
			/*
			 * AN UPPER CASE CONVERSION MEANS LONG.  %X is long
			 * hex, %D long decimal, and so on - it is not the
			 * C meaning, where %X is upper case hex, and man3
			 * printf.3 documents only "dox" and never says so.
			 * Worth knowing before reading "%X" of an int and
			 * wondering why four bytes went by.
			 */
			if(isupper(c))
				len = sizeof(long)/sizeof *a;
			if(prec < 0)
				prec = 0;
			if(ftype) {
				width = _fnum((char *)a, prec, width, sign, pputc);
				a += FLTSIZE/sizeof(*a);
			} else {
				/*
				 * (unsigned)*a first: a is int *, so going
				 * straight to unsigned long sign-extends,
				 * and every unsigned conversion of a value
				 * with the top bit set printed eight digits
				 * with ffff in front.  The linker's load map
				 * called sexit "FFFF9A41".
				 */
				width = _pnum((len == sizeof(int)/sizeof *a ? (sign ? (long)*a : (unsigned long)(unsigned int)*a) : *(long *)a), prec, width, sign, base, pputc);
				a += len;
			}
			while(left-- > width)
				pputc(' ');
		}
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
