/*
 * rules.c - the rewrites
 *
 * Each rule looks at the head of the window, and either rewrites it
 * and returns 1 or leaves it alone and returns 0.  When one fires the
 * window is examined again from the top, so a rule may feed another.
 *
 * The counts are what justify each of these.  They were taken over
 * ccc/pass2/rewrite.c, the largest source the compiler can build -
 * 25315 bytes of text, 13161 instructions:
 *
 *	inc sp runs before a pop	445 sites	~600 bytes
 *	push X / pop Y / push Y		 58 sites	 116 bytes
 *	ld a,<mem> then ld r,a		131 sites	 131 bytes
 *	ld a,r then and 0			 14 sites	  28 bytes
 *
 * Control flow was almost clean - no dead jumps, no unreachable
 * instructions, no jump chains.  The one branch shape worth having
 * turned out to be the loop bottom: a conditional hop over the jump
 * back, which inverting removes.
 *
 * Jump threading has been looked at twice now and is still not worth a
 * rule.  The shape it would collapse - a conditional jump landing on
 * another with the same condition - is all over the Whitesmith's
 * objects in attic/ (libu/sbreak.s tests a pair against -1 that way
 * twice), but this compiler does not emit it: the count above is zero.
 *
 * A later count, over the asmsnap corpus rather than rewrite.c - 118
 * files, 43014 instructions:
 *
 *	ld de,-1 / or a / sbc hl,de	 38 sites	  76 bytes
 */
#include <string.h>
#include <stdio.h>
#include "peep.h"

long n_incsp = 0;
long n_pushpop = 0;
long n_bounce = 0;
long n_and0 = 0;
long n_frame = 0;
long n_invjp = 0;
long n_outi = 0;
long n_exx = 0;
long n_constdup = 0;
long n_orclr = 0;
long n_reuse = 0;
long n_pushsrc = 0;
long n_ptrload = 0;
long n_ixcopy = 0;
long n_autozero = 0;
long n_xordup = 0;
long n_exde = 0;
long n_ldazero = 0;
long n_m1cmp = 0;
long n_ccall = 0;
long n_cret = 0;
long n_jpnext = 0;
long n_hlarg = 0;
long n_noframe = 0;
long saved = 0;

/* does the key at window line i match s exactly */
int
is(int i, char *s)
{
	if (i >= nwin || win[i].kind != L_INSN)
		return 0;
	return strcmp(win[i].key, s) == 0;
}

/* does the key at window line i start with s */
int
starts(int i, char *s)
{
	if (i >= nwin || win[i].kind != L_INSN)
		return 0;
	return strncmp(win[i].key, s, strlen(s)) == 0;
}

/*
 * Stack cleanup after a call is emitted as one inc sp per byte.  When
 * the run is followed by a pop, that pop's register is about to be
 * loaded from the stack anyway, so it is free to use as a sink: two
 * inc sp (two bytes) become one pop of the same register (one byte).
 *
 * This is the only rule here that needs nothing proved.  The register
 * is dead by inspection - the very next instruction writes it - and
 * the stack moves by the same amount either way.
 */
int
r_incsp(void)
{
	int n = 0, k, i;
	char buf[KLEN + 8];

	while (is(n, "inc sp"))
		n++;
	if (n < 2)
		return 0;
	if (!starts(n, "pop "))
		return 0;

	k = n / 2;
	sprintf(buf, "\t%s\n", win[n].key);
	delline(0, n - (n & 1));
	for (i = 0; i < k; i++)
		insline(0, buf);
	/* an odd byte, if there ever is one, keeps its inc sp */
	if (n & 1)
		insline(k, "\tinc sp\n");

	n_incsp++;
	saved += n - k - (n & 1);
	return 1;
}

/*
 * push X / pop Y / push Y - the value is moved into Y only to be
 * pushed straight back.  If nothing wants Y afterwards the move is
 * dead and the first push already put the right thing on the stack.
 *
 * This shows up because pass2 materialises an operand into hl before
 * pushing it, without knowing that the rule that produced the operand
 * had already left it somewhere pushable.
 */
int
r_pushpop(void)
{
	char x[KLEN], y[KLEN];

	if (!starts(0, "push ") || !starts(1, "pop ") || !starts(2, "push "))
		return 0;
	strcpy(x, win[0].key + 5);
	strcpy(y, win[1].key + 4);
	if (strcmp(y, win[2].key + 5) != 0)
		return 0;
	if (strcmp(x, y) == 0)
		return 0;				/* push hl/pop hl/push hl: not ours */

	/* is Y wanted after the third line */
	if (!isdead(reads(win[1].key) | writes(win[1].key), 3))
		return 0;

	delline(1, 2);
	n_pushpop++;
	saved += 2;
	return 1;
}

/*
 * ld a,<something> then ld r,a - the Z80 can load most things straight
 * into r, and going through a costs the extra ld.  Absolute addresses
 * are the exception: only a can be loaded from (nn), so those stay.
 */
