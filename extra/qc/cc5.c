/********************************************************/
/* */
/* Q/C Compiler Version 3.1		 */
/* (Part 5)			 */
/* */
/* Copyright (c) 1983 Quality Computer Systems	 */
/* */
/* 07/23/83			 */
/********************************************************/

#include "qstdio.h"
#include "cstddef.h"
#include "globals.h"

/*
 * This section identifies tokens in declarations and manages all of the
 * compiler tables
 */

/* Check next token to see if it is a keyword */
chkeyword()
{
    return (chks("break") || chks("case") || chks("continue")
	    || chks("default") || chks("do") || chks("double")
	    || chks("else") || chks("entry") || chks("enum")
	    || chks("float") || chks("for") || chks("goto")
	    || chks("if") || chks("return") || chks("sizeof")
	    || chks("struct") || chks("switch")
	    || chks("union") || chks("while") || chkdecl());
}
/* Check to see if we have a declaration */
chkdecl()
{
    return (chktype() || chksc());
}
/* Check for a valid C variable type */
chktype()
{
    blanks();
    if (chks("int"))
	return T_INT;
    else if (chks("char"))
	return T_CHAR;
    else if (chks("unsigned"))
	return T_UNSIGNED;
    else if (chks("struct"))
	return T_STRUCT;
    else if (chks("union"))
	return T_UNION;
    else if (chks("short"))
	return T_SHORT;
    else if (chks("long"))
	return T_LONG;
    else
	return T_NONE;
}
/* Check for a valid C variable storage class */
chksc()
{
    blanks();
    if (chks("register"))
	return SC_REG;
    else if (chks("static"))
	return SC_STATIC;
    else if (chks("extern"))
	return SC_EXTERN;
    else if (chks("typedef"))
	return SC_TYPE;
    else if (chks("auto"))
	return SC_AUTO;
    else
	return SC_NONE;
}
/* Get a valid C variable type if present */
struct typeinfo *
gettype()
{
    register	    type, newtype;
    struct typeinfo *istypedef(), *decltag(), *p_type;

    type = T_NONE;		/* start with no type */
    for (;;) {			/* process all type specifiers */
	switch (newtype = chktype()) {
	case T_LONG:
	    notavail("long integers");
	case T_SHORT:
	    if (type == T_NONE)
		type = T_INT;
	    else
		typerr();
	    break;
	case T_UNION:
	case T_STRUCT:
	case T_UNSIGNED:
	case T_CHAR:
	    if (type == T_NONE)
		type = newtype;
	    else
		typerr();
	    break;
	case T_INT:
	    if (type == T_CHAR)	/* type may be T_INT if we */
		typerr();	/* found long/short already */
	    else if (type == T_NONE)	/* leave unsigned alone */
		type = T_INT;
	    break;
	default:
	    switch (type) {
	    case T_STRUCT:
	    case T_UNION:
		return decltag(type);
	    case T_UNSIGNED:
		return unsgntype;
	    case T_CHAR:
		return chartype;
	    case T_INT:
		return inttype;
	    case T_NONE:
		if (p_type = istypedef())
		    skip();	/* skip typedef name */
		return p_type;
	    }
	}
	skip();			/* skip the type keyword */
    }
}
/* See if next token is a typedef name */
struct typeinfo *
istypedef()
{
    register struct st *sym;
    struct st      *findglb(), *findloc();
    char           *chksym(), name[NAMESIZE];

    if (!chksym(name))
	return NULL;
    if (infunc && (sym = findloc(name))
	|| (sym = findglb(name)) && sym->st_info == DECL_GLB) {
	if (sym->st_sc == SC_TYPE)
	    return sym->st_type;
    }
    return NULL;
}
/* Get a valid C variable storage class if present */
getsc()
{
    int		    sc;
    if (sc = chksc())
	skip();			/* skip over the keyword */
    return sc;
}
/* Parse an abstract declarator */
struct typeinfo *
abstdecl()
{
    register struct typeinfo *type;
    struct typeinfo *gettype(), *loadtype();
    struct parsestack ts;

    if (type = gettype()) {
	ts.curr_ptr = ts.stack;	/* initialize local stack */
	dodecl(&ts, NULL);	/* analyze the declarator */
	type = loadtype(type, &ts);
	if (type->t_code == T_ARRAY && type->t_size == -1)
	    sizerr();
	return type;
    } else
	return NULL;
}
/* Load a local type entry into the type table */
struct typeinfo *
loadtype(type, ts)
    struct typeinfo *type;
    struct parsestack *ts;
{
    register struct typestack *p, *startstack;
    register	    size;
    struct typeinfo *addtype(), *ptrto();

