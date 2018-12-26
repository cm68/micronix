/********************************************************/
/*							*/
/*		Q/C Compiler Version 3.2		*/
/*			(Part 2)			*/
/*							*/
/*     Copyright (c) 1984 Quality Computer Systems	*/
/*							*/
/*			12/29/83			*/
/********************************************************/

#include "qstdio.h"
#include "cstddef.h"
#include "cglbdecl.c"

/*
 * This section processes function definitions
 */

static argcnt;

/* Look for a function definition */
isfunc(funcname, type, sc)
register char *funcname;
struct typeinfo *type;
	{
	register char *p;
	char *index();
	struct st *addglb();

	locptr = STARTLOC;	/* clear local symbols */
	locspace =		/* amount of local stack space */
	nregvar = 0;		/* # of register variables used */
	procarg();
	if (!nextc('{') && argcnt == 0)
		return FALSE;
	if (verbose)
		qprintf("=== %s()\n", funcname);
	dumplits();		/* dump literals for last function */
	infunc = TRUE;
	addglb(funcname, type, sc, ID_VAR, DECL_GLB);
	if (libflag) {		/* is this a library generation */
		closeout();	/* close last module file */
		if (p = index(outfil, ':'))/* build new output filename */
			++p;
		else
			p = outfil; /* no drive specified */
		getfilename(funcname, p, defext);
		while (p = index(p, '_')) /* no underscore in filename */
			*p = '1';
		getoutfil();
		}
	if (strcmp(funcname, "main") == 0) {
		mainflag = TRUE;
		shell(argcnt);	/* parse command line */
		}
	codeseg();
	glblabel(funcname, sc); /* print function name */
	needpunc('{');		/* function must begin with { */
	prologue(funcname);	/* beginning code for function */
	compound();		/* evaluate all statments */
	epilogue(funcname);	/* ending code for function */
	infunc = FALSE;
	return TRUE;
	}
/* Process all args for a function */
procarg()
	{
	int sc;
	struct typeinfo *type;
	struct st *addloc(), *findloc();
	char name[NAMESIZE];
	argcnt = 0;
	while (!matchc(')') && !endst()) {
		if (symname(name)) {
			if (findloc(name)) /* already defined? */
				multidef(name);
			else {	/* add arg to symbol table */
				addloc(name, (struct typeinfo *) NULL,
					SC_ARG, ID_VAR, 0);
				++argcnt;
				}
			}
		if (!nextc(')') && !matchc(','))
			error("Expected comma");
		}
	while (isdecl(&sc, &type)) {	/* look for arg declarations */
		if (sc == SC_STATIC || sc == SC_EXTERN) {
			scerr();	/* invalid storage class */
			sc = SC_ARG;
			}
		declarg((sc==SC_NONE)? SC_ARG: sc, type);
		}
	}
/* Add arguments to local symbol table */
declarg(sc, type)
register sc;
struct typeinfo *type;
	{
	int info;
	char name[NAMESIZE];
	register struct st *sym;
	register struct typeinfo *t;
	struct typeinfo *declvar(), *ptrto(), *ptrtype;
	struct st *findloc();

	do {
		t = declvar(name, type, SC_ARG, &info);
		if (t->t_code == T_ARRAY) { /* chg array-of-x to ptr-to-x */
			reftype(ptrtype = ptrto(t->t_base.p_type));
			unreftype(t);
			t = ptrtype;
			}
		else if (t->t_code > T_SIMPLE) {
			error("Argument can't be that type");
			continue;
			}
		if (sc == SC_REG && !(info=needreg(t)))
			sc = SC_ARG;	/* can't put in register */
		if ((sym=findloc(name)) == NULL)
			error("Name not in argument list");
		else if (sym->st_type != NULL)
			multidef(name);
		else {			/* fill in symbol table info */
			sym->st_type = t;
			sym->st_sc = sc;
			if (sc == SC_REG)
				sym->st_info = info;
			}
		} while (matchc(','));
	ns();
	}
/* Do function begin code */
prologue(funcname)
char *funcname;
	{
	int sc;
	struct typeinfo *type;
	register offset, regno;
	register struct st *sym;

	retvalue = retlab = 0;	       /* no return statement found yet */
	if (trace.is_on)
		gentrace('>', funcname);
	while (isdecl(&sc, &type))	/* process local declarations */
		/* if storage class not set, use default */
		declloc((sc==SC_NONE) ? SC_AUTO: sc, type);
	codeseg();
	if (locspace == 0) {		/* generate call to appropriate */
		if (nregvar == 0) {	/* entry routine */
			if (argcnt != 0)
				callib("en"); /* just set stk frame */
			/* else nothing to do */
			}
		else
			callib("enr");	/* save registers */
		}
	else	{
		if (nregvar == 0)
			callib("ens");	/* alloc stk space for locals */
		else
			callib("ensr"); /* alloc stk space & save reg */
		defword();	/* specify how much stk space needed */
		outdec(-locspace);
		nl();
		}
	offset = locspace + ARGOFFSET;		/* fix arg offsets */
	for (sym=STARTLOC+1-argcnt; sym<= STARTLOC; ++sym) {
		if (sym->st_type == NULL)	/* undeclared args */
			reftype(sym->st_type=inttype);/* default to int */
		regno = (sym->st_sc == SC_REG)? sym->st_info: 0;
		sym->st_sc = SC_AUTO;
		sym->st_info = offset;
		if (regno)			/* register assigned? */
			argtoreg(sym, regno);	/* copy arg into preg */
		offset += 2;	/* offset from local stk frame pointer */
		}
	}
/* Do function end code */
epilogue(funcname)
char *funcname;
	{
	register struct st *sym;
	register struct typeinfo *type;
	register sc;
	register char *name;
	char *regname();

	if (retlab)			/* was an early return found? */
		linelabel(retlab);	/* if so, print return label */
	if (trace.is_on)
		gentrace('<', funcname);
	if (retvalue)			/* is there a return value? */
		dotest();		/* if so, test for zero */
	if (locspace == 0) {		/* generate call to exit routine */
		if (nregvar == 0) {
			if (argcnt != 0)
				restsfp(); /* just restore stk frame */
			/* else nothing to do */
			cret();
			}
		else
			callib("exr");	/* restore registers */
		}
	else	{
		if (nregvar == 0)
			callib("exs");	/* clean up stack only */
		else
			callib("exrs"); /* restore regs & clean up stk */
		defword();	/* specify how much stk space to clear */
		outdec(locspace);
		nl();
		}
	minsym = imin(minsym, locptr-glbptr); /* remember symtab space */
	for (sym = STARTLOC; sym > locptr; --sym) {
		name = sym->st_name;
		sc = sym->st_sc;
		type = sym->st_type;
		if (type->t_code == T_LABEL && sc == SC_NONE)
			errname("Undefined label", name);
		else if (type->t_code == T_STRUCT
		     && sc == SC_TYPE) {	/* local template? */
			if (type->t_size == 0)
				tagerr(name);	/* undefined */
			else	/* delete member list from member table */
				delmem(type->t_base.memlist);
			}
		unreftype(type);
		if (libflag) {			/* library gen run? */
			if (sc == SC_EXTERN	/* declare EXTs */
			   && strcmp(name, funcname) != 0)
				declext(name);
			else if (sc == SC_REG)
				declext(regname(sym->st_info));
			}
		}
	}
/* end of CC2.C */
;
	}
/* Declare (or find) a structure/union tag */
struct typeinfo *
decltag(typecode) /* T_STRUCT or