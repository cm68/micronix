/*
 * lowering helpers for the rewrite engine.
 *
 * Split from rewrite.c: the biggest pass2 source has to fit
 * through the preprocessor ON the target, and cpp's per-unit
 * tables are paid per translation unit.  rewrite.c keeps the rule
 * matcher; this file keeps what the matched rules call to lower
 * compounds, longs, calls and spills.
 */
/*
 * No <stdlib.h>: this file calls nothing from it.  NULL was the only
 * thing it wanted, and a plain 0 is a null pointer constant wherever
 * one is expected - the header is 34 names, and c0 keeps every name
 * for the whole translation unit.
 */
#include "pass2.h"
#include "expr.h"
#include "opcodes.h"
#include "lexeme.h"

/*
 * How many shapes the rules could not build code for.
 *
 * A marker is a COMMENT.  asz never sees it, so a statement the rules
 * cannot match is simply absent from the output while the compiler
 * exits 0 and the object file looks fine.  "*(int *)0x50 = &cmd" cost
 * two hours of looking at a disk controller before anyone read the
 * assembly, because everything upstream of it was plausible.
 *
 * A compiler is allowed not to implement something.  It is not
 * allowed to say it did.  pass2 counts them and fails.
 */
int nincomplete;

extern int labelcnt;
Expr *rewrite1(Expr *e);
char *idxregname(unsigned char reg);
#ifdef DEBUG
#include "debug.h"
extern short verbose;
#endif


Expr * donehl(Expr *e, unsigned char op);
int ispow2(unsigned long n);
void matflag(unsigned char r);
void normtree(Expr *e);
Expr * step(Expr *e);
Expr * valtohl(Expr *e);

/*
 * A symbol reference, counting the one that has not been folded yet.
 *
 * The compare staging below asks this of an operand before the
 * children are reduced, and "&tab[0]" is a bare SYM there while
 * "&tab[9]" is still a PLUS - it only becomes a SYMREF when the rules
 * fold the offset in, which is after the staging has decided.  So the
 * first was staged into HL and matched, and the second reduced to a
 * shape with a symbol on one side and a register home on the other
 * that the table has no form for: "t <= &basictypes[N_BASIC-1]" in
 * pass1's isBasicType emitted no comparison at all and jumped on
 * whatever flags the conjunct before it had left.
 */
int
issymish(Expr *e)
{
	if (!e)
		return 0;
	if (e->op == SYM || e->op == SYMREF)
		return 1;
	return e->op == PLUS && e->left && e->right &&
	    (e->left->op == SYM || e->left->op == SYMREF) &&
	    e->right->op == NUMBER;
}

/*
 * A frame slot the (iy+d) operand cannot reach.
 *
 * The displacement is seven bits signed, with headroom left for the
 * +3 a long adds, and label() and the LOCALVAR->INDEX rule already
 * turn on exactly this test.  A slot past the window is not an
 * addressing mode at all: its address has to be formed with 16-bit
 * arithmetic, which lands in HL and costs a register like any other
 * computed subexpression.
 */
static int
farslot(Expr *e)
{
	return e && e->op == LOCALVAR &&
	    (e->u.var.off < -126 || e->u.var.off > 124);
}

int
islocdesc(Expr *e)
{
	if (!e)
		return 0;
	switch (e->op) {
	/*
	 * A frame slot only names a place while (iy+d) can reach it.
	 * Past the window it is an address the tree works out, and
	 * calling it a location said it needed no register - so the
	 * store path did not stage it, the address of the destination
	 * and the value both went to HL, and the second overwrote the
	 * first.  "av[i++] = big" with big on a twelve-kilobyte frame
	 * computed the slot's address, threw it away, computed big's,
	 * and then found no rule for =(D(H),H) and left a marker.
	 */
	case LOCALVAR:
		return !farslot(e);
	case REGVAR:
	case INDEX:
	case SYM:
	case SYMREF:
		return 1;
	/*
	 * A literal address - "*(int *)0x50" - names a place as
	 * plainly as a symbol does; the Z80 reaches (nn) with one
	 * instruction either way.  Without this the store path found
	 * an address it could not call a location, gave up before the
	 * rules were ever consulted, and left a marker where the
	 * store should have been.  That is the shape a bootstrap uses
	 * to hand a disk controller its command address.
	 */
	case DEREF:
		return e->left && e->left->op == NUMBER;
	}
	return 0;
}

/*
 * The lvalue-DEREF keep test for a pure chain of loads: does this
 * DEREF chain stand on an operand that carries its variable's value
 * in itself?  A register variable and a frame slot do - REGVAR is
 * the content, INDEX is loaded by the rule that touches it - so for
 * a chain over one of those, the child's value IS the store target
 * and the top DEREF can stay as the store.  A SYMREF is only an
 * ADDRESS: the load of the global's cell is spelled as its own
 * DEREF in the tree, so the same chain over a global carries one
 * more DEREF than it does over a slot, and the top one is not the
 * store but the member fetch.  Those chains have to be reduced
 * whole - value of the chain = the target - and rewrapped.
 */
static char
keepchain(Expr *e)
{
	while (e->op == DEREF)
		e = e->left;
	if (!e)
		return 0;
	return e->op == REGVAR || e->op == LOCALVAR || e->op == INDEX;
}

/*
 * Will working this operand out take HL?
 *
 * A value read through an address needs that address in HL to be
 * read through - unless it names a place the load rules reach where
 * it stands, which is what islocdesc lists: a frame slot and a
 * register home have (iy+d) and (ix+d) forms and never touch HL.
 * Anything else under the DEREF is an address the tree works out,
 * and working it out is what takes the register.
 *
 * Two operands that both answer yes cannot both be reduced in place:
 * the first is in HL when the second goes to load through it.  The
 * caller uses this to run the second one first.
 */
int
needshl(Expr *e)
{
	if (!e || e->op != DEREF)
		return 0;
	return !islocdesc(e->left);
}

/*
 * Did this subtree actually reduce?  A tree that did is a single
 * register node with nothing under it; anything else means a rule was
 * missing somewhere below and no code came out for it.  The root
 * check at the end of rewrite() asks the same question - this is that
 * test, named, so the places that have to ask it earlier can.
 */
int
reduced(Expr *e)
{
	if (!e)
		return 0;
	if (e->left || e->right)
		return 0;
	switch (e->op) {
	case CODE:
	case INHL:
	case INDE:
	case INBC:
	case INA:
	case INE:
		return 1;
	case REGVAR:
		/*
		 * A register variable needs no code: the value is in the
		 * register already.  This is valtohl's set - the homes a
		 * value can be sitting in - and it is here because a
		 * result left in IX now reduces to REGVAR rather than to
		 * a CODE node.
		 */
		return e->u.var.reg == R_IX || e->u.var.reg == R_BC;
	}
	return 0;
}

/*
 * A register standing on the left of an assignment as the destination
 * itself, rather than as somewhere to store through - which is how
 * RETURN and the call-argument wrapper ask for a value in a given
 * register.  Only meaningful before the lvalue is reduced; afterwards
 * the same node means an address that was worked out.
 */
int
isdestreg(Expr *e)
{
	if (!e)
		return 0;
	switch (e->op) {
	case INHL:
	case INDE:
	case INBC:
	case INA:
	case INE:
	case CODE:
		return 1;
	}
	return 0;
}

/*
 * Base operator behind a compound assignment: += is +, and so on.
 * Returns 0 for anything that is not a compound assignment.
 */
unsigned char
baseop(unsigned char op)
{
	switch (op) {
	case PLUSEQ:   return PLUS;
	case SUBEQ:    return MINUS;
	case MULTEQ:   return STAR;
	case DIVEQ:    return DIV;
	case MODEQ:    return MOD;
	case RSHIFTEQ: return RSHIFT;
	case LSHIFTEQ: return LSHIFT;
	case ANDEQ:    return AND;
	case OREQ:     return OR;
	case XOREQ:    return XOR;
	}
	return 0;
}

/*
 * Does evaluating this tree do anything besides produce a value?
 */
int
sideeffect(Expr *e)
{
	if (!e)
		return 0;
	switch (e->op) {
	case CALL:
	case ASSIGN:
	case PREINC:
	case POSTINC:
	case PREDEC:
	case POSTDEC:
		return 1;
	}
	if (baseop(e->op))
		return 1;
	return sideeffect(e->left) || sideeffect(e->right);
}

/*
 * Can this lvalue be named twice?  The expansion below reads the
 * location and then writes it, so the location expression appears
 * twice.  This runs before anything has been emitted for the subtree,
 * so a second copy costs at most some recomputed address arithmetic -
 * but a second call, or a second POSTINC, would be a real bug, which
 * is what "evaluate the lvalue once" forbids.
 *
 * pass1 keeps the DEREF on an lvalue, so a bare REGVAR here really is
 * the variable ("i += 5" with i in BC) and DEREF(REGVAR) is the memory
 * it points at ("*p += 10").  Both are fine to name twice.
 */
int
dupableloc(Expr *e)
{
	return e && !sideeffect(e);
}

/*
 * The same location read rather than written.  A location and the
 * value in it are different expressions, and the difference is one
 * load for every level of indirection:
 *
 *   REGVAR      the register is the storage - it already is the value
 *   DEREF(a)    the memory at a, so the value is a load from a's
 *               value - and a is a location in its own right, so
 *               this has to recur
 *   everything else (SYMREF, LOCALVAR, or an address the tree works
 *               out for itself) names a location, so reading it takes
 *               a load
 *
 * The recursion is the whole point.  "*p" is DEREF(REGVAR) when p is
 * in a register and DEREF(LOCALVAR) when it is in the frame; the
 * first already reads memory, while the second reads only the frame
 * slot and still owes the load the pointer stands for.  Treating the
 * two alike read the pointer and used it as the value.
 */
Expr *
locvalue(Expr *e, char w)
{
	if (e->op == REGVAR)
		return e;
	if (e->op == DEREF) {
		/* what it points through is an address: word sized */
		e->left = locvalue(e->left, 's');
		/*
		 * But what comes BACK through it is the operand, at the
		 * operand's width.  The lvalue spelling this was copied
		 * from carries the address's width, and a long compound
		 * through a pointer inherited it: the value read read
		 * as a short, longable() said no, and "*p <<= n" on a
		 * long emitted nothing.
		 */
		e->width = w;
		return e;
	}
	return mkunary(DEREF, w, e);
}

/*
 * Lower "x OP= y" to "x = x OP y".  This has to run before the children
 * are reduced: once the lvalue has become a register, copying it would
 * re-emit whatever code produced it.
 */
Expr *
lowercompound(Expr *e)
{
	unsigned char op = baseop(e->op);
	char w;
	Expr *loc, *val, *rhs;

	if (!op || !dupableloc(e->left))
		return 0;

	w = e->width;
	loc = e->left;
	rhs = e->right;
	e->left = e->right = 0;
	freeexpr(e);

	val = locvalue(dupexpr(loc), w);

	/*
	 * A byte multiply, divide or remainder has no helper of its own -
	 * candemote knows this and never demotes them, but a compound
	 * assignment BUILDS its operation at the lvalue's width, which
	 * put "*=" on a byte in front of the rules with nothing to say.
	 * Compute at word width, exactly as the promotion would have, and
	 * let the byte store take the low byte back.
	 */
	if (ISBYTE(w) && (op == STAR || op == DIV || op == MOD)) {
		unsigned char ww = ISSIGNED(w) ? T_SHORT : T_USHORT;

		val = mkunary(ISSIGNED(w) ? SEXT : WIDEN, ww, val);
		if (rhs->op == NUMBER)
			rhs->width = ww;
		else
			rhs = mkunary(ISSIGNED(rhs->width) ? SEXT : WIDEN,
			    ww, rhs);
		return mkbinary(ASSIGN, w, loc,
		    mkbinary(op, ww, val, rhs));
	}

	return mkbinary(ASSIGN, w, loc, mkbinary(op, w, val, rhs));
}

