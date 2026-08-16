/*
 * sed -- stream editor
 *
 * 2.11BSD sed (sed.h 4.1, 85/04/05), ported to micronix
 *
 * cmd/sed/sed.h
 *
 * Three things changed here, and why:
 *
 * WHAT WAS union reptr IS A struct.  The original declared two
 * struct shapes inside a union and reached members through the
 * union pointer directly - rep->ad1, ipc->lb1 - which is the old C
 * where a member name was an offset and any pointer would do.  The
 * two shapes differ in exactly one member: re1 is text (an RE, or
 * the argument of a, i, c, r) and lb1 is a branch target.  That
 * pair is a union inside one struct now, and every reference reads
 * u.re1 or u.lb1.
 *
 * THE GLOBALS MOVED OUT.  This header defined every global and was
 * included by both sed0.c and sed1.c, which an old linker resolved
 * by merging common symbols.  The definitions live at the top of
 * sed0.c now and this header only declares them - the same shape
 * as sh.h next door.
 *
 * THE BUFFERS SHRANK.  2.11 sized these for a PDP-11 with split
 * instruction and data space, 64k of data alone.  Here the image
 * is code, data, stack and heap in one 64k, so:
 *
 *	LBSIZE	4000 -> 2000	longest line (linebuf, holdsp, genbuf)
 *	RESIZE	10000 -> 5000	compiled REs and a/i/c text, in sum
 *	PTRSIZE	200 -> 128	compiled commands
 *	NLINES	256 -> 128	numeric addresses
 *
 * which takes the static data from about 28k to about 13k.  A
 * script that overruns any of them says so and exits; nothing
 * fails quietly.
 *
 * vim: tabstop=8 shiftwidth=8 noexpandtab:
 */

#define CBRA	1
#define	CCHR	2
#define	CDOT	4
#define	CCL	6
#define	CNL	8
#define	CDOL	10
#define	CEOF	11
#define CKET	12
#define CNULL	13
#define CLNUM	14
#define CEND	16
#define CDONT	17
#define	CBACK	18

#define	STAR	01

#define NLINES	128
#define	DEPTH	20
#define PTRSIZE	128
#define RESIZE	5000
#define	ABUFSIZE	20
#define	LBSIZE	2000
#define	ESIZE	256
#define	LABSIZE	50
#define NBRA	9

#define ACOM	01
#define BCOM	020
#define CCOM	02
#define	CDCOM	025
#define	CNCOM	022
#define COCOM	017
#define	CPCOM	023
#define DCOM	03
#define ECOM	015
#define EQCOM	013
#define FCOM	016
#define GCOM	027
#define CGCOM	030
#define HCOM	031
#define CHCOM	032
#define ICOM	04
#define LCOM	05
#define NCOM	012
#define PCOM	010
#define QCOM	011
#define RCOM	06
#define SCOM	07
#define TCOM	021
#define WCOM	014
#define	CWCOM	024
#define	YCOM	026
#define XCOM	033

struct	reptr {
	char	*ad1;
	char	*ad2;
	union {
		char	*re1;
		struct	reptr *lb1;
	} u;
	char	*rhs;
	FILE	*fcode;
	char	command;
	char	gfl;
	char	pfl;
	char	inar;
	char	negfl;
};

struct label {
	char	asc[9];
	struct	reptr *chain;
	struct	reptr *address;
};

extern FILE	*fin;
extern struct	reptr *abuf[ABUFSIZE];
extern struct	reptr **aptr;
extern char	*lastre;
extern char	ibuf[BUFSIZ];
extern char	*cbp;
extern char	*ebp;
extern char	genbuf[LBSIZE];
extern char	*loc1;
extern char	*loc2;
extern char	*locs;
extern char	seof;
extern char	*reend;
extern char	*lbend;
extern char	*hend;
extern char	*lcomend;
extern struct	reptr *ptrend;
extern int	eflag;
extern char	dolflag;
extern char	sflag;
extern char	jflag;
extern char	numbra;
extern char	delflag;
extern long	lnum;
extern char	linebuf[LBSIZE+1];
extern char	holdsp[LBSIZE+1];
extern char	*spend;
extern char	*hspend;
extern char	nflag;
extern char	gflag;
extern char	*braelist[NBRA];
extern char	*braslist[NBRA];
extern long	tlno[NLINES];
extern unsigned char	nlno;
extern char	fname[12][40];
extern FILE	*fcode[12];
extern char	nfiles;
extern char	*cp;
extern struct	reptr ptrspace[PTRSIZE];
extern struct	reptr *rep;
extern char	respace[RESIZE];
extern struct	label ltab[LABSIZE];
extern struct	label *lab;
extern struct	label *labend;
extern char	f;
extern char	depth;
extern int	eargc;
extern char	**eargv;
extern struct	reptr **cmpend[DEPTH];
extern struct	reptr *pending;
extern char	*badp;
extern char	bad;
extern char	compfl;
extern char	bittab[];

char	*compile();
char	*ycomp();
char	*address();
char	*text();
char	*compsub();
struct	label *search();
char	*gline();
char	*place();
