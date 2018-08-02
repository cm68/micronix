/********************************************************
*							*
*		Q/C Compiler Version 3.1		*
*			(Part 3)			*
*							*
*	Copyright (c) 1983 Quality Computer Systems	*
*							*
*			07/08/83			*
********************************************************/

#include "qstdio.h"
#include "cstddef.h"
#include "globals.h"

/* This section parses all the C statements */

#define LOAD	TRUE
#define NOLOAD	FALSE

/* Compound statement	 */
/* Do all statements between {} */
compound()
{
    ++ncmp;			/* start new level */
    while (!matchc('}')) {
	if (eof)
	    return;
	else
	    statement();
    }
    --ncmp;			/* close this level */
}
/* Statement parser */
statement()
{
    if (ch() == 0 && eof)
	return;
    chklabel();			/* check for a statement label */
    if (matchc('{'))
	compound();
    else if (amatch("if"))
	doif();
    else if (amatch("while"))
	dowhile();
    else if (amatch("for"))
	dofor();
    else if (amatch("return"))
	doreturn();
    else if (amatch("break")) {
	dobreak();
	ns();
    } else if (amatch("continue")) {
	docont();
	ns();
    } else if (amatch("switch"))
	doswitch();
    else if (amatch("do")) {
	dodowhile();
	ns();
    } else if (amatch("else"))
	error("else not matched with if");
    else if (amatch("case") || amatch("default")) {
	error("No active switch statement");
	matchc(':');
    } else if (amatch("goto")) {
	dogoto();
	ns();
    } else if (matchc(';'));	/* null statement */
    else {
	expression(NOLOAD);
	ns();
    }
}
/* Check for a statement label */
chklabel()
{
    register char  *ptr;
    char	    labname[NAMESIZE];
    if (!(ptr = chksym(labname)) || chks("default"))
	return 0;		/* was "default" or not a valid name */
    while (isspace(*ptr))
	++ptr;
    if (*ptr != ':')		/* is it a label? */
	return FALSE;
    if (chkeyword())		/* trying to use keyword as label? */
	keyerr(labname);
    linelabel(addlabel(labname, SC_AUTO));
    lptr = ptr + 1;		/* move parse line ptr past label */
    chklabel();			/* check for additional labels */
    return TRUE;
}
/* Called whenever syntax requires a semicolon */
ns()
{
    if (!matchc(';'))
	error("Missing semicolon");
}
/* "if" statement		 */
doif()
{
    register	    lab;
    lab = getlabel();		/* get false label and */
    getlabel();			/* reserve a label to skip "else" */
    testexp();			/* evaluate expression */
    jumpcond(FALSE, lab);	/* & branch false */
    statement();		/* if true, do a statement */
    if (!amatch("else")) {	/* if...else? */
	linelabel(lab);		/* no, just print false label */
	return;
    }
    /* an "if...else" statement */
    jump(lab + 1);		/* jump around false code */
    linelabel(lab);		/* print false label */
    statement();		/* and do "else" clause */
    linelabel(lab + 1);		/* print label to skip "else" */
}
/* "while" statement	 */
dowhile()
{
    struct swq	    q, *delswq();
    q.loop = getlabel();	/* looping label */
    q.exit = getlabel();	/* exit label for "break" */
    addswq(&q);
    linelabel(q.loop);
    testexp();			/* evaluate condition */
    jumpcond(FALSE, q.exit);	/* & exit if false */
    statement();		/* else, do loop body */
    jump(q.loop);		/* loop to condition test */
    linelabel(q.exit);
    delswq();
}
/* "for" statement 	 */
dofor()
{
    struct swq	    q, *delswq();
    register	    lab1, lab2;

    needpunc('(');
    q.exit = getlabel();	/* exit label for "break" */
    if (!matchc(';')) {		/* check for null initialization */
	expression(NOLOAD);	/* do initialization */
	ns();
    }
    if (matchc(';'))		/* check for null condition */
	lab1 = 0;
    else {
	linelabel(lab1 = getlabel());
	expression(LOAD);	/* check condition */
	ns();
    }
    if (matchc(')'))		/* check for null increment */
	if (lab1) {		/* see if there was an cond expr */
	    jumpcond(FALSE, q.exit);
	    q.loop = lab1;	/* if so, loop back there */
	} else
	    linelabel(q.loop = getlabel());
    else {			/* there is an increment expression */
	lab2 = getlabel();
	if (lab1) {		/* is there a condition? */
	    jumpcond(TRUE, lab2);
	    jump(q.exit);
	} else
	    jump(lab2);
	linelabel(q.loop = getlabel());	/* loop to here */
	expression(NOLOAD);	/* evaluate increment expression */
	needpunc(')');
	if (lab1)
	    jump(lab1);		/* there was a cond - check it */
	linelabel(lab2);
    }
    addswq(&q);
    statement();		/* do body of for loop */
    jump(q.loop);
    linelabel(q.exit);		/* exit label */
    delswq();
}
/* "do while" statement	 */
dodowhile()
{
    struct swq	    q, *delswq();
    int		    loop_lab;

    q.loop = getlabel();	/* "continue" label */
    q.exit = getlabel();	/* exit label for "break" */
    addswq(&q);
    linelabel(loop_lab = getlabel());
    statement();		/* do loop body */
    if (!match("while"))
	error("Missing while in dowhile");
    linelabel(q.loop);
    testexp();			/* evaluate condition */
    jumpcond(TRUE, loop_lab);	/* & loop if true */
    linelabel(q.exit);
    delswq();
}
/* "return" statement	 */
doreturn()
{
    if (!endst()) {		/* if not end of statement... */
	expression(LOAD);	/* eval return expression */
	retvalue = TRUE;
    }
    ns();
    if (!nextc('}') || ncmp > 1) {	/* not end of function */
	if (!retlab)		/* is there a return label? */
	    retlab = getlabel();
	jump(retlab);
    }
}
/* "break" statement	 */
dobreak()
{
    struct swq     *p, *getswq();

    if (p = getswq())		/* any switches or loops open? */
	jump(p->exit);
    else
	error("No active switches or loops");
}
/* "continue" statement	 */
docont()
{
    register struct swq *p;
    struct swq     *getswq();

    if ((p = getswq()) && p->loop) {	/* any loops open? */
	jump(p->loop);
	return;
    }
    error("No active loop statement");
}
/* "switch" statement	 */
doswitch()
{
    struct swq	    q;
    register	    ncases;
    register struct case_table *p;
    struct swq *pswq;
    auto	    skiplab, defaultlab;
    struct swq     *delswq(), *getswq();
    struct case_table *delcase();

    testexp();			/* evaluate expression in parentheses */
    jump(skiplab = getlabel());	/* skip body of switch */
    q.exit = getlabel();	/* exit label for "break" */
    /* if we're in a loop, copy loop label for continue */
    q.loop = (pswq = getswq()) ? pswq->loop : 0;
    addswq(&q);
    defaultlab = ncases = 0;	/* no default or cases yet */
    if (matchc('{')) {		/* is body a compound statement? */
	++ncmp;			/* bump compound statement counter */
	while (!matchc('}')) {
	    if (eof)
		return;
	    ncases += docase(&defaultlab);
	}
	--ncmp;			/* close this level */
    } else			/* switch body is a simple statement */
	ncases += docase(&defaultlab);
    jump(q.exit);		/* skip over lib call & its args */
    linelabel(skiplab);
    callib("sw");		/* call library switch routine */
    defword();			/* build arg list for lib routine */
    outdec(ncases);		/* tell it number of cases */
    nl();
    while (ncases--) {		/* arg list entry for each case */
	if ((p = delcase()) == NULL)
	    break;
	defword();		/* value which selects this case */
	outdec(p->value);
	outbyte(',');		/* now, label for this case */
	printlabel(p->label);
	nl();
    }
    defword();			/* arg list ends with default case label */
    printlabel(defaultlab ? defaultlab : q.exit);
    nl();
    linelabel(q.exit);		/* "break" statements arrive here */
    delswq();
}
/* Evaluate a switch case	 */
docase(defaultlab)
    int            *defaultlab;	/* default case label */
{
    struct case_table casentry;
    register	    ncases;
    struct operand  cexp;

    ncases = 0;
    for (;;) {			/* eval all cases/labels before statement */
	if (amatch("case")) {
	    if (constexp(&cexp) != CONSTANT)
		cexperr();
	    else {
		++ncases;
		casentry.value = cexp.op_val;
		linelabel(casentry.label = getlabel());
		addcase(&casentry);
	    }
	    needpunc(':');
	} else if (amatch("default")) {
	    needpunc(':');
	    if (*defaultlab)	/* is there a default case already? */
		error("Multiple default cases");
	    else
		linelabel(*defaultlab = getlabel());
	} else if (chklabel())	/* labels can be mixed in */
	    ;			/* chklabel does all the work */
	else			/* it's not a case, default or a label */
	    break;
    }
    statement();		/* process the statement */
    return ncases;
}
/* "goto" statement	 */
dogoto()
{
    char	    labelname[NAMESIZE];
    if (symname(labelname))
	jump(addlabel(labelname, SC_NONE));
}
testexp()
{
    needpunc('(');
    expression(LOAD);		/* evaluate an expression and get value */
    needpunc(')');
}
/* end of CC3.C */
