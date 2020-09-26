/********************************************************/
/*							*/
/*		Q/C V3.2 Utility Library		*/
/*							*/
/*     Copyright (c) 1984 Quality Computer Systems	*/
/*							*/
/*			01/19/84			*/
/********************************************************/

#include "qstdio.h"

/*
 * If PORTABLE is defined, then only C versions of library routines
 * are used. Otherwise, assembler versions are used where available.

#define PORTABLE 1
*/

/* Get formatted input from a string */
sscanf(nargs)
	{
	static char *p, *fmt, **pargs, *s;
	static count, c, f, quit, suppress, width,
		base, d, maxd, sign, value;

	pargs = (char **)(&nargs + nargs - 1);
	p = *(pargs + 1);
	fmt = *pargs;
	count = 0;
	quit = FALSE;
	while (f = *fmt++) {
		if (isspace(f)) 	/* skip white space in format */
			continue;
		if (f != '%') { 	/* must match next input string */
			while (isspace(c=*p++))
				;
			if (f == c)
				continue;
			else {		/* doesn't match so put it back */
				--p;
				break;
				}
			}
		if (*fmt == '*') {
			suppress = TRUE;
			++fmt;
			}
		else
			suppress = FALSE;
		width = (isdigit(*fmt))? _atoi(&fmt): -1;
		if (*fmt == 'h')	/* ignore short int modifier */
			++fmt;
		switch (chupper(*fmt++)) {
		case '%':
			while (isspace(c=*p++))
				;
			if (c == '%')
				continue;
			else {
				quit = TRUE;
				break;
				}
		case 'X':
			base = 16;
			maxd = 'F';
			sign = 1;
			goto getnum;
		case 'O':
			base = 8;
			maxd = '7';
			sign = 1;
			goto getnum;
		case 'D':
			base = 10;
			maxd = '9';
			if (c == '-') {
				sign = -1;
				c = *p++;
				}
			else
				sign = 1;
		getnum:
			while (isspace(c=*p++))
				;
			d = chupper(c);
			if (isalnum(d) && d <= maxd) {
				if (!suppress)
					++count;
				}
			else {
				quit = TRUE;
				break;
				}
			value = 0;
			while (width--) {
				d = chupper(c);
				if (isdigit(d) && d <= maxd)
					d -= '0';
				else if (isalpha(d) && d <= maxd)
					d -= ('A' - 10);
				else
					break;
				value = base * value + d;
				c = *p++;
				}
			if (!suppress)
				*(int *)*--pargs = sign * value;
			break;
		case 'C':
			if ((c=*p++) == '\0')
				break;
			if (suppress) {
				if (width == -1)
					continue; /* no char to unget */
				while (width--) {
					if ((c=*p++) == '\0')
						break;
					}
				break;
				}
			++count;
			if (width == -1) { /* get single char */
				**--pargs = c;
				continue;
				}
			else {		/* get string */
				s = *--pargs;
				while (width--) {
					*s++ = c;
					if ((c=*p++) == '\0')
						break;
					}
				*s = '\0';
				}
			break;
		case 'S':
			while (isspace(c=*p++))
				;
			if (c == '\0')
				break;
			if (!suppress) {
				++count;
				s = *--pargs;
				}
			while (width--) {
				if (!suppress)
					*s++ = c;
				if (isspace(c=*p++) || c == '\0')
					break;
				}
			if (!suppress)
				*s = '\0';
			break;
		default:		/* can't figure this out */
			quit = TRUE;
			break;
			}
		if (c == '\0' || quit == TRUE)
			break;
		--p;			/* go back to get the last char */
		}
	return ((count == 0 && c == '\0')? EOF: count);
	}
/* format a string */
sprintf(nargs)
	{
	int _cat();		/* function _fmt will use as output */
	register *p;
	register char *s;

	p = &nargs + nargs;	/* address of first arg (string) */
	s = *p; 		/* point to the string */
	*s = '\0';		/* initialize the string */
	_fmt(_cat, s, p - 1);
	}
