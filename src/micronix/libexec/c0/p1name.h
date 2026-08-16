/*
 * names: symbols, scopes, and storage class
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
#ifndef _P1NAME_H
#define _P1NAME_H

#include "p1base.h"

/*
 * enum constants are lowered to #defines by cpp, so their names are
 * global: prefix with k to keep them clear of member/variable names.
 */
typedef enum {
    kprim, ketag, kstag, kutag, kvar, kelem, ktdef, kfdef, kbitfield,
    kfunarg, klocal
} kind;

/*
 * note that at the same scope, you can have
 * multiple instances of the same name with different namespaces.
 * this is a container for types, functions, variables, constants, and fields
 *
 * Fields ordered so basicnames[] static initializer needs no trailing zeros:
 * name, type, chain are always set; remaining fields zero from calloc/elision
 */
struct name {
	/* Non-zero in basicnames[] - put first for trailing zero elision */
	unsigned short id;      // interned identifier (0 = unnamed)
	struct type *type;
	struct name *chain;     // symbol table chain (most recent first)

	/* Zero in basicnames[], set dynamically elsewhere */
	kind kind;
	unsigned char level;    // lexical level (0+)
	unsigned char is_tag;   // true if (enum, struct, union)
	unsigned char sclass;   // storage class (SC_STATIC, SC_EXTERN, etc.)
	unsigned char emitted;  // true if string literal already emitted
	unsigned char static_id; // 0=normal, >0=static (emit as S<id-1>)
	struct name *next;		// all names in same container
	union {
		struct expr *init;  // value of constant or initializer (for var)
		struct local *locals; // local variables (for fdef)
	} u;
	/*
	 * struct members and bitfields (kind elem/bitfield) never reach
	 * register allocation, and vars/locals/funargs are never struct
	 * members, so the two field groups can share storage.
	 */
	union {
		struct {
			unsigned short offset;  // offset inside the struct
			                        // (short, not char: the disk
			                        // superblock is 480 bytes and
			                        // its later members live past
			                        // 255.  Free - the r arm of
			                        // this union is 7 bytes and
			                        // this one was 3.)
			unsigned char bitoff;   // bit offset (0-7)
			unsigned char width;    // bitfield width (1-32)
		} m;
		struct {
			unsigned char ref_count;  // reference count (capped at 255)
			unsigned char agg_refs;   // struct member access count (for IX allocation)
			unsigned char reg;        // allocated register: 0=none, 1=B, 2=C, 3=BC, 4=IX
			unsigned char addr_taken; // true if address taken (can't use register)
			unsigned char blkid;      // which block declared it (0 = function
			                          // body).  With level, this is enough to
			                          // walk the scope tree in declaration
			                          // order and let sibling blocks share
			                          // frame space.
			short frm_off;            // frame offset: params positive, locals
			                          // negative; arrays sit below the save
			                          // slots and may pass -128
		} r;
	} w;
};

/*
 * Storage class specifiers (used in struct name sclass field)
 * these are in the same order as the storage class lexemes
 */
#define	SC_TYPEDEF	0x01
#define	SC_AUTO		0x02
#define	SC_EXTERN	0x04
#define	SC_STATIC	0x08
#define	SC_REGISTER	0x10

/* Register allocation values (used in struct name reg field) */
#define REG_NONE    0   /* Not allocated to a register (on stack) */
#define REG_B       1   /* B register (byte) */
#define REG_C       2   /* C register (byte) */
#define REG_BC      3   /* BC register pair (word) */
#define REG_IX      4   /* IX index register (struct pointer) */

extern struct name *newName(unsigned short id, kind k, struct type *t,
    unsigned char is_tag);
extern struct name *addName(struct name *n);
extern struct name *findName(unsigned short id, unsigned char is_tag);
extern void pushScope(char *name);
extern void popScope(void);

/* declare.c */
/*
 * How deep the block numbering goes.  Past this a local keeps block 0
 * and simply does not share, which costs frame space and nothing else.
 */
#define MAXBLKLVL 24

extern unsigned char lexlevel;
extern struct local *blockLocals;	/* nested-block locals awaiting hoist */
extern unsigned char curblk(void);
extern void clrblklocs(void);
extern struct name *names;
extern struct name *deadNames;
extern struct type *types;
extern char *kindname[];
extern struct name *declare(struct type **btp, unsigned char struct_elem);
extern char isCastStart(void);
extern struct type *parseTypeName(void);

extern struct type *chartype;
extern struct type *inttype;
extern struct type *longtype;
extern struct type *uchartype;
extern struct type *ushorttype;
extern struct type *ulongtype;
extern struct type *voidtype;

void parse();
void parseSpan();
extern int spanStop;	/* a function definition just ended */
void cleanupParse();
void drainGraves();
void dropName(struct name *n);
int isasgn(unsigned char t);

/*
 * Moved out of p1stmt.h, where they sat beside the switch
 * tables for no reason but history: each of these is about the
 * thing this header declares, and leaving them there made every
 * caller take the statement machinery too.
 */
extern int idOnce(unsigned short id);	/* an id, and nothing else */
void doInitlzr(struct name *n);	/* takes a struct name */

#endif
