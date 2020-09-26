/********************************************************/
/*							*/
/*		Q/C Compiler Version 3.2		*/
/*		    (Part 7 for Z80)			*/
/*							*/
/*     Copyright (c) 1984 Quality Computer Systems	*/
/*							*/
/*			01/28/84			*/
/********************************************************/

#include "qstdio.h"
#include "cstddef.h"
#include "cglbdecl.c"

/* This section contains all code generation routines
 * which generate mnemonics for specific assemblers
 */

/* Names of Z80 registers used */
static char *z80reg[] = {"HL", "DE", "BC", "IX"};

/* Print assembler info before any code is generated */
header()
	{
	outstr(";Compiled by Q/C ");
	outstr(version);
	nl();
	ol(".Z80");
	}
/* Generate code to be done before main() */
shell(argcnt)
	{
	static char *func;
	char hex[5];

	codeseg();
	glblabel("_main", SC_GLOBAL);	/* establish entry point */
	if (romflag) {
		ot("LD\tSP,0"); 	/* stack addr from -S switch */
		outstr(itob(spaddr, hex, 16));
		outstr("H\n");
		}
	else {
		ol("LD\tSP,(6)");	/* start stack below CP/M BDOS */
		if (redirect)		/* full redirection requested */
			func = "_rshell";
		else if (argcnt)	/* parse CP/M command line */
			func = "_shell";
		else {
			ccall("main");
			func = "_exit";
			}
		ccall(func);
		declext(func);
		}
	}
/* Print any assembler stuff needed after all code */
trailer()
	{
	static int n;
	static struct st *p;
	static struct typeinfo *t;
	char *regname();
	static char extrns[][55] = {
		"EXTRN\t?gcf,?gc,?sxt,?gcs,?g,?gf,?gs,?p,?o,?x,?a,?e,?ne",
		"EXTRN\t?gt,?lt,?le,?ge,?ugt,?ult,?ule,?uge,?asr,?asr1",
		"EXTRN\t?asl,?asl1,?lsr,?lsr1,?s,?neg,?com,?n,?mult,?div",
		"EXTRN\t?udiv,?sw,?enr,?en,?ensr,?ens,?exr,?exrs,?exs"
		};
	if (libflag)			/* building a library? */
		ol("END");
	else {				/* declare externals */
		for (p = STARTGLB; p < glbptr; ++p) {
			if (p->st_sc == SC_EXTERN)
				declext(p->st_name);
			t = p->st_type;
			if (t->t_code == T_STRUCT && t->t_size == 0
			   && p->st_sc == SC_TYPE)
				tagerr(p->st_name);
			}
		for (n = 0; n < MAXREG; ) /* declare registers */
			declext(regname(++n));
		ol(extrns[0]);
		ol(extrns[1]);
		ol(extrns[2]);
		ol(extrns[3]);
		ot("END");
		if (mainflag) {
			outbyte('\t');	/* define entry point */
			outname("_main");
			}
		nl();
		}
	}
/* Comment line for the assembler */
comment()
	{
	newline();			/* start new line if necessary */
	outbyte(';');
	}
/* "#asm" preprocessor command */
doasm()
	{
	register c;
	if (match("8080")) {	/* look for 8080 compatibility request */
		newline();
		ol(".8080");
		}
	cmode = FALSE;		/* this is not C */
	newline();		/* start a new line if necessary */
	for (;;) {
		inline();	/* get next line */
		if (ch() == ';')
			continue; /* discard comment line */
		if (eof)
			break;
		if (ch() == '#') {
			junk(); /* skip to next alphanumeric */
			if (amatch("endasm"))
				break;
			else if (amatch("include")) {
				doinclude();
				continue;
				}
			}
		while (c = gch()) {
			if (c == ';')
				break; /* strip comments */
			outbyte(c);
			}
		nl();
		}
	kill_line();		/* clear #endasm line */
	ol(".Z80");
	cmode = TRUE;		/* back in C mode */
	if (trace.is_on)	/* if TRACE was requested */
		retvalue = TRUE;/* force compiler to set Z flag */
	}
