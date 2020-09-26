/********************************************************/
/*							*/
/*		Q/C Compiler Version 3.2		*/
/*		  Standard Definitions			*/
/*							*/
/*     Copyright (c) 1984 Quality Computer Systems	*/
/*							*/
/*			01/03/84			*/
/********************************************************/

#define Z80	TRUE

/* Uncomment the following line to compile a reasonably
 * portable version of Q/C. The compiler will contain
 * the C version of all functions and various pieces of
 * implementation-dependent code are eliminated.

#define PORTABLE	1
*/

/*
 *	Define things needed for compiler portability
 */
#ifdef PORTABLE
#define qprintf printf
#endif
/*
 *	Look for UNIX standard buffer size definition
 */
#ifdef	BUFSIZ
#define IOBUF	BUFSIZ
#define INCBUF	BUFSIZ
#else
#define IOBUF	512
#define INCBUF	128
#endif
/*
 *	Define code generation parameters
 */
#define DEFASM		'm'	/* default assembler: 'm'-M80, 'a'-RMAC */
#define MAXREG		5	/* max number of register variables */
#define MAXINCL 	3	/* max depth for nested #include */
#define MAXIF		6	/* max depth for nested #if */
#define PREG		1	/* the primary register (HL) */
#define SREG		2	/* the secondary register (DE) */
#define WREG		2	/* the work register (DE) */
#define SFP		3	/* the stack frame pointer (BC) */
#if Z80
#define XREG		4	/* Z80 index register (IX) */
#endif
#define ARGOFFSET	4	/* offset of arguments from local space */
#define EQ		1	/* codes for binary operator routine */
#define NE		2
#define LE		3
#define GE		4
#define LT		5
#define GT		6
#define CODE		1	/* values for segtype:	code segment */
#define DATA		2	/*			data segment */
#define INITSIZE	128	/* largest array initialized without -I */
#define FILNAMSIZE	14	/* size of a CP/M filename */
#define ASMULINE	'@'	/* What '_' changes to in assembler */
#define ASMRTS		'?'	/* To set off created names with */
#define FIRSTLABEL	1	/* first compiler-generated label no. */

/*	Define peephole optimizer constants	*/

#define PEEPBUFSIZE	50

#define JUMP		1
#define JUMPTRUE	2
#define JUMPFALSE	3
#define PREGSYM 	4
#define ADDRSFP 	5

#define TESTED		1
#define REGH		2
#define REGL		3

/*
 * Define "smart" switch for compiler options which can
 * be turned on or off by special comments of the form:
 *
 *	/* $ +|-S(witch)
 *
 * from within the C program.
 */
struct _switch {
	int	is_on;		/* current switch setting */
	int	enabled;	/* was option requested in compile */
	};

/*	Define the type table		*/

struct typeinfo {
	char	 t_code;	/* int, ptr to, array of, etc. */
	int	 t_size;	/* size of one thing of this type */
	unsigned t_refs;	/* # of vars pointing to this entry */
	union baseinfo {
	    struct memtab	/* pointer to the member table when */
		*memlist;	/*	t_code == T_STRUCT or ... */
	    struct typeinfo	/* pointer to the base type such as */
		*p_type;	/*	'int' of 'pointer of int' */
		} t_base;
	struct typeinfo 	/* next type entry for this t_code */
		*t_next;
	};

#define TSSIZE		30	/* size of type parsing stack */

struct parsestack {
	struct typestack {
		char	 t_code;
		int	 t_size;
		} *curr_ptr, stack[TSSIZE];
	};

/*
 *	Type codes:
 *
 *	T_xxx is the code for the type 'xxx'.
 *	S_xxx is the size, in bytes, of one item of type 'xxx'.
 *
 *	T_NONE is assumed to be non-negative, and less than all other
 *		type codes.
 *	All types between T_NONE and T_SIMPLE are 'simple' types:
 *		they may be returned by functions.  All other types
 *		are 'non-simple' and may not be returned by functions.
 */

