/*
 *	QRESET - Q/C Compiler V3.2 Customization Program
 *
 *			02/04/84
 */

#include <qstdio.h>

char	signon[] = "Q/C Compiler ",
	version[] = "V3.2",
	v_Z80[] = "(Z80)",
	v_8080[] = "(8080)";

int	flavor, 	/* 'z' - Z80, '8' - 8080 */
	chgflag = FALSE; /* no changes yet */

#define READ	0
#define WRITE	1
#define BUFSIZE 128
#define CTLC	0x3	/* ^C */

/* Offsets in CC.COM to modify */

#define PSYMSIZE  0x0		/* symbol table size */
#define PMEMSIZE  PSYMSIZE+1	/* member table size */
#define PTYPESIZE PMEMSIZE+1	/* type table size */
#define PSWQSIZE  PTYPESIZE+1	/* switch/loop table size */
#define PCASSIZE  PSWQSIZE+1	/* case table size */
#define PLITSIZE  PCASSIZE+1	/* macro (#define) pool size */
#define PMACSIZE  PLITSIZE+1	/* literal (string) pool size */
#define PAUSECNT  PMACSIZE+1	/* error pause count */
#define PVERBOSE  PAUSECNT+1	/* terse/verbose switch */
#define PINITSW   PVERBOSE+1	/* large array initialization switch */
#define PREDIRECT PINITSW+1	/* redirection switch */
#define PCODESW   PREDIRECT+1	/* assembler code switch */
#define PCODEEXT  PCODESW+1	/* assembler code file extension */

main(argc, argv)
int argc;
char *argv[];
	{
	static unsigned
		bdos;		/* used to compute CP/M size */
	int
		*pglobal,	/* pointer to start of global variables */
		fd;		/* file descriptor for compiler */
	char
		buffer[BUFSIZE],/* buffer for reading CC.COM */
		ccname[15],	/* complete CP/M filename of compiler */
		*findglb(),	/* function which locates global variables */
		*strcat(),
		*strcpy();

	qprintf("\nQRESET customization program for %s%s ",
		signon, version);

/* Was a drive or alternate filename specified for CC.COM? */

	if (argc > 1) { 	/* compiler name will be 2nd argument */
		strcpy(ccname, argv[1]);
		if (ccname[1] == ':' && ccname[2] == '\0')
			strcat(ccname, "CC.COM"); /* only a drive given */
		else if (!index(ccname, '.'))
			strcat(ccname, ".COM"); /* no file extension */
		}
	else			/* use the standard name */
		strcpy(ccname, "CC.COM");

/* Check for version override (undocumented) */
	if (argc > 2 && strncmp(argv[2], "-V", 2)==0) {
		strncpy(version, argv[2]+1, sizeof(version)-1);
		qprintf("[override %s] ", version);
		}

/* Get part of CC.COM to change */

	if ((fd = open(ccname, READ)) == -1) {
		fputs("\n\n", stderr);
		cantopen(ccname);
		}
	if(read(fd, buffer, BUFSIZE) == 0) {
		qprintf("\n\nError reading %s", ccname);
		_exit();
		}
	close(fd);

/* Check for good file and locate the global flags */

	pglobal = (int *) findglb(buffer, ccname);

/* Find true CP/M size */

	bdos = peek(0x7);	/* high order byte of bdos address */
	qprintf("\nYour CP/M TPA size is: %uK\n", bdos/4);

/* Report old settings and get new settings */

	setswitch(pglobal);
	settables(pglobal);

/* Rewrite the changed part of CC.COM */

	if (chgflag == FALSE)
		nochg();		/* no changes made */
	if ((fd = open(ccname, WRITE)) == -1 ||
	    write(fd, buffer, BUFSIZE) < BUFSIZE) {
		qprintf("\nError writing %s", ccname);
		_exit();
		}
	qprintf("\n%s has been changed", ccname);
	close(fd);
	}

