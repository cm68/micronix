/*
 * AST serialization for second pass - binary format
 */
#include <stdlib.h>
#include "p1core.h"
#include "p1expr.h"
#include "p1type.h"
#include "p1name.h"
#include "p1lex.h"
#include "p1outh.h"

/* Forward declarations */
extern int analyzeFunc(struct name *func);  /* regalloc.c */
extern int frameSaveBase;                   /* regalloc.c */

/* Current function's locals from phase 1 - for frm_off/reg lookup */
struct local *curFuncLocals = NULL;

/*
 * The function being emitted, for naming its statics.  A static in a
 * function is qualified by the function it is in - that is what makes
 * it unique inside the file, and it is what the source says about it.
 */
unsigned short curFuncId = 0;

/*
 * Set while emitting the lvalue of an assignment, and consumed by the
 * first node emitted after that - see the DEREF case in emitExpr.
 */
static char inLvalue;


/*
 * Helper: emit child expression (if non-null)
 */
void
emitChild(struct expr *e)
{
	if (e)
		emitExpr(e);
}

/*
 * pass2 takes the signedness of a comparison from its operands' width
 * letters, and an address arrives there as a plain sixteen bit integer
 * - a signed one, because pointers have no type of their own that far
 * down.  Two addresses either side of 0x8000 then compare backwards.
 *
 * Only the comparison needs saying: a store or a load of an address
 * does not care which way it would have been signed, and retyping
 * those would just miss the rules that already match them.  So the
 * ordered comparisons retype their own operands here, and nothing else
 * changes.  See SIGNEDPOINTER.
 */
static void
cmpunsigned(struct expr *e)
{
	if (e && e->type && (e->type->flags & (TF_POINTER | TF_ARRAY | TF_FUNC)))
		e->type = ushorttype;
}

/*
 * Emit an operand of an operator that works at width t, widening it
 * first if it is narrower.  Signed sources sign-extend and unsigned
 * ones zero-extend, which is precisely why the tree has to carry the
 * conversion rather than leaving pass2 to guess: the instructions
 * differ, and the node type is the only thing that knows which.
 */
void
emitOperand(struct expr *e, struct type *t)
{
	if (!e)
		return;
	/*
	 * Pointers, arrays, and functions keep their width: their value
	 * is an address, whatever their element size says.  An array
	 * member returned from a function otherwise picked up a SEXT of
	 * its first element's width, and pass2 dutifully loaded the two
	 * bytes the address pointed at instead of the address - which is
	 * how cpp's intern() returned the spelling's first characters as
	 * the canonical pointer.
	 */
	if (t && e->type->size < t->size &&
	    !(e->type->flags & (TF_POINTER | TF_ARRAY | TF_FUNC))) {
		if (e->op == CONST) {
			/*
			 * A constant is the same value at any width, so it
			 * just gets the wider type - wrapping it would break
			 * every rule that wants a literal operand.
			 */
			e->type = t;
		} else {
			emit1((e->type->flags & TF_UNSIGNED) ? WIDEN : SEXT);
			emit1(typeSfx(t));
		}
	}
	emitExpr(e);
}

/*
 * Emit the right hand side of an assignment, narrowed to what is being
 * assigned to.
 *
 * emitOperand() converts one way only - it widens, and an operand wider
 * than the target was emitted at its own width and left there.  Nothing
 * noticed while the only wide type arrived through a DEREF, because the
 * assignment path narrowed those itself: candemote() said the low bytes
 * were the ones lying at the object's own address and demote() retyped
 * the tree to take them, which costs no code.
 *
 * That stopped being true of a long when the high word moved to the
 * lower address (QLONG.md, NUXI).  The low half is two bytes along, so
 * candemote() refuses now, and with nothing else to catch it
 *
 *	f->_cnt -= roffs;	int -= long
 *	f->_ptr += roffs;	char * += long
 *
 * in libcpm's fseek reached pass2 as a short SUB with a long operand
 * under it.  No rule matches that shape, so no code was emitted for
 * either statement - the compiler said so, but only in a count that
 * the rule-coverage corpus was throwing away.
 *
 * NARROW is the honest spelling and costs nothing: a long is held in
 * HL':HL with its low word in HL, which is where a short already is.
 *
 * This is deliberately not in emitOperand().  That is called on lvalues
 * too, where the node carries the type of the object rather than of a
 * value being converted - "&lv + 2" over a long lv arrives as an INDEX
 * of type long that is really an address, and narrowing it there put a
 * NARROW around addresses in twelve of the runtime tests.
 */
