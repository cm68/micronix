/********************************************************/
/*							*/
/*		Q/C Compiler Version 3.2		*/
/*			(Part 8)			*/
/*							*/
/*     Copyright (c) 1984 Quality Computer Systems	*/
/*							*/
/*			01/03/84			*/
/********************************************************/

#include "qstdio.h"
#include "cstddef.h"
#include "cglbdecl.c"

static char *mptr;

/* This section handles I/O and preprocessor commands */

/* Get the next input file */
getinfil()
	{
	FILE *fopen();
	char *strcpy();
	if (incldepth > 0) {
		input = holdinput[--incldepth]; /* back to interrupted */
		strcpy(infil, inclfil[incldepth]); /* input file */
		lineno = holdlineno[incldepth];
		if (verbose)
			qprintf("*** Resume  %s\n", infil);
		}
	else if (ninfils--) {		/* any more input files? */
		getfilename(*nextfil++, infil, "C");
		if ((input = fopen(infil,"r")) == NULL)
			cantopen(infil);
		setbuf(input, inbuf);
#ifndef PORTABLE
		setbsize(input, iobufsize);
#endif
		lineno = 0;
		if (verbose)
			qprintf("*** Read    %s\n", infil);
		}
	else
		eof = TRUE;
	}
/* Open output file */
getoutfil()
	{
	FILE *fopen();
	if (output == stderr)		/* output to console */
		return;
	if ((output = fopen(outfil, "w")) == NULL)
		cantopen(outfil);
	setbuf(output, outbuf);
#ifndef PORTABLE
	setbsize(output, iobufsize);
#endif
	header();
	}
/* Close the output file */
closeout()
	{
	trailer();
	if (output != stderr && fclose(output) == EOF)
		out_err();
	}
out_err()
	{
	puts("Can't close output file");
	exit(1);
	}
/* Get file name, optionally append extension */
getfilename(s, name, ext)
char *s, *name, *ext;
	{
	register i;
	register char *p;
	char *index();

	p = name;
	i = 0;
	while (*s && ++i <= FILNAMSIZE)
		*p++ = *s++;
	*p = '\0';
	if (index(name, '.') == NULL && ++i < FILNAMSIZE) {
		*p++ =	'.';		      /* no extension given */
		while (*ext && ++i <= FILNAMSIZE) /* use .EXT */
			*p++ = *ext++;
		*p = '\0';
		}
	}
/* Open an include file */
doinclude()
	{
	register char *p, *buf;
	register c, delimiter;
	char *index(), *sbrk(), *strcpy();
	FILE *fopen();

	if (matchc('"'))
		delimiter = '"';
	else if (matchc('<'))
		delimiter = '>';
	else
		delimiter = 0;
	p = lptr;			/* beginning of file name */
	if (delimiter) {
		while ((c=ch())!=delimiter && c!='\0' && !isspace(c))
			gch();
		if (c != delimiter)
			needpunc(delimiter);
		*lptr = '\0';		/* end file name */
		}
	else
		errname("Missing delimiter", "\" or <");
	if (incldepth == MAXINCL)
		error("#include nested too deeply");
	else {
		holdinput[incldepth] = input;	/* save old FILE ptr */
		strcpy(inclfil[incldepth], infil); /* and name */
		holdlineno[incldepth] = lineno; /* save old line num */
		getfilename(p, infil, "C");	/* get include filename */
		buf = inclbuf[incldepth];	/* check for buffer */
		if (buf == NULL)
			buf = inclbuf[incldepth] = sbrk(inclbsize);
		if ((input=fopen(infil, "r")) == NULL || (int)buf == -1) {
			qprintf("#include file -- ");
			cantopen(infil);
			}
		setbuf(input, buf);		/* provide small buffer */
#ifndef PORTABLE
		setbsize(input, inclbsize);
#endif
		lineno = 0;
		++incldepth;
		if (verbose)
			qprintf("*** Include %s\n", infil);
		}
	}
/* Get the next line of input */
inline()
	{
	register c;
	chkabort();
	for (;;) {		/* find a non-empty line or EOF */
		if (eof)
			return;
		kill_line();		/* clear the input line */
		++lineno;
		while ((c = getc(input)) != EOF) {
			if (c == '\n' || lptr-line >= LINEMAX)
				break;
			*lptr++ = c;
			}
		*lptr = '\0';
		if (lptr != line) {	/* anything on this line? */
			if (fullist && cmode) {
				textline = TRUE;
				comment(); /* print C text */
				outstr(line);
				nl();
				textline = FALSE;
				}
			lptr = line;	/* reset line to beginning */
			return;
			}
		else if (c == EOF) {
			fclose(input);
			getinfil();	/* get the next input file */
			}
		}
	}
