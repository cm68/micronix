/********************************************************/
/* */
/* Q/C Compiler Version 3.1		 */
/* (Part 6)			 */
/* */
/* Copyright (c) 1983 Quality Computer Systems	 */
/* */
/* 07/01/83			 */
/********************************************************/

#include "qstdio.h"
#include "cstddef.h"
#include "globals.h"

/*
 * This section contains code generation routines which reflect the
 * architecture of the target machine but which don't generate any assembler
 * mnemonics
 */

/*
 * Do binary operators Return: CONSTANT if constant expression FALSE	 if
 * not
 */
binop(lop, binop, rop, heirn)
    register struct operand *lop, *rop;
    register	    binop;
    int             (*heirn) ();
{
    register	    oprev, unsignlop;

    (*heirn) (rop);
    if (specialcase(lop, binop, rop))
	return FALSE;
    else if ((oprev = loadops(lop, rop)) == CONSTANT)
	return CONSTANT;
    unsignlop = isptr(lop) || lop->op_type->t_code == T_UNSIGNED;
    switch (binop) {
    case '|':
	cor();
	break;
    case '^':
	cxor();
	break;
    case '&':
	cand();
	break;
    case EQ:
	ceq();
	break;
    case NE:
	cne();
	break;
    case LE:
	if (unsignlop)
	    (oprev) ? cuge() : cule();
	else
	    (oprev) ? cge() : cle();
	break;
    case GE:
	if (unsignlop)
	    (oprev) ? cule() : cuge();
	else
	    (oprev) ? cle() : cge();
	break;
    case LT:
	if (unsignlop)
	    (oprev) ? cugt() : cult();
	else
	    (oprev) ? cgt() : clt();
	break;
    case GT:
	if (unsignlop)
	    (oprev) ? cult() : cugt();
	else
	    (oprev) ? clt() : cgt();
	break;
    case '>':
	(unsignlop) ? clsr(oprev) : casr(oprev);
	break;
    case '<':
	casl(oprev);
	break;
    case '%':
    case '/':
	if (oprev)
	    swap();
	if (unsignlop)
	    cudiv();
	else
	    cdiv();
	if (binop == '%')
	    swap();
	break;
    }
    return FALSE;
}
specialcase(lop, binop, rop)
    register struct operand *lop, *rop;
    int		    binop;
{
    switch (binop) {
    case '*':
	domult(lop, rop);
	break;
    case '-':
	dosub(lop, rop);
	break;
    case '+':
	doadd(lop, rop);
	break;
    case EQ:
	if (!isconstant(lop) && isconstant(rop)) {
	    rvalp(lop);
	    switch (rop->op_val) {
	    case -1:
		cinc();
		goto case0;
	    case 1:
		cdec();
	    case 0:
	case0:
		cnot();
		break;
	    default:
		rvals(rop);
		ceq();
		break;
	    }
	    unreftype(inttype);
	    break;
	}
    default:
	return FALSE;
    }
    return TRUE;
}
/* Add rop to lop */
doadd(lop, rop)
    register struct operand *lop, *rop;
{
    register struct typeinfo *lptrtype, *rptrtype;
    register int    n;
    if (isconstant(lop) && isconstant(rop)) {
	lop->op_val += rop->op_val;	/* constant expression */
	unreftype(inttype);
	return;
    }
    lptrtype = isptr(lop);
    rptrtype = isptr(rop);
    if (isconstant(rop)) {	/* do special cases */
	if (lptrtype)		/* must scale for pointers */
	    n = rop->op_val * basesize(lop);
	else
	    n = rop->op_val;
	if ((lop->op_load & LOADADDR)
	    && (lop->op_load & (CONSTADDR | CONSTOFF)))
	    lop->op_val += n;	/* change offset only */
	else {
	    rvalp(lop);		/* load lop if necessary */
	    addconst(n);
	}
	unreftype(rop->op_type);
	return;
    }
    if (isconstant(lop)) {
	if (rptrtype)
	    lop->op_val *= basesize(rop);
	if ((rop->op_load & LOADADDR)
	    && (rop->op_load & (CONSTADDR | CONSTOFF)));	/* offset has been
								 * changed */
	else {
	    rvalp(rop);		/* load rop if necessary */
	    addconst(lop->op_val);
	}
	chtype(lop, rop->op_type);	/* now lop is that type */
	lop->op_load = rop->op_load;	/* record loading info */
	lop->op_sym = rop->op_sym;
	unreftype(rop->op_type);
	return;
    }
    if (lptrtype) {		/* must scale for pointer addition */
	if (rptrtype)
	    error("Can't add pointers");
	rvalp(rop);
	multconst(basesize(lop));
    } else if (rptrtype) {
	rvalp(lop);
	multconst(basesize(rop));
	chtype(lop, rop->op_type);
    }
    loadops(lop, rop);
    cadd();
}
addto(op, sign)
    struct operand *op;
    int		    sign;
{
    rvalpsav(op);
    increment(op, sign);
    store(op);
}
increment(op, sign)
    struct operand *op;
    int		    sign;
{
    if (isptr(op))
	addconst(sign * basesize(op));
    else
	addconst(sign);
}
/* Subtract rop from lop */
dosub(lop, rop)
    register struct operand *lop, *rop;
{
    register struct typeinfo *lptrtype, *rptrtype;
    register	    size;
    if (isconstant(rop)) {
	rop->op_val = -rop->op_val;
	doadd(lop, rop);	/* do lop + (-rop) */
	return;
    }
    rptrtype = isptr(rop);
    lptrtype = isptr(lop);
    if (lptrtype && !rptrtype) {
	rvalp(rop);		/* scale for ptr - scalar */
	multconst(basesize(lop));
    }
    if (loadops(lop, rop))
	swap();			/* operands loaded in reverse order */
    csub();
    if (rptrtype) {
	if (!lptrtype)
	    error("Can't subtract pointer from scalar");
	else if (lptrtype != rptrtype)
	    error("Can't subtract unlike pointers");
	else {
	    /* it's ptr - ptr so compute occurrences */
	    size = basesize(lop);
	    if (size > 1) {
		swap();		/* put diff in sreg */
		limmed(size, PREG);	/* and divide by size */
		cdiv();
	    }
	    chtype(lop, inttype);	/* now it's just a count */
	}
    }
}
/* Multiply lop by rop */
domult(lop, rop)
    register struct operand *lop, *rop;
{
    if (isconstant(lop) && isconstant(rop))
	lop->op_val *= rop->op_val;	/* constant expression */
    else if (isconstant(rop)) {
	rvalp(lop);
	multconst(rop->op_val);	/* chk for special cases */
    } else if (isconstant(lop)) {
	rvalp(rop);
	multconst(lop->op_val);
	chtype(lop, rop->op_type);	/* record what we did */
	lop->op_load = EXPRESSION;
    } else {
	loadops(lop, rop);
	cmult();
	return;
    }
    unreftype(rop->op_type);
}
/* Multiply primary reg by 'size' */
multconst(size)
    int		    size;
{
    switch (size) {
    case 64:
	doublereg();
    case 32:
	doublereg();
    case 16:
	doublereg();
    case 8:
	doublereg();
    case 4:
	doublereg();
    case 2:
	doublereg();
    case 1:
	return;
    default:
	limmed(size, SREG);
	cmult();
	return;
    }
}
/* Do a function call */
callfunction(sym)
    register struct st *sym;	/* symbol table entry (or NULL) */
{
    register int    nargs;
    register struct typeinfo *type;
    struct operand  op;
    register char  *p;
    nargs = 0;
    blanks();
    if (sym == NULL ||		/* if we're calling address in preg or */
	pregflag)		/* preg contains something... */
	cpush(PREG);		/* save it */
    while (!streq(lptr, ")")) {	/* process arguments */
	if (endst())
	    break;		/* premature end of function call */
	pregflag = FALSE;	/* start new expression */
	heir2(&op);		/* get arg, but no comma ops */
	type = op.op_type;
	if (type->t_code == T_STRUCT || type->t_code == T_UNION) {
	    error("Can't pass structures or unions");
	    op.op_load &= ~LOADVALUE;	/* chg to ptr ref */
	    op.op_load |= LOADADDR;
	}
	rvalp(&op);
	if (sym == NULL)	/* calling addr on top of stk */
	    swapstk();		/* keep addr on top of stack */
	cpush(PREG);		/* push addr or arg in preg */
	++nargs;
	unreftype(type);	/* remove type table ref */
	if (!matchc(','))	/* any more arguments? */
	    break;
    }
    needpunc(')');
    if (sym) {
	p = sym->st_name;	/* special case for "_printf" */
	if ((p = index(p, 'p')) && astreq(p, "printf", 6)) {
	    limmed(nargs, PREG);/* add argument count */
	    cpush(PREG);
	    ++nargs;		/* one more arg must be cleared */
	}
	ccall(sym->st_name);	/* call named function */
    } else
	callstk();		/* else call address on top of stack */
    pregflag = TRUE;
    clearstk(2 * nargs);
    testflag = TESTED;		/* return value is tested by function */
}
/* Copy an argument into a register */
argtoreg(p, regno)
    register struct st *p;
    int		    regno;
{
    struct operand  arg;

    initop(&arg, p, p->st_type, LOADVALUE);
    pregflag = FALSE;
    rvalp(&arg);		/* copy arg into preg */
    p->st_sc = SC_REG;
    p->st_info = regno;
    arg.op_sym = p;
    store(&arg);
    unreftype(p->st_type);
}
/*
 * Load operands in the proper registers Return: CONSTANT if both operands
 * are constant TRUE	if operands are loaded in reverse of normal order
 * FALSE	otherwise
 */
