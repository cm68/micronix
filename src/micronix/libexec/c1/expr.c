/*
 * expr.c - expression tree builder and dumper
 */
#include "pass2.h"
#include "lexeme.h"
#include "expr.h"
#include "astops.h"
#include "opcodes.h"
#include <stdlib.h>

extern void errout(char *);
#include <string.h>
#include <stdio.h>

#ifdef DEBUG
#include "debug.h"
#endif

Expr *
alloc(void)
{
	Expr *e = malloc(sizeof(Expr));
	if (!e) {
		out("!OOM\n");
		exit(1);
	}
	e->op = 0;
	e->width = 0;
	e->dest = DEST_NONE;
	e->regs = 0;
	e->tgt = 0;
	/*
	 * Every field, and this one is why.  Nodes are freed with free()
	 * and come back from malloc holding whatever the last occupant
	 * left, so a node that had once been the right operand of a byte
	 * comparison passed its "nored" on to whatever was allocated at
	 * that address next.  Inherited by a child, it means the child is
	 * never reduced: the shape that reaches the rules is the one that
	 * was read in, and nothing matches it.
	 *
	 * Which nodes that happens to depends on the whole allocation
	 * history of the file, so the same function compiled clean on its
	 * own and left a marker in the file it came from.
	 */
	e->nored = 0;
	e->left = NULL;
	e->right = NULL;
	e->u.val = 0;
	return e;
}

Expr *
mkconst(char width, long val)
{
	Expr *e = alloc();
	e->op = NUMBER;
	e->width = width;
	e->u.val = val;
	return e;
}

Expr *
mksym(char *name)
{
	Expr *e = alloc();
	e->op = SYM;
	e->width = 'p';
	e->u.name = strdup(name);
	return e;
}

Expr *
mklocalvar(char width, char reg, int off)
{
	Expr *e = alloc();
	e->op = LOCALVAR;
	e->width = width;
	e->u.var.reg = reg;
	e->u.var.off = off;
	return e;
}

Expr *
mkregvar(char width, char reg)
{
	Expr *e = alloc();
	e->op = REGVAR;
	e->width = width;
	e->u.var.reg = reg;
	return e;
}

Expr *
mkindex(char width, char reg, int off)
{
	Expr *e = alloc();
	e->op = INDEX;
	e->width = width;
	e->u.var.reg = reg;
	e->u.var.off = off;
	return e;
}




Expr *
mkunary(int op, char width, Expr *child)
{
	Expr *e = alloc();
	e->op = op;
	e->width = width;
	e->left = child;
	return e;
}

Expr *
mkbinary(int op, char width, Expr *left, Expr *right)
{
	Expr *e = alloc();
	e->op = op;
	e->width = width;
	e->left = left;
	e->right = right;
#ifdef DEBUG
	/*
	 * A binary node has two children, and the rest of this pass is
	 * written as though that were true: it reads e->left->op with
	 * no guard in one line and guards it in the next.  Measured
	 * over a self-compile and the whole runtime suite, thirty-two
	 * of the thirty-eight guards never saw a null - so the guards
	 * came out and the invariant is stated here instead, where it
	 * is made.  Loud in the build that can afford to be, free in
	 * the one that has to fit.
	 */
	if (!left || !right)
		{ errout("mkbinary: binary node missing a child\n"); exit(1); }
#endif
	return e;
}

Expr *
mkcall(char width, int argc, Expr *func, Expr *args)
{
	Expr *e = alloc();
	e->op = CALL;
	e->width = width;
	e->u.call.argc = argc;
	e->left = func;
	e->right = args;
	return e;
}

Expr *
mkarg(Expr *expr)
{
	Expr *e = alloc();
	e->op = ARGNODE;
	e->width = expr ? expr->width : 'v';
	e->left = expr;
	if (expr)
		expr->dest = DEST_STACK;
	return e;
}

Expr *
mkincdec(int op, char width, Expr *child, int amt)
{
	Expr *e = alloc();
	e->op = op;
	e->width = width;
	e->left = child;
	e->u.incdec.amt = amt;
	return e;
}

Expr *
mkbfext(char off, char wid, Expr *addr)
{
	Expr *e = alloc();
	e->op = BFEXTRACT;
	e->width = 's';
	e->u.bf.off = off;
	e->u.bf.wid = wid;
	e->left = addr;
	return e;
}

Expr *
mkbfass(char off, char wid, Expr *addr, Expr *val)
{
	Expr *e = alloc();
	e->op = BFASSIGN;
	e->width = 's';
	e->u.bf.off = off;
	e->u.bf.wid = wid;
	e->left = addr;
	e->right = val;
	return e;
}

Expr *
mksymref(char *name, short off)
{
	Expr *e = alloc();
	e->op = SYMREF;
	e->width = 'p';
	e->u.symref.name = strdup(name);
	e->u.symref.off = off;
	return e;
}