/* Write a character to the requested output location */
outbyte(c)
register c;
	{
	testflag =
	testdone = FALSE;	/* turn off when anything written */
	if (peepflag && !textline) /* is a peephole pattern pending? */
		dumpeep();	/* dump it out, this is not part of it */
	if (genflag || textline) {
#if Z80 == FALSE
		if (c == '!' && codeflag == 'a')
			c = '#';
#endif
		midline = (c != '\n');
		if (putc(c, output) == EOF)
			out_err();
		}
	}
/* Put a string out via outbyte */
outstr(p)
char *p;
	{
	chkabort();		/* did user type ^C? */
	while (*p)
		outbyte(*p++);
	}
/* Start a new line if we're not at the beginning */
newline()
	{
	if (midline)
		nl();
	}
/* Print a string with a leading TAB */
ot(s)
char *s;
	{
	outbyte('\t');
	outstr(s);
	}
/* Print a line with a leading TAB */
ol(s)
char *s;
	{
	ot(s);
	nl();
	}
nl()
	{
	outbyte('\n');
	}
/* Print an erroneous line with a pointer (^) to	*/
/*	the location where the error was discovered	*/
/*	and a message describing the error		*/
error(mesg)
char *mesg;
	{
	errfile();
	qprintf("%s\n", mesg);
	errptr();
	errpause();		/* time to pause for full screen? */
	}
errname(mesg,name)
char *mesg,*name;
	{
	errfile();
	qprintf("%s: %s\n", mesg, name);
	errptr();
	errpause();
	}
/* Print the name of the file which contains the error */
errfile()
	{
	if (incldepth)			/* are we in a #include file? */
		qprintf("<%s> @ %d: #include ", inclfil[0], holdlineno[0]);
	qprintf("<%s> @ %d: ", infil, lineno);
	}
/* Print the input line with a pointer (^) under the error */
errptr()
	{
	register char *p;
	p = line;
	if (*p == '\0')
		return; 		/* empty line or EOF */
	qprintf("%s\n", p);		/* print erroneous line */
	while (p < lptr)		/* get to error location */
		putchar((*p++ == '\t') ? '\t': ' ');
	puts("^");			/* print error pointer ^ */
	}
/* If screen is full, pause for keyboard response */
errpause()
	{
	++errcnt;
	if ((errcnt % pausecnt == 0) || verbose) {
		qprintf("Press any key to continue or ^C to abort:");
		while (chkabort() == EOF)
			;		/* wait for response */
		putchar('\n');
		}
	}
/* If a ^C was typed abort the run */
chkabort()
	{
	static char mesg[] = "\nCompilation aborted";
	int c;
	if ((c = getkey()) == 0x3) {
		outstr(mesg);
		puts(mesg);
		exit(1);
		}
	return c;
	}
/*	Common error messages		*/
matcherr()
	{
	error("Must match a #if command");
	}
scerr()
	{
	error("Invalid storage class");
	}
typerr()
	{
	error("Invalid type");
	}
illname()
	{
	error("Illegal symbol name");
	junk();
	}
notavail(feature)
char *feature;
	{
	errname("Not implemented", feature);
	}
multidef(name)
char *name;
	{
	errname("Already defined", name);
	}
tagerr(tag)
char *tag;
	{
	errname("Undeclared tag", tag);
	}
needpunc(c)
	{
	static char s[2];
	if (!matchc(s[0]=c))
		errname("Missing punctuation -- assumed present", s);
	}
keyerr(keyword)
char *keyword;
	{
	errname("Ilegal use of a keyword", keyword);
	}
sizerr()
	{
	error("Need explicit array size");
	}
initerr()
	{
	error("Initializer must be constant expression");
	}
cexperr()
	{
	error("Must be constant expression");
	}

/* Test parse line to see if lit matches */
/*	the next sequence of characters */
/*	and skip over them if it does	*/
match(lit)
char *lit;
	{
	register k;
	blanks();	/* skip white space */
	if (k = streq(lptr, lit)) {
		lptr += k;
		return TRUE;
		}
	return FALSE;
	}
/* Test parse line to see if c matches */
/*	the next character and skip it */
matchc(c)
	{
	blanks();	/* skip white space */
	if (*lptr == c) {
		++lptr;
		return TRUE;
		}
	return FALSE;
	}
/* Test parse line to see if 'c' is the next char */
nextc(c)
	{
	blanks();
	return (*lptr == c);
	}
/* Test parse line to see if lit matches */
/*	the next token and skip over it */
/*	if a match is found		*/
amatch(lit)
char *lit;
	{
	blanks();
	if (chks(lit)) {
		skip();
		return TRUE;
		}
	return FALSE;
	}
