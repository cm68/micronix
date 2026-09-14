/*
 * value.c - value numbering over a straight-line run
 *
 * The state every rule now reads from.  Each of the seven 8-bit
 * registers holds a value number, and a side table maps some numbers
 * to the constant they are known to hold; the carry flag is tracked
 * separately as unknown/clear/set.  This is SSA in miniature: a
 * definition mints a fresh number, a copy (ld b,a) gives the
 * destination the source's *current* number, and a later write to the
 * source mints a new number that does not touch the copy.  Two
 * registers holding one number hold one value.
 *
 * A jump or a label ends the run and resets everything to unknown,
 * because control can arrive from elsewhere.  A call does not end the
 * run - control returns - but it clobbers the caller-saved set and the
 * flags.
 *
 * The transition is a pure fold over the emitted lines, so peep.c
 * carries one state and advances it per emitted instruction; a rule
 * that rewrites the window leaves the state alone, because the window
 * is ahead of the state and the state only ever describes what has
 * already gone out.
 */
#include <string.h>
#include "peep.h"

#define C_UNK 0
#define C_CLR 1
#define C_SET 2

#define VN_UNKNOWN 0
#define VN_MAX 256
/* the "not a known constant" marker; -1 cannot serve because -1 (0xff) is a
 * real 8-bit constant, and a value known to be -1 would read as unknown */
#define VNOTCONST 256

/* value number -> the constant it is known to hold, or -1 */
static int vconst[VN_MAX];
static int vn_next = 1;

/* register index for an 8-bit name, or -1 */
static int
v8idx(char *name)
{
	switch (regat(name)) {
	case R_A: return VA;
	case R_B: return VB;
	case R_C: return VC;
	case R_D: return VD;
	case R_E: return VE;
	case R_H: return VH;
	case R_L: return VL;
	}
	return -1;
}

/* the 8-bit halves of a 16-bit pair name; -1 if not a tracked pair */
static void
v16idx(char *name, int *hi, int *lo)
{
	if (strncmp(name, "hl", 2) == 0) { *hi = VH; *lo = VL; }
	else if (strncmp(name, "de", 2) == 0) { *hi = VD; *lo = VE; }
	else if (strncmp(name, "bc", 2) == 0) { *hi = VB; *lo = VC; }
	else { *hi = *lo = -1; }	/* sp, ix, iy: not tracked */
}

/* the constant this value number is known to be, or -1 */
static int
vnconst(int vn)
{
	return vn > 0 ? vconst[vn] : VNOTCONST;
}

/* give register idx a fresh value number, known to be constant c */
static void
setconst(struct vstate *s, int idx, int c)
{
	int vn = vn_next;

	if (vn >= VN_MAX) {
		/* too many values in this one run: give up, not alias */
		s->r[idx] = VN_UNKNOWN;
		return;
	}
	vn_next++;
	vconst[vn] = c;
	s->r[idx] = vn;
}

/* a fresh value number, not a known constant */
static void
setfresh(struct vstate *s, int idx)
{
	setconst(s, idx, VNOTCONST);
}

/* parse a constant: decimal, 0x hex, or trailing-h hex; 1 on success */
int
vnumber(char *p, int *out)
{
	int n = 0, neg = 0, hex = 0, c, got = 0;
	char *q;

	while (*p == ' ' || *p == '\t')
		p++;
	if (*p == '-') { neg = 1; p++; }
	else if (*p == '+') p++;

	if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) {
		hex = 1;
		p += 2;
	}
	/* a trailing h marks the digits hex, the way pass2 spells 80h */
	for (q = p; *q; q++)
		if (*q == 'h' || *q == 'H') { hex = 1; break; }

	for (; *p && *p != 'h' && *p != 'H'; p++) {
		c = *p;
		if (c >= '0' && c <= '9')
			n = n * (hex ? 16 : 10) + (c - '0');
		else if (c >= 'a' && c <= 'f')
			{ if (!hex) return 0; n = n * 16 + (c - 'a' + 10); }
		else if (c >= 'A' && c <= 'F')
			{ if (!hex) return 0; n = n * 16 + (c - 'A' + 10); }
		else
			return 0;
		got = 1;
	}
	if (!got)
		return 0;

	*out = neg ? -n : n;
	return 1;
}

void
vinit(struct vstate *s)
{
	int i;

	for (i = 0; i < 7; i++)
		s->r[i] = VN_UNKNOWN;
	s->c = C_UNK;
}

/* a jump or label: nothing is known, and a fresh numbering begins */
void
vreset(struct vstate *s)
{
	vinit(s);
	vn_next = 1;
}

