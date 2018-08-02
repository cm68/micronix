/*********************************************************
 *							 *
 * Q/C Compiler Version 3.1				 *
 * (Part 1)						 *
 *							 *
 * Copyright (c) 1983 Quality Computer Systems		 *
 *							 *
 * 08/06/83						 *
 *							 *
 * modified by curt mayer, 2018 to be a cross and native *
 * compiler for the micronix revival                     *
 *							 *
 *********************************************************/

#include "qstdio.h"
#include "cstddef.h"
#include "globals.h"

static int	initted;

/*
 * Zero free memory space - this is everything between the start of the heap
 * to the bottom of the stack.
 * 
 * WARNING: Don't use any register variables in this function or the calling
 * function's registers will be zeroed
 */
void
zeromem()
{
    static char    *freespace, *ptr;
    char           *malloc();
    auto char	    stack;

    freespace = malloc(0);	/* beginning of free space */
    ptr = &stack;		/* stack is on stack */
    while (ptr > freespace)
	*--ptr = 0;
}

/*
 * Obtain compiler table and buffer space from free space this is really just
 * a crapload of malloc's
 */
void
getspace()
{
    char           *malloc();
    struct st      *addglb();
    struct typeinfo *ptrto();

    peepbuf = malloc(LINESIZE);
    line = malloc(LINESIZE);
    mline = malloc(LINESIZE);
    plitpool =
	litpool =
	malloc(LITSIZE);
    pcasetab =
	casetab =
	malloc(ncases * sizeof(struct case_table));
    pswq =
	swq =
	malloc(nswq * sizeof(struct swq));
    typelist = malloc(n_types * sizeof(struct typeinfo));
    inclbuf[0] = malloc(inclbsize);
    inbuf = malloc(iobufsize);
    outbuf = malloc(iobufsize);
    symtab = malloc(nsym * sizeof(struct st));
    pmacdef = malloc(macpsize);
    macpool = pmacsym = pmacdef + macpsize - MACSIZE;
    if (outbuf == NULL || symtab == NULL || pmacdef == NULL) {
	puts("Not enough table space");
	exit(1);
    }
    glbptr = STARTGLB;
    locptr = STARTLOC;
    inittypes();		/* initialize type table */
    lit_sym.st_type = ptrto(chartype);	/* "symbol" for literal pool */
    reftype(lit_sym.st_type);
    if (trace)			/* force loading of "puts()" */
	addglb("puts", functype, SC_EXTERN, ID_VAR, DECL_LOC);
}

/*
 * Initialize a byte
 */
void
initbyte(val)
    int		    val;
{
    ++initted;
    if (initted % 10 == 1 || !midline)
	defbyte();		/* start new line */
    else
	outbyte(',');		/* separate bytes */
    outdec(val & 0xFF);		/* insure value is single byte */
    if (initted % 10 == 0)
	nl();			/* filled this line */
}

