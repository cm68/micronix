/* UNIX DEPENDENT PROCEDURES */


/* DEFAULT RULES FOR UNIX */

char *builtin[]
	{
	".SUFFIXES : .o .c .e .r .f .y .yr .ye .l .s .w .t .h .z .x",
	"YACC=yacc",
	"YACCR=yacc -r",
	"YACCE=yacc -e",
	"YFLAGS=",
	"LEX=lex",
	"LFLAGS=",
	"CC=cc",
	"AS=as -",
	"CFLAGS=",
	"RC=ec",
	"RFLAGS=",
	"EC=ec",
	"EFLAGS=",
	"FFLAGS=",
	"CAWK=awk -c -f",
	"SCOMP=scomp",
	"SCFLAGS=",
	"CMDICT=cmdict",
	"CMFLAGS=",
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
	"\t$(RC) $(RFLAGS) -c -2 y.tab.r",
	"\trm y.tab.r",
	"\tmv y.tab.o $@",
	".ye.o :",
	"\t$(YACCE) $(YFLAGS) $<",
	"\t$(EC) $(RFLAGS) -c -2 y.tab.e",
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
	".w.c:",
	"\t$(CAWK) $<",
	"\tmv yy.awk.c $@",
	".w.o:",
	"\t$(CAWK) $<",
	"\t$(CC) $(CFLAGS) -c yy.awk.c",
	"\trm yy.awk.c",
	"\tmv yy.awk.o $@",
	".t.o:",
	"\t$(SCOMP) $(SCFLAGS) -c $<",
	".t.c:",
	"\t$(SCOMP) $(SCFLAGS) -t $<",
	".h.z .t.z:",
	"\t$(CMDICT) $(CMFLAGS) $<",
	".h.x .t.x:",
	"\t$(CMDICT) $(CMFLAGS) -x $<",
	0 };

#include "defs"


TIMETYPE exists(filename)
char *filename;
{
#include <stat.h>
struct statb buf;

if(stat(filename,&buf) < 0) 
	return(0);
else	return(buf.i_mtime);
}


TIMETYPE prestime()
{
long int t;
time(&t);
return(t);
}



 char n15[15];
 char *n15end &n15[14];



struct depblock *srchdir(pat, mkchain, nextdbl)

char *pat; /* pattern to be matched in directory */
int mkchain;  /* nonzero if results to be remembered */
struct depblock *nextdbl;  /* final value for chain */
{
FILE * dirf;
int i, nread;
char *dirname, *dirpref, *endir, *filepat, *p, temp[100];
char fullname[100], *p1, *p2, *copys();
struct nameblock *q, *srchname(), *makename();
struct depblock *thisdbl;
struct opendir *od;
struct pattern *patp;

struct	{
	int ino;
	char name[14];
	}  entry[32];


thisdbl=0;

if(mkchain == 0)
	for(patp=firstpat ; patp!=0 ; patp = patp->nextp)
		if(equals(pat, patp->patval)) return(0);

patp = intalloc(sizeof(*patp));
patp->nextp = firstpat;
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

for(od=firstod ; od!=0; od = od->nextp)
	if(equals(dirname, od->dirn))
		{
		dirf = od->dirfc;
		fseek(dirf,0L,0); /* start over at the beginning  */
		break;
		}

if(dirf == NULL)
	{
	dirf = fopen(dirname, "r");
	od = intalloc(sizeof(*od));
	od->nextp = firstod;
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
	nread = fread(entry,sizeof(entry[0]),32,dirf) ;
	for(i=0;i<nread;++i)
		if(entry[i].ino!= 0)
			{
			p1 = entry[i].name;
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
					thisdbl = intalloc(sizeof(*thisdbl));
					thisdbl->nextp = nextdbl;
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

amatch(s, p)
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
				k =| lc <= scc & scc <= (cc=p[1]);
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

umatch(s, p)
char *s, *p;
{
	if(*p==0) return(1);
	while(*s)
		if (amatch(s++,p)) return(1);
	return(0);
}

#ifdef METERFILE
int meteron 0;	/* default: metering off */

meter(file)
char *file;
{
int tvec[2];
char *p, buf[520], *ctime();
FILE * mout;

if(file==0 || meteron==0) return;

getpw( getuid() & 0377  ,  buf);
for(p=buf;  *p!='\0' && *p!=':'  ; ++p);
*p = '\0';

time(tvec);

if( (mout=fopen(file,"a")) != NULL )
	{
	p = ctime(tvec);
	p[16] = '\0';
	fprintf(mout,"User %s, %s\n",buf,p+4);
	fclose(mout);
	}
}
#endif
