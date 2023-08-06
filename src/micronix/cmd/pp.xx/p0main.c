/*
 * DEFINE/INCLUDE PREPROCESSOR FOR C
 * copyright (c) 1978 by Whitesmiths, Ltd.
 *
 * pp/p0main.c
 *
 * Changed: <2023-07-01 21:47:37 curt>
 */

/*
 * extensions are -I -D syntax
 */

#include <stdio.h>
#include "int0.h"

/*	DATA FLOW:
 *	getl(line)->
 *	getln(logical line)-> getin(includes)->
 *	getex(expanded)-> main()-> putgr(if group)->
 *	putns(non skipped)-> putls(less sharps)-> lex
 */

static char *ofile = { NULL };

static unsigned char cflag = { 0 };
extern unsigned char xflag = { 0 };
extern unsigned char v6flag = { 0 };
extern unsigned short pchar = { '#' };
extern unsigned short schar = { '@' };

/*
 * we seed the include path with the current directory
 */
struct incpath dotdir = {
    0, "./"
};

struct incpath *incs = &dotdir;

/*
 *	FILE CONTROL:
 *	pincl-> current I/O structure
 *	nerrors = no of errors seen
 *	pflag => name has changed
 */

extern struct incl *pincl = { NULL };
extern short nerrors = { 0 };
extern unsigned char pflag = { 0 };

/*
 *	the table of predefined #keywords
 */
#define NPPS	18
static struct pretab pptab[] = {
	"\2IF", PIF,
	"\2if", PIF,
	"\4ELSE", PELSE,
	"\4LINE", PLINE,
	"\4else", PELSE,
	"\4line", PLINE,
	"\5ENDIF", PENDIF,
	"\5IFDEF", PIFDEF,
	"\5UNDEF", PUNDEF,
	"\5endif", PENDIF,
	"\5ifdef", PIFDEF,
	"\5undef", PUNDEF,
	"\6DEFINE", PDEFINE,
	"\6IFNDEF", PIFNDEF,
	"\6define", PDEFINE,
	"\6ifndef", PIFNDEF,
	"\7include", PINCLUD,
	"\7INCLUDE", PINCLUD,
};

/*
 *	get an expanded token list
 */
struct tlist *
getex()
{
	extern unsigned short pchar, schar;
	extern struct pretab pptab[];
	register struct tlist *p;
	register int tok;

	if (p = getin()) {
		if (punct(p, pchar) || punct(p, schar)) {
			if (tok = scntab(pptab, NPPS, p->next->text, p->next->ntext)) {
				p = lfree(p, p->next);
				p->type = tok;
			} else
				p->type = PSHARP;
		}
		switch (p->type) {
		case PDEFINE:
		case PUNDEF:
		case PINCLUD:
		case PSHARP:
			break;
		case PIFDEF:
		case PIFNDEF:
			if (p->next->type != PIDENT)
				errmsg("bad #%s", p->text, p->ntext);
			else if (!lookup(p->next->text, p->next->ntext))
				p->next = frelst(p->next, NULL);
			break;
		default:
			p = doexp(p);
		}
	}
	return (p);
}

/*
 * get an included line, as a token list
 */
struct tlist *
getin()
{
	extern unsigned char pflag;
	extern struct incl *pincl;
    extern struct incl *nxtfile();
	register char *s;

	for (;;) {
		if (!pincl)
			pincl = nxtfile();
		if (!pincl)
			return (NULL);
		else if (s = getln(pincl))
			return (stotl(s));
		else {
			fclose(&pincl->file);
			free(pincl->fname);
			pincl = lfree(pincl, pincl->next);
			pflag = YES;
		}
	}
}

/*
 * get a full line into the I/O buffer
 *  sans continuations and comments
 */
char *
getln(pi)
struct incl *pi;
{
	extern unsigned char cflag;
	register short i;
	register FILE *pf;
	register char *s;
	short j, k;
	static char buf[BUFSIZE];
	char strchar, *savs;