int
r_bounce(void)
{
	char src[KLEN], dst;
	char buf[KLEN + 8];

	if (!starts(0, "ld a,") || !starts(1, "ld "))
		return 0;
	if (strlen(win[1].key) != 6)
		return 0;				/* "ld r,a" and nothing else */
	if (strcmp(win[1].key + 4, ",a") != 0)
		return 0;
	dst = win[1].key[3];
	if (!strchr("bcdehl", dst))
		return 0;

	strcpy(src, win[0].key + 5);
	/* (nn) loads only into a; (hl) and (ix+d) load into any register */
	if (src[0] == '(' && !memok(src))
		return 0;
	/* ld r,r' and ld r,(hl)/(ix+d)/(iy+d) and ld r,n are all encodable */
	if (!isdead(R_A, 2))
		return 0;

	sprintf(buf, "\tld %c,%s\n", dst, src);
	delline(0, 2);
	insline(0, buf);
	n_bounce++;
	saved += 1;
	return 1;
}

/*
 * and 0 always produces zero, so whatever was loaded into a first was
 * loaded for nothing.  Three bytes become one.
 *
 * pass2 emits this when a mask works out to zero rather than noticing
 * that the answer is a constant; the fix belongs in the rules table
 * too, but the sequence is cheap to spot here and the table is not the
 * only thing that can produce it.
 */
int
r_and0(void)
{
	if (!starts(0, "ld a,") || !is(1, "and 0"))
		return 0;
	/* the load is only there to feed the and, whose answer ignores it */
	delline(0, 2);
	insline(0, "\txor a\n");
	n_and0++;
	saved += 2;
	return 1;
}

/*
 * The frame setup and teardown every function carries, moved into one
 * shared copy.  This is zc3's arrangement - it calls ncsv and jumps to
 * cret - but it cannot use zc3's helpers: those point ix at the frame
 * and save both index registers, and ccc points iy at the frame and
 * needs ix free.  fenter and fexit in libc/csv.s lay the frame out
 * exactly as these three instructions did, so every (iy+d) already
 * emitted still addresses what it did before.
 *
 *	push iy / ld iy,0 / add iy,sp	8 bytes -> call fenter	3
 *	ld sp,iy / pop iy / ret			5 bytes -> jp fexit		3
 *
 * 49 functions in rewrite.c, so 343 bytes there.  The register saves
 * and the frame allocation stay where they are: they are conditional,
 * and they have to happen after iy is pointing at the frame.
 */
int
r_fenter(void)
{
	if (!is(0, "push iy") || !is(1, "ld iy,0") || !is(2, "add iy,sp"))
		return 0;
	/*
	 * fenter reaches its caller through hl.  On entry to a function
	 * that takes its arguments on the stack hl holds nothing, and
	 * this sequence only occurs there - but it is cheap to insist,
	 * and the day some other code grows an iy frame it will matter.
	 */
	if (!isdead(R_HL, 3))
		return 0;

	delline(0, 3);
	insline(0, "\tcall\tfenter\n");
	n_frame++;
	saved += 5;
	return 1;
}

int
r_fexit(void)
{
	if (!is(0, "ld sp,iy") || !is(1, "pop iy") || !is(2, "ret"))
		return 0;

	delline(0, 3);
	insline(0, "\tjp\tfexit\n");
	n_frame++;
	saved += 2;
	return 1;
}

/*
 * A conditional hop over an unconditional jump, landing on the very
 * next label:
 *
 *	jp z,L1
 *	jp L2
 * L1:
 *
 * is "jp nz,L2" and the label.  Loop bottoms all look like this -
 * the condition is emitted as "skip the jump back when false" - so
 * every do-while and for pays the three bytes this returns.  The
 * label stays: anything else may target it.
 */
static char *ccinv[] = {
	"z", "nz", "nz", "z", "c", "nc", "nc", "c",
	"m", "p", "p", "m", "pe", "po", "po", "pe", 0
};

/* the next line that is code or a label, skipping comments and
 * blanks - they sit between every statement and must not blind a
 * rule that spans one */
int
nextsig(int i)
{
	for (i++; i < nwin; i++)
		if (win[i].kind != L_BLANK)
			return i;
	return -1;
}