/*
 * One arm of a ternary, landed in HL.  Wrapping it in ASSIGN(INHL, v)
 * reuses the whole =(H,...) rule set - which is what makes a constant
 * arm emit anything at all, and what puts both arms in one register so
 * the expression has a value wherever the branch went.
 */
void
branchval(Expr *v)
{
	Expr *hl, *asn;

	if (!v)
		return;
	hl = mkcode(v->width, R_HL);
	hl->op = INHL;
	asn = mkbinary(ASSIGN, v->width, hl, v);
	setdest(asn, DEST_VALUE);
	freeexpr(rewrite(asn));
}

/*
 * The condition code that branches when a condition is false, which is
 * the inverse of the flag the condition produced.  A comparison leaves
 * one of Z/NZ/C/NC/M/P; anything else came back as a value and has to
 * be tested for zero first, which this emits.
 *
 * Both the if statement and the ternary need this.  The ternary used
 * to assume Z meant false, which is only true of a value that has just
 * been tested - a comparison leaves its answer somewhere else, and
 * loading a register leaves the flags alone entirely.
 */
/*
 * A constant operand sets no flags, and u.var.reg on a NUMBER reads
 * the value's own bytes out of the union - so the switch below picked
 * a condition code out of the constant and emitted no test at all.
 * "x && 1" branched on whatever the last instruction happened to
 * leave.  Only && and || and the ternary reach here with a bare
 * NUMBER: an if() goes through rewrite(), which folds a constant
 * condition at the root.
 */
int
constflag(Expr *e)
{
	if (!e || e->op != NUMBER)
		return 0;
	out("\txor a\n");
	if (e->u.val)
		out("\tinc a\n");
	return 1;
}

/*
 * A condition that never reduced still reaches the cc functions, and
 * the union field they read is garbage there - the branch goes on
 * whatever flags were lying around.  "p > q" between two register
 * homes did exactly that, silently.  Say so in the output: it is
 * only a comment, but it is what the differential greps for.
 */
void
ccguard(Expr *e)
{
	unsigned char op;

	if (!e)
		return;
	op = e->op;
	if (op == CODE || op == REGVAR || op == NUMBER ||
	    op == INA || op == INHL || op == INDE ||
	    op == INBC || op == INE)
		return;
	/*
	 * Counted, because it is the same fault as the one above wearing
	 * a different word.  "if (*(char *)0x50 == 1)" says unreduced
	 * condition and "v = *(char *)0x50" says incomplete: the wording
	 * records where it surfaced, not what went wrong.
	 *
	 * I excluded it once, reasoning that four tests emitted it and
	 * all four passed.  They pass on STALE FLAGS.  "ub != 200" came
	 * out as
	 *
	 *	ld a,(_ub)
	 *	jp z,no12_3
	 *
	 * with no cp - and ld a,(nn) sets no flags, so the branch reads
	 * whatever was left further back and happens to go the right
	 * way.  A green test is not the same as correct code, which is
	 * the entire lesson of this bug.
	 */
	nincomplete++;
	/*
	 * With the tree, not without it.  The bare marker says an
	 * expression did not reduce and leaves you to guess which
	 * operand and in which register - and the answer has three
	 * times been "the same shape with the value in another
	 * register", which the tree states outright and the source
	 * does not.
	 */
	out("; XXXXXX unreduced condition: ");
#ifdef DEBUG
	dumpexpr(e);		/* ends the line */
#else
	outc('\n');
#endif
}

char *
falsecc(Expr *e)
{
	if (constflag(e))
		return "z";
	ccguard(e);
	switch (e ? e->u.var.reg : 0) {
	case F_Z:  return "nz";
	case F_NZ: return "z";
	case F_C:  return "nc";
	case F_NC: return "c";
	case F_M:  return "p";
	case F_P:  return "m";
	case R_A:  out("\tor a\n"); return "z";
	case R_HL:
		/*
		 * A long answers in HL':HL and all four bytes vote - the
		 * pair test alone read only half of it, and "if (n & 1)"
		 * was false for every odd long.  A is not banked, so the
		 * fold just carries on across the exx.
		 */
		if (ISLONG(e->width))
			out("\tld a,l\n\tor h\n\texx\n"
			    "\tor l\n\tor h\n\texx\n");
		else
			out("\tld a,l\n\tor h\n");
		return "z";
	case R_DE: out("\tld a,e\n\tor d\n"); return "z";
	case R_BC: out("\tld a,c\n\tor b\n"); return "z";
	}
	return "z";
}

/*
 * The other half: the condition that branches when a condition is
 * true.  Same shape, same zero test where the answer came back as a
 * value rather than a flag.
 */
char *
truecc(Expr *e)
{
	if (constflag(e))
		return "nz";
	ccguard(e);
	switch (e ? e->u.var.reg : 0) {
	case F_Z:  return "z";
	case F_NZ: return "nz";
	case F_C:  return "c";
	case F_NC: return "nc";
	case F_M:  return "m";
	case F_P:  return "p";
	case R_A:  out("\tor a\n"); return "nz";
	case R_HL:
		if (ISLONG(e->width))
			out("\tld a,l\n\tor h\n\texx\n"
			    "\tor l\n\tor h\n\texx\n");
		else
			out("\tld a,l\n\tor h\n");
		return "nz";
	case R_DE: out("\tld a,e\n\tor d\n"); return "nz";
	case R_BC: out("\tld a,c\n\tor b\n"); return "nz";
	}
	return "nz";
}

/*
 * Both operands are bytes and both have to be worked out.  A is the
 * only byte register the ALU works in, so there is no second one to
 * assign and reducing the right operand lands it on top of the left.
 * Nothing downstream notices: the tree reads (A) op (A), no rule
 * matches a register against itself, and the code that came out
 * applied the operator to the right operand and whatever was left
 * over.
 *
 * So the left waits on the stack while the right is worked out, and
 * the right comes back to E - the second operand every byte rule
 * already expects.  Q(A,K), +(A,K) and the rest of that family were
 * there all along with nothing able to reach them.
 *
 * Three bytes, and push af rather than a spare register because there
 * is none: B and C may hold a register variable, and D and E are where
 * the answer is going.  The flags ride along and come back, which
 * costs nothing - the operator sets its own.
 */
void
bytepair(Expr *e)
{
	char w;

	out("\tpush af\n");
	e->right = rewrite1(e->right);
	if (e->right->op == INA) {
		w = e->right->width;
		out("\tld e,a\n");
		freeexpr(e->right);
		e->right = mkcode(w, R_E);
		e->right->op = INE;
	}
	out("\tpop af\n");
}

/*
 * A reduced operand does not always land where it was asked to: a byte
 * operation can only end in A and a call only in HL, whatever target
 * they were given.  Move it, for the cases where something else is
 * about to want that register.
 *
 * Only safe while the other operand has not been evaluated yet - the
 * HL form goes through ex de,hl, which would trample it.
 *
 * What a caller asking for DE wants from a long is its low word, and
 * under HL':HL that is in HL - so the narrowing is free, and the move
 * itself is then the ordinary exchange the 16-bit path already does.
 * Saying the width is short first is the whole of the difference; skip
 * that and the store rules go on reading four bytes.
 *
 * Under the old HL:DE layout the low word was already in DE and this
 * emitted nothing at all, which is a difference worth naming, because
 * the version before THAT ran the 16-bit path on a long without
 * narrowing it: "0123456789ABCDEF"[i % base] swapped the halves and
 * then indexed by the high word, so _pnum wrote a zero for every digit
 * and printf("%d") printed nothing at all.
 */
Expr *
movetotgt(Expr *e, unsigned char tgt)
{
	if (!e || tgt != R_DE)
		return e;
	if (ISLONG(e->width)) {
		if (e->op != INHL)
			return e;
		e->width = ISSIGNED(e->width) ? T_SHORT : T_USHORT;
	}
	if (e->op == INA) {
		out("\tld e,a\n");
		e->op = INE;
		e->u.var.reg = R_E;
	} else if (e->op == INHL) {
		out("\tex de,hl\n");
		e->op = INDE;
		e->u.var.reg = R_DE;
	}
	return e;
}

/*
 * Compound assignment whose lvalue has side effects, so the expansion
 * in lowercompound() cannot be used - naming "*p++" twice would
 * increment p twice.  The address is worked out once and waits on the
 * stack while the value is read, updated and written back, the stack
 * standing in for the temporary the expression tree has no way to
 * spell.
 *
 * Returns 0 if the address did not reduce to HL, leaving the node
 * to be flagged rather than guessed at.
 */
Expr *
docompound(Expr *e)
{
	unsigned char op = baseop(e->op);
	char w = e->width;
	int isbyte = ISBYTE(w);
	Expr *addr, *val, *rhs, *sum, *n;

	if (!op || !e->left || !e->right)
		return 0;

	/*
	 * The address, once - the side effects happen here and only here.
	 * Its value is wanted, and has to say so: a postfix step inside
	 * it yields the value from before the step, and anything deciding
	 * that from the destination would otherwise read "discarded" and
	 * be free to hand back the one after.
	 */
	setdest(e->left, DEST_VALUE);
	addr = rewrite1(e->left);
	e->left = 0;
	if (!addr || (addr->op != INHL &&
	    !(addr->op == CODE && addr->u.var.reg == R_HL))) {
		e->left = addr;
		return 0;
	}
	freeexpr(addr);
	out("\tpush hl\n");

	/* read through it */
	if (isbyte)
		out("\tld a,(hl)\n");
	else
		out("\tld a,(hl)\n\tinc hl\n\tld h,(hl)\n\tld l,a\n");

	/* apply the operator to what was there */
	val = mkcode(w, isbyte ? R_A : R_HL);
	val->op = isbyte ? INA : INHL;
	rhs = e->right;
	e->right = 0;
	sum = mkbinary(op, w, val, rhs);
	setdest(sum, DEST_VALUE);
	/*
	 * The left is already a byte in A.  Working the right side out
	 * lands byte loads in A too - on top of the value - and the
	 * fresh nodes here carry no labels, so the register machinery
	 * that usually notices never runs.  bytepair() is exactly this
	 * situation's tool: park A on the stack, reduce the right into
	 * E, take A back.
	 */
	if (isbyte && rhs->op != NUMBER && !reduced(rhs))
		bytepair(sum);
	sum = rewrite(sum);
	/*
	 * The store below asserts the answer landed in A or HL.  When no
	 * rule reduced the operator that assertion stored garbage - the
	 * count of a shift, whatever a half-finished reduction left in
	 * the register - and said nothing.  The else-if bitmap bug rode
	 * exactly that silence one level up; this path gets the marker
	 * it always owed.
	 */
	if (!sum || (isbyte ? sum->op != INA : sum->op != INHL)) {
		nincomplete++;
		out("; XXXXXX incomplete: compound rhs");
#ifdef DEBUG
		if (sum) {
			out(" ");
			dumpexpr(sum);
		}
#endif
		outc('\n');
	}

	/* store it back through the address that was waiting */
	if (isbyte) {
		out("\tpop hl\n\tld (hl),a\n");
		n = mkcode(w, R_A);
		n->op = INA;
	} else {
		out("\tpop de\n\tex de,hl\n\tld (hl),e\n\tinc hl\n\tld (hl),d\n\tex de,hl\n");
		n = mkcode(w, R_HL);
		n->op = INHL;
	}
	freeexpr(sum);
	n->dest = e->dest;
	freeexpr(e);
	return n;
}

