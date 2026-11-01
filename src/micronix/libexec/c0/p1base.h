/*
 * the types and limits every part of pass1 shares
 *
 * The smallest thing a pass1 source can include.  It has no system
 * headers behind it and declares nothing, so it costs cpp almost
 * nothing - which is the point of the split; see p1expr.h.
 */
#ifndef _P1BASE_H
#define _P1BASE_H

/*
 * basic types - everything externally visible has one of these
 */
typedef char *cstring;		// counted string - first char is length
typedef unsigned char byte;
typedef unsigned short word;
typedef unsigned long dword;

/*
 * Compiler limits - adjust these for larger files
 */
#define MAXPARMS 10            // macro parameters
#define TBSIZE 1024            // text buffer size for includes/macros
#define STRBUFSIZE 128         // string/symbol/identifier buffer
#define MAXSYMLEN 32           // maximum symbol/identifier length
#define PSIZE 80               // max string containing bitdefs
/*
 * The info byte bfins/bfext take (lib/libc/bitfield.s) is four bits of
 * width and four of offset.  This was 32, and a field of 16..32 bits
 * was accepted and encoded as (offset<<4)|width with the width spilling
 * into the offset nibble: width 16 became offset 1 / width 0 (a field
 * that stores nothing), 17 became a one-bit field one bit along, and 32
 * another dead one - silently.  Fifteen is what the encoding holds.
 */
#define MAXBITS 15             // maximum size of bitfield
#define MAX_COUNTS 256         // pre-computed counts for streaming AST
#define MAXDIM 6               // dimensions in one array declarator

#endif
