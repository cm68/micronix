/*
 * pass2.h - Code generator header
 */
#ifndef PASS2_H
#define PASS2_H


/* Type suffixes from AST */
#define T_BYTE    'b'
#define T_UBYTE   'B'
#define T_SHORT   's'
#define T_USHORT  'S'
#define T_LONG    'l'
#define T_ULONG   'L'
#define T_VOID    'v'

/* Type size helpers */
#define ISBYTE(t)  ((t) == T_BYTE || (t) == T_UBYTE)
#define ISWORD(t)  ((t) == T_SHORT || (t) == T_USHORT)
#define ISLONG(t)  ((t) == T_LONG || (t) == T_ULONG)
#define TSIZE(t)   (ISBYTE(t) ? 1 : ISWORD(t) ? 2 : ISLONG(t) ? 4 : 0)

/* Register codes */
#define R_B     1
#define R_C     2
#define R_BC    3
#define R_IX    4
#define R_DE    5
#define R_HL    6
#define R_A     7
#define R_IY    8
#define R_E     9       /* low byte of DE */
#define R_D     10      /* high byte of DE */
#define R_L     11      /* low byte of HL */
#define R_H     12      /* high byte of HL */

/* Flag codes (for comparison results) */
#define F_Z     16      /* zero flag */
#define F_NZ    17      /* not zero */
#define F_C     18      /* carry flag */
#define F_NC    19      /* not carry */
#define F_M     20      /* sign set: negative */
#define F_P     21      /* sign clear: non-negative */
/*
 * Not a flag: what a rule writes in place of one when it serves a
 * whole family of comparisons and the answer depends on which.  The
 * rules that name it match with 'c' or 'd' rather than with a single
 * comparison letter, and tryrule works the real flag out.
 */
#define F_CC    22      /* the flag this comparison answers in */

/* signed widths are lower case, unsigned upper */
#define ISSIGNED(w) ((w) == 'b' || (w) == 's' || (w) == 'l')

/* Global state */
extern int infd;
extern int in2fd;
extern int outfd;
extern void initOpTab(void);	/* rewrite.c: fill op_table */
extern int bcinuse(void);		/* parseast.c: BC holds a variable here */

/* AST I/O */
unsigned char read1(void);
void unread1(unsigned char c);
unsigned short read2(void);
void read4(void);
extern unsigned long val4;	/* read4 leaves its answer here */
void readS(char *buf, int size);
void nidopen(char *f1);		/* .nam sidecar (cpp -j), from the .ast name */
#ifdef DEBUG
void rulehit(int i);		/* rule coverage, host build only */
void dumphits(void);
#endif

/* Output */
void out(char *s);
void outc(char c);
void outd(int n);
void outu(int n);
void outf(char *fmt, ...);
void copyinit(void);

/* Parser */
void parse(void);

struct Expr;
void condfalse(struct Expr *e, char *lbl);	/* rewrite.c: branch chaining */
char *fmtstr(char *buf, char *fmt, ...);	/* libccc */

/* debug options */
#ifdef DEBUG
#define VERBOSE(x) (verbose & (x))
extern short verbose;
#else
#define VERBOSE(x) (0)
#endif

#endif /* PASS2_H */

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */

