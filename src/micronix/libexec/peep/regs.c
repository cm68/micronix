/*
 * regs.c - what an instruction reads and what it writes
 *
 * Every rule that removes an instruction has to show that nothing
 * wanted what it produced.  That is the whole of the danger in a
 * peephole: the pattern is easy to recognise and the proof that it is
 * safe to rewrite is where the bugs live.  So this is deliberately
 * pessimistic - an instruction that is not recognised is treated as
 * reading everything and writing nothing, which makes isdead() say no
 * and the rule decline to fire.
 *
 * The one piece of real knowledge encoded here is the leading
 * underscore.  A C function compiled by this tree is _name and takes
 * its arguments on the stack, so a call to one reads no registers and
 * destroys the caller-saved set.  The hand written helpers in libsrc -
 * lstde, swtab, swidx - have no underscore and do take arguments in
 * registers, so a call to one of those is assumed to read all of them.
 * Getting that backwards would silently delete the instruction that
 * set up a helper's argument.
 */
#include <string.h>
#include "peep.h"

/*
 * Register name to mask.  Longest first: ixh has to be tested before
 * ix, or "ixh" reads as ix followed by a stray h.
 */
static char *regname[] = {
	"ixh", "ixl", "iyh", "iyl",
	"af", "bc", "de", "hl", "ix", "iy", "sp",
	"a", "b", "c", "d", "e", "h", "l",
	0
};

static int regmask[] = {
	R_IX, R_IX, R_IY, R_IY,
	R_A | R_F, R_BC, R_DE, R_HL, R_IX, R_IY, 0,
	R_A, R_B, R_C, R_D, R_E, R_H, R_L,
	0
};

int
isalnum_(char c)
{
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
		(c >= '0' && c <= '9') || c == '_';
}

/*
 * The register named at p, or 0.  Only matches when the name is not
 * part of a longer word, so the label _hlthing is not read as hl.
 */
int
regat(char *p)
{
	int i, n;

	for (i = 0; regname[i]; i++) {
		n = strlen(regname[i]);
		if (strncmp(p, regname[i], n) == 0 && !isalnum_(p[n]))
			return regmask[i];
	}
	return 0;
}

/*
 * Every register mentioned anywhere in an operand, including the ones
 * inside a memory reference: (hl) reads hl, (ix+4) reads ix.  Used for
 * the read side, where naming a register at all is enough to count.
 */
int
allregs(char *p)
{
	int m = 0, r;

	while (*p) {
		if (isalnum_(*p)) {
			r = regat(p);
			if (r)
				m |= r;
			while (isalnum_(*p))
				p++;
			continue;
		}
		p++;
	}
	return m;
}

/*
 * The destination of an ld, which is written rather than read - unless
 * it is a memory reference, in which case the registers naming the
 * address are read and nothing is written.
 */
int
dstregs(char *p, int *rd)
{
	*rd = 0;
	if (*p == '(') {
		*rd = allregs(p);
		return 0;
	}
	return regat(p);
}

/* copy the mnemonic of insn into buf */
void
mnemof(char *insn, char *buf, int n)
{
	int i = 0;

	while (insn[i] && insn[i] != ' ' && i < n - 1) {
		buf[i] = insn[i];
		i++;
	}
	buf[i] = '\0';
}

/* the operand text, or "" */
char *
operof(char *insn)
{
	while (*insn && *insn != ' ')
		insn++;
	while (*insn == ' ')
		insn++;
	return insn;
}

/* the second operand of a two operand instruction, or "" */
char *
oper2(char *op)
{
	int depth = 0;

	while (*op) {
		if (*op == '(')
			depth++;
		else if (*op == ')')
			depth--;
		else if (*op == ',' && depth == 0)
			return op + 1;
		op++;
	}
	return op;
}

/* is this a call to a C function - underscore, arguments on the stack */
int
iscfn(char *op)
{
	return *op == '_';
}

/*
 * Is this memory reference one that any register can be loaded from?
 * (hl), (ix+d) and (iy+d) are; (nn) is not - the absolute form exists
 * only for a, and rewriting "ld a,(nn) / ld l,a" into "ld l,(nn)"
 * would be an instruction the assembler cannot encode.
 */
int
memok(char *p)
{
	int r;

	if (*p != '(')
		return 0;
	r = regat(p + 1);
	return r == R_HL || r == R_IX || r == R_IY;
}

/*
 * Instructions that end a straight line run.  Analysis stops at one:
 * what happens after a branch depends on where it goes.
 */
int
isbranch(char *insn)
{
	char m[8];

	mnemof(insn, m, sizeof(m));
	return strcmp(m, "jp") == 0 || strcmp(m, "jr") == 0 ||
		strcmp(m, "ret") == 0 || strcmp(m, "djnz") == 0 ||
		strcmp(m, "reti") == 0 || strcmp(m, "retn") == 0 ||
		strcmp(m, "rst") == 0;
}

/*
 * What insn reads.  When in doubt, everything: an unrecognised
 * instruction must not let a rule conclude that a register is dead.
 */