/* add a character to the end of a string (internal routine for sprintf) */
_cat(c, s)
char c, *s;
	{
	while (*s++)
		;		/* find end of string */
	*(s - 1) = c;		/* add c at old end */
	*s = '\0';		/* new end of string */
	}

/* Internal routine for printf/fprintf/sprintf

	Full K&R 7.3 except for long and float

	Original from the C Users Group, Volume III
	Modified for Q/C by Lyle P. Bickley, Phila., PA
	Further modifications by Jim Colvin
*/

#define MAXFLD 128

_fmt(func,dest,pargs)
register (*func)(),dest;
char *pargs;
{
	static char *format, *sptr;
	register char *wptr;
	register c, *args;
	static base, pf, ljf, padchr, width, precision;
	char wbuf[MAXFLD], *itob();

	args = pargs;
	format = *args--;

	while (c = *format++) {
	    if (c == '%') {
		wptr = wbuf;
		precision = 6;
		ljf = pf = FALSE;

		if (*format == '-') {
			++format;
			ljf = TRUE;
			}
		padchr = (*format == '0') ? '0': ' ';
		width = (isdigit(*format)) ? _atoi(&format) : 0;
		if ((c = *format++) == '.') {
			precision = _atoi(&format);
			pf = TRUE;
			c = *format++;
			}
		switch(chupper(c)) {

		case 'D':
			base = -10; goto val;
		case 'U':
			base = 10; goto val;
		case 'X':
			base = 16; goto val;
		case 'O':
			base = 8;
		val:	itob(*args--, wptr, base);
			width -= strlen(wptr);
			goto pad2;
		case 'C':
			*wptr++ = *args--;
			--width;
			goto pad;
		case 'S':
			if (!pf) precision = MAXFLD;
			sptr = *args--;
			while (*sptr && precision) {
				*wptr++ = *sptr++;
				--precision;
				--width;
			}
		pad:	*wptr = '\0';
		pad2:	wptr = wbuf;
			if (!ljf)
				while (--width >= 0)
					(*func)(padchr,dest);
			while (*wptr)
				(*func)(*wptr++,dest);
			if (ljf)
				while (--width >= 0)
					(*func)(padchr,dest);
			break;

		default:
			(*func)(c,dest);
		}
	    }
	    else (*func)(c,dest);
	}
}
/* convert an integer to a string in any base (2-36) */
char *itob(n, s, base)
char *s;
	{
	register unsigned int u;
	register char *p, *q;
	register negative, c;

	if (n < 0 && base == -10) {
		negative = TRUE;
		u = -n;
		}
	else {
		negative = FALSE;
		u = n;
		}
	if (base == -10)	/* signals signed conversion */
		base = 10;
	p = q = s;
	do {			/* generate digits in reverse order */
		if ((*p = u % base + '0') > '9')
			*p += ('A' - ('9' + 1));
		++p;
		} while ((u /= base) > 0);
	if (negative)
		*p++ = '-';
	*p = '\0';		/* terminate the string */
	while (q < --p) {	/* reverse the digits */
		c = *q;
		*q++ = *p;
		*p = c;
		}
	return s;
	}
/* convert an ASCII decimal string to integer */
atoi(s)
register char *s;
	{
	register sign;
	char *p;

	while (isspace(*s))
		++s;
	sign = 1;
	switch (*s) {
	case '-':
		sign = -1;
	case '+':
		++s;
		break;
		}
	p = s;
	return (sign * _atoi(&p));
	}
/*
 *	internal routine used by atoi and _fmt to perform ascii-
 *	to-decimal conversion and update an associated pointer:
 */
_atoi(sptr)
char **sptr;
	{
	register n;
	register char *p;

	n = 0;
	p = *sptr;
	while (isdigit(*p))
		n = 10 * n + *p++ - '0';
	*sptr = p;
	return n;
	}
/* add second string to end of first string */
char *strcat(s1, s2)
char *s1, *s2;
	{
	register char *p;

	p = s1;
	while (*p)		/* find end of first string */
		++p;
	while (*p++ = *s2++)	/* copy 2nd string to end of 1st */
		;
	return s1;
	}
