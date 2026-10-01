/*
 * cal [month] [year]
 *
 * v7 cal (usr/src/cmd/cal.c), ported to micronix.
 *
 * cmd/cal/cal.c
 *
 * The calendar is v7's, unchanged: the same day line and month names,
 * the same 24 and 72 column string, the same pstr that blanks the
 * empty cells and trims the right end of each week, the same jan1 and
 * the same eighteen day September of 1752.  What is new is the
 * argument scan, because the micronix command cal (1) documents more
 * than v7's did:
 *
 *	v7		micronix
 *	cal [m] y	cal		whole year, this year
 *			cal year	whole year
 *			cal month	single month, this year
 *			cal month year	single month
 *
 * so a month may be spelled out, abbreviated or numbered, a year may
 * be left off to mean this year, and no arguments at all is this
 * year.  v7 printed a usage line instead, and its month had to be a
 * number.
 *
 * A month name is matched by prefix, case insensitively, first name
 * wins - "ju" is June and "ma" is March, which is what the 1982
 * command did; see month() below.  A number is read as a month only
 * where the page says so, when there are two arguments: "cal 8" is
 * the year 8 and "cal 8 2026" is August 2026.
 *
 * The year comes off the clock, through time and localtime, which is
 * also new - v7 had no default year to find.
 *
 * Two small things the port changed besides: the argument that is not
 * a year or a month now goes to the standard error and exits 1, where
 * v7 wrote "Bad argument" on the standard output and then fell off
 * the end of main; and month() is a new function, number() being
 * v7's.
 *
 * The month heading of the full-year calendar is positioned with tabs,
 * and ccc turns a tab typed inside a string into a blank - a string
 * literal keeps its spaces and loses its tabs - so those four formats
 * are spelled with \t escapes here.  v7's compiler took the literal
 * tab, which is what the original source has.
 *
 * vim: tabstop=8 shiftwidth=8 noexpandtab:
 */

#include <types.h>
#include <stdio.h>
#include <time.h>

char	dayw[] = " S  M Tu  W Th  F  S";
char	*smon[]= {
	"January", "February", "March", "April",
	"May", "June", "July", "August",
	"September", "October", "November", "December",
};
char	string[432];

main(argc, argv)
	int argc;
	char *argv[];
{
	register y, i, j;
	int m;

	m = 0;
	if (argc > 2) {
		m = month(argv[1]);
		if (m == 0) {
			m = number(argv[1]);
			if (m < 1 || m > 12)
				goto badarg;
		}
		y = number(argv[2]);
		if (y < 1 || y > 9999)
			goto badarg;
	} else if (argc == 2) {
		m = month(argv[1]);
		if (m == 0) {
			y = number(argv[1]);
			if (y < 1 || y > 9999)
				goto badarg;
		} else
			y = thisyear();
	} else
		y = thisyear();

	if (m == 0)
		goto xlong;

/*
 *	print out just month
 */

	printf("   %s %u\n", smon[m-1], y);
	printf("%s\n", dayw);
	cal(m, y, string, 24);
	for(i=0; i<6*24; i+=24)
		pstr(string+i, 24);
	exit(0);

/*
 *	print out complete year
 */

xlong:
	printf("\n\n\n");
	printf("\t\t\t\t%u\n", y);
	printf("\n");
	for(i=0; i<12; i+=3) {
		for(j=0; j<6*72; j++)
			string[j] = '\0';
		printf("\t %.3s", smon[i]);
		printf("\t\t\t%.3s", smon[i+1]);
		printf("\t\t       %.3s\n", smon[i+2]);
		printf("%s   %s   %s\n", dayw, dayw, dayw);
		cal(i+1, y, string, 72);
		cal(i+2, y, string+23, 72);
		cal(i+3, y, string+46, 72);
		for(j=0; j<6*72; j+=72)
			pstr(string+j, 72);
	}
	printf("\n\n\n");
	exit(0);

badarg:
	fprintf(stderr, "cal: bad argument\n");
	exit(1);
}

/*
 *	the year the clock says, for a calendar with no year named
 */
thisyear()
{
	time_t now;
	struct tm *tp;

	time(&now);
	tp = localtime(&now);
	return(tp->tm_year + 1900);
}

/*
 *	the month a name or an abbreviation names, 1 to 12, or 0.
 *	The first name the argument is a prefix of wins, so short
 *	abbreviations are ambiguous the way the 1982 command made
 *	them: "ju" is June, "ma" is March, "j" is January.
 */
month(s)
	char *s;
{

	register int i;

	if (*s == '\0')
		return(0);
	for (i = 0; i < 12; i++)
		if (prefix(s, smon[i]))
			return(i+1);
	return(0);
}

prefix(s, t)
	register char *s, *t;
{

	while (*s)
		if (lower(*s++) != lower(*t++))
			return(0);
	return(1);
}

lower(c)
	register int c;
{

	if (c >= 'A' && c <= 'Z')
		return(c - 'A' + 'a');
	return(c);
}

number(str)
	char *str;
{
	register n, c;
	register char *s;

	n = 0;
	s = str;
	while(c = *s++) {
		if(c<'0' || c>'9')
			return(0);
		n = n*10 + c-'0';
	}
	return(n);
}

pstr(str, n)
	char *str;
{
	register i;
	register char *s;

	s = str;
	i = n;
	while(i--)
		if(*s++ == '\0')
			s[-1] = ' ';
	i = n+1;
	while(i--)
		if(*--s != ' ')
			break;
	s[1] = '\0';
	printf("%s\n", str);
}

char	mon[] = {
	0,
	31, 29, 31, 30,
	31, 30, 31, 31,
	30, 31, 30, 31,
};

cal(m, y, p, w)
	char *p;
{
	register d, i;
	register char *s;

	s = p;
	d = jan1(y);
	mon[2] = 29;
	mon[9] = 30;

	switch((jan1(y+1)+7-d)%7) {

	/*
	 *	non-leap year
	 */
	case 1:
		mon[2] = 28;
		break;

	/*
	 *	1752
	 */
	default:
		mon[9] = 19;
		break;

	/*
	 *	leap year
	 */
	case 2:
		;
	}
	for(i=1; i<m; i++)
		d += mon[i];
	d %= 7;
	s += 3*d;
	for(i=1; i<=mon[m]; i++) {
		if(i==3 && mon[m]==19) {
			i += 11;
			mon[m] += 11;
		}
		if(i > 9)
			*s = i/10+'0';
		s++;
		*s++ = i%10+'0';
		s++;
		if(++d == 7) {
			d = 0;
			s = p+w;
			p = s;
		}
	}
}

/*
 *	return day of the week
 *	of jan 1 of given year
 */

jan1(yr)
{
	register y, d;

/*
 *	normal gregorian calendar
 *	one extra day per four years
 */

	y = yr;
	d = 4+y+(y+3)/4;

/*
 *	julian calendar
 *	regular gregorian
 *	less three days per 400
 */

	if(y > 1800) {
		d -= (y-1701)/100;
		d += (y-1601)/400;
	}

/*
 *	great calendar changeover instant
 */

	if(y > 1752)
		d += 3;

	return(d%7);
}