/*
 * Land one value in HL - HL':HL when it is long.  Wrapping it in
 * ASSIGN(INHL, value) reuses the whole =(H,...) rule set, the same
 * trick RETURN uses.  Scalars are widened to a word first: C promotes
 * char arguments to int.  Returns the width it landed.
 */
char
landarg(Expr *a)
{
	Expr *hl, *asn;
	char w;

	w = a->width;
	if (w != 'l' && w != 'L')
		w = 's';

	hl = mkcode(w, R_HL);
	hl->op = INHL;
	asn = mkbinary(ASSIGN, w, hl, a);
	setdest(asn, DEST_VALUE);
	freeexpr(rewrite(asn));
	return w;
}

/*
 * Push one call argument, returning the stack bytes it consumed.
 * Only arguments after the first come here: the first rides to the
 * callee in HL.
 */
int
pusharg(Expr *a)
{
	if (!a)
		return 0;
	if (landarg(a) == 's') {
		out("\tpush hl\n");
		return 2;
	}
	/*
	 * A long lives in HL':HL, and an argument goes on the stack the
	 * way it sits in memory: low word first, so the HIGH word ends up
	 * at the lower address.  Two pushes rather than one pair, because
	 * the halves are in different banks.
	 */
	out("\tpush hl\n\texx\n\tpush hl\n\texx\n");
	return 4;
}

/* a 32-bit integer, which the helpers handle */
#define ISLONGINT(t) ((t) == T_LONG || (t) == T_ULONG)

/*
 * Did this reduce to a 32-bit value sitting in HL:DE?  A rule hands one
 * back as a register node, or as a CODE that has not been converted
 * yet; either way the high word is in HL.
 */
int
islongreg(Expr *e)
{
	if (!e)
		return 0;
	if (e->op == INHL)
		return 1;
	return e->op == CODE && e->u.var.reg == R_HL;
}

char *longhelper(unsigned char op, int sign);

/*
 * Is this one of the operators the long path above is responsible for?
 * Only those, because only those have no rules of their own to fall
 * back on - assignment and the conversions are handled in the table
 * and must be left alone to reach it.
 *
 * A comparison is a byte wide whatever it compared, so it is the
 * operand that says whether this is 32-bit work.
 */
/*
 * Say so when a long helper's operand did not reduce.  Silence here
 * is a wrong number: the helper is called regardless and reads
 * whatever the registers held.
 */
static void
longincomplete(Expr *e)
{
	if (!e || reduced(e))
		return;
	nincomplete++;
	out("; XXXXXX incomplete: ");
#ifdef DEBUG
	dumpexpr(e);		/* ends the line */
#else
	outc('\n');
#endif
}

int
islongop(Expr *e)
{
	if (!e || !e->left || !e->right)
		return 0;
	switch (e->op) {
	case PLUS: case MINUS: case STAR: case DIV: case MOD:
	case AND: case OR: case XOR: case LSHIFT: case RSHIFT:
		return ISLONGINT(e->width);
	case EQ: case NEQ: case LT: case GT: case LE: case GE:
		return ISLONGINT(e->left->width);
	}
	return 0;
}

/*
 * The two halves of a 32-bit constant.  A constant operand never
 * reduces - it stays a NUMBER so the ",N)" rules can see it - so the
 * long path has to place one itself.
 *
 * Into the first accumulator, HL':HL.  The exx pair costs two bytes
 * that the old HL:DE layout did not, and gets them straight back at
 * the call site: this used to be followed by pushlongc for the other
 * operand, four instructions and eight bytes of stack traffic that
 * loadlongd replaces with a pair of loads.
 */
void
loadlongc(long v)
{
	outf("\tld hl,%d\n\texx\n\tld hl,%d\n\texx\n",
	    (int)(v & 0xffff), (int)((v >> 16) & 0xffff));
}

/* and into the second, DE':DE */
void
loadlongd(long v)
{
	outf("\tld de,%d\n\texx\n\tld de,%d\n\texx\n",
	    (int)(v & 0xffff), (int)((v >> 16) & 0xffff));
}

/*
 * Move the value in HL':HL to DE':DE, freeing the first accumulator
 * for the other operand.  Four bytes, and the exx pair is what makes
 * it possible at all - ex de,hl reaches only the half of each pair
 * that is currently selected.
 */
void
longtode(void)
{
	out("\tex de,hl\n\texx\n\tex de,hl\n\texx\n");
}

/*
 * Park a long on the stack, low word first so it sits the way a long
 * does in memory - high word at the lower address - and get it back
 * into DE':DE.
 *
 * This is the expensive path and it is meant to be the rare one: the
 * two halves are in different banks, so each needs its own push, where
 * the old HL:DE layout could push both from one.  It is only reached
 * when the right operand is too complicated for longsimple().
 */
void
pushlong(void)
{
	out("\tpush hl\n\texx\n\tpush hl\n\texx\n");
}

void
poplongd(void)
{
	out("\texx\n\tpop de\n\texx\n\tpop de\n");
}

/*
 * Will this operand end up as a 32-bit value in HL:DE?  Asked before
 * any code is emitted, because the first operand of a helper call goes
 * on the stack and there is no backing out after that.
 *
 * A constant is placed directly.  Anything else has to be a shape the
 * long rules cover, or something that reduces to one - a nested long
 * operator, or a widening of something narrower.
 */
int
longable(Expr *e)
{
	if (!e)
		return 0;
	if (e->op == NUMBER || islongreg(e))
		return 1;
	if (!ISLONGINT(e->width))
		return 0;
	switch (e->op) {
	case DEREF:
	case SEXT:
	case WIDEN:
	case CALL:
		return 1;
	case ASSIGN:
		/*
		 * A long assignment used as a value: the store rules'
		 * value forms leave HL:DE holding what was stored, so
		 * the operand is as good as any other long.  Without
		 * this, "if ((pos = off - ftell(f)) == 0)" was declined
		 * here and nothing else could compare a long - the
		 * condition never reduced.
		 */
		return 1;
	case NOT:
		/* complement has a rule of its own */
		return 1;
	case QUES:
		/*
		 * A long-valued ternary lands each arm in HL:DE through
		 * ASSIGN(INHL, arm) and answers as INHL like any other
		 * long.  It was missing from this list, so any long
		 * arithmetic with a conditional operand - "(c ? a : b)
		 * + 10" - was declined here, refused by the width guard
		 * above the rules, and the whole statement left as an
		 * XXXXXX marker: no code at all.
		 */
		return e->right->op == TERNBRANCH;
	case LSHIFT:
	case RSHIFT:
		/* handled here too, but by their own path - the count is not
		 * a long and longhelper has no name for them */
		return e->left && e->right;
	}
	/* a nested long operator, which recurses through this same path */
	return longhelper(e->op, 1) != 0 && e->left && e->right;
}

/*
 * The runtime helper for a 32-bit operator, or 0 if there is none.
 *
 * These are ccc's own 32-bit runtime, the q set - see
 * libsrc/libc/QLONG.md.  Both operands are in registers, HL':HL and
 * DE':DE, so the caller pushes nothing and none of them touches BC.
 *
 * Only the operations that genuinely differ have two names.  Add,
 * subtract, multiply and the bitwise three give the same bits whether
 * the top one is a sign or a value; divide, remainder and comparison
 * do not.
 */
char *
longhelper(unsigned char op, int sign)
{
	switch (op) {
	case PLUS:   return "qadd";
	case MINUS:  return "qsub";
	case STAR:   return "qmul";
	case DIV:    return sign ? "qdiv" : "qudiv";
	case MOD:    return sign ? "qmod" : "qumod";
	case AND:    return "qand";
	case OR:     return "qor";
	case XOR:    return "qxor";
	case EQ: case NEQ: case LT: case GT: case LE: case GE:
		return sign ? "qcmp" : "qucmp";
	}
	return 0;
}

/*
 * The flag a comparison helper leaves the answer in.
 *
 * arelop reports a signed comparison the way a subtraction would, in
 * the sign flag; lrelop reports an unsigned one in carry, where the
 * 16-bit code looks for it.  Neither gives "greater" or "at or below"
 * as one flag, so those are had by swapping the operands, which the
 * caller does by handing them over the other way round.
 */
unsigned char
longflag(unsigned char op, int sign)
{
	switch (op) {
	case EQ:  return F_Z;
	case NEQ: return F_NZ;
	case LT: case GT:  return sign ? F_M : F_C;
	case GE: case LE:  return sign ? F_P : F_NC;
	}
	return 0;
}

/*
 * Widen an operand to a long, if it is not one already.  Signed sources
 * sign-extend and unsigned ones do not, which is the whole difference
 * between the SEXT and WIDEN rules at long width.  A constant is left
 * as it is: the callers below load and push one directly, at whatever
 * width it was written.
 */
Expr *
tolong(Expr *e, char w)
{
	Expr *n;

	if (!e || e->op == NUMBER || ISLONG(e->width))
		return e;
	n = mkunary(ISSIGNED(e->width) ? SEXT : WIDEN, w, e);
	n->dest = DEST_VALUE;
	return n;
}

/*
 * A binary operator on 32-bit values.  There are two 32-bit
 * accumulators now, HL':HL and DE':DE, so this is mostly a matter of
 * getting one operand into each and calling the helper - and what it
 * used to be instead is worth recording, because nearly everything
 * awkward about the long path came from there being only one.
 *
 * The right operand was worked out first and pushed, because HL:DE was
 * also where the left one had to be.  BC was saved around the call,
 * because the helper took the pushed operand back with a pop bc and
 * would otherwise destroy a register variable living there.  And
 * longable() had to be asked, in advance, whether both operands were
 * shapes that could reach HL:DE at all: "settled before anything is
 * emitted - once the right operand has been pushed there is no way
 * back".  That gate is why a long-valued ternary emitted no code at
 * all until QUES was added to its list, and why comparing against an
 * assignment did not reduce until ASSIGN was.  Guessing wrong was
 * silent.
 *
 * None of it is needed.  Register operands are reversible, the q
 * helpers pop nothing and touch no BC, and both sides go through
 * rewrite1 like any other value.
 *
 * The right operand still has to get to DE':DE without disturbing the
 * left, which is what longsimple() is about below.
 *
 * "Greater than" and "at or below" have no flag of their own, so they
 * are passed over as "less than" and "at or above" with the operands
 * the other way round.  Evaluation order is already unspecified here,
 * and the reorder elsewhere in this file relies on the same thing.
 */