/* add at most n characters from second string to end of first string */
char *strncat(s1, s2, n)
char *s1, *s2;
	{
	register char *p;

	p = s1;
	while (*p)		/* find end of first string */
		++p;
	while (n--) {		/* copy 2nd string to end of 1st */
		if ((*p = *s2++) == '\0')
			break;
		else
			++p;
		}
	*p = '\0';
	return s1;
	}
#ifdef PORTABLE
/* copy second string into first string */
char *strcpy(s1, s2)
char *s1, *s2;
	{
	register char *p;

	p = s1;
	while (*p++ = *s2++)	/* copy 2nd string into 1st */
		;
	return s1;
	}
#else
char *strcpy() {
#asm 8080
	LXI	H,2	;get s2
	DAD	SP
	MOV	E,M
	INX	H
	MOV	D,M
	INX	H
	MOV	A,M	;get s1
	INX	H
	MOV	H,M
	MOV	L,A
	PUSH	H	;save s1 for return
?scpy1: LDAX	D	;*s2
	MOV	M,A	;copy to *s1
	ORA	A	;check for end-of-string '\0'
	JZ	?scpy2
	INX	D	;move to next char
	INX	H
	JMP	?scpy1
?scpy2: POP	HL	;return s1
	MOV	A,H
	ORA	L	;set Z flag
#endasm
	}
#endif

/* copy second string into first string & point to end */
char *strmov(s1, s2)
char *s1, *s2;
	{
	register char *p;

	p = s1;
	while (*p = *s2++)	/* copy 2nd string into 1st */
		++p;
	return p;
	}

#ifdef PORTABLE
/* find the length of a string */
strlen(s)
char *s;
	{
	register len;

	len = 0;
	while (*s++)
		++len;
	return len;
	}
#else
strlen() {
#	asm 8080
	LXI	H,2
	DAD	SP
	MOV	E,M	;get s in DE
	INX	H
	MOV	D,M
	LXI	H,0	;init length counter
?slen1: LDAX	D	;get next character
	ORA	A	;end of string?
	JZ	?slen2
	INX	D
	INX	H	;count one more
	JMP	?slen1
?slen2: MOV	A,H	;length in HL
	ORA	L	;set Z flag
#	endasm
	}
#endif

#ifdef PORTABLE
/* compare two strings */
strcmp(s1, s2)
register char *s1, *s2;
	{
	while (*s1 == *s2) {
		if (*s1 == '\0')
			return 0;	/* they're equal to the end */
		++s1; ++ s2;
		}
	return (*s1 - *s2);
	}
#else
strcmp() {
#	asm 8080
	LXI	H,2	;get args
	DAD	SP
	MOV	E,M	;s2
	INX	H
	MOV	D,M
	INX	H
	MOV	A,M	;s1
	INX	H
	MOV	H,M
	MOV	L,A
	XCHG		;reverse args
?scmp1: LDAX	D	;current char of s1
	SUB	M	;compute *s1-*s2
	JNZ	?scmp2	;they don't match here
	ORA	M	;end-of-string (0)?
	JZ	?scmp2	;they match to end-to-string
	INX	D	;next char of s1
	INX	H	;same for s2
	JMP	?scmp1	;loop
?scmp2: MOV	L,A	;set return value (*s1-*s2)
	RLC		;sign-extend into H
	SBB	A
	MOV	H,A
	ORA	L	;set Z flag
#	endasm
	}
#endif

#ifdef PORTABLE
/* copy one string to another for exactly n characters */
strncpy(s1, s2, n)
char *s1, *s2;
	{
	char *p;

	p = s1;
	while (--n >= 0) {
		if (*p++ = *s2) /* copy and check for end of s2 */
			++s2;	/* not at end so get next char */
		}
	return s1;
	}
