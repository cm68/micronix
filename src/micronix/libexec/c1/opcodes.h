/*
 * opcodes.h - pass2 synthetic opcodes
 *
 * These opcodes are created during tree rewriting and don't appear
 * in the AST stream from pass1. Values start at 226 per lexeme.h.
 */
#ifndef OPCODES_H
#define OPCODES_H

#define INDEX       226     /* register+offset addressing (ix+d), (iy+d) */
#define INHL        227     /* value is in HL */
#define INDE        228     /* value is in DE */
#define INA         229     /* value is in A (accumulator) */
#define INBC        230     /* value is in BC */
#define SYMREF      231     /* symbol+offset (linker-resolvable address) */
#define CODE        232     /* emitted assembly code */
#define INE         233     /* value is in E (low byte of DE) */
#define INL         234     /* value is in L (low byte of HL) */
/* ARGNODE is defined in lexeme.h (218) */

#endif
