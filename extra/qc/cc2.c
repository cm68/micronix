/********************************************************/
/* */
/* Q/C Compiler Version 3.1		 */
/* (Part 2)			 */
/* */
/* Copyright (c) 1983 Quality Computer Systems	 */
/* */
/* 08/05/82			 */
/********************************************************/

#include "qstdio.h"
#include "cstddef.h"
#include "globals.h"

static		argcnt;

/* Look for a function definition */
isfunc(funcname, type, sc)
    register char  *funcname;
    struct typeinfo *type;
    int		    sc;
{
    register char  *p;
    char           *index();
    struct st      *addglb();

    locptr = STARTLOC;		/* clear local symbols */
    locspace =			/* amount of local stack space */
	nregvar = 0;		/* # of register variables used */
    procarg();
    if (!nextc('{') && argcnt == 0)
	return FALSE;
    if (verbose)
	qprintf("=== %s()\n", funcname);
    dumplits();			/* dump literals for last function */
    infunc = TRUE;
    addglb(funcname, type, sc, ID_VAR, DECL_GLB);
    if (libflag) {		/* is this a library generation */
	closeout();		/* close last module file */
	if (p = index(outfil, ':'))	/* build new output filename */
	    ++p;
	else
	    p = outfil;		/* no drive specified */
	getfilename(funcname, p, defext);
	while (p = index(p, '_'))	/* no underscore in filename */
	    *p = '1';
	getoutfil();
    }
    if (astreq(funcname, "main", 4)) {
	mainflag = TRUE;
	shell();		/* parse command line */
    }
    codeseg();
    glblabel(funcname, sc);	/* print function name */
    needpunc('{');		/* function must begin with { */
    procdecl();			/* process local declarations */
    codeseg();
    prologue(funcname);		/* beginning code for function */
    compound();			/* evaluate all statments */
    epilogue(funcname);		/* ending code for function */
    infunc = FALSE;
    return TRUE;
}
/* Process all args for a function */
procarg()
{
    int		    sc;
    struct typeinfo *type;
    struct st      *addloc(), *findloc();
    char	    name   [NAMESIZE];
    argcnt = 0;
    while (!matchc(')') && !endst()) {
	if (symname(name)) {
	    if (findloc(name))	/* already defined? */
		multidef(name);
	    else {		/* add arg to symbol table */
		addloc(name, NULL, SC_ARG, ID_VAR, 0);
		++argcnt;
	    }
	}
	if (!nextc(')') && !matchc(','))
	    error("Expected comma");
    }
    while (isdecl(&sc, &type)) {/* look for arg declarations */
	if (sc == SC_STATIC || sc == SC_EXTERN) {
	    scerr();		/* invalid storage class */
	    sc = SC_ARG;
	}
	declarg((sc == SC_NONE) ? SC_ARG : sc, type);
    }
}
/* Add arguments to local symbol table */
declarg(sc, type)
    register int    sc;
    struct typeinfo *type;
{
    int		    info;
    char	    sname  [NAMESIZE];
    register struct st *sym;
    register struct typeinfo *t;
    struct typeinfo *declvar(), *ptrtype;
    struct st      *findloc();

    do {
	t = declvar(sname, type, SC_ARG, &info);
	if (t->t_code == T_ARRAY) {	/* chg array-of-x to ptr-to-x */
	    reftype(ptrtype = ptrto(t->t_base));
	    unreftype(t);
	    t = ptrtype;
	} else if (t->t_code > T_SIMPLE) {
	    error("Argument can't be that type");
	    continue;
	}
	if (sc == SC_REG && !(info = needreg(t)))
	    sc = SC_ARG;	/* can't put in register */
	if ((sym = findloc(sname)) == NULL)
	    error("Name not in argument list");
	else if (sym->st_type != NULL)
	    multidef(sname);
	else {			/* fill in symbol table info */
	    sym->st_type = t;
	    sym->st_sc = sc;
	    if (sc == SC_REG)
		sym->st_info = info;
	}
    } while (matchc(','));
    ns();
}
/* Do local declarations */
procdecl()
{
    int		    sc    , type;
    while (isdecl(&sc, &type))
	/* if storage class not set, use default */
	declloc((sc == SC_NONE) ? SC_AUTO : sc, type);
}
/* Do function begin code */
prologue(funcname)
    char           *funcname;
{
    register int    offset, regno;
    register struct st *p;

    if (trace)
	gentrace('>', funcname);
    if (locspace == 0) {	/* generate call to appropriate */
	if (nregvar == 0) {	/* entry routine */
	    if (argcnt != 0)
		callib("en");	/* just set stk frame */
	    /* else nothing to do */
	} else
	    callib("enr");	/* save registers */
    } else {
	if (nregvar == 0)
	    callib("ens");	/* alloc stk space for locals */
	else
	    callib("ensr");	/* alloc stk space & save reg */
	defword();		/* specify how much stk space needed */
	outdec(-locspace);
	nl();
    }
    offset = locspace + ((Z80) ? 6 : 4);	/* fix arg offsets */
    for (p = STARTLOC + 1 - argcnt; p <= STARTLOC; ++p) {
	if (p->st_type == NULL)	/* undeclared args */
	    reftype(p->st_type = inttype);	/* default to int */
	regno = (p->st_sc == SC_REG) ? p->st_info : 0;
	p->st_sc = SC_AUTO;
	p->st_info = offset;
	if (regno)		/* register assigned? */
	    argtoreg(p, regno);	/* copy arg into preg */
	offset += 2;		/* offset from local stk frame pointer */
    }
    retvalue = retlab = 0;	/* signal no return statement found */
}
/* Do function end code */
epilogue(funcname)
    char           *funcname;
{
    register struct st *sym;
    register struct typeinfo *type;
    register	    length;

    if (retlab)			/* was an early return found? */
	linelabel(retlab);	/* if so, print return label */
    if (trace)
	gentrace('<', funcname);
    if (retvalue)		/* is there a return value? */
	dotest();		/* if so, test for zero */
    if (locspace == 0) {	/* generate call to exit routine */
	if (nregvar == 0) {
	    if (argcnt != 0)
		restsfp();	/* just restore stk frame */
	    /* else nothing to do */
	    cret();
	} else
	    callib("exr");	/* restore registers */
    } else {
	if (nregvar == 0)
	    callib("exs");	/* clean up stack only */
	else
	    callib("exrs");	/* restore regs & clean up stk */
	defword();		/* specify how much stk space to clear */
	outdec(locspace);
	nl();
    }
    minsym = imin(minsym, locptr - glbptr);	/* remember symtab space */
    length = strlen(funcname);
    for (sym = STARTLOC; sym > locptr; --sym) {
	type = sym->st_type;
	if (type->t_code == T_LABEL && sym->st_sc == SC_NONE)
	    errname("Undefined label", sym->st_name);
	unreftype(type);
	if (libflag) {		/* library gen run? */
	    if (sym->st_sc == SC_EXTERN	/* declare EXTs */
		&& !astreq(sym->st_name, funcname, length))
		declext(sym->st_name);
	    else if (sym->st_sc == SC_REG)
		declext(regname(sym->st_info));
	}
    }
}
/* end of CC2.C */