Expr *
dolongbin(Expr *e)
{
	unsigned char op = e->op;
	int iscmp = (longflag(op, 1) != 0);
	Expr *opnd = iscmp ? e->left : e;
	int swap = (op == GT || op == LE);
	int sign;
	char *fn;
	Expr *l, *r, *n;

	if (!e->left || !e->right)
		return 0;
	/*
	 * A comparison carries the width of what it answers, not of what
	 * it compares, so the operands are where to look - and either one
	 * of them may be the long.  Reading only the left meant "s < l"
	 * was judged by s and declined here, while "l < s" got as far as
	 * the gate below and was declined there instead.
	 */
	if (iscmp && !ISLONGINT(opnd->width) && ISLONGINT(e->right->width))
		opnd = e->right;
	if (!opnd || !ISLONGINT(opnd->width))
		return 0;
	sign = ISSIGNED(opnd->width);

	l = swap ? e->right : e->left;
	r = swap ? e->left : e->right;

	/*
	 * A shift is not like the rest: the count is a plain int and goes
	 * in B rather than on the stack.  Work it out first and park it,
	 * because reducing the value may call a helper of its own and
	 * those use BC.
	 */
	if (op == LSHIFT || op == RSHIFT) {
		e->left = e->right = 0;
		if (r->op == NUMBER) {
			if (l->op == NUMBER)
				loadlongc(l->u.val);
			else
				l = rewrite1(l);
			outf("\tld a,%d\n", (int)(r->u.val & 0xff));
		} else {
			/*
			 * The count is an ordinary int and would have been aimed
			 * at DE, being the right operand of a binary node.  It
			 * has to come back in HL: that is where this parks it,
			 * and DE':DE belongs to nothing here, a shift having
			 * only the one operand.
			 */
			assign(r, R_HL);
			r = rewrite1(r);
			/*
			 * "Aimed at HL" is a request, not a guarantee: a
			 * count living in a register home reduces to itself
			 * through the empty =(C,x) rules and never moves.
			 * ispow2's own loop counter sat in BC, the push
			 * below parked garbage, and the self-hosted c1
			 * shifted by whatever HL last held.
			 */
			r = valtohl(r);
			/*
			 * Only the low byte of the count can matter, and a
			 * long count keeps its low word in HL now like any
			 * other - under the old layout it was in DE, so
			 * pushing HL parked the HIGH word and every count
			 * below 65536 shifted by nought.
			 */
			out("\tpush hl\n");
			if (l->op == NUMBER)
				loadlongc(l->u.val);
			else
				l = rewrite1(l);
			/* the count is on the stack; its low byte to A */
			out("\tpop de\n\tld a,e\n");
		}
		freeexpr(l);
		freeexpr(r);
		/*
		 * The count goes in A and the helper counts in B', so there
		 * is no longer a BC to save around this.
		 */
		outf("\tcall %s\n",
		    op == LSHIFT ? "qshl" : sign ? "qsar" : "qshr");
		return donehl(e, INHL);
	}

	fn = longhelper(op, sign);
	if (!fn)
		return 0;

	/*
	 * A long against something narrower.  C converts the narrow side,
	 * and the rules for that conversion are already in the table - what
	 * was missing is that nobody put the conversion in the tree.  A
	 * constant is left alone: the loaders take one at whatever width
	 * it is written.
	 */
	l = tolong(l, opnd->width);
	r = tolong(r, opnd->width);

	e->left = e->right = 0;

	/*
	 * A constant right operand goes straight into the second
	 * accumulator once the left is out of the way, and costs nothing:
	 * every comparison against a literal, every "+ 1L".
	 *
	 * Loading a global or a frame slot straight into DE':DE was tried
	 * here too and taken out again.  It is six bytes better per site
	 * in principle, and it fired - but on this tree it fired nowhere
	 * that mattered: cpp and c0 came out byte-identical with and
	 * without it, and the two functions it took to decide and emit
	 * cost c1 five hundred bytes.  The stack path below is what the
	 * measurement preferred.
	 *
	 * Anything else has to be worked out in HL':HL, because that is
	 * where rewrite1 puts a long, and then moved aside.  Moving it to
	 * DE':DE is four bytes and would be the whole story if working out
	 * the LEFT afterwards could be trusted not to touch DE - but DE is
	 * the ordinary 16-bit scratch pair and almost anything uses it.
	 * So the stack, one push per half because the halves are in
	 * different banks.
	 */
	if (r->op == NUMBER) {
		if (l->op == NUMBER)
			loadlongc(l->u.val);
		else {
			l = rewrite1(l);
			longincomplete(l);
		}
		freeexpr(l);
		loadlongd(r->u.val);
		freeexpr(r);
	} else {
		assign(r, R_HL);
		r = rewrite1(r);
		longincomplete(r);	/* before the free: see the note there */
		freeexpr(r);
		pushlong();
		if (l->op == NUMBER)
			loadlongc(l->u.val);
		else {
			assign(l, R_HL);
			l = rewrite1(l);
			longincomplete(l);
		}
		freeexpr(l);
		poplongd();
	}

	outf("\tcall %s\n", fn);

	if (iscmp) {
		/*
		 * A comparison answers in a flag, and this returns straight
		 * to the caller rather than through the loop that would
		 * otherwise turn one into a number where a number is what
		 * was asked for.  So do it here: everything downstream then
		 * sees an ordinary byte, and a long comparison can be
		 * assigned and widened like any other.
		 */
		unsigned char f = longflag(op, sign);

		if (e->dest == DEST_FLAGS) {
			n = mkcode(e->width, f);
		} else {
			matflag(f);
			n = mkcode('B', R_A);
			n->op = INA;
		}
	} else {
		n = mkcode(e->width, R_HL);
		n->op = INHL;
	}
	n->dest = e->dest;
	freeexpr(e);
	return n;
}

/*
 * Emit a call: arguments pushed right-to-left, then the call, then the
 * caller drops the arguments.  The arg chain is already built
 * last-to-first, which is push order.
 */
Expr *
docall(Expr *e)
{
	Expr *a, *next, *fn;
	Expr *first = 0;
	int nbytes = 0;
	int i;
	int direct;

	/*
	 * A name resolves to a SYMREF and emits nothing, so it can be
	 * settled now.  Anything else is a pointer whose value has to be
	 * loaded, and loading it here would only see it trampled by the
	 * argument pushes - so it waits.
	 */
	fn = e->left;
	direct = fn && (fn->op == SYM || fn->op == SYMREF);
	if (direct) {
		e->left = fn = rewrite1(fn);
		if (!fn || fn->op != SYMREF)
			return 0;
	}

	/*
	 * BC survives a call now, and the callee is what makes it.
	 *
	 * It did not use to.  The runtime was built by zc3, which uses
	 * BC as scratch and never saves it, and the hand-written
	 * routines popped their arguments straight into it - so a
	 * register variable in BC was gone across any call into the
	 * library, and
	 *
	 *	for (i = 0; i < argc; i++)	i came back holding the
	 *		printf("%s\n", argv[i]);   last character printed
	 *
	 * The answer was two bytes at every call site that had a
	 * variable in BC: 731 of them in c1 alone, 1462 bytes, to guard
	 * against a handful of routines.  The knowledge belongs in those
	 * routines.  Every one of them saves BC on the stack now - not
	 * in a static, which recursion and any chain of them would tread
	 * on - and ccc's own prologue has always saved it, so a call is
	 * a call whatever it lands in.
	 *
	 * What is left is $[ and $]: the arithmetic helpers really do
	 * count in B, they are reached by name from the rule templates
	 * rather than through here, and guarding them is the same two
	 * bytes at far fewer places.
	 */

	/*
	 * The first argument does not go on the stack: it is evaluated
	 * last - the chain is built last-to-first, so it already comes
	 * around last - and rides to the callee in HL, or HL':HL when it
	 * is long.  The callee's own prologue helper spills it into the
	 * slot the caller used to push it into, so from the callee's
	 * side nothing has moved.
	 */
	for (a = e->right; a && a->op == ARGNODE; a = next) {
		Expr *v = a->left;
		next = a->right;
		a->left = a->right = 0;
		freeexpr(a);
		if (next && next->op == ARGNODE)
			nbytes += pusharg(v);
		else
			first = v;
	}
	e->right = 0;

	if (direct) {
		if (first)
			landarg(first);
		outf("\tcall %s\n", fn->u.symref.name);
	} else {
		/*
		 * Through a pointer.  The Z80 can jump to the address in HL
		 * but not call it, and the whole difference is the return
		 * address - so borrow one: call a trampoline that is
		 * nothing but jp (hl).  The call pushes the return address,
		 * the jump hands over, and the function's own ret comes
		 * back here.  One byte of library and an ordinary call at
		 * every site.
		 *
		 * With arguments, HL is spoken for.  The address is worked
		 * out first and parked on the stack while the first
		 * argument is evaluated, then comes back in DE - dead at
		 * every call - and the trampoline for that is push de /
		 * ret.
		 */
		e->left = 0;
		landarg(fn);
		if (first) {
			out("\tpush hl\n");
			landarg(first);
			out("\tpop de\n\tcall trampde\n");
		} else
			out("\tcall tramp\n");
	}

	/*
	 * Drop the arguments.  A word at a time with pop af: one byte of
	 * code per two of stack, where inc sp costs a byte apiece - and
	 * the pop touches nothing that is live, because a result comes
	 * back in HL (or HL:DE) and never in A.  That halving is worth
	 * about a kilobyte in each of the three passes, which is the
	 * kind of money this machine deals in.
	 *
	 * Past six words the HL form is smaller still, but it has to
	 * shuffle the result through DE - which a LONG result also lives
	 * in, so the shuffle mangled it: the first call in the ecosystem
	 * with ten arg bytes and a long result came back with one half
	 * swapped and the other replaced by the byte count.  Long results
	 * keep popping however many words it takes.
	 */
	if (nbytes > 0 && (nbytes <= 12 || ISLONG(e->width))) {
		for (i = 0; i + 1 < nbytes; i += 2)
			out("\tpop af\n");
		if (nbytes & 1)
			out("\tinc sp\n");
	} else if (nbytes > 0) {
		outf("\tex de,hl\n\tld hl,%d\n\tadd hl,sp\n\tld sp,hl\n\tex de,hl\n",
		    nbytes);
	}

	/* Result is in HL.  Hand back an INHL rather than a bare CODE:
	 * we return straight to the caller, skipping the step() loop that
	 * would otherwise do the CODE -> IN* conversion for us. */
	return donehl(e, INHL);
}

/*
 * Load a SYMREF's address into HL and hand back the register node.
 * A SYMREF is left unreduced so the store and load rules can use it
 * as an address; the two places that need its VALUE staged share
 * this.
 */
Expr *
symtohl(Expr *s)
{
	Expr *n;
	char w = s->width;

	out("\tld hl,");
	out(s->u.symref.name);
	if (s->u.symref.off) {
		if (s->u.symref.off > 0)
			outc('+');
		outd(s->u.symref.off);
	}
	outc('\n');
	freeexpr(s);
	n = mkcode(w, R_HL);
	n->op = INHL;
	return n;
}

/*
 * An INDEX being used as a value rather than a location: "a + 2"
 * with a in a register.  It reduces to itself and emits nothing -
 * that is what lets the (reg+d) rules read through it - so a path
 * that stages values through HL has to form the number itself, or
 * the pop below it stores whatever HL last held.  add hl takes only
 * bc/de/hl/sp, so the register goes through the stack; DE is free
 * everywhere this is called, the sibling being on the stack.
 */
Expr *
idxtohl(Expr *s)
{
	Expr *n;
	char w = s->width;

	outf("\tpush %s\n\tpop hl\n", idxregname(s->u.var.reg));
	if (s->u.var.off)
		outf("\tld de,%d\n\tadd hl,de\n", s->u.var.off);
	freeexpr(s);
	n = mkcode(w, R_HL);
	n->op = INHL;
	return n;
}

/*
 * A reduced operand whose VALUE the caller is about to stage through
 * HL - push it, pass it, keep it while something else runs.  Half a
 * dozen node kinds reduce to themselves by design so the rules can
 * read them in place, and every staging path that assumed "reduced
 * means it is in HL" has been bitten by one of them: SYMREF (an
 * address), INDEX (a register-relative address), and the register
 * homes.  This is the one place that knows the whole list.
 */
