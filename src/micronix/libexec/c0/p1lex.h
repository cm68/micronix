/*
 * the lexeme stream, and the AST output side
 *
 * Split out of cc1.h, which every source included for everything.
 * That cost more than tidiness: cpp builds a table for every name it
 * sees, and the whole of cc1.h left it room for forty-six more
 * declarations - not enough for any real source, which is why pass1
 * could not be compiled on CP/M at all.  A file that wants types and
 * not statements now says so and pays for what it uses.
 *
 * cc1.h includes all of these, so anything not yet narrowed still
 * gets what it always did.
 */
#ifndef _P1LEX_H
#define _P1LEX_H

#include "p1base.h"

/* lexread.c - lexeme stream reader */
struct token {
	token_t type;
	union {
		long numeric;
		cstring str;
		unsigned short id;	/* SYM/LABEL: the interned identifier */
		unsigned char b[4];	/* numeric, a byte at a time - see readLE4 */
	} v;
};

/*
 * Identifiers are 2-byte ids everywhere inside this pass; the
 * spelling never enters this address space at all.  nameOf() is
 * for output alone - the .1/.2 streams and diagnostics - and
 * prints the @id form for c1 and the driver to undress.  Ids from
 * cpp are 1-based; 0 means "no name" (anonymous params, the basic
 * types); SYNTH and up are pass1's own string literals.
 */
#define SYNTH 0x8000		/* + n: the strn literal */
extern char *nameOf(unsigned short id);
extern char *staticName(char *buf, unsigned short id, unsigned short fid,
    unsigned char sid);

extern struct token cur, next;
extern char strbuf[];
extern char match(token_t t);
extern void gettoken(void);
extern void lexOpen(char *filename);
extern void lexClose(void);
extern void lexRewind(void);
long lexTell(void);
void lexSeek(long off);

/* Error reporting */
extern int lineno;
extern char *filename;
extern int exitCode;

/* cc1.c */
extern void gripe(error_t errcode);
extern void fatal(error_t errcode);
extern char *galloc(unsigned int size);
extern char *permalloc(unsigned int n);
extern void recover(error_t errcode, token_t skipto);
extern void need(token_t check, token_t skipto, error_t errcode);
extern void expect(token_t check, error_t errcode);
int main(int argc, char **argv);
void process(char *f, char *o1, char *o2);
void usage(char *complaint);

/* util.c */
#ifndef CCC
char *bitdef(unsigned char v, char **defs);
#endif
char *fmtstr(char *buf, char *fmt, ...);
#ifdef DEBUG
int fdprintf(unsigned char fd, char *fmt, ...);
#endif
void emit1(unsigned char b);
void emit2(unsigned short w);
void emit4(unsigned long l);
void emitN(char *s, unsigned char len);
void emitS(char *s);
void emitRaw(char *s, unsigned short len);

/* Assembly output helpers (write to asmFd) */
void asmStr(char *s);
void asmLine(char *s);
void asmLabel(char *name);
void asmDb(int val);
void asmDbStr(unsigned char *data, int len);
void asmDw(int val);
void asmDwSym(char *name);
void asmDs(int size);

/* Segment tracking */
#define SEG_TEXT 0
#define SEG_DATA 1
#define SEG_BSS  2
void setSeg(unsigned char seg);

/* debug options */
#ifdef DEBUG
#define VERBOSE(x) (verbose & (x))
extern short verbose;
#else
#define VERBOSE(x) (0)
#endif

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */

#endif
