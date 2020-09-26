/*
 * QRESET - Q/C Compiler V3.1 Customization Program
 * 
 * 07/23/83
 */

#include <qstdio.h>

char		signon    [] = "Q/C Compiler ", version[] = "V3.1", v_Z80[] = "(Z80)", v_8080[] = "(8080)";

int		flavor    ,	/* 'z' - Z80, '8' - 8080 */
		chgflag = FALSE;/* record whether any changes were made */

#define READ	0
#define WRITE	1
#define BUFSIZE 128
#define CTLC	0x3		/* CP/M ^C character */

/* Offsets in CC.COM to modify	 */

#define PSYMSIZE  0x0		/* symbol table size */
#define PTYPESIZE 0x1		/* type table size */
#define PSWQSIZE  0x2		/* switch/loop table size */
#define PCASSIZE  0x3		/* case table size */
#define PMACSIZE  0x4		/* macro (#define) pool size */
#define PAUSECNT  0x5		/* error pause count */
#define PINITSW   0x6		/* default array init switch */
#define PVERBOSE  0x7		/* terse/verbose switch */
#define PREDIRECT 0x8		/* redirection switch */
#define PCODESW   0x9		/* assembler code switch */
#define PCODEEXT  0xA		/* assembler code file extension */

main(argc, argv)
    int		    argc;
    char           *argv[];
{
    static unsigned
    		    bdos;	/* used to compute CP/M size */
    int
                   *pglobal,	/* pointer to start of global variables */
    		    fd;		/* file descriptor for compiler */
    char
    		    buffer    [BUFSIZE],	/* buffer for reading CC.COM */
    		    ccname    [15],	/* complete CP/M filename of compiler */
                   *findglb(),	/* function which locates global variables */
                   *strcat(), *strcpy();

    extern char	    _patch;	/* patch location for non-CP/M systems */
    _patch = 0x26;		/* force H register to zero in bdos() */

    printf("\nQRESET customization program for %s%s ",
	   signon, version);

    /* Was a drive or alternate filename specified for CC.COM? */

    if (argc > 1) {		/* compiler name will be 2nd argument */
	strcpy(ccname, argv[1]);
	if (ccname[1] == ':' && ccname[2] == '\0')
	    strcat(ccname, "CC.COM");	/* only a drive given */
	else if (!index(ccname, '.'))
	    strcat(ccname, ".COM");	/* no file extension */
    } else			/* use the standard name */
	strcpy(ccname, "CC.COM");

    /* Get part of CC.COM to change */

    if ((fd = open(ccname, READ)) == -1) {
	fputs("\n\n", stderr);
	cantopen(ccname);
    }
    if (read(fd, buffer, BUFSIZE) == 0) {
	printf("\n\nError reading %s", ccname);
	_exit();
    }
    close(fd);

    /* Check for good file and locate the global flags */

    pglobal = (int *)findglb(buffer, ccname);

    /* Find true CP/M size */

    bdos = peek(0x7);		/* high order byte of bdos address */
    printf("\nYour CP/M TPA size is: %uK\n", bdos / 4);

    /* Report old settings and get new settings */

    setswitch(pglobal);
    settables(pglobal);

    /* Rewrite the changed part of CC.COM */

    if (chgflag == FALSE)
	nochg();		/* no changes made */
    if ((fd = open(ccname, WRITE)) == -1 ||
	write(fd, buffer, BUFSIZE) < BUFSIZE) {
	printf("\nError writing %s", ccname);
	_exit();
    }
    printf("\n%s has been changed", ccname);
    close(fd);
}

/* Check for good CC.COM file and locate the global flags */
char           *
findglb(buffer, ccname)
    char           *buffer, *ccname;
{
    register char  *p;
    register	    i , found, length;

    found = FALSE;
    length = strlen(signon);	/* locate sign-on message */
    for (p = buffer, i = 0; i < BUFSIZE - length; ++p, ++i) {
	if (strcmp(p, signon) == 0) {
	    found = TRUE;
	    break;
	}
    }
    p += length + 1;		/* version number comes next */
    if (!found || *p != 'V')
	badfile(ccname);
    length = strlen(version);
    if (strncmp(p, version, length) != 0)
	wrongversion();
    else {
	p += length;
	i += length;
    }
    flavor = ' ';
    for (; i < BUFSIZE; ++p, ++i) {
	if (strcmp(p, v_Z80) == 0) {	/* check flavor: Z80 or 8080 */
	    printf("%s\n", v_Z80);
	    flavor = 'z';
	    p += strlen(v_Z80) + 1;
	    break;
	} else if (strcmp(p, v_8080) == 0) {
	    printf("%s\n", v_8080);
	    flavor = '8';
	    p += strlen(v_8080) + 1;
	    break;
	}
    }
    if (flavor == ' ') {
	printf("\nCan't tell whether this is for 8080 or Z80\n");
	nochg();
    }
    return (p);			/* beginning of globals */
}

strncmp(s1, s2, n)
    register char  *s1, *s2;
    int		    n;
{
    while (--n > 0 && *s1) {
	if (*s1 != *s2)
	    break;
	++s1;
	++s2;
    }
    return (*s1 - *s2);
}

/* Report that QRESET and Q/C are not the same version */
wrongversion()
{
    printf("\nThis version of QRESET only works with Q/C %s\n",
	   version);
    nochg();
}

