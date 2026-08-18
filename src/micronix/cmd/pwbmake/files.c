/* UNIX DEPENDENT PROCEDURES */

#include "defs.h"


/* DEFAULT RULES FOR MICRONIX */

char *builtin[] =
	{
	".SUFFIXES : .out .o .c .f .e .r .y .yr .ye .l .s",
	"YACC=yacc",
	"YACCR=yacc -r",
	"YACCE=yacc -e",
	"YFLAGS=",
	"LEX=lex",
	"LFLAGS=",
	"CC=ccc",
	"AS=asz",
	"CFLAGS=",
	"RC=f77",
	"RFLAGS=",
	"EC=f77",
	"EFLAGS=",
	"FFLAGS=",
	"LOADLIBES=",

	".c.o :",
	"\t$(CC) $(CFLAGS) -c $<",

	".e.o .r.o .f.o :",
	"\t$(EC) $(RFLAGS) $(EFLAGS) $(FFLAGS) -c $<",

	".s.o :",
	"\t$(AS) -o $@ $<",

	".y.o :",
	"\t$(YACC) $(YFLAGS) $<",
	"\t$(CC) $(CFLAGS) -c y.tab.c",
	"\trm y.tab.c",
	"\tmv y.tab.o $@",

	".yr.o:",
	"\t$(YACCR) $(YFLAGS) $<",
	"\t$(RC) $(RFLAGS) -c y.tab.r",
	"\trm y.tab.r",
	"\tmv y.tab.o $@",

	".ye.o :",
	"\t$(YACCE) $(YFLAGS) $<",
	"\t$(EC) $(RFLAGS) -c y.tab.e",
	"\trm y.tab.e",
	"\tmv y.tab.o $@",

	".l.o :",
	"\t$(LEX) $(LFLAGS) $<",
	"\t$(CC) $(CFLAGS) -c lex.yy.c",
	"\trm lex.yy.c",
	"\tmv lex.yy.o $@",

	".y.c :",
	"\t$(YACC) $(YFLAGS) $<",
	"\tmv y.tab.c $@",

	".l.c :",
	"\t$(LEX) $<",
	"\tmv lex.yy.c $@",

	".yr.r:",
	"\t$(YACCR) $(YFLAGS) $<",
	"\tmv y.tab.r $@",

	".ye.e :",
	"\t$(YACCE) $(YFLAGS) $<",
	"\tmv y.tab.e $@",

	".s.out .c.out .o.out :",
	"\t$(CC) $(CFLAGS) $< $(LOADLIBES) -o $@",

	".f.out .r.out .e.out :",
	"\t$(EC) $(EFLAGS) $(RFLAGS) $(FFLAGS) $< $(LOADLIBES) -o $@",
	"\t-rm $*.o",

	".y.out :",
	"\t$(YACC) $(YFLAGS) $<",
	"\t$(CC) $(CFLAGS) y.tab.c $(LOADLIBES) -ly -o $@",
	"\trm y.tab.c",

	".l.out :",
	"\t$(LEX) $<",
	"\t$(CC) $(CFLAGS) lex.yy.c $(LOADLIBES) -ll -o $@",
	"\trm lex.yy.c",

	0 };


TIMETYPE exists(filename)
char *filename;
{
struct stat buf;
register char *s;
register char *a, *t;
char aname[100];

for(s = filename ; *s!='\0' && *s!='(' ; ++s)
	;

if(*s == '(')
	{
	/*
	 * a(b) or a((b)): an archive member.  Micronix's object format
	 * is Whitesmith's obj, not a.out, so the member's own timestamp
	 * is not resolved; stat the archive itself and take its time.
	 */
	for(a = filename, t = aname ; a < s ; )
		*t++ = *a++;
	*t = '\0';
	return( stat(aname, &buf) < 0 ? 0 : buf.st_mtime );
	}

if(stat(filename,&buf) < 0)
	return(0);
else	return(buf.st_mtime);
}


