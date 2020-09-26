/********************************************************/
/*							*/
/*		Q/C Compiler Version 3.2		*/
/*			(Part 1)			*/
/*							*/
/*     Copyright (c) 1984 Quality Computer Systems	*/
/*							*/
/*			02/03/84			*/
/********************************************************/

#include "qstdio.h"
#include "cstddef.h"
#include "cglbdef.c"

static initted, initcnt, lastinit;

#define BYTE	1
#define WORD	2

main(argc, argv)
int argc;
char *argv[];
	{
	init();
	getoptions(argc,argv);	/* get user options */
	getspace();		/* get table and buffer space */
#ifndef PORTABLE
	zeromem();
#endif
	inittable();
	getinfil();
	getoutfil();
	clearpeep();		/* clear peephole buffer */
	kill_line();		/* clear parsing line */
	parse();
	dumplits();		/* dump literal pool for last func */
	closeout();
	summary();
	exit(errcnt);
	}
#ifndef PORTABLE
/* Zero free memory space */
zeromem()
	{
	static char *p;
	extern char *_free;
	auto char stack;
/* WARNING: Don't use any register variables in this function or the
		calling function's registers will be zeroed */
	p = &stack;			/* stack is on stack */
	while (p > _free)
		*--p = 0;
	}
#endif
/* Initialize compiler tables */
inittable()
	{
	register n;
	register struct memtab *p;
	struct st *addglb();
	struct typeinfo *ptrto();

	inittypes();			/* initialize type table */
	lit_sym.st_type = ptrto(chartype); /* "symbol" for literal pool */
	reftype(lit_sym.st_type);
	for (n=1,p=memtab; n<nmem; ++n,++p)
		p->nextmem = p + 1;	/* initialize member table by */
	p->nextmem = NULL;		/*	chaining free space */
	if (trace.enabled)		/* force loading of "puts()" */
		addglb("puts", functype, SC_EXTERN, ID_VAR, DECL_LOC);
	}
/* Process all input text */
parse()
	{
	static sc;
	struct typeinfo *type;
	for (;;) {
		blanks();
		if (eof)
			break;
		if (isdecl(&sc, &type)) {	/* is it a declaration? */
			switch (sc) {		/* check storage class */
			case SC_REG: case SC_AUTO:
				scerr();	/* invalid storage class */
			case SC_NONE:		/* use default */
				sc = SC_GLOBAL;
				break;
			case SC_STATIC:
				sc = SC_ST_GLB;
				break;
				}
			declglb(sc, type);
			}
		else		/* must be a function definition */
			declglb(SC_GLOBAL, inttype);
		}
	if (ncmp)		/* anything left hanging? */
		error("Unbalanced braces");
	if (ifdepth)
		error("Missing #endif");
	}
/* Dump the literal pool */
dumplits()
	{
	register char *p;

	if (plitpool == litpool)
		return; 	/* if nothing there, exit...*/
	if (romflag)
		codeseg();	/* put in ROM so strings are initialized */
	else
		dataseg();
	linelabel(lit_sym.st_info); /* print literal label */
	initcnt = initted = 0;
	for (p = litpool; p < plitpool; ++p)
		initbyte(*p);
	newline();		/* end last line if not full */
	minlit = imin(minlit, litpsize-initted); /* record space left */
	plitpool = litpool;	/* reset for next time */
	lit_sym.st_info = getlabel();
	}
/* Report statistics and errors */
summary()
	{
	register char *p;
	char s[7], *itob();
	extern char *_free;
	p = _free;		/* beginning of free space */
#ifndef PORTABLE
	while (*++p == 0);	/* look for bottom of stack */
#endif
	qprintf("Symbol table entries left: %d\tMemory unused: %d\n",
		minsym, p - _free);
	qprintf("Literal space left: %d bytes\tMacro space left: %d bytes\n",
		minlit, pmacsym - pmacdef + MACSIZE);
	qprintf("%s error%s found",
		(errcnt == 0) ? "No": itob(errcnt, s, -10),
		(errcnt == 1) ? "": "s");
	}
/* Check for declarations
 * Used by:
 *	parse (externals)
 *	decltag (struct/union members)
 *	procarg (arguments)
 *	prologue (locals)
 */
