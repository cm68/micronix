#define W(x) (x==' ' || x=='\t' || x=='\n')
#define L(x) ('a'<=x&&x<='z')||('A'<=x&&x<='Z')
#define D(x) ('0'<=x&&x<='9')
#define S(x) (x=='/'||x=='.'||x=='?'||x=='*'||x=='['||x=='-'||x==']'||x=='_')
#define NONTERM(x) (x!='\0'&&x!='\n'&&x!=' '&&x!='\t'&&x!=':'&&x!=';'&&x!='&'&&x!='>')

char *zznextc;	/* zero if need another line; otherwise points to next char */
int yylineno;
extern int fin;

yylex()
{
register char *p;
register char *q;
char word[YYLMAX];

if(zznextc == 0)
	return( nextlin() );

while(W(*zznextc)) ++zznextc;

if(*zznextc == '\0')
	return( nextlin() );

if(*zznextc == ':')
	{
	if(*++zznextc == ':')
		{
		++zznextc;
		return(DOUBLECOLON);
		}
	else	return(COLON);
	}

if(*zznextc == '>')
	{
	++zznextc;
	return(GREATER);
	}

if(*zznextc == ';')
	return( retsh(zznextc) );

p = zznextc;
q = word;

while(NONTERM(*p))
	*q++ = *p++;

if(p != zznextc)
	{
	*q = '\0';
	if((yylval=srchname(word))==0)
		yylval = makename(word);
	zznextc = p;
	return(NAME);
	}

else	{
	fprintf(stderr,"Bad character %c (octal %o), line %d",
		*zznextc,*zznextc,yylineno);
	fatal("");
	}
return(0);	/* never executed */
}





retsh(q)
char *q;
{
register char *p;
struct shblock *sp;
char *copys();

for(p=q+1 ; *p==' '||*p=='\t' ; ++p)  ;

sp = intalloc(sizeof(*sp));
sp->nextp = 0;
sp->shbp = (fin == NULL ? p : copys(p) );
yylval = sp;
zznextc = 0;
return(SHELLINE);
}

nextlin()
{
static char yytext[YYLMAX];
static char *yytextl yytext+YYLMAX;
char *text, templin[YYLMAX];
register char c;
register char *p, *t;
char lastch, *lastchp;
extern char **linesptr;
int incom;

incom = 0;
zznextc = 0;

if(fin == NULL)
	{
	if( (text = *linesptr++) == 0)
		return(0);
	++yylineno;
	}

else	{
	for(p=yytext ; ; ++p)
		{
		if(p > yytextl) fatal("line too long");
		*p = getc(fin);
		if(*p == EOF)
			{
			*p = '\0';
			return(0);
			}
		else if(*p == ';') ++incom;
		else if (*p=='#' && !incom && yytext[0]!='\t')
			*p = '\0';
		else if (*p == '\n')
			{
			++yylineno;
			if(p==text || p[-1]!='\\') break;
			p[-1] = ' ';
			while( (*p=getc(fin))=='\t' || *p==' ' || *p=='\n') 
				if(*p == '\n') ++yylineno;

			if(*p == EOF)
				{
				*p = '\0';
				return(0);
				}
			}
		}
	*p = '\0';
	text = yytext;
	}

c = text[0];

if(c == '\t')   return( retsh(text) );

if(L(c)||D(c)||c==' '||c=='.')
	for(p=text+1; *p!='\0'; )
		if(*p++ == '=')
			{
			eqsign(text);
			return(MACRODEF);
			}

/* substtitute for macros on dependency line up to the semicolon if any */

for(t = yytext ; *t!='\0' && *t!=';' ; ++t)
	;

lastchp = t;
lastch = *t;
*t = '\0';

subst(yytext, templin);		/* Substitute for macros on dependency lines */

if(lastch)
	{
	for(t = templin ; *t ; ++t)
		;
	*t = lastch;
	while( *++t = *++lastchp ) ;
	}

p = templin;
t = yytext;
while( *t++ = *p++ )
	;

zznextc = text;
return(START);
}
