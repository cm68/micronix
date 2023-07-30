/*
 * COLLECT FLAGS FROM COMMAND ARGUMENT LIST
 * copyright (c) 1981 by Whitesmiths, Ltd.
 *
 * cmd/whitesmith/lib/libws/getfl.c
 *
 * Changed: <2023-07-29 22:00:51 curt>
 *
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
#include <std.h>

#ifdef linux
#include <stdarg.h>
#endif

LOCAL COUNT putcw();

#define BRKLINE 60

/*
 *  shape of stacks for '>' type format
 */
#define ISTK struct intstak

struct intstak {
	BYTES ntop;
	TEXT *intval[1];			/* BOOL, BYTES, TEXT * */
};

#define LSTK struct lostak

struct lostak {
	BYTES ntop;
	LONG loval[1];
};

/*
 *  put a char with wraparound
 */
LOCAL COUNT
putcw(fd, pc, i)
FAST FILE fd;
FAST TEXT *pc;
FAST COUNT i;
{
	if (BRKLINE < i && (*pc == ' ' || *pc == '\t')) {
		write(fd, "\n\t", 2);
		return (4);
	} else {
		write(fd, pc, 1);
		return (i + 1);
	}
}

/*
 *  print usage and flags, and die
 */
LOCAL VOID
_error(mesg, fmt)
FAST TEXT *mesg, *fmt;
{
	FAST COUNT i;

	i = usage(NULL);

	for (; *mesg; ++mesg) {
		if (*mesg != 'F') {
			i = putcw(STDERR, mesg, i);
		} else {
			i += write(STDERR, "-[", 2);
			for (; *fmt && *fmt != ':'; ++fmt) {
				if (*fmt == ',' && fmt[1] != ':') {
					i = putcw(STDERR, " ", i);
				} else if (*fmt == '>') {
					i += write(STDERR, "^", 1);
					if (fmt[1] != ':')
						i = putcw(STDERR, " ", i);
				} else {
					i += write(STDERR, fmt, 1);
				}
			}
			i += write(STDERR, "]", 1);
		}
	}
	write(STDERR, "\n", 1);
	exit(NO);
}

/*
 * collect flags from command argument list
 * this function is not varargs friendly, so the linux implementation is
 * totally different
 */
TEXT *
#ifdef linux
getflags(BYTES *pac, TEXT ***pav, TEXT *fmt, ...)
#else
getflags(pac, pav, fmt, args)
BYTES *pac;
TEXT ***pav;
TEXT *fmt;
TEXT **args;
#endif
{
	FAST TEXT *f, *q, *r;
	BOOL done, islong, suc;
	COUNT v;
	LONG lo;
	TEXT ***p, *t, *val;

#ifdef linux
#define MAXARGS 20
    /*
     * snarf in the variable arguments into a local array
     */
    va_list ap;
    void *args[MAXARGS];
    int nargs = 0;
    char *s;
    va_start(ap, fmt);
    for (s = fmt; *s; s++) {
        if (*s == ',' || *s == ':' || *s == '\0') {
            args[nargs++] = va_arg(ap, void *);
        }
    }
    va_end(ap);
#endif

	for (++*pav; *pac && --*pac; ++*pav) {
		q = **pav;
		if (cmpstr(q, "--")) {
			--*pac;
			++*pav;
			break;
		} else if (cmpstr(q, "-") || (*q != '-' && *q != '+'))
			break;

		if (*q == '-')
			++q;

		if (cmpbuf("help", q, 4)) {
			TEXT *msg;

			if (q[4] == ':')
				msg = "plugh";
			else if (*(msg = fmt + scnstr(fmt, ':')))
				msg++;
			else
				msg = "F";
			_error(msg, fmt);
		}

		while (*q) {
			suc = NO;
			islong = NO;
			p = &args;
			r = q;
			for (f = fmt, done = NO; !done; ++f) {
				switch (*f) {

				case '*':
					suc = YES;
					val = (*r) ? r : (0 < --*pac) ? *++*pav : r;
					for (r = val, t = r; *t; ++t)
						*r++ = (*t == '\\') ? doesc(&t, NULL) : *t;
					*r = '\0';
					break;

				case '?':
					suc = YES;
					if (*r != '\\')
						val = *r;
					else {
						t = r;
						val = doesc(&t, NULL);
						r = t;
					}
					if (*r)
						++r;
					break;

				case '#':
					if (!*r)
						r = (0 < --*pac) ? *++*pav : r;
					r += btol(r, lenstr(r), &lo, 1);
					suc = !*r;
					if (!suc)
						f += scnstr(f, ':') - 1;
					else if (f[1] == '#') {
						++f;
						islong = YES;
					} else
						val = lo;
					break;

				case ',':
				case ':':
				case '\0':
					if (suc) {
						if (islong)
							**(LONG **) p = lo;
						else
							**p = val;
						q = r;
						done = YES;
					} else if (!*f)
						return (q);
					else if (*f == ':')
						_error(++f, fmt);
					else {
						++p;
						r = q;
					}
					break;

				case '>':
					if (!suc) {
						++p;
						r = q;
					} else if (!(**(ISTK **) p).ntop)
						if (f[v = scnstr(f, ':')])
							_error(&f[v + 1], fmt);
						else
							return (q);
					else {
						if (islong)
							(**(LSTK **) p).loval[--(**(LSTK **) p).ntop] =
								lo;
						else
							(**(ISTK **) p).intval[--(**(ISTK **) p).
												   ntop] = val;
						q = r;
						done = YES;
					}
					break;

				default:
					if (*f == *r) {
						suc = YES;
						val = YES;
						++r;
					} else {
						suc = NO;
						while (*f && *f != ',' && *f != '>' && *f != ':')
							++f;
						--f;
					}
					break;
				}
			}
		}
	}
	return (NULL);
}