/* Check for good CC.COM file and locate the global flags */
char *findglb(buffer, ccname)
char *buffer, *ccname;
	{
	register char *p;
	register i, found, length;

	found = FALSE;
	length = strlen(signon);	/* locate sign-on message */
	for (p = buffer, i = 0; i < BUFSIZE-length; ++p, ++i) {
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
	for ( ; i < BUFSIZE; ++p, ++i) {
	    if (strcmp(p, v_Z80) == 0) { /* check flavor: Z80 or 8080 */
		qprintf("%s\n", v_Z80);
		flavor = 'z';
		p += strlen(v_Z80) + 1;
		break;
		}
	    else if (strcmp(p, v_8080) == 0) {
		qprintf("%s\n", v_8080);
		flavor = '8';
		p += strlen(v_8080) + 1;
		break;
		}
	    }
	if (flavor == ' ') {
		qprintf("\nCan't tell whether this is for 8080 or Z80\n");
		nochg();
		}
	return (p);			/* beginning of globals */
	}

/* Report that QRESET and Q/C are not the same version */
wrongversion()
	{
	qprintf("\nThis version of QRESET only works with Q/C %s\n",
		version);
	nochg();
	}

/* Report that CC.COM doesn't look good */
badfile(ccname)
char *ccname;
	{
	qprintf("\n%s does not look like a Q/C Compiler -\n", ccname);
	qprintf("  either the file is damaged, or you have changed\n");
	qprintf("  the order of global variables in CGLBDEF.C\n");
	nochg();
	}

/* Confirm that no changes will be made and quit */
nochg()
	{
	qprintf("\nNo changes made");
	_exit();
	}

/* Get response from user and check for request to quit */
getanswer()
	{
	register c;

	c = chlower(getchar());
	if (c == CTLC) {
		qprintf("^C");
		_exit();
		}
	return c;
	}

/* Do all compiler switch settings */
setswitch(pglobal)
register *pglobal;
	{
	register
		*pc,		/* pointer to compiler setting */
		newsize,	/* new size setting */
		newsw;		/* hold area for new switch setting */
	char
		*strcpy(),
		num[7]; 	/* hold area for numeric answers */

/* ASSEMBLER CODE SWITCH */

	if (flavor == '8') {	/* only 8080 has a choice */
	    pc = pglobal + (PCODESW);
	    qprintf("\nCurrently compiler will generate code for %s",
		(*pc == 'a') ? "RMAC" : "M80");
	    do {
		qprintf("\n    Enter A (RMAC), M (M80) or <CR> for no change: ");
		newsw = getanswer();
		} while (newsw != 'a' && newsw != 'm' && newsw != '\n');
	    if (newsw != '\n') {
		*pc = newsw;
		strcpy(pglobal+PCODEEXT, (newsw == 'a') ? "ASM": "MAC");
		qprintf("\n    Compiler changed to generate code for %s\n",
			(newsw == 'a') ? "RMAC" : "M80");
		chgflag = TRUE;
		}
	    }

/* TERSE/VERBOSE SWITCH */

	pc = pglobal + (PVERBOSE);
	qprintf("\nCurrently compiler is in %s mode",
		(*pc) ? "verbose" : "terse");
	do {
		qprintf("\n    Enter T (terse), V (verbose) or <CR> for no change: ");
		newsw = getanswer();
		} while (newsw != 't' && newsw != 'v' && newsw != '\n');
	if (newsw != '\n') {
		*pc = (newsw == 'v');
		qprintf("\n    Compiler changed to %s mode\n",
			(*pc) ? "verbose" : "terse");
		chgflag = TRUE;
		}

/* ERROR PAUSE COUNT */

	pc = pglobal + (PAUSECNT);
	qprintf("\nCurrently compiler pauses after %d error%s\n",
		*pc, (*pc == 1) ? "": "s");
	qprintf("    Enter new size or <CR> for no change: ");
	if (newsize = atoi(gets(num))) {
		qprintf("   New compiler pause count = %d\n", newsize);
		*pc = newsize;
		chgflag = TRUE;
		}
/* LARGE ARRAY INITIALIZATION SWITCH */

	pc = pglobal + (PINITSW);
	qprintf("\nCurrently compiler %s large arrays",
		(*pc) ? "initializes": "does not initialize");
	do {
		qprintf("\n    Enter I (initialize), N (do not initialize)\
 or <CR> for no change: ");
		newsw = getanswer();
		} while (newsw != 'i' && newsw != 'n' && newsw != '\n');
	if (newsw != '\n') {
		*pc = (newsw == 'i');
		qprintf("\n    Compiler changed to %s large arrays\n",
			(*pc) ? "initialize": "skip initialization of");
		chgflag = TRUE;
		}
/* REDIRECTION SWITCH */

	pc = pglobal + (PREDIRECT);
	qprintf("\nCurrently compiler %s redirection",
		(*pc) ? "includes": "excludes");
	do {
		qprintf("\n    Enter R (redirect), N (do not redirect)\
 or <CR> for no change: ");
		newsw = getanswer();
		} while (newsw != 'r' && newsw != 'n' && newsw != '\n');
	if (newsw != '\n') {
		*pc = (newsw == 'r');
		qprintf("\n    Compiler changed to %s redirection\n",
			(*pc) ? "include": "exclude");
		chgflag = TRUE;
		}
	}

/* Check for table size changes */
settables(pglobal)
register *pglobal;
	{
	register *pc;

/* SYMBOL TABLE */

	pc = pglobal + (PSYMSIZE);
	*pc = getsize("SYMBOL TABLE size", *pc, "entries");

/* MEMBER TABLE */

	pc = pglobal + (PMEMSIZE);
	*pc = getsize("MEMBER TABLE size", *pc, "entries");

/* TYPE TABLE */

	pc = pglobal + (PTYPESIZE);
	*pc = getsize("TYPE TABLE size", *pc, "types");

/* LITERAL (string) POOL */

	pc = pglobal + (PLITSIZE);
	*pc = getsize("LITERAL (string) POOL size", *pc, "characters");

/* MACRO (#define) POOL */

	pc = pglobal + (PMACSIZE);
	*pc = getsize("MACRO (#define) POOL size", *pc, "characters");

/* SWITCH/LOOP QUEUE */

	pc = pglobal + (PSWQSIZE);
	*pc = getsize("SWITCH/LOOP nesting depth", *pc, "levels");

/* SWITCH/CASE TABLE */

	pc = pglobal + (PCASSIZE);
	*pc = getsize("SWITCH/CASE TABLE size", *pc, "cases");
	}

/* Report a table size and ask for a change */
getsize(table, size, units)
char *table, *units;
int size;
	{
	char	num[7]; 	/* hold area for numeric answers */
	int	newsize;

	qprintf("\n%s is: %d %s\n", table, size, units);
	qprintf("    Enter new size or <CR> for no change: ");
	if (newsize = atoi(gets(num))) {
		qprintf("    New %s is: %d %s\n", table, newsize, units);
		chgflag = TRUE;
		return newsize;
		}
	else
		return size;
	}
/* end of QRESET.C */
n switch */
#define PREDIRECT PINITSW+1	/* redirecti