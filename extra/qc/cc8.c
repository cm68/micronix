/*********************************************************
 *							 *
 * Q/C Compiler Version 3.1				 *
 * (Part 8)						 *
 *							 *
 * Copyright (c) 1983 Quality Computer Systems		 *
 *							 *
 * 08/06/83						 *
 *							 *
 * the i/o and the lexer				 *
 *							 *
 *********************************************************/

#include "qstdio.h"
#include "cstddef.h"
#include "globals.h"

static char    *mptr;

/* This section handles I/O and preprocessor commands */

/* Get the next input file */
getinfil()
{
    FILE           *fopen();
    char           *strcpy();
    if (incldepth > 0) {
	input = holdinput[--incldepth];	/* back to interrupted */
	strcpy(infil, inclfil[incldepth]);	/* input file */
	lineno = holdlineno[incldepth];
	if (verbose)
	    qprintf("*** resume  %s\n", infil);
    } else if (ninfils--) {	/* any more input files? */
	getfilename(*nextfil++, infil, "C");
	if ((input = fopen(infil, "r")) == NULL)
	    cantopen(infil);
	setbuf(input, inbuf);
	setbsize(input, iobufsize);
	lineno = 0;
	if (verbose)
	    qprintf("*** read    %s\n", infil);
    } else
	eof = TRUE;
}

