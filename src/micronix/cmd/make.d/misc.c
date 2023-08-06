#include "defs"


FSTATIC char *nextchar 0;
FSTATIC char *lastchar 0;

FSTATIC int *nextint 0;
FSTATIC int *lastint 0;

FSTATIC struct nameblock *hashtab[HASHSIZE];
FSTATIC int nhashed 0;


/* simple linear hash.  hash function is sum of
   characters mod hash table size.
*/
hashloc(s)
char *s;
{
register int i;
register int hashval;
register char *t;

hashval = 0;

for(t=s; *t!='\0' ; ++t)
	hashval =+ *t;

hashval =% HASHSIZE;

for(i=hashval;
	hashtab[i]!=0 && equals(s,hashtab[i]->namep)==0;
	i = (i+1)%HASHSIZE ) ;

return(i);
}


struct nameblock *srchname(s)
char *s;
{
return( hashtab[hashloc(s)] );
}



struct nameblock *makename(s)
char *s;
{
/* make a fresh copy of the string s */

char *copys();
register struct nameblock *p;

if(nhashed++ > HASHSIZE-3)
	fatal("Hash table overflow");

p = intalloc(sizeof(*p));
p->nextp = firstname;
p->namep = copys(s);
p->linep = 0;
p->done = 0;
p->septype = 0;
p->modtime = 0;

firstname = p;
if(mainname==0 && s[0]!='.') mainname = p;

hashtab[hashloc(s)] = p;

return(p);
}



char *copys(s)
register char *s;
{
register char *t;

for(t=s; *t++ ; );
if( (t-s) >= (lastchar-nextchar) )
	{
	if( (nextchar=calloc(NCHARS,sizeof(*t))) == NULL)
		fatal("Cannot allocate memory");
	lastchar = nextchar + NCHARS;
	}

t = nextchar;
while(*nextchar++ = *s++);
return(t);
}

equals(a,b)
register char *a,*b;
{
while(*a == *b)
	if(*a == '\0') return(1);
	else {++a; ++b;}

return(0);
}


char *concat(a,b,c)   /* c = concatenation of a and b */
register char *a,*b;
char *c;
{
register char *t;
t = c;

while(*t = *a++) t++;
while(*t++ = *b++);
return(c);
}

suffix(a,b,p)  /* is b the suffix of a?  if so, set p = prefix */
register char *a,*b,*p;
{
char *a0,*b0;
a0 = a;
b0 = b;

while(*a++);
while(*b++);

if( (a-a0) < (b-b0) ) return(0);

while(b>b0)
	if(*--a != *--b) return(0);

while(a0<a) *p++ = *a0++;
*p = '\0';

return(1);
}



int intalloc(n)
int n;
{
int p;

n = (n + sizeof(n)-1) / sizeof(n);

if(lastint-nextint <= n)
	{
	if( (nextint=calloc(NINTS,sizeof(n))) == NULL)
		fatal("Cannot allocate memory");
	lastint = nextint + NINTS;
	}

p = nextint;
nextint =+ n;

return(p);
}

/* copy string a into b, substituting for arguments */
subst(a,b)
register char *a,*b;
{
register char *s;
char vname[100];
struct varblock *varptr(), *vbp;
char *copstr();
char closer;

if(a!=0)  while(*a)
	{
	if(*a != '$') *b++ = *a++;
	else if (*++a == '\0') *b++ = *a++;
	else	{
		s = vname;
		if( *a=='(' || *a=='{' )
			{
			closer = ( *a=='(' ? ')' : '}');
			++a;
			while(*a == ' ') ++a;
			while(*a!=' ' && *a!=closer && *a!='\0') *s++ = *a++;
			while(*a!=closer && *a!='\0') ++a;
			if(*a == closer) ++a;
			}
		else	*s++ = *a++;

		*s = '\0';
		if( (vbp = varptr(vname)) ->varval != 0)
			{
			b = copstr(b, vbp->varval);
			vbp->used = 1;
			}
		}
	}

*b++ = '\0';
return(b);
}

/* copy s into t, return the location of the next
free character in s */
char *copstr(s,t)
char *s,*t;
{
while (*t) *s++ = *t++;
return(s);
}

setvar(v,s)
char *v, *s;
{
struct varblock *varptr(), *p;

p = varptr(v);
if(p->noreset == 0)
	{
	p->varval = s;
	p->noreset = inarglist;
	if(p->used && !equals(v,"@") && !equals(v,"*")
	    && !equals(v,"<") && !equals(v,"?") )
		fprintf(stderr, "Warning: %s changed after being used\n",v);
	}
}


eqsign(a)   /*look for arguments with equal signs but not colons */
char *a;
{
register char *s, *t;

while(*a == ' ') ++a;
for(s=a  ;   *s!='\0' && *s!=':'  ; ++s)
	if(*s == '=')
		{
		for(t=a ; *t!='=' && *t!=' ' ;  ++t );
		*t = '\0';

		for(++s; *s==' ' || *s=='\t' ; ++s);
		setvar(a, copys(s));
		return(1);
		}

return(0);
}


struct varblock *varptr(v)
char *v;
{
register struct varblock *vp;

for(vp=firstvar; vp!=0 ; vp = vp->nextp)
	if(equals(v , vp->varname)) return(vp);

vp = intalloc(sizeof(*vp));
vp->nextp = firstvar;
firstvar = vp;
vp->varname = copys(v);
vp->varval = 0;
return(vp);
}


fatal(s)
char *s;
{
if(s) fprintf(stderr, "%s.  Stop.\n", s);
else fprintf(stderr, "\nStop.\n");
#ifdef unix
exit(1);
#endif
#ifdef gcos
exit(0);
#endif
}



yyerror(s)
char *s;
{
char buf[50];
extern int yylineno;

fatal( sprintf(buf, "line %d: %s", yylineno, s) );
}



appendq(head,tail)
struct chain **head;
char *tail;
{
struct chain *p;
p = intalloc(sizeof(*p));
p->datap = tail;
while(head->nextp) head = head->nextp;
head->nextp = p;
}





mkqlist(p)
struct chain *p;
{
register char *qbufp, *s;
static char qbuf[300];

qbufp = qbuf;

for( ; p ; p = p->nextp)
	{
	s = p->datap;
	if (qbufp+strlen(s)+2 >= &qbuf[300])
		break;
	*qbufp++ = ' ';
	while (*s)
		*qbufp++ = *s++;
	}
*qbufp = '\0';
return(qbuf);
}