loadops(lop, rop)
    register struct operand *lop, *rop;
{
    register	    oprev;
    oprev = FALSE;		/* record order operands are loaded */
    if (isconstant(lop)) {
	/* LOP is CONSTANT */
	if (isconstant(rop)) {
	    unreftype(inttype);
	    return CONSTANT;
	} else {		/* ROP not CONSTANT */
	    rvalp(rop);
	    rvals(lop);
	}
    } else if (isloaded(lop)) {
	/* LOP is already LOADED */
	if (isloaded(rop))	/* ROP is LOADED */
	    cpop(SREG);		/* retrieve lop in sreg */
	else if (isconstant(rop)) {
	    /* ROP is CONSTANT */
	    rvals(rop);		/* rop into sreg */
	    oprev = TRUE;	/* reverse order */
	} else if (isaddr(rop)) {	/* ROP is ADDRESS */
	    rvalp(rop);		/* addr of rop in preg */
	    cpop(SREG);		/* retrieve lop */
	} else {		/* ROP is VARIABLE */
	    swap();		/* move lop into sreg */
	    pregflag = FALSE;	/* preg empty */
	    rvalp(rop);		/* load rop into preg */
	}
    } else if (isaddr(lop)) {
	/* LOP ADDRESS is in PREG */
	if (isloaded(rop)) {	/* ROP is LOADED */
	    swap();		/* move rop into sreg */
	    pregflag = FALSE;	/* preg empty */
	    cpop(PREG);		/* retrieve lop addr */
	    rvalp(lop);		/* load lop in preg */
	    oprev = TRUE;	/* reverse order */
	} else if (isaddr(rop)) {	/* ROP is ADDRESS */
	    rvals(rop);		/* rop into sreg */
	    cpop(PREG);		/* get addr of lop */
	    rvalp(lop);		/* lop in preg */
	    oprev = TRUE;	/* reverse order */
	} else {		/* ROP CONSTANT or VAR */
	    rvals(lop);		/* lop indirect to sreg */
	    rvalp(rop);		/* rop into preg */
	}
    } else {
	/* LOP is a VARIABLE */
	if (isconstant(rop)) {	/* ROP is CONSTANT */
	    rvalp(lop);		/* lop in preg */
	    rvals(rop);		/* rop in sreg */
	    oprev = TRUE;	/* reverse order */
	} else if (isloaded(rop)) {	/* ROP is LOADED */
	    swap();		/* rop into sreg */
	    pregflag = FALSE;	/* preg empty */
	    rvalp(lop);		/* lop into preg */
	    oprev = TRUE;	/* reverse order */
	} else if (isaddr(rop)) {	/* ROP is ADDRESS */
	    rvals(rop);		/* rop indirect to sreg */
	    rvalp(lop);		/* lop into preg */
	    oprev = TRUE;	/* reverse order */
	} else {		/* ROP is VARIABLE */
	    rvalp(lop);		/* lop in preg */
	    swap();		/* switch to sreg */
	    pregflag = FALSE;	/* preg empty */
	    rvalp(rop);		/* rop in preg */
	}
    }
    settype(lop, rop);		/* set type of expression */
    return oprev;
}
/* Get the value of op in preg */
rvalp(op)
    register struct operand *op;
{
    register	    load_info;
    if (isloaded(op))
	return;
    if (isaddr(op)) {		/* addr of op already loaded */
	indirect(op->op_type->t_size);
	op->op_load = EXPRESSION;
	pregflag = TRUE;
	return;
    }
    savepreg();			/* save preg if it's in use */
    load_info = op->op_load;
    if (load_info & (CONSTANT | ASGCONST))	/* is it a constant? */
	limmed(op->op_val, PREG);
    else if (load_info & LOADADDR)	/* do we want the address... */
	getaddr(op);
    else if (load_info & LOADVALUE)	/* or the value of op? */
	load(op);
    op->op_load = EXPRESSION;	/* record that op is loaded */
}
/* Get the value of op in preg and */
/* save its address if it's not fixed. */
rvalpsav(op)
    register struct operand *op;
{
    if (op->op_sym == NULL)	/* if addr of op already in preg */
	cpush(PREG);		/* save it */
    else if (op->op_sym->st_sc == SC_AUTO) {
	savepreg();
	getaddr(op);		/* load address and */
	cpush(PREG);		/* save it */
    }
    rvalp(op);
}
/* Get the value of op in sreg */
rvals(op)
    register struct operand *op;
{
    if (isconstant(op))
	limmed(op->op_val, SREG);	/* load value immediate */
    else if (op->op_sym == NULL) {	/* address is in preg */
	sindirect(op->op_type->t_size);
	pregflag = FALSE;	/* preg is now free */
    }
    op->op_load = EXPRESSION;	/* record that op is loaded */
}
/* Load operand into preg */
load(op)
    register struct operand *op;
{
    register struct st *sym;

    sym = op->op_sym;
    if (peepflag == PREGSYM
	&& astreq(sym->st_name, peepsym, strlen(peepsym))
	&& op->op_val == peepoffset)
	return;			/* sym is already in preg */
    if (sym->st_sc == SC_AUTO)
	autoload(op);
    else
	getmem(op);
}
/* Store preg into operand op */
store(op)
    register struct operand *op;
{
    if (op->op_sym == NULL)	/* store at address on stack */
	putstk(op->op_type->t_size);
    else
	putmem(op);		/* store in memory */
    op->op_load = EXPRESSION;	/* keep op from being loaded */
}
/* Load specified object size indirect into preg */
indirect(size)
    int		    size;
{
    if (peepflag == ADDRSFP) {
	clearpeep();		/* clear pattern buffer */
	callib((size == 1) ? "gcf" : "gf");
    } else
	callib((size == 1) ? "gc" : "g");
}
/* Load specified object size indirect thru preg into sreg */
sindirect(size)
    int		    size;
{
    callib((size == 1) ? "gcs" : "gs");
}
/* Add constant amt to primary reg */
addconst(amt)
    int		    amt;
{
    switch (amt) {
    case 3:
	cinc();
    case 2:
	cinc();
    case 1:
	cinc();
    case 0:
	return;
    case -3:
	cdec();
    case -2:
	cdec();
    case -1:
	cdec();
	return;
    default:
	limmed(amt, SREG);
	cadd();
	return;
    }
}
/* Subtract preg from sreg */
csub()
{
    callib("s");
    testflag = REGH;		/* remember that H is in A */
}
/* Multiply preg by sreg */
cmult()
{
    callib("mult");
}
/* Divide (signed) sreg by preg */
/* (quotient in preg, remainder in sreg) */
cdiv()
{
    callib("div");
}
/* Divide (unsigned) sreg by preg */
cudiv()
{
    callib("udiv");
}
/* Inclusive 'or' preg and sreg */
cor()
{
    callib("o");
    testflag = REGH;		/* remember that H is in A */
}
/* Exclusive 'or' preg and sreg */
cxor()
{
    callib("x");
    testflag = REGH;		/* remember that H is in A */
}
/* 'and' preg and sreg */
cand()
{
    callib("a");
    testflag = REGH;		/* remember that H is in A */
}
/* Arithmetic right shift sreg number of times in preg */
casr(oprev)
{
    callib(oprev ? "asr1" : "asr");
}
/* Arithmetic left shift sreg number of times in preg */
casl(oprev)
{
    callib(oprev ? "asl1" : "asl");
}
/* Logical right shift sreg number of times in preg */
clsr(oprev)
{
    callib(oprev ? "lsr1" : "lsr");
}
/* two's complement of preg */
cneg()
{
    callib("neg");
    testflag = REGL;		/* remember that L is in A */
}
/* one's complement of preg */
ccom()
{
    callib("com");
    testflag = REGL;		/* remember that L is in A */
}
/* Logical not of preg. Set HL and Z flag to indicate TRUE/FALSE */
cnot()
{
    callib("n");
    testdone = TRUE;
}
/*
 * The following are the conditional operators. They compare sreg to preg and
 * set preg = 1 if the condition is true and 0 if not. The Z flag is reset if
 * TRUE and set if FALSE.
 */