int
r_invjp(void)
{
	char cc[4];
	char buf[KLEN + 8];
	char *comma;
	int i, n, j1, j2;

	if (!starts(0, "jp "))
		return 0;
	j1 = nextsig(0);
	if (j1 < 0 || win[j1].kind != L_INSN ||
	    strncmp(win[j1].key, "jp ", 3) != 0)
		return 0;
	j2 = nextsig(j1);
	if (j2 < 0 || win[j2].kind != L_LABEL)
		return 0;
	comma = strchr(win[0].key + 3, ',');
	if (!comma)
		return 0;
	if (strchr(win[j1].key + 3, ','))
		return 0;			/* second jump conditional: not ours */
	n = comma - (win[0].key + 3);
	if (n < 1 || n > 2)
		return 0;
	memcpy(cc, win[0].key + 3, n);
	cc[n] = 0;

	/* the label the hop targets must be the next one down */
	n = strlen(comma + 1);
	if (strncmp(win[j2].key, comma + 1, n) != 0 ||
	    win[j2].key[n] != ':' || win[j2].key[n + 1] != '\0')
		return 0;

	for (i = 0; ccinv[i]; i += 2)
		if (strcmp(cc, ccinv[i]) == 0)
			break;
	if (!ccinv[i])
		return 0;

	sprintf(buf, "\tjp %s,%s\n", ccinv[i + 1], win[j1].key + 3);
	delline(j1, 1);
	delline(0, 1);
	insline(0, buf);
	n_invjp++;
	saved += 3;
	return 1;
}

/*
 * out() with a constant argument, which the code generator says 182
 * times:
 *
 *	ld hl,strN
 *	push hl
 *	call _out
 *	inc sp
 *	inc sp
 *
 * is "call oarg" with the argument planted inline after the call -
 * the swtab trick - and the libc helper feeds it to _out through
 * the return address.  Nine bytes become five.  Must run before the
 * inc sp rule turns the cleanup into a pop.  Only a constant
 * operand qualifies: "ld hl,(sym)" is a load, and its value cannot
 * be spelled in a .dw.
 */
/*
 * A call jumped around, which is what "if (c) foo();" comes to when
 * foo takes no arguments:
 *
 *	jp z,L		3	    call nz,_foo	3
 *	call _foo	3  ->
 * L:			    L:
 *
 * The Z80 calls on a condition as readily as it jumps on one, so the
 * hop is the whole cost.  Six bytes become three, 35 sites over the
 * compiler's own sources.
 *
 * The label stays.  Another jump may target it - "if (a && b) foo();"
 * lands two of them there, and only the second is adjacent to the call
 * - and it costs nothing to leave.
 *
 * Only when the call is the ONLY thing hopped over.  With arguments
 * the pushes and the caller's cleanup sit between the branch and the
 * label, and making just the call conditional would push for a call
 * that never happens.  Those cases fail the adjacency test below and
 * are left alone.
 */
int
r_ccall(void)
{
	char cc[4];
	char buf[KLEN + 8];
	char *comma;
	int i, n, j1, j2;

	if (!starts(0, "jp "))
		return 0;
	j1 = nextsig(0);
	if (j1 < 0 || win[j1].kind != L_INSN || strncmp(win[j1].key, "call ", 5) != 0)
		return 0;
	if (strchr(win[j1].key + 5, ','))
		return 0;			/* already conditional */
	j2 = nextsig(j1);
	if (j2 < 0 || win[j2].kind != L_LABEL)
		return 0;
	comma = strchr(win[0].key + 3, ',');
	if (!comma)
		return 0;			/* unconditional hop: not ours */
	n = comma - (win[0].key + 3);
	if (n < 1 || n > 2)
		return 0;
	memcpy(cc, win[0].key + 3, n);
	cc[n] = 0;

	/* the label hopped to must be the one right after the call */
	n = strlen(comma + 1);
	if (strncmp(win[j2].key, comma + 1, n) != 0 ||
	    win[j2].key[n] != ':' || win[j2].key[n + 1] != '\0')
		return 0;

	for (i = 0; ccinv[i]; i += 2)
		if (strcmp(cc, ccinv[i]) == 0)
			break;
	if (!ccinv[i])
		return 0;

	sprintf(buf, "\tcall %s,%s\n", ccinv[i + 1], win[j1].key + 5);
	delline(j1, 1);
	delline(0, 1);
	insline(0, buf);
	n_ccall++;
	saved += 3;
	return 1;
}

/*
 * And the same for a return, which is "if (c) return;" in a function
 * that has no frame to unwind:
 *
 *	jp z,L		3	    ret nz		1
 *	ret		1  ->
 * L:			    L:
 *
 * Four bytes become one.  This only arises since pass2 started
 * returning with a ret rather than a jump to the epilogue - it can do
 * that only where there is no frame, and where there is one the return
 * is a jp to the unwind and the shape above never appears.
 */