Expr *
valtohl(Expr *e)
{
	Expr *n;
	char w;

	if (!e)
		return e;
	if (e->op == SYMREF)
		return symtohl(e);
	if (e->op == INDEX)
		return idxtohl(e);
	if (e->op == INBC || e->op == INDE ||
	    (e->op == REGVAR &&
	     (e->u.var.reg == R_BC || e->u.var.reg == R_IX))) {
		if (e->op == INDE)
			out("\tld l,e\n\tld h,d\n");
		else if (e->op == REGVAR && e->u.var.reg == R_IX)
			out("\tpush ix\n\tpop hl\n");
		else
			out("\tld l,c\n\tld h,b\n");
		w = e->width;
		freeexpr(e);
		n = mkcode(w, R_HL);
		n->op = INHL;
		return n;
	}
	return e;
}

/*
 * Finish a store whose address AND value both need registers.  The
 * address is in HL and has already been pushed; compute the value
 * over it and bring the address back out from underneath, leaving
 * the shapes the store rules name.  A byte value ends in A, which
 * the address does not disturb, so the address comes straight back
 * to HL; a word is in HL itself and has to move aside; a long fills
 * HL':HL, so the address stays where it is - on the stack, which
 * is where qst wants it.
 *
 * Returns the finished node for the long case, null when the caller
 * should fall through to the matcher with the rebuilt children.
 */
static Expr *
spiltstore(Expr *e, Expr *oldleft)
{
	Expr *a;

	e->right = valtohl(rewrite1(e->right));
	if (ISLONGINT(e->width)) {
		out("\tcall qst\n");
		freeexpr(oldleft);
		freeexpr(e->right);
		e->left = e->right = 0;
		return donehl(e, INHL);
	}
	if (e->right->op == INA) {
		out("\tpop hl\n");
		freeexpr(oldleft);
		freeexpr(e->right);
		e->right = mkcode(e->width, R_A);
		e->right->op = INA;
	} else if (ISBYTE(e->width)) {
		/*
		 * A byte store whose value came back as a word - a masked
		 * call result, say.  The byte is the low one; bring it to
		 * A so the store meets the =(D(H),A):b family instead of
		 * a word shape no byte rule names.
		 */
		out("\tld a,l\n\tpop hl\n");
		freeexpr(oldleft);
		freeexpr(e->right);
		e->right = mkcode(e->width, R_A);
		e->right->op = INA;
	} else {
		out("\tpop de\n\tex de,hl\n");
		freeexpr(oldleft);
		freeexpr(e->right);
		e->right = mkcode(e->width, R_DE);
		e->right->op = INDE;
	}
	a = mkcode(e->width, R_HL);
	a->op = INHL;
	e->left = mkunary(DEREF, e->width, a);
	return 0;
}

