/********************************************************/
/*							*/
/*		Q/C Compiler Version 3.2		*/
/*			(Part 4)			*/
/*							*/
/*     Copyright (c) 1984 Quality Computer Systems	*/
/*							*/
/*			11/27/83			*/
/********************************************************/

#include "qstdio.h"
#include "cstddef.h"
#include "cglbdecl.c"

/* This section is the recursive descent
 *	parser of C expressions.
 *	heir1() thru heir15() implement
 *	the hierarchy of operators.
 */
/* Evaluate an expression */
expression(load)
	{
	struct operand op;	/* op describes the expression found */

	pregflag = FALSE;	/* no value in preg yet */
	heir1(&op);
	if (load)		/* is the value of expression needed? */
		rvalp(&op);
	unreftype(op.op_type);	/* remove reference from type table */
	}
/*	sequence operator (,)	*/
heir1(op)
struct operand *op;
	{
	heir2(op);
	while (matchc(',')) {
		pregflag = FALSE;
		heir2(op);	/* staying at this level causes value */
				/* in preg to be right-most expression */
		}
	}
/*	assigment operators (=, +=, |=, etc.	*/
heir2(lop)
register struct operand *lop;
	{
	struct operand rop;
	register optype;
	struct typeinfo *exprtype;
	static depth = 0;	/* keep track of nesting depth */

	++depth;
	heir3(lop);
	if (matchc('=')) {
	    if (needlval(lop)) {
		doasgn(lop, &rop, depth);
		unreftype(rop.op_type);
		}
	    }
	else {
	    optype = ch();		/* check for assignment operator */
	    if ((match("+=") || match("|=") || match("^=")
		|| match("&=") || match("*=") || match("-=")
		|| match(">>=") || match("<<=") || match("/=")
		|| match("%=")) && needlval(lop)) {
		rvalpsav(lop);		/* get value of var & save addr */
		exprtype = lop->op_type;/* preserve type of expr */
		binop(lop, optype, &rop, heir2);
		chtype(lop, exprtype);
		store(lop);		/* store result (lop = lop op rop) */
		}
	    }
	--depth;
	}
/* parse a conditional expression e1 ? e2 : e3 */
#define ZERO	1
#define NONZERO 2
heir3(op1)
register struct operand *op1;
	{
	register cexp1, cexp2, cexp3;
	register label1, saveflag;
	struct operand op2, op3;
	struct typeinfo *isptr(), *ptrtype2, *ptrtype3;

	heir4(op1);
	if (!matchc('?'))
		return;
	cexp1 = cexp2 = cexp3 = FALSE;
	saveflag = genflag;
	if (isconstant(op1))
		cexp1 = (op1->op_val) ? NONZERO: ZERO;
	else {
		rvalp(op1);		/* load e1 */
		jumpcond(FALSE, label1=getlabel()); /* test e1 */
		getlabel();		/* reserve a label in case */
		}
	pregflag = FALSE;		/* new expression so preg empty */
	genflag = (saveflag && cexp1 != ZERO); /* generate code? */
	heir3(&op2);			/* evaluate e2 */
	needpunc(':');
	if (isconstant(&op2))
		cexp2 = (op2.op_val) ? NONZERO: ZERO;
	if (!cexp1) {			/* don't need jumps if e1 constant */
		rvalp(&op2);
		jump(label1 + 1);	/* don't evaluate e3 */
		linelabel(label1);	/* arrive here if e1 is false */
		}
	pregflag = FALSE;
	genflag = (saveflag && cexp1 != NONZERO);
	heir3(&op3);			/* evaluate e3 */
	genflag = saveflag;
	if (isconstant(&op3))
		cexp3 = (op3.op_val) ? NONZERO: ZERO;
	if (cexp1 && cexp2 && cexp3) {	/* constant expression? */
		op1->op_val = (cexp1==NONZERO)? op2.op_val: op3.op_val;
		return;
		}
	ptrtype2 = isptr(&op2);
	ptrtype3 = isptr(&op3);
	if ((ptrtype2 || ptrtype3) && ptrtype2 != ptrtype3
	   && cexp2 != ZERO && cexp3 != ZERO)
		error("Inconsistent use of pointers in conditional\
 expression");
	if (cexp1 == NONZERO) {
		pregflag = FALSE;	/* preg is really empty */
		rvalp(&op2);
		}
	else
		rvalp(&op3);
	if (cexp1)
		op1->op_load = EXPRESSION; /* record that op1 is loaded */
	else
		linelabel(label1 + 1);	/* arrive here if e1 is true */
	settype(&op2, &op3);
	chtype(op1, op2.op_type);	/* set type of result */
	unreftype(op2.op_type); 	/* remove ref from type table */
	}
