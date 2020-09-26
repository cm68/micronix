/*************************************************
 *						 *
 * Q/C V3.1 Utility Library			 *
 * Copyright (c) 1983 Quality Computer Systems	 *
 * 						 *
 * 07/21/83					 *
 *************************************************/

#include "qstdio.h"

/* format a string */
sprintf(nargs)
    int		    nargs;
{
    int		    _cat   ();	/* function _fmt will use as output */
    register       *p;
    register char  *s;

    p = &nargs + nargs;		/* address of first arg (string) */
    s = *p;			/* point to the string */
    *s = '\0';			/* initialize the string */
    _fmt(_cat, s, p - 1);
}

/*
 * add a character to the end of a string (internal routine for sprintf)
 */
_cat(c, s)
    char	    c     , *s;
{
    while (*s++);		/* find end of string */
    *(s - 1) = c;		/* add c at old end */
    *s = '\0';			/* new end of string */
}

/*
 * internal routine for printf/fprintf/sprintf - do actual formatting
 */
_fmt(func, dest, pargs)
    int             (*func) (), dest;	/* output is sent to dest by calling
					 * func */
    char           *pargs;
{
    register char  *s, *fmt;
    register int    c, base, *arg;
    char	    num    [7];	/* hold largest number */
    char           *itob();

    arg = pargs;
    fmt = *arg--;		/* first arg is the format */
    while (c = *fmt++) {
	if (c != '%') {		/* ordinary character */
	    (*func) (c, dest);
	    continue;
	}
	switch (c = *fmt++) {	/* handle conversion character */
	case '\0':		/* early end of format */
	    return;
	case 'o':		/* octal number */
	    base = 8;
	    goto convert;
	case 'x':		/* hex number */
	    base = 16;
	    goto convert;
	case 'u':		/* unsigned decimal number */
	    base = 10;
	    goto convert;
	case 'd':		/* signed decimal number */
	    base = -10;
    convert:
	    s = itob(*arg--, num, base);
	    while (*s)
		(*func) (*s++, dest);
	    continue;
	case 'c':		/* format a single character */
	    (*func) (*arg--, dest);
	    continue;
	case 's':		/* format a string */
	    s = *arg--;
	    while (*s)
		(*func) (*s++, dest);
	    continue;
	default:		/* not a format; just copy c */
	    (*func) (c, dest);
	    continue;
	}
    }
}

/*
 * convert an integer to a string in any base (2-36)
 */
char           *
itob(n, s, base)
    int		    n     , base;
    char           *s;
{
    register unsigned int u;
    register char  *p, *q;
    register	    negative, c;

    if (n < 0 && base == -10) {
	negative = TRUE;
	u = -n;
    } else {
	negative = FALSE;
	u = n;
    }
    if (base == -10)		/* signals signed conversion */
	base = 10;
    p = q = s;
    do {			/* generate digits in reverse order */
	if ((*p = u % base + '0') > '9')
	    *p += ('A' - ('9' + 1));
	++p;
    } while ((u /= base) > 0);
    if (negative)
	*p++ = '-';
    *p = '\0';			/* terminate the string */
    while (q < --p) {		/* reverse the digits */
	c = *q;
	*q++ = *p;
	*p = c;
    }
    return s;
}

/*
 * convert an ASCII decimal string to integer
 */
int
atoi(s)
    register char  *s;
{
    register int    n, sign;

    while (isspace(*s))
	++s;
    sign = 1;
    if (*s == '-' || *s == '+')
	sign = (*s++ == '+') ? 1 : -1;
    n = 0;
    while (isdigit(*s))
	n = n * 10 + *s++ - '0';
    return (sign * n);
}

/*
 * add second string to end of first string
 */
char *
strcat(s1, s2)
    char           *s1, *s2;
{
    register char  *p;
    char           *strcpy();

    p = s1;
    while (*p)			/* find end of first string */
	++p;
    strcpy(p, s2);		/* copy 2nd string at end of 1st */
    return s1;
}

/*
 * copy second string into first string
 */
