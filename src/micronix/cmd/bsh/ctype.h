#
/*
 *	UNIX shell
 *
 *	S. R. Bourne
 *	Bell Telephone Laboratories
 *
 */


/* table 1 */
#define T_SUB	01
#define T_MET	02
#define	T_SPC	04
#define T_DIP	010
#define T_EOF	020
#define T_EOR	040
#define T_QOT	0100
#define T_ESC	0200

/* table 2 */
#define T_BRC	01
#define T_DEF	02
#define T_AST	04
#define	T_DIG	010
#define T_FNG	020
#define T_SHN	040
#define	T_IDC	0100
#define T_SET	0200

/*
 * The per-character names for the two tables are in ctype.c, where
 * the tables are.  Every one of these is a macro definition, and on
 * the Z80 pass0 keeps every definition of every file it reads: two
 * dozen here cost it most of a kilobyte in each source, and ctype.c
 * is the only source that ever spells them.
 */

/* abbreviations for tests */
#define _IDCH	(T_IDC|T_DIG)
#define _META	(T_SPC|T_DIP|T_MET|T_EOR)

char	_ctype1[];

/* nb these args are not call by value !!!! */
#define	space(c)	(((c)&QUOTE)==0 ANDF _ctype1[c]&(T_SPC))
#define eofmeta(c)	(((c)&QUOTE)==0 ANDF _ctype1[c]&(_META|T_EOF))
#define qotchar(c)	(((c)&QUOTE)==0 ANDF _ctype1[c]&(T_QOT))
#define eolchar(c)	(((c)&QUOTE)==0 ANDF _ctype1[c]&(T_EOR|T_EOF))
#define dipchar(c)	(((c)&QUOTE)==0 ANDF _ctype1[c]&(T_DIP))
#define subchar(c)	(((c)&QUOTE)==0 ANDF _ctype1[c]&(T_SUB|T_QOT))
#define escchar(c)	(((c)&QUOTE)==0 ANDF _ctype1[c]&(T_ESC))

char	_ctype2[];

#define	digit(c)	(((c)&QUOTE)==0 ANDF _ctype2[c]&(T_DIG))
#define fngchar(c)	(((c)&QUOTE)==0 ANDF _ctype2[c]&(T_FNG))
#define dolchar(c)	(((c)&QUOTE)==0 ANDF _ctype2[c]&(T_AST|T_BRC|T_DIG|T_IDC|T_SHN))
#define defchar(c)	(((c)&QUOTE)==0 ANDF _ctype2[c]&(T_DEF))
#define setchar(c)	(((c)&QUOTE)==0 ANDF _ctype2[c]&(T_SET))
#define digchar(c)	(((c)&QUOTE)==0 ANDF _ctype2[c]&(T_AST|T_DIG))
#define	letter(c)	(((c)&QUOTE)==0 ANDF _ctype2[c]&(T_IDC))
#define alphanum(c)	(((c)&QUOTE)==0 ANDF _ctype2[c]&(_IDCH))
#define astchar(c)	(((c)&QUOTE)==0 ANDF _ctype2[c]&(T_AST))
