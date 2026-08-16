/*
 * Register allocation and frame offset assignment for locals
 */
#include <stdlib.h>
#include "p1core.h"
#include "p1type.h"
#include "p1name.h"
#include "p1stmt.h"
#include "p1lex.h"

/* scalar area size of the current function: the callee-save slots sit
 * just below it (outast emits this as the FUNC header savebase) */
int frameSaveBase;

/*
 * Assign stack frame offsets to parameters and locals.
 * Params get positive offsets (above FP), locals get negative (below FP).
 * Register-allocated variables get frm_off=0 (not on stack).
 * Returns frame size (bytes needed for locals on stack).
 */
int
assignFrmOff(struct name *func)
{
	struct name *p;
	struct local *n, *locals;
	int off, maxoff, i;
	int mark[MAXBLKLVL];
	unsigned char arrays, prevlvl, prevblk;

	frameSaveBase = 0;
	if (!func->type || !func->u.locals)
		return 0;
	locals = func->u.locals;

	/* Parameters: positive offsets starting at +4 (skip saved FP + ret addr) */
	off = 4;
	for (p = func->type->elem; p; p = p->next) {
		if (p->type->size == 0)
			continue;  /* skip void */
		if (p->id) {
			struct local *local;
			for (local = locals; local; local = local->next)
				if (local->id == p->id) {
					local->frm_off = off;
					break;
				}
		}
		off += p->type->size > 2 ? p->type->size : 2;
	}

	/*
	 * Locals: negative offsets for non-register vars.
	 * Scalars go first, nearest the frame pointer, so they stay
	 * inside the 7-bit (iy+d) window.  Arrays follow, below a
	 * 4-byte gap reserved for the callee-save slots: array bases
	 * are formed with 16-bit address arithmetic, so they may sit
	 * past the window.
	 */
	/*
	 * Locals in nested blocks share space with their siblings.  Two
	 * blocks at the same level cannot both be live, so "if (a) { int
	 * b; } else { int c; }" wants one slot and not two.
	 *
	 * C puts declarations at the head of a block, so capLocals hands
	 * them over in the order the blocks were entered: a block's own
	 * locals, then the blocks inside it.  That is a depth-first walk
	 * of the scope tree already, and one pass over it is enough.
	 * mark[] remembers where each level started; going deeper sets
	 * the mark, and a new block at the same level rewinds to it.
	 *
	 * The frame is the deepest the running offset ever reached, not
	 * where it ended.
	 */
	off = 0;
	maxoff = 0;
	prevlvl = 2;
	prevblk = 0;
	for (i = 0; i < MAXBLKLVL; i++)
		mark[i] = 0;
	for (n = locals; n; n = n->next) {
		if (n->kind == kfunarg)
			continue;
		if (n->reg) {
			n->frm_off = 0;
			continue;
		}
		/*
		 * A static local is addressed by its S<n> label and has its
		 * storage in data - that is what makes it survive the call -
		 * so a frame slot for it is reserved on every call and never
		 * touched.  canAlloc has refused it a register for the same
		 * reason since a read of one came back as the address; the
		 * frame accounting never asked.
		 *
		 * Nothing was wrong at runtime, which is why it stood: the
		 * cost is stack, and it scales with the declaration, which
		 * is backwards.  The bigger the buffer the more reason there
		 * was to make it static, and the more stack it cost - ld's
		 * copy_segment declared "static unsigned char cbuf[512]"
		 * precisely to keep it off the frame, and reported
		 * frame=522.  On a machine where text, data, heap and stack
		 * share one 64K, that is half a kilobyte the heap cannot
		 * have, and a frame size that lies about where it went.
		 */
		if (n->sclass & SC_STATIC) {
			n->frm_off = 0;
			continue;
		}
		if (n->type->flags & TF_ARRAY)
			continue;
		if (n->level < MAXBLKLVL &&
		    (n->level != prevlvl || n->blkid != prevblk)) {
			if (n->level > prevlvl)
				mark[n->level] = off;
			else
				off = mark[n->level];
			prevlvl = n->level;
			prevblk = n->blkid;
		}
		off += n->type->size;
		n->frm_off = -off;
		if (off > maxoff)
			maxoff = off;
	}
	off = maxoff;
	frameSaveBase = off;
	if (off > 120)
		gripe(ER_D_FL);
	arrays = 0;
	for (n = locals; n; n = n->next) {
		if (n->kind == kfunarg || n->reg)
			continue;
		if (n->sclass & SC_STATIC) {	/* in data, see above */
			n->frm_off = 0;
			continue;
		}
		if (!(n->type->flags & TF_ARRAY))
			continue;
		if (!arrays) {
			arrays = 1;
			off += 4;	/* callee-save slots */
		}
		/*
		 * typesize, not the byte-wide size field: a type node holds
		 * "how big is one of me" in a char, and an array keeps its
		 * real extent as count times the element.  Reading size here
		 * gave every array its extent modulo 256, so qsort's
		 * "char xbuf[800]" got 32 bytes of frame and the pivot copy
		 * wrote 800 bytes through it - over the saved registers, the
		 * return address and whatever else was below.
		 */
		off += typesize(n->type);
		n->frm_off = -off;
	}
	return off;  /* frame size = total local stack space */
}