/* parse a logical OR expression (e1 || e2) */
heir4(lop)
register struct operand *lop;
	{
	struct operand rop;
	register resultset, label1;

	heir5(lop);		/* evaluate e1 */
	resultset = testdone;	/* if lop is test or &&, result is set */
	blanks();
	if (!streq(lptr,"||"))
		return;
	label1 = getlabel();
	getlabel();		/* reserve a label in case we need it */
	rvalp(lop);
	while (match("||")) {
		jumpcond(TRUE, label1); /* if e1 TRUE, expression is TRUE */
		pregflag = FALSE;
		heir5(&rop);		/* evaluate e2 */
		rvalp(&rop);
		unreftype(rop.op_type);
		resultset &= testdone;	/* record if result is set */
		}
	if (resultset)			/* see if result already set */
		linelabel(label1);
	else {
		jumpcond(FALSE, label1 + 1); /* if e2 0, e1||e2 is 0 */
		linelabel(label1);	/* arrive here if true */
		set1(); 		/* set value of e1 || e2 to 1 */
		linelabel(label1 + 1);
		}
	testdone = TRUE;	/* record that we just did a test */
	chtype(lop, inttype);	/* result of logical OR is integer */
	}
/* parse a logical AND expression (e1 && e2) */
heir5(lop)
register struct operand *lop;
	{
	struct operand rop;
	register resultset, label1;

	heir6(lop);
	resultset = testdone;		/* if lop is test, result is set */
	blanks();
	if (!streq(lptr,"&&"))
		return;
	label1 = getlabel();
	rvalp(lop);			/* load lop in preg if necessary */
	while (match("&&")) {
		jumpcond(FALSE, label1); /* if e1 is false, we're thru */
		pregflag = FALSE;
		heir6(&rop);		/* evaluate e2 */
		rvalp(&rop);
		unreftype(rop.op_type);
		resultset &= testdone;	/* record if result is set */
		}
	if (!resultset) {		/* see if result must be set */
		jumpcond(FALSE, label1); /* if e2 is 0, e1&&e2 is 0 */
		set1();
		}
	linelabel(label1);		/* false label */
	testdone = TRUE;
	chtype(lop, inttype);		/* result of logical AND is int */
	}
/* bitwise inclusive OR */
heir6(lop)
struct operand *lop;
	{
	struct operand rop;

	heir7(lop);
	blanks();
	for (;;) {
		if (nch() != '|' && nch() != '=' && matchc('|')) {
			if (binop(lop, '|', &rop, heir7) == CONSTANT)
				lop->op_val |= rop.op_val;
			}
		else break;
		}
	}
/* bitwise exclusive OR */
heir7(lop)
struct operand *lop;
	{
	struct operand rop;

	heir8(lop);
	blanks();
	for (;;) {
		if (nch() != '=' && matchc('^')) {
			if (binop(lop, '^', &rop, heir8) == CONSTANT)
				lop->op_val ^= rop.op_val;
			}
		else break;
		}
	}
/* bitwise AND */
heir8(lop)
struct operand *lop;
	{
	struct operand rop;

	heir9(lop);
	blanks();
	for (;;) {
		if (nch() != '&' && nch() != '=' && matchc('&')) {
			if (binop(lop, '&', &rop, heir9) == CONSTANT)
				lop->op_val &= rop.op_val;
			}
		else
			break;
		}
	}
heir9(lop)
register struct operand *lop;
	{
	struct operand rop;

	heir10(lop);
	for (;;) {
		if (match("==")) {
			if (binop(lop, EQ, &rop, heir10) == CONSTANT)
				lop->op_val = (lop->op_val == rop.op_val);
			}
		else if (match("!=")) {
			if (binop(lop, NE, &rop, heir10) == CONSTANT)
				lop->op_val = (lop->op_val != rop.op_val);
			}
		else
			break;
		chtype(lop, inttype); /* result of equality op is int */
		}
	}
