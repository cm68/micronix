# include "defs"
/*
command make to update programs.
Flags:	'd'  print out debugging comments
	'p'  print out a version of the input graph
	's'  silent mode--don't print out commands
	'f'  the next argument is the name of the description file;
	     "makefile" is the default
	'i'   ignore error codes from the shell
	'n'   don't issue, just print, commands
	't'   touch (update time of) files but don't issue command
	'q'   don't do anything, but check if object is up to date;
	      returns exit code 0 if up to date, -1 if not
*/

struct nameblock *mainname ;
struct nameblock *firstname;
struct lineblock *sufflist;
struct varblock *firstvar;
struct pattern *firstpat ;
struct opendir *firstod;

int sigivalue 0;
int sigqvalue 0;
int dbgflag 0;
int prtrflag 0;
int silflag 0;
int noexflag 0;
int noruleflag 0;
int touchflag 0;
int questflag 0;
int ndocoms 0;
int ignerr 0;    /* default is to stop on error */
int okdel 1;
int inarglist;
char *prompt ">";	/* other systems -- pick what you want */

main(argc,argv)
int argc;
char **argv;
{
struct nameblock *p, *makename(), *srchname();
int i, j;
int descset, nfargs;
char c, *s;
#ifdef unix
int intrupt();



#endif

#ifdef METERFILE
meter(METERFILE);
#endif

pexinit();
descset = 0;

inarglist = 1;
for(i=1; i<argc; ++i)
	if(argv[i]!=0 && argv[i][0]!='-' && eqsign(argv[i]))
		argv[i] = 0;

setvar("$","$");
inarglist = 0;

for(i=1; i<argc; ++i)
    if(argv[i]!=0 && argv[i][0]=='-')
	{
	for(j=1 ; (c=argv[i][j])!='\0' ; ++j)  switch(c)
		{
		case 'd':
			dbgflag = 1;
			break;

		case 'p':
			prtrflag = 1;
			break;

		case 's':
			silflag = 1;
			break;

		case 'i':
			ignerr = 1;
			break;

		case 'n':
			noexflag = 1;
			break;

		case 'r':
			noruleflag = 1;
			break;

		case 't':
			touchflag = 1;
			break;

		case 'q':
			questflag = 1;
			break;

		case 'f':
			if(i >= argc-1)
			  fatal("No description argument after -f flag");
			if( rddescf(argv[i+1]) )
				{
				fprintf(stderr,"Cannot open %s",argv[i+1]);
				fatal("");
				}
			argv[i+1] = 0;
			++descset;
			break;

		default:
			fprintf(stderr,"Unknown flag argument %c", c);
			fatal("");
		}

	argv[i] = 0;
	}

if( !descset )
#ifdef unix
	if( rddescf("makefile") )  rddescf("Makefile");
#endif
#ifdef gcos
	rddescf("makefile");
#endif

if(prtrflag) printdesc(0);

if( srchname(".IGNORE") ) ++ignerr;
if( srchname(".SILENT") ) silflag = 1;
if(p=srchname(".SUFFIXES")) sufflist = p->linep;
if( !sufflist ) fprintf(stderr,"No suffix list.\n");

#ifdef unix
sigivalue = signal(SIGINT,1) & 01;
sigqvalue = signal(SIGQUIT,1) & 01;
enbint(intrupt);
#endif

nfargs = 0;

for(i=1; i<argc; ++i)
	if((s=argv[i]) != 0)
		{
		if((p=srchname(s)) == 0)
			{
			p = makename(s);
/*
			fprintf(stderr,"Warning: %s not in description.\n",s);
*/
			}
		++nfargs;
		doname(p,0);
		if(dbgflag) printdesc(1);
		}

/*
If no file arguments have been encountered, make the first
name encountered that doesn't start with a dot
*/

if(nfargs == 0)
	if(mainname == 0)
		fatal("No arguments or description file");
	else	{
		doname(mainname,0);
		if(dbgflag) printdesc(1);
		}

exit(0);
}



#ifdef unix
intrupt()
{
struct varblock *varptr();
char *p;
TIMETYPE exists();

if( okdel &&  (p=varptr("@")->varval) && exists(p)>0 && !noexflag && !touchflag )
	{
	fprintf(stderr, "\n***  %s removed.", p);
	unlink(p);
	}

fprintf(stderr, "\n");
exit(2);
}



disint()
{
signal(SIGINT,1);
signal(SIGQUIT,1);
}



enbint(k)
int (*k)();
{
if(sigivalue == 0)
	signal(SIGINT,k);
if(sigqvalue == 0)
	signal(SIGQUIT,k);
}
#endif

extern char *builtin[];

char **linesptr builtin;

FILE * fin;
int firstrd 0;


rddescf(descfile)
char *descfile;
{
extern int yylineno;
extern char *zznextc;
FILE * k;

/* read and parse description */

if( !firstrd++ )
	{
	if( !noruleflag )
		rdd1(NULL);

#ifdef pwb
		{
		char *nlog, s[100];
		nlog = logdir();
		if ( (k=fopen( concat(nlog,"/makecomm",s), "r")) != NULL)
			rdd1(k);
		else if ( (k=fopen( concat(nlog,"/Makecomm",s), "r")) != NULL)
			rdd1(k);
	
		if ( (k=fopen("makecomm", "r")) != NULL)
			rdd1(k);
		else if ( (k=fopen("Makecomm", "r")) != NULL)
			rdd1(k);
		}
#endif

	}
if(equals(descfile, "-"))
	return( rdd1(stdin) );

if( (k = fopen(descfile,"r")) != NULL)
	return( rdd1(k) );

return(1);
}




rdd1(k)
FILE * k;
{
fin = k;
yylineno = 0;
zznextc = 0;

if( yyparse() )
	fatal("Description file error");

if(fin != NULL)
	fclose(fin);

return(0);
}

printdesc(prntflag)
int prntflag;
{
struct nameblock *p;
struct depblock *dp;
struct varblock *vp;
struct opendir *od;
struct shblock *sp;
struct lineblock *lp;

#ifdef unix
if(prntflag)
	{
	printf("Open directories:\n");
	for(od=firstod; od!=0; od = od->nextp)
		printf("\t%d: %s\n", fileno(od->dirfc), od->dirn);
	}
#endif

if(firstvar != 0) printf("Macros:\n");
for(vp=firstvar; vp!=0; vp=vp->nextp)
	printf("\t%s = %s\n" , vp->varname , vp->varval);

for(p=firstname; p!=0; p = p->nextp)
	{
	printf("\n\n%s",p->namep);
	if(p->linep != 0) printf(":");
	if(prntflag) printf("  done=%d",p->done);
	if(p==mainname) printf("  (MAIN NAME)");
	for(lp = p->linep ; lp!=0 ; lp = lp->nextp)
		{
		if( dp = lp->depp )
			{
			printf("\n depends on:");
			for(; dp!=0 ; dp = dp->nextp)
				if(dp->depname != 0)
					printf(" %s ", dp->depname->namep);
			}
	
		if(sp = lp->shp)
			{
			printf("\n commands:\n");
			for( ; sp!=0 ; sp = sp->nextp)
				printf("\t%s\n", sp->shbp);
			}
		}
	}
printf("\n");
fflush(stdout);
}