#else
strncpy() {
#	asm 8080
	PUSH	B	;save stack frame
	LXI	H,4
	DAD	SP
	MOV	C,M	;get n
	INX	H
	MOV	B,M
	INX	H
	MOV	E,M	;get s2
	INX	H
	MOV	D,M
	INX	H
	MOV	A,M	;get s1
	INX	H
	MOV	H,M
	MOV	L,A
	PUSH	H	;save s1 for return
?sncy1: MOV	A,C	;check count
	ORA	B	;if zero, we're thru
	JZ	?sncy2
	DCX	B	;count down
	LDAX	D	;next character in s2
	MOV	M,A	;copy to s1
	INX	H
	ORA	A	;end of s2?
	JZ	?sncy1	;if so, pad s1 with zeros
	INX	D	;else, go to next char in s2
	JMP	?sncy1
?sncy2: POP	H	;return s1
	POP	B	;restore stack frame
	MOV	A,H	;set Z flag
	ORA	L
#	endasm
	}
#endif

#ifdef PORTABLE
strncmp(s1, s2, n)
register char *s1, *s2;
	{
	if (n <= 0)
		return 0;
	while (--n > 0 && *s1) {
		if (*s1 != *s2)
			break;
		++s1; ++s2;
		}
	return (*s1 - *s2);
	}
#else

strncmp()
	{
#	asm 8080
	PUSH	B	;save stack frame pointer
	LXI	H,4	;get args off stack
	DAD	SP
	MOV	C,M	;len
	INX	H
	MOV	B,M
	INX	H
	MOV	E,M	;s2
	INX	H
	MOV	D,M
	INX	H
	MOV	A,M	;s1
	INX	H
	MOV	H,M
	MOV	L,A
	XCHG		;reverse args
?sncm1: MOV	A,B	;check number of chars left
	ORA	C
	JZ	?sncm2	;strings match to specified length
	LDAX	D	;current char of s1
	SUB	M	;compute *s1-*s2
	JNZ	?sncm2	;they don't match here
	ORA	M	;end-of-string (0)?
	JZ	?sncm2	;they match to end-to-string
	INX	D	;next char of s1
	INX	H	;same for s2
	DCX	B	;count down chars to compare
	JMP	?sncm1	;loop
?sncm2: MOV	L,A	;set return value (*s1-*s2)
	RLC		;sign-extend into H
	SBB	A
	MOV	H,A
	ORA	L	;set Z flag
	POP	B
#	endasm
	}
#endif

/* find the first occurrence of a given character */
char *index(s, c)
register char *s;
	{
	while (*s) {
		if (*s == c)
			return s;
		++s;
		}
	return NULL;
	}
/* find the last occurrence of a character in a string */
char *rindex(s, c)
register char *s;
	{
	register char *p;

	p = NULL;
	while (*s) {
		if (*s == c)
			p = s;
		++s;
		}
	return p;
	}
/* convert a lower case letter to upper case */
chupper(c)
register c;
	{
	return ((c>='a' && c<='z') ? c - ('a' - 'A') : c);
	}
/* convert an upper case letter to lower case */
chlower(c)
register c;
	{
	return ((c>='A' && c<='Z') ? c - ('A' - 'a') : c);
	}
/* Is c a punctation character? */
ispunct(c)
	{
	return (isprint(c) && !isalnum(c) && c != ' ');
	}

#ifdef PORTABLE
/* Is c an ASCII control character? */
iscntrl(c)
	{
	return (isascii(c) && !isprint(c));
	}
/* Is c a letter or a number */
isalnum(c)
	{
	return (isalpha(c) || isdigit(c));
	}
/* Is c a letter */
isalpha(c)
	{
	return (isupper(c) || islower(c));
	}
/* Is c an upper case letter */
isupper(c)
	{
	return (c >= 'A' && c <= 'Z');
	}
/* Is c a lower case letter */
islower(c)
	{
	return (c >= 'a' && c <= 'z');
	}
/* Is c a decimal digit */
isdigit(c)
	{
	return (c >= '0' && c <= '9');
	}
/* Is c white space */
isspace(c)
register c;
	{
	return (c == ' ' || c == '\t' || c == '\n'
		|| c == '\r' || c == '\f');
	}
/* Is c an ASCII character? */
isascii(c)
	{
	return (c >= '\0' && c <= '\177');
	}
/* Is c a printable character? */
isprint(c)
	{
	return (c >= ' ' && c <= '~');
	}
