/*
 * rewrite.c - table-driven expression tree rewriting
 *
 * Compact pattern language:
 *   Operators: + * - / % & | ^ < > D V L I N P _ 0 =
 *   Pattern:   op(left,right) or op(child) or op
 *   Examples:  L          matches LOCALVAR
 *              +(D(V),N)  matches PLUS(DEREF(REGVAR),NUMBER)
 *              *(_,P)     matches STAR(any,POW2)
 */
#include "pass2.h"
#include "expr.h"
#include "opcodes.h"
#include "rules.h"
#include "lexeme.h"
#include <stdlib.h>

/* Label counter for short-circuit jumps */
int labelcnt;		/* shared with lower.c */

#ifdef DEBUG
#include <stdio.h>

/*
 * Which rules have fired.
 *
 * A rule that matches nothing is worse than absent: it reads as
 * coverage that does not exist.  One sat in this table for a long time
 * emitting bit n,(iy+d), correct and unreachable, because an AND
 * reduced its left operand before any rule could see it - and every
 * test passed the whole time, because the code that ran instead was
 * right, only longer.
 *
 * Set CCC_RULEHITS to a file name and each run appends what it used.
 * Debug build only: the counters are host-side bookkeeping and the
 * Z80 build has never seen them.
 */
#define MAXRULES 1024
static unsigned long rulehits[MAXRULES];

void
rulehit(int i)
{
	if (i >= 0 && i < MAXRULES)
		rulehits[i]++;
}

void
dumphits(void)
{
	char *path = getenv("CCC_RULEHITS");
	FILE *f;
	int i;

	if (!path || !(f = fopen(path, "a")))
		return;
	/*
	 * The rule's own fields, not rulepat[].  That array is a second
	 * copy of what the table already says and it had drifted: 705
	 * spellings against 765 rules, six short before this year's
	 * additions, and once the table was sorted by root op the two
	 * no longer described the same rule at any index - the dump
	 * named a multiply for a program that has none.  Walking rules[]
	 * while indexing it also ran off the end and dropped a core.
	 *
	 * Printed from the rule itself, this cannot drift.
	 */
	for (i = 0; rules[i].op && i < MAXRULES; i++)
		fprintf(f, "%d\t%lu\top=%d lop=%d rop=%d sub=%d sfx=%d%s\n",
		    i, rulehits[i], rules[i].op, rules[i].lop, rules[i].rop,
		    rules[i].subop, rules[i].sfx,
		    rules[i].asmtpl ? "" : " (transform)");
	fclose(f);
}
#endif

/* Forward declarations */
int pmatch(struct rule *rp, Expr *e);
int pnode(unsigned char pat, unsigned char w, Expr *e);
Expr *rewrite1(Expr *e);
Expr *valtohl(Expr *e);
unsigned char baseop(unsigned char op);
int islocdesc(Expr *e);
int isdestreg(Expr *e);

/*
 * Check if expression matches any preserve pattern.
 * Returns 1 if should be preserved (not reduced).
 */
int
shouldpres(Expr *e)
{
	unsigned char *pp;
	if (!e) return 0;
	for (pp = preserve; *pp; pp++) {
		if (pnode(*pp, 0, e))
			return 1;
	}
	return 0;
}


/*
 * Sethi-Ullman labeling: compute registers needed for each node
 * With only HL and DE available:
 *   0 = already in register (INHL, INDE, some REGVAR)
 *   1 = needs one register
 *   2 = needs both HL and DE
 *   3+ = needs spill to stack
 */
void
label(Expr *e)
{
	unsigned char l, r;

	if (!e) return;

	/* Label children first (post-order) */
	label(e->left);
	label(e->right);

	/*
	 * What the children cost is wanted by most of what follows, and
	 * every case used to fetch it again through the pointer it had
	 * just finished testing.  Once, here.  A child that is not there
	 * costs one: the operand still has to be loaded from somewhere.
	 */
	l = e->left ? e->left->regs : 1;
	r = e->right ? e->right->regs : 1;

	switch (e->op) {
	/* Already in register: 0 */
	case INHL:
	case INDE:
	case INA:
	case INBC:
	case INE:
	case CODE:
		e->regs = 0;
		return;

	/* REGVAR: 0 if HL/DE, 1 if BC/IX (needs move) */
	case REGVAR:
		if (e->u.var.reg == R_HL || e->u.var.reg == R_DE)
			e->regs = 0;
		else
			e->regs = 1;
		return;

	/* LOCALVAR past the (iy+d) window (big-array bases): the
	 * address arithmetic needs HL and DE, not a free (iy+d)
	 * operand, so cost it like a computed subexpression */
	case LOCALVAR:
		if (e->u.var.off < -126 || e->u.var.off > 124) {
			e->regs = 2;
			return;
		}
		e->regs = 1;
		return;

	/* Leaves that need loading: 1 */
	case NUMBER:
	case SYM:
	case SYMREF:
	case INDEX:
		e->regs = 1;
		return;

	/* DEREF: depends on address complexity */
	case DEREF:
		e->regs = l > 1 ? l : 1;
		/*
		 * A load through anything the tree has to work out goes
		 * through HL: the machine has no ld de,(de), so the walk
		 * is HL's whatever register the value was aimed at.  Cost
		 * it like a call, which is bound the same way - the number
		 * is what decides that it goes first, before an operand is
		 * sitting in HL to be clobbered.  "g + *p" reduced g and
		 * then walked p through the register g was in.
		 *
		 * A location descriptor is exempt - the fused load rules
		 * read those in place - and so are the pre-reduced nodes
		 * a driver expansion leaves, and a constant address.
		 */
		if (e->left && !islocdesc(e->left) && !isdestreg(e->left) &&
		    e->left->op != NUMBER && e->regs < 2)
			e->regs = 2;
		return;

	/*
	 * ASSIGN: the rvalue's cost, or the lvalue's if the address
	 * costs more.
	 *
	 * The lvalue really is free when the store names its address -
	 * a global, a register pointer, (ix+d).  It is not free when
	 * the address is arithmetic, and counting it free put the two
	 * in the wrong order:
	 *
	 *	if ((rep->ad2 = p) > reend)
	 *
	 * loaded reend into hl, worked rep->ad2 out on top of it, and
	 * left both sides of the comparison claiming hl - which no rule
	 * builds code for, rightly.  Taking the larger side makes the
	 * assignment outrank the operand beside it, so it goes first
	 * and reend is loaded afterwards, into the register the store
	 * did not want.  DEREF carries the same guard, three cases up.
	 *
	 * It has to come from the child's own label.  Asking about the
	 * address's shape here does not work: label() runs through the
	 * reduction, not once ahead of it, and by the time an ASSIGN is
	 * labeled its address has often reduced to INHL already - which
	 * honestly costs nothing, the work having been done.
	 */
	case ASSIGN:
		e->regs = r ? r : 1;
		if (l > e->regs)
			e->regs = l;
		return;

	/*
	 * CALL: the result comes back in HL and nowhere else - the ABI
	 * leaves no choice - so a call cannot be evaluated into DE the
	 * way a loadable operand can.  Costing it two makes two of them
	 * add up to three, which takes the spill path and puts the first
	 * result on the stack instead of letting the second call
	 * overwrite it.
	 */
	case CALL:
		e->regs = 2;
		return;

	/* ARGNODE: each arg independent, pushed to stack */
	case ARGNODE:
		e->regs = l;
		return;

	/*
	 * Short-circuit: sides evaluated separately.  Ternary likewise:
	 * condition, then and else are all separate.
	 *
	 * Separate, but not free.  Each of these evaluates a condition
	 * into hl and branches on it, so whatever an operator above had
	 * already put in hl is gone by the time the answer arrives -
	 * and the answer arrives in hl too.  That is a call's shape,
	 * and it takes a call's cost, which is what says go first:
	 *
	 *	newcol + (state ? ue_width : 0) > sc_width
	 *
	 * loaded newcol into hl, read state into hl on top of it, and
	 * added the ternary to whatever ex de,hl turned up.
	 */
	case LAND:
	case LOR:
	case QUES:
	case TERNBRANCH:
		e->regs = l > r ? l : r;
		if (e->regs < 2)
			e->regs = 2;
		return;

	/* Unary ops: same as child, min 1 */
	case BANG:
	case NEG:
	case NOT:
		e->regs = l ? l : 1;
		return;

	/*
	 * A step whose rules can only put the answer in HL costs two, the
	 * same as a call and for the same reason.  This number is what
	 * chooses which side is worked out first, and an operand that
	 * cannot be held in DE has to go before whatever is going to sit
	 * there - saying it costs one is how the spill gets avoided by
	 * accident instead of on purpose.
	 *
	 * Costed one, "buf[pos++]" put the array's address in HL and then
	 * the step on top of it, and "i <= ++j" over a register variable
	 * did the same.
	 *
	 * A frame slot is the exception, and the table says so: those
	 * templates write through $t and $T and so land wherever they
	 * were asked to.  Everything else - a global, a register variable
	 * - names l and h outright.
	 *
	 * But the $t/$T forms only step by one.  A step by more than one
	 * - a pointer to anything wider than a byte - has no such rule:
	 * it is lowered to a compound assignment, which loads through HL
	 * whatever register it was asked for.  Costing that two, the same
	 * as the non-frame forms above, stops a parent from leaving its
	 * own operand in HL for the load to walk over.
	 */
	case PREINC:
	case POSTINC:
	case PREDEC:
	case POSTDEC:
		e->regs = l;
		if (e->regs < 2 &&
		    (e->u.incdec.amt != 1 ||
		     (e->left->op != LOCALVAR && e->left->op != INDEX)))
			e->regs = 2;
		if (!e->regs)
			e->regs = 1;
		return;

	/*
	 * A pointer plus a constant is normally free: it becomes the
	 * (ix+d) operand of whatever dereferences it, and costs what a
	 * leaf costs.  Past the 7-bit window it is not an addressing
	 * mode at all - the address has to be formed with 16-bit
	 * arithmetic, which needs HL and DE - so it costs two, the same
	 * as the far LOCALVAR above and for the same reason.
	 *
	 * Getting this wrong does not produce wrong addresses; it
	 * produces a bad choice about which side to work out first, and
	 * the damage shows up somewhere else entirely.
	 */
	case PLUS:
		if (e->left && e->left->op == REGVAR &&
		    e->right && e->right->op == NUMBER) {
			short poff = (short)e->right->u.val;

			if (poff < -126 || poff > 124) {
				e->regs = 2;
				return;
			}
		}
		if (l == r)
			e->regs = l + 1;
		else
			e->regs = l > r ? l : r;
		return;

	/* Binary ops: Sethi-Ullman formula */
	default:
		if (l == r)
			e->regs = l + 1;
		else
			e->regs = l > r ? l : r;
		/*
		 * A compound assignment holds the location while it works
		 * out the value, so it needs two whatever its operands cost
		 * - the expansion puts the address in HL and the value in
		 * DE, and the side-effecting form keeps the address on the
		 * stack and uses both on the way back.  Scoring it as one
		 * would let a parent believe a register survives it.
		 */
		if (baseop(e->op) && e->regs < 2)
			e->regs = 2;
		return;
	}
}