int
viszero(struct vstate *s, int idx)
{
	return vnconst(s->r[idx]) == 0;
}

int
visclear(struct vstate *s)
{
	return s->c == C_CLR;
}

/* does the 16-bit pair (hi, lo) hold the constant n? */
int
vpairconst(struct vstate *s, int hi, int lo, int n)
{
	return vnconst(s->r[hi]) == ((n >> 8) & 0xff) &&
	       vnconst(s->r[lo]) == (n & 0xff);
}

/*
 * Is insn a constant load the state already satisfies?  Returns the
 * bytes the load occupies - 2 for an 8-bit load, 3 for a 16-bit one -
 * so the rule knows what it saved, or 0 if the load is not redundant.
 * This is what turns func(0,0,0) into one ld hl,0 and three pushes.
 */
int
vredundant(struct vstate *s, char *insn)
{
	char m[8];
	char *op, *o2;
	int d, hi, lo, n;

	mnemof(insn, m, sizeof(m));
	if (strcmp(m, "ld") != 0)
		return 0;
	op = operof(insn);
	o2 = oper2(op);

	d = v8idx(op);
	if (d >= 0) {
		if (!vnumber(o2, &n))
			return 0;
		return vnconst(s->r[d]) == n ? 2 : 0;
	}
	if (strncmp(op, "hl,", 3) == 0) { hi = VH; lo = VL; }
	else if (strncmp(op, "de,", 3) == 0) { hi = VD; lo = VE; }
	else if (strncmp(op, "bc,", 3) == 0) { hi = VB; lo = VC; }
	else
		return 0;			/* a store, or an untracked pair */
	if (!vnumber(o2, &n))
		return 0;
	if (vnconst(s->r[hi]) == ((n >> 8) & 0xff) &&
	    vnconst(s->r[lo]) == (n & 0xff))
		return 3;
	return 0;
}

/*
 * The transition.  Reads mnemonic and operand and advances s by one
 * instruction.  Everything not named here is treated as writing every
 * register and the carry - the safe direction for a value tracker,
 * where a stale value costs bytes that were never there.
 */