Expr *
rewrite1(Expr *e)
{
	Expr *n, *next;
	char lw, rw;
	unsigned char narrow, both;

	if (!e) return 0;

	/*
	 * A comma is its right operand, once the left has been emitted for
	 * whatever it does.  Collapse it rather than matching it: no rule
	 * reduces a bare constant - a constant only ever becomes a load as
	 * part of some parent rule that names it - so a comma whose right
	 * was a constant or a frame slot had nothing to match, and the
	 * ";(_,H)" family could only ever catch the cases that happened to
	 * reduce to a register on their own.  Collapsing hands the right
	 * operand to whatever encloses the comma, which has the rules for
	 * it already.
	 */
	if (e->op == COMMA && e->left && e->right) {
		Expr *val;

		e->left->dest = DEST_NONE;
		freeexpr(rewrite1(e->left));
		val = e->right;
		val->dest = e->dest;
		val->tgt = e->tgt;
		e->left = e->right = 0;
		freeexpr(e);
		return rewrite1(val);
	}

	/* CALL: args have to be pushed one at a time, not rewritten as a
	 * batch - each one lands in HL and would clobber the last. */
	if (e->op == CALL) {
		n = docall(e);
		if (n)
			return n;
	}

	/*
	 * A compare whose left operand is a bare symbol.  The address is
	 * the value, so load it up front and let the (H,x) forms carry
	 * every context - the table only ever grew flag-context rules
	 * for the symbol-on-the-left shapes, and "n = s > buf" sat
	 * unreduced in value context.  Only for a register right-hand
	 * side: anything bigger may pass through HL itself on the way.
	 */
	if ((e->op == LT || e->op == GE || e->op == LE || e->op == GT ||
	     e->op == EQ || e->op == NEQ) && e->left && e->right) {
		/*
		 * Declared here and worked out below the staging: the
		 * byte-pair block replaces both operands, so what these
		 * describe is the tree as it stands after it has run.  The
		 * declarations cannot follow it - this compiler has to
		 * build itself, and it takes declarations at the top of a
		 * block only.
		 */
		char lsym, rsym, linreg, rinreg;

		/*
		 * Both operands living in the byte register homes: no rule
		 * pairs b against c, and the condition fell to ccguard's
		 * marker with the branch on stale flags - label()'s own
		 * "l > r ? l : r" answered wrong in the self-hosted build
		 * and every evaluation-order decision downstream of it
		 * followed.  Stage them where the (A,K) family lives: the
		 * right into E first, then the left into A - the loads
		 * touch neither home.
		 */
		if (ISBYTE(e->left->width) && ISBYTE(e->right->width) &&
		    e->left->op == REGVAR && e->right->op == REGVAR &&
		    (e->left->u.var.reg == R_B || e->left->u.var.reg == R_C) &&
		    (e->right->u.var.reg == R_B || e->right->u.var.reg == R_C)) {
			char lw = e->left->width;
			char rw = e->right->width;

			outf("\tld e,%c\n",
			    e->right->u.var.reg == R_B ? 'b' : 'c');
			outf("\tld a,%c\n",
			    e->left->u.var.reg == R_B ? 'b' : 'c');
			freeexpr(e->left);
			freeexpr(e->right);
			e->left = mkcode(lw, R_A);
			e->left->op = INA;
			e->right = mkcode(rw, R_E);
			e->right->op = INE;
		}
		lsym = issymish(e->left);
		rsym = issymish(e->right);
		linreg = e->left->op == REGVAR ||
		    e->left->op == INBC || e->left->op == INDE;
		rinreg = e->right->op == REGVAR ||
		    e->right->op == INBC || e->right->op == INDE;

		if (rsym && linreg) {
			/* mirror it: a<b is b>a, and equality commutes */
			Expr *t = e->left;
			e->left = e->right;
			e->right = t;
			if (e->op == LT) e->op = GT;
			else if (e->op == GT) e->op = LT;
			else if (e->op == LE) e->op = GE;
			else if (e->op == GE) e->op = LE;
			lsym = 1;
			rinreg = linreg;
		}
		if (lsym && (rinreg || rsym)) {
			/* a bare SYM reduces to SYMREF first, emitting nothing */
			e->left = rewrite1(e->left);
			if (e->left->op == SYMREF)
				e->left = symtohl(e->left);
		}
	}

	/*
	 * A 32-bit binary operator, before the children are reduced: both
	 * operands want HL:DE, so the ordinary depth-first walk would put
	 * the second one on top of the first.
	 */
	if (e->left && e->right && !baseop(e->op)) {
		n = dolongbin(e);
		if (n)
			return n;
		/*
		 * If that declined, stop here.  The arithmetic rules carry no
		 * width in their patterns, so a 32-bit operator falls through
		 * into them and is quietly done sixteen bits wide - "a << 31"
		 * became seven add hl,hl and a comparison against the low
		 * half of the constant.  Refusing leaves a marker instead.
		 */
		if (islongop(e))
			return e;
	}

	/*
	 * A step of one is what inc and dec do; anything else is a
	 * pointer to something wider, and "p++" means "p += sizeof".
	 * The rules only ever emitted the single step, so stepping a
	 * short pointer moved it one byte and landed between elements.
	 *
	 * It is a compound assignment, so make it one and let that path
	 * have it - including the part that names a side-effecting
	 * location only once.  A prefix yields the new value either way;
	 * a postfix yields the old one, which this does not produce, so
	 * that is only converted where the value is being thrown away.
	 */
	/*
	 * A step by other than one, both fixes.  These were two blocks
	 * asking the same three questions - is there a child, is the
	 * amount other than one, is this a step at all - and the second
	 * only ever ran when the first had not, the first having already
	 * relabelled the node as a compound assignment.  One guard, and
	 * the arm below picks which.
	 */
	if (e->left && e->u.incdec.amt != 1 &&
	    (e->op == PREINC || e->op == PREDEC ||
	     e->op == POSTINC || e->op == POSTDEC)) {
		long amt = e->u.incdec.amt;
		unsigned char post = e->op == POSTINC || e->op == POSTDEC;
		unsigned char nop =
		    (e->op == PREINC || e->op == POSTINC) ? PLUSEQ : SUBEQ;

		if (!post || e->dest == DEST_NONE) {
			e->op = nop;
			e->right = mkconst(e->width, amt);
			label(e);
			assign(e, e->tgt ? e->tgt : R_HL);
		} else if (dupableloc(e->left)) {
		/*
		 * Postfix where the value is wanted: that is the value from
		 * before, so it has to be kept while the location is
		 * updated.  The stack is the only temporary there is.
		 */
		Expr *loc = e->left;
		Expr *val, *step;

		val = locvalue(dupexpr(loc), e->width);
		setdest(val, DEST_VALUE);
		/* the value must actually BE in HL before the push - a
		 * register home or an address form reduces to itself */
		freeexpr(valtohl(rewrite(val)));
		out("\tpush hl\n");

		step = mkbinary(nop, e->width, loc, mkconst(e->width, amt));
		setdest(step, DEST_NONE);
		freeexpr(rewrite(step));
		out("\tpop hl\n");

		e->left = 0;
		return donehl(e, INHL);
		}
	}

	/* x OP= y -> x = x OP y, before the children are reduced */
	if (baseop(e->op)) {
		unsigned char tgt = e->tgt ? e->tgt : R_HL;
		unsigned char dst = e->dest;

		n = lowercompound(e);
		if (n) {
			setdest(n, dst);
			label(n);
			assign(n, tgt);
			return rewrite1(n);
		}
		/*
		 * The lvalue has side effects, so it cannot be named twice
		 * the way the expansion above does - "*p++ += 5" has to
		 * increment p once.  Work the address out once and keep it
		 * on the stack, which serves as the temporary the tree has
		 * no way to spell, then read, update and store through it.
		 */
		n = docompound(e);
		if (n)
			return n;
	}

	/* ARGNODE: evaluate left, push, then process right chain */
	if (e->op == ARGNODE) {
		e->left = rewrite1(e->left);
		next = e->right;
		e->right = 0;  /* detach chain before step */
		/* Fixed-point on this ARGNODE */
		for (;;) {
			n = step(e);
			if (!n)
				break;
			n->left = rewrite1(n->left);
			e = n;
		}
		/* Now process next argument */
		if (next)
			rewrite1(next);
		return e;
	}

	/*
	 * The short-circuit operators, in flag context.
	 *
	 * Both used to assume their operands answered in Z - jump on z
	 * for "&&", on nz for "||" - and then to claim the result was in
	 * NZ.  Neither holds.  A signed comparison answers in the sign
	 * flag and an unsigned one in carry, so the branch went on a flag
	 * that meant nothing; and where an operand short-circuits, the
	 * flags arriving at the label are that operand's, in whatever
	 * encoding it used, not the NZ the result claimed.
	 *
	 * So the branch asks each operand which flag it answers in, and
	 * both paths are made to arrive with the same one: zero for
	 * false, non-zero for true, which is what the result then names.
	 */
	if (e->op == LAND || e->op == LOR) {
		int out_lbl = labelcnt++;
		int end_lbl = labelcnt++;
		int isand = (e->op == LAND);
		char *cc;

		e->left->dest = DEST_FLAGS;
		e->left = rewrite1(e->left);
		cc = isand ? falsecc(e->left) : truecc(e->left);
		outf("\tjp %s,_L%d\n", cc, out_lbl);

		e->right->dest = DEST_FLAGS;
		e->right = rewrite1(e->right);
		cc = isand ? falsecc(e->right) : truecc(e->right);
		outf("\tjp %s,_L%d\n", cc, out_lbl);

		/* both operands agreed with the expression: force the
		 * answer "&&" wants when true, "||" wants when false */
		out(isand ? "\txor a\n\tinc a\n" : "\txor a\n");
		outf("\tjp _L%d\n_L%d:\n", end_lbl, out_lbl);
		out(isand ? "\txor a\n" : "\txor a\n\tinc a\n");

		outf("_L%d:\n", end_lbl);

		/*
		 * Both paths leave a definite nought or one in A, so this
		 * serves either context: a flag for a branch, and the value
		 * itself where a number was wanted - which had no rule at
		 * all and reduced to nothing.
		 */
		if (e->dest == DEST_FLAGS) {
			n = mkcode(e->width, F_NZ);
			n->dest = DEST_FLAGS;
		} else {
			n = mkcode('B', R_A);
			n->op = INA;
			n->dest = e->dest;
		}
		freeexpr(e);
		return n;
	}

	/*
	 * QUES (ternary): cond ? then : else.
	 *
	 * Both arms have to leave their value in the same place for the
	 * expression to have one, and returning whichever node the then
	 * arm happened to reduce to did not arrange that.  Worse, a
	 * constant arm reduces to itself - a bare NUMBER matches no rule,
	 * because as an operand it has to stay a NUMBER for the ",N)"
	 * rules - so "x ? 1 : 0" emitted two empty branches and handed
	 * back the constant 1 as though it were the answer.
	 *
	 * Landing each arm in HL through ASSIGN(INHL, arm) settles both:
	 * it reuses the whole =(H,...) rule set, which knows how to put a
	 * constant there, and it puts both arms in the same register.
	 * RETURN and argument pushing land a value the same way.
	 */
	if (e->op == QUES && e->right->op == TERNBRANCH) {
		int lbl = labelcnt++;
		Expr *tb = e->right;
		unsigned char dest = e->dest;
		unsigned char tgt = e->tgt ? e->tgt : R_HL;
		char *cc;

		/* the condition, and the branch that skips the then arm.
		 * falsecc may emit the zero test it needs, so it has to run
		 * before any of the jump is written */
		e->left->dest = DEST_FLAGS;
		e->left = rewrite1(e->left);
		cc = falsecc(e->left);
		outf("\tjp %s,_T%d\n", cc, lbl);

		branchval(tb->left);
		outf("\tjp _E%d\n_T%d:\n", lbl, lbl);
		branchval(tb->right);
		outf("_E%d:\n", lbl);

		e->left = 0;
		tb->left = 0;
		tb->right = 0;
		n = mkcode(e->width, R_HL);
		n->op = INHL;
		n->dest = dest;
		freeexpr(e);
		return movetotgt(n, tgt);
	}

	/*
	 * Handle long (32-bit) binary operations - the path for the ones
	 * dolongbin did not take.  Values are in HL':HL and DE':DE; see
	 * libsrc/libc/QLONG.md.
	 */
	if ((e->width == 'l' || e->width == 'L') && e->left && e->right) {
		char *helper = 0;
		int iscompare = 0;
		Expr *tmp;

		/* GT and LE need operand swap: a>b becomes b<a, a<=b becomes b>=a */
		if (e->op == GT || e->op == LE) {
			tmp = e->left;
			e->left = e->right;
			e->right = tmp;
		}

		switch (e->op) {
		case PLUS:   helper = "qadd"; break;
		case MINUS:  helper = "qsub"; break;
		case STAR:   helper = "qmul"; break;
		case DIV:    helper = (e->width == 'l') ? "qdiv" : "qudiv"; break;
		case MOD:    helper = (e->width == 'l') ? "qmod" : "qumod"; break;
		case AND:    helper = "qand"; break;
		case OR:     helper = "qor"; break;
		case XOR:    helper = "qxor"; break;
		case EQ: case NEQ: case LT: case GT: case LE: case GE:
			helper = (e->width == 'l') ? "qcmp" : "qucmp";
			iscompare = 1;
			break;
		}

		if (helper) {
			/*
			 * The right operand first, then out of the way.
			 *
			 * And each of them checked.  rewrite1 reduces what
			 * it can and returns what it could not, and nothing
			 * here used to look: an operand that did not reduce
			 * emitted no code at all, pushlong pushed whatever
			 * hl happened to hold, and the helper ran on it.
			 *
			 *	t = nd - (&buf[33] - b);
			 *
			 * came out 65337 instead of 77 for want of one rule,
			 * and compiled without a word - the missing rule was
			 * a bug, and the silence was this.  The same check
			 * stands over the store path below, put there when a
			 * multiply did the same thing; this path never got
			 * it.
			 */
			e->right = rewrite1(e->right);
			longincomplete(e->right);
			pushlong();
			e->left = rewrite1(e->left);
			longincomplete(e->left);
			poplongd();
			outf("\tcall %s\n", helper);
			/*
			 * A comparison answers in a flag, and which one
			 * depends on the signedness: qcmp reports in the
			 * sign flag and qucmp in carry.  This used to call
			 * lrelop for both and read carry for both, so every
			 * signed long comparison down this path was answered
			 * as an unsigned one.  longflag knows the rule; the
			 * operands were swapped above for GT and LE, so it is
			 * asked about the operator as written.
			 */
			if (iscompare) {
				n = mkcode('b',
				    longflag(e->op, e->width == 'l'));
				n->dest = DEST_FLAGS;
			} else {
				/* the value is in HL':HL, named by HL */
				n = mkcode(e->width, R_HL);
				n->dest = e->dest;
			}
			freeexpr(e);
			return n;
		}
	}

	/*
	 * Handle spill for expressions needing > 2 registers.
	 * Exclude ASSIGN - left side is target, not operand - and a
	 * constant right operand, which needs no register and which
	 * nothing would materialise if it were pushed and popped around.
	 */
	if (e->regs >= 3 && e->left && e->right && e->op != ASSIGN &&
	    e->right->op != NUMBER) {
		/* Evaluate left subtree (result in HL) */
		e->left = rewrite1(e->left);
		/*
		 * A SYMREF is left unreduced on purpose, so the rules can
		 * take it as an address rather than loading it.  That means
		 * it emits no code, and the push below spills whatever the
		 * last statement left in HL - which the other operand is
		 * then added to, and the sum read as a pointer.
		 */
		/*
		 * The value has to actually BE in HL before the push: a
		 * register variable, a SYMREF or an INDEX reduces to
		 * itself by design - the rules read them in place.
		 * Without this, "p += 1 + (w ? 2 : 1) + len" with p in BC
		 * pushed whatever the condition had left in HL and
		 * marched p off into it - which is how cpp's define store
		 * walked garbage and wrote six interned names' ids over
		 * with it.
		 */
		e->left = valtohl(e->left);
		/*
		 * Unless it is a byte, which lands in A whatever target it
		 * was given.  Pushing HL then would spill the address the
		 * value was read through, and the operator would be applied
		 * to two addresses.
		 */
		if (e->left->op == INA) {
			bytepair(e);
		} else {
		/* Spill left result to stack */
		out("\tpush hl\n");
		/* Evaluate right subtree (result in HL) */
		e->right = valtohl(rewrite1(e->right));
		/*
		 * A byte right operand lands in A and valtohl leaves it
		 * there on purpose - the assignment path wants exactly
		 * that.  Here it met "pop de, relabel as INDE", which
		 * moved nothing: the operator then read whatever DE
		 * held, and "g >> *p" shifted by the spilled pointer.
		 * Widen it into DE for real, at its own signedness.
		 */
		if (e->right->op == INA) {
			if (ISSIGNED(e->right->width))
				out("\tld e,a\n\trla\n\tsbc a,a\n\tld d,a\n");
			else
				out("\tld e,a\n\tld d,0\n");
			out("\tpop hl\n");
			lw = e->left->width;
			rw = e->right->width;
			freeexpr(e->left);
			freeexpr(e->right);
			e->left = mkcode(lw, R_HL);
			e->left->op = INHL;
			e->right = mkcode(rw, R_DE);
			e->right->op = INDE;
			goto spilled;
		}
		/* Pop left result, exchange so left in HL, right in DE */
		out("\tpop de\n\tex de,hl\n");
		/*
		 * Now left in HL, right in DE - convert children to register
		 * nodes, each keeping the width it had.  Taking the parent's
		 * made every operand of a comparison unsigned, because a
		 * comparison yields ubyte whatever it compared: the signed
		 * rules ask about the operand's width and stopped matching,
		 * so "f(x) < g(y)" on two negative shorts read the carry and
		 * answered an unsigned question.
		 */
		lw = e->left->width;
		rw = e->right->width;
		freeexpr(e->left);
		freeexpr(e->right);
		e->left = mkcode(lw, R_HL);
		e->left->op = INHL;
		e->right = mkcode(rw, R_DE);
		e->right->op = INDE;
spilled:	;
		}
		/* Fall through to step() to apply operation */
	} else {
		/* Rewrite children first (depth-first) */
		/* Skip children marked nored (preserve for parent rules) */
		if (e->op == ASSIGN && e->left && e->right &&
		    !islocdesc(e->left) && !isdestreg(e->left) &&
		    e->left->op != DEREF &&
		    !islocdesc(e->right) && e->right->op != NUMBER) {
			Expr *addr;

			/*
			 * Storing through an address the tree has to work out,
			 * to a value it also has to work out - "arr[i] += n".
			 * Both want HL, so the address waits on the stack
			 * while the value is computed and comes back with the
			 * value beside it in DE, which is what the
			 * =(D(H),E) store rule expects.
			 *
			 * Only when the value really needs a register: a
			 * constant stores straight through the address, and
			 * spilling for that would just cost bytes.
			 *
			 * And only when the address really needs one, which
			 * is not known until it has been reduced.  "arr[2]"
			 * folds to a symbol and an offset and emits nothing
			 * at all, so there would be nothing in HL to push:
			 * the push would spill whatever the last statement
			 * happened to leave there and the store would go to
			 * that address.  A descriptor needs no register and
			 * no temporary, so it is simply used where it is.
			 */
			addr = rewrite1(e->left);
			/*
			 * A literal address - "*(int *)0x54 = v" - is a
			 * place as much as a frame slot is, and costs no
			 * register to name: ld (84),hl reaches it whole.
			 * But it arrives as a BARE NUMBER on the lvalue
			 * side, because an lvalue in this tree IS its
			 * address, while the load of one - the shape
			 * islocdesc names - is DEREF(NUMBER).  So the test
			 * just above does not see it here, the address is
			 * not in HL for the push either, and the store fell
			 * through to the marker below.
			 *
			 * Give it the DEREF the =(D(num),H) family expects,
			 * exactly as the sibling branch does for an address
			 * that folded to a descriptor, and the one
			 * instruction comes out.  Storing a CONSTANT there
			 * always worked, which is why this survived the
			 * first pass over the bug: a constant right operand
			 * fails the gate above and never comes here.
			 */
			if (addr->op == NUMBER) {
				e->left = mkunary(DEREF, e->width, addr);
				e->right = rewrite1(e->right);
				goto children_end;
			}
			if (islocdesc(addr)) {
				e->left = addr;
				e->right = rewrite1(e->right);
				goto children_end;
			}
			/*
			 * The address is only worth pushing if it is
			 * actually in HL.  If the subtree did not reduce -
			 * a rule missing somewhere below it - then nothing
			 * put it there, and the push spills whatever the
			 * last statement happened to leave: the store then
			 * goes to that address and writes over something
			 * unrelated.  The comment above says exactly this
			 * about "arr[2]", and islocdesc was the whole test
			 * for it, which only covers the shapes that reduce
			 * to a descriptor rather than the ones that do not
			 * reduce at all.
			 *
			 * That is not a case the root check can catch.  By
			 * the time it looks, the address has been replaced
			 * by the register nodes the store rule wants and
			 * the tree reads as fully reduced - which is how
			 *
			 *	parms[m->parmcount++] = permdup(s);
			 *
			 * in cpp's macdefine came to store through a stale
			 * HL, land on the input buffer, and eat a character
			 * out of the next line of every source that defines
			 * a function-like macro with an empty body.
			 *
			 * The right operand still has to be evaluated: it
			 * is a call in the case that found this, and its
			 * side effects are not optional.  Only the store is
			 * dropped, and the marker says so.
			 */
			if (!reduced(addr)) {
				e->right = rewrite1(e->right);
				nincomplete++;
		out("; XXXXXX incomplete: ");
#ifdef DEBUG
				dumpexpr(addr);
#endif
				outc('\n');
				freeexpr(addr);
				freeexpr(e->right);
				e->left = e->right = 0;
				return donehl(e, INHL);
			}
			out("\tpush hl\n");
			/*
			 * An address-as-value or register-homed right
			 * reduces to itself and emits nothing - spiltstore
			 * sends it through valtohl so the pop below cannot
			 * store the slot's own address; "paths[np++] =
			 * a + 2" filed the slot into itself that way.
			 */
			{
				Expr *w = spiltstore(e, addr);
				if (w)
					return w;
			}
			goto children_end;
		} else if ((e->op == AND || e->op == OR || e->op == XOR) &&
			   e->dest == DEST_FLAGS && e->left &&
			   (e->left->op == WIDEN || e->left->op == SEXT) &&
			   e->left->left && ISBYTE(e->left->left->width) &&
			   e->right->op == NUMBER &&
			   (e->right->u.val & 0xffL) == e->right->u.val) {
			/*
			 * A byte masked with a constant and asked for as a
			 * condition.  The integer promotions make it int
			 * arithmetic, so the byte was widened, masked
			 * sixteen bits wide, had its high half zeroed
			 * again because the constant has none, and was
			 * then tested with ld a,l / or h - nine
			 * instructions where "and n" already sets Z on the
			 * answer.  Nothing downstream can tell: only the
			 * flags are wanted, and the bits the mask keeps
			 * all live in the low byte.  Worth a kilobyte
			 * across the three passes, which are written in
			 * flag tests over storage classes and type bits.
			 */
			Expr *w = e->left;

			e->left = w->left;
			w->left = 0;
			freeexpr(w);
			e->width = e->left->width;
			e->right->width = e->width;
			label(e);
			assign(e, e->tgt ? e->tgt : R_HL);
			e->dest = DEST_FLAGS;
			return rewrite1(e);
		} else if (e->op == AND && e->dest == DEST_FLAGS &&
			   e->left->op == DEREF &&
			   ISBYTE(e->left->width) &&
			   e->right->op == NUMBER &&
			   ispow2(e->right->u.val) > 0) {
			/*
			 * One bit of a byte in memory, asked for as a
			 * condition.  The Z80 tests it where it lies - bit
			 * 4,(ix+3) - but only if the rule can still see
			 * where that is, and reducing the left operand the
			 * usual way loads it into A first and leaves the
			 * rule nothing to match.  So the address underneath
			 * is reduced and the DEREF left standing, which is
			 * what the ASSIGN lvalue below does for the same
			 * reason.
			 *
			 * Where the address lands decides whether there is a
			 * rule to match at all, so the DEREF is only left
			 * standing for the three that have one.  Anything
			 * else is reduced the ordinary way - a global among
			 * them, whose "ld a,(nn)" is the shortest the Z80
			 * has and which bit cannot address regardless.
			 * Leaving it standing with nothing to match would
			 * emit no code at all.
			 */
			n = rewrite1(e->left->left);
			e->left->left = n;
			if (!(n->op == INDEX || n->op == INHL ||
			      (n->op == REGVAR && n->u.var.reg == R_IX)))
				e->left = rewrite1(e->left);
			goto children_end;
		} else if (e->left && e->left->op == REGVAR &&
			   ISBYTE(e->width) &&
			   (e->op == ASSIGN ||
			    e->op == PREINC || e->op == PREDEC ||
			    e->op == POSTINC || e->op == POSTDEC)) {
			/*
			 * A byte register variable being assigned to, or
			 * stepped.  Leave it alone: reducing it turns it into
			 * A, the ?(V,..):b rules that would have written b or
			 * c never match, and the A forms match instead - which
			 * work on a copy and stop there.  "c = -1" came out as
			 * "ld a,b" and "ld a,-1", and c kept whatever it had;
			 * "while (--n)" decremented A, tested that, and never
			 * touched n, so it did not terminate.
			 *
			 * Only bytes.  A word register variable reduces to
			 * INBC and has a working family of its own.
			 */
			;
		} else if (e->op == ASSIGN && e->left &&
			   e->left->op == DEREF &&
			   (islocdesc(e->left->left) ||
			    isdestreg(e->left->left) ||
			    (e->left->left->op == DEREF &&
			     keepchain(e->left->left)))) {
			/*
			 * An assignment's lvalue is a location, not a value.
			 * Reduce the address underneath but leave the DEREF
			 * standing, so the =(D(..),..) store rules can still
			 * see it - reducing it here would apply a load rule
			 * and quietly turn the store into a fetch.
			 *
			 * Only where the child's VALUE is the target,
			 * though: an operand - =(D(V)) stores at the
			 * register, =(D(I)) loads the slot and stores
			 * through - or another DEREF, whose reduction loads
			 * the pointer and leaves the address wanted in HL.
			 *
			 * A DEREF over address ARITHMETIC is different: the
			 * sum is where a pointer LIVES, and this DEREF is
			 * the load that fetches it.  "*f->_base = c"
			 * reaches here as D(+(D(I),4)), and keeping the
			 * DEREF while reducing only the sum left the
			 * ADDRESS of _base in HL for a rule that stores at
			 * HL - the character went into the pointer member,
			 * not through it, and every 513th byte of a
			 * buffered stream overwrote the buffer pointer's
			 * low byte.  Those fall through to the branch
			 * below, which reduces the whole load - the
			 * pointer's VALUE lands in HL - and wraps it in
			 * the DEREF the store rules expect.
			 */
			e->left->left = rewrite1(e->left->left);
			/*
			 * The address may now be LIVE in a register the
			 * value is about to need.  A location descriptor is
			 * safe - the store rule reads it after the value is
			 * computed - and so is a value that needs no code:
			 * a constant, a descriptor, something already in a
			 * register.  Anything else clobbers HL on its way,
			 * and the address has to wait on the stack -
			 * "**args++ = v" computes the target through the
			 * stepped pointer, and the value's own load then
			 * wrote over it: doscan stored nothing into two of
			 * scanf's conversions, and no rule could have
			 * matched the wreck - =(D(H),H) names one register
			 * holding two values.
			 */
			if (!islocdesc(e->left->left) && e->right &&
			    !islocdesc(e->right) && !isdestreg(e->right) &&
			    e->right->op != NUMBER) {
				Expr *w;


				e->left->left = valtohl(e->left->left);
				out("\tpush hl\n");
				w = spiltstore(e, e->left);
				if (w)
					return w;
				goto children_end;
			}
		} else if ((e->op == PREINC || e->op == PREDEC ||
			    e->op == POSTINC || e->op == POSTDEC) &&
			   e->left->op == DEREF &&
			   e->left->left->op == REGVAR) {
			/*
			 * Stepping through a pointer kept in a register.
			 *
			 * Every other pointer works already, because the
			 * ordinary reduction of DEREF(pointer variable) loads
			 * the pointer, and a loaded pointer in HL is exactly
			 * what "step through an address in HL" wants.  A
			 * register variable has nothing to load: the reduction
			 * applies a load rule anyway and fetches what p points
			 * at, so the step ran on the value - and at short
			 * width on that value used as an address again, making
			 * "(*p)++" step what *p pointed at.
			 *
			 * Keeping the DEREF and reducing only underneath is
			 * what the assignment above does, for the same reason:
			 * dropping it leaves a bare INBC, and "i(B)" - step BC
			 * itself - is a real rule that would then match, which
			 * is the "no way to tell *p = x from p = x" that the
			 * pass1 side of this warns about.  The ?(D(B)) rules
			 * name the shape that is left.
			 */
			e->left->left = rewrite1(e->left->left);
		} else if (e->op == ASSIGN && e->left &&
			   !islocdesc(e->left) && !isdestreg(e->left)) {
			/*
			 * An lvalue that is neither a location descriptor nor
			 * a destination register is an address the tree works
			 * out - an array element, say.  Reduce it, and unless
			 * it folded into a descriptor of its own (a constant
			 * subscript becomes a SYMREF) give it the DEREF the
			 * store rules expect, so such a store needs no rules
			 * of its own.
			 */
			Expr *addr = rewrite1(e->left);
			if (islocdesc(addr)) {
				e->left = addr;
			} else if (e->right && !islocdesc(e->right) &&
				   !isdestreg(e->right) &&
				   e->right->op != NUMBER) {
				/*
				 * The address is live in HL and the value
				 * is about to be computed over it.  Same
				 * collision, same answer as the sibling
				 * branch above: the address waits on the
				 * stack.  This is the shape doscan's
				 * "**args++ = v" arrives in - the target
				 * address worked out through the stepped
				 * pointer, then the value's own load - and
				 * without the spill the value load wrote
				 * over the address and no rule could match
				 * =(D(H),H): one register, two values.
				 */
				Expr *w;

				addr = valtohl(addr);
				out("\tpush hl\n");
				w = spiltstore(e, addr);
				if (w)
					return w;
			} else {
				e->left = mkunary(DEREF, e->width, addr);
			}
		} else if (e->op != COMMA && e->left && e->right &&
			   e->left->regs <= 1 &&
			   (e->right->regs > e->left->regs ||
			    (e->right->regs == e->left->regs &&
			     needshl(e->left) && needshl(e->right))) &&
			   !(e->left->nored & NR_NORED) && !(e->right->nored & NR_NORED)) {
			/*
			 * Sethi-Ullman: work out the costlier side first, so
			 * the cheaper one can follow without spilling.  Each
			 * still ends up in the register it was assigned, so a
			 * non-commutative operator is unaffected - only the
			 * order of evaluation changes, which C leaves open.
			 *
			 * Held to a left side costing one register, because
			 * that is the case that cannot disturb DE while the
			 * right operand is sitting in it.  And never for the
			 * comma, whose order is the whole point of it.
			 *
			 * At or above, not above.  Two sides that each cost
			 * one register - "*p - *q" - left the first in HL and
			 * then reduced the second, which needs HL to load
			 * through and took it: the left operand was gone
			 * before the operator ever ran, and no rule spells
			 * -(H,H) so nothing was emitted at all.  Doing the
			 * right first parks it in DE, where the operator
			 * wants it.  A comparison function is written this
			 * way, so qsort compared nothing and reversed its
			 * input instead of sorting it.
			 */
			unsigned char rtgt = e->right->tgt;

			e->right = rewrite1(e->right);
			e->right = movetotgt(e->right, rtgt);
			e->left = rewrite1(e->left);
			goto children_end;
		} else if (!e->left || !(e->left->nored & NR_NORED)) {
			e->left = rewrite1(e->left);
		}
		if (!e->right || !(e->right->nored & NR_NORED)) {
			if (e->left && e->left->op == INA && e->right &&
			    e->op != ASSIGN && e->right->regs > 0 &&
			    e->right->op != NUMBER && ISBYTE(e->right->width))
				bytepair(e);
			else
				e->right = rewrite1(e->right);
		}
	children_end: ;
		/*
		 * The fixups below all ask whether this is a narrow node,
		 * and most of them whether both children are there.  They
		 * were four standalone blocks asking it four times over.
		 * Nothing here nulls a child - the two that replace one
		 * put a register node in its place - so one answer serves.
		 */
		narrow = !ISLONG(e->width);
		both = e->left && e->right;
		/*
		 * A long stored into something narrower.  Under HL':HL the
		 * low word is already in HL, which is where every narrowing
		 * store rule looks, so all that is left to do is say so -
		 * the width has to change or the store rules go on treating
		 * it as four bytes.
		 *
		 * This used to be an ex de,hl as well, the low word having
		 * been in DE.  Getting it wrong is quiet: "buf[1] = val &
		 * 0xff" was right and "buf[2] = (val >> 8) & 0xff" was not,
		 * which made it look like a shift bug - the first reads the
		 * low byte straight out of memory and never goes through a
		 * register pair at all.  cpp emits every number this way,
		 * so every constant above 255 lost its high bytes: 0644
		 * arrived as 164 and 256 as 0.
		 */
		if (e->op == ASSIGN && narrow &&
		    e->right->op == INHL &&
		    ISLONG(e->right->width)) {
			e->right->width = T_SHORT;
		}

		/*
		 * The right operand of a comparison left standing as an
		 * INDEX.  A local array's name is its address, so "p !=
		 * local" and "p < def + 32" reduce to a frame descriptor
		 * that no comparison rule has a form for - the table has
		 * (x,E), (x,H), (x,B) and nothing against (iy+d), because
		 * an address is not what a frame slot usually means.
		 *
		 * The value wanted is the descriptor's ADDRESS, and it
		 * belongs in DE beside the left operand in HL, which is
		 * where every comparison rule expects to find it.  Doing it
		 * here rather than as a rule is the point: a rule matching a
		 * bare INDEX is greedy - it steals the (reg+d) addressing
		 * forms before the load and store rules ever see them, which
		 * is why the one that used to exist was taken out.
		 *
		 * qsort, doscan and cpp's filtenum all compare a walking
		 * pointer against the end of a local buffer, and all three
		 * branched on stale flags.
		 */
		/*
		 * The same descriptor as the VALUE being stored: "*pp =
		 * local" files a local array's address through a pointer.
		 * The store rules take their value from HL, and the left
		 * here is a dereferenced register home, so HL is free -
		 * valtohl knows how to work an (iy+d) into it.
		 */
		if (e->op == ASSIGN && both &&
		    e->right->op == INDEX && narrow &&
		    e->left->op == DEREF && e->left->left &&
		    (e->left->left->op == INBC ||
		     (e->left->left->op == REGVAR &&
		      e->left->left->u.var.reg == R_IX)))
			e->right = valtohl(e->right);

		/*
		 * The descriptor on the LEFT of a subtraction: "&buf[NDIG]
		 * - cp", the span of what a routine has filled in so far.
		 * PLUS has +(I,H), +(I,E) and +(I,B) because a subscript
		 * reaches the frame slot without a register, but MINUS does
		 * not commute and there is no -(I,x) at all - so nothing
		 * matched and nothing was emitted.  Every -(H,x) form does
		 * exist, so putting the address in HL is all it takes.
		 *
		 * Only where HL is free: against a right already in HL the
		 * existing -(H,I) rule is the better code.
		 *
		 * _pnum measures its digit run this way, so printf("%d")
		 * printed one character of any number and dropped the rest.
		 *
		 * Forming the address costs DE when the slot has an offset,
		 * and the right operand has already been worked out by now -
		 * "buf + 6 - q" loaded q into DE and then overwrote it with
		 * the offset, subtracting the offset from itself.
		 */
		if (e->op == MINUS && both &&
		    e->left->op == INDEX && narrow &&
		    reduced(e->right) && e->right->op != INHL) {
			int keepde = e->right->op == INDE &&
			    e->left->u.var.off;

			if (keepde)
				out("\tpush de\n");
			e->left = valtohl(e->left);
			if (keepde)
				out("\tpop de\n");
		}


		if (both && e->right->op == INDEX &&
		    narrow && reduced(e->left) &&
		    (e->op == EQ || e->op == NEQ || e->op == LT ||
		     e->op == GT || e->op == LE || e->op == GE ||
		     /*
		      * and the difference or sum against one, "p - def",
		      * which is how the span is read back afterwards.
		      *
		      * A sum is let through to +(I,H), which forms the
		      * address the way this does and costs less.  A
		      * difference is not: -(H,I) read the two bytes AT the
		      * slot instead, and a bare INDEX operand is never a
		      * value - a scalar's contents arrive as D(I) and
		      * reduce through the load rules well before here.  So
		      * "q - buf" subtracted buf[0] and buf[1] from q.  The
		      * rule is gone and this covers the shape; the push
		      * below is what makes a left in HL safe.
		      */
		     e->op == MINUS ||
		     (e->op == PLUS && e->left->op != INHL))) {
			char rw = e->right->width;
			unsigned char reg = e->right->u.var.reg;
			short off = e->right->u.var.off;
			/* HL only has to be kept if the left operand is in it -
			 * against a register home it is free to use */
			char keep = e->left->op == INHL;

			if (keep)
				out("\tpush hl\n");
			outf("\tpush %s\n\tpop hl\n",
			    idxregname(reg ? reg : R_IY));
			if (off)
				outf("\tld de,%d\n\tadd hl,de\n", off);
			out("\tex de,hl\n");
			if (keep)
				out("\tpop hl\n");
			freeexpr(e->right);
			e->right = mkcode(rw, R_DE);
			e->right->op = INDE;
		}
	}

	/* Fixed-point: keep rewriting until no change */
	for (;;) {
		unsigned char tgt = e->tgt;
		/* Re-label and re-assign after transformations */
		label(e);
		assign(e, tgt);
		n = step(e);
		if (!n)
			break;
		/* Transformation may create new children - rewrite them */
		n->left = rewrite1(n->left);
		n->right = rewrite1(n->right);
		e = n;
	}
	return e;
}

