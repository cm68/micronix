/* Q/C Compiler Version 3.2
 *	Global Declarations
 *	Copyright (c) 1984 Quality Computer Systems
 *	01/29/84
 */
extern
int	segtype,
	fullist,
	mainflag,
	libflag,
	genflag,
	romflag,
	spaddr,
	cmode,
	midline,
	pregflag,
	retvalue,
	testdone,
	eof,
	stopeol,
	infunc,
	textline;
extern
struct _switch
	trace;
extern
struct typeinfo
	*typelist,
	*basetypes[],
	*functype,
	*labeltype,
	*chartype,
	*inttype,
	*unsgntype;
extern
struct st
	*symtab,
	*glbptr,
	*locptr;
extern
unsigned
	minsym;
extern
struct memtab
	*memtab,
	*freemem;
extern
struct st
	lit_sym;
extern
unsigned
	minlit;
extern
char	*litpool,
	*plitpool;
extern
char	*macpool,
	*pmacsym,
	*pmacdef;
extern
struct swq
	*swq,
	*pswq;
extern
struct case_table
	*casetab,
	*pcasetab;
extern
char	*line,
	*lptr,
	*mline;
extern
char	infil[],
	**nextfil,
	*inbuf,
	outfil[],
	*outbuf,
	inclfil[][FILNAMSIZE+1],
	*inclbuf[];
extern
FILE	*input,
	*output,
	*holdinput[];
extern
int	ninfils,
	iobufsize,
	inclbsize,
	lineno,
	incldepth,
	holdlineno[],
	ifdepth,
	holdif[],
	holdelse[],
	condif,
	condelse,
	locspace,
	retlab,
	nregvar,
	ncmp,
	errcnt;
extern
int	peepflag,
	testflag,
	jcondlabel,
	jumplabel;
extern
char	*peepbuf,
	peepsym[];
extern
int	peepoffset,
	peeplen;
extern
char	copyright[],
	signon[],
	version[];
extern
int	nsym,
	nmem,
	n_types,
	nswq,
	ncases,
	litpsize,
	macpsize,
	initflag,
	pausecnt,
	verbose,
	redirect,
	codeflag;
extern
char	defext[];
/* end of CGLBDECL.C */
ter struct fcb *fcb;

	fd -= 6;	/* skip stdin, stdout, stderr, CP/M LST: */