    startstack = ts->stack;
    for (p = ts->curr_ptr; --p >= startstack;) {
	switch (p->t_code) {
	case T_PTR:
	    type = ptrto(type);
	    break;
	case T_FUNC:
	    if (type->t_code <= T_NONE || type->t_code > T_SIMPLE) {
		error("Function can't return aggregate");
		type = inttype;
	    }
	    type = addtype(T_FUNC, type, S_FUNC);
	    break;
	case T_ARRAY:
	    if (type->t_code == T_FUNC) {
		error("No arrays of functions");
		type = ptrto(type);
	    }
	    if (p->t_size == -1 && p != startstack) {
		sizerr();
		size = type->t_size;
	    } else if (p->t_size == -1)
		size = -1;
	    else
		size = p->t_size * type->t_size;
	    type = addtype(T_ARRAY, type, size);
	    break;
	}
    }
    return type;
}
/* Process a declarator */
dodecl(ts, name)
    register struct parsestack *ts;
    register char  *name;
{
    if (matchc('*')) {
	dodecl(ts, name);
	pushtype(ts, T_PTR, 1);
	return;
    }
    if (matchc('(')) {
	dodecl(ts, name);
	needpunc(')');
    } else if (name != NULL)	/* no name for abstract declarator */
	symname(name);
    if (matchc('(')) {
	if (name == NULL	/* real declarations will do own chk */
	    || ts->curr_ptr > ts->stack)	/* pointer-to-func */
	    needpunc(')');
	pushtype(ts, T_FUNC, 1);
    } else
	while (matchc('['))
	    pushtype(ts, T_ARRAY, arraysize());
}
/* Push a type onto a local type parsing stack */
pushtype(ts, code, itemct)
    register struct parsestack *ts;
    int		    code  , itemct;
{
    register struct typestack *tsptr;

    tsptr = ts->curr_ptr;
    if (tsptr < &ts->stack[TSSIZE]) {
	tsptr->t_code = code;
	tsptr->t_size = itemct;
	++ts->curr_ptr;
    } else
	error("This type is too ornate");
}
/* Test for legal symbol name */
symname(sname)
    char           *sname;
{
    char           *chksym();
    if (!chksym(sname)) {
	skip();			/* pass by the name */
	illname();		/* illegal symbol name */
	return FALSE;
    } else if (chkeyword()) {
	skip();
	keyerr(sname);		/* illegal use of keyword */
	return FALSE;
    } else {			/* valid symbol name */
	skip();
	return TRUE;
    }
}
/* Check for a valid symbol */
char           *
chksym(name)
    register char  *name;
{
    register int    i;
    register char  *ptr;
    blanks();
    if (!isletter(ch())) {	/* name must start with an alpha */
	*name = '\0';
	return FALSE;
    }
    i = 0;
    ptr = lptr;
    while (isletnum(*ptr)) {
	if (i++ < NAMEMAX)
	    *name++ = *ptr;
	++ptr;
    }
    *name = '\0';
    return ptr;			/* return location of char after name */
}
/* Add a statement label to the local symbol table */
addlabel(labname, sc)
    register char  *labname;
    register int    sc;
{
    register struct st *p;
    register int    labno;
    struct st      *addloc(), *findloc();
    /* check if labname is already in local symbol table */
    if (p = findloc(labname)) {
	if (p->st_type->t_code != T_LABEL) {
	    multidef(labname);	/* previously */
	    return 0;		/* defined as a variable */
	} else if (p->st_sc == SC_AUTO) {
	    if (sc == SC_AUTO)	/* 2nd definition */
		multidef(labname);
	} else			/* labname previously used in goto only */
	    p->st_sc = sc;	/* define it now */
	return p->st_info;	/* return label # assigned */
    }
    addloc(labname, labeltype, sc, ID_VAR, labno = getlabel());
    return labno;
}
/* Return next available internal label number */
getlabel()
{
    static	    nextlabel = 0;
    return (++nextlabel);
}
/* Add an entry to the switch/loop queue */
addswq(p)
    struct swq     *p;
{
    if (pswq >= swq + nswq)
	error("Too many active switches or loops");
    else {
	pswq->loop = p->loop;
	pswq->exit = p->exit;
	++pswq;
    }
}
/* Delete an entry from the switch/loop queue */
delswq()
{
    struct swq     *getswq();
    if (getswq())
	--pswq;
    else
	error("No active switches or loops to delete");
}
/* Get the most recent entry in the switch/loop queue */
struct swq     *
getswq()
{
    return (pswq == swq) ? NULL : pswq - 1;
}
/* Add an entry to the switch case table */
addcase(p)
    struct case_table *p;
{
    if (pcasetab >= casetab + ncases)
	error("Too many switch cases");
    else {
	pcasetab->value = p->value;
	pcasetab->label = p->label;
	++pcasetab;
    }
}
/* Delete an entry from the switch case table */
struct case_table *
delcase()
{
    if (pcasetab > casetab)
	return (--pcasetab);
    else {
	error("No entry in case table to delete");
	return NULL;
    }
}
/* See if name is defined in macro pool */

