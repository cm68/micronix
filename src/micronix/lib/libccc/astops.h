/*
 * astops.h - the arity of an AST operator record
 *
 * Split out of format.h so that pass2 and astpp can both include it
 * unconditionally.  format.h is a bundle of display helpers that
 * pass2 only wants under DEBUG, and a shared table that only one
 * side compiles is not shared at all - it is a copy waiting to
 * disagree.
 */
#ifndef ASTOPS_H
#define ASTOPS_H

#include "lexeme.h"

/*
 * How many children an operator record carries in the AST stream.
 *
 * This is the one thing a reader of that stream cannot get wrong and
 * still recover: the arity decides how many subtrees to consume, so a
 * disagreement here does not misprint one node, it desynchronises
 * everything after it.  astpp used to keep its own list and default
 * the rest to two, which read a unary node's sibling as its own
 * operand and turned the remainder of the function into noise.
 *
 * pass1 writes the record, pass2 reads it, astpp prints it.  One
 * table, so they cannot drift.
 */
static int
astBinary(int c)
{
	switch (c) {
	case PLUS: case MINUS: case TIMES: case STAR:
	case DIV: case MOD: case AND: case OR: case XOR:
	case LSHIFT: case RSHIFT: case URSHIFT:
	case EQ: case NEQ: case LT: case LE: case GT: case GE:
	case LAND: case LOR: case ASSIGN:
	case PLUSEQ: case SUBEQ: case MULTEQ: case DIVEQ: case MODEQ:
	case ANDEQ: case OREQ: case XOREQ: case LSHIFTEQ: case RSHIFTEQ:
	case COMMA:
		return 1;
	}
	return 0;
}


#endif
