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
#define MAXBITS 32             // maximum size of bitfield
#define MAX_COUNTS 256         // pre-computed counts for streaming AST
#define MAXDIM 6               // dimensions in one array declarator

#endif