/*
 * Public entry point
 */
/*
 * A constant that survived as the whole expression.  This is only
 * reachable at the root - as an operand a NUMBER has to stay a NUMBER
 * for the ",N)" rules to match it, which is why there is no plain "N"
 * rule in the table.  What it has to produce depends on where the
 * value was wanted: nothing at all for a statement, a flag for a
 * condition, HL for a value.
 */
Expr *
constresult(Expr *e)
{
	long v = e->u.val;
	Expr *n;

	switch (e->dest) {
	case DEST_NONE:
		/* a constant statement does nothing */
		n = mkcode(e->width, 0);
		break;
	case DEST_FLAGS:
		/*
		 * Z means false.  xor a clears A and sets Z; inc a then
		 * makes it nonzero for a true constant.
		 */
		out("\txor a\n");
		if (v)
			out("\tinc a\n");
		n = mkcode(e->width, F_NZ);
		break;
	default:
		outf("\tld hl,%d\n", (int)v);
		n = mkcode(e->width, R_HL);
		n->op = INHL;
		break;
	}
	n->dest = e->dest;
	freeexpr(e);
	return n;
}

/*
 * Branch-chained conditions.  An if's condition wants a jump taken
 * when the expression is false; the old path rewrote && and || to
 * a nought-or-one in A and then tested A - six bytes of join and a
 * retest per operator, six hundred xor a's across this pass alone.
 * Here the short-circuit IS the branch: every conjunct jumps
 * straight to the consumer's label, and nothing materialises.
 *
 * condgo(e, lbl, wf): emit code that jumps to lbl when e is false
 * (wf=1) or true (wf=0), consuming e.  Leaves go through the
 * ordinary flag-context rewrite and one conditional jump.
 */