/* Open output file */
getoutfil()
{
    FILE           *fopen();
    if (output == stderr)	/* output to console */
	return;
    if ((output = fopen(outfil, "w")) == NULL)
	cantopen(outfil);
    setbuf(output, outbuf);
    setbsize(output, iobufsize);
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
getfilename(s, filname, ext)
    char           *s, *filname, *ext;
{
    register	    i;
    register char  *ptr;
    ptr = filname;
    i = 0;
    while (*s && ++i <= FILNAMSIZE)
	*ptr++ = chupper(*s++);
    *ptr = '\0';
    if (index(filname, '.') == NULL && ++i < FILNAMSIZE) {
	*ptr++ = '.';		/* no extension given */
	while (*ext && ++i <= FILNAMSIZE)	/* use .EXT */
	    *ptr++ = chupper(*ext++);
	*ptr = '\0';
    }
}

/* Open an include file */
doinclude()
{
    register char  *p, *buf;
    register	    delimiter;
    char           *strcpy();
    FILE           *fopen();

    if (matchc('"'))
	delimiter = '"';
    else if (matchc('<'))
	delimiter = '>';
    else
	delimiter = 0;
    if (delimiter) {
	if ((p = index(lptr, delimiter)) == NULL)
	    delimerr(delimiter);
	else
	    *p = '\0';
    }
    if (incldepth == MAXINCL)
	error("#include nested too deeply");
    else {
	holdinput[incldepth] = input;	/* save old FILE ptr */
	strcpy(inclfil[incldepth], infil);	/* and name */
	holdlineno[incldepth] = lineno;	/* save old line num */
	getfilename(lptr, infil, "C");	/* get include filename */
	buf = inclbuf[incldepth];	/* check for buffer */
	if (buf == NULL)
	    buf = inclbuf[incldepth] = malloc(inclbsize);
	if ((input = fopen(infil, "r")) == NULL || buf == NULL) {
	    qprintf("#include file -- ");
	    cantopen(infil);
	}
	setbuf(input, buf);	/* provide small buffer */
	setbsize(input, inclbsize);
	lineno = 0;
	++incldepth;
	if (verbose)
	    qprintf("*** include %s\n", infil);
    }
}

/* Get the next line of input */
in_line()
{
    register int    c;
    chkabort();
    for (;;) {			/* find a non-empty line or EOF */
	if (eof)
	    return;
	kill();			/* clear the input line */
	++lineno;
	while ((c = getc(input)) != EOF) {
	    if (c == '\n' || lptr - line >= LINEMAX)
		break;
	    *lptr++ = c;
	}
	*lptr = '\0';
	if (lptr != line) {	/* anything on this line? */
	    if (fullist && cmode) {
		textline = TRUE;
		comment();	/* print C text */
		outstr(line);
		nl();
		textline = FALSE;
	    }
	    lptr = line;	/* reset line to beginning */
	    return;
	} else if (c == EOF) {
	    fclose(input);
	    getinfil();		/* get the next input file */
	}
    }
}

/* Write a character to the requested output location */
outbyte(c)
    register int    c;
{
    testflag =
	testdone = FALSE;	/* turn off when anything written */
    if (peepflag && !textline)	/* is a peephole pattern pending? */
	dumpeep();		/* dump it out, this is not part of it */
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
outstr(ptr)
    char           *ptr;
{
    chkabort();			/* did user type ^C? */
    while (*ptr)
	outbyte(*ptr++);
}

/* Start a new line if we're not at the beginning */
newline()
{
    if (midline)
	nl();
}

/* Print a string with a leading TAB */
ot(s)
    char           *s;
{
    outbyte('\t');
    outstr(s);
}

/* Print a line with a leading TAB */
ol(s)
    char           *s;
{
    ot(s);
    nl();
}

nl()
{
    outbyte('\n');
}

/* Print an erroneous line with a pointer (^) to	 */
/* the location where the error was discovered	 */
/* and a message describing the error		 */
error(mesg)
    char           *mesg;
{
    errpause();			/* time to pause for full screen? */
    qprintf("%s\n", mesg);
    errptr();
}

errname(mesg, name)
    char           *mesg, *name;
{
    errpause();
    qprintf("%s: %s\n", mesg, name);
    errptr();
}

/* If screen is full, pause for keyboard response */
errpause()
{
    if (errcnt > 0 && errcnt % pausecnt == 0) {
	qprintf("Press any key to continue or ^C to abort:");
	while (chkabort() == EOF);	/* wait for response */
	putchar('\n');
    }
    ++errcnt;			/* print current file name */
    if (incldepth)		/* are we in a #include file? */
	qprintf("<%s> @ %d: #include ", inclfil[0], holdlineno[0]);
    qprintf("<%s> @ %d: ", infil, lineno);
}

/* If a ^C was typed abort the run */
chkabort()
{
    static char	    mesg[] = "\nCompilation aborted";
    int		    c;
    if ((c = getkey()) == 0x3) {
	outstr(mesg);
	puts(mesg);
	exit(1);
    }
    return c;
}

/* Print the input line with a pointer (^) under the error */
errptr()
{
    register char  *ptr;
    qprintf("%s\n", line);	/* print erroneous line */
    ptr = line;
    while (ptr < lptr)		/* get to error location */
	putchar((*ptr++ == '\t') ? '\t' : ' ');
    puts("^");			/* print error pointer ^ */
}

#ifdef NATIVE
/* Short version of printf to save space */
qprintf(nargs)
    int		    nargs;
{
    register char  *fmt;
    register	    c , *arg;
    char	    s      [7], *itob();

    arg = &nargs + nargs;
    fmt = *arg;
    while (c = *fmt++) {
	if (c != '%') {
	    putchar(c);
	    continue;
	}
	switch (c = *fmt++) {
	case 'c':
	    putchar(*--arg);
	    continue;
	case 's':
	    fputs(*--arg, stdout);
	    continue;
	case 'd':
	    fputs(itob(*--arg, s, -10), stdout);
	    continue;
	default:
	    putchar(c);
	    continue;
	}
    }
}
#endif

/* Common error messages		 */
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
    char           *feature;
{
    errname("Not implemented", feature);
}

multidef(sname)
    char           *sname;
{
    errname("Already defined", sname);
}

needpunc(c)
    char	    c;
{
    if (!matchc(c)) {
	errpause();
	qprintf("Missing punctuation -- assumed present: %c\n", c);
	errptr();
    }
}

delimerr(delimiter)
    char           *delimiter;
{
    errname("Missing delimiter", delimiter);
}

keyerr(keyword)
    char           *keyword;
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
/* the next sequence of characters */
/* and skip over them if it does	 */
match(lit)
    char           *lit;
{
    register int    k;
    blanks();			/* skip white space */
    if (k = streq(lptr, lit)) {
	lptr += k;
	return TRUE;
    }
    return FALSE;
}

/* Test parse line to see if c matches	 */
/* the next character and skip it	 */
matchc(c)
    int		    c;
{
    blanks();			/* skip white space */
    if (*lptr == c) {
	++lptr;
	return TRUE;
    }
    return FALSE;
}

/* Test parse line to see if 'c' is the next char */
nextc(c)
    int		    c;
{
    blanks();
    return (*lptr == c);
}

/* Test parse line to see if lit matches */
/* the next token and skip over it */
/* if a match is found		 */
amatch(lit)
    char           *lit;
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
    register int    c;
    for (;;) {
	while ((c = ch()) == '\0') {	/* at eol? */
	    if (stopeol)	/* OK to pass eol? */
		return;		/* no */
	    in_line();		/* get next line */
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
    char           *s;
{
    return (astreq(lptr, s, strlen(s)));
}

/* Test two strings for equality up to	 */
/* the end of the second string	 */
streq(s1, s2)
    char           *s1, *s2;
{
    register	    len;
    len = 0;
    while (*s2) {
	if (*s1++ != *s2++)
	    return 0;
	++len;
    }
    return len;
}

/* Test two strings for equality of	 */
/* contents up to len (<256)	 */
astreq(s1, s2, len)
    char           *s1, *s2;
    int		    len;
{
    register	    cnt;
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
    int		    c;
{
    return (isletter(c) || isdigit(c));
}

isletter(c)
    int		    c;
{
    return (isalpha(c) || c == '_');
}

/* Skip token in input line */
skip()
{
    while (isletnum(*lptr))	/* skip all a/n chars */
	++lptr;
}
#endif

/* Throw away garbage in input line */
junk()
{
    while (!isletnum(ch())) {
	gch();			/* flush non-a/n characters */
	if (endst())		/* up to EOL or ';' */
	    break;
    }
}

/* Test for end of statement */
endst()
{
    blanks();			/* skip white space */
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
    return (*lptr ? *(lptr + 1) : 0);
}

/* Get current character in parsing buffer */
gch()
{
    return (*lptr ? *lptr++ : 0);
}

/* Put a character back in the parsing buffer */
putback(c)
    int		    c;
{
    if (lptr > line)
	*--lptr = c;
}

/* Clear current line from parsing buffer */
kill()
{
    lptr = line;
    *lptr = '\0';
}

/* Get the next character from parsing buffer */
/* refilling buffer first if at EOL      */
inbyte()
{
    while (ch() == 0) {
	if (eof)
	    return 0;
	in_line();
	preprocess();		/* do any reqd preprocessing */
    }
    return gch();
}

/* Check for #preprocessor commands */
preprocess()
{
    int		    val;
    char           *isdef(), name[NAMESIZE];
    struct operand  cexp;

    if (!matchc('#')) {		/* if it's not a #command... */
	if (condif == PROCESS) {/* check conditional compilation */
	    lptr = line;	/* start at beginning of line */
	    procline();		/* preprocess the input line */
	} else
	    kill();		/* skip this line */
    } else {			/* do a #preprocessor command */
	stopeol = TRUE;		/* look only on this line */
	/* #if, #else and #endif are only commands recognized */
	/* when C text is being skipped because of #if */
	if (amatch("ifdef"))
	    dopreif(isdef());
	else if (amatch("ifndef"))
	    dopreif(!isdef());
	else if (amatch("if")) {
	    procline();		/* preprocess the const expr */
	    if (constexp(&cexp) != CONSTANT) {
		cexperr();
		cexp.op_val = 0;
	    }
	    dopreif(cexp.op_val);
	} else if (amatch("else")) {
	    if (ifdepth == 0)
		matcherr();	/* no matching #if... */
	    else if (ifdepth > MAXIF);	/* nested too deep-can't check */
	    else if (condelse)
		error("Only one #else allowed");
	    else {
		condelse = TRUE;/* #else found */
		if (condif != IGNORE)	/* turn it around */
		    condif = (condif == SKIP) ?
			PROCESS : SKIP;
	    }
	} else if (amatch("endif")) {
	    if (ifdepth == 0)
		matcherr();	/* no matching #if... */
	    else {
		--ifdepth;	/* back up one level */
		if (ifdepth < MAXIF) {	/* get status */
		    condif = holdif[ifdepth];
		    condelse = holdelse[ifdepth];
		}
	    }
	} else if (condif != PROCESS)	/* chk #if condition */
	    ;			/* skip this line */
	else if (amatch("define")) {
	    if (symname(name)) {/* valid name? */
		procline();	/* strip comments & apply */
		addmac(name);	/* prev defs recursively */
	    } else
		kill();		/* kill rest of line */
	} else if (amatch("undef"))
	    delmac();
	else if (amatch("include"))
	    doinclude();	/* get include file */
	else if (amatch("asm")) {
	    stopeol = FALSE;	/* don't stop at end-of-line */
	    doasm();
	    return;
	} else if (amatch("line")) {
	    procline();		/* strip comments */
	    if (!decimal(&val))	/* new line number? */
		error("#line number must be decimal");
	    else {
		lineno = val;
		if (!endst())	/* optional filename? */
		    getfilename(lptr, infil, "C");
	    }
	} else
	    error("Unknown #preprocessor command");
	stopeol = FALSE;	/* don't stop at end-of-line */
	kill();
    }
}

/* Common code for processing #if preprocessor commands */
dopreif(newstatus)
    int		    newstatus;	/* zero means SKIP; non-zero means PROCESS */
{
    if (ifdepth >= MAXIF)
	error("#if nested too deeply");
    else {
	holdif[ifdepth] = condif;	/* record previous status */
	holdelse[ifdepth] = condelse;
	if (condif == PROCESS)	/* currently processing? */
	    condif = (newstatus) ? PROCESS : SKIP;
	else
	    condif = IGNORE;
	condelse = FALSE;	/* no #else yet */
    }
    ++ifdepth;			/* keep track even if too deep */
}

/* "#asm" preprocessor command */
doasm()
{
    register	    c;
    cmode = FALSE;		/* this is not C */
    newline();			/* start a new line if necessary */
    for (;;) {
	in_line();		/* get next line */
	if (ch() == ';')
	    continue;		/* discard comment line */
	if (streq(line, "#endasm") || eof)
	    break;
	if (ch() == '#' && match("#include")) {
	    doinclude();
	    continue;
	}
	while (c = gch()) {
	    if (c == ';')
		break;		/* strip comments */
	    outbyte(c);
	}
	nl();
    }
    kill();			/* clear #endasm line */
    cmode = TRUE;		/* back in C mode */
}

/*
 * Preprocess input lines by removing comments and excess white space,
 * checking strings and char constants, & applying #define macros to the rest
 */
procline()
{
    register char  *ptr, *savelptr;
    register	    c;
    char           *chksym(), *findmac(), sname[NAMESIZE], *strcpy();
    mptr = mline;
    savelptr = lptr;		/* remember where we are on this line */
    for (ptr = line; ptr < lptr; ++ptr)
	keepch(*ptr);		/* keep anything already processed */
    while (c = ch()) {
	if (c == ' ' || c == '\t') {	/* eliminate excess */
	    keepch(' ');	/* white space */
	    while ((ch() == ' ') || (ch() == '\t'))
		gch();
	} else if (chksym(sname)) {	/* is it a symbol? */
	    skip();		/* skip over it */
	    if (ptr = findmac(sname))	/* if it's a macro, get */
		ptr = *(int *)(ptr + MACPTR);	/* replacement */
	    else
		ptr = sname;	/* just a name */
	    while (*ptr)
		keepch(*ptr++);
	} else if (c == '/' && *(lptr + 1) == '*') {
	    gch();
	    gch();		/* discard comments */
	    while (*lptr != '*' || *(lptr + 1) != '/') {
		if (gch() == 0)	/* if EOL get */
		    in_line();	/* next line */
		if (eof) {
		    error("Unclosed comment");
		    break;
		}
	    }
	    gch();
	    gch();		/* discard end of comment */
	} else if (c == '"') {
	    keepch(gch());	/* pass strings through */
	    while (ch() != '"') {	/* unchanged */
		if (ch() == 0) {
		    error("Missing quote");
		    break;
		}
		if (ch() == '\\') {	/* pass an escape char */
		    if (nch())	/* not EOL */
			keepch(gch());
		    else {	/* continue long literal */
			in_line();
			continue;
		    }
		}
		keepch(gch());
	    }
	    gch();
	    keepch('"');
	} else if (c == '\'') {
	    keepch(gch());	/* pass char constants */
	    while (ch() != '\'') {	/* unchanged */
		if (ch() == 0) {
		    error("Missing apostrophe");
		    break;
		}
		if (ch() == '\\')	/* pass an escape char */
		    keepch(gch());
		keepch(gch());
	    }
	    gch();
	    keepch('\'');
	} else
	    keepch(gch());	/* pass all else unchanged */
    }
    *mptr = '\0';
    if (mptr - mline >= LINEMAX)
	error("Line too long");
    strcpy(line, mline);	/* put processed line back */
    lptr = savelptr;		/* restore original position */
}

/* Put a char in macro buffer */
keepch(c)
    char	    c;
{
    if ((mptr - mline) < LINEMAX)
	*mptr++ = c;
}

/* Output an integer in decimal */
outdec(number)
    int		    number;
{
    char           *itob(), s[7];
    outstr(itob(number, s, -10));
}
/* end of CC8.C */