static void
emitAssignRHS(unsigned char op, struct expr *e, struct type *t)
{
	/*
	 * Plain assignment is not the case: pass2 stores a long into a
	 * short by storing the half it wants, and has rules that say so.
	 * It is the ten compound forms that break, because pass2 takes
	 * "a += b" apart into an assignment of "a + b" and that inner
	 * operator is the one left holding operands of two widths.
	 * Narrowing a plain one instead put a NARROW around a long local
	 * that pass2 was reading perfectly well.
	 */
	if (op == ASSIGN)
		return emitOperand(e, t);

	/*
	 * Folded first because the constant may not be one yet - a
	 * character literal arrives as a cast around a number and only
	 * becomes 115 further down, and wrapping that in a NARROW made a
	 * shape with no rule where retyping the literal costs nothing.
	 */
	if (e)
		e = foldTree(e);

	if (e && t && e->type->size > t->size &&
	    !(e->type->flags & (TF_POINTER | TF_ARRAY | TF_FUNC))) {
		if (e->op == CONST || (e->flags & E_CONST)) {
			e->type = t;
		} else {
			emit1(NARROW);
			emit1(typeSfx(t));
			emitExpr(e);
			return;
		}
	}
	emitOperand(e, t);
}

/*
 * Does evaluating this subtree do anything besides produce a value?
 *
 * Asked of an lvalue that is about to be emitted a second time, so
 * the question is whether reading it twice is the same as reading it
 * once.  Anything that steps a variable, stores, or calls out is not,
 * and the answer has to be yes for the whole subtree - the address in
 * "*p++" is a plain DEREF at the top with the step underneath it.
 */
static int
sideeffect(struct expr *e)
{
	if (!e)
		return 0;
	if (e->op == INCR || e->op == DECR || e->op == CALL ||
	    e->op == BFASSIGN || isAssignOp(e->op))
		return 1;
	return sideeffect(e->left) || sideeffect(e->right);
}

/*
 * Output an expression in paren-free format
 * Constants: just the value (hex with dot)
 * Symbols: $name
 * Binary ops: op width left right
 * Unary ops: op width operand
 * Memory ops annotated with size: Mb expr, =l lvalue rvalue
 * Empty/null expression: _
 */