void
initchar()
{
    struct operand  cexp;
    register	    val;
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

/*
 * Calculate the base size to initialize
 */
int
elemsize(tp)
    struct typeinfo *tp;
{
    register struct typeinfo *base;

    for (base = tp; base->t_code == T_ARRAY; base = base->t_base);
    return (base->t_size == S_INT) ? S_INT : S_CHAR;
}

/*
 * Initialize a word
 */
void
initword(sym, val)
    struct st      *sym;
    int		    val;
{
    static	    charcnt;
    if (!midline || initted == 0 || charcnt > 70) {
	if (initted)		/* start new line if necessary */
	    newline();
	defword();
	charcnt = 16;
    } else
	outbyte(',');		/* more room on the current line */
    charcnt += 5;
    if (sym) {
	charcnt += strlen(sym->st_name);
	prtsymname(sym, val);
    } else
	outdec(val);
    initted += 2;
}

void
inititem()
{
    struct operand  cexp;
    switch (constexp(&cexp)) {
    case CONSTANT:
    case CONSTADDR:
	initword(cexp.op_sym, cexp.op_val);
	break;
    default:
	initerr();
	initword(NULL, 0);
	break;
    }
}

void
initvar(tp)
    struct typeinfo *tp;
{
    if (elemsize(tp) == S_INT)
	inititem();
    else
	initchar();
    if (!nextc(',') && !chkeyword() && isletter(ch())) {
	needpunc(',');		/* looks like another variable */
	putback(',');		/* supply missing comma */
    }
}

void
initstr(tp)
    register struct typeinfo *tp;
{
    register	    c , size, warned;
    if (tp->t_code == T_ARRAY && tp->t_base->t_code == T_CHAR) {
	size = tp->t_size;
	warned = FALSE;
	gch();			/* strip leading quote */
	do {
	    c = getstring();
	    if (initted == size && !warned) {
		error("String is bigger than array");
		warned = TRUE;
	    } else
		initbyte((c == -1) ? '\0' : c);
	} while (c != -1);
	if (size == -1)		/* fix up type table */
	    tp->t_size = initted;
    } else
	initvar(tp);
}

/*
 * Default initialization
 */
void
definit(size, amt)
    int		    size  , amt;
{
    for (; amt > 0; amt -= size) {
	if (size == 2)
	    initword(NULL, 0);
	else
	    initbyte(0);
    }
}

void
initbrace(tp, resize)
    register struct typeinfo *tp;
{
    register struct typeinfo *tpb;
    register	    init1, warned, needbrace, ischar, base;

    warned = FALSE;
    needbrace = TRUE;
    ischar = (base = elemsize(tp)) == S_CHAR;
    init1 = initted;
    if (tp->t_code == T_STRUCT || tp->t_base->t_code == T_STRUCT)
	error("Can't initialize structures yet");
    for (;;) {
	if (matchc('{')) {
	    tpb = tp->t_base;
	    if (tpb == NULL ||
		(tpb->t_code != T_STRUCT && tpb->t_code != T_ARRAY)) {
		error("Only aggregates can be initialized this way");
		initbrace(tp, FALSE);
	    } else
		initbrace(tpb, FALSE);
	} else if (ischar)
	    initchar();
	else
	    inititem();
	if (initted - init1 > tp->t_size && !resize && !warned) {
	    error("Too many initializers");
	    warned = TRUE;
	}
	/* check for punctuation after constant expr */
	if (matchc(','))
	    continue;		/* more inits for this var */
	if (ch() == '}' || ch() == ';' || chkeyword())
	    break;		/* end of inits for this variable */
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
	    gch();		/* get rid of whatever is left */
	break;			/* and quit */
    }
    if (needbrace)
	needpunc('}');		/* skip ending delimiter if any */
    if (resize)			/* fix size in type table */
	tp->t_size = initted - init1;
    else			/* default init if any left */
	definit(base, tp->t_size - (initted - init1));
}

/*
 * Do initialization for one variable
 */
void
doinit(tp)
    register struct typeinfo *tp;
{
    register	    size;

    size = tp->t_size;
    if (matchc('{'))
	initbrace(tp, size == -1);
    else if (ch() == '"')
	initstr(tp);
    else
	initvar(tp);
    definit(elemsize(tp), size - initted);	/* init anything left */
}

/*
 * Declare a variable
 */
struct typeinfo *
declvar(sname, type, sc, info)
    char	    sname  [];
    register struct typeinfo *type;
    register int    sc;
    int            *info;
{
    struct parsestack ts;
    struct typeinfo *loadtype();
    register	    putds, size;

    ts.curr_ptr = ts.stack;	/* init local type parsing stack */
    dodecl(&ts, sname);		/* analyze the declarator */
    type = loadtype(type, &ts);
    initted = 0;
    putds = FALSE;
    if ((sc == SC_GLOBAL || sc == SC_ST_GLB) && type->t_code != T_FUNC) {
	dataseg();
	glblabel(sname, sc);
	putds = TRUE;
    } else if (sc == SC_STATIC) {
	dataseg();
	linelabel(*info = getlabel());
	putds = TRUE;
    }
    reftype(type);		/* preserve type during initialization */
    if (matchc('=')) {
	if (!putds)
	    error("Can only initialize global and static variables");
	else {
	    if (type->t_code == T_UNION)
		error("Can't initialize unions");
	    doinit(type);
	}
    } else if (putds) {		/* default initialization */
	size = type->t_size;
	if (size <= INITSIZE || initflag == TRUE)
	    definit(elemsize(type), size);
	else {			/* -I switch says don't initialize */
	    defstorage();
	    outdec(size);
	    nl();
	}
    }
    newline();			/* end last line if necessary */
    if (type->t_code == T_ARRAY && type->t_size <= 0
	&& sc != SC_EXTERN && sc != SC_ARG) {
	sizerr();
	type->t_size = S_INT;
    }
    return type;
}

/*
 * Dump the literal pool
 */
void
dumplits()
{
    register char  *ptr;
    if (plitpool == litpool)
	return;			/* if nothing there, exit... */
    if (romflag)
	codeseg();		/* put in ROM so strings are initialized */
    else
	dataseg();
    linelabel(lit_label);	/* print literal label */
    initted = 0;
    for (ptr = litpool; ptr < plitpool; ++ptr)
	initbyte(*ptr);
    newline();			/* end last line if not full */
    plitpool = litpool;		/* reset for next time */
    lit_label = getlabel();
}

/*
 * Report statistics and errors
 */
void
summary()
{
    register char  *p;
    char	    errnum [7], *itob(), *malloc();

    p = malloc(0);		/* beginning of free space */
    while (*++p == 0);		/* look for bottom of stack */

    qprintf("Symbol table entries left: %d\n", minsym);

    qprintf("Macro space left: %d bytes  Memory unused: %d bytes\n",
	    pmacsym - pmacdef + MACSIZE, p - malloc(0));

    qprintf("%s error%s found",
	    (errcnt == 0) ? "No" : itob(errcnt, errnum, -10),
	    (errcnt == 1) ? "" : "s");
}

/*
 * Declare a global variable
 */
void
declglb(sc, type)
    int		    sc;
    struct typeinfo *type;
{
    register struct typeinfo *t;
    register struct st *p;
    register	    t_sc;
    struct typeinfo *declvar();
    struct st      *addglb();
    char	    sname  [NAMESIZE];
    int		    info;
    if (matchc(';'))
	return;			/* probably struct template */
    do {
	t = declvar(sname, type, sc, &info);
	if (t->t_code == T_FUNC) {
	    if (isfunc(sname, t, sc)) {
		unreftype(t);
		return;
	    } else
		t_sc = (sc == SC_ST_GLB) ? SC_ST_GLB : SC_EXTERN;
	} else
	    t_sc = sc;
	addglb(sname, t, t_sc, ID_VAR, DECL_GLB);
	unreftype(t);		/* eliminate extra reference */
    } while (matchc(','));
    ns();			/* check for semicolon */
}

/*
 * Process all input text - this is the top of the recursive descent parser
 */
void
parse()
{
    int		    sc;
    struct typeinfo *type;
    for (;;) {
	blanks();
	if (eof)
	    break;
	if (isdecl(&sc, &type)) {	/* is it a declaration? */
	    switch (sc) {	/* check storage class */
	    case SC_REG:
	    case SC_AUTO:
		scerr();	/* invalid storage class */
	    case SC_NONE:	/* use default */
		sc = SC_GLOBAL;
		break;
	    case SC_STATIC:
		sc = SC_ST_GLB;
		break;
	    }
	    declglb(sc, type);
	} else {
	    /* must be a function definition */
	    declglb(SC_GLOBAL, inttype);
	}
    }
    if (ncmp)			/* anything left hanging? */
	error("Unbalanced braces");
    if (ifdepth)
	error("Missing #endif");
}

/*
 * Check for declarations Used by: parse (externals) procarg (arguments)
 * procdecl (locals)
 */
int
isdecl(sc, type)
    int            *sc;
    struct typeinfo **type;
{
    register	    holdsc, rsc;
    register struct typeinfo *holdtype, *rtype;
    struct typeinfo *gettype();
    holdtype = holdsc = 0;
    do {
	if (rsc = getsc()) {	/* storage class? */
	    if (holdsc == 0)	/* first one? */
		holdsc = rsc;	/* remember it */
	    else
		scerr();
	}
	if (holdtype && infunc && istypedef())
	    break;		/* local redefinition */
	/* of typedef name */
	if (rtype = gettype()) {
	    if (holdtype == NULL)
		holdtype = rtype;
	    else
		typerr();
	}
    } while (rsc || rtype);	/* loop if either found */
    *sc = holdsc;
    if (holdsc && !holdtype)	/* if storage class but */
	*type = inttype;	/* no type, use default */
    else
	*type = holdtype;
    return (holdsc || holdtype);
}

/*
 * Declare local variables
 */
void
declloc(sc, type)
    register int    sc;
    struct typeinfo *type;
{
    register	    locsc;
    int		    info;
    char	    sname  [NAMESIZE];
    register struct typeinfo *t;
    struct typeinfo *declvar();
    struct st      *addglb(), *addloc(), *findloc();
    if (matchc(';'))
	return;			/* probably struct template */
    do {
	t = declvar(sname, type, sc, &info);
	if (t->t_code == T_FUNC) {
	    needpunc(')');
	    locsc = SC_EXTERN;
	} else {
	    if (findloc(sname))
		multidef(sname);
	    locsc = sc;
	}
	switch (locsc) {
	case SC_EXTERN:
	    addglb(sname, t, SC_EXTERN, ID_VAR, DECL_LOC);
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
	addloc(sname, t, locsc, ID_VAR, info);
	unreftype(t);		/* eliminate extra reference */
    } while (matchc(','));
    ns();			/* check for semicolon */
}

/*
 * Get required array size
 */
int
arraysize()
{
    struct operand  cexp;
    register	    size;
    if (matchc(']'))
	return -1;		/* null size */
    if (constexp(&cexp) != CONSTANT) {
	cexperr();		/* not a constant expression */
	size = 1;
    } else {
	size = cexp.op_val;
	if (size < 0) {
	    error("Negative array size illegal");
	    size = -size;
	}
    }
    needpunc(']');
    return size;
}

/*
 * Declare (or find) a structure/union tag
 */
struct typeinfo *
decltag(typecode)
    int		    typecode;	/* T_STRUCT or T_UNION */
{
    char	    tag    [NAMESIZE], var[NAMESIZE];
    struct typeinfo *declvar(), *uniquetype(), *vartype;
    struct st      *addglb(), *addloc(), *findtag();
    int		    varsc , info;
    register struct st *tp, *mp;
    register struct typeinfo *type, *vt;
    register int    size, offset, hastag;

    type = NULL;
    tp = NULL;
    hastag = FALSE;
    if (!nextc('{')) {		/* tag name */
	if (!symname(tag))
	    illname();
	else {
	    tp = findtag(tag);
	    hastag = TRUE;
	}
    }
    if (!matchc('{')) {
	if (!hastag)
	    error("No tag or template");
	else if (tp == NULL)
	    error("Undeclared tag");
	else
	    type = tp->st_type;
    } else {			/* template follows */
	type = uniquetype(typecode, NULL, 0);

	if (!hastag);
	else if (tp != NULL)
	    multidef(tag);
	else if (infunc)
	    tp = addloc(tag, type, SC_TYPE, ID_STRUCT, 0);
	else
	    tp = addglb(tag, type, SC_TYPE, ID_STRUCT, DECL_GLB);

	size = offset = 0;
	while (isdecl(&varsc, &vartype)) {
	    if (varsc != SC_NONE)
		error("Member can't have storage class");
	    do {
		vt = declvar(var, vartype, SC_MEMBER, &info);
		if (mp = findtag(var)) {
		    if (mp->st_type != vt ||
			mp->st_info != offset)
			error("Member has another meaning");
		} else if (vt == type || vt->t_code == T_FUNC) {
		    error("Can't be structure member");
		    continue;
		} else {
		    if (infunc)
			addloc(var, vt, SC_MEMBER, ID_STRUCT, offset);
		    else
			addglb(var, vt, SC_MEMBER, ID_STRUCT, offset);
		}
		if (typecode == T_STRUCT)
		    offset = size += vt->t_size;
		else		/* union */
		    size = imax(size, vt->t_size);
		unreftype(vt);	/* eliminate extra reference */
	    } while (matchc(','));
	    ns();
	}
	needpunc('}');
	type->t_size = size;	/* fill in structure size */
    }
    if (type == NULL)		/* in case of error, hand back new structure
				 * type */
	type = uniquetype(typecode, NULL, 0);
    return type;
}

/*
 * Allocate a register if possible
 */
int
needreg(type)
    struct typeinfo *type;
{
    register int    t_code;

    t_code = type->t_code;
    if (t_code != T_PTR && t_code != T_INT && t_code != T_UNSIGNED
	|| nregvar >= MAXREG)
	return 0;
    else
	return ++nregvar;
}

int
main(argc, argv)
    int		    argc;
    char           *argv[];
{
    init();
    getoptions(argc, argv);	/* get user options */
    zeromem();
    getspace();			/* get table and buffer space */
    getinfil();
    getoutfil();
    kill();
    parse();
    dumplits();			/* dump literal pool for last func */
    closeout();
    summary();
    exit(errcnt);
}

/* end of CC1.C */