TIMETYPE prestime()
{
TIMETYPE t;
time(&t);
return(t);
}


FSTATIC char n15[15];
FSTATIC char *n15end	= &n15[14];

static amatch();
static umatch();



struct depblock *srchdir(pat, mkchain, nextdbl)
register char *pat; /* pattern to be matched in directory */
int mkchain;  /* nonzero if results to be remembered */
struct depblock *nextdbl;  /* final value for chain */
{
FILE * dirf;
int i, nread;
char *dirname, *dirpref, *endir, *filepat, *p, temp[100];
char fullname[100], *p1, *p2;
struct nameblock *q;
struct depblock *thisdbl;
struct opendir *od;
struct pattern *patp;

struct direct entry[32];


thisdbl = 0;

if(mkchain == NO)
	for(patp=firstpat ; patp ; patp = patp->nxtpattern)
		if(! unequal(pat, patp->patval)) return(0);

patp = ALLOC(pattern);
patp->nxtpattern = firstpat;
firstpat = patp;
patp->patval = copys(pat);

endir = 0;

for(p=pat; *p!='\0'; ++p)
	if(*p=='/') endir = p;

if(endir==0)
	{
	dirname = ".";
	dirpref = "";
	filepat = pat;
	}
else	{
	dirname = pat;
	*endir = '\0';
	dirpref = concat(dirname, "/", temp);
	filepat = endir+1;
	}

dirf = NULL;

for(od = firstod ; od; od = od->nxtopendir)
	if(! unequal(dirname, od->dirn) )
		{
		dirf = od->dirfc;
		fseek(dirf,0L,0); /* start over at the beginning  */
		break;
		}

if(dirf == NULL)
	{
	dirf = fopen(dirname, "r");
	od = ALLOC(opendir);
	od->nxtopendir = firstod;
	firstod = od;
	od->dirfc = dirf;
	od->dirn = copys(dirname);
	}

if(dirf == NULL)
	{
	fprintf(stderr, "Directory %s: ", dirname);
	fatal("Cannot open");
	}

else do
	{
	nread = fread( (char *) &entry[0], sizeof(struct direct), 32, dirf) ;
	for(i=0; i<nread; ++i)
		if(entry[i].d_ino!= 0)
			{
			p1 = entry[i].d_name;
			p2 = n15;
			while( (p2<n15end) &&
			  (*p2++ = *p1++)!='\0' );
			if( amatch(n15,filepat) )
				{
				concat(dirpref,n15,fullname);
				if( (q=srchname(fullname)) ==0)
					q = makename(copys(fullname));
				if(mkchain)
					{
					thisdbl = ALLOC(depblock);
					thisdbl->nxtdepblock = nextdbl;
					thisdbl->depname = q;
					nextdbl = thisdbl;
					}
				}
			}

	} while(nread==32);

if(endir != 0)  *endir = '/';

return(thisdbl);
}

/* stolen from glob through find */

static amatch(s, p)
char *s, *p;
{
	register int cc, scc, k;
	int c, lc;

	scc = *s;
	lc = 077777;
	switch (c = *p) {

	case '[':
		k = 0;
		while (cc = *++p) {
			switch (cc) {

			case ']':
				if (k)
					return(amatch(++s, ++p));
				else
					return(0);

			case '-':
				k |= (lc <= scc)  & (scc <= (cc=p[1]) ) ;
			}
			if (scc==(lc=cc)) k++;
		}
		return(0);

	case '?':
	caseq:
		if(scc) return(amatch(++s, ++p));
		return(0);
	case '*':
		return(umatch(s, ++p));
	case 0:
		return(!scc);
	}
	if (c==scc) goto caseq;
	return(0);
}

static umatch(s, p)
char *s, *p;
{
	if(*p==0) return(1);
	while(*s)
		if (amatch(s++,p)) return(1);
	return(0);
}