#ifdef PORTABLE
char           *
findmac(name)
    register char  *name;
{
    register char  *ptr;
    ptr = macpool;
    while (ptr > pmacsym) {
	if (astreq(name, ptr, NAMEMAX))
	    return ptr;
	ptr -= MACSIZE;
    }
    return NULL;
}
#endif

/* Add a define macro to the macro pool */
addmac(name)
    register char  *name;
{
    register char  *p, c;
    char           *findmac();
    while (ch() == ' ' || ch() == '\t')
	gch();
    if (p = findmac(name)) {	/* already defined? */
	p = (int *)(p + MACPTR);/* get replacement text */
	while (c = gch()) {	/* redefinition must */
	    if (c != *p)	/* be the same */
		break;
	    ++p;
	}
	if (c != '\0' || *p != '\0') {	/* either not at end? */
	    multidef(name);
	    kill();
	}
	return;
    }
    if (pmacsym <= pmacdef) {
	macfull();		/* can't fit another macro */
	kill();
	return;
    }
    strcpy(pmacsym, name);
    *(int *)(pmacsym + MACPTR) = pmacdef;	/* record def location */
    while (*pmacdef++ = gch()) {/* copy definition */
	if (pmacdef >= pmacsym) {
	    macfull();		/* next char will overflow */
	    kill();
	    *(--pmacdef) = '\0';
	    break;
	}
    }
    pmacsym -= MACSIZE;
}
macfull()
{
    error("Macro (#define) pool is full");
}
/* Remove next name from the #define macro pool */
delmac()
{
    char           *ptr, *isdef();
    if (ptr = isdef())		/* if the name is defined, */
	*ptr = '\0';		/* wipe it out */
    else
	error("Can't #undef - not defined");
}
/* See if the next token is a #defined name */
char           *
isdef()
{
    char           *findmac(), name[NAMESIZE];
    if (symname(name))
	return findmac(name);
    else
	return NULL;
}
/* Evaluate a constant expression		 */
constexp(cexp)
    register struct operand *cexp;
{
    register	    sc;
    genflag = FALSE;		/* kill code generation */
    heir2(cexp);		/* no comma operators or assignment */
    genflag = TRUE;
    unreftype(cexp->op_type);
    return (cexp->op_sym) ?	/* if a symbol was found ... */
	(cexp->op_load & CONSTADDR) :	/* chk constant address */
	(cexp->op_load & CONSTANT);	/* else chk for constant */
}
/* If next token is a constant build an operand with its value */
constant(op)
    register struct operand *op;
{
    int		    val;
    if (hex(&val) || octal(&val) || decimal(&val) || charconst(&val)) {
	op->op_sym = NULL;
	reftype(op->op_type = inttype);
	op->op_load = (LOADVALUE | CONSTANT);
	op->op_val = val;
	return TRUE;
    } else
	return FALSE;
    return TRUE;
}
/* Check for hexadecimal constant */
hex(val)
    int            *val;
{
    if (match("0x") || match("0X")) {
	*val = gethex();
	return TRUE;
    } else
	return FALSE;
}
/* Convert string to hex value */
gethex()
{
    register	    v , d;
    v = 0;
    for (;;) {
	d = chupper(ch());
	if (isdigit(d))
	    d -= '0';
	else if (d >= 'A' && d <= 'F')
	    d -= ('A' - 10);
	else
	    break;
	v = 16 * v + d;
	gch();
    }
    return v;
}
/* Check for octal constant */
octal(val)
    int            *val;
{
    register	    k;
    if (!matchc('0'))
	return FALSE;
    k = 0;
    while (isdigit(ch()))
	k = k * 8 + gch() - '0';
    *val = k;
    return TRUE;
}
/* Check for decimal constant */
decimal(val)
    int            *val;
{
    register	    c;
    blanks();
    if ((c = ch()) == '+' || c == '-')
	c = nch();		/* next character must be digit */
    if (!isdigit(c))
	return FALSE;
    *val = atoi(lptr);
    do {
	gch();
    } while (isdigit(ch()));
    return TRUE;
}
/* Check for character constant */
charconst(val)
    int            *val;
{
    register	    c , k;
    if (!matchc('\''))
	return FALSE;
    k = 0;
    while ((c = gch()) != '\'' && c != 0) {
	if (c == '\\')
	    c = escape();
	k = (k << 8) + (c & 0xFF);
    }
    *val = k;
    return TRUE;
}
/* Check for string, build an operand, and add it to the literal pool */
string(op)
    struct operand *op;
{
    register	    c , label;
    static	    litfull = FALSE;

