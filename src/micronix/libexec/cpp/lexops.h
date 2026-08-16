/*
 * lexops.h - the operator and AST token codes
 *
 * Split out of lexeme.h, which is the whole token vocabulary: 129
 * codes, and cpp holds every one it lexes.  pass2's rule table names
 * thirty-three of them and pays for all 129 - about eleven bytes
 * apiece, which on rules.c is the difference between fitting a CP/M 3
 * TPA and not.
 *
 * These are the codes a table of code-generation patterns talks
 * about: what the operators are and what an AST node can be.  The
 * delimiters, the keywords and the line-tracking codes belong to
 * whoever parses text, and a rule table does not.
 *
 * COMMA and SYM sit here rather than with their own groups because
 * the rule tables name them and nothing else in this file is worth
 * dragging along for two codes.  lexeme.h includes this, so the
 * values are still written down once and cannot drift - and the .x
 * format, which is these numbers, is unchanged.
 */
#ifndef LEXOPS_H
#define LEXOPS_H

#define COMMA   9
#define SYM     20
/* Unary/Binary operators (30-54) */
#define INCR    30
#define DECR    31
#define BANG    34
#define AMPER   35
#define STAR    36
#define TWIDDLE 38
#define DOT     39
#define PLUS    40
#define MINUS   41
#define TIMES   42
#define DIV     43
#define MOD     44
#define RSHIFT  45
#define LSHIFT  46
#define AND     47
#define OR      48
#define XOR     49
#define ARROW   50
#define LAND    53
#define LOR     54

/* Relational (60-65) */
#define EQ      60
#define NEQ     61
#define LE      62
#define LT      63
#define GE      64
#define GT      65

/* Assignment operators (70-80) */
#define PLUSEQ  70
#define SUBEQ   71
#define MULTEQ  72
#define DIVEQ   73
#define MODEQ   74
#define RSHIFTEQ 75
#define LSHIFTEQ 76
#define ANDEQ   77
#define OREQ    78
#define XOREQ   79
#define ASSIGN  80

/* Internal tokens - not from lexer, used in AST (200+) */
#define TOK_NONE    200
#define DEREF       201
#define NEG         203
#define NOT         204
#define CALL        205
#define NARROW      206
#define WIDEN       207
#define SEXT        208
#define INITLIST    209
#define PREINC      210
#define POSTINC     211
#define PREDEC      212
#define POSTDEC     213
#define BFEXTRACT   214
#define BFASSIGN    215
#define REGVAR      216   /* pass2: register variable */
#define LOCALVAR    217   /* pass2: local variable (IY/IX indexed) */
#define ARGNODE     218   /* pass2: function call argument wrapper */
#define TERNBRANCH  219   /* pass2: ternary then/else container */
#define URSHIFT     220   /* unsigned right shift >>> */

/*
 * AST structure markers - only for concepts without existing lexemes.
 * Values chosen to avoid collision with operator tokens (60-80).
 */
#define AST_FUNC    221     /* function header */
#define AST_BLOCK   222     /* block/compound statement */
#define AST_GLOBAL  223     /* global variable */
#define AST_DECL    224     /* variable/parameter declaration */
#define AST_EMPTY   225     /* null/empty expression */

/* 226+ reserved for pass2 synthetic opcodes (see pass2/opcodes.h) */

#endif