/* Skip white space in parse line */
blanks()
	{
	register c;
	for (;;) {
		while ((c = ch()) == '\0') {	/* at eol? */
			if (stopeol)		/* OK to pass eol? */
				return; 	/* no */
			inline();		/* get next line */
			if (eof)
				return;
			preprocess();
			}
		if (isspace(c))
			gch();
		else
			return;
		}
	}

#ifdef PORTABLE

/* Match string against next token in parsing buffer */
chks(s)
char *s;
	{
	return (astreq(lptr, s, strlen(s)));
	}
/* Test two strings for equality up to	*/
/*	the end of the second string	*/
streq(s1, s2)
char *s1, *s2;
	{
	register len;
	len = 0;
	while (*s2) {
		if (*s1++ != *s2++)
			return 0;
		++len;
		}
	return len;
	}
/* Test two strings for equality of	*/
/*	contents up to len (<256)	*/
astreq(s1, s2, len)
char *s1, *s2;
	{
	register cnt;
	for (cnt = 0; cnt < len; ++cnt) {
		if (*s2 == '\0')
			break;
		if (*s1++ != *s2++)
			return 0;
		}
	return (isletnum(*s1)) ? 0 : cnt;
	}
/* Test for valid C symbol name characters */
isletnum(c)
	{
	return (isletter(c) || isdigit(c));
	}
isletter(c)
	{
	return (isalpha(c) || c == '_');
	}
/* Skip token in input line */
skip()
	{
	while (isletnum(*lptr)) 	/* skip all a/n chars */
		++lptr;
	}
#endif

/* Throw away garbage in input line */
junk()
	{
	while (!isletnum(ch())) {
		gch();			/* flush non-a/n characters */
		if (endst())		/*	up to EOL or ';' */
			break;
		}
	}
/* Test for end of statement */
endst()
	{
	blanks();	/* skip white space */
	return (ch() == ';' || ch() == '\0');
	}
/* Look at current character in parsing buffer */
ch()
	{
	return *lptr;
	}
/* Look at next character in parsing buffer */
nch()
	{
	return (*lptr ? *(lptr+1) : 0);
	}
/* Get current character in parsing buffer */
gch()
	{
	return (*lptr ? *lptr++ : 0);
	}
/* Put a character back in the parsing buffer */
putback(c)
	{
	if (lptr > line)
		*--lptr = c;
	}
/* Clear current line from parsing buffer */
kill_line()
	{
	lptr = line;
	*lptr = '\0';
	}
/* Check for #preprocessor commands */
preprocess()
	{
	int val;
	char *isdef(), name[NAMESIZE];
	struct operand cexp;

	if (!matchc('#')) {		/* if it's not a #command... */
		if (condif == PROCESS) {/* check conditional compilation */
			lptr = line;	/* start at beginning of line */
			procline();	/* preprocess the input line */
			}
		else
			kill_line();	/* skip this line */
		}
	else {				/* do a #preprocessor command */
		stopeol = TRUE; 	/* look only on this line */
		/* #if, #else and #endif are only commands recognized */
		/*	when C text is being skipped because of #if */
		if (amatch("ifdef"))
			dopreif((int) isdef());
		else if (amatch("ifndef"))
			dopreif(!isdef());
		else if (amatch("if")) {
			procline();	/* preprocess the const expr */
			if (constexp(&cexp) != CONSTANT) {
				cexperr();
				cexp.op_val = 0;
				}
			dopreif(cexp.op_val);
			}
		else if (amatch("else")) {
			if (ifdepth == 0)
				matcherr(); /* no matching #if... */
			else if (ifdepth > MAXIF)
				;	/* nested too deep-can't check */
			else if (condelse)
				error("Only one #else allowed");
			else {
				condelse = TRUE; /* #else found */
				if (condif != IGNORE) /* turn it around */
					condif = (condif==SKIP)?
							PROCESS: SKIP;
				}
			}
		else if (amatch("endif")) {
			if (ifdepth == 0)
				matcherr(); /* no matching #if... */
			else {
				--ifdepth; /* back up one level */
				if (ifdepth < MAXIF) { /* get status */
					condif = holdif[ifdepth];
					condelse = holdelse[ifdepth];
					}
				}
			}
		else if (condif != PROCESS) /* chk #if condition */
			;		/* skip this line */
		else if (amatch("define")) {
			if (symname(name)) { /* valid name? */
				procline();  /* strip comments & apply */
				addmac(name);/* prev defs recursively */
				}
			else
				kill_line(); /* kill rest of line */
			}
		else if (amatch("undef"))
			delmac();
		else if (amatch("include"))
			doinclude();	/* get include file */
		else if (amatch("asm"))
			doasm();	/* copy assembler thru */
		else if (amatch("line")) {
			procline();	/* strip comments */
			if (!decimal(&val))	/* new line number? */
				error("#line number must be decimal");
			else {
				lineno = val;
				if (!endst())	/* optional filename? */
					getfilename(lptr, infil, "C");
				}
			}
		else
			error("Unknown #preprocessor command");
		stopeol = FALSE;	/* don't stop at end-of-line */
		kill_line();
		}
	}