void
vnext(struct vstate *s, char *insn)
{
	char m[8];
	char *op, *o2;
	int d, src, hi, lo, n;

	mnemof(insn, m, sizeof(m));
	op = operof(insn);
	o2 = oper2(op);

	if (strcmp(m, "ld") == 0) {
		d = v8idx(op);
		if (d >= 0) {
			src = v8idx(o2);
			if (src >= 0)
				s->r[d] = s->r[src];	/* copy */
			else if (vnumber(o2, &n))
				setconst(s, d, n);	/* constant */
			else
				setfresh(s, d);		/* (mem) */
			return;
		}
		if (*op == '(')
			return;				/* ld (mem),r: store */
		if (strncmp(op, "hl,", 3) == 0) { hi = VH; lo = VL; }
		else if (strncmp(op, "de,", 3) == 0) { hi = VD; lo = VE; }
		else if (strncmp(op, "bc,", 3) == 0) { hi = VB; lo = VC; }
		else
			return;				/* sp, ix, iy: not tracked */
		if (vnumber(o2, &n)) {
			setconst(s, hi, (n >> 8) & 0xff);
			setconst(s, lo, n & 0xff);
		} else {
			int shi, slo;
			v16idx(o2, &shi, &slo);
			if (shi >= 0) {
				s->r[hi] = s->r[shi];	/* copy pair */
				s->r[lo] = s->r[slo];
			} else {
				setfresh(s, hi);	/* (mem) */
				setfresh(s, lo);
			}
		}
		return;
	}

	if (strcmp(m, "push") == 0)
		return;

	if (strcmp(m, "pop") == 0) {
		if (strcmp(op, "af") == 0) {
			setfresh(s, VA);
			s->c = C_UNK;
		} else {
			v16idx(op, &hi, &lo);
			if (hi >= 0) {
				setfresh(s, hi);
				setfresh(s, lo);
			}
		}
		return;
	}

	if (strcmp(m, "add") == 0) {
		if (strncmp(op, "hl,", 3) == 0 || strncmp(op, "ix,", 3) == 0 ||
		    strncmp(op, "iy,", 3) == 0) {
			setfresh(s, VH);		/* 16-bit add: C changes */
			setfresh(s, VL);
			s->c = C_UNK;
		} else {
			setfresh(s, VA);
			s->c = C_UNK;
		}
		return;
	}

	if (strcmp(m, "adc") == 0 || strcmp(m, "sbc") == 0) {
		if (strcmp(m, "sbc") == 0 && strcmp(op, "a,a") == 0) {
			/* sbc a,a: A = -C, and the carry is preserved */
			if (s->c == C_CLR)
				setconst(s, VA, 0);
			else if (s->c == C_SET)
				setconst(s, VA, 0xff);
			else
				setfresh(s, VA);
			return;
		}
		if (strncmp(op, "hl,", 3) == 0) {
			setfresh(s, VH);
			setfresh(s, VL);
			s->c = C_UNK;
		} else {
			setfresh(s, VA);
			s->c = C_UNK;
		}
		return;
	}

	if (strcmp(m, "sub") == 0 || strcmp(m, "cp") == 0) {
		if (strcmp(m, "sub") == 0)
			setfresh(s, VA);
		s->c = C_UNK;
		return;
	}

	if (strcmp(m, "and") == 0 || strcmp(m, "or") == 0 || strcmp(m, "xor") == 0) {
		if (strcmp(m, "xor") == 0 && strcmp(op, "a") == 0)
			setconst(s, VA, 0);	/* xor a: A = 0 */
		else if (strcmp(op, "a") == 0)
			;			/* or a / and a: A unchanged */
		else
			setfresh(s, VA);	/* or/and/xor r|n: A = A op x */
		s->c = C_CLR;			/* all three clear carry */
		return;
	}

	if (strcmp(m, "inc") == 0 || strcmp(m, "dec") == 0) {
		d = v8idx(op);
		if (d >= 0) {
			setfresh(s, d);		/* 8-bit: flags change, C does not */
		} else {
			v16idx(op, &hi, &lo);
			if (hi >= 0) {
				setfresh(s, hi);	/* 16-bit: no flags at all */
				setfresh(s, lo);
			}
		}
		return;
	}

	if (strcmp(m, "neg") == 0) {
		setfresh(s, VA);
		s->c = C_UNK;			/* C = (A != 0) */
		return;
	}

	if (strcmp(m, "cpl") == 0) {
		setfresh(s, VA);		/* C untouched */
		return;
	}

	if (strcmp(m, "daa") == 0) {
		setfresh(s, VA);
		s->c = C_UNK;
		return;
	}

	if (strcmp(m, "scf") == 0) {
		s->c = C_SET;
		return;
	}

	if (strcmp(m, "ccf") == 0) {
		s->c = C_UNK;			/* C = ~C */
		return;
	}

	if (strcmp(m, "rla") == 0 || strcmp(m, "rra") == 0 ||
	    strcmp(m, "rlca") == 0 || strcmp(m, "rrca") == 0) {
		setfresh(s, VA);
		s->c = C_UNK;
		return;
	}

	if (strcmp(m, "rl") == 0 || strcmp(m, "rr") == 0 ||
	    strcmp(m, "rlc") == 0 || strcmp(m, "rrc") == 0 ||
	    strcmp(m, "sla") == 0 || strcmp(m, "sra") == 0 ||
	    strcmp(m, "sll") == 0 || strcmp(m, "srl") == 0) {
		d = v8idx(op);
		if (d >= 0)
			setfresh(s, d);
		s->c = C_UNK;
		return;
	}

	if (strcmp(m, "bit") == 0) {
		/* Z changes, C does not */
		return;
	}

	if (strcmp(m, "res") == 0 || strcmp(m, "set") == 0) {
		d = v8idx(oper2(op));
		if (d >= 0)
			setfresh(s, d);
		return;
	}

	if (strcmp(m, "ex") == 0) {
		if (strncmp(op, "af", 2) == 0) {
			setfresh(s, VA);
			s->c = C_UNK;
		} else if (strncmp(op, "de,hl", 5) == 0 ||
			   strncmp(op, "hl,de", 5) == 0) {
			int t;
			t = s->r[VD]; s->r[VD] = s->r[VH]; s->r[VH] = t;
			t = s->r[VE]; s->r[VE] = s->r[VL]; s->r[VL] = t;
		} else {
			setfresh(s, VH);	/* ex (sp),hl */
			setfresh(s, VL);
		}
		return;
	}

	if (strcmp(m, "exx") == 0) {
		vinit(s);			/* bc, de, hl swapped with shadow */
		return;
	}

	if (strcmp(m, "call") == 0) {
		vinit(s);			/* caller-saved set and the flags */
		return;
	}

	if (strcmp(m, "jp") == 0 || strcmp(m, "jr") == 0 ||
	    strcmp(m, "ret") == 0 || strcmp(m, "djnz") == 0 ||
	    strcmp(m, "reti") == 0 || strcmp(m, "retn") == 0 ||
	    strcmp(m, "rst") == 0) {
		vreset(s);
		return;
	}

	/* unknown instruction: assume it writes everything */
	vreset(s);
}