#else
iscntrl() {
#	asm 8080
	LXI	H,2
	DAD	SP
	MOV	A,M
	CPI	32
	RC
	CPI	127
	JNZ	?ISCN1
	ORA	H
	RET
?ISCN1: XRA	A
	MOV	H,A
	MOV	L,A
#	endasm
	}
isalnum() {
#asm 8080
	LXI	H,2
	DAD	SP
	MOV	A,M
	CPI	48
	JC	?ISAN1
	CPI	58
	RC
	CPI	65
	JC	?ISAN1
	CPI	91
	RC
	CPI	97
	JC	?ISAN1
	CPI	123
	RC
?ISAN1: XRA	A
	MOV	H,A
	MOV	L,A
#endasm
	}
isalpha() {
#asm 8080
	LXI	H,2
	DAD	SP
	MOV	A,M
	CPI	65
	JC	?ISAL1
	CPI	91
	RC
	CPI	97
	JC	?ISAL1
	CPI	123
	RC
?ISAL1: XRA	A
	MOV	H,A
	MOV	L,A
#endasm
	}
isupper() {
#asm 8080
	LXI	H,2
	DAD	SP
	MOV	A,M
	CPI	65
	JC	?ISUP1
	CPI	91
	RC
?ISUP1: XRA	A
	MOV	H,A
	MOV	L,A
#endasm
	}
islower() {
#asm 8080
	LXI	H,2
	DAD	SP
	MOV	A,M
	CPI	97
	JC	?ISLO1
	CPI	123
	RC
?ISLO1: XRA	A
	MOV	H,A
	MOV	L,A
#endasm
	}
isdigit() {
#asm 8080
	LXI	H,2
	DAD	SP
	MOV	A,M
	CPI	48
	JC	?ISDG1
	CPI	58
	RC
?ISDG1: XRA	A
	MOV	H,A
	MOV	L,A
#endasm
	}
isspace() {
#	asm 8080
	LXI	H,2
	DAD	SP
	MOV	A,M
	CPI	9
	JZ	?ISSP1
	CPI	10
	JZ	?ISSP1
	CPI	32
	JZ	?ISSP1
	CPI	12
	JZ	?ISSP1
	CPI	13
	JZ	?ISSP1
	XRA	A
	MOV	H,A
	MOV	L,A
	RET
?ISSP1: ORA	H
#	endasm
	}
isascii() {
#asm 8080
	LXI	H,3
	DAD	SP
	MOV	A,M
	ORA	A
	JNZ	?ISAS1
	DCX	H
	MOV	A,M
	CPI	128
	RC
?ISAS1: XRA	A
	MOV	H,A
	MOV	L,A
#endasm
	}
isprint() {
#asm 8080
	LXI	H,2
	DAD	SP
	MOV	A,M
	CPI	32
	JC	?ISPR1
	CPI	127
	RC
?ISPR1: XRA	A
	MOV	H,A
	MOV	L,A
#endasm
	}
#endif
/* do upper case translation even if c is not a letter */
toupper(c)
	{
	return (c - ('a' - 'A'));
	}
/* do lower case translation even if c is not a letter */
tolower(c)
	{
	return (c - ('A' - 'a'));
	}
/* find the minimum of two integers */
imin(i, j)
	{
	return (i < j) ? i : j;
	}
/* find the maximum of two integers */
imax(i, j)
	{
	return (i > j) ? i : j;
	}
/* get the byte at address */
peek(address)
char *address;
	{
	return (*address & 0xFF);
	}
/* put value in the byte at address */
poke(address, value)
char *address, value;
	{
	int oldval;

	oldval = (*address & 0xFF);
	*address = value;
	return oldval;
	}
/* get the word at address */
wpeek(address)
int *address;
	{
	return *address;
	}
/* put value in the word at address */
wpoke(address, value)
int *address;
	{
	int oldval;

	oldval = *address;
	*address = value;
	return oldval;
	}
/* end of CUTILIB.C */
ing to end of first string */
char *strncat(s1, s2, n)
char *s1, *s2;
	{
	regist