Expr *
mkcode(char width, char reg)
{
	Expr *e = alloc();
	e->op = CODE;
	e->width = width;
	e->u.var.reg = reg;
	return e;
}

void
setdest(Expr *e, char dest)
{
	if (!e) return;
	e->dest = dest;

	/* Propagate flag context to logical/comparison ops */
	if (dest == DEST_FLAGS) {
		switch (e->op) {
		case LAND: case LOR:
			setdest(e->left, DEST_FLAGS);
			setdest(e->right, DEST_FLAGS);
			break;
		case BANG:
			setdest(e->left, DEST_FLAGS);
			break;
		}
	}
}

Expr *
dupexpr(Expr *e)
{
	Expr *n;
	if (!e)
		return NULL;
	n = malloc(sizeof(Expr));
	if (!n) {
		out("!OOM\n");
		exit(1);
	}
	memcpy(n, e, sizeof(Expr));
	n->left = dupexpr(e->left);
	n->right = dupexpr(e->right);
	if (e->op == SYM || e->op == SYMREF)
		n->u.name = strdup(e->u.name);
	return n;
}

void
freeexpr(Expr *e)
{
	if (!e)
		return;
	freeexpr(e->left);
	freeexpr(e->right);
	if (e->op == SYM || e->op == SYMREF)
		free(e->u.name);
	free(e);
}

/* check if op is binary */
int
isbinary(int op)
{
	/*
	 * The table lives in astops.h so that astpp reads the stream
	 * with the same one.  A reader that disagrees about arity does
	 * not misprint a node, it loses sync and turns everything after
	 * it into noise - which is what astpp did for years.
	 */
	return astBinary(op);
}

Expr *
readexpr(void)
{
	static char buf[64];
	unsigned char op, t, n, i;
	unsigned long v;
	Expr *e, *args, *arg;

	op = read1();

#ifdef DEBUG
	if (VERBOSE(V_EXPR))
		fprintf(stderr, "expr: op=%d '%c'\n", op, op);
#endif

	switch (op) {
	case AST_EMPTY:
#ifdef DEBUG
		if (VERBOSE(V_EXPR))
			fprintf(stderr, "  NULL expr\n");
#endif
		return NULL;

	case NUMBER:
		t = read1();
		read4();
#ifdef DEBUG
		if (VERBOSE(V_EXPR))
			fprintf(stderr, "  CONST type=%c val=%lu\n", t, val4);
#endif
		return mkconst(t, (long)val4);

	case SYM:
		readS(buf, sizeof(buf));
#ifdef DEBUG
		if (VERBOSE(V_EXPR))
			fprintf(stderr, "  SYM %s\n", buf);
#endif
		return mksym(buf);

	case LOCALVAR: {
		int off;
		t = read1();
		off = (short)read2();
#ifdef DEBUG
		if (VERBOSE(V_EXPR))
			fprintf(stderr, "  LOCALVAR type=%c off=%d\n", t, off);
#endif
		return mklocalvar(t, 0, off);
	}

	case REGVAR:
		t = read1();
		n = read1();
#ifdef DEBUG
		if (VERBOSE(V_EXPR))
			fprintf(stderr, "  REGVAR type=%c reg=%d\n", t, n);
#endif
		return mkregvar(t, n);

	case CALL:
		t = read1();
		n = read1();
#ifdef DEBUG
		if (VERBOSE(V_EXPR))
			fprintf(stderr, "  CALL type=%c argc=%d\n", t, n);
#endif
		e = readexpr();
		/* chain args in reverse order for C calling convention */
		args = NULL;
		for (i = 0; i < n; i++) {
			arg = mkarg(readexpr());
			arg->right = args;  /* prepend to chain */
			args = arg;
		}
		return mkcall(t, n, e, args);

	case PREINC:
	case POSTINC:
	case PREDEC:
	case POSTDEC:
		t = read1();
		e = readexpr();
		v = read2();
#ifdef DEBUG
		if (VERBOSE(V_EXPR))
			fprintf(stderr, "  INCDEC op=%d type=%c amt=%lu\n", op, t, v);
#endif
		return mkincdec(op, t, e, v);

	case BFEXTRACT:
		t = read1();
		n = read1();
#ifdef DEBUG
		if (VERBOSE(V_EXPR))
			fprintf(stderr, "  BFEXTRACT off=%d wid=%d\n", t, n);
#endif
		e = readexpr();
		return mkbfext(t, n, e);

	case BFASSIGN:
		t = read1();
		n = read1();
#ifdef DEBUG
		if (VERBOSE(V_EXPR))
			fprintf(stderr, "  BFASSIGN off=%d wid=%d\n", t, n);
#endif
		e = readexpr();
		return mkbfass(t, n, e, readexpr());

	case QUES:
		t = read1();
#ifdef DEBUG
		if (VERBOSE(V_EXPR))
			fprintf(stderr, "  TERNARY type=%c\n", t);
#endif
		e = alloc();
		e->op = QUES;
		e->width = t;
		e->left = readexpr();
		{
			/*
			 * One read per statement.  These are two arguments to
			 * one call, and C does not say which order arguments
			 * are evaluated in - this compiler is built by one that
			 * takes them right to left, so the else arm was read
			 * out of the stream first and the two arms came back
			 * swapped.  Every ternary took the wrong branch.
			 */
			Expr *then, *els;

			then = readexpr();
			els = readexpr();
			e->right = mkbinary(TERNBRANCH, t, then, els);
		}
		return e;

	case BEGIN:
		n = read1();
#ifdef DEBUG
		if (VERBOSE(V_EXPR))
			fprintf(stderr, "  INITLIST (BEGIN) n=%d\n", n);
#endif
		e = alloc();
		e->op = INITLIST;
		e->width = 's';
		args = NULL;
		for (i = 0; i < n; i++) {
			arg = readexpr();
			if (!args)
				args = arg;
			else
				args = mkbinary(COMMA, 's', args, arg);
		}
		e->left = args;
		read1();
		return e;

	case LBRACK:
		t = read1();
		n = read1();
#ifdef DEBUG
		if (VERBOSE(V_EXPR))
			fprintf(stderr, "  INITLIST (LBRACK) type=%c n=%d\n", t, n);
#endif
		e = alloc();
		e->op = INITLIST;
		e->width = t;
		args = NULL;
		for (i = 0; i < n; i++) {
			arg = readexpr();
			if (!args)
				args = arg;
			else
				args = mkbinary(COMMA, t, args, arg);
		}
		e->left = args;
		read1();
		return e;

	default:
		t = read1();
#ifdef DEBUG
		if (VERBOSE(V_EXPR))
			fprintf(stderr, "  OP %d type=%c binary=%d\n", op, t, isbinary(op));
#endif
		e = readexpr();
		if (isbinary(op))
			return mkbinary(op, t, e, readexpr());
		return mkunary(op, t, e);
	}
}