/* Check if variable can be allocated to a register */
int
canAlloc(struct local *n, int no_arg_regs)
{
	if (n->reg != REG_NONE || n->addr_taken)
		return 0;
	/*
	 * A static local lives at a label, not in the frame and not in
	 * a register: it has to survive the call, and a register does
	 * not.  It is a local by level, which is what let it through
	 * here - and being handed a register made a read of it come
	 * back as the address.
	 */
	if (n->sclass & SC_STATIC)
		return 0;
	if (no_arg_regs && n->kind == kfunarg)
		return 0;
	return 1;
}

/* Register allocation for local variables */
void
allocRegs(struct local *locals)
{
	struct local *n, *best;
	char regs = 0;  /* bits: 1=IX, 2=BC, 4=B, 8=C */
	unsigned char no_arg_regs = 0;

	/* If any funarg has address taken, no funargs can use registers */
	for (n = locals; n; n = n->next)
		if (n->kind == kfunarg && n->addr_taken)
			no_arg_regs = 1;

	/* Allocate register-marked variables first */
	for (n = locals; n; n = n->next) {
		if (!(n->sclass & SC_REGISTER) || !canAlloc(n, no_arg_regs))
			continue;
		/* an aggregate is its storage, whatever it was asked for */
		if (n->type->flags & (TF_ARRAY | TF_AGGREGATE))
			continue;
		if ((n->type->flags & TF_POINTER) && !(regs & 1)) {
			n->reg = REG_IX;
			regs |= 1;
		} else if (n->type->size == 2 && !(regs & 14)) {
			/* the pair is free only if neither half is taken */
			n->reg = REG_BC;
			regs |= 2;
		} else if (n->type->size == 1 && !(regs & 2)) {
			if (!(regs & 4)) { n->reg = REG_B; regs |= 4; }
			else if (!(regs & 8)) { n->reg = REG_C; regs |= 8; }
		}
	}

	/* IX to pointer with highest agg_refs (only if used for field access) */
	if (!(regs & 1)) {
		best = NULL;
		for (n = locals; n; n = n->next) {
			if (!canAlloc(n, no_arg_regs))
				continue;
			if (!(n->type->flags & TF_POINTER))
				continue;
			/*
			 * An array carries TF_POINTER too - it decays - so the
			 * pointer bit alone does not mean there is a pointer
			 * here to put in a register.  An array IS its storage;
			 * giving it IX made "stack[0].l = 11" assign to the
			 * register and every field reference address the frame
			 * from wherever that left it.  The two allocators below
			 * have always excluded aggregates; this one did not,
			 * and only escaped notice because a declared register
			 * pointer usually takes IX first - which is exactly
			 * what was hiding it in qsort.
			 */
			if (n->type->flags & (TF_ARRAY | TF_AGGREGATE))
				continue;
			/* Only use IX if pointer is used for field access */
			if (n->agg_refs > 0) {
				if (!best || n->agg_refs > best->agg_refs)
					best = n;
			}
		}
		if (best) { best->reg = REG_IX; regs |= 1; }
	}

	/*
	 * BC to word variable with highest ref_count - and word before
	 * byte is not an accident of ordering, it is the measured
	 * answer.  Three attempts to be cleverer all made the compiler
	 * bigger: weighting loop references 8x and 64x by depth cost
	 * 855 bytes (a variable with fifteen sites lost BC to one with
	 * two hot sites, and all fifteen got three bytes longer);
	 * weighting them 2x cost 323; and giving B and C to the two
	 * hottest bytes when their counts beat twice the best word's
	 * cost 264 even with no weighting at all, because a word in BC
	 * saves four bytes a site while a byte in B or C mostly reduces
	 * through A and saves one or two.  This allocator is buying
	 * bytes, not cycles: a reference site costs the same wherever
	 * it sits in the loop structure, so count sites and give the
	 * pair to the word.
	 */
	if (!(regs & 14)) {
		/*
		 * regs & 14, not regs & 2: B or C alone being taken - which
		 * a register-class byte parameter can now cause - leaves no
		 * pair to give.  Checking only the pair bit handed BC to a
		 * word local while B was carrying a parameter, and the
		 * initializer of one overwrote the staging of the other.
		 */
		best = NULL;
		for (n = locals; n; n = n->next) {
			if (!canAlloc(n, no_arg_regs))
				continue;
			if (n->type->flags & (TF_ARRAY | TF_AGGREGATE))
				continue;
			if (n->type->size == 2 && n->ref_count > 1)
				if (!best || n->ref_count > best->ref_count)
					best = n;
		}
		if (best) { best->reg = REG_BC; regs |= 2; }
	}

	/* B and C to byte variables */
	if (!(regs & 2)) {
		for (n = locals; n; n = n->next) {
			if (!canAlloc(n, no_arg_regs))
				continue;
			if (n->type->flags & (TF_ARRAY | TF_AGGREGATE))
				continue;
			if (n->type->size == 1 && n->ref_count > 1) {
				if (!(regs & 4)) { n->reg = REG_B; regs |= 4; }
				else if (!(regs & 8)) { n->reg = REG_C; regs |= 8; }
				if ((regs & 12) == 12) break;
			}
		}
	}
}

/* Entry point: analyze function and allocate registers, returns frame size */
int
analyzeFunc(struct name *func)
{
	struct local *n;

	if (!func)
		return 0;

	/* Locals are already captured (from phase 1) and ref_count is
	 * already populated during phase 1 parseExpr. Just reset reg. */
	for (n = func->u.locals; n; n = n->next) {
		n->reg = REG_NONE;
	}

	/* Allocate registers based on usage (ref_count from phase 1) */
	allocRegs(func->u.locals);

	/* Assign frame offsets to non-register vars, return frame size */
	return assignFrmOff(func);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
