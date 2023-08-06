#include "defs"

/*  BASIC PROCEDURE.  RECURSIVE.  */

/*
p->done = 0   don't know what to do yet
p->done = 1   file in process of being updated
p->done = 2   file already exists in current state
*/

TIMETYPE doname(p,reclevel)
struct nameblock *p;
int reclevel;
{
int okdel1;
int didwork;
TIMETYPE td, td1, tdep, ptime, ptime1, prestime(), exists();
struct depblock *q, *qtemp, *srchdir(), *suffp, *suffp1;
struct nameblock *p1, *p2, *srchname();
struct shblock *implcom, *explcom;
struct lineblock *lp, *lp1, *lp2;
char *concat(),sourcename[100],prefix[100],temp[100],concsuff[20];
char *pnamep, *p1namep;
struct chain *qchain;

if(p == 0) return(0);

if(dbgflag)
	{
	printf("doname(%s,%d)\n",p->namep,reclevel);
	fflush(stdout);
	}

if(p->done > 0)  return(p->modtime);

tdep = 0;
implcom = 0;
explcom = 0;
ptime = exists(p->namep);
ptime1 = 0;
didwork = 0;
p->done = 1;	/* avoid infinite loops */

qchain = 0;

/* Expand any names that have embedded metacharaters */

for(lp = p->linep ; lp!=0 ; lp = lp->nextp)
	for(q=lp->depp ; q!=0 ; q=qtemp )
		{
		qtemp = q->nextp;
		expand(q);
		}

/* make sure all dependents are up to date */

for(lp = p->linep ; lp!=0 ; lp = lp->nextp)
	{
	td = 0;
	for(q = lp->depp ; q!=0 ; q=q->nextp)
		{
		td1 = doname(q->depname,reclevel+1);
		if(dbgflag)
		    printf("TIME(%s)=%ld\n", q->depname->namep, td1);
		if(td1 > td) td = td1;
		if(ptime < td1)
			appendq(&qchain, q->depname->namep);
		}
	if(p->septype == SOMEDEPS)
		{
		if(lp->shp!=0)
		     if( ptime<td || (ptime==0 && td==0) )
			{
			okdel1 = okdel;
			okdel = 0;
			setvar("@", p->namep);
			setvar("?", mkqlist(qchain) );
			qchain = 0;
			if( !questflag )
				docom(lp->shp);
			setvar("@", 0);
			okdel = okdel1;
			ptime1 = prestime();
			++didwork;
			}
		}

	else	{
		if(lp->shp != 0)
			if(explcom)
				fprintf(stderr, "Too many command lines for `%s'\n",
					p->namep);
			else	explcom = lp->shp;

		if(td > tdep) tdep = td;
		}
	}

/* Look for implicit dependents, using suffix rules */

for(lp=sufflist ; lp!=0 ; lp = lp->nextp)
    for(suffp = lp->depp ; suffp!=0 ; suffp = suffp->nextp)
	{
	pnamep = suffp->depname->namep;
	if(suffix(p->namep , pnamep , prefix))
		{
		srchdir( concat(prefix,"*",temp) , 0,0);
		for(lp1 = sufflist ; lp1!=0 ; lp1 = lp1->nextp)
		    for(suffp1=lp1->depp ; suffp1!=0 ; suffp1 = suffp1->nextp)
			{
			p1namep = suffp1->depname->namep;
			if( (p1=srchname(concat(p1namep, pnamep ,concsuff))) &&
			    (p2=srchname(concat(prefix, p1namep ,sourcename))) )
				{
				td = doname(p2,reclevel+1);
				if(ptime < td)
					appendq(&qchain, p2->namep);
if(dbgflag) printf("TIME(%s)=%ld\n", p2->namep, td);
				if(td > tdep) tdep = td;
				setvar("*", prefix);
				setvar("<", copys(sourcename));
				for(lp2=p1->linep ; lp2!=0 ; lp2 = lp2->nextp)
					if(implcom = lp2->shp) break;
				goto endloop;
				}
			}
		}
	}

endloop:


if(ptime<tdep || (ptime==0 && tdep==0) )
	{
	ptime = (tdep>0 ? tdep : prestime() );
	setvar("@", p->namep);
	setvar("?", mkqlist(qchain) );
	if(explcom)  docom(explcom);
	else if(implcom) docom(implcom);
	else if(p->septype == 0)
		if(p1=srchname(".DEFAULT"))
			{
			setvar("<", p->namep);
			for(lp2=p1->linep ; lp2!=0 ; lp2 = lp2->nextp)
				if(implcom = lp2->shp)
					{
					docom(implcom);
					break;
					}
			}
		else	{
			fprintf(stderr," Don't know how to make %s", p->namep);
			fatal("");
			}

	setvar("@", 0);
	if( (ptime = exists(p->namep)) == 0)
		ptime = prestime();
	}

else if(!questflag && reclevel==0  &&  didwork==0)
	printf("`%s' is up to date.\n", p->namep);

if(questflag && reclevel==0)
	exit(ndocoms>0 ? -1 : 0);

p->done = 2;
if(ptime1 > ptime) ptime = ptime1;
p->modtime = ptime;
return(ptime);
}

docom(q)
struct shblock *q;
{
char *s;
struct varblock *varptr();
int status;
int ign, nopr;
char string[400];

++ndocoms;
if(questflag)
	return;

if(touchflag)
	{
	s = varptr("@")->varval;
	if(!silflag)
		printf("touch(%s)\n", s);
	if(!noexflag)
		touch(s);
	}

else for( status = 0; q!=0 ; q = q->nextp )
	{
	subst(q->shbp,string);

	ign = ignerr;
	nopr = 0;
	for(s = string ; *s=='-' || *s=='@' ; ++s)
		if(*s == '-')  ign = 1;
		else nopr = 1;

	status =| docom1(s, ign, nopr);

	}
}



docom1(comstring, nohalt, noprint)
char *comstring;
int nohalt, noprint;
{
int status;

if(comstring[0] == '\0') return(0);

if(!silflag && (!noprint || noexflag) )
	{
	printf("%s%s\n", (noexflag ? "" : prompt), comstring);
	fflush(stdout);
	}

if(noexflag) return(0);

if( status = dosys(comstring, nohalt) )
	if (!nohalt)
		{
		printf("*** Error code %d", status );
		fatal(0);
		}

return(status);
}


/*
   If there are any Shell meta characters in the name,
   expand into a list, after searching directory
*/

expand(q)
struct depblock *q;
{
register char *s;
char *s1;
struct depblock *p, *srchdir();

s1 = q->depname->namep;
for(s=s1 ; ;) switch(*s++)
	{
	case '\0':
		return;

	case '*':
	case '?':
	case '[':
		if(p = srchdir(s1 , 1 , q->nextp) )
			{
			q->nextp = p;
			q->depname = 0;
			}
		return;
	}
}