#define T_NONE		0

#define T_CHAR		1
#define S_CHAR		1
#define	T_SHORT		2
#define S_SHORT 	2
#define T_INT		3
#define S_INT		2
#define	T_UNSIGNED	4
#define S_UNSIGNED	2
#define T_LONG		5
/*	#define S_LONG	4	*/
#define T_PTR		6
#define S_PTR		2

#define T_SIMPLE	T_PTR

#define T_LABEL 	7		/* statement label */
#define T_STRUCT	8
#define T_UNION 	9
	/* The following are all compound types */
#define T_ARRAY 	10		/* array of ... */
#define T_FUNC		11		/* function returning ... */
#define S_FUNC		0		/* functions have no size */
#define T_MAX		12

/*	Define the symbol table parameters	*/

#define STARTGLB symtab
#define STARTLOC (symtab+(nsym-1))
#define NAMEMAX  8
#define NAMESIZE (NAMEMAX+1)

/*	Define symbol table			*/
struct st {
	char	 st_sc; 	/* storage class */
	struct typeinfo		/* variable type */
		 *st_type;
	int	 st_info;	/* stack offset for local auto	*/
				/* register # for local register*/
				/* offset for structure member	*/
				/* label # for local static	*/
				/* label # for statement label	*/
	char	 st_name[NAMESIZE];
	char	 st_idset;	/* tag/member or variable */
	};

/*	Define possible entries for "st_idset"	*/
#define ID_VAR		1
#define ID_STRUCT	2

/*	Define possible entries for "st_sc"  */

#define SC_NONE 	0
#define SC_GLOBAL	1	/* unspecified external */
#define SC_ST_GLB	2	/* static ... (global) */
#define SC_EXTERN	3	/* extern ... */
#define SC_STATIC	4	/* static ... (local) */
#define SC_AUTO 	5	/* auto ... or unspecified local */
#define SC_ARG		6	/* like auto, but for arg on stack */
#define SC_REG		7	/* register ... */
#define SC_MEMBER	8	/* member of, or tag for, structure */
				/* distinguished by st_ident */
#define SC_TYPE 	9	/* typedef ..., or struct xxx */

/*	Define possible entries for "st_info"	*/

#define DECL_GLB	1
#define DECL_LOC	2

/*	Define structure member table		*/

struct memtab {
	struct st     *p_sym;	/* pointer to symbol table entry */
	struct memtab *nextmem; /* next member or NULL */
	};

/*	Define expression operand for parser	*/

struct operand {
	char		op_load;
	struct st	*op_sym;
	int		op_val;
	struct typeinfo *op_type;
	};

/*	Define possible entries for "op_load"	*/

#define EXPRESSION	0	/* an operand whose value is in preg */
#define LOADVALUE	01	/* an op whose value needs loading */
#define LOADADDR	02	/* an op whose address needs loading */
#define LVALUE		04	/* an lvalue */
#define CONSTANT	010	/* a constant expression */
#define CONSTADDR	020	/* a constant address expression */
#define CONSTOFF	040	/* a stack var with constant offset */
/* This is the result of constant assignment statements */
#define ASGCONST	0100

/*	Define the switch/loop queue		*/

struct swq {
	int loop;
	int exit;
	};

/*	Define the switch case table		*/

struct case_table {
	int value;
	int label;
	};

/*	Define the macro (#define) pool 	*/

#define MACPTR	8
#define MACSIZE 10

/*	Define the input line			*/

#define LINESIZE 128
#define LINEMAX (LINESIZE-1)

/*	Define conditional compilation parameters	*/

#define IGNORE	0
#define PROCESS 1
#define SKIP	2

/* end of CSTDDEF.H */
or divisons with small	|
	DEC	A		; dividends			|
	JR	NZ,?udiv1	;				|
	POP	BC		;				|
	RET			;---------------