int
r_cret(void)
{
	char cc[4];
	char buf[KLEN + 8];
	char *comma;
	int i, n, j1, j2;

	if (!starts(0, "jp "))
		return 0;
	j1 = nextsig(0);
	if (j1 < 0 || win[j1].kind != L_INSN || strcmp(win[j1].key, "ret") != 0)
		return 0;
	j2 = nextsig(j1);
	if (j2 < 0 || win[j2].kind != L_LABEL)
		return 0;
	comma = strchr(win[0].key + 3, ',');
	if (!comma)
		return 0;
	n = comma - (win[0].key + 3);
	if (n < 1 || n > 2)
		return 0;
	memcpy(cc, win[0].key + 3, n);
	cc[n] = 0;

	n = strlen(comma + 1);
	if (strncmp(win[j2].key, comma + 1, n) != 0 ||
	    win[j2].key[n] != ':' || win[j2].key[n + 1] != '\0')
		return 0;

	for (i = 0; ccinv[i]; i += 2)
		if (strcmp(cc, ccinv[i]) == 0)
			break;
	if (!ccinv[i])
		return 0;

	sprintf(buf, "\tret %s\n", ccinv[i + 1]);
	delline(j1, 1);
	delline(0, 1);
	insline(0, buf);
	n_cret++;
	saved += 3;
	return 1;
}

/*
 * A jump to the very next instruction, which is no jump at all.
 *
 *	jp Xfoo
 * Xfoo:
 *
 * Every function whose last statement is a return ends this way: the
 * return is compiled as a jump to the epilogue, and the epilogue is
 * what comes next.  pass2 cannot see it - it emits the jump while
 * walking the body and the label when the body is done - and it is
 * three bytes in 379 places over the compiler's own sources.
 *
 * The label stays: it is what every OTHER return in the function
 * jumps to, and only this one jump is redundant.
 */
int
r_jpnext(void)
{
	int j1, n;

	if (!starts(0, "jp ") || strchr(win[0].key + 3, ','))
		return 0;			/* conditional: it may fall through */
	j1 = nextsig(0);
	if (j1 < 0 || win[j1].kind != L_LABEL)
		return 0;
	n = strlen(win[0].key + 3);
	if (strncmp(win[j1].key, win[0].key + 3, n) != 0 ||
	    win[j1].key[n] != ':' || win[j1].key[n + 1] != '\0')
		return 0;
	delline(0, 1);
	n_jpnext++;
	saved += 3;
	return 1;
}

int
r_outi(void)
{
	char buf[KLEN + 16];
	int j1;

	/*
	 * The first argument travels in HL now, so the site is just the
	 * load and the call - six bytes - and the inline form is five.
	 * One byte rather than the four the stack convention gave up,
	 * but it is still the most repeated call in the code generator.
	 */
	if (win[0].kind != L_INSN ||
	    !starts(0, "ld hl,") || win[0].key[6] == '(')
		return 0;
	j1 = nextsig(0);
	if (j1 < 0 || win[j1].kind != L_INSN ||
	    strcmp(win[j1].key, "call _out") != 0)
		return 0;

	sprintf(buf, "\t.dw %s\n", win[0].key + 6);
	/* back to front, so the indices stay true */
	delline(j1, 1);
	delline(0, 1);
	insline(0, "\tcall oarg\n");
	insline(1, buf);
	n_outi++;
	saved += 1;
	return 1;
}

/*
 * The first argument arrives in HL, and the w-family entry helpers
 * promise that HL reaches the body intact and EQUAL TO (iy+4) - see
 * libc/csv.s.  So a read of (iy+4) made before anything writes H or
 * L is a read of a value a register already holds:
 *
 *	ld l,(iy+4) / ld h,(iy+5)	deleted			6 bytes
 *	ld c,(iy+4) / ld b,(iy+5)	ld c,l / ld b,h		4
 *	ld a,(iy+4)			ld a,l			2
 *
 * Measured on the stock compiler over its own sources before the
 * convention changed (HLARG-PROJECTION.md): 190 functions reload the
 * first parameter into HL as their first act, 53 stage it into BC,
 * 40 read it as a byte - about 1,430 bytes, and it hands back the
 * time the helper's spill costs in exactly the common case.
 *
 * The property belongs to the five w names and is asserted here, not
 * analysed.  The q family makes no such promise - a long's high word
 * is in HL' and the helper does not reload it - and is not matched.
 * The scan looks past anything that provably cannot disturb HL or
 * the slot: no stores (a push is fine - the slot is above IY and SP
 * stays below it), no branches, no calls, nothing that writes H, L
 * or IY.  A label ends it: someone may arrive there another way.
 */