isdecl(sc, type)
int *sc;
struct typeinfo **type;
	{
	register holdsc, rsc;
	register struct typeinfo *holdtype, *rtype;
	struct typeinfo *gettype(), *istypedef();
	holdtype = holdsc = 0;
	do {
		if (rsc = getsc()) {		/* storage class? */
			if (holdsc == 0)	/* first one? */
				holdsc = rsc;	/* remember it */
			else
				scerr();
			}
		if (holdtype && infunc && (int) istypedef())
			break;			/* local redefinition */
						/* of typedef name */
		if (rtype = gettype()) {
			if (holdtype == NULL)
				holdtype = rtype;
			else
				typerr();
			}
		} while (rsc || rtype); 	/* loop if either found */
	*sc = holdsc;
	if (holdsc && !holdtype)		/* if storage class but */
		*type = inttype;		/* no type, use default */
	else
		*type = holdtype;
	return (holdsc || holdtype);
	}
/* Declare a global variable */
declglb(sc, type)
struct typeinfo *type;
	{
	register struct typeinfo *t;
	register t_sc;
	struct typeinfo *declvar();
	struct st *addglb();
	char name[NAMESIZE];
	int info;
	if (matchc(';'))
		return; 		/* probably struct template */
	do {
		t = declvar(name, type, sc, &info);
		if (t->t_code == T_FUNC) {
			if (isfunc(name, t, sc)) {
				unreftype(t);
				return;
				}
			else t_sc = (sc==SC_ST_GLB)? SC_ST_GLB: SC_EXTERN;
			}
		else
			t_sc = sc;
		addglb(name, t, t_sc, ID_VAR, DECL_GLB);
		unreftype(t);		/* eliminate extra reference */
		} while (matchc(','));
	ns();	/* check for semicolon */
	}
/* Declare local variables */
declloc(sc, type)
register sc;
struct typeinfo *type;
	{
	register locsc;
	int info;
	char name[NAMESIZE];
	register struct typeinfo *t;
	struct typeinfo *declvar();
	struct st *addglb(), *addloc(), *findloc();
	if (matchc(';'))
		return; 	/* probably struct template */
	do {
		t = declvar(name, type, sc, &info);
		if (t->t_code == T_FUNC) {
			needpunc(')');
			locsc = SC_EXTERN;
			}
		else {
			if (findloc(name))
				multidef(name);
			locsc = sc;
			}
		switch (locsc) {
		case SC_EXTERN:
			addglb(name, t, SC_EXTERN, ID_VAR, DECL_LOC);
			break;
		case SC_REG:
			if (info = needreg(t))
				break;		/* ok to go in reg */
			else
				locsc = SC_AUTO;
		case SC_AUTO:
			info = locspace;
			locspace += t->t_size;	/* space used */
			break;
			}
		addloc(name, t, locsc, ID_VAR, info);
		unreftype(t);		/* eliminate extra reference */
		} while (matchc(','));
	ns();				/* check for semicolon */
	}
/* Get required array size */
arraysize()
	{
	struct operand cexp;
	register size;
	if (matchc(']'))
		return -1;		/* null size */
	if (constexp(&cexp) != CONSTANT) {
		cexperr();		/* not a constant expression */
		size = 1;
		}
	else {
		size = cexp.op_val;
		if (size < 0) {
			error("Negative array size illegal");
			size = -size;
			}
		}
	needpunc(']');
	return size;
	}