/* Assign rop to lop */
doasgn(lop, rop, depth)
register struct operand *lop, *rop;
int depth;
	{
	register unsigned value;
	register struct st *sym;
	register offset;
	int size;

	sym = lop->op_sym;
	if (sym != NULL) {			/* not loaded? */
		offset = sym->st_info + lop->op_val;
		if ((sym->st_sc == SC_AUTO	/* on stack & */
		     && offset >= 127)		/* can't use xreg */
		   || lop->op_load&LOADADDR) {	/* or is address? */
			savepreg();
			getaddr(lop);
			}
		}
	heir2(rop);		/* same level for right-to-left eval */
	if (depth == 1		/* if assignments are not nested */
	   &&  isconstant(rop)) { /* and we're assigning a constant */
		size = lop->op_type->t_size;
		if (sym == NULL) { /* address of lop is in preg */
			value = lop->op_val = rop->op_val;
			ldmemi(value & 0xFF);
			if (size == 2) {
				cinc();
				ldmemi(value >> 8);
				}
			lop->op_load = (ASGCONST|LOADVALUE);
			pregflag = FALSE; /* preg no longer in use */
			return;
			}
		else if (sym->st_sc == SC_AUTO && offset < 127) {
			ot("LD\t");
			xreg(offset);
			outbyte(',');
			value = lop->op_val = rop->op_val;
			outdec(value & 0xFF);
			nl();
			if (size == 2) {
				ot("LD\t");
				xreg(offset+1);
				outbyte(',');
				outdec(value >> 8);
				nl();
				}
			lop->op_load = (ASGCONST|LOADVALUE);
			return;
			}
		}
	rvalp(rop);			/* normal assign */
	store(lop);
	}
/* Load an auto variable */
autoload(op)
struct operand *op;
	{
	register struct st *sym;
	register offset, size;

	size = op->op_type->t_size;
	sym = op->op_sym;
	offset = sym->st_info + op->op_val;
	if (offset > 0 && offset < 127) {
		if (size == 1) {
			ot("LD\tA,");
			xreg(offset);
			prtname(sym);	/* print name as comment */
			callib("sxt");
			}
		else {
			ot("LD\tL,");
			xreg(offset);
			prtname(sym);
			ot("LD\tH,");
			xreg(offset+1);
			nl();
			}
		}
	else {
		getaddr(op);
		indirect(size); 	/* load indirect */
		}
	}
/* Load the address of op in preg */
getaddr(op)
register struct operand *op;
	{
	register struct st *sym;
	register offset;

	sym = op->op_sym;
	if (sym == NULL)
		return; 		/* address already loaded */
	offset = op->op_val;		/* constant offset from sym */
	if (sym->st_sc == SC_AUTO) {
		offset += sym->st_info; /* offset of sym from stk frame */
		if (offset == 0) {
			dumpeep(); /* clear any pending pattern */
			loadpeep("\tLD\tL,C\n\tLD\tH,B\n");
			peepflag = ADDRSFP;
			}
		else {
			limmed(offset, PREG);
			ot("ADD\tHL,BC");
			prtname(sym);
			}
		}
	else {				/* address of global or static */
		pimmed();
		prtsymname(sym, offset);
		prtname(sym);
		}
	op->op_sym = NULL;		/* record that preg has address */
	op->op_load &= ~LOADADDR;
	pregflag = TRUE;
	}
/* Load global/static/register variable into preg */
getmem(op)
register struct operand *op;
	{
	register offset;
	register struct st *sym;

	offset = op->op_val;
	sym = op->op_sym;
	if (op->op_type->t_size == 1) { /* load 1 byte */
		ot("LD\tA,(");
		prtsymname(sym, offset);
		outbyte(')');
		prtname(sym);
		callib("sxt");
		}
	else {			/* all other types are 2 bytes in V3 */
		ot("LD\tHL,(");
		prtsymname(sym, offset);
		outbyte(')');
		prtname(sym);
		}
	}
/* Store specified size from preg to address on top of stack */
putstk(size)
int size;
	{
	cpop(SREG);
	if (size == 1) {
		ol("LD\tA,L");
		ol("LD\t(DE),A");
		testflag = REGL;
		}
	else {		/* all other types in Q/C V3 are 2 bytes */
		callib("p");
		testflag = REGH;
		}
	}