int
r_hlarg(void)
{
	int j, k;

	if (!is(0, "call fentbw") && !is(0, "call fentxw") &&
	    !is(0, "call fentbxw") && !is(0, "call fentnw") &&
	    !is(0, "call fenterw"))
		return 0;

	for (j = 1; j < nwin; j++) {
		if (win[j].kind == L_BLANK || win[j].kind == L_DIRECT)
			continue;	/* the .dw rides after the call */
		if (win[j].kind != L_INSN)
			return 0;

		if (is(j, "ld l,(iy+4)")) {
			k = nextsig(j);
			if (k < 0 || !is(k, "ld h,(iy+5)"))
				return 0;	/* half a load: leave it */
			delline(k, 1);
			delline(j, 1);
			n_hlarg++;
			saved += 6;
			return 1;
		}
		if (is(j, "ld c,(iy+4)")) {
			k = nextsig(j);
			if (k > 0 && is(k, "ld b,(iy+5)")) {
				delline(k, 1);
				insline(k, "\tld b,h\n");
				delline(j, 1);
				insline(j, "\tld c,l\n");
				saved += 4;
			} else {
				delline(j, 1);
				insline(j, "\tld c,l\n");
				saved += 2;
			}
			n_hlarg++;
			return 1;
		}
		if (is(j, "ld e,(iy+4)")) {
			k = nextsig(j);
			if (k > 0 && is(k, "ld d,(iy+5)")) {
				delline(k, 1);
				insline(k, "\tld d,h\n");
				delline(j, 1);
				insline(j, "\tld e,l\n");
				saved += 4;
			} else {
				delline(j, 1);
				insline(j, "\tld e,l\n");
				saved += 2;
			}
			n_hlarg++;
			return 1;
		}
		if (is(j, "ld b,(iy+4)")) {
			delline(j, 1);
			insline(j, "\tld b,l\n");
			n_hlarg++;
			saved += 2;
			return 1;
		}
		if (is(j, "ld a,(iy+4)")) {
			delline(j, 1);
			insline(j, "\tld a,l\n");
			n_hlarg++;
			saved += 2;
			return 1;
		}

		if (starts(j, "ld (") || starts(j, "call") ||
		    isbranch(win[j].key))
			return 0;
		if (writes(win[j].key) & (R_HL | R_IY))
			return 0;
	}
	return 0;
}

/*
 * The frame that stops being necessary - see nfemit() in peep.c.  It
 * is not a window rule: the tree's frame-free functions run past a
 * hundred significant lines, and a window that held whole functions
 * would be the buffering this program exists to avoid.  The counter
 * lives here with the others.
 */

/*
 * A 16-bit equality test against -1, which is how every failed system
 * call is checked:
 *
 *	ld de,-1		3
 *	or a			1	clear the carry for the sbc
 *	sbc hl,de		2
 *	jp z|nz,L
 *
 * hl - 0ffffh is hl + 1, and hl + 1 carries out of bit 15 exactly when
 * hl was 0ffffh.  The add says the same thing in four bytes:
 *
 *	ld de,1			3
 *	add hl,de		1
 *	jp c|nc,L
 *
 * HL is left holding hl+1 either way, bit for bit, so nothing that
 * reads it afterwards can tell the two apart.  Neither form disturbs A:
 * the "or a" is a carry clear, not a use of it.  What moves is which
 * flag carries the answer, so the branch condition goes from z/nz to
 * c/nc, and S, P/V and the old carry stop meaning anything.
 *
 * That is this rule's one assumption - that nothing downstream reads
 * the flags except the branch on the end - and it cannot be proved
 * here.  isdead() stops at a branch (regs.c) because what happens after
 * one depends on where it goes, and the taken path is somewhere else in
 * the file.  So it was measured instead.  Over the asmsnap corpus, 118
 * files and 43014 instructions, the sequence occurs 38 times, and at
 * every one of them the first instruction on BOTH the fall through and
 * the branch target writes the flags rather than reading them - a load,
 * a push, a call, or an scf.  pass2 emits a compare and its branch as
 * one unit and never carries a condition into a join.
 *
 * The tempting rewrite is "ld a,h / and l / inc a", which is three
 * bytes rather than four.  It cannot be had: it clobbers A, so it needs
 * isdead(R_A) across the branch, which always answers no - and rightly,
 * because A really does arrive live at 21 labels in that same corpus.
 *
 * 38 sites, two bytes each.
 */
static char *ccm1[] = { "z", "c", "nz", "nc", 0 };

int
r_m1cmp(void)
{
	char cc[4];
	char buf[KLEN + 8];
	char *comma;
	int i, n, j1, j2, j3;

	if (!is(0, "ld de,-1"))
		return 0;
	j1 = nextsig(0);
	if (j1 < 0 || !is(j1, "or a"))
		return 0;
	j2 = nextsig(j1);
	if (j2 < 0 || !is(j2, "sbc hl,de"))
		return 0;
	j3 = nextsig(j2);
	if (j3 < 0 || win[j3].kind != L_INSN)
		return 0;
	if (strncmp(win[j3].key, "jp ", 3) != 0 &&
	    strncmp(win[j3].key, "jr ", 3) != 0)
		return 0;

	/* the branch has to be the one consuming the answer */
	comma = strchr(win[j3].key + 3, ',');
	if (!comma)
		return 0;			/* unconditional: nothing to translate */
	n = comma - (win[j3].key + 3);
	if (n < 1 || n > 2)
		return 0;
	memcpy(cc, win[j3].key + 3, n);
	cc[n] = 0;
	for (i = 0; ccm1[i]; i += 2)
		if (strcmp(cc, ccm1[i]) == 0)
			break;
	if (!ccm1[i])
		return 0;			/* branches on a flag the add does not set */

	/* %.2s keeps the mnemonic: jr c and jr nc are both encodable */
	sprintf(buf, "\t%.2s %s,%s\n", win[j3].key, ccm1[i + 1], comma + 1);
	/* back to front, so the indices stay true */
	delline(j3, 1);
	delline(j2, 1);
	delline(j1, 1);
	delline(0, 1);
	insline(0, "\tld de,1\n");
	insline(1, "\tadd hl,de\n");
	insline(2, buf);
	n_m1cmp++;
	saved += 2;
	return 1;
}

