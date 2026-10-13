/*
 * parseast.c - AST parser with expression tree building
 */
#include "pass2.h"
#include "expr.h"
#include "opcodes.h"
#include "lexeme.h"
#include <stdio.h>

#ifdef DEBUG
#include "debug.h"

char *
stmtname(int op)
{
	switch (op) {
	case AST_BLOCK: return "BLOCK";
	case IF: return "IF";
	case RETURN: return "RETURN";
	case LABEL: return "LABEL";
	case GOTO: return "GOTO";
	case SWITCH: return "SWITCH";
	case CASE: return "CASE";
	case DEFAULT: return "DEFAULT";
	case ASM: return "ASM";
	case AST_FUNC: return "FUNC";
	default:  return "EXPR";
	}
}
#endif

static char buf[64];

/*
 * What a label the programmer wrote is spelled as.
 *
 * C keeps labels in a namespace of their own, so "out:" is a good
 * label and says nothing about any "out" elsewhere.  The assembler has
 * no such namespace: a line beginning "out" is the instruction, and
 * the label vanished into an opcode with a stray colon after it.
 * Every mnemonic is exposed this way, and "out", "end", "in", "set"
 * and "cp" are all names a person reaches for at the bottom of a loop.
 *
 * The prefix has to be something C cannot spell, or it only moves the
 * collision: '_' is what pass1 puts in front of every global, so a
 * label "out" and a function "out" would meet at "_out".  '@' meant
 * nothing to asz until alpha() was taught it, which is where the rest
 * of this note lives.
 *
 * This belongs here and not in pass1, where the label is still an @id
 * back-reference into the name file rather than its spelling; putting
 * anything in front of THAT '@' stops nidxp() recognising it and the
 * compressed form reaches the assembler intact.  By the time the name
 * has been read here it has been expanded, and definition and goto -
 * the only two records that carry one - are both below.
 */
#define LBLPFX	"@"

/* Label generation */
static int labelcnt;		/* per-function label counter */
static int fnindex;		/* function index for unique labels */

/* Current function state */
/*
 * Function name, as it goes into the assembly - so with the leading
 * underscore, which pass1 has already added.  A 15-character C
 * identifier becomes 16 here, and 16 is what the old size held with no
 * room for the terminator.  The object format's own limit is the
 * assembler's to complain about, and it does.
 */
/*
 * Case values that cannot be dispatched.  Counted rather than fatal on
 * the spot so one run names them all, and reported by pass2.c as a
 * failed compile - see the note at the CASE below.
 */
int nbadcase;

static char funcname[20];
static char funcstatic;		/* this function is not to be exported */
static short framesize;		/* bytes of local stack frame */
static short nparams;		/* how many parameters, for the frame test */
static char arg1w;		/* width of the first parameter: it arrives
				 * in HL (HL':HL when long), and the helper
				 * variant that spills it is chosen by this */
static char noframe;		/* this function needs no frame at all */
static short savebase;		/* scalar area size: save slots below it */
/*
 * Set from the header: phase 1 saw the whole body and says nothing in
 * it reaches through IY except possibly the single parameter that
 * arrives in HL.  Whether that one reference can be served from HL is
 * a question for the first expression, one pass later.
 */
static unsigned char framefree;
static unsigned char regsused;	/* bitmask of callee-save regs */
static short bcoff, ixoff;	/* IY-relative offsets for saved regs */

/*
 * Switch dispatch.
 *
 * The stream is sequential - SWITCH, then each CASE with its value and
 * its body - so the values are not all known before the first body has
 * to be emitted.  So the control value is worked out, the bodies are
 * jumped over, and the comparisons go after them, where every case
 * label is known.  Nothing falls into that block: the last body jumps
 * past it, and the bodies were jumped over rather than run, so the
 * control value is still in the register when the comparisons are
 * reached.
 *
 * Case values are bytes in this compiler, so the chain is a cp against
 * A.  A control expression need not be one - a state machine over an
 * int is the usual shape - so a word control tests its high byte once
 * before comparing the low one, and any value that does not fit a byte
 * cannot match and goes to the default.
 */
/*
 * Nested switches.  A level costs a context struct and nothing else
 * now that the values are pooled, so this can be generous: the
 * deepest nesting in this tree is two.
 */
#define MAXSWNEST 8		/* nested switches */
/*
 * Case values for every open switch, in one shared pool.
 *
 * This was MAXSWNEST fixed slices of 256 - every level provisioned for
 * the widest switch a byte can express, whether or not any level was
 * open.  Two kilobytes of bss, and bss is where the heap starts, so it
 * was two kilobytes c1 did not have to work in.  Nothing came near it:
 * the widest switch in the tree has 125 cases and nothing nests deeper
 * than two.
 *
 * The levels take from one bump pointer instead.  A switch records at
 * its own base plus its own ncase, so its values are contiguous no
 * matter what happens in between; a nested switch is handed the
 * enclosing switch's next free slot, so the two regions overlap.
 *
 * That overlap is the part worth understanding.  It is safe because a
 * switch's values are read at its own pop - swdispatch runs above,
 * before swtop comes down - and the enclosing switch does not record
 * another case until after that.  The enclosing switch's earlier
 * values sit below the inner base and are never written at all.  So
 * the inner switch is done with its slice before anything overwrites
 * it, and what overwrites it is the enclosing switch continuing its
 * own run.
 *
 * Giving the space back on pop is what keeps the base from climbing
 * with every switch in a function rather than with the nesting: it is
 * a space property, not a correctness one, which is why no test here
 * can catch its absence.  The peak is about the widest single switch
 * plus whatever arms its enclosing switches had already recorded, so
 * 384 holds a full 256-arm dispatch nested inside a switch with 128
 * arms ahead of it.  Running out is not silent - see the case below,
 * which emits a .error the assembler then refuses.
 */