    if (!matchc('"'))
	return FALSE;
    lit_sym.st_info = lit_label;/* point to current literal pool */
    initop(op, &lit_sym, lit_sym.st_type, (LOADADDR | CONSTADDR));
    op->op_val = plitpool - litpool;	/* offset from start of litpool */
    do {
	c = getstring();
	if (plitpool - litpool >= LITSIZE) {
	    if (romflag) {	/* ROM strings go in CSEG */
		label = getlabel();
		jump(label);	/* jump over string */
	    }
	    dumplits();
	    litfull = TRUE;
	}
	*plitpool++ = (c == -1) ? '\0' : c;
    } while (c != -1);
    if (litfull) {		/* if part of literal dumped */
	dumplits();		/* already, dump the rest */
	litfull = FALSE;
	codeseg();		/* back to code segment */
	if (romflag)
	    linelabel(label);
    }
    return TRUE;
}
/* Get the next character in a literal string */
getstring()
{
    int		    c;
    switch (c = gch()) {
    case '"':
	return -1;
    case '\\':
	return escape();
    default:
	return c;
    }
}
/* Evaluate an escape character constant */
escape()
{
    register	    c , count, octal;
    switch (c = gch()) {
    case 'f':
	return '\f';
    case 'r':
	return '\r';
    case 'b':
	return '\b';
    case 't':
	return '\t';
    case 'n':
	return '\n';
    }
    if (isdigit(c)) {		/* octal sequence */
	count = 0;
	octal = c - '0';
	while (isdigit(ch()) && ++count < 3)
	    octal = octal * 8 + gch() - '0';
	return octal;
    }
    return c;
}
/*
 * Symbol table handling functions
 */
struct st      *
findglb(sname)
    char           *sname;
{
    struct st      *findvar();
    return findvar(STARTGLB, glbptr, sname, ID_VAR);
}
struct st      *
findloc(sname)
    char           *sname;
{
    struct st      *findvar();
    return findvar(locptr + 1, symtab + nsym, sname, ID_VAR);
}
struct st      *
findtag(sname)
    char           *sname;
{
    struct st      *p, *findvar();

    if (p = findvar(locptr + 1, symtab + nsym, sname, ID_STRUCT))
	return p;
    else
	return findvar(STARTGLB, glbptr, sname, ID_STRUCT);
}

#ifdef PORTABLE

struct st      *
findvar(start, end, sname, idset)
    struct st      *start;
    register struct st *end;
    register char  *sname;
    register int    idset;
{
    register struct st *p;
    for (p = start; p < end; ++p) {
	if (astreq(sname, p->st_name, NAMEMAX)
	    && p->st_idset == idset)
	    return p;
    }
    return NULL;
}
#endif

struct st      *
addglb(sname, type, sc, idset, info)
    register char  *sname;
    register struct typeinfo *type;
    int		    sc    , idset, info;
{
    struct st      *findglb(), *addvar();
    register struct st *p;

    if (idset != ID_STRUCT && (p = findglb(sname))) {
	/* check for consistency (struct info is checked when */
	/* it is declared since locals must be checked also) */
	if (sc != SC_EXTERN) {	/* is this the definition? */
	    if (p->st_sc == SC_GLOBAL || p->st_sc == SC_ST_GLB) {
		multidef(sname);/* already defined */
		return NULL;
	    }
	    p->st_sc = sc;	/* say it's defined */
	    p->st_info = info;
	}
	if (p->st_type != type)	/* types must be the same */
	    errname("Inconsistent declaration", sname);
    } else if (p = addvar(glbptr, sname, type, sc, idset, info))
	++glbptr;
    return p;
}
struct st      *
addloc(sname, type, sc, idset, info)
    char           *sname;
    struct typeinfo *type;
    int		    sc    , idset, info;
{
    struct st      *p, *addvar();

    if (p = addvar(locptr, sname, type, sc, idset, info))
	--locptr;
    return p;
}
struct st      *
addvar(p, sname, type, sc, idset, info)
    register struct st *p;
    char           *sname;
    register struct typeinfo *type;
    int		    sc    , idset, info;
{
    if (glbptr >= locptr) {
	error("Symbol table full");
	return NULL;
    }
    if (type != NULL)
	reftype(type);
    strcpy(p->st_name, sname);
    p->st_type = type;
    p->st_sc = sc;
    p->st_idset = idset;
    p->st_info = info;
    return p;
}
/*
 * Type-handling functions
 */
inittypes()
{
    register int    i;
    struct typeinfo *addtype();
    for (i = 0; i < n_types; ++i) {
	typelist[i].t_code = T_NONE;
	typelist[i].t_next = &typelist[i + 1];
    }
    typelist[n_types - 1].t_next = NULL;
    for (i = 0; i < T_MAX; ++i)
	basetypes[i] = NULL;
    basetypes[T_NONE] = typelist;
    /* define some oft-used types, and mark them permanent */
    chartype = addtype(T_CHAR, NULL, S_CHAR);
    reftype(chartype);
    inttype = addtype(T_INT, NULL, S_INT);
    reftype(inttype);
    unsgntype = addtype(T_UNSIGNED, NULL, S_UNSIGNED);
    reftype(unsgntype);
    labeltype = addtype(T_LABEL, NULL, 0);
    reftype(labeltype);
    functype = addtype(T_FUNC, inttype, 0);
    reftype(functype);
}
/* Add a new type to table */
struct typeinfo *
addtype(typecode, base, size)
    int		    typecode, size;
    struct typeinfo *base;
{
    register struct typeinfo *p;
    struct typeinfo *findtype(), *uniquetype();

    if ((p = findtype(typecode, base, size)) == NULL)
	p = uniquetype(typecode, base, size);
    return p;
}
struct typeinfo *
uniquetype(typecode, base, size)
    int		    typecode, size;
    struct typeinfo *base;
{
    register struct typeinfo *p;
    struct typeinfo *alloctype();

    if ((p = alloctype(typecode, base, size)) == NULL)
	return inttype;
    else {
	p->t_next = basetypes[typecode];
	basetypes[typecode] = p;
	return p;
    }
}
reftype(p)
    struct typeinfo *p;
{
    ++p->t_refs;
}
unreftype(p)
    struct typeinfo *p;
{
    if (--p->t_refs == 0)
	scraptype(p);
}
/* Convert "type" to "ptr-to-type" */
struct typeinfo *
ptrto(type)
    struct typeinfo *type;
{
    struct typeinfo *addtype();

    return addtype(T_PTR, type, S_PTR);
}
/* Find a type in the table */
struct typeinfo *
findtype(typecode, base, size)
    int		    typecode, size;
    struct typeinfo *base;
{
    register struct typeinfo *p, *q;

    for (p = basetypes[typecode]; p != NULL;) {
	q = p->t_next;
	if (p->t_size == size && p->t_base == base)
	    return p;
	p = q;
    }
    return NULL;
}
/* Allocate an entry in the type table */
struct typeinfo *
alloctype(typecode, base, size)
    int		    typecode, size;
    register struct typeinfo *base;
{
    register struct typeinfo *p;

    if (base != NULL)
	reftype(base);
    if (basetypes[T_NONE] == NULL && !typecollect()) {
	error("Too many different types in use");
	if (base != NULL)
	    unreftype(base);
	return NULL;
    } else {
	p = basetypes[T_NONE];
	basetypes[T_NONE] = p->t_next;
	p->t_code = typecode;
	p->t_size = size;
	p->t_refs = 0;
	p->t_next = NULL;
	p->t_base = base;
	return p;
    }
}
/* Remove a type from the type table */
scraptype(p)
    register struct typeinfo *p;
{
    register struct typeinfo *t, *prev;

    if (p->t_base != NULL)
	unreftype(p->t_base);
    prev = NULL;
    for (t = basetypes[p->t_code]; t != NULL; t = t->t_next) {
	if (t == p) {
	    freetype(prev, p);
	    break;
	}
	prev = t;
    }
}
/* Return a type node to the free list */
freetype(prev, type)
    struct typeinfo *prev;
    register struct typeinfo *type;
{
    if (prev == NULL)
	basetypes[type->t_code] = type->t_next;
    else
	prev->t_next = type->t_next;
    type->t_next = basetypes[T_NONE];
    basetypes[T_NONE] = type;
    type->t_code = T_NONE;
}
typecollect()
{
    register int    code, freed;
    register struct typeinfo *p, *prev, *next;

    freed = 0;
    for (code = T_NONE + 1; code < T_MAX; ++code) {
	prev = NULL;
	for (p = basetypes[code]; p != NULL; p = next) {
	    next = p->t_next;
	    if (p->t_refs > 0)
		prev = p;
	    else {
		/*
		 * Unused node - free it after removing the reference to its
		 * base type. Unreftype() isn't used because a node somewhere
		 * in the chain from here to the real base type may have
		 * already been freed, and that will make unreftype() choke.
		 * Instead, we just decrement the use count, and let the next
		 * collection pick it up.
		 */
		if (p->t_base != NULL)
		    p->t_base->t_refs--;
		freetype(prev, p);
		++freed;
	    }
	}
    }
    return freed;
}
/* Strip a layer off of op's type */
deref(op)
    struct operand *op;
{
    chtype(op, op->op_type->t_base);
}
/* Set type of expression to highest of either operand */
settype(lop, rop)
    struct operand *lop, *rop;
{
    register struct typeinfo *rtype;

    rtype = rop->op_type;
    if (rtype->t_code > lop->op_type->t_code)
	chtype(lop, rtype);
    unreftype(rtype);
    if (lop->op_type->t_code == T_ARRAY)	/* Change array to ptr */
	chtype(lop, ptrto(lop->op_type->t_base));
}
chtype(op, new)
    register struct operand *op;
    register struct typeinfo *new;
{
    reftype(new);
    unreftype(op->op_type);
    op->op_type = new;
    if (new->t_code == T_ARRAY)
	unarray(op);
}
basesize(op)
    struct operand *op;
{
    register struct typeinfo *p;

    p = op->op_type->t_base;
    if (p->t_size == 0)
	error("Size unknown");
    return p->t_size;
}
/* Check op to see if it is a pointer */
struct typeinfo *
isptr(op)
    struct operand *op;
{
    register struct typeinfo *type;
    type = op->op_type;
    if (type->t_code == T_PTR)
	return type;
    else if (type->t_code == T_ARRAY)
	return ptrto(type->t_base);
    else
	return NULL;
}
islval(op)
    struct operand *op;
{
    return (op->op_load & LVALUE);
}
isconstant(op)
    struct operand *op;
{
    return (op->op_load & CONSTANT);
}
isscalar(op)
    struct operand *op;
{
    return (op->op_type->t_code < T_SIMPLE) ? op->op_type : NULL;
}
isloaded(op)
    struct operand *op;
{
    return (op->op_load & (LOADVALUE | LOADADDR)) == 0;
}
isaddr(op)
    struct operand *op;
{
    return (islval(op) && op->op_sym == NULL);
}
/* end of CC5.C */