/*
 * exx is its own inverse, so two of them in a row are two bytes that
 * do nothing.  They arise constantly now that a long lives in HL':HL:
 * every load, store and widen brackets its high word in a pair, and
 * where two long operations abut - which is most of them, one operand
 * being loaded straight after the one before - the closing exx of the
 * first meets the opening exx of the second.
 *
 * No liveness question has to be asked of this one.  Nothing can
 * observe the state between the two instructions, because there is
 * nothing between them.
 */
int
r_exx(void)
{
	if (!is(0, "exx") || !is(1, "exx"))
		return 0;

	delline(0, 2);
	n_exx++;
	saved += 2;
	return 1;
}

/*
 * A constant load into a register that already holds that constant.
 * The value state knows what every register holds, so this is one
 * query and no forward scan.  It is what turns func(0,0,0) into one
 * ld hl,0 and three pushes, and it subsumes the ld h,0 zero-extend
 * rule that used to live here - that was this same check, hard-wired
 * to the one constant 0.
 */
int
r_constdup(void)
{
	int n = vredundant(&vbase, win[0].key);

	if (!n)
		return 0;
	delline(0, 1);
	n_constdup++;
	saved += n;
	return 1;
}

/*
 * Register reuse: pass2 materialises a constant into a pair right
 * before an add that consumes it, even when the constant is already
 * sitting in the other pair.  Reuse it instead of loading it again:
 *
 *	ld de,2             add hl,bc
 *	add hl,de     ->
 *
 * which is "bcreg = 2; func(someint+2)" compiling to add hl,bc.  The
 * pair being discarded has to be dead after the add, because the
 * rewrite leaves it holding whatever it held before the load.
 */
int
r_reuse(void)
{
	char m[8];
	char buf[KLEN + 8];
	char *op, *o2, *jop;
	int hi, lo, ohi, olo, n, j;

	mnemof(win[0].key, m, sizeof(m));
	if (strcmp(m, "ld") != 0)
		return 0;
	op = operof(win[0].key);
	o2 = oper2(op);

	if (strncmp(op, "de,", 3) == 0) { hi = VD; lo = VE; ohi = VB; olo = VC; }
	else if (strncmp(op, "bc,", 3) == 0) { hi = VB; lo = VC; ohi = VD; olo = VE; }
	else
		return 0;

	if (!vnumber(o2, &n))
		return 0;
	if (!vpairconst(&vbase, ohi, olo, n))
		return 0;

	j = nextsig(0);
	if (j < 0 || win[j].kind != L_INSN)
		return 0;
	jop = operof(win[j].key);
	if (strncmp(win[j].key, "add ", 4) != 0 || strncmp(jop, "hl,", 3) != 0)
		return 0;
	if (strncmp(jop + 3, op, 2) != 0)	/* the add consumes this pair */
		return 0;

	/* the discarded pair must be dead after the add */
	if (!isdead(hi | lo, j + 1))
		return 0;

	sprintf(buf, "\tadd hl,%s\n", (hi == VD) ? "bc" : "de");
	delline(j, 1);
	delline(0, 1);
	insline(0, buf);
	n_reuse++;
	saved += 3;
	return 1;
}

/*
 * ld l,c / ld h,b / push hl copies BC into HL only to push it.  push
 * bc does the same thing directly, two bytes shorter.  HL has to be
 * dead after the push - the rewrite leaves it holding whatever it held
 * before the copy.
 */
int
r_pushsrc(void)
{
	int j, k;

	if (!is(0, "ld l,c"))
		return 0;
	j = nextsig(0);
	if (j < 0 || !is(j, "ld h,b"))
		return 0;
	k = nextsig(j);
	if (k < 0 || !is(k, "push hl"))
		return 0;
	if (!isdead(R_HL, k + 1))
		return 0;

	delline(k, 1);
	delline(j, 1);
	delline(0, 1);
	insline(0, "\tpush bc\n");
	n_pushsrc++;
	saved += 2;
	return 1;
}