#define SWPOOL 384
/*
 * The case values live OUTSIDE the context struct, on purpose: a
 * struct type's size is a byte in pass1, so "long val[256]" inside
 * the struct silently contributed nothing and swstk came out 48
 * bytes for what should have been 8K - every case value recorded
 * past the tenth stomped the statics that follow, which is how the
 * twelfth case of any switch stopped existing in the self-hosted
 * build.  A bare array is sized on the wide path and survives; the
 * struct keeps a pointer into it.
 */
static struct swctx {
	int id;			/* label number for this switch */
	int ncase;
	int hasdef;
	int wide;		/* a case value outside 0..255: dispatch on HL */
	int isbyte;		/* the control reduced to A */
	unsigned char *val;	/* this nesting level's slice of swvals */
} swstk[MAXSWNEST];
/*
 * Bytes, not longs: case values are bytes in this compiler - the
 * dispatch masks to eight bits anyway - and the long version of
 * this pool, once it actually existed, was 8K of bss that left the
 * self-hosted c1 under a thousand bytes of heap.
 *
 * A wide switch stores two bytes a case in the same pool, low then
 * high, and widens what it already had at the moment its first wide
 * case arrives - see swwiden.  Only the switch on top of the stack is
 * ever being added to, and a nested one gives its slice back when it
 * closes, so the top switch's values are always the last thing in the
 * pool and there is room above them to spread into.  A narrow switch
 * is untouched by any of this and still costs one byte a case.
 */
static unsigned char swvals[SWPOOL];
static int swtop;
static int swused;		/* bump pointer into swvals */

/*
 * Spread the values already stored for this switch from one byte a
 * case to two, low then high, when its first wide case arrives.
 *
 * Backwards, so nothing is overwritten before it has been moved, and
 * in place because there is always room: this switch is the one on
 * top of the stack, so its slice is the last thing in the pool.
 * Everything stored so far fitted a byte, which is why it was stored
 * as one, so every high byte is zero.
 */
/*
 * The value of case i, however this switch happens to store it.  One
 * spelling so the duplicate check is one loop rather than one per
 * width.
 */
static unsigned int
swcval(struct swctx *sw, int i)
{
	if (sw->wide)
		return (unsigned int)(sw->val[2 * i] |
		    (sw->val[2 * i + 1] << 8));
	return (unsigned int)sw->val[i];
}

static void
swwiden(struct swctx *sw)
{
	int i;

	if (swused + sw->ncase > SWPOOL) {
		outf("\t.error more than %d case values in switches open at once\n",
		    SWPOOL);
		return;
	}
	for (i = sw->ncase - 1; i >= 0; i--) {
		sw->val[2 * i] = sw->val[i];
		sw->val[2 * i + 1] = 0;
	}
	swused += sw->ncase;
	sw->wide = 1;
}

/*
 * _Kn_f_m: case m of switch n in function f.  _Dn_f: the dispatch,
 * _Nn_f: nothing matched, _Fn_f: the default, _Xn_f: past it all.
 *
 * The function index is not decoration.  labelcnt starts again at zero
 * in every function, so without it the first switch of one function
 * and the first of the next are both _D0.
 */
void
swlabel(char k, int id, int n)
{
	outf("_%c%d_%d", k, id, fnindex);
	if (n >= 0)
		outf("_%d", n);
}

/*
 * Emit the dispatch for a switch whose cases have all been seen.
 *
 * Three shapes, and which is smallest is a matter of counting rather
 * than taste.  With n cases spanning span values:
 *
 *	chain	5n		cp v / jp z,L per case
 *	swtab	4 + 3n		call, count byte, value + label per case
 *	swidx	5 + 2*span	call, lo, span, a label per slot
 *
 * So the chain wins to n=2, and above that it is swidx when the values
 * are dense enough to beat the pair table - 2*span < 3n-1, which is a
 * little over two thirds - and swtab when they are not.  Over the
 * tree's own 85 switches that is 4175 bytes of dispatch down to 2705.
 *
 * The two live at opposite ends and both are needed: every switch with
 * more than about twenty cases here is sparse (the largest, 125 cases
 * in nm.c, spans the whole byte at 48%) and would want a 517 byte
 * index against a 379 byte pair table, while the dense ones are nearly
 * all small - and for those swidx is not just smaller but constant
 * time instead of a scan.
 *
 * A count or span of 256 would store as a zero byte, so anything that
 * large stays on the chain.  MAXSWCASE bounds it at 256 and a switch
 * that big is theoretical; correctness is worth more than the bytes.
 */
/*
 * The dispatch for a switch with a case value outside 0..255.
 *
 * The control stays in HL and the comparison is sixteen bits.  One
 * shape, not the byte path's three: a pair table at 4 + 4n, which
 * beats a chain of sixteen bit compares - 13 bytes a case, since
 * there is no cp for a pair and sbc hl,de has to be undone - from the
 * very first case.  There is nothing for the chain to win, so there
 * is no chain.
 *
 * The dense index shape has no wide form either.  It would want the
 * value biased by lo before swidx sees it, 13 bytes to save
 * 4n - 2*span, and that only pays for a switch both wide and tightly
 * packed - which is not what wide switches look like.  The one that
 * prompted this dispatches S_IFDIR, S_IFBLK and S_IFCHR: three values
 * spanning 16384.
 *
 * Values go out as two .db and not one .dw because outf has only %d
 * and an int is sixteen bits on the target: the pattern for "case -1"
 * would print as 65535 from the host build of c1 and -1 from the
 * self-hosted one, and the two would stop producing identical
 * assembly.  A byte at a time is the same number to both.
 */
static void
swwide(struct swctx *sw)
{
	int i, n;

	n = sw->ncase;
	/* the count is one byte in the table, as it is for swtab */
	if (n > 255) {
		out("\t.error more than 255 cases in a wide switch\n");
		return;
	}
	outf("\tcall swtabw\n\t.db %d\n", n);
	for (i = 0; i < n; i++) {
		outf("\t.db %d\n\t.db %d\n",
		    (int)sw->val[2 * i], (int)sw->val[2 * i + 1]);
		out("\t.dw ");
		swlabel('K', sw->id, i);
		outc('\n');
	}
}

void
swdispatch(struct swctx *sw)
{
	int i, j, n, lo, hi, span;

	n = sw->ncase;
	if (n == 0)
		return;

	if (sw->wide) {
		swwide(sw);
		return;
	}

	lo = hi = (int)(sw->val[0] & 0xff);
	for (i = 1; i < n; i++) {
		j = (int)(sw->val[i] & 0xff);
		if (j < lo) lo = j;
		if (j > hi) hi = j;
	}
	span = hi - lo + 1;

	if (n >= 3 && span <= 255 && 2 * span < 3 * n - 1) {
		/* dense: bias and index, gaps pointing at no-match */
		outf("\tcall swidx\n\t.db %d\n\t.db %d\n", lo, span);
		for (i = 0; i < span; i++) {
			out("\t.dw ");
			for (j = 0; j < n; j++)
				if ((int)(sw->val[j] & 0xff) == lo + i)
					break;
			if (j < n)
				swlabel('K', sw->id, j);
			else
				swlabel('N', sw->id, -1);
			outc('\n');
		}
		return;
	}
	if (n >= 3 && n <= 255) {
		/*
		 * Sparse: values together so the scan is one cpir, labels
		 * after them and backwards, which is what lets the helper
		 * find the slot from what cpir leaves in HL and BC.
		 */
		outf("\tcall swtab\n\t.db %d\n", n);
		for (i = 0; i < n; i++)
			outf("\t.db %d\n", (int)(sw->val[i] & 0xff));
		for (i = n - 1; i >= 0; i--) {
			out("\t.dw ");
			swlabel('K', sw->id, i);
			outc('\n');
		}
		return;
	}
	for (i = 0; i < n; i++) {
		outf("\tcp %d\n\tjp z,", (int)(sw->val[i] & 0xff));
		swlabel('K', sw->id, i);
		outc('\n');
	}
}

/* Param staging: move from stack to register */
#define MAXSTAGE 8
struct stgent {
	unsigned char reg;	/* target register */
	unsigned char off;	/* stack offset from IY */
	unsigned char width;	/* b/B/s/S = size */
};
static struct stgent stage[MAXSTAGE];
static unsigned char nstage;

/* Register bitmasks for callee-save tracking */
#define REGBIT(r) (1 << (r))
#define USES_BC (REGBIT(R_B) | REGBIT(R_C) | REGBIT(R_BC))

/*
 * Does this function keep a VARIABLE in BC?  Every 32-bit runtime
 * helper takes its second operand off the stack with a pop bc, so a
 * call to one destroys whatever was there, and the $[ $] guards in
 * the rule table save it across those.  Only a variable needs that:
 * scratch does not care what a helper leaves behind.
 */
int
bcinuse(void)
{
	return (regsused & USES_BC) != 0;
}

/*
 * Must this function hand the caller's BC back?  When it keeps a
 * variable there, which the function header says and bcinuse() reads.
 *
 * This returned 1 unconditionally for a long while, and the reason
 * given was that the code generator uses BC as SCRATCH in functions
 * with no variable in it, which the header cannot say.  That was true
 * once and is not now: over the tree's own 864 functions there is not
 * one that writes BC without a variable living there.  What remained
 * true was the other half - a CALLED helper could destroy it - and
 * amul and adiv did, so every function in the tree paid a frame for
 * two arithmetic routines.
 *
 * Those two now save what they use, as every other helper already
 * did, so BC is preserved across any call a compiled function can
 * make and the header's answer is the whole answer.  See the note in
 * imul.s.  This is what makes fenter, fentn and fentx reachable at
 * all - they had been dead code, unreachable through this test.
 */
int
savesbc(void)
{
	return bcinuse();
}

