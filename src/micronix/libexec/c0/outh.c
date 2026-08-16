/*
 * AST-writer helpers, split from outast.c so no single translation
 * unit carries both the emitters and their support machinery.
 */

#include "p1core.h"
#include "p1expr.h"
#include "p1type.h"
#include "p1name.h"
#include "p1stmt.h"
#include "p1lex.h"

extern struct local *curFuncLocals;

/*
 * Look up a local variable in phase 1's captured locals, to get the
 * frame offset and register decided there.
 *
 * By name is not enough.  Every local of a function is in one list
 * now, so a name declared in a nested block sits beside the one it
 * shadows and both answer to the same string - the L<n> renaming is
 * for what gets emitted, not for what is looked up.  Matching on the
 * name alone returned whichever came first, so
 *
 *	short v; v = 1; { short v; v = 100; } return v;
 *
 * put both of them in the outer one's register.
 *
 * The level and the block say which is which: two variables of the
 * same name cannot be declared in the same block.
 */
struct local *
findInLocals(struct name *want)
{
	struct local *n;
	for (n = curFuncLocals; n; n = n->next) {
		if (n->id == want->id &&
		    n->level == want->level &&
		    n->blkid == want->w.r.blkid)
			return n;
	}
	return NULL;
}

/*
 * Assignment operators: plain = plus the ten compound forms.
 */
int
isAssignOp(unsigned char op)
{
	switch (op) {
	case ASSIGN:
	case PLUSEQ:
	case SUBEQ:
	case MULTEQ:
	case DIVEQ:
	case MODEQ:
	case RSHIFTEQ:
	case LSHIFTEQ:
	case ANDEQ:
	case OREQ:
	case XOREQ:
		return 1;
	}
	return 0;
}


/*
 * Check if expression is a SYM that maps to a REGVAR.
 * Returns the register number if so, 0 otherwise.
 */
char
isRegvar(struct expr *e)
{
	struct name *np;
	struct local *local;
	if (!e || e->op != SYM)
		return 0;
	np = (struct name *)e->var;
	/* a static is never in a register - see canAlloc */
	if (np->level > 1 && !(np->sclass & (SC_EXTERN | SC_STATIC))) {
		local = findInLocals(np);
		return local ? local->reg : np->w.r.reg;
	}
	return 0;
}

/*
 * Does this chain of DEREFs stand on a register variable?
 *
 * The lvalue flag has to ride down the WHOLE spine, not one level:
 * "*(*h)->p = c" with h in a register is three fetches deep, and
 * keeping only the first left its spelling identical to the
 * two-deep "*q->p = c" - pass2 stored one indirection short, at the
 * struct base.  Every DEREF on a register-rooted spine keeps itself,
 * so depth is depth.
 */
char
dchainreg(struct expr *e)
{
	while (e->op == DEREF)
		e = e->left;
	return isRegvar(e);
}

/*
 * Truncation-transparent operators: the low n bits of the result
 * depend only on the low n bits of the operands, whatever the signs.
 * So if the value is about to be narrowed anyway, the whole
 * computation can be done narrow.
 *
 * / and % need the full value to pick a quotient, >> pulls down bits
 * from above, and a comparison yields int regardless of what its
 * operands were - none of them belong here.
 */
int
truncok(unsigned char op)
{
	switch (op) {
	case PLUS:
	case MINUS:
	/*
	 * STAR belongs here on the maths - the low n bits of a product
	 * depend only on the low n bits of the operands - but there is no
	 * 8-bit multiply helper, so demoting one only produces a shape
	 * nothing can generate.  Leave it wide and let the narrowing
	 * store take the low byte.
	 */
	case AND:
	case OR:
	case XOR:
	case LSHIFT:
	case NEG:
	case NOT:
	/*
	 * A widening is the identity on the bytes it keeps, so
	 * narrowing back through one lands on what it widened.  This
	 * is what lets an equality test between byte-valued operands
	 * be a byte test - the integer promotions put the WIDEN there
	 * in the first place.
	 */
	case WIDEN:
	case SEXT:
		return 1;
	}
	return 0;
}

/*
 * Can this subtree be computed in 'size' bytes without changing the
 * low 'size' bytes of its value?
 */
int
candemote(struct expr *e, int size)
{
	if (!e)
		return 1;
	if (e->type->size <= size)
		return 1;
	if (e->op == CONST)
		return 1;
	if (e->op == DEREF) {
		/*
		 * A narrower read takes the low bytes at the object's own
		 * address.  Inside a word they are the ones there, so a
		 * short read a byte wide is still free.
		 *
		 * A long is not: it keeps its HIGH word at the lower address
		 * (QLONG.md, NUXI), so the low half is two bytes along and a
		 * narrow read of one would take the wrong end.  "val & 0xff"
		 * on a long parameter used to come out as ld a,(iy+d) and
		 * now has to load the long and mask it.
		 *
		 * Not for a register variable either - it is emitted as the
		 * whole register, with no addressable low part.
		 */
		if (e->type && e->type->size > 2)
			return 0;
		return !(e->left->op == SYM && isRegvar(e->left));
	}
	if (!truncok(e->op))
		return 0;
	/* a shift count keeps its own width; only the value narrows */
	if (e->op == LSHIFT)
		return candemote(e->left, size);
	return candemote(e->left, size) && candemote(e->right, size);
}

/*
 * Does this expression provably fit in an unsigned byte?  Not the
 * same question as candemote(), which only asks whether the low
 * bytes may be taken: a long compared against zero passes that and
 * compares false one byte wide.  Everything here is exact - an
 * unsigned byte value, a constant that fits one, or a mask or
 * merge of those - so an equality test between two of them means
 * the same thing at either width, whatever the promotions did.
 */