	strchar = '\0';
	s = buf;
	pf = &pi->file;
	for (i = getl(pf, buf, BUFSIZE); 0 < i;)
		if (!cflag && *s == '\\' && 1 < i && s[1] == '\n') {
			++pi->nline;
			i = getl(pf, s, BUFSIZE - (s - buf));
			for (savs = s; 0 < i && *s != '\n' && iswhite(*s); --i)
				++s;
			if (savs != s) {
				cpybuf(savs, s, i);
				s = savs;
			}
		} else if (!cflag && *s == '\\') {
			s += 2;
			i -= 2;
		} else if (!cflag && !strchar && *s == '/' && 1 < i && s[1] == '*') {
			for (j = 2; j < i - 1; ++j)
				if (s[j] == '*' && s[j + 1] == '/')
					break;
			if (j < i - 1) {
				*s++ = ' ';
				i -= j + 2;
				for (k = 0; k < i; ++k)
					s[k] = s[k + j + 1];
			} else if ((i = getl(pf, s + 2, BUFSIZE - (s + 2 - buf))) <= 0) {
				errmsg("missing */");
				break;
			} else {
				++pi->nline;
				i += 2;
			}
		} else if (*s == '\n') {
			++pi->nline;
			return (buf);
		} else {
			if (!cflag && (*s == '"' || *s == '\'')) {
				if (strchar == *s)
					strchar = '\0';
				else if (!strchar)
					strchar = *s;
				else;
			}
			++s;
			--i;
		}
	if (s == buf)
		return (NULL);
	else {
		errmsg("truncated line");
		buf[BUFSIZE - 1] = '\n';
		return (buf);
	}
}

/*
 * add s to our include possibilities
 * chase down the chain, leaving p pointing at the tail
 * note that all these strings are on the main's stack
 * so we don't need to strdup them unless they don't end 
 * in a slash.
 * since we call this both from a -i and -I, we support
 * an odd feature: foo|bar|baz puts all three into the
 * the include path.
 */
add_incpath(path)
char *path;
{
    char *s;
    int i;
    struct incpath **p = &incs;

    /* find the tail */
    while (*p) {
        p = &((*p)->next);
    }
    while (*path) {
        *p = alloc(sizeof(struct incpath)); 
        for (i = 0; path[i] && path[i] != '|'; i++)
            ;
        if (path[i - 1] == '/') {
            (*p)->path = path;
            path += i;
        } else {
            s = (*p)->path = alloc(i+1);
            while (--i) {
                *s++ = *path++;
            }
            *s++ = '/';
            *s++ = '\0';
        }
        (*p)->next = 0;
        if (*path == '|') path++;
    }
}

char *progname = 0;

usage(c)
char c;
{
    fprintf(stderr, "%s: usage\n", progname);
    fprintf(stderr, "-c\tdon't drop comments, continues\n");
    fprintf(stderr, "-d name\tdefine name as 1\n");
    fprintf(stderr, "-i dir[|dir..]\tinclude path\n");
    fprintf(stderr, "-o name\toutput to name\n");
    fprintf(stderr, "-px\tuse x instead of #\n");
    fprintf(stderr, "-sx\tuse x in addition to px\n");
    fprintf(stderr, "-x\toutput lexemes\n");
    fprintf(stderr, "-6\tput SOH, extra newlines for v6 compiler\n");
    fprintf(stderr, "-Dname[=value]\n");
    fprintf(stderr, "-I<dir>\tinclude path\n");
    if (c)
        fprintf(stderr, "unknown option %c\n", c);
    exit(1);
}

char **sources = 0;
int srcfiles = 0;

#define MAXDEFS 10
char *defines[MAXDEFS] = 0;
int ndefs = 0;

/*
 * expand define/include/if, perform lexical analysis
 * we reuse the argv array to contain the list of input files
 */
