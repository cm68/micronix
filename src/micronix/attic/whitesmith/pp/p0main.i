 
 

 

 
typedef char TEXT;
typedef TEXT TBOOL;
typedef char TINY;
typedef double DOUBLE;
typedef int ARGINT, BOOL, VOID;
typedef long LONG;
typedef short COUNT, FILE, METACH;
typedef unsigned BYTES;
typedef unsigned char UTINY;
typedef unsigned long ULONG;
typedef unsigned short BITS, UCOUNT;

 

 

 
typedef struct fio
	{
	FILE _fd;
	COUNT _nleft;
	COUNT _fmode;
	TEXT *_pnext;
	TEXT _buf[512];
	} FIO;
 


 

 

 

 

struct incl
	{
	TEXT *next;
	TEXT *fname;
	COUNT nline;
	FIO pfio;
	};

 

struct pretab
	{
	TEXT *prename;
	int pretype;
	};

 

struct tlist
	{
	struct tlist *next;
	int type;
	TEXT *white;
	BYTES nwhite;
	TEXT *text;
	BYTES ntext;
	};

 

struct alist
	{
	TEXT *next;
	struct tlist *astart;
	struct tlist *aend;
	};

 

struct def
	{
	TEXT *next;
	BYTES dnlen;
	TEXT *defn;
	TEXT dname[8];
	};

 

struct args
	{
	BYTES ntop;
	TEXT *anames[10];
	};


struct incl *nxtfile();
struct tlist *getin();
struct tlist *putgr();
TEXT *getln();
extern TEXT *buybuf();

 
 
static BOOL cflag = {0};
static struct args pdefs = {10};
static TEXT *ofile = {0};
extern BOOL xflag = {0};
extern BOOL v6flag = {0};
extern BYTES pchar = {'#'};
extern BYTES schar = {'@'};
extern TEXT *iprefix = {"|"};
extern TEXT *_pname = {"pp"};

 
extern TEXT **argv = {0};
extern BYTES argc = {0};
extern FILE errfd = {2};
extern struct incl *pincl = {0};
extern COUNT nerrors = {0};
extern BOOL pflag = {0};

 
static struct pretab pptab[] = {
	"\2IF", 13,
	"\2if", 13,
	"\4ELSE", 11,
	"\4LINE", 17,
	"\4else", 11,
	"\4line", 17,
	"\5ENDIF", 12,
	"\5IFDEF", 14,
	"\5UNDEF", 19,
	"\5endif", 12,
	"\5ifdef", 14,
	"\5undef", 19,
	"\6DEFINE", 10,
	"\6IFNDEF", 15,
	"\6define", 10,
	"\6ifndef", 15,
	"\7INCLUDE", 16,
	"\7include", 16,
	};

 
struct tlist *getex()
	{
	extern BYTES pchar, schar;
	extern struct pretab pptab[];
	register struct tlist *p;
	register int tok;

	if (p = getin())
		{
		if (punct(p, pchar) || punct(p, schar))
			{
			if (tok = scntab(pptab, 18, p->next->text, p->next->ntext))
				{
				p = wsfree(p, p->next);
				p->type = tok;
				}
			else
				p->type = 18;
			}
		switch (p->type)
			{
		case 10:
		case 19:
		case 16:
		case 18:
			break;
		case 14:
		case 15:
			if (p->next->type != 3)
				perror("bad #%b", p->text, p->ntext);
			else if (!lookup(p->next->text, p->next->ntext))
				p->next = frelst(p->next, 0);
			break;
		default:
			p = doexp(p);
			}
		}
	return (p);
	}

 
struct tlist *getin()
	{
	extern BOOL pflag;
	extern struct incl *pincl;
	register TEXT *s;

	for (;;)
		{
		if (!pincl)
			pincl = nxtfile();
		if (!pincl)
			return (0);
		else if (s = getln(pincl))
			return (stotl(s));
		else
			{
			fclose(&pincl->pfio);
			wsfree(pincl->fname, 0);
			pincl = wsfree(pincl, pincl->next);
			pflag = 1;
			}
		}
	}

 
