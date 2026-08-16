/*
 * cpp.h - C Preprocessor header
 *
 * Common definitions for the C preprocessor that produces
 * lexeme streams (.x) and preprocessed output (.i)
 */

#ifndef CPP_H
#define CPP_H

#include "lexeme.h"

/*
 * The system headers are NOT here.  They cost names, c0 keeps every
 * name it has ever seen for the whole translation unit, and that is
 * what runs it out of memory on the big sources - <stdio.h> alone is
 * 36 names, <string.h> 47, <stdlib.h> 35, against the 217 this header
 * declares itself.
 *
 * Paid for by everyone and used by almost nobody: of the fourteen
 * sources built here, one uses stdio outside DEBUG (cpp.c, for
 * perror and one fclose), eight use string.h, ten use stdlib.h.
 * norm.c - the source that needs the most room - uses no string.h at
 * all.  So each source includes what it uses, and this header only
 * declares cpp's own.
 *
 * DEBUG builds print, so debug.h brings stdio with it.
 */
#ifdef DEBUG
#include <stdio.h>
#include "debug.h"
#endif

typedef unsigned char token_t;

/* CPP-only directives (not emitted to .x) */
#define PP_INCLUDE 240
#define PP_DEFINE 241
#define PP_UNDEF 242
#define PP_IF 243
#define PP_IFDEF 244
#define PP_IFNDEF 245
#define PP_ENDIF 246
#define PP_ELIF 247
#define PP_ELSE 248
#define NONE 255

/*
 * Basic types
 */
typedef char *cstring;      /* counted string - first char is length */
typedef unsigned char byte;
typedef unsigned short word;
typedef unsigned long dword;

/*
 * Limits
 */
#define MAXPARMS 16         /* macro parameters */
/*
 * Text buffer size.  512 matched the Micronix disk block, but with
 * three files open at the peak of a header closure the difference
 * is 768 bytes of heap on the machine where cpp itself must fit;
 * half-block reads cost a few extra syscalls and no correctness.
 */
#define TBSIZE 256
#define MACBUF 512          /* macro line/expansion buffer - see macbuf_init */
#define STRBUFSIZE 256      /* string/symbol/identifier buffer */
#define MAXSYMLEN 16        /* symbol buffer size (15 chars + null) */

/*
 * Token flags (tflags)
 */
#define ONELINE   0x01      /* CPP: single line mode */
#define CPPFUNCS  0x02      /* CPP: allow defined() pseudo-function */

/*
 * Error codes - simplified for cpp
 */
typedef int error_t;
#define ER_C_NX     1       /* invalid escape sequence */
#define ER_C_BC     2       /* bad character constant */
#define ER_C_CD     3       /* bad numeric constant */
#define ER_C_TL     4       /* token too long */
#define ER_C_MN     5       /* macro name expected */
#define ER_C_CU     6       /* #elif without #if */
#define ER_C_ME     7       /* missing #endif */
#define ER_C_ID     8       /* invalid directive */
#define ER_C_BD     9       /* bad digit */
#define ER_C_UT     10      /* unknown token */
#define ER_C_DP     11      /* defined requires identifier */
#define ER_C_MA     12      /* macro argument count mismatch */
#define ER_C_EV     13      /* bad enum */
#define ER_C_PC     14      /* too many parameters */
/*
 * Warnings last, and named as such.  What made one used to be being
 * the highest-numbered code, so the next error added after it quietly
 * turned the warning above it into a failure.
 */
#define ER_W_FIRST  15
#define ER_W_SYMTRUNC 15    /* symbol truncated */
#define ER_LAST     ER_W_SYMTRUNC

/*
 * Token structure - lexeme with value
 */
struct token {
    token_t type;
    int lineno;             /* line number where token was scanned */
    char *filename;         /* file where token was scanned */
    union {
        long numeric;       /* char, short, int, long */
        char *name;         /* if we have a symbol */
        cstring str;        /* counted literal string */
    } v;
};

