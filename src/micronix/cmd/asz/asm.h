/*
 * includes
 *
 * /usr/src/cmd/asz/asm.h
 *
 * Changed: <2023-08-02 08:56:38 curt>
 *
 */
#ifndef ASM_H
#define ASM_H

extern FILE *input_file;
extern int lineNum;
extern char *infile;
extern char verbose;
extern char g_flag;
extern char no_relax;

#define SYMLEN 15

/*
 * A SYMBOL COSTS WHAT ITS NAME COSTS.  The name used to be a fixed
 * char[SYMLEN+1] in the middle of the structure, so every symbol paid
 * for sixteen bytes whether it needed them or not - and the average
 * name in what this assembles is six characters.  It is a tail now:
 * last member, one byte declared, and the rest of it allocated behind
 * the structure by whoever makes the symbol.
 *
 * name[1] and not name[0] or name[], deliberately.  sizeof then
 * carries the terminator, so permalloc(sizeof(struct symbol) + len)
 * is exactly right for a name of len characters on both compilers -
 * ccc gives a flexible member one byte and gcc gives it none, and
 * "char name[0]" is a GNU extension that -pedantic -Werror rejects
 * outright.  One byte declared is the arithmetic everything agrees on.
 */
struct symbol {
    unsigned char seg;              /* SEG_* */
    unsigned short index;           /* object file ordinal */
    unsigned short value;           /* segment relative */
    struct symbol *next;            /* bucket chain, then list: see asm.c */
    char name[1];                   /* variable length tail */
};

struct jump {
    unsigned short idx;         /* the jump's ordinal, its name in jrbits */
    unsigned short addr;        /* address of jp instruction */
    struct symbol *sym;         /* target symbol (NULL for absolute) */
    unsigned short offset;      /* target offset */
    unsigned char cond;         /* condition (T_NZ..T_CR) or 0 for unconditional */
    unsigned char is_jr;        /* 1 if converted to jr */
    struct jump *next;
};

/*
 * local labels: numeric labels with forward/backward references
 * Nf = next occurrence forward, Nb = previous occurrence backward
 *
 * Synthetic name architecture: local labels are converted to synthetic
 * symbols (__L0_001, __L1_002, etc.) allowing unlimited reuse.
 *
 * State per label number:
 *   pending - synthetic symbol awaiting next N: definition (for Nf refs)
 *   last    - symbol from most recent N: definition (for Nb refs)
 * Stored in hash table keyed by label number.
 */
#define LOCAL_HASHSZ 32

struct local_state {
    int num;                    /* label number */
    struct symbol *pending;     /* pending forward ref symbol */
    struct symbol *last;        /* most recent definition */
    struct local_state *next;   /* hash chain */
};

/*
 * expressions can take values of this:
 * if both sym and num are present, this is a biased symbol
 * something like .dw  foo+34
 */
struct expval {
    struct symbol *sym;
    union {
        unsigned long l;
        unsigned int w;
        unsigned char b;
    } num;
    unsigned char hilo;     /* RELOC_WORD/LO/HI */
};

/* interface functions */

void appendtmp();
void list_line();
void list_take();
void list_symbols();
void asm_reset();
void assemble();
char peek();
char get_next();
void outbyte();
void outtmp();

#endif

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