/* Store preg in the specified memory location */
putmem(op)
register struct operand *op;
	{
	register struct st *sym;
	register offset, size;

	offset = op->op_val;		/* constant offset from sym */
	size = op->op_type->t_size;
	sym = op->op_sym;
	if (sym->st_sc == SC_AUTO) {
		ot("LD\t");
		offset += sym->st_info; /* offset of sym from stk frame */
		xreg(offset);
		outstr(",L");
		prtname(sym);
		if (size == 2) {
			ot("LD\t");
			xreg(offset+1);
			outstr(",H\n");
			}
		}
	else {
		if (size == 1) {
			ol("LD\tA,L");
			ot("LD\t(");
			prtsymname(sym, offset);
			outstr("),A");
			testflag = REGL;
			}
		else {
			ot("LD\t(");
			prtsymname(sym, offset);
			outstr("),HL");
			}
		prtname(sym);
		}
	peepflag = PREGSYM;	/* remember the variable in preg */
	strcpy(peepsym, sym->st_name);
	peepoffset = op->op_val;
	peeplen = size;
	}
/* Print external or internal symbol name */
prtsymname(sym, offset)
register struct st *sym;
int offset;
	{
	char *regname();

	switch (sym->st_sc) {
	case SC_GLOBAL:
	case SC_EXTERN:
	case SC_ST_GLB:
		outname(sym->st_name);
		break;
	case SC_REG:
		outname(regname(sym->st_info));
		break;
	case SC_STATIC:
		printlabel(sym->st_info);
		break;
		}
	if (offset) {
		if (offset > 0)
			outbyte('+');
		outdec(offset);
		}
	}
/* Print symbol name as comment */
prtname(sym)
struct st *sym;
	{
	if (fullist) {
		ot(";");	/* prt extra .asm info */
		outstr(sym->st_name);
		}
	nl();
	}
/* Print index register plus offset as address */
xreg(offset)
int offset;
	{
	outstr("(IX+");
	outdec(offset);
	outstr(")");
	}
/* Return the name of register n */
char *
regname(n)
int n;
	{
	static char name[4] = {'r', ASMRTS, 'n', '\0'};
	name[2] = n + '0';
	return name;
	}
/* Swap preg and sreg */
swap_reg()
	{
	ol("EX\tDE,HL");
	}
/* Load immediate into preg */
pimmed()
	{ot("LD\tHL,");}
/* Load immediate into sreg */
simmed()
	{ot("LD\tDE,");}
/* Load immediate into memory byte specified by preg */
ldmemi(value)
	{
	ot("LD\t(HL),");
	outdec(value);
	nl();
	}
set1()
	{
	ol("LD\tHL,1");
	}
/* Push reg onto stack */
cpush(reg)
int reg;
	{
	if (peepflag == ADDRSFP) {	/* is addr stk frame ptr? */
		clearpeep();
		ol("PUSH\tBC"); 	/* replacement pattern */
		peepflag = ADDRSFP;
		}
	else {
		ot("PUSH");
		ol(z80reg[reg - PREG]);
		}
	}
/* Pop top of the stack into reg */
cpop(reg)
int reg;
	{
	ot("POP");
	ol(z80reg[reg - PREG]);
	}
/* Restore stack frame at end of function */
restsfp()
	{
	cpop(SFP);
	cpush(SFP);
	cpop(XREG);
	}
/* Swap preg and top of the stack */
swapstk()
	{
	ol("EX\t(SP),HL");
	}
/* Call the specified library routine */
callib(sname)
char *sname;
	{
	static char libname[7] = {ASMRTS};
	strcpy(libname+1, sname);
	ot("CALL");
	ol(libname);
	if (libflag && !findloc(libname)) /* record this use */
		addloc(libname, functype, SC_EXTERN, ID_VAR, 0);
	}
/* Call the specified subroutine name */
ccall(sname)
char *sname;
	{
	ot("CALL\t");
	outname(sname);
	nl();
	}
/* Perform subroutine call to value on top of stack */
callstk()
	{
	pimmed();	/* load address for return */
	outstr("$+5");
	nl();
	swapstk();	/* call addr in preg, ret addr on stk */
	ol("JP\t(HL)"); /* jump to call address */
	}