/* Common code for processing #if preprocessor commands */
dopreif(newstatus) /* zero means SKIP; non-zero means PROCESS */
	{
	if (ifdepth >= MAXIF)
		error("#if nested too deeply");
	else {
		holdif[ifdepth] = condif; /* record previous status */
		holdelse[ifdepth] = condelse;
		if (condif == PROCESS)	/* currently processing? */
			condif = (newstatus) ? PROCESS: SKIP;
		else
			condif = IGNORE;
		condelse = FALSE;	/* no #else yet */
		}
	++ifdepth;			/* keep track even if too deep */
	}
/* Preprocess input lines by removing
 * comments and excess white space,
 * checking strings and char constants,
 * & applying #define macros to the rest
 */
procline()
	{
	register char *ptr, *savelptr;
	register c;
	char *chksym(), *findmac(), name[NAMESIZE], *strcpy();
	mptr = mline;
	savelptr = lptr;	/* remember where we are on this line */
	for (ptr = line; ptr < lptr; ++ptr)
		keepch(*ptr);	/* keep anything already processed */
	while (c = ch()) {
		if (c == ' ' || c == '\t') {	/* eliminate excess */
			keepch(' ');		/* white space */
			while (ch() == ' '|| ch() == '\t')
				gch();
			}
		else if (chksym(name)) {	/* is it a symbol? */
			skip(); 		/* skip over it */
			if (ptr=findmac(name))	/* if it's a macro, get */
				ptr = *(int *)(ptr+MACPTR); /* replacement */
			else
				ptr = name;	/* just a name */
			while (*ptr)
				keepch(*ptr++);
			}
		else if (c == '/' && *(lptr+1) == '*') {
			gch();gch();		/* discard comments */
			stopeol = TRUE; 	/* look for special */
			chkcommand();		/* command on current */
			stopeol = FALSE;	/* line only */
			while (*lptr != '*' || *(lptr+1) != '/') {
				if (gch()==0)	/* if EOL get */
					inline(); /* next line */
				if (eof) {
					error("Unclosed comment");
					break;
					}
				}
			gch();gch();		/* discard end of comment */
			}
		else if (c == '"') {
			keepch(gch());		/* pass strings through */
			while (ch()!='"') {	/* unchanged */
				if (ch()==0) {
					error("Missing quote");
					break;
					}
				if (ch()=='\\') { /* pass an escape char */
					if (nch())	/* not EOL */
						keepch(gch());
					else {	/* continue long literal */
						inline();
						continue;
						}
					}
				keepch(gch());
				}
			gch();
			keepch('"');
			}
		else if (c == '\'') {
			keepch(gch());	/* pass char constants */
			while (ch()!='\'') {	/* unchanged */
				if (ch() == 0) {
					error("Missing apostrophe");
					break;
					}
				if (ch() == '\\') /* pass an escape char */
					keepch(gch());
				keepch(gch());
				}
			gch();
			keepch('\'');
			}
		else keepch(gch());	/* pass all else unchanged */
		}
	*mptr = '\0';
	if (mptr-mline>=LINEMAX)
		error("Line too long");
	strcpy(line, mline);		/* put processed line back */
	lptr = savelptr;		/* restore original position */
	}
/* Put a char in macro buffer */
keepch(c)
	{
	if ((mptr - mline) < LINEMAX)
		*mptr++ = c;
	}
/* Process special comment commmands */
chkcommand()
	{
	register on_off;
	register char *option;

	if (matchc('$')) {
	    if ((on_off = matchc('+')) || matchc('-')) {
		switch (chupper(ch())) {
		case 'T':
			if (trace.enabled) {
				trace.is_on = on_off;
				option = "Trace  ";
				break;
				}
			else
				return;
		default:
			return;
			}
		if (verbose)
			qprintf("*** %s %s\n",option,on_off?"ON":"OFF");
		}
	    }
	}
/* Output an integer in decimal */
outdec(n)
	{
	char *itob(), s[7];
	outstr(itob(n, s, -10));
	}
/* end of CC8.C */
found		*/
amatch