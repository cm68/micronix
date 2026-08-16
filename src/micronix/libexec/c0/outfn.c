/*
 * function- and file-level emitters, split from outast.c so no
 * single translation unit carries them alongside the expression
 * emitter.
 */

#include <string.h>
#include "p1core.h"
#include "p1type.h"
#include "p1name.h"
#include "p1stmt.h"
#include "p1lex.h"
#include "p1expr.h"
#include "p1swcnt.h"
#include "p1outh.h"

extern struct local *curFuncLocals;
extern unsigned short curFuncId;
extern int analyzeFunc(struct name *func);  /* regalloc.c */
extern int frameSaveBase;                   /* regalloc.c */

/* Emit function parameter declarations */
void
emitPrmDecls(struct type *functype, struct local *locals)
{
	struct name *param;
	struct local *local, *found;

	if (!functype || !(functype->flags & TF_FUNC))
		return;
	for (param = functype->elem; param; param = param->next) {
		if (param->type->size == 0)
			continue;
		found = NULL;
		if (param->id)
			for (local = locals; local; local = local->next)
				if (local->id == param->id) {
					found = local;
					break;
				}
		emit1(AST_DECL);
		emit1(typeSfx(param->type));
		emitS(param->id ? nameOf(param->id) : "_");
		emit1(found ? found->reg : 0);
		emit1(found ? (unsigned char)found->frm_off : 0);
	}
}

/* Emit local variable declarations */
void
emitLocals(struct local *locals)
{
	struct local *local;
	char lbuf[32];

	for (local = locals; local; local = local->next) {
		if (local->kind == kfunarg)
			continue;
		if (local->sclass & SC_STATIC)
			staticName(lbuf, local->id, curFuncId,
			    local->static_id);
		else if (local->static_id)
			fmtstr(lbuf, "L%d", local->static_id - 1);
		else
			fmtstr(lbuf, "%s", nameOf(local->id));
		emit1(AST_DECL);
		emit1(typeSfx(local->type));
		emitS(lbuf);
		emit1(local->reg);
		emit2((unsigned short)local->frm_off);
	}
}

/*
 * Output a global asm block - write directly to assembly file
 */
void
emitGlobalAsm(char *text)
{
	/* Phase 1: don't emit */
	if (phase == 1)
		return;

	if (!text)
		return;
	setSeg(SEG_TEXT);
	asmLine(text);
}

/*
 * Output an asm block inside a function body.  Unlike a global asm
 * block this must stay in place relative to the generated code, so it
 * rides in the AST stream and pass2 copies it to the output.
 * Format: ASM len(2) text
 */
void
emitAsmStmt(char *text)
{
	unsigned short len;

	/* Phase 1: don't emit */
	if (phase == 1)
		return;

	len = text ? strlen(text) : 0;
	emit1(ASM);
	emit2(len);
	emitRaw(text, len);
}


/*
 * Can this function do without a frame pointer?
 *
 * Phase 1 has already walked the whole body - that is what fills in
 * ref_count and addr_taken - so by the time phase 2 writes the header
 * every fact below is settled.  That is the only reason a question
 * about the body can be answered before a byte of it is emitted.
 *
 * The answer is deliberately conservative and means: NOTHING in this
 * body reaches through IY except possibly the single parameter that
 * now arrives in HL, and that one is read while HL still holds it.
 *
 *	frm_size	a scalar area is a frame by definition
 *	param_count	the second parameter lives at (iy+6)
 *	addr_taken	an address into the frame outlives any register
 *	ref_count	a parameter read twice cannot live in HL alone
 *	hlArgSym	and read once is not enough either - it has to be
 *			read while HL still holds it
 *
 * That last one is what counting alone kept getting wrong.  stfind
 * and tdfind name their parameter exactly once, inside a loop that
 * rebuilds HL on every pass; pushCount names its own once, an
 * expression after a bounds test has been through HL.  All three were
 * read back off a frame this predicate had already promised was not
 * needed.  Sixteen of the compiler's own functions were that shape.
 */
static int
frameFree(struct name *func, int frm_size, int param_count)
{
	struct local *l;

	if (frm_size != 0 || param_count > 1)
		return 0;
	for (l = func->u.locals; l; l = l->next) {
		if (l->addr_taken)
			return 0;
		if (l->kind != kfunarg)
			continue;
		if (l->ref_count > 1)
			return 0;
		if (l->ref_count == 1 && l->id != hlArgSym)
			return 0;
	}
	return 1;
}