/* Test for equal */
ceq()
{
    callib("e");
    testdone = TRUE;
}
/* Test for not equal */
cne()
{
    callib("ne");
    testdone = TRUE;
}
/* Test for less than (signed) */
clt()
{
    callib("lt");
    testdone = TRUE;
}
/* Test for less than or equal (signed) */
cle()
{
    callib("le");
    testdone = TRUE;
}
/* Test for greater than (signed) */
cgt()
{
    callib("gt");
    testdone = TRUE;
}
/* Test for greater than or equal (signed) */
cge()
{
    callib("ge");
    testdone = TRUE;
}
/* Test for less than (unsigned) */
cult()
{
    callib("ult");
    testdone = TRUE;
}
/* Test for less than or equal (unsigned) */
cule()
{
    callib("ule");
    testdone = TRUE;
}
/* Test for greater than (unsigned) */
cugt()
{
    callib("ugt");
    testdone = TRUE;
}
/* Test for greater than or equal (unsigned) */
cuge()
{
    callib("uge");
    testdone = TRUE;
}
/* Save preg if it's in use */
savepreg()
{
    if (pregflag)
	cpush(PREG);
    else
	pregflag = TRUE;
}
/* Do an immediate load of n into reg */
limmed(n, reg)
    int		    n     , reg;
{
    if (reg == PREG)
	pimmed();
    else
	simmed();
    outdec(n);
    nl();
}
/* Print a name so it won't upset the assembler */
outname(name)
    char           *name;
{
    register	    c;
    register char  *s;

    s = name;
    while (c = *s++)
	outbyte((c == '_') ? ASMULINE : c);
    if (*name != ASMRTS)	/* don't change library function names */
	outbyte(ASMRTS);
}
/* Print specified number as label */
printlabel(label)
    int		    label;
{
    outbyte(ASMRTS);
    outdec(label);
}
/* Generate a compiler label in the peephole buffer */
peeplabel(label)
{
    static char	    s[7] = { ASMRTS } ;

    itob(label, s + 1, 10);
    loadpeep(s);
    loadpeep("\n");
}
/* Load pattern into peephole optimization buffer */
loadpeep(s)
    char           *s;
{
    strcat(peepbuf, s);
}
/* Dump the contents of the peephole buffer */
dumpeep()
{
    peepflag = FALSE;
    outstr(peepbuf);
    clearpeep();
}
/* Clear the peephole pattern buffer */
clearpeep()
{
    peepflag = FALSE;
    *peepbuf = '\0';
}
/* Generate trace message */
gentrace(inout, funcname)
    char	    inout , *funcname;
{
    register int    lab1;
    jump(lab1 = getlabel());	/* jump over trace message */
    linelabel(getlabel());
    defbyte();			/* build trace message */
    outdec(inout);		/* enter (>) or exit (<) */
    do {			/* print function name */
	outbyte(',');
	outdec(*funcname);
    } while (*funcname++);
    linelabel(lab1);		/* set up call to prt message */
    cpush(PREG);		/* save preg */
    pimmed();			/* load address of message */
    printlabel(lab1 + 1);
    nl();
    cpush(PREG);
    ccall("puts");		/* print message */
    cpop(PREG);			/* clear arg off stack */
    cpop(PREG);			/* restore preg */
}
/* end of CC6.C */