/* Declare (or find) a structure/union tag */
struct typeinfo *
decltag(typecode) /* T_STRUCT or T_UNION */
	{
	char name[NAMESIZE];
	struct typeinfo *declvar(), *uniquetype(), *memtype;
	struct st *addglb(), *addloc(), *findtag();
	struct memtab *prevmem, *addmem();
	int memsc, info;
	register struct st *tp, *mp;
	register struct typeinfo *type, *mt;
	register size, offset, hastag;

	prevmem = type = tp = NULL;
	hastag = FALSE;
	if (!nextc('{')) {	/* should be tag name */
		if (!symname(name))
			illname();
		else {
			tp = findtag(name); /* tag declared? */
			hastag = TRUE;
			}
		}
	if (tp != NULL) 		/* tag is declared */
		type = tp->st_type;
	else {				/* first appearance */
		type = uniquetype(typecode, (union baseinfo *)NULL, 0);
		if (hastag) {		/* declare tag if there is one */
			if (infunc)
			    tp = addloc(name,type,SC_TYPE,ID_STRUCT,0);
			else
			    tp = addglb(name,type,SC_TYPE,ID_STRUCT,DECL_GLB);
			}
		}
	if (matchc('{')) {	/* if there was a tag, we have it now */
		if (type->t_size != 0)
			multidef(name);
		size = offset = 0;
		while (isdecl(&memsc, &memtype)) {
		    if (memsc != SC_NONE)
			error("Member can't have storage class");
		    do {
			mt = declvar(name,memtype,SC_MEMBER,&info);
			if (mp=findtag(name)) {
				if (mp->st_type != mt
				 || mp->st_info != offset
				 || mp->st_sc != SC_MEMBER)
					error("Member has another meaning");
				}
			else if (mt == type || mt->t_code == T_FUNC) {
				error("Can't be a member");
				continue;
				}
			else		/* declare the member */
			    mp=(infunc) ?
				addloc(name,mt,SC_MEMBER,ID_STRUCT,offset):
				addglb(name,mt,SC_MEMBER,ID_STRUCT,offset);
			if (typecode == T_STRUCT) {
				prevmem = addmem(mp, type, prevmem);
				offset = size += mt->t_size;
				}
			else		/* union */
				size = imax(size, mt->t_size);
			unreftype(mt);	/* eliminate extra reference */
			} while (matchc(','));
		    ns();
		    }
		needpunc('}');
		type->t_size = size;	 /* fill in structure size */
		}
	return type;
	}
/* Declare a variable */
struct typeinfo *
declvar(name, type, sc, info)
char name[];
register struct typeinfo *type;
register sc;
int *info;
	{
	struct parsestack ts;
	struct typeinfo *loadtype();
	register do_init, putds, size;

	ts.curr_ptr = ts.stack; 	/* init local type parsing stack */
	dodecl(&ts, name);		/* analyze the declarator */
	type = loadtype(type, &ts);
	do_init = matchc('=');		/* check for initialization */
	initcnt = initted = 0;		/* amount initialized */
	putds = FALSE;
	if ((sc==SC_GLOBAL || sc==SC_ST_GLB) && type->t_code!=T_FUNC) {
		if (romflag && do_init)
			codeseg();
		else
			dataseg();
		glblabel(name, sc);
		putds = TRUE;
		}
	else if (sc == SC_STATIC) {
		dataseg();
		linelabel(*info = getlabel());
		putds = TRUE;
		}
	reftype(type);		/* preserve type during initialization */
	if (do_init) {
	    if (!putds)
		error("Can only initialize global and static variables");
	    else
		initvar(type);
	    }
	else if (putds) {	/* default initialization */
		size = type->t_size;
		if ((size > INITSIZE && initflag == FALSE)
		   || romflag == TRUE) { /* can't initialize RAM */
			defstorage();
			outdec(size);
			nl();
			}
		else
			definit(size);
		}
	newline();			/* end last line if necessary */
	if (type->t_code == T_ARRAY && type->t_size <= 0
	   && sc != SC_EXTERN && sc != SC_ARG) {
		sizerr();
		type->t_size = S_INT;
		}
	return type;
	}
/* Allocate a register if possible */
needreg(type)
struct typeinfo *type;
	{
	register t_code;

	t_code = type->t_code;
	if (t_code != T_PTR && t_code != T_INT && t_code != T_UNSIGNED
	   || nregvar >= MAXREG)
		return 0;
	else
		return ++nregvar;
	}
/* Do initialization for one variable */
initvar(tp)
struct typeinfo *tp;
	{
	if (matchc('{'))
		initaggreg(tp, TRUE);
	else {
		switch (tp->t_code) {
		case T_UNION:
			error("Can't initialize unions");
			initword();	/* clear out what's there */
			break;
		case T_STRUCT:
		case T_ARRAY:
			initaggreg(tp, FALSE);
			break;
		case T_CHAR:
			initchar();
			break;
		default:
			initword();
			break;
			}
		}
	}
