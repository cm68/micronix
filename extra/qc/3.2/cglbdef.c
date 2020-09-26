/********************************************************/
/*							*/
/*		Q/C Compiler Version 3.2		*/
/*		   Global Definitions			*/
/*							*/
/*     Copyright (c) 1984 Quality Computer Systems	*/
/*							*/
/*			03/12/84			*/
/********************************************************/

int	segtype = CODE, 	/* record whether code or data segment */
	fullist = FALSE,	/* commented listing? */
	mainflag = FALSE,	/* has main() been defined? */
	libflag = FALSE,	/* is library being generated? */
	genflag = TRUE, 	/* FALSE to suppress code generation */
	romflag = FALSE,	/* generating ROMable code? */
	spaddr, 		/* if romflag==TRUE, addr to init SP */
	cmode = TRUE,		/* parsing C or passing asm thru? */
	midline = FALSE,	/* at beginning of output line? */
	pregflag = FALSE,	/* does preg contain curr val of expr? */
	retvalue = FALSE,	/* is return value in preg? */
	testdone = FALSE,	/* was a test (e.g. ==) just done? */
	eof = FALSE,		/* reached end-of-file? */
	stopeol = FALSE,	/* TRUE if parser must stop at end of */
				/*	line when skipping white space */
	infunc = FALSE, 	/* compiling a function? */
	textline = FALSE;	/* printing a C text line? */
struct _switch
	trace = {FALSE, FALSE}; /* print trace messages? */
/*
 *	Compiler tables & pointers
 */
struct typeinfo
	*typelist,		/* the type table */
	*basetypes[T_MAX],	/* heads of each base-type list */
	*functype,		/* constant 'function returning int' */
	*labeltype,		/* constant 'statement label' */
	*chartype,		/* constant 'char'	*/
	*inttype,		/* constant 'int'	*/
	*unsgntype;		/* constant 'unsigned'	*/
/*
 *	Symbol table
 */
struct st
	*symtab,	/* beginning of symbol table */
	*glbptr,	/* next global entry */
	*locptr;	/* next local entry */
unsigned
	minsym; 	/* record minimum symbol table space */
/*
 *	Member table
 */
struct memtab
	*memtab,	/* beginning of structure member table */
	*freemem;	/* first free entry in member table */
/*
 *	Literal pool
 */
struct st
	lit_sym 	/* special "symbol" for literal pool */
		= {SC_STATIC, NULL, FIRSTLABEL, "$LITPOOL", ID_VAR};
unsigned
	minlit; 	/* record minimum literal pool space */
char	*litpool,	/* beginning of literal pool */
	*plitpool;	/* next char in literal pool */
/*
 *	Macro pool
 */
char	*macpool,	/* beginning of macro pool */
	*pmacsym,	/* next macro pool sym entry */
	*pmacdef;	/* next macro pool def entry */
/*
 *	Switch/loop queue
 */
struct swq
	*swq,		/* start of switch/loop queue */
	*pswq;		/* next switch/loop entry */
/*
 *	Switch/case table
 */
struct case_table
	*casetab,	/* start of switch case table */
	*pcasetab;	/* next switch case entry */

char	*line,		/* parsing buffer */
	*lptr,		/* parsing line buffer */
	*mline; 	/* temp macro buffer */
/*
 *	Misc variables
 */
char	infil[FILNAMSIZE+1],	/* current input file name */
	**nextfil,		/* ptr to name of next input file */
	*inbuf, 		/* ptr to input buffer */
	outfil[FILNAMSIZE+1],	/* output file name */
	*outbuf,		/* ptr to output buffer */
	inclfil[MAXINCL][FILNAMSIZE+1], /* include file names */
	*inclbuf[MAXINCL];	/* ptrs to include buffers */

FILE	*input, 	/* iob for current input file */
	*output,	/* iob for output file (if any) */
	*holdinput[MAXINCL]; /* hold file iob during #include */

int	ninfils,	/* # of input files */
	iobufsize=IOBUF,/* size of input/output buffers */
	inclbsize=INCBUF,/* size of include buffers */
	lineno, 	/* line number in current input file */
	incldepth,	/* current depth of #include nesting */
	holdlineno[MAXINCL], /* hold input file line no during #include */
	ifdepth = 0,	/* current depth of #if nesting */
	holdif[MAXIF],	/* hold process status of nested #if */
	holdelse[MAXIF],/* record presence of #else for nested #if */
	condif=PROCESS, /* records whether #if says to PROCESS or SKIP */
	condelse=FALSE, /* records whether matching #else was found */
	locspace,	/* local space needed in current function */
	retlab, 	/* label # of return for current function */
	nregvar,	/* # of register variables in current function */
	ncmp,		/* # open compound statements */
	errcnt; 	/* # errors in compilation */
/*
 *	Peephole optimization variables
 */
int	peepflag,	/* type of pattern (if any) */
	testflag,	/* = TESTED, REGH, or REGL */
	jcondlabel,	/* label on last conditional jump */
	jumplabel;	/* label on following unconditional jump */
char	*peepbuf,	/* buffer for patterns */
	peepsym[NAMESIZE]; /* name of symbol in preg */
int	peepoffset,	/* offset from symbol in preg */
	peeplen;	/* length of symbol in preg */

/********************************************************/
/*	Global variables referenced by QRESET		*/
/*							*/
/* If you change the compiler & you still want QRESET	*/
/* to work, this code must remain unchanged.		*/
/********************************************************/

#ifndef PORTABLE
#asm
	CSEG
#endasm
#endif

char	copyright[] = " Copyright (c) 1984 Quality Computer Systems";
char	signon[] = "Q/C Compiler ";

#if Z80
char	version[] = "V3.2b (Z80)";
#else
char	version[] = "V3.2b (8080)";
#endif

int	nsym = 150,		/* size of symbol table */
	nmem = 50,		/* size of structure member table */
	n_types = 50,		/* size of type table */
	nswq = 10,		/* max entries in switch/loop queue */
	ncases = 50,		/* max entries in switch case table */
	litpsize = 1000,	/* size of literal pool */
	macpsize = 1000,	/* size of macro (#define) pool */
	pausecnt = 6,		/* pause after this many errors on screen */
	verbose = TRUE, 	/* TRUE to chat during compile */
	initflag= FALSE,	/* TRUE to initialize arrays>INITSIZE */
	redirect = FALSE,	/* TRUE if redirection requested */
	codeflag = DEFASM;	/* default assembler: 'm'-M80, 'a'-RMAC */
#if DEFASM == 'm'
char	defext[] = "MAC";
#else
char	defext[] = "ASM";
#endif

#ifndef PORTABLE
#asm
	DSEG
#endasm
#endif

/********************************************************/
/*	End of global variables referenced by QRESET	*/
/********************************************************/

/* end of CGLBDEF.C */