unsigned char
main(argc, argv)
short argc;
char **argv;
{
	extern unsigned char cflag, xflag, v6flag;
	extern unsigned short pchar, schar;
	extern short nerrors;
	extern struct incl *pincl;
    extern struct incl *predef();
	register struct tlist *p;
    unsigned char c;
    char *s;
    char *v;

    progname = argv[0];
    sources = argv;
    argv++;
    argc--;

    while (argc) {
        if ((*(s = *argv)) == '-') {
            while ((c = *++s)) switch (c) {
            case 'c':
                cflag=1;
                break;
            case 'x':
                xflag=1;
                break;
            case '6':
                v6flag=1;
                break;
            case 'p':
                pchar = *++s;
                break;
            case 's':
                schar = *++s;
                break;
            case 'd':
                defines[ndefs++] = *++argv;
                argc--;
                s += strlen(s) - 1;
                break;
            case 'o':
                ofile = *++argv;
                argc--;
                s += strlen(s) - 1;
                break;
            case 'i':
                add_incpath(*++argv);
                argc--;
                s += strlen(s) - 1;
                break;
            case 'D':
                defines[ndefs++] = s;
                s += strlen(s) - 1;
                break;
            case 'I':
                add_incpath(++s);
                s += strlen(s) - 1;
                break;
            default:
                usage(c);
                break;
            }
        } else {
            sources[srcfiles++] = *argv;
        }
        argv++;
        argc--;
    }

    if (ndefs > MAXDEFS) {
        fprintf(stderr, "no more than %d command line defs\n", MAXDEFS);
        exit(1);
    }
    /*
     * the first input file is the defines from the command line.
     */
	pincl = predef();

	if (ofile) {
		if (!freopen(ofile, 'w', stdout)) {
			error("bad output file");
		}
    }
	while (p = putgr(getex(), NO)) {
		errmsg("misplaced #%s", p->text, p->ntext);
		frelst(p, NULL);
	}
	fclose(stdout);
	return (nerrors == 0);
}

/*
 *	put an if group
 */
struct tlist *
putgr(p, skip)
register struct tlist *p;
register unsigned char skip;
{
	register unsigned char doit;

	while (p && p->type != PELSE && p->type != PENDIF)
		if (p->type != PIF && p->type != PIFDEF && p->type != PIFNDEF) {
			if (skip)
				frelst(p, NULL);
			else
				putns(p);
			p = getex();
		} else {
			if (p->type == PIF)
				doit = skip ? NO : eval(p->next);
			else if (p->type == PIFDEF)
				doit = (p->next != 0);
			else
				doit = (p->next == 0);
			frelst(p, NULL);
			p = putgr(getex(), skip || !doit);
			if (p && p->type == PELSE) {
				frelst(p, NULL);
				p = putgr(getex(), skip || doit);
			}
			if (p && p->type == PENDIF) {
				frelst(p, NULL);
				p = getex();
			} else
				errmsg("missing #endif");
		}
	return (p);
}

/*
 *	put a non skipped token line
 */
putns(p)
register struct tlist *p;
{
	extern unsigned char pflag;
	extern struct incl *pincl;
	register char *fname;
	register struct tlist *q;
	short fd;
	short i;
    struct incpath *ip;
    char *s;
    static char namebuf[80];

	switch (p->type) {
	case PDEFINE:
		if (p->next->type != PIDENT)
			errmsg("bad #define");
		else {
			for (q = p->next; q->next; q = q->next);
			install(p->next->text, p->next->ntext,
					buybuf(p->next->next->white,
						   q->text + q->ntext - p->next->next->white));
		}
		break;
	case PUNDEF:
		if (p->next->type != PIDENT)
			errmsg("bad #undef");
		else
			undef(p->next->text, p->next->ntext);
		break;
	case PINCLUD:
        /* if <foo>, don't look in dot */
        if (p->next->type == PSTRING) {
            fname = alloc(p->next->ntext - 1);
            ip = incs;
        } else if (p->next->ntext == 1 && p->next->text == '<') {
            ip = incs->next;
        }
        /* find the file */
        while (ip) {
            s = ip->path;
            for (i = 0; (namebuf[i] = s[i]); i++)
                ;
            for (s = p->next->text; *s; s++) {
                if (*s == '>') break;
                namebuf[i++] = *s;
            }
            namebuf[i] = '\0';
            if (access(namebuf, 4) == 0) {
                break;
            }
            ip = ip->next;
        }
        if (!ip) {
            errmsg("cannot find #include file %s", fname);
            break;
        }
		pincl = lalloc(sizeof(*pincl), pincl);
		pincl->fname = fname;
		pincl->nline = 0;
        pincl->file = fdopen(fd, "r");
		pflag = YES;
		break;
	case PLINE:
		if (p->next->type != PNUM)
			errmsg("bad #line");
		else {
			btos(p->next->text, p->next->ntext, &i, 10);
			pincl->nline = i;
			if (fname = getfname(p->next->next)) {
				pflag = YES;
				if (pincl->fname)
					free(pincl->fname);
				pincl->fname = fname;
			}
		}
		break;
	case PSHARP:
		if (p->next->type != PEOL)
			errmsg("bad #xxx");
		break;
	default:
		putls(p);
	}
	frelst(p, NULL);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