char *
strcpy(s1, s2)
    char           *s1, *s2;
{
    register char  *p;

    p = s1;
    while (*p++ = *s2++)	/* copy 2nd string into 1st */
	;
    return s1;
}

/*
 * copy second string into first string & point to end
 */
char *
strmov(s1, s2)
    char           *s1, *s2;
{
    register char  *p;

    p = s1;
    while (*p = *s2++)		/* copy 2nd string into 1st */
	++p;
    return p;
}

/*
 * find the length of a string
 */
int
strlen(s)
    char           *s;
{
    register int    len;

    len = 0;
    while (*s++)
	++len;
    return len;
}

/*
 * compare two strings
 */
int
strcmp(s1, s2)
    register char  *s1, *s2;
{
    while (*s1 == *s2) {
	if (*s1 == '\0')
	    return 0;		/* they're equal to the end */
	++s1;
	++s2;
    }
    return (*s1 - *s2);
}

/*
 * convert a lower case letter to upper case
 */
int
chupper(c)
    register	    c;
{
    return (islower(c) ? c - ('a' - 'A') : c);
}

/*
 * convert an upper case letter to lower case
 */
int
chlower(c)
    register	    c;
{
    return (isupper(c) ? c - ('A' - 'a') : c);
}

/*
 * Is c a punctation character?
 */
int
ispunct(c)
    int		    c;
{
    return (isprint(c) && !isalnum(c) && c != ' ');
}

/*
 * Is c an ASCII control character?
 */
int
iscntrl(c)
    int		    c;
{
    return (isascii(c) && !isprint(c));
}

/*
 * Is c a letter or a number
 */
int
isalnum(c)
    int		    c;
{
    return (isalpha(c) || isdigit(c));
}

/*
 * Is c a letter
 */
int
isalpha(c)
    int		    c;
{
    return (isupper(c) || islower(c));
}

/*
 * Is c an upper case letter
 */
int
isupper(c)
    int		    c;
{
    return (c >= 'A' && c <= 'Z');
}

/*
 * Is c a lower case letter
 */
int
islower(c)
    int		    c;
{
    return (c >= 'a' && c <= 'z');
}

/*
 * Is c a decimal digit
 */
int
isdigit(c)
    int		    c;
{
    return (c >= '0' && c <= '9');
}

/*
 * Is c white space
 */
int
isspace(c)
    register	    c;
{
    return (c == ' ' || c == '\t' || c == '\n'
	    || c == '\r' || c == '\f');
}

/*
 * Is c an ASCII character?
 */
int
isascii(c)
    int		    c;
{
    return (c >= '\0' && c <= '\177');
}

/*
 * Is c a printable character?
 */
int
isprint(c)
    int		    c;
{
    return (c >= ' ' && c <= '~');
}

/*
 * find the first occurrence of a given character
 */
char *
index(s, c)
    register char  *s;
    int		    c;
{
    while (*s) {
	if (*s == c)
	    return s;
	++s;
    }
    return NULL;
}

/*
 * return upper case form of c even if c is not a letter
 */
int
toupper(c)
    int		    c;
{
    return (c - ('a' - 'A'));
}

/*
 * return lower case form of c even if c is not a letter
 */
int
tolower(c)
    int		    c;
{
    return (c - ('A' - 'a'));
}

/*
 * find the minimum of two integers
 */
int
imin(i, j)
    int		    i     , j;
{
    return (i < j) ? i : j;
}

/*
 * find the maximum of two integers
 */
int
imax(i, j)
    int		    i     , j;
{
    return (i > j) ? i : j;
}

/*
 * get the byte at address
 */
char
peek(address)
    char           *address;
{
    return (*address & 0xFF);
}

/*
 * put value in the byte at address
 */
char
poke(address, value)
    char           *address, value;
{
    int		    oldval;

    oldval = (*address & 0xFF);
    *address = value;
    return oldval;
}

/*
 * get the word at address
 */
int
wpeek(address)
    int            *address;
{
    return *address;
}

/*
 * put value in the word at address
 */
int
wpoke(address, value)
    int            *address, value;
{
    int		    oldval;

    oldval = *address;
    *address = value;
    return oldval;
}
/* end of CUTILIB.C */
