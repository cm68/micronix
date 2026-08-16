/*
 * outh.c - the AST writer's helpers
 *
 * What outh.c exports: the label and goto emitters, the width and
 * demotion questions pass2 needs answered, and the locals lookup.
 *
 * One source's contract in one header, so a caller takes this and not
 * everything else that used to share p1stmt.h with it.
 */
#ifndef _P1OUTH_H
#define _P1OUTH_H

#include "p1base.h"
struct expr;
struct type;
struct name;
struct local;

struct local *findInLocals(struct name *want);
int isAssignOp(unsigned char op);
char dchainreg(struct expr *e);
int truncok(unsigned char op);
int bytevalued(struct expr *e);
int candemote(struct expr *e, int size);
struct expr *demote(struct expr *e, struct type *t);
int iscmpop(unsigned char op);
unsigned char valwidth(struct type *t);
struct type *opwidth(struct expr *e);
char typeSfx(struct type *t);
char *mkLbl(char *base, char *suffix);
void emitLabel(char *base, char *suffix);
void emitGoto(char *base, char *suffix);
int cntCondLbls(struct expr *e);

char localReg(struct local *l);
short localOff(struct local *l);

#endif