/* Initialize an aggregate */
initaggreg(tp, needbrace)
register struct typeinfo *tp;
	{
	struct typeinfo *arraybase;
	register struct memtab *currmem;
	register amt_init, size, typecode;
	int prev_init, resize, warned;
	static level = 0;	/* must be aggregate if level > 1 */

	++level;
	warned = FALSE;
	size = tp->t_size;
	resize = (size == -1);
	prev_init = initted;
	typecode = tp->t_code;
	if (typecode == T_STRUCT)
		currmem = tp->t_base.memlist;
	else if (typecode == T_ARRAY)
		arraybase = tp->t_base.p_type;
	else if (level > 1)
		error("Only aggregates can be initialized this way");
	for (;;) {
		if (typecode == T_STRUCT) {
			if (currmem == NULL)
				initword();
			else {
				initvar(currmem->p_sym->st_type);
				currmem = currmem->nextmem;
				}
			}
		else if (typecode == T_ARRAY) {
			if (arraybase->t_code == T_CHAR && matchc('"')) {
				amt_init = initstr(tp);
				break;
				}
			else
				initvar(arraybase);
			}
		else
			initvar(tp);
		amt_init = initted - prev_init;
		if (amt_init == size && !needbrace)
			break;		/* got enough for this guy */
		if (amt_init > size && !resize && !warned) {
			error("Too many initializers");
			warned = TRUE;
			}
		/* check for punctuation after constant expr */
		if (matchc(','))
			continue; /* more inits for this guy */
		if (ch() == '}' || ch() == ';' || chkeyword())
			break;	/* end of inits for this variable */
		if (isdigit(ch())) {
			needpunc(',');	/* looks like more inits */
			continue;
			}
		if (isletter(ch())) {	/* is this another var? */
			needpunc('}');
			needbrace = FALSE;
			needpunc(',');
			putback(',');	/* supply missing comma */
			break;
			}
		while (ch() != ',' && !endst() && ch() != '}')
		       gch();	/* get rid of whatever is left */
		break;		/* and quit */
		}
	if (needbrace)
		needpunc('}');	/* skip ending delimiter if any */
	if (resize)		/* fix size in type table */
		tp->t_size = amt_init;
	else			/* default init if any left */
		initted += definit(size - amt_init);
	--level;
	}
/* Initialize an array of characters with a string */
initstr(tp)
struct typeinfo *tp;
	{
	register c, size, str_size, warned;

	size = tp->t_size;
	warned = FALSE;
	str_size = 0;
	do {
		c = getstring();
		if (str_size == size) {
		    if (!warned) {
			error("String is bigger than array");
			warned = TRUE;
			}
		    }
		else {
			initbyte((c == -1)? '\0': c);
			++str_size;
			}
		} while (c != -1);
	return str_size;
	}
/* Initialize a character scalar */
initchar()
	{
	struct operand cexp;
	register val;
	switch (constexp(&cexp)) {
	case CONSTADDR:
		error("Char cannot hold address");
		val = 0;
		break;
	case CONSTANT:
		val = cexp.op_val;
		break;
	default:
		initerr();
		val = 0;
		break;
		}
	initbyte(val);
	}
/* Initialize a byte */
initbyte(val)
	{
	if (lastinit == BYTE)
		++initcnt;
	else {
		initcnt = 1;
		lastinit = BYTE;
		}
	if (initcnt % 10 == 1 || !midline) {
		initcnt = 1;
		newline();
		defbyte();
		}
	else
		outbyte(',');	/* separate initializers */
	outdec(val & 0xFF);	/* insure value is single byte */
	++initted;
	}
/* Initialize a word */
initword()
	{
	struct operand cexp;

	if (lastinit == WORD)
		++initcnt;
	else {
		initcnt = 1;
		lastinit = WORD;
		}
	if (initcnt % 8 == 1 || !midline) {
		initcnt = 1;
		newline();
		defword();
		}
	else
		outbyte(',');	/* separate initializers */
	switch (constexp(&cexp)) {
	case CONSTANT:
	case CONSTADDR:
		if (cexp.op_sym == NULL)
			outdec(cexp.op_val);
		else
			prtsymname(cexp.op_sym, cexp.op_val);
		break;
	default:
		initerr();
		outdec(0);
		break;
		}
	initted += 2;
	}
/* end of CC1.C */
();		/* not a constant expression */
		size = 1;
		}
	else {
		size = cexp.op_val;
		if (s