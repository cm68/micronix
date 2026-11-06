/*
 * UNIX shell
 *
 * S. R. Bourne
 * Bell Telephone Laboratories
 *
 * Argument scanning and word splitting: trim, mactrim, scan and
 * getarg, with the two helpers they use.  This was the tail of
 * service.c; it is a source of its own so that the ccc first pass
 * has less to hold for the shell's largest translation unit - the
 * guest's pass0 ran out of memory on service.c as one file.
 */

#include	"defs.h"

PROC VOID	gsort(STRING [], STRING []);

#define ARGMK	01

BOOL		nosubst;


/* prototypes for functions defined below */
INT cf(REG STRING, REG STRING);
INT expand(STRING, int);
int makearg(REG ARGPTR);
LOCAL INT split(REG STRING);
trim(at)
	STRING		at;
{
	REG STRING	p;
	REG CHAR	c;
	REG CHAR	q=0;

	IF p=at
	THEN	WHILE c = *p
		DO *p++=c&STRIP; q |= c OD
	FI
	nosubst=q&QUOTE;
}

STRING	mactrim(s)
	STRING		s;
{
	REG STRING	t=macro(s);
	trim(t);
	return(t);
}

STRING	*scan(argn)
	INT		argn;
{
	REG ARGPTR	argp = Rcheat(gchain)&~ARGMK;
	REG STRING	*comargn, *comargm;

	comargn=getstak(BYTESPERWORD*argn+BYTESPERWORD); comargm = comargn += argn; *comargn = ENDARGS;

	WHILE argp
	DO	*--comargn = argp->argval;
		IF argp = argp->argnxt
		THEN trim(*comargn);
		FI
		IF argp==0 ORF Rcheat(argp)&ARGMK
		THEN	gsort(comargn,comargm);
			comargm = comargn;
		FI
		/* Lcheat(argp) &= ~ARGMK; */
		argp = Rcheat(argp)&~ARGMK;
	OD
	return(comargn);
}

LOCAL VOID	gsort(from,to)
	STRING		from[], to[];
{
	INT		k, m, n;
	REG INT		i, j;

	IF (n=to-from)<=1 THEN return FI

	FOR j=1; j<=n; j*=2 DONE

	FOR m=2*j-1; m/=2;
	DO  k=n-m;
	    FOR j=0; j<k; j++
	    DO	FOR i=j; i>=0; i-=m
		DO  REG STRING *fromi; fromi = &from[i];
		    IF cf(fromi[m],fromi[0])>0
		    THEN break;
		    ELSE STRING s; s=fromi[m]; fromi[m]=fromi[0]; fromi[0]=s;
		    FI
		OD
	    OD
	OD
}

/* Argument list generation */

INT	getarg(ac)
	COMPTR		ac;
{
	REG ARGPTR	argp;
	REG INT		count=0;
	REG COMPTR	c;

	IF c=ac
	THEN	argp=c->comarg;
		WHILE argp
		DO	count += split(macro(argp->argval));
			argp=argp->argnxt;
		OD
	FI
	return(count);
}

LOCAL INT	split(s)
	REG STRING	s;
{
	REG STRING	argp;
	REG ARGPTR	ap;
	REG INT		c;
	INT		count=0;

	LOOP	sigchk(); argp=locstak()+BYTESPERWORD;
		WHILE (c = *s++, !any(c,ifsnod.namval) && c)
		DO *argp++ = c OD
		IF argp==staktop+BYTESPERWORD
		THEN	IF c
			THEN	continue;
			ELSE	return(count);
			FI
		ELIF c==0
		THEN	s--;
		FI
		argp=endstak(argp); ap=(ARGPTR)argp;
		IF c=expand(ap->argval,0)
		THEN	count += c;
		ELSE	/* assign(&fngnod, argp->argval); */
			makearg(ap); count++;
		FI
		gchain=(ARGPTR)(Rcheat(gchain)|ARGMK);
	POOL
}