int
reads(char *insn)
{
	char m[8];
	char *op, *o2;

	mnemof(insn, m, sizeof(m));
	op = operof(insn);
	o2 = oper2(op);

	if (strcmp(m, "ld") == 0) {
		int rd;

		dstregs(op, &rd);
		return rd | allregs(o2);
	}
	if (strcmp(m, "push") == 0)
		return allregs(op);
	if (strcmp(m, "pop") == 0)
		return 0;
	if (strcmp(m, "call") == 0)
		return iscfn(op) ? 0 : R_CALL;
	if (strcmp(m, "ret") == 0)
		return R_HL | R_F;		/* the return value, and the condition */
	if (strcmp(m, "ex") == 0)
		return allregs(op) | allregs(o2);
	/*
	 * exx reads all three pairs, because their values are not lost -
	 * they go to the primes and the matching exx brings them back.
	 * Saying so is what stops a rule deleting the instruction that
	 * set up the half of a long living in HL' across the swap.
	 *
	 * It reads nothing else, and that is the point of naming it at
	 * all: unrecognised, it read everything, and A, the flags and
	 * both index registers became invisible past the first exx of a
	 * 32-bit sequence.  None of the four is banked.
	 */
	if (strcmp(m, "exx") == 0)
		return R_BC | R_DE | R_HL;
	if (strcmp(m, "add") == 0 || strcmp(m, "adc") == 0 ||
		strcmp(m, "sbc") == 0) {
		int r = allregs(op) | allregs(o2);
		if (strcmp(m, "adc") == 0 || strcmp(m, "sbc") == 0)
			r |= R_F;
		return r;
	}
	if (strcmp(m, "sub") == 0 || strcmp(m, "and") == 0 ||
		strcmp(m, "or") == 0 || strcmp(m, "xor") == 0 ||
		strcmp(m, "cp") == 0)
		return R_A | allregs(op);
	if (strcmp(m, "inc") == 0 || strcmp(m, "dec") == 0)
		return allregs(op);
	if (strcmp(m, "jp") == 0 || strcmp(m, "jr") == 0)
		return allregs(op) | R_F;
	if (strcmp(m, "neg") == 0 || strcmp(m, "cpl") == 0 ||
		strcmp(m, "daa") == 0 || strcmp(m, "rla") == 0 ||
		strcmp(m, "rra") == 0 || strcmp(m, "rlca") == 0 ||
		strcmp(m, "rrca") == 0)
		return R_A | R_F;
	if (strcmp(m, "nop") == 0)
		return 0;

	return ~0;					/* unknown: assume it wants everything */
}

/*
 * What insn writes.  Unknown instructions write nothing, which is the
 * pessimistic answer here: a register only counts as dead once
 * something is known to have overwritten it.
 */
int
writes(char *insn)
{
	char m[8];
	char *op, *o2;
	int rd;

	mnemof(insn, m, sizeof(m));
	op = operof(insn);
	o2 = oper2(op);

	if (strcmp(m, "ld") == 0)
		return dstregs(op, &rd);
	if (strcmp(m, "pop") == 0)
		return allregs(op);
	if (strcmp(m, "push") == 0)
		return 0;
	if (strcmp(m, "call") == 0) {
		/*
		 * fenter is ours and touches only hl and iy.  Claiming the
		 * whole caller-saved set for it would be the dangerous
		 * direction of wrong: a later rule would read that as "a is
		 * overwritten here" and delete the instruction that set it.
		 */
		if (strcmp(op, "fenter") == 0)
			return R_HL | R_IY;
		return R_CALL;
	}
	if (strcmp(m, "ex") == 0)
		return allregs(op) | allregs(o2);
	/* and it writes all three: what comes back is not what went in */
	if (strcmp(m, "exx") == 0)
		return R_BC | R_DE | R_HL;
	if (strcmp(m, "add") == 0 || strcmp(m, "adc") == 0 ||
		strcmp(m, "sbc") == 0) {
		int w = 0;
		if (*op == '(')
			return R_F;
		w = regat(op);
		return w | R_F;
	}
	if (strcmp(m, "sub") == 0 || strcmp(m, "and") == 0 ||
		strcmp(m, "or") == 0 || strcmp(m, "xor") == 0)
		return R_A | R_F;
	if (strcmp(m, "cp") == 0)
		return R_F;
	if (strcmp(m, "inc") == 0 || strcmp(m, "dec") == 0) {
		int w;
		if (*op == '(')
			return R_F;
		w = regat(op);
		/* the 16 bit forms leave the flags alone */
		if (w == R_BC || w == R_DE || w == R_HL || w == R_IX ||
			w == R_IY || w == 0)
			return w;
		return w | R_F;
	}
	if (strcmp(m, "neg") == 0 || strcmp(m, "cpl") == 0 ||
		strcmp(m, "daa") == 0 || strcmp(m, "rla") == 0 ||
		strcmp(m, "rra") == 0 || strcmp(m, "rlca") == 0 ||
		strcmp(m, "rrca") == 0)
		return R_A | R_F;

	return 0;					/* unknown: nothing is proved overwritten */
}

/*
 * Is every bit of reg overwritten before any of it is read?
 *
 * Partial writes are why this accumulates rather than testing one
 * instruction at a time: after "ld l,a" the low half of hl is dead and
 * the high half is not, and a later "ld h,0" is what finishes the job.
 * Reading a part that has not yet been overwritten is what proves the
 * value is still wanted.
 */
int
isdead(int reg, int from)
{
	int i, got = 0;

	for (i = from; i < nwin; i++) {
		if (win[i].kind == L_LABEL)
			return 0;			/* a jump target: no idea what reaches it */
		if (win[i].kind != L_INSN)
			continue;
		/*
		 * Spelled without the complement: pass2 has no rule yet
		 * for ~ inside a condition, and what it does with a shape
		 * it cannot reduce is leave a marker and a wrong branch.
		 * "some bit of reg is read that got has not yet covered"
		 * says the same thing with an OR: adding the read bits to
		 * got changes it exactly when such a bit exists.
		 */
		if (((reads(win[i].key) & reg) | got) != got)
			return 0;
		got |= writes(win[i].key);
		if ((got & reg) == reg)
			return 1;
		if (isbranch(win[i].key))
			return 0;
	}
	return 0;					/* fell off the window: assume wanted */
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
