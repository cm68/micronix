/*
 * lexeme.h - Token definitions for lexeme stream
 *
 * Shared between cpp (emitter) and pass1 (reader).
 * Binary .x format uses these token values directly.
 */
#ifndef LEXEME_H
#define LEXEME_H

#include "lexops.h"

/* Delimiters (0-9) */
#define E_O_F   0
#define SEMI    1
#define BEGIN   2
#define END     3
#define LBRACK  4
#define RBRACK  5
#define LPAR    6
#define RPAR    7
#define COLON   8

/* Terminals (20-26) */
#define NUMBER  21
#define STRING  22
/*
 * A NUMBER the preprocessor folded from a construct that C types as
 * int - sizeof, today - so pass1 must type it int too, not by
 * magnitude the way a spelled literal is.  Same 4-byte record.
 */
#define INUMBER 23
#define LNUMBER 25
/*
 * An interned identifier: 26 + 2-byte little-endian id.  Always
 * emitted in place of SYM (and of LABEL's name); the id-to-name
 * table travels beside the .x in a .n sidecar - cpp/OUTPUT.md §4.2
 * has the format.  c0 never needs the names; c1 reads the sidecar
 * to spell symbols in assembly, and the driver uses it to translate
 * @{id} markers in the passes' stderr.
 */
#define SYMID   26
#define LABELID 27	/* LABEL's id form: 27 + 2-byte id */

/* Special (90-92) */
#define QUES    90
#define SIZEOF  91
#define ELLIPSIS 92

/* Line tracking (112-118) */
#define LABEL   112
#define LINENO  116
#define NEWLINE 117
#define ASMSTR  118

/* Keyword tokens (128-160) */
#define INT         128
#define CHAR        129
/*
 * 130 and 131 held FLOAT and DOUBLE (23, once FNUMBER, is INUMBER
 * now).  ccc has no floating
 * point, and float and double are deliberately not reserved words so a
 * program can typedef them.  The numbers stay vacant rather than being
 * reused: token_props is a positional table and .x streams carry these
 * codes, so renumbering would silently reinterpret both.
 */
#define STRUCT      132
#define SIGNED      133
#define LONG        134
#define UNSIGNED    135
#define UNION       136
#define SHORT       137
#define VOID        138
#define ENUM        139

#define	SCLASS_MIN	TYPEDEF
#define TYPEDEF     140
#define AUTO        141
#define EXTERN      142
#define STATIC      143
#define REGISTER    144
#define	SCLASS_MAX	REGISTER

#define GOTO        145
#define RETURN      146
#define IF          147
#define WHILE       148
#define ELSE        149
#define SWITCH      150
#define CASE        151
#define BREAK       152
#define CONTINUE    153
#define DO          154
#define DEFAULT     155
#define FOR         156
#define ASM         157
#define CONST       158
#define VOLATILE    159
#define SIZEOF_KW   160

/* Keyword range for is-keyword check */
#define KW_FIRST    128
#define KW_LAST     160


/*
 * Token properties bitmask for fast classification
 */
#define TF_TYPE     0x01    /* type keyword (INT, CHAR, etc) */
#define TF_COND     0x02    /* conditional control (IF, WHILE, FOR, SWITCH) */
#define TF_DO       0x04    /* DO keyword */
#define TF_ELSE     0x08    /* ELSE keyword */
#define TF_TERM     0x10    /* statement terminator (SEMI, END) */
#define TF_OPEN     0x20    /* opening delimiter (LPAR, LBRACK, BEGIN) */
#define TF_CLOSE    0x40    /* closing delimiter (RPAR, RBRACK, END) */

extern unsigned char token_props[];

#endif /* LEXEME_H */

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