/*
 * The comparisons, which read their operands as values.
 */
static int
iscmpop(unsigned char op)
{
	switch (op) {
	case EQ:
	case NEQ:
	case LT:
	case GT:
	case LE:
	case GE:
		return 1;
	}
	return 0;
}

/*
 * Register assignment: top-down pass to set target registers
 * Most ops are HL-centric on Z80, so:
 *   - Binary ops: left→HL, right→DE
 *   - Unary ops: child inherits parent's target
 *   - Result may need move if parent wants different reg
 */
void
assign(Expr *e, unsigned char tgt)
{
	if (!e) return;

	e->tgt = tgt;

	switch (e->op) {
	/* Already in register: no children */
	case INHL:
	case INDE:
	case INA:
	case INBC:
	case INE:
	case CODE:
	case NUMBER:
	case SYM:
	case SYMREF:
	case LOCALVAR:
	case INDEX:
	case REGVAR:
		return;

	/* Unary: child inherits target */
	case DEREF:
	case BANG:
	case NEG:
	case NOT:
	case PREINC:
	case POSTINC:
	case PREDEC:
	case POSTDEC:
		assign(e->left, tgt);
		return;

	/* ASSIGN: lvalue doesn't need target, rvalue→tgt */
	case ASSIGN:
		/*
		 * An lvalue that names a place needs no register.  One that
		 * dereferences is an address the tree has to work out, and
		 * the store rules want it in HL - saying no target left
		 * whatever computed it with nowhere to go, and a rule whose
		 * destination follows the target then produced a node with
		 * no register at all.  "*p++ = 0" stepped the pointer, left
		 * the old value in HL, and reported it as being nowhere.
		 *
		 * An assignment's left is a location, so "*p" as an lvalue is
		 * just p and "*p++" is just the step - there is no DEREF to
		 * look for, only the question of whether the location has to
		 * be worked out before it can be stored through.
		 */
		assign(e->left, islocdesc(e->left) ? 0 : R_HL);
		assign(e->right, tgt);
		return;

	/* CALL: args go to stack, result in HL */
	case CALL:
		assign(e->left, 0);  /* function address */
		/* args handled specially */
		return;

	/* ARGNODE: each arg evaluated to HL, then pushed */
	case ARGNODE:
		assign(e->left, R_HL);
		if (e->right)
			assign(e->right, R_HL);
		return;

	/* Short-circuit: each side independent, wants flags */
	case LAND:
	case LOR:
		assign(e->left, R_HL);
		assign(e->right, R_HL);
		return;

	/* Ternary: condition→flags, branches→tgt */
	case QUES:
		assign(e->left, R_HL);  /* condition */
		if (e->right) {
			assign(e->right->left, tgt);   /* then */
			assign(e->right->right, tgt);  /* else */
		}
		return;

	case TERNBRANCH:
		assign(e->left, tgt);
		assign(e->right, tgt);
		return;

	/* Binary ops: left→HL, right→DE (Z80 is HL-centric) */
	default:
		/*
		 * A comparison reads its operands as VALUES, so an address
		 * among them has to be formed rather than left as a place
		 * to read through.  Marked here because rewrite runs bottom
		 * up: by the time the (ix+d) form is chosen the node no
		 * longer knows what asked for it.
		 */
		if (iscmpop(e->op)) {
			if (e->left)
				e->left->nored |= NR_ADDR;
			if (e->right)
				e->right->nored |= NR_ADDR;
		}
		if (e->regs >= 3 && e->right && e->right->op == NUMBER) {
			/*
			 * A constant costs no register - the rule that names
			 * it writes it into the instruction - so however dear
			 * the other side is, there is nothing here to spill
			 * for.  Sending it to HL with the left made the spill
			 * path push the left operand, rewrite a bare constant
			 * to nothing at all, and pop back a copy of what it
			 * had just pushed: "(v & m) != 0" compared the result
			 * against itself and was always equal.
			 */
			assign(e->left, R_HL);
			assign(e->right, R_DE);
		} else if (e->regs >= 3) {
			/* Need spill: both children compute to HL */
			assign(e->left, R_HL);
			assign(e->right, R_HL);
		} else if (e->regs == 2) {
			/* Need both registers */
			assign(e->left, R_HL);
			assign(e->right, R_DE);
			/*
			 * A byte comparison keeps its right operand whole so
			 * rules like Q(A,N):F can still see a literal.  Test
			 * the operand width, not the node's: a comparison
			 * yields ubyte whatever it compared, so keying off
			 * e->width preserved the right operand of every
			 * comparison and stranded the ones that needed
			 * reducing - a REGVAR never became INBC.
			 */
			if (e->left && ISBYTE(e->left->width) &&
			    (e->op == EQ || e->op == NEQ || e->op == LT ||
			     e->op == GT || e->op == LE || e->op == GE) &&
			    e->right && shouldpres(e->right)) {
				e->right->nored |= NR_NORED;
			}
		} else {
			/* Only need one, propagate target */
			assign(e->left, tgt);
			assign(e->right, tgt);
		}
		/* For ADD with NUMBER, preserve NUMBER for address rules */
		/* +(V,N)->I, +(S,N)->O, +(O,N)->O */
		if (e->op == PLUS && e->right->op == NUMBER &&
		    e->left && (e->left->op == REGVAR || e->left->op == SYM ||
		                e->left->op == SYMREF)) {
			e->right->nored |= NR_NORED;
		}
		return;
	}
}

