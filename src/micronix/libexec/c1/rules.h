/*
 * rules.h - Rewrite rule definitions
 */
#ifndef RULES_H
#define RULES_H

/*
 * Replacement flags, in one byte.
 *
 * The five modifiers are bits.  The register requirements are not:
 * they name which register the rule wants, and a rule cannot want two,
 * so they are a value in the top three bits rather than a bit apiece.
 * As bits they needed twelve, which meant a short, which cost the
 * table 474 bytes to say nothing more.
 */
#define RF_POW2  0x01    /* transform constant through log2 */
#define RF_NOTEQ 0x02    /* NEQ->BANG(EQ): wrap children in EQ node */
#define RF_INC1  0x04    /* increment right constant by 1 */
#define RF_TDE   0x08    /* require TARGET is DE (for RHS of binary ops) */
#define RF_SIGNL 0x10    /* require left operand has a signed width */

#define RF_REG   0xe0    /* the register requirement lives here */
#define RF_IXIY  0x20    /* require reg is IX or IY */
#define RF_BC    0x40    /* require reg is BC */
#define RF_DE    0x60    /* require reg is DE */
#define RF_HL    0x80    /* require reg is HL */
#define RF_IX    0xa0    /* require reg is IX */
#define RF_C     0xc0    /* require reg is C (low byte of BC) */
#define RF_B     0xe0    /* require reg is B (high byte of BC) */

/*
 * Path constants for compact navigation
 */
#define P_NONE   0
#define P_L      1
#define P_R      2
#define P_LL     3

/*
 * Rewrite rule.  Eight bytes: two pointers and four bytes, which is
 * what the table is multiplied by 474 times.
 *
 * The three source paths share a byte - each is one of four values, so
 * a byte apiece spent five bits saying nothing.  R() packs them, so
 * the rules themselves are written the same way they always were.
 */
/*
 * A rule's pattern, decoded.  It used to be the string it is written
 * as - "=(D(H),N):s" - parsed afresh on every attempt, and the strings
 * with their pointers were the largest single thing in the table.  The
 * shape is small and fixed: an operator, up to two operands, and at
 * most one level under either of those, so six bytes hold all of it.
 * Nothing deeper occurs, and of the operands only a left child ever
 * carries a width of its own.  DEBUG keeps the spellings in rulepat[]
 * for the trace to print.
 */
struct rule {
	char *asmtpl;   /* asm template: $L/$R/$LL/etc for interpolation */
	unsigned char op;    /* pattern letter of the root */
	unsigned char lop;   /* left operand, 0 if the root has no children */
	unsigned char rop;   /* right operand, 0 if unary */
	/*
	 * The grandchild pattern: what sits under one of the two
	 * operands.  No rule in the table has ever named both - a
	 * shape that deep on both sides has never been worth a rule -
	 * so the two share a byte and a spare bit of paths says which
	 * side it belongs to.  That bit is 682 bytes of table, which
	 * on the machine this has to fit is a real number.
	 */
	unsigned char subop;
	unsigned char sfx;   /* width | dest << 3 | left child's width << 5 */
	unsigned char rep;   /* replacement op char */
	unsigned char paths; /* lsrc | rsrc<<2 | dsrc<<4 */
	unsigned char flags;
	unsigned char destval; /* result location: R_HL, R_A, etc (0=none) */
};

/* Special pattern values */
#define P_ANY    234
#define P_NULL   255
#define P_NUM    254
#define P_POW2   253
#define P_ZERO   252
#define P_SMALL  251    /* 1-4: can use inc/dec */
#define P_MUL3   250    /* constant 3 */
#define P_MUL5   249    /* constant 5 */
#define P_MUL6   248    /* constant 6 */
#define P_MUL7   247    /* constant 7 */
#define P_MUL9   246    /* constant 9 */
#define P_MUL10  245    /* constant 10 */
#define P_MUL11  244    /* constant 11 */
#define P_MUL12  243    /* constant 12 */
#define P_MUL14  242    /* constant 14 */
#define P_MUL15  241    /* constant 15 */
#define P_MUL20  240    /* constant 20 */
#define P_MUL24  239    /* constant 24 */
#define P_MUL40  238    /* constant 40 */
#define P_EIGHT  237    /* constant 8: a shift by a whole byte */
/*
 * A comparison, of either family.
 *
 * For one pair of operands the six comparisons are two pieces of
 * code, not six: EQ, NEQ, LT and GE all subtract and then read a
 * different flag off the same subtraction, and LE and GT are the
 * same thing again with the operands the other way round.  The table
 * held a row for each, alike but for the flag it named, and that was
 * eighty-nine rows saying what the operator already says.
 *
 * So one row now, matched by P_CMP for the four that need no swap
 * and P_CMPX for the two that do, with the result named F_CC - "the
 * flag this comparison answers in", worked out from e->op and the
 * signedness by ccflag() below.
 */
/*
 * P_CMP and P_CMPX were here: 236 and 235, one row standing for the
 * four comparisons that need no swap and the two that do.  They are
 * gone, and the rows they saved are written out instead - see the
 * note at the head of the table in rules.c.  Nothing matched them
 * anywhere but at the root, and the root is what the scan indexes on.
 */

/* the suffix byte */
#define SFX_W(s)   ((s) & 7)		/* width: 0 none, b s l p */
#define SFX_D(s)   (((s) >> 3) & 3)	/* dest: 0 none, F V S */
#define SFX_LW(s)  (((s) >> 5) & 3)	/* left child's width: 0 none, b s l */
#define PW_B 1
#define PW_S 2
#define PW_L 3
#define PW_P 4
#define PD_F 1
#define PD_V 2
#define PD_S 3

#define RP_L(rp)  ((rp)->paths & 3)		/* left child source path */
#define RP_R(rp)  (((rp)->paths >> 2) & 3)	/* right child source path */
#define RP_D(rp)  (((rp)->paths >> 4) & 3)	/* data source path */
#define RP_SUBR	0x40		/* subop belongs to the right operand */
#define RP_LLOP(rp) (((rp)->paths & RP_SUBR) ? 0 : (rp)->subop)
#define RP_RLOP(rp) (((rp)->paths & RP_SUBR) ? (rp)->subop : 0)

/* Rule table (defined in rules.c) */
extern struct rule rules[];

/*
 * Where each root opcode's rules begin, or NORULE.  GENERATED by
 * mkruleidx from the table itself - see ruleidx.c and the makefile.
 */
#define NORULE 0xffff
extern unsigned short ruleidx[];
extern void ruleindex(void);
#ifdef DEBUG
extern char *rulepat[];	/* the patterns as written, for the trace */
#endif

/* Preserve patterns - subtrees matching these are not reduced */
extern unsigned char preserve[];

/*
 * The instruction sequences that templates share.  A template names one
 * with a single byte, the high bit set and the low seven the index.
 */
extern char *fragtab[];
/* longest template once its fragments are put back */
#define TPLMAX 160

#endif /* RULES_H */

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