void
emitprolog(void)
{
	/*
	 * Emit the function label: one colon keeps it in this file, two
	 * export it.
	 *
	 * This read the name for a leading S, which a static function
	 * defined at top level does not have and never did - its name
	 * is spelled exactly as a global's is - so every static came
	 * out exported, and static was no namespace at all.  pass1 says
	 * so in the type letter now.
	 */
	out(funcname);
	if (funcstatic)
		out(":\n");
	else
		out("::\n");

	bcoff = ixoff = 0;

	/*
	 * A function with no parameters, no locals and no register
	 * variables has nothing to point a frame at.  IY is only ever
	 * used to reach a parameter or a local - the Z80 has no (sp+d),
	 * which is why the frame pointer exists at all - so with neither,
	 * the whole prologue is nothing and the epilogue is one ret.
	 *
	 * Ten bytes to one, and two helper calls per invocation to none.
	 * The caller is unaffected: arguments still go on the stack and
	 * are still cleaned up by the caller, so this is not a second
	 * calling convention, just the same one with nothing to set up.
	 *
	 * What settles it is whether anything lives in the frame, not
	 * whether anything has to be saved.  A local with a register
	 * home lives in BC or IX and never in the frame at all, so a
	 * function can have two of them, owe both to its caller, and
	 * still have no use for IY - which is what the prefix parser's
	 * pfxStar, pfxAddr, pfxSizeof and pfxString all are, and
	 * resetSwitch and kreset and drop_assigns beside them.  This
	 * used to test regsused == 0 and send them all through a frame
	 * to hold nothing.
	 *
	 * peep cannot take that back afterwards.  Its rewrite has to fit
	 * the entry it replaces, and two pushes do not fit in the one
	 * call line that fentbx occupies; it says so where the table is
	 * declared.  The pass that knows there is nothing in the frame
	 * is this one, and it is cheaper to never write the frame than
	 * to write it and patch it out.
	 */
	noframe = (nparams == 0 && framesize == 0 && savebase == 0);
	if (noframe) {
		/* the saves are still owed - just no frame around them */
		if (savesbc())
			out("\tpush\tbc\n");
		if (regsused & REGBIT(R_IX))
			out("\tpush\tix\n");
		return;
	}

	{
		short off, rest;
		char *h, *sfx;

		/*
		 * The first argument arrives in HL - HL':HL when it is
		 * long - and the w/q helper variants spill it back into
		 * its old stack slot as part of frame setup, so the frame
		 * they build is laid out exactly as the stack convention's
		 * was.  A function with no parameters has nothing to
		 * place and keeps the plain helpers.
		 */
		sfx = nparams == 0 ? "" : (ISLONG(arg1w) ? "q" : "w");

		/*
		 * Frame pointer, then the scalar area, then the callee
		 * saves just under it - so they stay inside the 7-bit
		 * (iy+d) window - and the rest last, where big arrays live
		 * and are addressed with 16-bit arithmetic rather than
		 * (iy+d).
		 *
		 * All but the last of that is one call: eleven bytes of
		 * prologue become five, and the only thing particular to
		 * the function - how big the scalar area is - rides in the
		 * word after the call.
		 *
		 * A function with neither a save nor a scalar area wants
		 * plain fenter, and it is CALLED rather than written out.
		 * The bare sequence is eight bytes - push iy is two, ld
		 * iy,0 is four, add iy,sp is two - against three for the
		 * call, and leaving it to the peephole to substitute meant
		 * paying the eight in every build that does not run it.
		 */
		h = savesbc() ?
		      ((regsused & REGBIT(R_IX)) ? "fentbx" : "fentb") :
		      ((regsused & REGBIT(R_IX)) ? "fentx" : "fentn");

		/*
		 * The scalar-area word is written in a field, not tight.
		 *
		 * peep rewrites an entry it has decided needs no frame by
		 * overwriting the lines in place, so what it puts there has
		 * to fit inside what was written.  An entry saving both BC
		 * and IX needs two pushes and has two lines to put them in,
		 * but "\t.dw\t0\n" is seven bytes and "\tpush\tix\n" is
		 * nine, so the second push had nowhere to go and the most
		 * common entry in the compiler - 140 of them - could never
		 * lose its frame however plainly it did not need one.
		 *
		 * The field costs nothing.  Trailing blanks are whitespace
		 * to the assembler and normalise strips them before any
		 * rule sees the line, so this is two characters of room in
		 * the text and not one byte in the object.  They are
		 * written out rather than asked for with a width: outf is
		 * the tree's own and takes no flags, and "%-3d" put the
		 * number down followed by a literal d.
		 */
		if (!savesbc() && !(regsused & REGBIT(R_IX)) && savebase == 0)
			outf("\tcall\tfenter%s\n", sfx);
		else
			outf("\tcall\t%s%s\n\t.dw\t%d  \n", h, sfx, -savebase);

		off = -savebase;
		if (savesbc()) {
			off -= 2;
			bcoff = off;
			if (bcoff < -128)
				out("\t.error scalar frame too large for BC restore\n");
		}
		if (regsused & REGBIT(R_IX)) {
			off -= 2;
			ixoff = off;
			if (ixoff < -128)
				out("\t.error scalar frame too large for IX restore\n");
		}
		/* rest = arrays plus any unused save-slot bytes
		 * (off is -savebase-pushed, so this is
		 * framesize - savebase - pushed) */
		rest = framesize + off;
		if (rest > 0)
			outf("\tld\thl,-%d\n\tadd\thl,sp\n\tld\tsp,hl\n",
			    rest);
	}

	/* Stage params from stack to registers.  The walk runs from the
	 * top down to keep the emitted order what it always was; the
	 * static count itself stays intact (it is reset per function). */
	{
	register struct stgent *sp = stage + nstage;
	unsigned char ns = nstage + 1;

	while (--ns) {
		unsigned char r, off, w;

		sp--;
		r = sp->reg;
		off = sp->off;
		w = sp->width;

		if (ISBYTE(w)) {
			/* Byte: ld r,(iy+off) */
			out("\tld\t");
			switch (r) {
			case R_B: outc('b'); break;
			case R_C: outc('c'); break;
			}
			outf(",(iy+%d)\n", off);
		} else {
			/* Word: load low then high */
			switch (r) {
			case R_BC:
				outf("\tld\tc,(iy+%d)\n\tld\tb,(iy+%d)\n",
				    off, off + 1);
				break;
			case R_IX:
				outf("\tld\tl,(iy+%d)\n\tld\th,(iy+%d)\n\tpush\thl\n\tpop\tix\n",
				    off, off + 1);
				break;
			}
		}
	}
	}
}

