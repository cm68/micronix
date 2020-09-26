/********************************************************/
/*							*/
/*		Q/C Compiler Version 3.2		*/
/*			(Part 9)			*/
/*							*/
/*     Copyright (c) 1984 Quality Computer Systems	*/
/*							*/
/*			01/26/84			*/
/********************************************************/

/* NOTE: This module becomes part of the free space after it
 *		is used if you link Q/C using PLINK-II
 */
#include "qstdio.h"
#include "cstddef.h"
#include "cglbdecl.c"

/* Initialization code */
init()
	{
#ifndef PORTABLE
	moat(1200);		/* set free space to reserve for stack */
#endif
	qprintf("%s%s%s\n", signon, version, copyright);
	minsym = nsym;
	minlit = litpsize;
	}
/* Get options from command line */
getoptions(argc, argv)
char *argv[];
	{
	register char *p;
	char *index(), *strcat(), *strcpy();
	static rename = FALSE;

	if (argc < 2)			/* must have at least "cc" + */
		usagerr();		/*	one input file name */
	nextfil = argv + 1;
	while (--argc) {
		if (**++argv == '-')
			break;		/* stop at options */
		++ninfils;
		}
	if (ninfils == 0)
		usagerr();		/* must have an input file */
	getfilename(*nextfil, infil, "C");
	while (argc--) {		/* look for options */
		p = *argv++;
		if (*p == '-') {
		    while (*++p) {
			switch (chlower(*p)) {
#if Z80 == FALSE
			case 'a':	/* generate code for RMAC */
				codeflag = 'a';
				strcpy(defext, "ASM");
				continue;
#endif
			case 'c':	/* commented asm listing */
				fullist = TRUE;
				continue;
			case 'd':	/* output to console (debug) */
				output = stderr;
				continue;
			case 'i':	/* toggle initialization of */
				initflag = !initflag; /* arrays>INITSIZE */
				continue;
			case 'l':	/* generate a library */
				libflag = TRUE;
				continue;
#if Z80 == FALSE
			case 'm':	/* generate code for M80 */
				codeflag = 'm';
				strcpy(defext, "MAC");
				continue;
#endif
			case 'o':	/* rename output file */
				rename = 1;
				continue;
			case 'r':	/* toggle redirection flag */
				redirect = !redirect;
				continue;
			case 's':	/* include shell in ROM code */
				romflag = TRUE;
				lptr = p + 1;	/* look for stk addr */
				spaddr = gethex();
				break;
			case 't':	/* generate trace messages */
				trace.is_on = trace.enabled = TRUE;
				continue;
			case 'v':	/* toggle verbose/terse */
				verbose = !verbose;
				continue;
			default:
				usagerr();
				}
			break;		/* break out of -S switch */
			}
		    }
		else if (rename == 1) { /* should be new output file name */
		    if (p[1] == ':' && p[2] == '\0') {
			strcpy(outfil, p); /* build drive:infil.defext */
			strcat(outfil, infil[1]==':' ? infil+2 : infil);
			*(index(outfil, '.') + 1) = '\0';
			strcat(outfil, defext);
			}
		    else
			getfilename(p, outfil, defext);
		    if (streq(infil, outfil)) {
			puts("Output file name same as input");
			exit(1);
			}
		    rename = 2; 	/* remember we did it */
		    }
		else
			usagerr();
		}
	if (!rename && output == 0) {	/* use infil.defext as output */
		strcpy(outfil, infil);
		*(index(outfil, '.')+1) = '\0'; /* end name after '.' */
		strcat(outfil, defext); /* add default extension */
		}
	else if (rename == 1)		/* new file name not found */
		usagerr();
	}
usagerr()
	{
#if Z80
	puts("Usage:cc infile ... -cdilortv -sxxxx outfile");
#else
	puts("Usage:cc infile ... -acdilmortv -sxxxx outfile");
#endif
	exit(1);
	}
/* Obtain compiler table and buffer space from free space */
getspace()
	{
	char *sbrk();

	peepbuf = sbrk(PEEPBUFSIZE);
	line =	sbrk(LINESIZE);
	mline = sbrk(LINESIZE);
	pcasetab =
	casetab =
		(struct case_table *)
		sbrk(ncases * sizeof(struct case_table));
	pswq =
	swq =
		(struct swq *) sbrk(nswq * sizeof(struct swq));
	typelist =
		(struct typeinfo *)
		sbrk(n_types * sizeof(struct typeinfo));
	inclbuf[0] = sbrk(inclbsize);
	inbuf = sbrk(iobufsize);
	outbuf = sbrk(iobufsize);
	freemem =
	memtab =
		(struct memtab *) sbrk(nmem * sizeof(struct memtab));
	symtab =
	glbptr =
		(struct st *) sbrk(nsym * sizeof(struct st));
	locptr = STARTLOC;
	pmacdef = sbrk(macpsize);
	macpool = pmacsym = pmacdef + macpsize - MACSIZE;
	plitpool =
	litpool =
		sbrk(litpsize);
	if ((int)symtab==-1 || (int)litpool==-1 || (int)pmacdef==-1) {
		puts("Not enough table space");
		exit(1);
		}
	}
/* end of CC9.C */
mon code for processing #if preprocessor commands */
dopreif(newstatus) /* zero means SKIP; n