/*
 * ld l,c / ld h,b / ld a,(hl) copies a pointer from BC into HL only to
 * dereference it once.  ld a,(bc) reads through BC directly, two bytes
 * shorter - the Z80 keeps the 8080's LDAX B.  As with r_pushsrc, HL has
 * to be dead after the load, because the rewrite leaves it holding
 * whatever it held before the copy.
 */
int
r_ptrload(void)
{
	int j, k;

	if (!is(0, "ld l,c"))
		return 0;
	j = nextsig(0);
	if (j < 0 || !is(j, "ld h,b"))
		return 0;
	k = nextsig(j);
	if (k < 0 || !is(k, "ld a,(hl)"))
		return 0;
	if (!isdead(R_HL, k + 1))
		return 0;

	delline(k, 1);
	delline(j, 1);
	delline(0, 1);
	insline(0, "\tld a,(bc)\n");
	n_ptrload++;
	saved += 2;
	return 1;
}

/*
 * ld c,ixl / ld b,ixh copies IX into BC two bytes at a time through the
 * undocumented half registers.  push ix / pop bc is the same move one
 * byte shorter, and it needs no liveness proof: the destination ends
 * up holding IX either way, and neither form disturbs the flags.
 */
int
r_ixcopy(void)
{
	char buf[KLEN + 8];
	char *src, *dst;

	if (is(0, "ld c,ixl") && is(1, "ld b,ixh")) { src = "ix"; dst = "bc"; }
	else if (is(0, "ld e,ixl") && is(1, "ld d,ixh")) { src = "ix"; dst = "de"; }
	else if (is(0, "ld l,ixl") && is(1, "ld h,ixh")) { src = "ix"; dst = "hl"; }
	else
		return 0;

	sprintf(buf, "\tpush %s\n\tpop %s\n", src, dst);
	delline(0, 2);
	insline(0, buf);
	n_ixcopy++;
	saved += 1;
	return 1;
}

/* is window line i a zeroing of an auto: ld (iy+d),0 or ld (ix+d),0 */
static int
is_zerostore(int i)
{
	char *op;

	if (i >= nwin || win[i].kind != L_INSN)
		return 0;
	if (strncmp(win[i].key, "ld (", 4) != 0)
		return 0;
	op = operof(win[i].key);
	if (strncmp(op, "(iy", 3) != 0 && strncmp(op, "(ix", 3) != 0)
		return 0;
	return strcmp(oper2(op), "0") == 0;
}

/*
 * A run of autos initialised to zero - "int a,b,c,d;" - is emitted as
 * one "ld (iy+d),0" per byte, four bytes each.  Load A with zero once
 * and store it instead: "xor a" and one "ld (iy+d),a" per byte, three
 * bytes each.  A run of n stores is n-1 bytes shorter, at the cost of
 * one line - the xor a - which is what the slack slot in the window is
 * for.  A and the flags are the price of the xor, so both have to be
 * dead once the run is past.
 */
int
r_autozero(void)
{
	int n = 0, i;
	char buf[KLEN + 8];
	char full[KLEN + 16];
	char *p;

	while (is_zerostore(n))
		n++;
	if (n < 2)
		return 0;
	if (!isdead(R_A | R_F, n))
		return 0;

	/* back to front so the indices stay true: each store retargets to A */
	for (i = n - 1; i >= 0; i--) {
		strcpy(buf, win[i].key);	/* "ld (iy+d),0" */
		p = strrchr(buf, ',');
		p[1] = 'a';
		p[2] = 0;				/* "ld (iy+d),a" */
		sprintf(full, "\t%s\n", buf);
		delline(i, 1);
		insline(i, full);
	}
	insline(0, "\txor a\n");
	n_autozero++;
	saved += n - 1;
	return 1;
}

/*
 * xor a when A is already known to be zero.  It still clears the carry
 * and sets Z, so the flags have to be dead after it; the value state
 * says the value is already there, which is the only thing this rule
 * needs beyond that.
 */
int
r_xordup(void)
{
	if (!is(0, "xor a"))
		return 0;
	if (!viszero(&vbase, VA))
		return 0;
	if (!isdead(R_F, 1))
		return 0;
	delline(0, 1);
	n_xordup++;
	saved += 1;
	return 1;
}

/*
 * ex de,hl is its own inverse, so two of them in a row - in either
 * spelling - are two bytes that do nothing.  The same shape r_exx
 * already catches for the shadow set.
 */
int
r_exde(void)
{
	if (!starts(0, "ex ") || !starts(1, "ex "))
		return 0;
	if ((strcmp(win[0].key + 3, "de,hl") != 0 &&
	     strcmp(win[0].key + 3, "hl,de") != 0) ||
	    (strcmp(win[1].key + 3, "de,hl") != 0 &&
	     strcmp(win[1].key + 3, "hl,de") != 0))
		return 0;

	delline(0, 2);
	n_exde++;
	saved += 2;
	return 1;
}

/*
 * ld a,0 is a two-byte zero of A that leaves the flags alone.  xor a is
 * the one-byte form; it clobbers the flags, so they have to be dead.
 */
