/*
 * types: what a declaration describes
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
#ifndef _P1TYPE_H
#define _P1TYPE_H

#include "p1base.h"

/*
 * synthetic type information like enum, struct, union, etc goes away as soon
 * as the scope does.  that means that by the time we get to expression trees
 * we only have references to primitive types, sizes and offsets.
 *
 * there are dialects of C that have polluted the lexical scope, for example
 * where struct elements or tags leak upwards.  this isn't one of them.
 *
 * if you have a c source that depends on this kind of thing, fix the source.
 */

/*
 * how big a pointer is 
 */
#define TS_PTR  2

/*
 * a function is an odd type, since it has a return type and argument types,
 * so there needs to be a collection of the latter.  also, a declaration of
 * an instance of a function type can and will have the arguments with different
 * names than a prototype or forward reference.  old style forward function 
 * declarations don't have argument types.
 */

/*
 * this is a handle for types.
 */
struct type {
	unsigned short size;	// how big is one of me.  A short because
							// a struct can be bigger than a register:
							// v6's filsys is 480 bytes.  Arrays still
							// go through typesize(), which works the
							// extent out from the count - see there.
	int count;		    	// array: how many; function: 1 if elem is
							// a slim farg list (see below), else 0
	struct name *elem;		// element list (struct members, function parameters)
    struct type *sub;		// pointer to, array of, function return type
    unsigned char flags;
    struct type *next;
};

/*
 * A function type is its signature: the return type, the parameter
 * types in order, and whether it is variadic.  Parameter names are
 * not part of it; they hang off the declaration that defines the
 * function, where parsefunc/emitPrmDecls/regalloc bind by name.
 *
 * Every function type is a node from fnType()'s table, so two
 * functions with the same signature are the same type, and the
 * identity test is the right one.  That matters because the calling
 * convention is in the signature: the first argument goes in A when
 * it is an 8-bit scalar, and the return value comes back in A or HL
 * depending on its type.
 *
 * elem is always the compact farg list and count is 1, whether or
 * not the declaration named its parameters.  An empty parameter
 * list, "f()", is K&R silence and matches any signature.
 */
struct farg {
	struct type *type;
	struct farg *next;
};
extern struct type *fnType(struct type *ret, struct name *params,
    unsigned char variadic);
extern struct type *fnUnder(struct type *t);
extern char fnCompatible(struct type *t1, struct type *t2);
extern struct name *fnparamsGet(struct name *func);
extern void fnparamsFree(struct name *func);
#define TF_AGGREGATE	0x01
#define TF_INCOMPLETE	0x02
#define TF_UNSIGNED		0x04
#define TF_FUNC         0x08
#define	TF_POINTER		0x10
#define	TF_ARRAY		0x20
/*
 * A union, as against a struct.  Both are TF_AGGREGATE and both lay
 * every member at its own offset - a union's happen all to be 0 - so
 * nothing in the type told the two apart, and the tag's kind (kutag)
 * is no help for an untagged one.  Initialization is the place it
 * matters: K&R's book does not allow a union to be initialized at all.
 */
#define	TF_UNION		0x40
#define TF_VARIADIC     0x80    // for functions: has ... parameter

extern struct type *getbasetype();
extern int incomplete(struct type *t);
extern unsigned char parseSclass();
extern int isBasicType(struct type *t);
struct type *getType(char flags, struct type *sub, int count);
int typesize(struct type *t);
struct type *fnArgType(struct type *t, unsigned char i);
extern char sameRet(struct type *t1, struct type *t2);
extern char sameType(struct type *a, struct type *b);

/*
 * Moved out of p1stmt.h, where they sat beside the switch
 * tables for no reason but history: each of these is about the
 * thing this header declares, and leaving them there made every
 * caller take the statement machinery too.
 */
int streamInitVal(struct type *type);	/* takes a struct type */

#endif