int
bytevalued(struct expr *e)
{
	if (!e)
		return 0;
	if (e->op == CONST)
		return e->v <= 0xffL;
	if (e->op == WIDEN)
		return e->left->type &&
		    e->left->type->size == 1 &&
		    (e->left->type->flags & TF_UNSIGNED);
	if (e->op == AND)
		return bytevalued(e->left) || bytevalued(e->right);
	if (e->op == OR || e->op == XOR)
		return bytevalued(e->left) && bytevalued(e->right);
	return e->type->size == 1 &&
	    (e->type->flags & TF_UNSIGNED);
}

/*
 * Retype a subtree that candemote() has approved.
 */
struct expr *
demote(struct expr *e, struct type *t)
{
	if (!e || e->type->size <= t->size)
		return e;
	/*
	 * A WIDEN or SEXT is there to make its child wider.  Demote it
	 * to a width the child already has and it becomes a conversion
	 * to its own type - which no rule in pass2 matches, so the whole
	 * comparison it sat in emitted a marker and no code:
	 *
	 *	if ((long)uc == 0L)	(EQ:ubyte (WIDEN:ubyte uc) 0:ubyte)
	 *
	 * The byte test above it is right - a ubyte against zero really
	 * is one - and the cast was right when it was built.  It is
	 * demoting the cast along with everything else that leaves the
	 * node behind saying nothing.  Return the child instead: if it
	 * is narrower than the width we settled on, emitOperand puts a
	 * conversion back, at the width the operand is actually read at.
	 */
	if ((e->op == WIDEN || e->op == SEXT) && e->left &&
	    e->left->type->size <= t->size)
		return demote(e->left, t);
	e->type = t;
	if (e->op == CONST) {
		/* truncate to match, so the emitted value fits the width */
		if (t->size == 1)
			e->v &= 0xff;
		else if (t->size == 2)
			e->v &= 0xffff;
		return e;
	}
	if (e->op == DEREF)
		return e;		/* narrower load, address unchanged */
	e->left = demote(e->left, t);
	if (e->op != LSHIFT)
		e->right = demote(e->right, t);
	return e;
}

int
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
 * The width an operator actually works at.  For most that is the node
 * type, but a comparison yields int whatever it compared, so its own
 * type says nothing about the operands - they meet at their common
 * width instead.
 */
/*
 * The width an operand's VALUE occupies - an array or function
 * compares as its address, not its extent.  A comparison against a
 * forty-byte array otherwise chose forty as the common width and
 * sign-extended the pointer beside it into a long.
 */
unsigned char
valwidth(struct type *t)
{
	if (!t || (t->flags & (TF_POINTER | TF_ARRAY | TF_FUNC)))
		return 2;
	return t->size;
}

struct type *
opwidth(struct expr *e)
{
	if (!iscmpop(e->op))
		return e->type;
	if (!e->left)
		return e->type;
	if (!e->right)
		return e->left->type;
	return valwidth(e->left->type) >= valwidth(e->right->type) ?
	    e->left->type : e->right->type;
}

/*
 * Get size suffix for memory operations based on type
 * Returns: 'b' (byte), 's' (short/int), 'l' (long),
 * 'f' (float), 'd' (double), 'v' (void)
 * Uppercase B/S/L for unsigned types
 * Pointers use 's' since they're 16-bit on Z80
 */
char
typeSfx(struct type *t)
{
	char c;
	if (!t)
		return 's';  /* default to short */

	if (t->flags & (TF_POINTER | TF_ARRAY | TF_FUNC))
		return 's';  /* address-valued: 16 bits, whatever the extent */

	/* Check primitive types by size */
	if (t->size == 0)
		return 'v';  /* void */
	else if (t->size == 1)
		c = 'b';  /* char/byte */
	else if (t->size == 2)
		c = 's';  /* short/int */
	else if (t->size == 4)
		c = 'l';  /* long */
	else
		c = 's';  /* default to short */

	/* Uppercase for unsigned */
	if (t->flags & TF_UNSIGNED)
		c = c - 'a' + 'A';
	return c;
}

/* Helper: build label name from base+suffix */
static char lblBuf[16];

char *
mkLbl(char *base, char *suffix)
{
	fmtstr(lblBuf, "%s%s", base, suffix);
	return lblBuf;
}

void
emitLabel(char *base, char *suffix)
{
	emit1(LABEL);
	emitS(mkLbl(base, suffix));
}

void
emitGoto(char *base, char *suffix)
{
	emit1(GOTO);
	emitS(mkLbl(base, suffix));
}

void emitExpr(struct expr *e);  /* forward decl */

/*
 * Count intermediate labels needed for short-circuit && and || in condition.
 * Each && or || that's not at the top of its kind needs a label.
 */
int
cntCondLbls(struct expr *e)
{
	unsigned char cnt = 0;
	if (!e) return 0;
	/* Count in children first */
	cnt += cntCondLbls(e->left);
	cnt += cntCondLbls(e->right);
	/* && and || each need one label for short-circuit */
	if (e->op == LAND || e->op == LOR)
		cnt++;
	return cnt;
}

/*
 * Where a local ended up.  outast.c wants the register and the frame
 * offset and nothing else about a local, and reaching into the struct
 * for two fields cost it the whole of p1stmt.h.  Asking is cheaper
 * than knowing.
 */
char
localReg(struct local *l)
{
	return l->reg;
}

short
localOff(struct local *l)
{
	return l->frm_off;
}