#ifdef DEBUG

#include "format.h"

char *
destName(int d)
{
	switch (d) {
	case DEST_FLAGS: return "/f";
	case DEST_VALUE: return "/v";
	case DEST_STACK: return "/s";
	default: return "";
	}
}

void
dumpnode(Expr *e)
{
	char buf[64];
	Expr *n;

	if (!e) {
		out("_");
		return;
	}

	switch (e->op) {
	case NUMBER:
		sprintf(buf, "%ld:%s%s", e->u.val, widthName(e->width),
		        destName(e->dest));
		out(buf);
		if (e->regs) { sprintf(buf, "#%d", e->regs); out(buf); }
		if (e->tgt) { sprintf(buf, "->%s", regName(e->tgt)); out(buf); }
		return;
	case SYM:
		out("$");
		out(e->u.name);
		return;
	case LOCALVAR:
		sprintf(buf, "(LOCALVAR:%s%s %s%+d", widthName(e->width),
		        destName(e->dest),
		        regName(e->u.var.reg ? e->u.var.reg : R_IY),
		        (int)e->u.var.off);
		out(buf);
		if (e->regs) { sprintf(buf, "#%d", e->regs); out(buf); }
		if (e->tgt) { sprintf(buf, "->%s", regName(e->tgt)); out(buf); }
		out(")");
		return;
	case REGVAR:
		sprintf(buf, "(REGVAR:%s%s %s", widthName(e->width),
		        destName(e->dest), regName(e->u.var.reg));
		out(buf);
		if (e->regs) { sprintf(buf, "#%d", e->regs); out(buf); }
		if (e->tgt) { sprintf(buf, "->%s", regName(e->tgt)); out(buf); }
		out(")");
		return;
	case INDEX:
		sprintf(buf, "(INDEX:%s%s %s%+d", widthName(e->width),
		        destName(e->dest), regName(e->u.var.reg),
		        (int)e->u.var.off);
		out(buf);
		if (e->regs) { sprintf(buf, "#%d", e->regs); out(buf); }
		if (e->tgt) { sprintf(buf, "->%s", regName(e->tgt)); out(buf); }
		out(")");
		return;
	case CALL:
		sprintf(buf, "(CALL:%s%s/%d", widthName(e->width),
		        destName(e->dest), e->u.call.argc);
		out(buf);
		if (e->regs) { sprintf(buf, "#%d", e->regs); out(buf); }
		if (e->tgt) { sprintf(buf, "->%s", regName(e->tgt)); out(buf); }
		out(" ");
		dumpnode(e->left);
		/* args are wrapped in ARGNODE, linked via right */
		for (n = e->right; n && n->op == ARGNODE; n = n->right) {
			out(" ");
			dumpnode(n->left);
		}
		out(")");
		return;
	case PREINC:
	case POSTINC:
	case PREDEC:
	case POSTDEC:
		sprintf(buf, "(%s:%s%s/%d", opName(e->op), widthName(e->width),
		        destName(e->dest), e->u.incdec.amt);
		out(buf);
		if (e->regs) { sprintf(buf, "#%d", e->regs); out(buf); }
		if (e->tgt) { sprintf(buf, "->%s", regName(e->tgt)); out(buf); }
		out(" ");
		dumpnode(e->left);
		out(")");
		return;
	case BFEXTRACT:
		sprintf(buf, "(BFEXT %d:%d ", e->u.bf.off, e->u.bf.wid);
		out(buf);
		dumpnode(e->left);
		out(")");
		return;
	case BFASSIGN:
		sprintf(buf, "(BFSET %d:%d ", e->u.bf.off, e->u.bf.wid);
		out(buf);
		dumpnode(e->left);
		out(" ");
		dumpnode(e->right);
		out(")");
		return;
	case SYMREF:
		sprintf(buf, "(SYMREF%s%s %s%+d", widthName(e->width),
		        destName(e->dest), e->u.symref.name, e->u.symref.off);
		out(buf);
		if (e->regs) { sprintf(buf, "#%d", e->regs); out(buf); }
		if (e->tgt) { sprintf(buf, "->%s", regName(e->tgt)); out(buf); }
		out(")");
		return;
	case CODE:
		sprintf(buf, "(CODE:%s%s @%s", widthName(e->width),
		        destName(e->dest), regName(e->u.var.reg));
		out(buf);
		if (e->regs) { sprintf(buf, "#%d", e->regs); out(buf); }
		if (e->tgt) { sprintf(buf, "->%s", regName(e->tgt)); out(buf); }
		out(")");
		return;
	case INHL:
		sprintf(buf, "(HL:%s%s", widthName(e->width), destName(e->dest));
		out(buf);
		if (e->regs) { sprintf(buf, "#%d", e->regs); out(buf); }
		if (e->tgt) { sprintf(buf, "->%s", regName(e->tgt)); out(buf); }
		out(")");
		return;
	case INDE:
		sprintf(buf, "(DE:%s%s", widthName(e->width), destName(e->dest));
		out(buf);
		if (e->regs) { sprintf(buf, "#%d", e->regs); out(buf); }
		if (e->tgt) { sprintf(buf, "->%s", regName(e->tgt)); out(buf); }
		out(")");
		return;
	case INBC:
		sprintf(buf, "(BC:%s%s", widthName(e->width), destName(e->dest));
		out(buf);
		if (e->regs) { sprintf(buf, "#%d", e->regs); out(buf); }
		if (e->tgt) { sprintf(buf, "->%s", regName(e->tgt)); out(buf); }
		out(")");
		return;
	case INA:
		sprintf(buf, "(A:%s%s", widthName(e->width), destName(e->dest));
		out(buf);
		if (e->regs) { sprintf(buf, "#%d", e->regs); out(buf); }
		if (e->tgt) { sprintf(buf, "->%s", regName(e->tgt)); out(buf); }
		out(")");
		return;
	case INE:
		sprintf(buf, "(E:%s%s", widthName(e->width), destName(e->dest));
		out(buf);
		if (e->regs) { sprintf(buf, "#%d", e->regs); out(buf); }
		if (e->tgt) { sprintf(buf, "->%s", regName(e->tgt)); out(buf); }
		out(")");
		return;
	case QUES:
		sprintf(buf, "(?:%s%s", widthName(e->width), destName(e->dest));
		out(buf);
		if (e->regs) { sprintf(buf, "#%d", e->regs); out(buf); }
		if (e->tgt) { sprintf(buf, "->%s", regName(e->tgt)); out(buf); }
		out(" ");
		dumpnode(e->left);
		out(" ");
		dumpnode(e->right->left);
		out(" ");
		dumpnode(e->right->right);
		out(")");
		return;
	}

	/* regular unary/binary ops */
	out("(");
	out(opName(e->op));
	out(":");
	out(widthName(e->width));
	out(destName(e->dest));
	if (e->regs) {
		sprintf(buf, "#%d", e->regs);
		out(buf);
	}
	if (e->tgt) {
		sprintf(buf, "->%s", regName(e->tgt));
		out(buf);
	}
	out(" ");
	dumpnode(e->left);
	if (e->right) {
		out(" ");
		dumpnode(e->right);
	}
	out(")");
}

void
dumpexpr(Expr *e)
{
	out("; ");
	dumpnode(e);
	out("\n");
}

#endif

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