void
emitExpr(struct expr *e)
{
	/* Hoisted locals for stack reuse */
	struct name *np;
	struct expr *left, *right, *ep;
	struct type *type;
	unsigned char op, uc;
	char fullname[32], c, lval;
	/*
	 * An argument count and an initializer count fit a byte, and
	 * emit1 writes one byte of them regardless.  An element size
	 * does not: it is what ++ steps a pointer by, the INCR case
	 * below writes it with emit2, and a struct is allowed to be
	 * bigger than 255 bytes.
	 *
	 *	struct buf { struct buf *next, *prev; long block;
	 *		     char data[1024]; };	- 1032 bytes
	 *	for (bp = &bufs[0]; bp < &bufs[n]; bp++)
	 *
	 * stepped bp by 8, which is 1032 with the top of it gone, so
	 * the loop wrote five buffers into the first forty bytes of
	 * the first one and ran off the end of the arena.
	 */
	unsigned short n;

	/* Fold constants before emitting */
	e = foldTree(e);

	if (!e) {
		emit1(AST_EMPTY);
		return;
	}

	op = e->op;
	left = e->left;
	right = e->right;
	type = e->type;

	/* consume the lvalue flag: it applies to this node only */
	lval = inLvalue;
	inLvalue = 0;

	switch (op) {
	case CONST:
		emit1(NUMBER);
		emit1(typeSfx(type));
		emit4(e->v);
		break;

	case SYM:
		np = (struct name *)e->var;
		/*
		 * Local variables: emit LOCALVAR/REGVAR directly.
		 *
		 * Not a static one.  It is inside a function, so its level
		 * is above one, but it is not in the frame: its storage is
		 * emitted with the globals under an S<n> label, which the
		 * name path below produces.  Coming here addressed it as a
		 * frame slot instead, so the value did not survive the
		 * call that set it and a static array read whatever was on
		 * the stack - which is how pass1's own sclassBit, a static
		 * table, made the c0 that ccc built reject every typedef.
		 */
		if (np->level > 1 && !(np->sclass & (SC_EXTERN | SC_STATIC))) {
			/* Look up frm_off/reg from phase 1 captured locals */
			struct local *local = findInLocals(np);
			char reg = local ? localReg(local) : np->w.r.reg;
			short off = local ? localOff(local) : np->w.r.frm_off;
			if (reg) {
				emit1(REGVAR);
				emit1(typeSfx(type));
				emit1(reg);
			} else {
				emit1(LOCALVAR);
				emit1(typeSfx(type));
				emit2((unsigned short)off);
			}
			break;
		}
		/* extern/global get underscore prefix */
		if ((np->sclass & SC_EXTERN) ||
		    (np->level == 1 && !(np->sclass & SC_STATIC)))
			fmtstr(fullname, "_%s", nameOf(np->id));
		else if (np->sclass & SC_STATIC)
			staticName(fullname, np->id,
			    np->level > 1 ? curFuncId : 0,
			    np->static_id);
		else if (np->static_id)
			fmtstr(fullname, "L%d", np->static_id - 1);
		else
			fmtstr(fullname, "%s", nameOf(np->id));
		emit1(SYM);
		emitS(fullname);
		break;

	case STRING:
		/* String literals - reference by name (already emitted in phase 1) */
		np = (struct name *)e->var;
		emit1(SYM);
		emitS(nameOf(np->id));
		break;

	case CALL:
		/* Function call: CALL type count func arg1 arg2 ... */
		n = 0;
		c = typeSfx(type);
		/* Count arguments from the expression tree */
		for (ep = right; ep; ep = ep->next)
			n++;
		emit1(CALL);
		emit1(c);
		emit1(n);
		emitChild(left);
		for (ep = right; ep; ep = ep->next)
			emitChild(ep);
		break;


	case INCR:
	case DECR:
		/* Increment/decrement operators: emit with increment amount */
		/*
		 * mkIncDec settled the step when the node was built; the
		 * type here may have been rewritten since - see the note
		 * there, and cmpunsigned above.
		 */
		n = e->v;
		c = typeSfx(type);
		if (op == INCR)
			uc = (e->flags & E_POSTFIX) ? POSTINC : PREINC;
		else
			uc = (e->flags & E_POSTFIX) ? POSTDEC : PREDEC;
		emit1(uc);
		emit1(c);
		/*
		 * What a step steps is a location, exactly as an assignment's
		 * left side is, so the DEREF case below has to keep itself:
		 * without this "(*p)++" on a register variable dropped the
		 * DEREF and stepped the pointer instead of what it points at.
		 * Clean code, no marker, wrong answer - and at both widths.
		 */
		inLvalue = 1;
		emitChild(left);
		emit2(n);
		break;

	case BFEXTRACT:
		/* Bitfield extract: offset width addr */
		np = (struct name *)e->var;
		emit1(BFEXTRACT);
		emit1(np ? np->w.m.bitoff : 0);
		emit1(np ? np->w.m.width : 0);
		emitChild(left);
		break;

	case BFASSIGN:
		/* Bitfield assign: offset width addr value */
		np = (struct name *)e->var;
		emit1(BFASSIGN);
		emit1(np ? np->w.m.bitoff : 0);
		emit1(np ? np->w.m.width : 0);
		emitChild(left);
		emitChild(right);
		break;

	case QUES:
		/* Ternary: QUES width cond then else - flatten the COLON node */
		emit1(QUES);
		emit1(typeSfx(type));
		emitChild(left);
		emitChild(right->left);
		emitChild(right->right);
		break;

	case INITLIST:
		/* Nested initializer list - emit contents */
		n = 0;
		for (ep = left; ep; ep = ep->next)
			n++;
		emit1(BEGIN);
		emit1(n);
		for (ep = left; ep; ep = ep->next)
			emitExpr(ep);
		emit1(END);
		break;

	case DEREF:
		/*
		 * DEREF(REGVAR) is just the value - skip the DEREF.
		 *
		 * Not on an assignment's lvalue though.  The assignment
		 * parser already unwrapped one DEREF to get the address, so
		 * what is left says "the register holds the address of the
		 * target".  Dropping it here emits the same REGVAR that
		 * "i = x" on a register variable emits, and pass2 then has no
		 * way to tell "*p = x" from "p = x".
		 */
		if (isRegvar(left) && !lval) {
			/*
			 * Carry this node's type down.  The name underneath
			 * is emitted in place of the DEREF, and it is
			 * emitted with its OWN type - so anything the DEREF
			 * had been relabelled with was thrown away.
			 *
			 * A cast is exactly that relabelling: "(unsigned
			 * int)p" on a register variable rewrites the DEREF
			 * and leaves the name alone, so the cast vanished
			 * and the comparison that followed ran signed.
			 * cpp's own null-pointer guard read a stack address
			 * as negative and took itself out that way.  The
			 * two types are the same whenever nothing was cast,
			 * which is nearly always, so this costs nothing.
			 */
			struct type *save = left->type;

			left->type = type;
			emitExpr(left);
			left->type = save;
			break;
		}
		/*
		 * A store through a pointer that is itself a member load:
		 * "*q->p = c" with p the first member.  The address here
		 * is DEREF(REGVAR) - load the word the register points at
		 * - and the child's DEREF has to survive, because bare
		 * "=(D(V))" is already taken: it is how "*q = c" spells
		 * "the register holds the address".  Without this the
		 * child collapsed to the same REGVAR a plain "*q" emits,
		 * and pass2 stored the character into the pointer member
		 * itself.  stdio's _flsbuf does exactly this - *f->_base
		 * = c - and every 513th byte through a buffered stream
		 * overwrote the buffer pointer's low byte instead of
		 * landing in the buffer.  dchainreg, not isRegvar: the
		 * flag rides the whole spine, so each level's DEREF makes
		 * the same choice and depth survives intact.
		 */
		if (lval && left->op == DEREF && dchainreg(left->left))
			inLvalue = 1;	/* the child DEREF keeps itself */
		/*
		 * Optimize: *++p -> (++p, *p) using comma operator
		 *
		 * The second half has to be the value at p, and that is two
		 * fetches from a variable in memory: read the pointer, then
		 * read what it points at.  One DEREF over the name is only
		 * the first of them - it is how a plain "p" is spelled - so
		 * what came out was "(++p, p)" and the address went on in
		 * place of the value.  Nothing reported it: the tree is
		 * well formed, every node reduces, and pass2 emitted exactly
		 * what it was handed.  v6 ls walks argv with *++argv and got
		 * &argv, printed the bytes of a pointer as a filename, and
		 * then spun in qsort over a list it never filled.
		 *
		 * A register variable is the exception, and the reason is
		 * the one at the top of this case: it has no address, the
		 * value is the register, so "*p" is a single DEREF over it
		 * and this was right for that case all along.
		 *
		 * An lvalue wants the same two fetches, which is not what
		 * you would guess.  "*++p = c" never arrives here - the
		 * assignment parser unwraps it into ASSIGN(PREINC ...) -
		 * but "**++g = c" does, with the outer store unwrapped and
		 * "*++g" left as the address to store through.  That
		 * address is one fetch past the pointer's value, the same
		 * as the rvalue case, so lval is not asked.
		 *
		 * The suffix underneath is always 's': what a step is
		 * applied to is a pointer or an array, and typeSfx calls
		 * everything address-valued 's' whatever it points at.
		 */
		if ((left->op == INCR || left->op == DECR) &&
		    !(left->flags & E_POSTFIX)) {
			emit1(COMMA);
			emit1(typeSfx(type));
			emitExpr(left);
			emit1(DEREF);
			emit1(typeSfx(type));
			if (!isRegvar(left->left)) {
				emit1(DEREF);
				emit1('s');
			}
			emitExpr(left->left);
			break;
		}
		/* fall through - standard unary */

	case NARROW:
	case WIDEN:
	case SEXT:
	case BANG:
	case NEG:
	case NOT:
		/* Unary operators */
		emit1(op);
		emit1(typeSfx(type));
		/*
		 * Negation and complement are done at the promoted width, so
		 * what they are applied to has to get there too.  emitChild
		 * hands the operand over as it found it, which left the
		 * operator working at a width its operand was not: a byte in
		 * A under a short negation, which no rule names.
		 *
		 * Not "!", whose answer is an int however narrow the thing
		 * tested, and not DEREF, which falls through to here and
		 * whose operand is an address rather than a value.
		 */
		if (op == NEG || op == NOT)
			emitOperand(left, type);
		else
			emitChild(left);
		break;

	case GT:
	case GE:
		/* Normalize GT/GE to LT/LE by swapping operands.  Still has
		 * to convert them: swapping is not a reason to skip the
		 * promotion the comparison would otherwise get. */
		{
			struct type *w = opwidth(e);
			cmpunsigned(left);
			cmpunsigned(right);
			emit1(op == GT ? LT : LE);
			emit1(typeSfx(type));
			emitOperand(right, w);
			emitOperand(left, w);
		}
		break;

	default:
		/*
		 * A chained assignment whose inner target is a dereference:
		 *
		 *	*p = *q = 0;
		 *
		 * pass2 has no rule for an assignment whose value is another
		 * assignment's, when that one stored through a pointer, and
		 * emitted no code at all for the statement - both stores
		 * gone, counted as an expression it could not build.  The
		 * inner target being a plain variable has a rule and is left
		 * alone; this is only the shape that has none.
		 *
		 * Emitted as the comma it is equivalent to, the same way
		 * "*++p" becomes "(++p, *p)" above:
		 *
		 *	(*q = 0, *p = *q)
		 *
		 * Reading the inner target back rather than reusing the
		 * value is what keeps the conversion right: the value of
		 * "y = z" is z converted to y's type, so a narrow y between
		 * two wide ones has to narrow.  Emitting z twice would store
		 * the unconverted value in the outer target.
		 *
		 * It costs a second load, so only where the load can be
		 * repeated, and BOTH halves of the inner assignment have to
		 * be still for that.  The target, because "*p++ = *q++ = 0"
		 * would step q twice.  And the value, because it runs first
		 * and can move the target out from under the re-read:
		 *
		 *	foo = *bar = kee();
		 *
		 * where kee() assigns to bar.  The store goes to the address
		 * bar held, and a re-read of "*bar" afterwards is a read of
		 * somewhere else entirely - so foo would get whatever lives
		 * at the new address instead of what was just stored.
		 *
		 * Neither is rewritten.  They are left exactly as they were
		 * for pass2 to refuse, which is a failed compile and not a
		 * quiet miscompile.
		 */
		/*
		 * A word only, which is to say an int or a pointer.
		 *
		 * The read this puts back has to be one pass2 can build, or
		 * the rewrite trades a shape it cannot build for another
		 * one.  A long lands as a long load through an address in a
		 * register and a byte lands in E, and there is no rule for
		 * either - so "gl = *pl = 7L" and "g1_b = *pb = 5" reduce as
		 * they stand and would stop if this touched them, while
		 * "*p = *q = 7L" and the byte form of it stay unbuilt.  That
		 * is the same trade refused twice, and refusing it leaves
		 * every width no worse than it was.
		 *
		 * The rules to close those two are the other half of this,
		 * and they belong in pass2 - but c1 is over its budget on
		 * both targets, so they wait for room rather than crowd it.
		 */
		if (op == ASSIGN && right && right->op == ASSIGN &&
		    right->left && right->left->op == DEREF &&
		    right->type && right->type->size <= inttype->size &&
		    !sideeffect(right->left) && !sideeffect(right->right)) {
			emit1(COMMA);
			emit1(typeSfx(type));
			emitExpr(right);		/* the inner store */
			emit1(ASSIGN);
			emit1(typeSfx(type));
			inLvalue = 1;
			emitChild(left);
			/*
			 * Reading the inner target back means putting a DEREF
			 * on, not just emitting its node again.  The
			 * assignment parser has already taken one off to
			 * leave an address - what stands there says "the
			 * target is at this address", not "the value at it" -
			 * so emitting it as an operand stored the pointer
			 * itself: "*p = *q = 0" cleared *q and then set *p to
			 * q.  Clean code, no marker, wrong answer.
			 *
			 * The widening is what emitOperand would have done,
			 * done here because the DEREF has to go between it
			 * and the address.
			 */
			if (right->type && type &&
			    right->type->size < type->size &&
			    !(right->type->flags &
			      (TF_POINTER | TF_ARRAY | TF_FUNC))) {
				emit1((right->type->flags & TF_UNSIGNED) ?
				    WIDEN : SEXT);
				emit1(typeSfx(type));
			}
			emit1(DEREF);
			emit1(typeSfx(right->type));
			emitExpr(right->left);
			break;
		}
		/* All operators get width suffix */
		emit1(op);
		emit1(typeSfx(type));
		if (isAssignOp(op)) {
			/*
			 * The result is about to be narrowed to the target, so
			 * compute it narrow where that cannot change the stored
			 * value.  This is the as-if rule standing in for the
			 * integer promotions: C says "c1 + c2" is int
			 * arithmetic, but if it lands in a char only the low
			 * byte was ever observable.
			 */
			if (right && candemote(right, type->size))
				right = demote(right, type);
			/* mark the lvalue so DEREF above knows to keep itself */
			inLvalue = 1;
			emitChild(left);		/* a location, never widened */
			emitAssignRHS(op, right, type);	/* convert to the target */
		} else if (op == LAND || op == LOR) {
			/*
			 * The two sides of && and || are each tested against
			 * zero, separately, and short-circuited between - so
			 * there is no common width for them to meet at and
			 * nothing to convert them to.  The node's own type is
			 * uchar, because the answer is 0 or 1, and narrowing
			 * the operands to that drops exactly the bytes the
			 * zero test needs:
			 *
			 *	256 && 1	was false
			 *
			 * and a pointer whose low byte happened to be zero
			 * tested as null.  That last one is how it was found:
			 * cpp's conditional stack is a malloc'd list, and when
			 * the allocation landed on a 0x??00 address every
			 * "#if" in the file leaked its body.
			 */
			emitChild(left);
			emitChild(right);
		} else {
			struct type *w = opwidth(e);
			/* ordered comparisons of addresses go unsigned */
			if (op == LT || op == LE) {
				cmpunsigned(left);
				cmpunsigned(right);
			}
			/*
			 * An equality test between two byte-valued
			 * operands is a byte test.  The integer
			 * promotions make it int arithmetic - a byte is
			 * widened, masked sixteen bits wide against a
			 * constant with no high half, and compared with
			 * sbc hl,de - where "and n / cp m" says the same
			 * thing in a third of the bytes.  Equality is the
			 * safe half of this: it does not care which way
			 * either operand would have been signed, so long
			 * as both really fit in the byte.
			 *
			 * bytevalued() asks whether a value provably fits
			 * in an unsigned byte, which is not what
			 * candemote() asks - that one says only that the
			 * low bytes may be taken, and a long against zero
			 * passes it while comparing false as a byte.  The
			 * passes are written in flag tests over storage
			 * classes, type bits and rule flags, so this is
			 * worth better than a kilobyte.
			 */
			if ((op == EQ || op == NEQ) && w->size > 1 &&
			    bytevalued(left) && bytevalued(right))
				w = uchartype;
			/*
			 * emitOperand widens a narrow operand and leaves a wide
			 * one alone, which is right for everything except an
			 * operand wider than the operator itself works at.
			 * "buf[pos + i]" with pos a long adds a long to a
			 * pointer and the sum is a pointer: only the low word
			 * can reach the address, and pass2 has no rule for
			 * adding the two widths together - it emitted nothing.
			 *
			 * So narrow it here, on the same terms the assignment
			 * above narrows what it stores: only where the operator
			 * cannot carry anything down from the bytes being
			 * dropped.
			 */
			if (left->type->size > w->size &&
			    candemote(left, w->size))
				left = demote(left, w);
			if (right && op != LSHIFT && op != RSHIFT &&
			    right->type->size > w->size &&
			    candemote(right, w->size))
				right = demote(right, w);
			emitOperand(left, w);
			/* a shift count is promoted on its own, not to the
			 * width of the value being shifted */
			if (op == LSHIFT || op == RSHIFT)
				emitChild(right);
			else
				emitOperand(right, w);
		}
		break;
	}
}