heir10(lop)
register struct operand *lop;
	{
	struct operand rop;

	heir11(lop);
	for (;;) {
		if (match("<=")) {
			if (binop(lop, LE, &rop, heir11) == CONSTANT)
				lop->op_val = (lop->op_val <= rop.op_val);
			}
		else if (match(">=")) {
			if (binop(lop, GE, &rop, heir11) == CONSTANT)
				lop->op_val = (lop->op_val >= rop.op_val);
			}
		else if (nch() != '<' && matchc('<')) {
			if (binop(lop, LT, &rop, heir11) == CONSTANT)
				lop->op_val = (lop->op_val < rop.op_val);
			}
		else if (nch() != '>' && matchc('>')) {
			if (binop(lop, GT, &rop, heir11) == CONSTANT)
				lop->op_val = (lop->op_val > rop.op_val);
			}
		else
			break;
		chtype(lop, inttype); /* result of relation op is int */
		}
	}
heir11(lop)
struct operand *lop;
	{
	struct operand rop;

	heir12(lop);
	blanks();
	for (;;) {
		if (!streq(lptr,">>=") && match(">>")) {
			if (binop(lop, '>', &rop, heir12) == CONSTANT)
				lop->op_val >>= rop.op_val;
			}
		else if (!streq(lptr,"<<=") && match("<<")) {
			if (binop(lop, '<', &rop, heir12) == CONSTANT)
				lop->op_val <<= rop.op_val;
			}
		else
			break;
		}
	}
heir12(lop)
struct operand *lop;
	{
	struct operand rop;
	heir13(lop);
	blanks();
	while (nch() != '=') {
		if (matchc('+')) {
			heir13(&rop);
			doadd(lop, &rop);
			}
		else if (matchc('-')) {
			heir13(&rop);
			dosub(lop, &rop);
			}
		else
			break;
		}
	}
heir13(lop)
register struct operand *lop;
	{
	struct operand rop;

	heir14(lop);
	blanks();
	while (nch() != '=') {
		if (matchc('*')) {
			heir14(&rop);
			domult(lop, &rop);
			}
		else if (matchc('/')) {
			if (binop(lop, '/', &rop, heir14) == CONSTANT)
				lop->op_val /= rop.op_val;
			}
		else if (matchc('%')) {
			if (binop(lop, '%', &rop, heir14) == CONSTANT)
				lop->op_val %= rop.op_val;
			}
		else break;
		}
	}
/* heir14 calls itself because unary operators */
/*	associate left to right */
heir14(op)
register struct operand *op;
	{
	register struct st *sym;
	register struct typeinfo *type;
	register sawparen, savegflag, savepflag;
	struct typeinfo *abstdecl(), *isptr(), *ptrto();

	if (match("++")) {		/* pre-increment */
		heir14(op);
		if (needlval(op))
			addto(op, 1);
		return;
		}
	if (match("--")) {		/* pre-decrement */
		heir14(op);
		if (needlval(op))
			addto(op, -1);
		return;
		}
	if (nextc('-')) {		/* unary minus (negation) */
		if (constant(op))	/* check for negative constant */
			return;
		gch();
		heir14(op);		/* evaluate expression */
		rvalp(op);		/* load in preg */
		cneg(); 		/* negate preg */
		return;
		}
	if (matchc('*')) {		/* indirection */
		heir14(op);		/* evaluate expression */
		if (!isptr(op))
			error("Not a pointer");
		else {
			rvalp(op);
			op->op_sym = NULL;/* record that addr of lvalue */
			op->op_load = (LOADVALUE|LVALUE); /* is in preg */
			deref(op);	/* remove a layer of ptr */
			}
		return;
		}
	if (matchc('!')) {		/* logical not (!exp) */
		heir14(op);		/* evaluate expression */
		rvalp(op);		/* load in preg */
		cnot(); 		/* logical not of preg */
		chtype(op, inttype);	/* result is always int */
		return;
		}
	if (matchc('~')) {		/* one's complement (~exp) */
		heir14(op);
		if (isconstant(op))
			op->op_val = ~op->op_val; /* constant expression */
		else {
			rvalp(op);	/* load in preg */
			ccom(); 	/* complement preg */
			}
		return;
		}
	if (matchc('&')) {		/* address */
		heir14(op);
		sym = op->op_sym;
		if (!islval(op) ||
		   (sym != NULL && sym->st_sc == SC_REG)) {
			error("Illegal address");
			return;
			}
		chtype(op, ptrto(op->op_type)); /* chg type to ptr_to_type */
		op->op_load &= ~(LVALUE|LOADVALUE); /* no longer lvalue */
		if (op->op_sym != NULL) { /* if not already loaded...*/
			op->op_load |= LOADADDR;/* need to load address */
			setconstaddr(op); /* check for constant address */
			}
		return;
		}
	if (amatch("sizeof")) {
		sawparen = matchc('(');
		if (type = abstdecl()) { /* sizeof abstract declarator */
			reftype(op->op_type = inttype);
			op->op_val = type->t_size;
			if (sawparen)
				needpunc(')');
			}
		else {			/* sizeof a variable */
			if (sawparen)
				putback('(');
			savegflag = genflag;
			savepflag = pregflag;
			pregflag = genflag = FALSE;
			heir14(op);
			genflag = savegflag;
			pregflag = savepflag;
			op->op_val = op->op_type->t_size;
			chtype(op, inttype);
			}
		op->op_sym = NULL;
		op->op_load = (LOADVALUE|CONSTANT);
		return;
		}
	if (matchc('(')) {		/* check for cast */
		if (type = abstdecl()) {
			needpunc(')');
			heir14(op);
			rvalp(op);
			chtype(op, type);
			return;
			}
		else
			putback('(');
		}
	heir15(op);			/* look for a primary expression */
	if (match("++")) {		/* post-increment */
		if (needlval(op))
			addto(op, 1);
		increment(op, -1);	/* recover original value */
		return;
		}
	if (match("--")) {		/* post-decrement */
		if (needlval(op))
			addto(op, -1);
		increment(op, 1);
		return;
		}
	}
heir15(op1)
register struct operand *op1;
	{
	register dot, offset;
	struct operand op2;
	struct typeinfo *isptr();

	primary(op1);
	if (matchc('(')) {
		if (op1->op_type->t_code != T_FUNC)
			error("Not a function");
		callfunction(op1->op_sym);
		deref(op1);		/* change to type func returns */
		op1->op_load = EXPRESSION; /* and mark as loaded */
		return;
		}
	for (;;) {			/* check for struct reference */
		if ((dot=matchc('.')) || match("->")) {
		    if (!dot) {
			rvalp(op1);
			op1->op_sym = NULL; /* says addr in preg */
			}
		    if (!ismember(&op2)) {
			error("Not a structure or union member");
			op1->op_load = EXPRESSION;
			}
		    else {
			offset = op2.op_sym->st_info;
			if (dot && (op1->op_sym != NULL))
				op1->op_val += offset;
			else
				addconst(offset);
			op1->op_load = (LVALUE|LOADVALUE);/*member is lval*/
			chtype(op1, op2.op_type); /* use member info */
			setconstaddr(op1); /* chk for constant address */
			unreftype(op2.op_type);
			}
		    }
		else if (matchc('[')) { /* check for array reference */
			heir1(&op2);
			doadd(op1, &op2);
			if (op1->op_load == EXPRESSION)
				op1->op_sym = NULL; /* says addr in preg */
			else			/* not an array now */
				op1->op_load &= ~LOADADDR;
			op1->op_load |= (LVALUE|LOADVALUE);
			if (isptr(op1))
				deref(op1);
			else {
				error("Can't subscript");
				op1->op_load = EXPRESSION;
				}
			needpunc(']');
			}
		else
			break;
		}
	}
/* See if next token is a structure or union member */
ismember(op)
struct operand *op;
	{
	char name[NAMESIZE];
	register struct st *p;
	struct st *findtag();

	if (!symname(name))
		return FALSE;
	else if ((p=findtag(name)) == NULL || p->st_sc == SC_TYPE)
		return FALSE;
	else {
		initop(op, p, p->st_type, LVALUE|LOADVALUE);
		return TRUE;
		}
	}