/* Jump to specified internal label number */
jump(label)
int label;
	{
	switch (peepflag) {	/* check for peephole optimization */
	case JUMP:		/* last line also JMP, do nothing */
		break;
	case JUMPTRUE:		/* 2nd part of	JNZ/JZ	jcondlabel */
	case JUMPFALSE: 	/*		JMP	jumplabel */
				/* jcondlabel:			  */
		if (jumplabel != 0)	/* if last line also a JMP */
			break;		/* skip this one */
		loadpeep("\tJP\t");	/* save this part of pattern */
		peeplabel(jumplabel = label);
		break;
	default:
		dumpeep();	/* dump some other pending pattern */
	case FALSE:
		ot("JP\t");
		printlabel(label);
		nl();
		peepflag = JUMP; /* remember this is start of pattern: */
		break;		/*	JMP	label1		*/
		}		/*	JMP	label2		*/
	}
/* Remember that this is start of a pattern consisting of
 *		JP Z/NZ jcondlabel
 *		JP	jumplabel
 * jcondlabel:
 */
jumpcond(type, label)
int type, label;
	{
	dumpeep();		/* dump any pending pattern */
	if(!testdone)		/* did we just test (e.g. $eq)? */
		dotest();	/* if not, generate a test */
	if (type == FALSE) {
		peepflag = JUMPFALSE;
		loadpeep("\tJP\tZ,");
		}
	else {
		peepflag = JUMPTRUE;
		loadpeep("\tJP\tNZ,");
		}
	peeplabel(jcondlabel = label);
	jumplabel = 0;	/* will be set if 2nd part of pattern found */
	}
/* Generate a test of the preg */
dotest()
	{
	switch (testflag) {	/* see what we need to do test */
	case REGL:
		ol("OR\tH");	/* L already in A */
		return;
	default:		/* need full test */
		ol("LD\tA,H");
	case REGH:		/* H already in A */
		ol("OR\tL");
	case TESTED:		/* zero/non-zero test already done */
		return;
		}
	}
/* Print a line label */
linelabel(label)
int label;
	{
	char *jump;
	if ((peepflag == JUMPTRUE || peepflag == JUMPFALSE) /* 1st part? */
	     && jumplabel != 0		/* 2nd part of pattern found? */
	     && label == jcondlabel) {	/* now check for final part */
		jump = (peepflag == JUMPTRUE) ? "JP\tZ,": "JP\tNZ,";
		clearpeep();		/* clear pattern buffer */
		ot(jump);
		printlabel(jumplabel);
		nl();
		return;
		}
	newline();			/* start new line if necessary */
	printlabel(label);
	outbyte(':');
	}
/* Print a global label */
glblabel(name, sc)
char *name;
int sc;
	{
	if (sc == SC_GLOBAL) {
		ot("PUBLIC\t");
		outname(name);
		nl();
		}
	outname(name);
	outbyte(':');
	}
/* Data segment */
dataseg()
	{
	if (segtype != DATA) {
		newline();	/* start new line if necessary */
		ol("DSEG");
		segtype = DATA;
		}
	}
/* Code segment */
codeseg()
	{
	if (segtype != CODE) {
		newline();
		ol("CSEG");
		segtype = CODE;
		}
	}
/* Declare external */
declext(name)
char *name;
	{
	ot("EXTRN\t");
	outname(name);
	nl();
	}
/* Default initialization of a global or static variable */
definit(amt)
register amt;
	{
	if (amt <= 0)
		return 0;
	newline();		/* start new line */
	switch (amt) {
	default:
		ot("REPT\t");
		outdec(amt);
		nl();
	case 1: defbyte();
		goto value;
	case 2: defword();
	value:	outstr("0\n");
		if (amt > 2)
			ol("ENDM");
		}
	return amt;
	}
/* pseudo-op to define a byte */
defbyte()
	{ot("DEFB\t");}
/* pseudo-op to define storage */
defstorage()
	{ot("DEFS\t");}
/* pseudo-op to define a word */
defword()
	{ot("DEFW\t");}
/* clear amount requested off the stack */
clearstk(amt)
int amt;
	{
	switch (amt) {
	case 8: cpop(WREG);	/* special case for 8 bytes or less */
	case 6: cpop(WREG);
	case 4: cpop(WREG);
	case 2: cpop(WREG);
	case 0: return;
	default:
		swap_reg();	/* save possible return value */
		limmed(amt, PREG);
		ol("ADD\tHL,SP");
		ol("LD\tSP,HL");
		swap_reg();	/* restore return value */
		return;
		}
	}