void
emitepilog(void)
{
	/* Emit return label: Xfuncname (local, same length as func) */
	outc('X');
	out(funcname + 1);
	out(":\n");

	/*
	 * No frame was made, so there is nothing to unwind - only the
	 * saves to hand back, in the order that balances the entry.
	 */
	if (noframe) {
		if (regsused & REGBIT(R_IX))
			out("\tpop\tix\n");
		if (savesbc())
			out("\tpop\tbc\n");
		out("\tret\n");
		return;
	}

	/*
	 * Restore callee-saves without touching the return value.
	 *
	 * This used to be written out here: twelve bytes for IX, which
	 * has to come back through A because HL is the return value and
	 * DE is the rest of it when the value is long - a long-returning
	 * function that restored IX through DE handed back the saved
	 * IX's address as its low word, and "int a[5]" reserved .ds
	 * <heap pointer> bytes - and six more for BC.  Two thousand
	 * bytes of it in c1 alone.
	 *
	 * The helpers in csv.s do it once.  The saves sit together just
	 * under the scalar area, so all the caller has to say is where
	 * the lower of them is; the helper points the stack there and
	 * pops.  Five bytes against twenty-one, and the unwind is the
	 * same code, so there is no jp fexit after it.
	 */
	/*
	 * The w/q variants match the prologue's: the spilled first
	 * argument sits above the return address in a slot the caller
	 * does not know exists, so the exit helper is what discards it.
	 */
	{
	char *sfx = nparams == 0 ? "" : (ISLONG(arg1w) ? "q" : "w");

	if (savesbc() || (regsused & REGBIT(R_IX))) {
		char *h;
		short off;

		if (!savesbc()) {
			h = "fexx";
			off = ixoff;
		} else if (!(regsused & REGBIT(R_IX))) {
			h = "fexb";
			off = bcoff;
		} else {
			h = "fexbx";
			off = ixoff;	/* pushed last, so the lower */
		}
		outf("\tcall\t%s%s\n\t.dw\t%d\n", h, sfx, off);
		return;
	}

	/*
	 * Nothing to restore: just the unwind, and jumped to for the
	 * same reason the entry is called.  Written out it is five bytes
	 * - ld sp,iy and pop iy are two each - against three, and the
	 * peephole that used to make the substitution only runs under
	 * -O.
	 */
	outf("\tjp\tfexit%s\n", sfx);
	}
}

void
parseStmt(void)
{
	unsigned char op = read1();
	unsigned char n;
	Expr *e;

#ifdef DEBUG
	if (VERBOSE(V_STMT))
		fprintf(stderr, "stmt op=%s\n", stmtname(op));
	out("; stmt "); out(stmtname(op)); outc('\n');
#endif
	switch (op) {
	case AST_BLOCK:
		read1();
		n = read1();
#ifdef DEBUG
		if (VERBOSE(V_STMT))
			fprintf(stderr, "  BLOCK n=%d\n", n);
		out("; BLOCK n="); outd(n); outc('\n');
#endif
		++n;
		while (--n)
			parseStmt();
		return;
	case IF: {
		int lbl, hasel;
		n = read1();		/* nlabels for short-circuit */
		/*
		 * Two, not one.  An if with an else emits "no<lbl>" for the
		 * false branch and "no<lbl+1>" to jump over the else, but
		 * only one was reserved - and whether there is an else is
		 * not known until the then-body has been read, by which time
		 * any if inside it has already taken the number.
		 *
		 * So "no<lbl+1>" was defined twice and every jump to it went
		 * to whichever the assembler kept.  In an else-if chain the
		 * body of a branch was simply skipped: cpp built this way
		 * read its own "-o" and did nothing with the name after it.
		 *
		 * One number wasted per if without an else is nothing; they
		 * are per function and start again at zero.
		 */
		lbl = labelcnt;
		labelcnt += 2 + n;	/* lbl, lbl+1, and the short-circuits */
#ifdef DEBUG
		if (VERBOSE(V_STMT))
			fprintf(stderr, "  IF nlbl=%d lbl=%d\n", n, lbl);
		out("; IF nlbl="); outd(n); outc('\n');
#endif
		e = readexpr();
		if (e) {
#ifdef DEBUG
			dumpexpr(e);
#endif
			/*
			 * The condition branch-chains: && and || become jumps
			 * straight to the false label, with no nought-or-one
			 * materialised and retested in between.
			 */
			fmtstr(buf, "no%d_%d", lbl, fnindex);
			condfalse(e, buf);
		}
		parseStmt();		/* then-body */
		hasel = read1();
		if (hasel) {
			/* Jump over else */
			outf("\tjp no%d_%d\n", lbl + 1, fnindex);
		}
		/* Emit false label */
		outf("no%d_%d:\n", lbl, fnindex);
		if (hasel) {
			parseStmt();	/* else-body */
			/* Emit end label */
			outf("no%d_%d:\n", lbl + 1, fnindex);
		}
		return;
	}
	case RETURN:
		n = read1();
#ifdef DEBUG
		if (VERBOSE(V_STMT))
			fprintf(stderr, "  RETURN hasval=%d\n", n);
		out("; RETURN hasval="); outd(n); outc('\n');
#endif
		if (n) {
			e = readexpr();
			if (e) {
				Expr *hl, *assign;
				char w = e->width;

				/*
				 * Wrap in ASSIGN to HL for return value.
				 *
				 * A byte comes back in HL like everything
				 * else, so it has to be widened first, and
				 * a signed one has to carry its sign: the
				 * assignment used to be byte-wide, took the
				 * store rule that zeroes H, and a function
				 * returning -1 as a char handed back 0x00ff.
				 * Callers read the sign out of H and saw a
				 * positive number.
				 */
				if (ISBYTE(w)) {
					/*
					 * A constant is already whatever
					 * width it is asked to be, and there
					 * is no rule for converting one -
					 * "return 0" would have become a
					 * widening of a literal and emitted
					 * nothing.
					 */
					if (e->op == NUMBER)
						e->width = 's';
					else
						e = mkunary(ISSIGNED(w) ?
						    SEXT : WIDEN, 's', e);
					w = 's';
				}
				hl = mkcode(w, R_HL);
				hl->op = INHL;
				assign = mkbinary(ASSIGN, w, hl, e);
				setdest(assign, DEST_VALUE);
				assign = rewrite(assign);
#ifdef DEBUG
				dumpexpr(assign);
#endif
				freeexpr(assign);
			}
		}
		/*
		 * Jump to the function epilogue - unless there is no
		 * epilogue to speak of.  A frameless function has nothing
		 * to unwind, so its Xname label is a bare ret, and jumping
		 * three bytes to a one byte instruction is the long way
		 * round to it.
		 *
		 * Returning here rather than at the label is also what
		 * lets the conditional form exist at all: peep turns
		 * "jp cc,L / ret / L:" into "ret ncc", which is one byte
		 * where the jump around was three, and there is no such
		 * thing as a conditional jump to an epilogue.
		 *
		 * Frameless is not by itself enough to return here.  A
		 * frameless function that saved BC or IX has those on the
		 * stack and the epilogue is where they come back off; a
		 * ret from the middle of one would return through the
		 * saved register.
		 */
		if (noframe && !savesbc() && !(regsused & REGBIT(R_IX))) {
			out("\tret\n");
			return;
		}
		out("\tjp\tX");
		out(funcname + 1);
		outc('\n');
		return;
	case LABEL:
		readS(buf, sizeof(buf));
#ifdef DEBUG
		if (VERBOSE(V_STMT))
			fprintf(stderr, "  LABEL %s\n", buf);
#endif
		/*
		 * A label belongs to its function, not to the file.  Two
		 * functions may each have "done:" - ordinary C, and
		 * micronix's fio.c does it in iread and iwrite - and
		 * emitting both as @done put two labels of one name in
		 * one assembly.  The assembler took the second quietly
		 * and one function's "goto done" jumped into the other.
		 * The function index makes them distinct and keeps the
		 * name the source gave them.
		 */
		out(LBLPFX);
		outd(fnindex);
		out(buf);
		out(":\n");
		return;
	case GOTO:
		readS(buf, sizeof(buf));
#ifdef DEBUG
		if (VERBOSE(V_STMT))
			fprintf(stderr, "  GOTO %s\n", buf);
#endif
		out("\tjp ");
		out(LBLPFX);
		outd(fnindex);
		out(buf);
		outc('\n');
		return;
	case SWITCH: {
		struct swctx *sw;
		int isbyte;

		read1();
		n = read1();
#ifdef DEBUG
		if (VERBOSE(V_STMT))
			fprintf(stderr, "  SWITCH n=%d\n", n);
		out("; SWITCH n="); outd(n); outc('\n');
#endif
		if (swtop >= MAXSWNEST) {
			outf("\t.error switches nested deeper than %d\n",
			    MAXSWNEST);
			swtop = MAXSWNEST - 1;
		}
		sw = &swstk[swtop++];
		sw->id = labelcnt++;
		sw->ncase = 0;
		sw->hasdef = 0;
		sw->wide = 0;
		sw->isbyte = 0;
		sw->val = swvals + swused;

		/* work out the control value, then jump over the bodies to
		 * the comparisons - which is what keeps it live */
		isbyte = 0;
		e = readexpr();
		if (e) {
			setdest(e, DEST_VALUE);
			e = rewrite(e);
#ifdef DEBUG
			dumpexpr(e);
#endif
			isbyte = e && e->op == INA;
			/*
			 * The value rules leave a live register variable
			 * where it sits - being in a register IS being a
			 * value - but the dispatch below reads HL.  A
			 * word regvar is the one shape that comes back
			 * unmoved; anything else unreduced is a fault
			 * worth hearing about, not guessing around.
			 */
			if (e && e->op == INBC) {
				out("\tld l,c\n\tld h,b\n");
			} else if (e && !isbyte && e->op != INHL) {
				out("\t.error switch control unreduced\n");
			}
			freeexpr(e);
		}
		/*
		 * The narrowing to A used to be emitted here, before a
		 * single case had been read - which is the whole reason a
		 * wide case had nowhere to attach.  It is the same
		 * instructions in the same order either way, because the
		 * only thing between here and _D is a jump over the bodies
		 * and nothing executes in between: the value computed here
		 * is still in HL when the dispatch reads it.  Emitting it
		 * down there instead means the choice is made when the
		 * cases are known.
		 */
		sw->isbyte = isbyte;
		out("\tjp ");
		swlabel('D', sw->id, -1);
		outc('\n');

		++n;
		while (--n)
			parseStmt();

		/* the last body must not fall into the comparisons */
		out("\tjp ");
		swlabel('X', sw->id, -1);
		outc('\n');

		swlabel('D', sw->id, -1);
		out(":\n");
		/*
		 * Now the cases are known, so the control can be put in
		 * whatever the dispatch about to be chosen wants.
		 */
		if (sw->wide) {
			/*
			 * A byte control with a wide case.  The tree only
			 * leaves a value in A when a byte holds all of it -
			 * anything narrower than its use is widened before
			 * it gets here - so the high half is zero.
			 */
			if (sw->isbyte)
				out("\tld l,a\n\tld h,0\n");
		} else if (!sw->isbyte) {
			out("\tld a,h\n\tor a\n\tjp nz,");
			swlabel('N', sw->id, -1);
			out("\n\tld a,l\n");
		}
		swdispatch(sw);
		/* no case matched, and a word control that did not fit a
		 * byte arrives here too.  Both helpers fall out of their
		 * table onto this label rather than storing its address */
		swlabel('N', sw->id, -1);
		out(":\n");
		if (sw->hasdef) {
			out("\tjp ");
			swlabel('F', sw->id, -1);
			outc('\n');
		}
		swlabel('X', sw->id, -1);
		out(":\n");
		/* give this switch's values back to the pool; an
		 * enclosing switch resumes where they started */
		swused = (int)(sw->val - swvals);
		swtop--;
		return;
	}
	case CASE: {
		struct swctx *sw = swtop ? &swstk[swtop - 1] : 0;

		n = read1();
#ifdef DEBUG
		if (VERBOSE(V_STMT))
			fprintf(stderr, "  CASE n=%d\n", n);
		out("; CASE n="); outd(n); outc('\n');
#endif
		/* the value is a constant - pass1 folded it - so take it
		 * rather than emitting code for it */
		e = readexpr();
		if (sw && e) {
			/*
			 * Two, not one: a wide case takes a pair, and the
			 * switch may turn wide on this very value.
			 */
			if (swused + 1 >= SWPOOL) {
				outf("\t.error more than %d case values in switches open at once\n",
				    SWPOOL);
			} else {
				/*
				 * Two cases the dispatch cannot tell apart.
				 * pass1 used to carry a long per case for this
				 * and never looked at it; the values are
				 * already here, in the array the jump table is
				 * built from, so the check costs a walk and no
				 * memory at all.
				 *
				 * A byte, because that is what the dispatch
				 * compares - see the masks in swdispatch.  So
				 * this also catches the pair that differ only
				 * above the low byte, which would silently
				 * share an arm rather than being diagnosed.
				 */
				unsigned int v =
				    (unsigned int)(e->u.val & 0xffffL);
				int i;

				/*
				 * A case value that does not fit a byte cannot
				 * be dispatched.  That is by design - the
				 * three shapes above all compare eight bits -
				 * and the design says such a value "cannot
				 * match and goes to the default".
				 *
				 * Going to the default SILENTLY is the part
				 * that is not worth keeping.  "case 256:" is
				 * ordinary C, it compiled without a word, and
				 * it took the default arm.  Where two such
				 * values shared a low byte the duplicate check
				 * below fired instead, which is how this was
				 * found - Morrow's formatmw.c switches a
				 * sector size over 128, 256, 512, 1024, 2048
				 * and four of those mask to zero.
				 *
				 * So say so.  The limit stays; a program that
				 * meets it now stops rather than running the
				 * wrong arm, and the fix in the source is to
				 * switch on something that fits - formatmw
				 * divides by 128 and its labels become
				 * 1,2,4,8,16.
				 *
				 * outf does %s, %c and int - there is no %ld.
				 */
				if (e->u.val < -32768L || e->u.val > 65535L) {
					/*
					 * Counted as well as written out: the
					 * count fails the compile here, with
					 * the source still named, and the
					 * .error catches anyone assembling a
					 * .s kept from a -s run, which never
					 * reaches the assembler otherwise.
					 *
					 * Sixteen bits is the limit now rather
					 * than eight.  The value is not
					 * printed: outf has only %d and an int
					 * is sixteen bits on the target, so a
					 * long case value has no spelling both
					 * builds of c1 agree on.
					 */
					nbadcase++;
					out("\t.error case value does not fit sixteen bits\n");
				}
				/*
				 * The first value outside a byte turns the
				 * whole switch wide, including the cases
				 * already seen.
				 */
				if (!sw->wide && v > 255)
					swwiden(sw);
				for (i = 0; i < sw->ncase; i++) {
					if (swcval(sw, i) == v) {
						out("\t.error duplicate case value\n");
						break;
					}
				}
				if (sw->wide) {
					sw->val[2 * sw->ncase] = v;
					sw->val[2 * sw->ncase + 1] = v >> 8;
					swused += 2;
				} else {
					sw->val[sw->ncase] = v;
					swused++;
				}
				swlabel('K', sw->id, sw->ncase);
				out(":\n");
				sw->ncase++;
			}
		}
		if (e)
			freeexpr(e);
		++n;
		while (--n)
			parseStmt();
		return;
	}
	case DEFAULT: {
		struct swctx *sw = swtop ? &swstk[swtop - 1] : 0;

		n = read1();
#ifdef DEBUG
		if (VERBOSE(V_STMT))
			fprintf(stderr, "  DEFAULT n=%d\n", n);
		out("; DEFAULT n="); outd(n); outc('\n');
#endif
		if (sw) {
			sw->hasdef = 1;
			swlabel('F', sw->id, -1);
			out(":\n");
		}
		++n;
		while (--n)
			parseStmt();
		return;
	}
	case ASM: {
		/* Inline asm - copy the text through verbatim */
		unsigned short len = read2();
#ifdef DEBUG
		if (VERBOSE(V_STMT))
			fprintf(stderr, "  ASM len=%d\n", len);
#endif
		outc('\n');
		outc('\t');
		while (len--)
			outc(read1());
		outc('\n');
		return;
	}
	case SEMI:
		/* Empty statement (bare semicolon) - no-op */
#ifdef DEBUG
		if (VERBOSE(V_STMT))
			fprintf(stderr, "  SEMI (empty)\n");
		out("; SEMI\n");
#endif
		return;
	default:
		/* Expression statement - op byte is start of expression */
		unread1(op);
#ifdef DEBUG
		if (VERBOSE(V_STMT))
			fprintf(stderr, "  EXPR\n");
		out("; EXPR\n");
#endif
		e = readexpr();
		if (e) {
			setdest(e, DEST_NONE);
			e = rewrite(e);
#ifdef DEBUG
			dumpexpr(e);
#endif
			freeexpr(e);
		}
		return;
	}
}