/*
 * Output function header in AST format
 * Format: F rettype name param_count local_count frm_size(2) savebase
 *         framefree
 *         params... locals...
 * savebase = scalar area size; the callee-save slots sit at
 * (iy-savebase-2)/(iy-savebase-4), with arrays below them.
 */
void
emitFuncPre(struct name *func)
{
	char func_name[32];
	int frm_size;
	char param_count, local_count;
	struct name *n;
	struct local *l;

	if (!func)
		return;
#ifdef DEBUG
	if (VERBOSE(V_EMIT))
		fdprintf(2, "EMIT func %s\n", nameOf(func->id));
#endif

	frm_size = analyzeFunc(func);
	curFuncLocals = func->u.locals;
	curFuncId = func->id;

	/* Count params and locals first */
	param_count = local_count = 0;
	if (func->type->flags & TF_FUNC)
		for (n = func->type->elem; n; n = n->next)
			if (n->type->size > 0)
				param_count++;
	for (l = func->u.locals; l; l = l->next)
		if (l->kind != kfunarg)
			local_count++;

	/*
	 * Emit function header.
	 *
	 * The name is the same either way - staticName with no
	 * enclosing function is "_name", which is what the global
	 * spelling is - so the name cannot carry the linkage, and pass2
	 * was reading it for an S that a top-level function never has.
	 * The type letter carries it instead, in its top bit.
	 *
	 * Not in its case: typeSfx spells unsigned by upper-casing the
	 * letter, so marking static that way made every function that
	 * returned an unsigned type static, and five of them left the
	 * symbol table in a build that had been linking.
	 */
	if (func->sclass & SC_STATIC)
		staticName(func_name, func->id, 0, 0);
	else
		fmtstr(func_name, "_%s", nameOf(func->id));
	emit1(AST_FUNC);
	{
		char t = func->type->sub ? typeSfx(func->type->sub) : 'v';

		if (func->sclass & SC_STATIC)
			t |= 0x80;
		emit1(t);
	}
	emitS(func_name);
	emit1(param_count);
	emit1(local_count);
	emit2((unsigned short)frm_size);
	emit1((unsigned char)frameSaveBase);
	emit1((unsigned char)frameFree(func, frm_size, param_count));

	/* Emit declarations */
	emitPrmDecls(func->type, func->u.locals);
	emitLocals(func->u.locals);

	/* Block header */
	emit1(AST_BLOCK);
	emit1(0);
	emit1(popFuncCnt());
}

/*
 * Output an uninitialized global variable declaration
 * Initialized globals are handled by streaming in doInitlzr()
 */
void
emitGv(struct name *var)
{
	char fullname[32];
	int size;

	/* Phase 1: don't emit, just build symbol table */
	if (phase == 1)
		return;

	if (!var)
		return;

	/* Build label: globals get ::, statics get : */
	if (var->sclass & SC_STATIC)
	{
		char *p = staticName(fullname, var->id,
		    var->level > 1 ? curFuncId : 0,
		    var->static_id);
		*p++ = ':';
		*p = 0;
	}
	else
		fmtstr(fullname, "_%s::", nameOf(var->id));

	/*
	 * An array with no extent gets no storage.
	 *
	 *	struct biovec { ... } biosw[];
	 *
	 * is how a header of this vintage declares an array that some
	 * other file defines, and there is nothing here to allocate:
	 * the size is not known and will not be known until the
	 * definition is seen.  It is a reference, so nothing is emitted
	 * and the use of it becomes an external reference in the object
	 * the ordinary way.
	 *
	 * It used to multiply the element size by the count, which for
	 * an incomplete array is -1, and emit ".ds -6".  The count is
	 * unsigned by the time the assembler has it, so that reserved
	 * 65530 bytes of bss without a word said, and every size the
	 * linker computes in that object is measured against it.
	 */
	if ((var->type->flags & TF_ARRAY) && var->type->count <= 0)
		return;

	/* Calculate total size */
	if (var->type->flags & TF_ARRAY)
		size = var->type->count * var->type->sub->size;
	else
		size = var->type->size;

	/* Uninitialized variable - use .bss */
	setSeg(SEG_BSS);
	asmLine(fullname);
	asmDs(size);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