/* Copy token: d = destination ptr, s = source ptr */
extern void tokcpy(struct token *d, struct token *s);
extern void toksynth(struct token *out, unsigned char type);
extern void toksynthnam(struct token *out, unsigned char type, char *name);

/* String interning - canonical pointer for SYM/LABEL names. */
extern char *intern(char *s);

/* Bump arena for never-freed objects (macros, typedefs, interns). */
extern char *permalloc(unsigned int n);
extern char *permdup(char *s);

/* Shared filter utilities */
extern int is_type_kw(unsigned char type);
extern int is_type_tok(struct token *t);

/* how much the token buffers grow by when they fill - see pend_grow */
#define GROWSTEP 16

/* Pending queue (circular): the typedef layer's expansion output */
struct pendbuf {
	struct token *buf;
	int size, rd, wr;
};

/* Dynamic token array (linear, for collecting tokens) */
struct tokarray {
	struct token *buf;
	int count;
	int alloc;
};
extern void pend_init(struct pendbuf *p, int initial);
extern void pend_push(struct pendbuf *p, struct token *t);
extern int pend_has(struct pendbuf *p);
extern void pend_pop(struct pendbuf *p, struct token *out);
extern void pend_tok(struct pendbuf *p, unsigned char type);
extern void pend_buf(struct pendbuf *p, struct token *buf, int len);
extern void pend_setup(struct pendbuf *p, int initial);
extern void tarr_setup(struct tokarray *a, int initial);
extern int tag_pending(struct tokarray *a);

/* Dynamic token array functions */
extern void tarr_init(struct tokarray *a, int initial);
extern void tarr_push(struct tokarray *a, struct token *t);
extern void tarr_reset(struct tokarray *a);

/*
 * Text buffer - for file/macro buffer management and output diversions
 * Used for both input (includes/macros) and output (loop body buffering)
 */
/*
 * NOTE: advance() in io.c reads this struct from hand-written assembly
 * when built for the Z80, by offset, because an asm block cannot see a
 * C declaration.  It wants
 *
 *	storage  at 3	offset  at 5	valid  at 7
 *
 * and it relies on those three being adjacent, addressing storage once
 * and walking to the other two with inc hl.  ccc packs this struct
 * with no padding, so the offsets are just the widths above each field
 * added up: char 1, pointer 2, short 2.
 *
 * Add a field above valid, widen one, or reorder them, and that code
 * reads the wrong bytes and says nothing.  Move the asm with the
 * struct, or take the fast path out.
 */
struct textbuf {
    char fd;                /* if == -1, memory buffer; else file descriptor */
    char *name;             /* filename or macro/buffer name */
    char *storage;          /* buffer data - malloc'd */
    short offset;           /* read/write position in storage */
    short valid;            /* valid bytes (input) */
    short lineno;           /* current line # (input only) */
    char saved_column;      /* saved column position for parent file */
    struct textbuf *prev;   /* stack link */
};

/*
 * Macro definition
 */
struct macro {
    unsigned char parmcount;
    char *name;
    char **parms;
    char *mactext;
    struct macro *next;
};

/*
 * CPP conditional state
 */
struct cond {
    unsigned char flags;
#define C_TRUE      0x01
#define C_ELSESEEN  0x02
#define C_TRUESEEN  0x04
    struct cond *next;
};

/* Global state */
extern char lexFd;          /* .x output file descriptor */
extern char *curFile;       /* current source file */
extern int lineNo;          /* current line number (for errors) */
extern char noLineMarkers;  /* -N flag: suppress LINENO/NEWLINE */

/*
 * Identifiers travel as 2-byte ids and the names go to the .n
 * sidecar, for the two consumers that need spelling - c1, turning
 * ids back into symbols, and the driver, translating @id markers
 * in diagnostics.  c0 never looks: its lookups are 16-bit
 * compares, which on this machine is also the difference between
 * walking a 200-name chain with strcmp and walking it with sbc.
 * This is the only format; there is no flag.
 */