TEXT *getln(pi)
	struct incl *pi;
	{
	extern BOOL cflag;
	register COUNT i;
	register FIO *pf;
	register TEXT *s;
	COUNT j, k;
	static TEXT buf[512];
	TEXT strchar, *savs;

	strchar = '\0';
	s = buf;
	pf = &pi->pfio;
	for (i = getl(pf, buf, 512); 0 < i; )
		if (!cflag && *s == '\\' && 1 < i && s[1] == '\n')
			{
			++pi->nline;
			i = getl(pf, s, 512 - (s - buf));
			for (savs = s; 0 < i && *s != '\n' && ((*s) <= ' ' || 0177 <= (*s)); --i)
				++s;
			if (savs != s)
				{
				cpybuf(savs, s, i);
				s = savs;
				}
			}
		else if (!cflag && *s == '\\')
			{
			s += 2;
			i -= 2;
			}
		else if (!cflag && !strchar && *s == '/' && 1 < i && s[1] == '*')
			{
			for (j = 2; j < i - 1; ++j)
				if (s[j] == '*' && s[j + 1] == '/')
					break;
			if (j < i - 1)
				{
				*s++ = ' ';
				i -= j + 2;
				for (k = 0; k < i; ++k)
					s[k] = s[k + j + 1];
				}
			else if ((i = getl(pf, s + 2, 512 - (s + 2 - buf))) <= 0)
				{
				perror("missing */");
				break;
				}
			else
				{
				++pi->nline;
				i += 2;
				}
			}
		else if (*s == '\n')
			{
			++pi->nline;
			return (buf);
			}
		else
			{
			if (!cflag && (*s == '"' || *s == '\''))
				{
				if (strchar == *s)
					strchar = '\0';
				else if (!strchar)
					strchar = *s;
				else
					;
				}
			++s;
			--i;
			}
	if (s == buf)
		return (0);
	else
		{
		perror("truncated line");
		buf[512 - 1] = '\n';
		return (buf);
		}
	}

 
BOOL main(ac, av)
	COUNT ac;
	TEXT **av;
	{
	extern struct args pdefs;
	extern BOOL cflag, xflag, v6flag;
	extern BYTES argc, pchar, schar;
	extern COUNT nerrors;
	extern FILE errfd;
	extern FIO stdout;
	extern struct incl *pincl;
	extern TEXT **argv, *iprefix, *ofile;
	register struct tlist *p;

	argv = av;
	argc = ac;
	getflags(&argc, &argv, "c,d*>i*,o*,p?,s?,x,6:F <files>", &cflag, &pdefs,
		&iprefix, &ofile, &pchar, &schar, &xflag, &v6flag);
	if (pincl = nxtfile())
		predef(&pdefs);
	if (ofile)
		if ((stdout._fd = create(ofile, 1, xflag != 0)) < 0)
			error("bad output file");
		else
			{
			errfd = 1;
			stdout._fmode = -1;
			}
	while (p = putgr(getex(), 0))
		{
		perror("misplaced #%b", p->text, p->ntext);
		frelst(p, 0);
		}
	fclose(&stdout);
	return (nerrors == 0);
	}

 
struct tlist *putgr(p, skip)
	register struct tlist *p;
	register BOOL skip;
	{
	register BOOL doit;

	while (p && p->type != 11 && p->type != 12)
		if (p->type != 13 && p->type != 14 && p->type != 15)
			{
			if (skip)
				frelst(p, 0);
			else
				putns(p);
			p = getex();
			}
		else
			{
			if (p->type == 13)
				doit = skip ? 0 : eval(p->next);
			else if (p->type == 14)
				doit = (p->next != 0);
			else
				doit = (p->next == 0);
			frelst(p, 0);
			p = putgr(getex(), skip || !doit);
			if (p && p->type == 11)
				{
				frelst(p, 0);
				p = putgr(getex(), skip || doit);
				}
			if (p && p->type == 12)
				{
				frelst(p, 0);
				p = getex();
				}
			else
				perror("missing #endif");
			}
	return(p);
	}

 
VOID putns(p)
	register struct tlist *p;
	{
	extern BOOL pflag;
	extern struct incl *pincl;
	register TEXT *fname;
	register struct tlist *q;
	FILE fd;
	COUNT i;

	switch (p->type)
		{
	case 10:
		if (p->next->type != 3)
			perror("bad #define");
		else
			{
			for (q = p->next; q->next; q = q->next)
				;
			install(p->next->text, p->next->ntext,
				buybuf(p->next->next->white,
				q->text + q->ntext - p->next->next->white));
			}
		break;
	case 19:
		if (p->next->type != 3)
			perror("bad #undef");
		else
			undef(p->next->text, p->next->ntext);
		break;
	case 16:
		if (!(fname = getfname(p->next)))
			perror("bad #include");
		else if ((fd = open(fname, 0, 0)) < 0)
			{
			perror("can't #include %p", fname);
			wsfree(fname, 0);
			}
		else
			{
			pincl = wsalloc(sizeof (*pincl), pincl);
			pincl->fname = fname;
			pincl->nline = 0;
			finit(&pincl->pfio, fd, 0);
			pflag = 1;
			}
		break;
	case 17:
		if (p->next->type != 4)
			perror("bad #line");
		else
			{
			btos(p->next->text, p->next->ntext, &i, 10);
			pincl->nline = i;
			if (fname = getfname(p->next->next))
				{
				pflag = 1;
				if (pincl->fname)
					wsfree(pincl->fname, 0);
				pincl->fname = fname;
				}
			}
		break;
	case 18:
		if (p->next->type != 2)
			perror("bad #xxx");
		break;
	default:
		putls(p);
		}
	frelst(p, 0);
	}