/* Report that CC.COM doesn't look good */
badfile(ccname)
    char           *ccname;
{
    printf("\n%s does not look like a Q/C Compiler -\n", ccname);
    printf(" either the file is damaged, or you have changed\n");
    printf(" the order of global variables in CGLBDEF.C\n");
    nochg();
}

/* Confirm that no changes will be made and quit */
nochg()
{
    printf("\nNo changes made");
    _exit();
}

/* Get response from user and check for request to quit */
getanswer()
{
    register	    c;

    c = chlower(getchar());
    if (c == CTLC) {
	printf("^C");
	_exit();
    }
    return c;
}

/* Do all compiler switch settings */
setswitch(pglobal)
    register       *pglobal;
{
    register
                   *pc,		/* pointer to compiler setting */
    		    newsize  ,	/* new size setting */
    		    newsw;	/* hold area for new switch setting */
    char
                   *strcpy(), num[7];	/* hold area for numeric answers */

    /* ASSEMBLER CODE SWITCH	 */

    if (flavor == '8') {	/* only 8080 has a choice */
	pc = pglobal + PCODESW;
	printf("\nCurrently compiler will generate code for %s",
	       (*pc == 'a') ? "RMAC" : "M80");
	do {
	    printf("\nEnter A (RMAC), M (M80) or <CR> for no change: ");
	    newsw = getanswer();
	} while (newsw != 'a' && newsw != 'm' && newsw != '\n');
	if (newsw != '\n') {
	    *pc = newsw;
	    strcpy(pglobal + PCODEEXT, (newsw == 'a') ? "ASM" : "MAC");
	    printf("\nCompiler changed to generate code for %s\n",
		   (newsw == 'a') ? "RMAC" : "M80");
	    chgflag = TRUE;
	}
    }
    /* TERSE/VERBOSE SWITCH	 */

    pc = pglobal + PVERBOSE;
    printf("\nCompiler is currently in %s mode",
	   (*pc) ? "verbose" : "terse");
    do {
	printf("\nEnter T (terse), V (verbose) or <CR> for no change: ");
	newsw = getanswer();
    } while (newsw != 't' && newsw != 'v' && newsw != '\n');
    if (newsw != '\n') {
	*pc = (newsw == 'v');
	printf("\nCompiler changed to %s mode\n",
	       (*pc) ? "verbose" : "terse");
	chgflag = TRUE;
    }
    /* ERROR PAUSE COUNT:	 */

    pc = pglobal + PAUSECNT;
    printf("\nCompiler currently pauses after %d error%s\n",
	   *pc, (*pc == 1) ? "" : "s");
    printf("Enter new size or <CR> for no change: ");
    if (newsize = atoi(gets(num))) {
	printf("New error pause count = %d\n", newsize);
	*pc = newsize;
	chgflag = TRUE;
    }
    /* LARGE VARIABLE INITIALIZATION	 */

    pc = pglobal + PINITSW;
    printf("\nCompiler currently %s large variables",
	   (*pc) ? "initializes" : "does not initialize");
    do {
	printf("\nEnter I (initialize), N (do not initialize)\
 or <CR> for no change: ");
	newsw = getanswer();
    } while (newsw != 'i' && newsw != 'n' && newsw != '\n');
    if (newsw != '\n') {
	*pc = (newsw == 'i');
	printf("\nCompiler changed to %s large variables\n",
	       (*pc) ? "initialize" :
	       "skip initialization of");
	chgflag = TRUE;
    }
    /* REDIRECTION SWITCH		 */

    pc = pglobal + PREDIRECT;
    printf("\nCompiler currently %s redirection",
	   (*pc) ? "includes" : "excludes");
    do {
	printf("\nEnter R (redirect), N (do not redirect)\
 or <CR> for no change: ");
	newsw = getanswer();
    } while (newsw != 'r' && newsw != 'n' && newsw != '\n');
    if (newsw != '\n') {
	*pc = (newsw == 'r');
	printf("\nCompiler changed to %s redirection\n",
	       (*pc) ? "include" : "exclude");
	chgflag = TRUE;
    }
}

/* Check for table size changes */
settables(pglobal)
    register       *pglobal;
{
    register       *pc;

    /* SYMBOL TABLE:	 */

    pc = pglobal + PSYMSIZE;
    *pc = getsize("SYMBOL TABLE size", *pc, "entries");

    /* TYPE TABLE:	 */

    pc = pglobal + PTYPESIZE;
    *pc = getsize("TYPE TABLE size", *pc, "types");

    /* MACRO (#define) POOL:	 */

    pc = pglobal + PMACSIZE;
    *pc = getsize("MACRO (#define) POOL size", *pc, "characters");

    /* SWITCH/LOOP QUEUE:	 */

    pc = pglobal + PSWQSIZE;
    *pc = getsize("SWITCH/LOOP nesting depth", *pc, "levels");

    /* SWITCH/CASE TABLE:	 */

    pc = pglobal + PCASSIZE;
    *pc = getsize("SWITCH/CASE TABLE size", *pc, "cases");
}

/* Report a table size and ask for a change */
getsize(table, size, units)
    char           *table, *units;
    int		    size;
{
    char	    num    [7];	/* hold area for numeric answers */
    int		    newsize;

    printf("\n%s is: %d %s\n", table, size, units);
    printf("Enter new size or <CR> for no change: ");
    if (newsize = atoi(gets(num))) {
	printf("New %s is: %d %s\n", table, newsize, units);
	chgflag = TRUE;
	return newsize;
    } else
	return size;
}
/* end of QRESET.C */