/* Double the preg */
doublereg()
	{
	ol("ADD\tHL,HL");
	}
/* Add preg and sreg */
cadd()
	{
	ol("ADD\tHL,DE");
	}
/* Increment preg */
cinc()
	{
	ol("INC\tHL");
	}
/* Decrement preg */
cdec()
	{
	ol("DEC\tHL");
	}
/* Return from subroutine */
cret()
	{
	ol("RET");
	}

#ifndef PORTABLE

char *
findmac()
	{
#asm
	PUSH	BC		;save calling routine's stack frame
	LD	HL,4		;get name off stack
	ADD	HL,SP
	LD	C,(HL)
	INC	HL
	LD	B,(HL)
	LD	HL,(pmacsym?)	;hold value of pmacsym
	EX	DE,HL
	LD	HL,(macpool?)	;initialize ptr = macpool
?fmc1:	LD	A,D		;ptr > pmacsym?
	CP	H
	JR	NZ,?fmc2
	LD	A,E
	CP	L
?fmc2:	JR	NC,?fmc9	;no we did't find it
	PUSH	DE		;save pmacsym
	PUSH	BC		;name is 1st arg to astreq
	PUSH	HL		;ptr is 2nd arg
	LD	HL,8		;since HL is saved,
	PUSH	HL		; use it for NAMEMAX
	CALL	astreq?
	POP	HL		;clear args off stack
	POP	HL		;which also restores their values
	POP	BC
	JR	Z,?fmc3 	;didn't match
	POP	DE		;did match, clear final arg
	POP	BC		;restore stk frame
	RET
?fmc3:	LD	DE,-10
	ADD	HL,DE		;ptr -= MACSIZE
	POP	DE		;restore pmacsym
	JR	?fmc1		;try again
?fmc9:	LD	HL,0		;no match
	XOR	A		;set Z flag to indicate FALSE
	POP	BC
#endasm
	}
struct st *
findvar()
	{
#asm
	PUSH	BC		;save calling routine's stk frame ptr
	LD	HL,4		;get idset off stack
	ADD	HL,SP
	LD	A,(HL)
	PUSH	AF
	INC	HL
	INC	HL
	LD	C,(HL)		;get name
	INC	HL
	LD	B,(HL)
	INC	HL
	LD	E,(HL)		;last symbol table entry
	INC	HL
	LD	D,(HL)
	INC	HL
	LD	A,(HL)		;first symbol table entry
	INC	HL
	LD	H,(HL)
	LD	L,A
?fv1:	LD	A,H		;p < end?
	CP	D
	JR	NZ,?fv2
	LD	A,L
	CP	E
?fv2:	JR	NC,?fv9 	;no we did't find it
	PUSH	DE		;save end
	PUSH	BC		;sname is 1st arg to astreq
	LD	DE,5		;since DE is saved, use it to
	ADD	HL,DE		; compute p->st_name
	PUSH	HL		; which is 2nd arg
	LD	DE,8		;NAMEMAX is 3rd arg
	PUSH	DE
	CALL	astreq?
	POP	DE		;clear args off the stack
	POP	HL
	POP	BC
	LD	DE,9		;move to p->st_idset
	ADD	HL,DE
	POP	DE		;restore end
	JR	Z,?fv3		;if it didn't match, try again
	POP	AF		;retrieve idset
	CP	(HL)		; if it matches also, we've found it
	JR	Z,?fv8
	PUSH	AF		;save idset for next time
?fv3:	INC	HL		;move p to next symbol table entry
	JR	?fv1		;loop
?fv8:	LD	DE,-14		;retrieve ptr to
	ADD	HL,DE		; symbol table entry
	OR	H		;set Z flag to indicate TRUE
	POP	BC		;restore calling routine stk frame ptr
	RET
?fv9:	POP	AF		;matched, finish clearing stack
	LD	HL,0		;no match
	XOR	A		;set Z flag to indicate FALSE
	POP	BC		;restore stk frame ptr
#endasm
	}