/* Evaluate a primary expression */
primary(op)
register struct operand *op;
	{
	register struct st *sym;
	register struct typeinfo *type;
	struct typeinfo *ptrto();
	struct st *addloc(), *findglb(), *findloc();
	char *chksym(), name[NAMESIZE];

	if (matchc('(')) {
		heir1(op);	/* start a new expression */
		needpunc(')');	/*	one level down */
		return;
		}
	if (chksym(name)) {	/* see if next token is legal name */
		skip();
		if (!(sym = findloc(name))) {	/* is it a local? */
		    if (sym = findglb(name)) {	/* or a global? */
			if (sym->st_info==DECL_LOC) /* declared locally? */
				sym = NULL;	/* not known here */
			else if (libflag && infunc)
				addloc(name, sym->st_type, SC_EXTERN,
					ID_VAR, 0);
			}
		    }
		if (sym) {		/* if it's defined ... */
			type = sym->st_type;
			initop(op, sym, type, EXPRESSION);
			switch (type->t_code) {
			case T_LABEL:
				errname("Illegal use of label",sym->st_name);
				break;
			case T_FUNC:	/* if not a call... */
				if (!nextc('(')) { /* make ptr-to-func */
					chtype(op, ptrto(type));
					op->op_load = LOADADDR;
					}
				break;
			case T_ARRAY:	/* chg array-of-x to ptr-to-x */
				op->op_load = LOADADDR;
				break;
			default:	/* other types are lvalue */
				op->op_load = (LVALUE|LOADVALUE);
				break;
				}
			setconstaddr(op); /* chk for constant address */
			return;
			}
		if (nextc('(')) {	/* is it a function reference? */
			addglb(name, functype, SC_EXTERN, ID_VAR, DECL_LOC);
			sym = addloc(name, functype, SC_EXTERN, ID_VAR, 0);
			initop(op, sym, functype, EXPRESSION);
			}
		else {	/* try to avoid more errors by defining */
			errname("Undefined variable",name);
			type = (nextc('[')) ? ptrto(inttype): inttype;
			sym = addloc(name, type, SC_EXTERN, ID_VAR, 0);
			initop(op, sym, type, (LOADVALUE|LVALUE));
			}
		return;
		}
	/* This should be a constant or a string */
	if (!constant(op) && !string(op)) { /* if not, it's an error */
		savepreg();
		limmed(0, PREG);
		error("Invalid expression");
		if (ch() != ';')
			junk(); 	/* skip bad stuff */
		initop(op, (struct st *) NULL, inttype, EXPRESSION);
		}
	}
/* Change array reference to address of array */
unarray(op)
register struct operand *op;
	{
	if (op->op_sym!=NULL && !isloaded(op)) /* if op is not loaded */
		op->op_load |= LOADADDR; /* must reference by address */
	op->op_load &= ~(LVALUE|LOADVALUE); /* array has no lvalue */
	}
/* Check for constant address or constant offset from stack pointer */
setconstaddr(op)
register struct operand *op;
	{
	register sc;

	if (op->op_load & LOADADDR) {	/* is op referenced by addr? */
		sc = op->op_sym->st_sc;
		if (sc == SC_GLOBAL || sc == SC_ST_GLB
		   || sc == SC_EXTERN || sc == SC_STATIC)
			op->op_load |= CONSTADDR;
		else if (sc == SC_AUTO)
			op->op_load |= CONSTOFF;
		}
	}
/* Initialize an expression operand */
initop(op, sym, type, load)
register struct operand *op;
struct st *sym;
struct typeinfo *type;
	{
#ifndef PORTABLE
	extern char *_free;
	if (&load <= _free) {
		puts("\nOut of memory");
		exit(1);
		}
#endif
	op->op_sym = sym;
	reftype(op->op_type = type);
	op->op_val = 0; 	/* offset of variable starts at zero */
	op->op_load = load;
	}
needlval(op)
struct operand *op;
	{
	if (islval(op))
		return TRUE;
	else {
		error("Must be lvalue");
		return FALSE;
		}
	}
/* end of CC4.C */

	if (match("++")) {		/* pre-increment