/*
 * PUT FORMATTED
 * copyright (c) 1978 by Whitesmiths, Ltd.
 *
 * cmd/whitesmith/lib/libws/cputf.c
 *
 * Changed: <2023-07-29 16:30:07 curt>
 *
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
#include <std.h>
#ifdef linux
#include <stdarg.h>
#endif

#define DBUF	24
#define DBL YES

#ifndef linux
/*
 * get a width count from string q.
 * if q == 'n' or 'N', then get a number from **ppp,
 * which presumably is the argument array
 * return q pointing at the first non-digit
 */
LOCAL TEXT 
*_gnum(q, pnum, ppp)
	FAST TEXT *q, ***ppp;
	FAST COUNT *pnum;
{
	if (*q == 'n' || *q == 'N') {
		++q;
		*pnum = ((COUNT)*(*ppp)++);
	} else {
		for (*pnum = 0; isdigit(*q); ++q)
			*pnum = *pnum * 10 + *q - '0';
	}
	return (q);
}
#endif

/*
 * put a formatted string to the output stream
 */
#ifdef linux
void 
_putf(COUNT (*outfn)(), TEXT *stream, TEXT *fmt, ...)
#else
VOID 
_putf(outfn, stream, fmt, pp)
	COUNT (*outfn)();
	TEXT *stream;
	FAST TEXT *fmt;
	BYTES *pp;
#endif
{
	FAST COUNT base;
	FAST TEXT *q;
	BOOL rjust;
	COUNT n, prec, width;
	LONG lo;
	TEXT buf[DBUF], cfill, mod, *s;

#ifdef linux
    va_list ap;
    va_start(ap, fmt);
#endif

	for (; ; fmt = q + 1) {

		/* skip to null or '%' */
		for (q = fmt; *q && *q != '%'; ++q)
			;

		/* send fixed characters */
		if (fmt < q)
			(*outfn)(stream, fmt, q - fmt);

		/* are we done, if not, skip over '%' */
		if (!*q++)
			return;

		/*
		 * get justification description if present
		 * -<fillchar>
		 * +<fillchar>
		 */
		rjust = YES;
		cfill = ' ';
		if (*q == '-') {
			rjust = NO;
			cfill = *++q;
			++q;
		} else if (*q == '+') {
			cfill = *++q;
			++q;
		}

		/*
		 * get field width and precision from fmt or args
		 * if absent, width = 0
		 * [<width>][.<precision>]
		 * where if field is == 'N' or 'n', fetched from args
		 * otherwise, decimal digits
		 */
#ifdef linux
        width = 0;
        if (*q == 'n' || *q == 'N') {
            q++;
            width = va_arg(ap, int);
        } else {
            while (isdigit(*q)) {
                width = width * 10 + (*q - '0');
                q++;
            }
        } 
        prec = 0;
        if (*q == '.') {
            q++;
            if (*q == 'n' || *q == 'N') {
                q++;
                width = va_arg(ap, int);
            } else {
                while (isdigit(*q)) {
                    prec = prec * 10 + (*q - '0');
                    q++;
                }
            }
        } 
#else
		q = _gnum(q, &width, &pp);

		prec = 0;
		if (*q == '.')
			q = _gnum(q + 1, &prec, &pp);
#endif
		/*
		 * next, get modifier
		 */
		if (*q == 'a' || *q == 'h' || *q == 'o' || *q == 'u')
			mod = *q++;
		else
			mod = '\0';

		/*
		 * most of these are numeric bases
		 */
		if (mod == 'h')
			base = 16;
		else if (mod == 'o')
			base = 8;
		else if (mod == 'u')
			base = 10;
		else
			base = -10;

		if (*q == 'x') {
			n = 0;
		} else if (*q == 'b') {
#ifdef linux
			s = va_arg(ap, char *);
			n = va_arg(ap, int);
#else
			s = *pp++;
			n = *pp++;
#endif
			if (prec)
				n = min(n, prec);
		} else if (*q == 'p') {
			/*
			 * if string, get string pointer and clamp it
			 */
#ifdef linux
			s = va_arg(ap, char *);
#else
			s = *pp++;
#endif
			n = lenstr(s);
			if (prec)
				n = min(n, prec);
		}
#if DBL
		/* if floating point types, format the value into s */
		else if (*q == 'f') {
			s = buf;
#ifdef linux
            n = sprintf(s, "%*.*f", width, prec, va_arg(ap, DOUBLE));
#else
			n = dtof(buf, *(DOUBLE *)pp, (DBUF-2) - prec, prec);
			pp = (DOUBLE *)pp + 1;
#endif
		} else if (*q == 'd') {
			s = buf;
#ifdef linux
            n = sprintf(s, "%*.*d", width, prec, va_arg(ap, DOUBLE));
#else
			n = dtoe(buf, *(DOUBLE *)pp, 1, min(DBUF-7, prec));
			pp = (DOUBLE *)pp + 1;
#endif
		}
#endif
		else if (*q == 'c' || *q == 's' || *q == 'i' || *q == 'l' || mod &&
			*--q)	/* ! */
			if (*q == 'l' || *q != 'c' && *q != 's' && sizeof (int) == sizeof (LONG))
				if (mod == 'a') {
#ifdef linux
                    n = sprintf("%04x", va_arg(ap, LONG));
#else
					for (lo = *(LONG *)pp, n = 4; 0 <= --n; lo >>= 8)
						buf[n] = lo;
					s = buf;
					n = 4;
					pp = (LONG *)pp + 1;
#endif
				} else {
#ifdef linux
                    n = sprintf("%04x", va_arg(ap, LONG));
#else
					s = buf;
					n = ltob(buf, *(LONG *)pp, base);
					pp = (LONG *)pp + 1;
#endif
				} 
			else {
				s = buf;
#ifdef linux
                n = va_arg(ap, int);
#else             
				n = *pp++;
#endif
				if (mod == 'a') {
					if (*q == 'c') {
						buf[0] = n;
						n = 1;
					} else {
						buf[0] = n >> 8;
						buf[1] = n;
						n = 2;
					}
				} else {
					if (base < 0)
						;
					else if (*q == 'c')
						n &= BYTMASK;
					else
						n &= 0177777;
#ifdef linux
                    n = sprintf(buf, (base == 8) ? "%o" : (base == 16) ? "%x" : "%d", n);    
#else
					n = stob(buf, n, base);
#endif
				}
			}
		else {
			s = q;
			n = 1;
		}

		if (rjust) {
			for (; n < width; --width)
				(*outfn)(stream, &cfill, 1);
        }

		if (0 < n)
			(*outfn)(stream, s, n);

		if (!rjust) {
			for (; n < width; --width)
				(*outfn)(stream, &cfill, 1);
        }
	}
    va_end(ap);
}

#ifdef TEST
wln(void *p, char *buf, int n)
{
    write(1, buf, n);
}

int
main()
{
    _putf(wln, 0, "%p %b %a\n", "foo", "foo", 2, 'a'); 
}
#endif