void
parse(void)
{
	unsigned char op, t, n, i;

#ifdef DEBUG
	if (VERBOSE(V_PARSE))
		fprintf(stderr, "parse: starting\n");
#endif
	while ((op = read1()) != E_O_F) {
#ifdef DEBUG
		if (VERBOSE(V_PARSE))
			fprintf(stderr, "parse: top op=%s\n", stmtname(op));
		out("; top "); out(stmtname(op)); outc('\n');
#endif
		switch (op) {
		case AST_FUNC:
			t = read1();
			funcstatic = (t & 0x80) != 0;	/* pass1 marks it there */
			t &= 0x7f;
			readS(funcname, sizeof(funcname));
			labelcnt = 0;
			fnindex++;
#ifdef DEBUG
			if (VERBOSE(V_PARSE))
				fprintf(stderr, "parse: FUNC %s type=%c\n", funcname, t);
			out("; FUNC "); out(funcname); outc(':'); outc(t); outc('\n');
#endif
			n = read1();		/* param count */
			nparams = n;
			i = read1();		/* local count */
			framesize = read2();	/* frame size */
			savebase = read1();	/* scalar area size */
			/*
			 * Phase 1 walked the body before this header was
			 * written, so this is a settled fact about code not
			 * yet seen: nothing in it reaches through IY except
			 * possibly the one parameter now arriving in HL.
			 */
			framefree = read1();
			regsused = 0;
			nstage = 0;
			arg1w = 0;
#ifdef DEBUG
			if (VERBOSE(V_PARSE))
				fprintf(stderr, "parse: params=%d locals=%d frame=%d\n",
					n, i, framesize);
			out("; params="); outd(n); out(" locals="); outd(i);
			out(" frame="); outd(framesize);
			out(" framefree="); outd(framefree); outc('\n');
#endif
			/* Scan params: may need staging to registers */
			++n;
			while (--n) {
				unsigned char reg, off;
				read1();	/* AST_DECL */
				t = read1();
				readS(buf, sizeof(buf));
				reg = read1();
				off = read1();
#ifdef DEBUG
				if (VERBOSE(V_PARSE))
					fprintf(stderr, "parse: param %s t=%c r=%d o=%d\n",
						buf, t, reg, off);
				out("; param "); out(buf); outc(':'); outc(t);
				out(" r="); outd(reg); out(" o="); outd(off); outc('\n');
#endif
				/* the first parameter arrives in HL; its
				 * width picks the spilling helper variant */
				if (n == nparams)
					arg1w = t;
				if (reg) {
					struct stgent *tp = &stage[nstage++];
					regsused |= REGBIT(reg);
					tp->reg = reg;
					tp->off = off;
					tp->width = t;
				}
			}
			/* Scan locals: just track register usage */
			++i;
			while (--i) {
				unsigned char reg;
				read1();	/* AST_DECL */
				t = read1();
				readS(buf, sizeof(buf));
				reg = read1();
				read2();	/* offset */
#ifdef DEBUG
				if (VERBOSE(V_PARSE))
					fprintf(stderr, "parse: local %s t=%c r=%d\n",
						buf, t, reg);
				out("; local "); out(buf); outc(':'); outc(t);
				out(" r="); outd(reg); outc('\n');
#endif
				if (reg)
					regsused |= REGBIT(reg);
			}
			emitprolog();
			parseStmt();
			emitepilog();
			break;
		}
	}
#ifdef DEBUG
	if (VERBOSE(V_PARSE))
		fprintf(stderr, "parse: EOF\n");
	out("; EOF\n");
#endif
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