chks()
	{
#asm
	PUSH	BC	;save calling routine stk frame ptr
	LD	HL,4	;get arg off stack
	ADD	HL,SP
	LD	E,(HL)
	INC	HL
	LD	D,(HL)	;string s
	LD	HL,(lptr?) ;parsing buffer
?chk1:	LD	A,(DE)	;current character in s
	OR	A	;end of string s?
	JR	Z,?asan ;parsing buffer matches to end of s
	CP	(HL)	;compare s to parsing buffer
	JR	NZ,?stne ;mismatch
	INC	DE	;next character in s
	INC	HL	;	and parsing buffer
	JR	?chk1	;loop
#endasm
	}
streq()
	{
#asm
	PUSH	BC	;save calling routine stk frame ptr
	LD	HL,4	;get args off stack
	ADD	HL,SP
	LD	E,(HL)	;s1
	INC	HL
	LD	D,(HL)
	INC	HL
	LD	A,(HL)	;s2
	INC	HL
	LD	H,(HL)
	LD	L,A
	LD	C,0	;init match length counter
?stlp:	LD	A,(DE)	;current char of s2
	OR	A	;is this end-of-string?
	JR	Z,?steq ;they match to end-to-string
	CP	(HL)	;compare to current char of s1
	JR	NZ,?stne ;they don't match here
	INC	DE	;get to next char of s2
	INC	HL	;same for s1
	INC	C	;count one more char matched
	JR	?stlp	;loop
#endasm
	}
astreq()
	{
#asm
	PUSH	BC	;save calling routine stk frame ptr
	LD	HL,4	;get args off stack
	ADD	HL,SP
	LD	B,(HL)	;len
	INC	HL
	INC	HL
	LD	E,(HL)	;s2
	INC	HL
	LD	D,(HL)
	INC	HL
	LD	A,(HL)	;s1
	INC	HL
	LD	H,(HL)
	LD	L,A
	LD	C,0	;init match length counter (max = 255)
?aslp:	LD	A,(DE)	;current char of s2
	OR	A	;is this end-of-string (0)?
	JR	Z,?asan ;they match to end-to-string
	CP	(HL)	;compare to current char of s1
	JR	NZ,?stne ;they don't match here
	INC	DE	;get to next char of s2
	INC	HL	;same for s1
	INC	C	;count one more char matched
	DJNZ	?aslp	;loop if more to match
?asan:	LD	A,(HL)	;is next char in s1 alphanumeric?
	CP	95	;'_'
	JR	Z,?stne
	CP	48	;'1'
	JR	C,?steq ;< 1
	CP	58	;'9' + 1
	JR	C,?stne ;numeric
	OR	32	;convert upper case to lower
	CP	97	;'a'
	JR	C,?steq
	CP	123	;'z' + 1
	JR	C,?stne ;lower case alpha
?steq:	LD	L,C	;set return value to matched length
	LD	H,0
	OR	1	;set Z flag to indicate TRUE
	POP	BC	;restore calling routine stk frame ptr
	RET
?stne:	LD	HL,0	;set FALSE
	XOR	A	;set Z flag to indicate FALSE
	POP	BC
#endasm
	}
isletnum()
	{
#asm
	LD	HL,2
	ADD	HL,SP
	LD	A,(HL)
	CP	48	;'1'
	JR	C,?lett
	CP	58	;'9'+1
	JR	NC,?lett
	OR	H	;set Z flag
#endasm
	}
isletter()
	{
#asm
	LD	HL,2
	ADD	HL,SP
	LD	A,(HL)
?lett:	CP	95	;'_'
	JR	Z,?islet
	OR	32	;upper to lower case
	CP	97	;'a'
	JR	C,?notlet
	CP	123	;'z'+1
	JR	C,?islet
?notlet: XOR	A	;set Z flag
	LD	H,A	;return FALSE
	LD	L,A
	RET
?islet: OR     H
#endasm
	}
/* Skip over token in parse line */
skip()
	{
#asm
	LD	HL,(lptr?)
?skip1: LD	E,(HL)	;next character in parse line
	PUSH	DE
	CALL	isletnum?
	POP	DE
	RET	Z	;end of alphanumerics
	LD	HL,(lptr?)
	INC	HL
	LD	(lptr?),HL
	JR	?skip1
#endasm
	}
#endif
/* end of CC7.C */
abel:
 */
jumpcond(type, labe