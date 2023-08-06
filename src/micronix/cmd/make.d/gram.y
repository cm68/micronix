%{#include "defs"
%}
%term NAME SHELLINE START MACRODEF COLON DOUBLECOLON GREATER


%%

%{
struct depblock *pp;
FSTATIC struct shblock *prevshp;

FSTATIC struct nameblock *lefts[NLEFTS];
struct nameblock *leftp;
FSTATIC int nlefts;

struct lineblock *lp, *lpp;
FSTATIC struct depblock *prevdep;
FSTATIC int sepc;
%}


file:
	| file comline
	;

comline:  START
	| MACRODEF
	| START namelist deplist shellist = {
	    while( --nlefts >= 0)
		{
		leftp = lefts[nlefts];
		if(leftp->septype == 0)
			leftp->septype = sepc;
		else if(leftp->septype != sepc)
			fprintf(stderr, "Inconsistent rules lines for `%s'\n",
				leftp->namep);
		else if(sepc==ALLDEPS && *(leftp->namep)!='.' && $4!=0)
			{
			for(lp=leftp->linep; lp->nextp!=0; lp=lp->nextp)
			    if(lp->shp)
				fprintf(stderr, "Multiple rules lines for `%s'\n",
				    leftp->namep);
			}

		lp = intalloc(sizeof(*lp));
		lp->nextp = 0;
		lp->depp = $3;
		lp->shp = $4;

		if(equals(leftp->namep, ".SUFFIXES") && $3==0)
			leftp->linep = 0;
		else if(leftp->linep == 0)
			leftp->linep = lp;
		else	{
			for(lpp = leftp->linep; lpp->nextp!=0;
				lpp = lpp->nextp) ;
				if(sepc == ALLDEPS)
					lpp->shp = 0;
			lpp->nextp = lp;
			}
		}
	}
	| error
	;

namelist: NAME	= { lefts[0] = $1; nlefts = 1; }
	| namelist NAME	= { lefts[nlefts++] = $2;
	    	if(nlefts>NLEFTS) fatal("Too many lefts"); }
	;

deplist:	= { fatal("Must be a separator on rules line"); }
	| dlist
	;

dlist:  sepchar	= { prevdep = 0;  $$ = 0; }
	| dlist NAME	= {
			  pp = intalloc(sizeof(*pp));
			  pp->nextp = 0;
			  pp->depname = $2;
			  if(prevdep == 0) $$ = pp;
			  else  prevdep->nextp = pp;
			  prevdep = pp;
			  }
	;

sepchar:  COLON 	= { sepc = ALLDEPS; }
	| DOUBLECOLON	= { sepc = SOMEDEPS; }
	;

shellist:	= {$$ = 0; }
	| shlist = { $$ = $1; }
	;

shlist:	SHELLINE   = { $$ = $1;  prevshp = $1; }
	| shlist SHELLINE = { $$ = $1;
			prevshp->nextp = $2;
			prevshp = $2;
			}
	;

%%



# include "lex.c"