extern unsigned short idOf(char *name);
extern int internWrite(char *fname);

extern unsigned char curchar;
extern unsigned char nextchar;
extern int lineno;
extern char *filename;
extern char column;
extern char *sysIncPath;
extern struct textbuf *tbtop;

extern struct token cur, next;
extern char strbuf[];
extern struct macro *macros;
extern char *macbuffer;
extern struct cond *cond;

/* io.c - input buffer stack */
extern void pushfile(char *name);
extern void insertmacro(char *name, char *macbuf);
extern void insertfile(char *name, int sysdirs);
extern void advance();
extern void ioinit();
extern void addInclude(char *name);

/* io.c - .x stream writer */
extern void outbufWrite(void *data, int len);

/* lex.c */
extern void gettoken();
extern void skipws();
extern void skipws1();
extern void skipallws();
extern char match(token_t t);
extern char issym();

/* kw.c */
extern unsigned char cppkw[];
extern unsigned char ckw[];
extern unsigned char kwlook(unsigned char *str, unsigned char *table);

/* macro.c */
extern void macdefine(char *s);
extern void macundefine(char *s);
extern void addDefine(char *s);

/* emit.c - output functions */
extern void emitFileStart(char *file);
extern void emitToken(unsigned char tok);
extern void emitSym(char *name);
extern void emitNumber(long val);
extern void emitLNumber(long val);
extern void emitString(char *str, int len);
extern void emitLabel(char *name);
extern void emitLine(int line, char *file);
extern void emitStructTok(struct token *t);
extern void emitflstr(void);
extern void emitChkBraces(void);

/* error handling */
extern void error(char *msg);
extern void gripe(error_t err);
extern char *xalloc(unsigned int n);
extern void xnomem(void);

/* lex.c additional exports */
extern unsigned char tflags;
extern cstring nextstr;
extern unsigned long readcppconst(void);
extern char cpppseudofunc(void);

/* macro.c additional exports */
extern struct macro *maclookup(char *name);
extern char macexpand(char *name);
extern char mdefined(char *name);	/* either store: macro or ndef */

/* util.c exports */
extern unsigned char lookupc(char *table, unsigned char c);

/* Utility functions - from lib/libutil.h */
#include "libutil.h"
#ifdef DEBUG
extern int fdprintf(int fd, char *fmt, ...);
extern void hexdump(char *tag, char *h, int l);
#endif
extern long parseConst(token_t stop);
/* norm.c/cfold.c: the fold replay queue's shared bound */
#define SZQ_MAX 22	/* >= CFSV_MAX + 1: a bail replays all */
void dofold(struct token *t);
void reginit(void);
unsigned short stfind(char *tag);
void stadd(char *tag, unsigned short size);
void vadd(char *name, unsigned short total, unsigned short elem,
    unsigned short deref);
void vpop(void);
unsigned short kwsz(unsigned char c, unsigned short base);
int szkw(unsigned char c);
extern unsigned char scopedep;
extern unsigned char cflimit;
extern unsigned char incf;
void srcget(struct token *out);
void knr(struct token *t);
void kout(struct token *t);
void knrinit(void);
void kscan(struct token *t);
extern int dlist;
extern unsigned char inagg;
void out(struct token *t);
void outt(unsigned char type);
void outarr(struct tokarray *a);
void pushb(struct token *t);
void pull(struct token *t);
int decl(struct token *t);
void tdinit(void);

/* debug options */
#ifdef DEBUG
#define VERBOSE(x) (verbose & (x))
extern short verbose;
#else
#define VERBOSE(x) (0)
#endif
#ifndef _XOPEN_SOURCE
extern char *strdup(char *s);
#endif

/* cpp.c - normalizer setup */
extern void filterInit(void);

/* Character classification */
#define iswhite(c) ((c) == ' ' || (c) == '\t' || (c) == '\r')

#endif /* CPP_H */

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