#ifdef DEBUG
#include "debug.h"
#include <stdio.h>
#endif


/*
 * Map single char to opcode (or special pattern value)
 */

/*
 * Check if n is power of 2, return exponent or -1
 */
int
ispow2(unsigned long n)
{
	int i;
	/*
	 * Divide down rather than shift up: a constant shifted by the
	 * loop counter has twice now found untested corners of this
	 * compiler's own shift lowering, and this predicate must agree
	 * between the host build and the self-hosted one before either
	 * can be trusted.  Right shift by constant 1 is the dullest
	 * long operation there is.
	 */
	if (n == 0) return -1;
	for (i = 0; n != 1; i++) {
		if (n & 1)
			return -1;
		n >>= 1;
	}
	return i;
}

/*
 * Match pattern byte against expression
 */
int
opmatch(unsigned char pat, Expr *e)
{
	/* indexed by pat-238: P_MUL40 (238) up through P_MUL3 (250) */
	static unsigned char multab[] = {
		40, 24, 20, 15, 14, 12, 11, 10, 9, 7, 6, 5, 3
	};

	if (pat == P_ANY) return 1;
	if (pat == P_NULL) return e == NULL;
	/*
	 * Every pattern below this line names a node, so say once that
	 * there has to be one.  It was said ten times over, and this is
	 * the matcher's innermost question - asked for every pattern
	 * byte of every rule against every node.
	 */
	if (!e) return 0;
	if (pat == P_NUM) return e->op == NUMBER;
	if (pat == P_POW2) return e->op == NUMBER && ispow2(e->u.val) > 0;
	if (pat == P_ZERO) return e->op == NUMBER && e->u.val == 0;
	if (pat == P_SMALL) return e->op == NUMBER && e->u.val >= 1 && e->u.val <= 4;
	if (pat == P_EIGHT) return e->op == NUMBER && e->u.val == 8;
	if (pat >= P_MUL40 && pat <= P_MUL3)
		return e->op == NUMBER && e->u.val == multab[pat-238];
	return e->op == pat;
}
/*
 * Does one node answer to a pattern letter, and to a width if the
 * pattern named one?
 *
 * A missing node passes the width test and fails a dest test, which is
 * what the string form did when it walked off the end of a tree: the
 * width comparison was guarded by the node existing and the dest
 * comparisons were not.  Kept exactly, because rules rely on both.
 */
int
pnode(unsigned char pat, unsigned char w, Expr *e)
{
	static char wchar[5] = { 0, 'b', 's', 'l', 'p' };

	if (!opmatch(pat, e))
		return 0;
	if (w && e && (e->width | 0x20) != (wchar[w] | 0x20))
		return 0;
	return 1;
}

/*
 * Match a rule's pattern against a tree.
 *
 * This was a recursive walk over the pattern as it is written -
 * "=(D(H),N):s" - re-reading the punctuation on every attempt against
 * every node, six hundred times a step.  The shape is in the rule now:
 * an operator, up to two operands, at most one level under either.
 */
int
pmatch(struct rule *rp, Expr *e)
{
	unsigned char d;
	Expr *k;

	if (!pnode(rp->op, SFX_W(rp->sfx), e))
		return 0;
	d = SFX_D(rp->sfx);
	if (d) {
		if (!e)
			return 0;
		if (d == PD_F && e->dest != DEST_FLAGS)
			return 0;
		if (d == PD_V && e->dest != DEST_VALUE)
			return 0;
		if (d == PD_S && e->dest != DEST_NONE)
			return 0;
	}
	/*
	 * A store whose value is wanted must not match a store that
	 * produces none.  The plain forms say nothing about dest, which
	 * made them match value-context assigns ahead of their V twins -
	 * or in place of twins that did not exist - and the consumer
	 * then read a register the rule never loaded: "gs = *pb = 5"
	 * stored 5 and handed gs the POINTER, sign extended.  Refusing
	 * here either finds the V form further down or leaves the tree
	 * unreduced, and unreduced is a marker instead of a wrong
	 * number.  Transforms (a null template) still match: they
	 * rebuild the tree and the result is matched again.
	 */
	if (rp->op == ASSIGN && e->dest == DEST_VALUE &&
	    rp->destval == 0 && rp->asmtpl != NULL)
		return 0;
	if (!rp->lop)
		return 1;
	k = e ? e->left : (Expr *)0;
	if (!pnode(rp->lop, SFX_LW(rp->sfx), k))
		return 0;
	if (RP_LLOP(rp) && !pnode(RP_LLOP(rp), 0, k ? k->left : (Expr *)0))
		return 0;
	if (!rp->rop)
		return 1;
	k = e ? e->right : (Expr *)0;
	if (!pnode(rp->rop, 0, k))
		return 0;
	if (RP_RLOP(rp) && !pnode(RP_RLOP(rp), 0, k ? k->left : (Expr *)0))
		return 0;
	return 1;
}

/*
 * Get node by path string: L=left, R=right, LL=left->left, etc.
 */
Expr *
getpath(Expr *e, unsigned char p)
{
	if (p == P_NONE) return e;
	if (p == P_L) return e ? e->left : NULL;
	if (p == P_R) return e ? e->right : NULL;
	if (p == P_LL) return (e->left) ? e->left->left : NULL;
	return NULL;
}

/*
 * Emit index register name
 */
char *
idxregname(unsigned char reg)
{
	switch (reg) {
	case R_IX: return "ix";
	case R_IY: return "iy";
	}
	return "??";
}

/*
 * Interpolate asm template, emitting to output
 * $X where X is path (L, R, LL, LR, etc) interpolates that node
 * Modifiers after path:
 *   l - low byte of number
 *   h - high byte of number
 *   + - increment index offset by 1
 * Special:
 *   $t - target low register (l or e based on e->tgt)
 *   $u - target high register (h or d based on e->tgt)
 *   $T - target register pair (hl or de)
 *   %(text) - repeat text N times where N is right operand value
 */
/*
 * Put the shared sequences back.  A byte with the high bit set is an
 * index into fragtab; everything else is itself.
 *
 * This runs before any interpolation rather than during it, because
 * the repeat construct copies its span literally and would otherwise
 * hand a raw index straight to the output.  Expanding first means
 * nothing else in here has to know fragments exist.
 */
void
expandtpl(char *tpl, char *buf)
{
	unsigned char *s = (unsigned char *)tpl;
	char *d = buf, *f;
	char *end = buf + TPLMAX - 1;

	while (*s) {
		if (*s & 0x80) {
			for (f = fragtab[*s & 0x7f]; *f && d < end; f++)
				*d++ = *f;
			s++;
		} else if (d < end) {
			*d++ = *s++;
		} else {
			break;
		}
	}
	*d = 0;
}

void
emitasm(char *tpl, Expr *e)
{
	char expbuf[TPLMAX];
	char *p;
	char path[8];
	int i, offadj;
	char mod;
	Expr *n;
	long val;
	int cnt;
	char *start;

	expandtpl(tpl, expbuf);
	p = expbuf;

	while (*p) {
		/* %(text) - repeat text N times where N is right operand */
		if (*p == '%' && *(p+1) == '(') {
			p += 2;
			start = p;
			/* find closing paren */
			while (*p && *p != ')') p++;
			/* get count from right operand */
			cnt = (e->right->op == NUMBER) ?
			      (int)e->right->u.val : 1;
			/* emit the enclosed text cnt times */
			for (i = 0; i < cnt; i++) {
				char *q;
				for (q = start; q < p; q++)
					outc(*q);
			}
			if (*p == ')') p++;
			continue;
		}
		if (*p == '$') {
			p++;
			/* $$ escapes the assembler's own $, the address of
			 * the current instruction */
			if (*p == '$') {
				outc('$');
				p++;
				continue;
			}
			/*
			 * $[ and $] bracket a call to one of the 16-bit
			 * helpers, which take their second operand off the
			 * stack with a pop bc and do not put it back.  A
			 * register variable living in BC has to be saved
			 * across that, and only here is it known whether
			 * there is one - the table cannot say.
			 *
			 * Without it "t = a * a" in a function with a
			 * register variable quietly destroyed the variable,
			 * and when the variable was a loop subscript doing
			 * the multiplying, the loop did not end.
			 */
			/*
			 * Unconditional now, where it used to ask
			 * bcinuse().  What is left inside a $[ $] pair is
			 * a template that writes B ITSELF - the variable
			 * shifts, which count with djnz - and the caller's
			 * BC has to survive that whether or not a variable
			 * of this function's own lives there.
			 *
			 * It could be conditional while the prologue saved
			 * BC in every function: a function with nothing in
			 * BC had already handed the caller's copy to the
			 * frame, so clobbering it here cost nothing.  Now
			 * that savesbc() answers from the header, such a
			 * function saves nothing, and this is the only
			 * thing standing between a variable shift and the
			 * caller's register variable.
			 *
			 * It costs nothing today - no function in the tree
			 * pairs a variable shift with an empty BC - which
			 * is precisely why it must not be left to luck.
			 */
			if (*p == '[') {
				out("\tpush bc\n");
				p++;
				continue;
			}
			if (*p == ']') {
				out("\tpop bc\n");
				p++;
				continue;
			}
			/* Target register substitution */
			if (*p == 't') {
				outc(e->tgt == R_DE ? 'e' : 'l');
				p++;
				continue;
			}
			if (*p == 'u') {
				outc(e->tgt == R_DE ? 'd' : 'h');
				p++;
				continue;
			}
			if (*p == 'T') {
				out(e->tgt == R_DE ? "de" : "hl");
				p++;
				continue;
			}
			/* collect path chars */
			for (i = 0; i < 7 && (*p == 'L' || *p == 'R'); i++)
				path[i] = *p++;
			path[i] = 0;
			/* check for modifier */
			mod = 0;
			offadj = 0;
			if (*p == 'l' || *p == 'h' ||
			    *p == '2' || *p == '3' ||
			    *p == 'a' || *p == 'w' || *p == 'W' ||
			    *p == 'o' || *p == 'r') {
				mod = *p++;
			}
			while (*p == '+') {
				offadj++;
				p++;
			}
			/* Special: $RL (right child name) */
			if (path[0] == 'R' && path[1] == 'L') {
				n = (e->right) ? e->right->left : NULL;
			} else {
				/* navigate to node */
				n = e;
				for (i = 0; path[i] && n; i++) {
					if (path[i] == 'L') n = n->left;
					else n = n->right;
				}
			}
			/* emit based on node type */
			if (n) {
				if (n->op == NUMBER) {
					val = n->u.val;
					/* l h 2 3 select the four bytes */
					/*
					 * w and W are the two WORDS of a
					 * long, as l h 2 3 are its bytes.
					 * Without them a long constant had
					 * to be laid down a byte at a time
					 * through (hl) - fourteen bytes
					 * where two ld (nn),hl are twelve.
					 */
					if (mod == 'w') val = val & 0xffff;
					else if (mod == 'W')
						val = (val >> 16) & 0xffff;
					else if (mod == 'l') val = val & 0xff;
					else if (mod == 'h') val = (val >> 8) & 0xff;
					else if (mod == '2') val = (val >> 16) & 0xff;
					else if (mod == '3') val = (val >> 24) & 0xff;
					/*
					 * 'a' - an ADDRESS.  Sixteen bits
					 * with no sign to them, because
					 * outd() spells a word signed and
					 * asz will not take "ld (-4096),hl".
					 * offadj reaches the second word of
					 * a long, as it does for a SYMREF.
					 */
					if (mod == 'a') {
						outu((int)(val + offadj));
						continue;
					}
					outd(val);
				} else if (n->op == SYMREF) {
					/* honour $L+ here too - without it a
					 * template reaching for the second
					 * word of a long silently addressed
					 * the first one again */
					out(n->u.symref.name);
					val = n->u.symref.off + offadj;
					if (val != 0) {
						/*
						 * "+80" is only right after a
						 * name; a nameless symref IS
						 * the address and prints bare.
						 */
						if (val > 0 && n->u.symref.name[0])
							outc('+');
						outd(val);
					}
				} else if (n->op == INDEX) {
					/* o and r split it into the offset and
					 * the register, for templates that
					 * have to do the arithmetic themselves
					 * rather than let (ix+d) do it */
					if (mod == 'o') {
						outd(n->u.var.off + offadj);
					} else if (mod == 'r') {
						out(idxregname(n->u.var.reg));
					} else {
						out(idxregname(n->u.var.reg));
						val = n->u.var.off + offadj;
						if (val >= 0) outc('+');
						outd(val);
					}
				} else if (n->op == LOCALVAR) {
					/* raw frame offset, for address
					 * arithmetic templates */
					outd(n->u.var.off + offadj);
				} else {
					/* template navigated to a node the
					 * emitter can't print - make the
					 * assembler flag it loudly */
					outf("?op%d?", n->op);
				}
			} else {
				out("?null?");
			}
		} else {
			outc(*p++);
		}
	}
}


/*
 * Nearly every rewrite that has emitted its own code ends the same way:
 * make a node standing for the answer in HL, give it the destination
 * the original was asked for, and free what it replaced.  Eleven copies
 * of four statements became this.
 *
 * op is what to call the answer: INHL where the rewrite names the
 * register outright, CODE where it leaves the typing to the pass below
 * that turns CODE into INHL/INDE/INA by its register.
 *
 * The seven rewrites that build their node in a branch - a different
 * register in each arm - keep their own tails.  Sharing just the last
 * three statements with them was tried and cost two bytes: passing two
 * pointers to a function is dearer than the stores it saves.
 */
Expr *
donehl(Expr *e, unsigned char op)
{
	Expr *n = mkcode(e->width, R_HL);

	n->op = op;
	n->dest = e->dest;
	freeexpr(e);
	return n;
}

/*
 * The register each RF_REG value demands, indexed by the field shifted
 * down.  Slot 1 is RF_IXIY, which accepts either index register and is
 * tested by hand; slot 0 is no requirement at all and never reached.
 */
static char regwant[8] = {
	0, 0, R_BC, R_DE, R_HL, R_IX, R_C, R_B
};

/*
 * The flag a comparison answers in, once its operands have been
 * subtracted.  EQ and NEQ read the zero bit whatever the signedness;
 * the orderings read carry when unsigned and sign when the template
 * has folded overflow into it.  LE and GT are not here: a rule that
 * serves them has already swapped the operands, which turns them into
 * GE and LT, and it is those the caller asks about.
 */
unsigned char
ccflag(unsigned char op, int signed_)
{
	switch (op) {
	case EQ:  return F_Z;
	case NEQ: return F_NZ;
	case LT:  return signed_ ? F_M : F_C;
	case GE:  return signed_ ? F_P : F_NC;
	case LE:  return signed_ ? F_P : F_NC;	/* swapped: reads as GE */
	case GT:  return signed_ ? F_M : F_C;	/* swapped: reads as LT */
	}
	return F_NZ;
}

/*
 * Try to apply a rule
 */
Expr *
tryrule(struct rule *rp, Expr *e)
{
	Expr *n, *src, *num, *lc, *rc;
	char reg;
	int off;
	int shift, changed;
	unsigned char newop, oldop;

	/* Match pattern */
	if (!pmatch(rp, e))
		return NULL;

	/*
	 * Check the register constraint.  A rule names at most one, so
	 * this is a value to compare rather than a set of bits to test.
	 * RF_REG is a three bit field, so the register a rule demands is
	 * a lookup rather than a ladder of seven comparisons - each of
	 * which used to reload src->u.var.reg from memory.  RF_IXIY is
	 * the one that accepts two, and stays written out.
	 */
	if (rp->flags & RF_REG) {
		unsigned char want = rp->flags & RF_REG;
		char have;

		src = getpath(e, RP_D(rp));
		if (!src)
			return NULL;
		have = src->u.var.reg;
		if (want == RF_IXIY) {
			if (have != R_IX && have != R_IY)
				return NULL;
		} else if (have != regwant[want >> 5])
			return NULL;
	}

	/* Sign-bit tests are only valid on a signed operand */
	if ((rp->flags & RF_SIGNL) && (!e->left || !ISSIGNED(e->left->width)))
		return NULL;

	/*
	 * Some forms only apply when the result is wanted in DE - a byte
	 * heading for E as the right operand of a binary op, rather than
	 * for A as the left.
	 */
	if ((rp->flags & RF_TDE) && e->tgt != R_DE)
		return NULL;

#ifdef DEBUG
	if (VERBOSE(V_RULES))
		fprintf(stderr, "rewrite: rule %d (op=%d) -> %c\n",
		    (int)(rp - rules), rp->op, rp->rep);
#endif

	oldop = e->op;
	newop = rp->rep;
	changed = 0;

	/* Get replacement children */
	lc = RP_L(rp) ? getpath(e, RP_L(rp)) : NULL;
	rc = RP_R(rp) ? getpath(e, RP_R(rp)) : NULL;

	/* Handle NEQ -> BANG(EQ) - caller must rewrite result */
	if (rp->flags & RF_NOTEQ) {
		Expr *eq = mkbinary(EQ, e->width, e->left, e->right);
		eq->dest = e->dest;
		e->op = BANG;
		e->dest = DEST_FLAGS;
		e->left = eq;
		e->right = NULL;
		return e;  /* tagged for re-rewrite */
	}

	/* Handle INDEX specially */
	if (newop == INDEX) {
		if (e->op == LOCALVAR) {
			reg = e->u.var.reg ? e->u.var.reg : R_IY;
			off = e->u.var.off;
		} else {
			src = getpath(e, RP_D(rp));
			reg = src ? src->u.var.reg : R_IY;
			num = e->right;
			off = num ? (short)num->u.val : 0;
			/* If source is INDEX, combine offsets */
			if (src->op == INDEX)
				off += src->u.var.off;
		}
		/*
		 * (iy+d) displacements are 7-bit signed; leave headroom
		 * for +3 word/long adjustments.  Out-of-window accesses
		 * (big arrays) must go through address arithmetic, so
		 * refuse the INDEX form and let other rules apply.
		 */
		if (off < -126 || off > 124)
			return NULL;
		/*
		 * An INDEX is a place to read through, not a value.  A
		 * comparison against an ADDRESS - "&p->c == g" - wants the
		 * address itself worked out, and answering with (ix+d)
		 * handed the comparison an operand it has no rule for: the
		 * test emitted a marker and then branched on whatever the
		 * preceding load had left in the flags.
		 *
		 * Refusing here falls through to the PLUS(REGVAR,NUMBER)
		 * -> CODE rule below - the same address arithmetic the
		 * far-member case does.  Only a comparison sets NR_ADDR, so
		 * "p->c" keeps ld l,(ix+4): deciding this on e->tgt instead
		 * looked equivalent and was not, because a DEREF passes its
		 * target down to the address underneath it.  Every struct
		 * read in the tree turned into push ix/pop hl/add and the
		 * suite went from one failure to twenty-three.
		 */
		if ((e->nored & NR_ADDR) && oldop == PLUS && e->left &&
		    e->left->op == REGVAR)
			return NULL;
		n = mkindex(e->width, reg, off);
		n->tgt = e->tgt;
		n->dest = e->dest;
		freeexpr(e);
		return n;
	}

	/* INDEX -> CODE: the effective address as a value.  add hl only
	 * takes bc/de/hl/sp, so the index register goes through the
	 * stack and the displacement is added the ordinary way.
	 *
	 * Only for a node that was HANDED a target: an operand a parent
	 * will dereference carries none, and converting it would steal
	 * every (reg+d) form before the addressing rules saw it. */
	if (newop == CODE && oldop == INDEX) {
		if (!e->tgt)
			return NULL;
		off = e->u.var.off;
		reg = e->u.var.reg ? e->u.var.reg : R_IY;
		if (e->tgt == R_DE) {
			/* sibling value lives in HL - preserve it */
			outf("\tpush hl\n\tpush %s\n\tpop hl\n\tld de,%d\n\tadd hl,de\n\tex de,hl\n\tpop hl\n",
			    idxregname(reg), off);
			n = mkcode(e->width, R_DE);
		} else {
			outf("\tpush %s\n\tpop hl\n", idxregname(reg));
			if (off)
				outf("\tld de,%d\n\tadd hl,de\n", off);
			n = mkcode(e->width, R_HL);
		}
		n->dest = e->dest;
		freeexpr(e);
		return n;
	}

	/* Far LOCALVAR -> CODE: form the frame address with 16-bit
	 * arithmetic (big-array bases sit past the (iy+d) window) */
	if (newop == CODE && oldop == LOCALVAR) {
		off = e->u.var.off;
		if (e->tgt == R_DE) {
			/* sibling value lives in HL - preserve it */
			outf("\tpush hl\n\tpush iy\n\tpop hl\n\tld de,%d\n\tadd hl,de\n\tex de,hl\n\tpop hl\n",
			    off);
			n = mkcode(e->width, R_DE);
		} else {
			outf("\tpush iy\n\tpop hl\n\tld de,%d\n\tadd hl,de\n",
			    off);
			n = mkcode(e->width, R_HL);
		}
		n->dest = e->dest;
		freeexpr(e);
		return n;
	}

	/*
	 * PLUS(REGVAR, NUMBER) -> CODE: a member past the (ix+d)
	 * window.  Same shape as the far LOCALVAR above, with the
	 * pointer's own register in place of the frame pointer.  Only
	 * reached when the INDEX conversion refused the displacement.
	 */
	if (newop == CODE && oldop == PLUS && e->left &&
	    e->left->op == REGVAR && e->right && e->right->op == NUMBER) {
		reg = e->left->u.var.reg ? e->left->u.var.reg : R_IX;
		off = (short)e->right->u.val;
		if (e->tgt == R_DE) {
			/* sibling value lives in HL - preserve it */
			outf("\tpush hl\n\tpush %s\n\tpop hl\n\tld de,%d\n\tadd hl,de\n\tex de,hl\n\tpop hl\n",
			    idxregname(reg), off);
			n = mkcode(e->width, R_DE);
		} else {
			outf("\tpush %s\n\tpop hl\n\tld de,%d\n\tadd hl,de\n",
			    idxregname(reg), off);
			n = mkcode(e->width, R_HL);
		}
		n->dest = e->dest;
		freeexpr(e);
		return n;
	}

	/* Handle NUMBER -> CODE: emit load instruction */
	if (newop == CODE && oldop == NUMBER) {
		long val = e->u.val;
		char w = e->width;
		if (w == 'b' || w == 'B') {
			/* Byte: load into A, or E if target is DE (for RHS of compare) */
			if (e->tgt == R_DE) {
				outf("\tld e,%d\n", (int)val);
				n = mkcode('b', R_E);
			} else {
				outf("\tld a,%d\n", (int)val);
				n = mkcode('b', R_A);
			}
		} else if (w == 'l' || w == 'L') {
			/* Long: load into HLDE (DE=low, HL=high) */
			outf("\tld de,%d\n\tld hl,%d\n",
			    (int)(val & 0xffff), (int)((val >> 16) & 0xffff));
			n = mkcode(w, R_HL);
		} else if (e->tgt == R_DE) {
			/* a word constant honours its target too - as the
			 * right operand of a binary op it belongs in DE, and
			 * putting it in HL would land on the left one */
			outf("\tld de,%d\n", (int)val);
			n = mkcode(e->width, R_DE);
		} else {
			outf("\tld hl,%d\n", (int)val);
			n = mkcode(e->width, R_HL);
		}
		n->dest = e->dest;
		freeexpr(e);
		return n;
	}

	/* Handle SYMREF: SYM, SYM+NUMBER, or SYMREF+NUMBER */
	if (newop == SYMREF) {
		char *name;
		short soff;
		if (e->op == SYM) {
			/* bare SYM -> SYMREF+0 */
			name = e->u.name;
			soff = 0;
		} else if (e->left->op == SYMREF) {
			/* SYMREF +/- NUMBER -> combine offsets */
			name = e->left->u.symref.name;
			soff = e->left->u.symref.off;
			if (e->right) {
				if (e->op == MINUS)
					soff -= (short)e->right->u.val;
				else
					soff += (short)e->right->u.val;
			}
		} else {
			/* SYM + NUMBER */
			name = e->left->u.name;
			soff = e->right ? (short)e->right->u.val : 0;
		}
		n = mksymref(name, soff);
		freeexpr(e);
		return n;
	}

	/* Reuse node, change op */
	if (newop != oldop)
		changed = 1;
	e->op = newop;
	if (lc != e->left || rc != e->right) {
		changed = 1;
		/* Detach children we're keeping */
		if (lc == e->left) e->left = NULL;
		if (lc == e->right) e->right = NULL;
		if (rc == e->left) e->left = NULL;
		if (rc == e->right) e->right = NULL;
		/* Free unused subtrees */
		freeexpr(e->left);
		freeexpr(e->right);
		e->left = lc;
		e->right = rc;
	}

	/* Transform POW2 to shift amount */
	if ((rp->flags & RF_POW2) && e->right->op == NUMBER) {
		shift = ispow2(e->right->u.val);
		if (shift > 0) {
			e->right->u.val = shift;
			changed = 1;
		}
	}

	/* Increment constant by 1 (for GT->GE, LE->LT transforms) */
	if ((rp->flags & RF_INC1) && e->right->op == NUMBER) {
		e->right->u.val++;
		changed = 1;
	}

	/* Emit assembly and create CODE node if template present */
	if (rp->asmtpl) {
		unsigned char dest;
		emitasm(rp->asmtpl, e);
		/* Use rule's destval, or target register if destval is 0 */
		dest = rp->destval ? rp->destval : e->tgt;
		/*
		 * A rule serving a whole family says F_CC and the operator
		 * says which flag.  The subtraction is the same for all
		 * four of the unswapped ones - only which bit is read
		 * differs - and the two swapped ones read the same bits as
		 * their mirrors, since swapping the operands is what makes
		 * "a <= b" into "b >= a".  Signed comparisons land in the
		 * sign flag instead, the overflow having been folded into
		 * it by the template.
		 */
		if (dest == F_CC)
			dest = ccflag(oldop, (rp->flags & RF_SIGNL) != 0);
		n = mkcode(e->width, dest);
		n->dest = e->dest;
		freeexpr(e);
		return n;
	}

	return changed ? e : NULL;
}

/*
 * Check if op is commutative
 */
int
iscommut(unsigned char op)
{
	switch (op) {
	case PLUS: case STAR: case AND: case OR: case XOR:
	case EQ: case NEQ: case LAND: case LOR:
		return 1;
	}
	return 0;
}

/*
 * Normalize: put constants on right for commutative ops
 * For relational ops, swap operands AND flip operator
 */
void
normalize(Expr *e)
{
	Expr *t;
	if (!e || !e->left || !e->right) return;
	/* Commutative ops: just swap operands */
	if (iscommut(e->op)) {
		if (e->left->op == NUMBER && e->right->op != NUMBER) {
			t = e->left;
			e->left = e->right;
			e->right = t;
		}
		return;
	}
	/* Relational ops: swap operands AND flip operator */
	/* 0 < x becomes x > 0, 0 > x becomes x < 0, etc */
	if (e->left->op == NUMBER && e->right->op != NUMBER) {
		switch (e->op) {
		case LT:
			t = e->left; e->left = e->right; e->right = t;
			e->op = GT;
			break;
		case GT:
			t = e->left; e->left = e->right; e->right = t;
			e->op = LT;
			break;
		case LE:
			t = e->left; e->left = e->right; e->right = t;
			e->op = GE;
			break;
		case GE:
			t = e->left; e->left = e->right; e->right = t;
			e->op = LE;
			break;
		}
	}
}

/*
 * Normalize the whole tree before it is labeled.  step() normalizes
 * each node too, but by the time it runs the children have already
 * been reduced into concrete registers, so an operand swap there
 * leaves the operand sitting in the wrong one.
 */
void
normtree(Expr *e)
{
	if (!e) return;
	normalize(e);
	/*
	 * A byte tested against a byte-range mask is a byte test.  C
	 * promoted the operand and emitOperand said so with a WIDEN or
	 * SEXT, but in flag context the promotion buys nothing: a mask
	 * under 256 zeroes the high byte whatever the extension put
	 * there - zeroes for WIDEN, the sign for SEXT, either way
	 * nothing survives the AND.  Dropping the widening is what lets
	 * the byte rules see the test at all, and one of those rules is
	 * the bit instruction.  "flags & TF_X" is the most repeated
	 * expression in both compilers, and every one of them was
	 * eleven instructions of word arithmetic.
	 */
	if (e->op == AND && e->dest == DEST_FLAGS &&
	    e->right->op == NUMBER &&
	    (e->right->u.val & ~0xffL) == 0 &&
	    e->left && (e->left->op == WIDEN || e->left->op == SEXT) &&
	    e->left->left && ISBYTE(e->left->left->width)) {
		Expr *w = e->left;

		e->left = w->left;
		w->left = NULL;
		freeexpr(w);
		e->width = e->left->width;
		e->right->width = e->left->width;
	}
	/*
	 * What an assignment stores is wanted as a value whatever is done
	 * with the assignment itself, and "i = k = 5" needs the inner one
	 * to know it: a store rule that writes straight to memory leaves
	 * nothing for the outer assignment to copy, and only the value
	 * context tells it to pay for a register.
	 */
	/*
	 * What a child is wanted for, decided by what the parent is.
	 *
	 * This was five standalone tests walking the same handful of
	 * operators, the last of them naming everything the other four
	 * did not - so an operator was compared against as many as nine
	 * constants to be told that what it operates on is a value.  One
	 * dispatch says it.  A node's destination starts as DEST_NONE and
	 * only a statement root is ever told otherwise, so an operand
	 * nobody had spoken for read as discarded, and a rule asking for
	 * statement context with :S would match it: "uc++ != 5" took the
	 * statement form of the step, which increments the byte in memory
	 * and leaves HL holding its address, then compared the address
	 * against five.
	 */
	switch (e->op) {
	/*
	 * An assignment's right side is the value.  Its left is a place,
	 * unless the place has to be worked out - an address is a value
	 * too: "*p++ = 7" wants the pointer from before the step.  Read
	 * as discarded, the step took the form that does not bother
	 * producing one, and the store went through the pointer after the
	 * step instead of before.
	 */
	case ASSIGN:
		if (e->right->dest == DEST_NONE)
			e->right->dest = DEST_VALUE;
		if (e->left && !islocdesc(e->left) &&
		    e->left->dest == DEST_NONE)
			e->left->dest = DEST_VALUE;
		break;
	/*
	 * A conversion is transparent: what it converts is wanted exactly
	 * as much as the conversion is.  Without this the destination
	 * stopped at the widening and everything under it read as
	 * discarded - so "r = g++" took the form of the step that throws
	 * its value away, and then widened the address it had left in HL.
	 *
	 * Where the conversion itself is wanted for nothing, the child is
	 * still a value: it used to reach that by falling through to the
	 * general arm below, which is the step this arm now does at once.
	 */
	case SEXT:
	case WIDEN:
		if (e->left->dest == DEST_NONE)
			e->left->dest = e->dest == DEST_NONE ?
			    DEST_VALUE : e->dest;
		break;
	/*
	 * A comma's left really is discarded - that is what it is for -
	 * but its right is the value, and is wanted exactly as much as
	 * the comma itself.
	 */
	case COMMA:
		if (e->right->dest == DEST_NONE)
			e->right->dest = e->dest;
		break;
	/* a step is an assignment wearing an operator's clothes */
	case PREINC:
	case POSTINC:
	case PREDEC:
	case POSTDEC:
		break;
	/* everything else operates on values, the compound assignments
	 * excepted for the same reason as the steps */
	default:
		if (baseop(e->op))
			break;
		if (e->left && e->left->dest == DEST_NONE)
			e->left->dest = DEST_VALUE;
		if (e->right && e->right->dest == DEST_NONE)
			e->right->dest = DEST_VALUE;
		break;
	}
	normtree(e->left);
	normtree(e->right);
}

/*
 * Flip flag code: Z<->NZ, C<->NC
 */
unsigned char
flipflag(unsigned char f)
{
	switch (f) {
	case F_Z:  return F_NZ;
	case F_NZ: return F_Z;
	case F_C:  return F_NC;
	case F_NC: return F_C;
	case F_M:  return F_P;
	case F_P:  return F_M;
	}
	return f;
}

int
isflag(unsigned char r)
{
	return r >= F_Z && r <= F_P;
}

/*
 * Turn a condition into the number 0 or 1 in A, for when a comparison
 * was wanted as a value rather than as a branch.
 *
 * Carry is nearly free: ld a,0 leaves the flags alone, so adc a,a adds
 * the carry into a cleared A - three bytes, and one more for a ccf to
 * take the inverse.  The others need a branch over an inc, and jr can
 * only test NZ/Z/NC/C, so sign has to go through jp.
 */
void
matflag(unsigned char r)
{
	switch (r) {
	case F_C:
		out("\tld a,0\n\tadc a,a\n");
		return;
	case F_NC:
		out("\tccf\n\tld a,0\n\tadc a,a\n");
		return;
	/* jr is 2 bytes, so $+3 clears the inc; jp is 3, so $+4 does */
	case F_Z:
		out("\tld a,0\n\tjr nz,$+3\n\tinc a\n");
		return;
	case F_NZ:
		out("\tld a,0\n\tjr z,$+3\n\tinc a\n");
		return;
	case F_M:
		out("\tld a,0\n\tjp p,$+4\n\tinc a\n");
		return;
	case F_P:
		out("\tld a,0\n\tjp m,$+4\n\tinc a\n");
		return;
	}
}

/*
 * The index lives in ruleidx.c, which mkruleidx writes from the table
 * at build time - see the note at the head of that program.  It was
 * built here at startup once; a generated one cannot disagree with
 * the table and cannot be filed wrong, because the same pass that
 * writes it refuses a table whose runs are broken.
 */

/*
 * Apply one rewrite step to node (not children)
 * Returns new node if changed, NULL if no change
 */
Expr *
step(Expr *e)
{
	struct rule *rp;
	Expr *n;
	unsigned short first;

	if (!e) return NULL;

	normalize(e);

	/* BANG(CODE) in flag context: flip the flag */
	if (e->op == BANG && e->dest == DEST_FLAGS &&
	    e->left->op == CODE) {
		n = e->left;
		n->u.var.reg = flipflag(n->u.var.reg);
		n->dest = e->dest;
		e->left = NULL;
		freeexpr(e);
		return n;
	}

	/*
	 * EQ(x, 0) in flag context: testing x is cheaper than comparing
	 * against zero, but the test is true when x is nonzero, so this
	 * is !x - the BANG below flips the flag once x has been reduced.
	 */
	if (e->op == EQ && e->dest == DEST_FLAGS &&
	    e->right->op == NUMBER && e->right->u.val == 0) {
		n = e->left;
		n->dest = DEST_FLAGS;
		e->left = NULL;
		freeexpr(e);
		n = mkunary(BANG, 'b', n);
		n->dest = DEST_FLAGS;
		return n;
	}

	/*
	 * Shift by a count only known at runtime.  The Z80 has no
	 * variable shift, so it loops - add hl,hl shifts left by one,
	 * srl/sra h with rr l shifts right, arithmetic or logical
	 * depending on the sign of the value.
	 *
	 * The count arrives in A already, or in E as a byte, or in DE as
	 * a word of which only the low byte can matter; E is the low half
	 * of DE, so the last two are the same load.  The zero guard is
	 * not optional: C defines "x << 0" as x, and the loop body would
	 * otherwise run once.
	 */
	if ((e->op == LSHIFT || e->op == RSHIFT) &&
	    e->left && (e->left->op == INHL || e->left->op == INBC ||
	     (e->left->op == NUMBER && !ISBYTE(e->width))) &&
	    e->right &&
	    (e->right->op == INA || e->right->op == INE ||
	     e->right->op == INDE)) {
		/*
		 * The value shifts in HL, so a register variable comes over
		 * first - the same move the constant-count rules make with
		 * T_BC_HL.  Without it "v <<= n" on a variable the allocator
		 * had put in BC matched nothing here and had no rule either,
		 * and emitted nothing at all.
		 *
		 * A constant left never reduces - it stays a NUMBER for the
		 * ",N)" rules - so "1 << i" reached this point with nothing
		 * in HL and matched nothing, silently.  ispow2 in this very
		 * file is built from that shape, so the self-hosted c1
		 * answered "not a power of two" to everything and multiplied
		 * by 2 with a helper call.
		 */
		if (e->left->op == INBC)
			out("\tld l,c\n\tld h,b\n");
		else if (e->left->op == NUMBER)
			outf("\tld hl,%d\n", (int)e->left->u.val);
		if (e->right->op != INA)
			out("\tld a,e\n");
		/*
		 * $ is the address of the instruction it appears in, so the
		 * displacements below are counted from each jr itself:
		 *
		 *   J+0  jr z    2   skip the loop entirely
		 *   J+2  body    1 for add hl,hl, 4 for the CB-prefixed pair
		 *   J+3  dec a   1
		 *   J+4  jr nz   2   back to the body at J+2
		 *   J+6  ...
		 *
		 * Recount both if the body ever changes size.
		 */
		out("\tor a\n");
		if (e->op == LSHIFT)
			out("\tjr z,$+6\n\tadd hl,hl\n");
		else if (ISSIGNED(e->width))
			out("\tjr z,$+9\n\tsra h\n\trr l\n");
		else
			out("\tjr z,$+9\n\tsrl h\n\trr l\n");
		out("\tdec a\n");
		out(e->op == LSHIFT ? "\tjr nz,$-2\n" : "\tjr nz,$-5\n");

		return donehl(e, INHL);
	}

	/* CALL is handled up in rewrite1() - args must be pushed one at a
	 * time, before the children are batch-rewritten. */

	/* Long unary operations */
	if ((e->width == 'l' || e->width == 'L') && e->left) {
		/* Long complement: ~HL':HL */
		if (e->op == NOT && e->left->op == INHL) {
			out("\tcall qcom\n");
			return donehl(e, CODE);
		}
		/*
		 * Long negation.  Twelve bytes inline against three for a
		 * call, so it is a call now: qneg is in the divide, which
		 * needs it anyway to put the sign back on a quotient.
		 */
		if (e->op == NEG && e->left->op == INHL) {
			out("\tcall qneg\n");
			return donehl(e, CODE);
		}
		/*
		 * Long shift by a count worked out at runtime.  The three
		 * forms - left, right signed, right unsigned - ask the same
		 * question of the tree and differ only in which helper they
		 * call.  The enclosing test has already established that the
		 * width is long, so 'l' against 'L' is the sign.  The count
		 * arrives in A, which is where the helpers want it - they
		 * count in B', so there is no ld b,a and no BC to protect.
		 */
		if ((e->op == LSHIFT || e->op == RSHIFT) &&
		    e->left->op == INHL && e->right->op == INA) {
			out(e->op == LSHIFT ? "\tcall qshl\n" :
			    e->width == 'l' ? "\tcall qsar\n" : "\tcall qshr\n");
			return donehl(e, CODE);
		}
	}

	/* CODE -> INHL/INDE/INBC/INA/INE: convert to typed register nodes */
	if (e->op == CODE) {
		unsigned char reg = e->u.var.reg;
		if (reg == R_HL) e->op = INHL;
		else if (reg == R_DE) e->op = INDE;
		else if (reg == R_BC) e->op = INBC;
		else if (reg == R_A) e->op = INA;
		else if (reg == R_E) e->op = INE;
		/*
		 * IX has no IN- node: the register is a variable's home,
		 * so the node that says "the value is in IX" is a REGVAR
		 * naming it - the same node valtohl and islocdesc already
		 * understand.  Without this a result left in IX stayed a
		 * bare CODE, which no operand pattern matches: the walk in
		 * malloc's coalesce loop
		 *
		 *	while (!testbusy(*(q = p->ptr)))
		 *
		 * reduced the assignment, then found no rule for (that + 2)
		 * and emitted neither the flag byte load nor the AND.  The
		 * jp nz below it tested whatever flags happened to be set,
		 * so the busy test never re-evaluated and the loop spun.
		 */
		else if (reg == R_IX) {
			e->op = REGVAR;
			e->u.var.off = 0;
		}
		else if (isflag(reg) && e->dest != DEST_FLAGS) {
			/* the condition was wanted as a number, not a jump */
			matflag(reg);
			e->op = INA;
			e->width = 'B';
			e->u.var.reg = R_A;
		}
		else goto no_regconv;
		return e;
	}
no_regconv:

	/*
	 * The table is sorted by root op and every rule is rooted at a
	 * real opcode, so the rules that can match this node are one
	 * run of it, and ruleidx says where that run starts.  What used
	 * to be a walk of all 760 rules asking each whether it was the
	 * right kind is now a subscript and a run of the ones that are:
	 * 11,996,703 rule examinations over nm.c became 454,914.
	 *
	 * Most of that is nodes with no rules at all - better than half
	 * of what arrives here is an already-reduced form like CODE or
	 * INBC, and each of those used to walk the whole table to learn
	 * there was nothing for it.  Now it is one failed subscript.
	 */
	first = ruleidx[e->op];
	if (first == NORULE)
		return NULL;

	for (rp = &rules[first]; rp->op == e->op; rp++) {
		n = tryrule(rp, e);
		if (n) {
#ifdef DEBUG
			rulehit(rp - rules);
#endif
			return n;
		}
	}
	return NULL;  /* no change */
}

/*
 * Rewrite node: depth-first, fixed-point at each level
 * ARGNODE handled specially: right chain processed after push
 */
/*
 * Does this node name a location outright, rather than work out an
 * address that a store then has to go through?
 */
