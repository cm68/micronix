/*
 * pblock.c - blocks and the locals in them
 *
 * What pblock.c exports.  A caller that only passes a local around -
 * decl.c hands one to freeLocals and never looks inside - takes this
 * and leaves the structures in p1stmt.h to the files that read them.
 */
#ifndef _P1PBLK_H
#define _P1PBLK_H

#include "p1base.h"
struct local;
struct name;

void parseBlockEx(int emitHdr);
void parseBlock(void);
struct local *mklocal(struct name *n);
int notaslot(struct name *n);
struct local *capLocals(void);
char *getAsmText(void);
void freeLocals(struct local *local);

#endif
