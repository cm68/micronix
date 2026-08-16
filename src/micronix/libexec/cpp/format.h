/*
 * format.h - shared formatting functions for AST display
 *
 * Used by astpp.c and pass2 for consistent expression output.
 */
#ifndef FORMAT_H
#define FORMAT_H

#include "lexeme.h"

static char *
widthName(int c)
{
	switch (c) {
	case 'b': return "byte";
	case 'B': return "ubyte";
	case 's': return "short";
	case 'S': return "ushort";
	case 'l': return "long";
	case 'L': return "ulong";
	case 'p': return "ptr";
	case 'f': return "float";
	case 'd': return "double";
	case 'v': return "void";
	default: { static char buf[2]; buf[0] = c; buf[1] = 0; return buf; }
	}
}

static char *
regName(int r)
{
	switch (r) {
	case 0: return "-";
	case 1: return "B";
	case 2: return "C";
	case 3: return "BC";
	case 4: return "IX";
	case 5: return "DE";
	case 6: return "HL";
	case 7: return "A";
	case 8: return "IY";
	case 16: return "Z";
	case 17: return "NZ";
	case 18: return "C";
	case 19: return "NC";
	case 20: return "M";
	case 21: return "P";
	default: return "?";
	}
}

static char *
opName(int c)
{
	switch (c) {
	/* Internal tokens (200+) */
	case NARROW: return "NARROW";
	case WIDEN: return "WIDEN";
	case SEXT: return "SEXT";
	case DEREF: return "DEREF";
	case NEG: return "NEG";
	/*
	 * The rest of what the stream can carry.  An unnamed opcode
	 * printed as "???" told the reader nothing; these are the ones
	 * that turned up while walking real ASTs.
	 */
	case NOT: return "NOT";
	case INCR: return "INCR";
	case DECR: return "DECR";
	case AMPER: return "ADDR";
	case DOT: return "DOT";
	case ARROW: return "ARROW";
	case INITLIST: return "INITLIST";
	case ARGNODE: return "ARG";
	case TERNBRANCH: return "TERNBR";
	case AST_FUNC: return "FUNC";
	case AST_BLOCK: return "BLOCK";
	case AST_GLOBAL: return "GLOBAL";
	case AST_DECL: return "DECL";
	case AST_EMPTY: return "EMPTY";
	case TOK_NONE: return "NONE";
	case CALL: return "CALL";
	case PREINC: return "PREINC";
	case POSTINC: return "POSTINC";
	case PREDEC: return "PREDEC";
	case POSTDEC: return "POSTDEC";
	case BFEXTRACT: return "BFEXT";
	case BFASSIGN: return "BFSET";
	case REGVAR: return "REGVAR";
	case LOCALVAR: return "LOCALVAR";
	case URSHIFT: return "URSHIFT";
#ifdef INDEX
	case INDEX: return "INDEX";
#endif

	/* Lexeme tokens */
	case TWIDDLE: return "NOT";
	case BANG: return "LNOT";
	case PLUS: return "ADD";
	case MINUS: return "SUB";
	case STAR: return "MUL";
	case DIV: return "DIV";
	case MOD: return "MOD";
	case AND: return "AND";
	case OR: return "OR";
	case XOR: return "XOR";
	case LSHIFT: return "LSHIFT";
	case RSHIFT: return "RSHIFT";
	case EQ: return "EQ";
	case NEQ: return "NE";
	case LT: return "LT";
	case GT: return "GT";
	case LE: return "LE";
	case GE: return "GE";
	case LAND: return "LAND";
	case LOR: return "LOR";
	case ASSIGN: return "ASSIGN";
	case PLUSEQ: return "ADDEQ";
	case SUBEQ: return "SUBEQ";
	case MULTEQ: return "MULEQ";
	case DIVEQ: return "DIVEQ";
	case MODEQ: return "MODEQ";
	case ANDEQ: return "ANDEQ";
	case OREQ: return "OREQ";
	case XOREQ: return "XOREQ";
	case LSHIFTEQ: return "SHLEQ";
	case RSHIFTEQ: return "SHREQ";
	case QUES: return "TERN";
	case COMMA: return "COMMA";
	case NUMBER: return "CONST";
	case SYM: return "SYM";
	default: return "???";
	}
}

/*
 * The attribute is gcc's and is here to stop it warning about a
 * static that a given includer does not call.  It has to be guarded:
 * ccc does not parse it, and until astpp moved into the micronix tree
 * nothing had ever asked it to - pass2 is the only other includer of
 * this file and its #include sits inside #ifdef DEBUG, so c0 had
 * never once seen these lines.  It reported them as "fn array" and
 * "bad decl token", which is what a compiler says about a declaration
 * with a word in it that it has no idea about.
 */
#ifdef __GNUC__
__attribute__((unused))
#endif
static int
opArity(int c)
{
	switch (c) {
	case NARROW: case WIDEN: case SEXT: case DEREF:
	case NEG: case TWIDDLE: case BANG:
	case PREINC: case POSTINC: case PREDEC: case POSTDEC:
		return 1;
	default:
		return 2;
	}
}

#endif /* FORMAT_H */