int
r_ldazero(void)
{
	if (!is(0, "ld a,0"))
		return 0;
	if (!isdead(R_F, 1))
		return 0;

	delline(0, 1);
	insline(0, "\txor a\n");
	n_ldazero++;
	saved += 1;
	return 1;
}

/*
 * or a before sbc clears the carry a 16-bit subtract borrows.  When
 * the carry is already clear the or a is a byte spent on a flag that
 * is already the right way.  The value state carries the carry across
 * the run, so this is one query instead of a forward scan and a
 * whitelist of what leaves C alone.
 */
int
r_orclr(void)
{
	int j;

	if (!is(0, "or a"))
		return 0;
	j = nextsig(0);
	if (j < 0 || !starts(j, "sbc "))
		return 0;
	if (!visclear(&vbase))
		return 0;
	delline(0, 1);
	n_orclr++;
	saved += 1;
	return 1;
}

/*
 * Which rules can possibly match, rather than all of them.
 *
 * Every rule opens by demanding a particular opcode at the head of
 * the window, so one character of the key rules most of them out.
 * No two rules under different arms here can want the same opcode,
 * which is why the order between arms cannot matter; inside an arm
 * it is the order they were always tried in.
 *
 * The whole list used to run at every window position - fourteen
 * calls, nearly all of them returning on their first comparison,
 * 1,027,880 of them over the compiler's own sources.  The ld arm is
 * a third of all lines by itself and splits again on the register it
 * loads, which leaves 66,492.
 */
int
applyrules(void)
{
	char *k = win[0].key;

	switch (k[0]) {
	case 'e':
		if (r_exx())
			return 1;
		return r_exde();
	case 'c':
		return r_hlarg();
	case 'i':
		return r_incsp();
	case 'o':
		return r_orclr();
	case 'x':
		return r_xordup();
	case 'p':
		if (r_fenter())
			return 1;
		return r_pushpop();
	case 'j':
		if (r_invjp())
			return 1;
		if (r_ccall())
			return 1;
		if (r_cret())
			return 1;
		return r_jpnext();
	case 'l':
		if (k[1] != 'd' || k[2] != ' ')
			return 0;
		if (k[3] == 's')
			return r_fexit();	/* ld sp,iy */
		if (k[3] == 'h' && r_outi())	/* ld hl,n -> oarg */
			return 1;
		if (k[3] == 'd' && r_m1cmp())	/* ld de,-1 */
			return 1;
		if (k[3] == 'a') {
			if (r_bounce())		/* ld a,x then ld r,a */
				return 1;
			if (r_and0())		/* ld a,x then and 0 */
				return 1;
			if (r_ldazero())	/* ld a,0 -> xor a */
				return 1;
		}
		if (k[3] == 'l' && r_pushsrc())	/* ld l,c ; ld h,b ; push hl -> push bc */
			return 1;
		if (k[3] == 'l' && r_ptrload())	/* ld l,c ; ld h,b ; ld a,(hl) -> ld a,(bc) */
			return 1;
		if (r_ixcopy())			/* ld c,ixl ; ld b,ixh -> push ix ; pop bc */
			return 1;
		if (r_autozero())		/* ld (iy+d),0 ... -> xor a ; ld (iy+d),a */
			return 1;
		if (r_reuse())			/* ld de,n ; add hl,de -> add hl,bc */
			return 1;
		return r_constdup();		/* ld reg,const already there */
	}
	return 0;
}

void
report(void)
{
	/*
	 * Two calls, not one.  A string literal is bounded at 126
	 * characters here - past that c0 says "expr paren" or, spliced,
	 * "unknown error" - and the one line this used to be was 125,
	 * so the counters added since would have run it off the end.
	 */
	fprintf(stderr, "peep: frame %ld  incsp %ld  pushpop %ld"
		"  bounce %ld  and0 %ld  invjp %ld  oarg %ld\n",
		n_frame, n_incsp, n_pushpop, n_bounce, n_and0, n_invjp,
		n_outi);
	fprintf(stderr, "peep: exx %ld  m1cmp %ld  ccall %ld  cret %ld"
		"  jpnext %ld  hlarg %ld  noframe %ld  pool %ld = %ld bytes\n",
		n_exx, n_m1cmp, n_ccall, n_cret, n_jpnext, n_hlarg,
		n_noframe, poolmerged, saved);
	fprintf(stderr, "peep: constdup %ld  orclr %ld  reuse %ld  pushsrc %ld"
		"  ptrload %ld  ixcopy %ld  autozero %ld  xordup %ld\n",
		n_constdup, n_orclr, n_reuse, n_pushsrc, n_ptrload, n_ixcopy,
		n_autozero, n_xordup);
	fprintf(stderr, "peep: exde %ld  ldazero %ld\n", n_exde, n_ldazero);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