void
condleaf(Expr *e, char *lbl, int wf)
{
	char *cc;

	e->dest = DEST_FLAGS;
	label(e);
	assign(e, R_HL);
	e = rewrite1(e);
	cc = wf ? falsecc(e) : truecc(e);
	outf("\tjp %s,%s\n", cc, lbl);
	freeexpr(e);
}

void
condgo(Expr *e, char *lbl, int wf)
{
	Expr *l, *r;
	int op;
	char sc[12];

	op = e->op;
	if (op == LAND || op == LOR) {
		l = e->left;
		r = e->right;
		e->left = e->right = 0;
		freeexpr(e);
		if ((op == LAND) == (wf != 0)) {
			/* every operand agrees with the jump: chain them */
			condgo(l, lbl, wf);
			condgo(r, lbl, wf);
		} else {
			/* the left short-circuits PAST the test instead */
			fmtstr(sc, "_C%d", labelcnt++);
			condgo(l, sc, !wf);
			condgo(r, lbl, wf);
			outf("%s:\n", sc);
		}
		return;
	}
	if (op == BANG) {
		l = e->left;
		e->left = 0;
		freeexpr(e);
		condgo(l, lbl, !wf);
		return;
	}
	condleaf(e, lbl, wf);
}

void
condfalse(Expr *e, char *lbl)
{
	normtree(e);
	condgo(e, lbl, 1);
}

Expr *
rewrite(Expr *e)
{
	Expr *r;

#ifdef DEBUG
	if (VERBOSE(V_REWRITE)) {
		out("; --- raw ---\n");
		dumpexpr(e);
	}
#endif

	/* Canonicalize operand order before anything is labeled */
	normtree(e);

	/* Label nodes with register requirements */
	label(e);

	/* Assign target registers based on labels */
	assign(e, R_HL);  /* root expression targets HL */

#ifdef DEBUG
	if (VERBOSE(V_REWRITE)) {
		out("; --- labeled ---\n");
		dumpexpr(e);
	}
#endif

	r = rewrite1(e);

	/* A constant is only left standing when it is the whole thing */
	if (r->op == NUMBER)
		r = constresult(r);

	/*
	 * Check if code generation is incomplete.
	 *
	 * A tree that reduced is a single register node with nothing
	 * under it, so children left standing mean a rule was missing
	 * somewhere below - and only the root used to be looked at.  A
	 * parent that still matched hid it: "arr[i] = i * a" with i in a
	 * register has no rule for multiplying BC by DE, so the multiply
	 * and its left operand emitted nothing, the store above them
	 * matched anyway, and the wrong value went into the array with
	 * nothing said.
	 */
	if (r && !reduced(r)) {
		nincomplete++;
		out("; XXXXXX incomplete: ");
#ifdef DEBUG
		/* dumpexpr ends the line */
		dumpexpr(r);
#else
		/*
		 * The newline is not decoration.  Without DEBUG nothing
		 * ended the comment, the NEXT emission joined it, and the
		 * assembler never saw that instruction - the self-hosted
		 * c1 marked drop_assigns incomplete and then commented
		 * out its own push ix.
		 */
		outc('\n');
#endif
	}

#ifdef DEBUG
	if (VERBOSE(V_REWRITE)) {
		out("; --- rewritten ---\n");
		dumpexpr(r);
	}
#endif
	return r;
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */

