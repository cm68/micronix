/*
 * type construction and parsing, split from type.c so no single
 * translation unit carries the type tables and the machinery that
 * builds on them.
 */

#include <string.h>
#include "p1core.h"
#include "p1type.h"
#include "p1name.h"
#include "p1lex.h"

extern struct type basictypes[];
extern struct name basicnames[];

/*
 * indexes into the basic type table
 */
#define UN_SIGNED   3
#define OTHERS      6
#define BYTES1     0
#define BYTES2     1
#define BYTES4     2
#define	MISC_BASIC	6

/*
 * Get parameter type i of a function type, or NULL past the end.
 * Handles both elem shapes: full struct name entries (definitions)
 * and slim farg nodes (declarations, count == 1).
 */
struct type *
fnArgType(struct type *t, unsigned char i)
{
    if (t->count) {
        struct farg *fa = (struct farg *)t->elem;
        while (fa && i--)
            fa = fa->next;
        return fa ? fa->type : NULL;
    } else {
        struct name *p = t->elem;
        while (p && i--)
            p = p->next;
        return p ? p->type : NULL;
    }
}

/*
 * Do two declarations agree about what a function hands back?
 *
 * What a caller needs from a return type is how wide the answer is:
 * an int read out of HL where a long was returned takes half of it,
 * which is the whole reason this is checked at all.  Anything the two
 * spellings disagree about that does not change the width, the call
 * site cannot tell apart.  A null side is silence too.
 */
char
sameRet(struct type *t1, struct type *t2)
{
	if (t1 == t2 || !t1 || !t2)
		return 1;
	return typesize(t1) == typesize(t2);
}

/*
 * The function under a pointer, or the function itself.  An array of
 * them is not one: it decays to a pointer to a pointer to a function,
 * and there is no function under that.
 */
struct type *
fnUnder(struct type *t)
{
	if (!t)
		return 0;
	if (t->flags & TF_FUNC)
		return t;
	if ((t->flags & TF_POINTER) && t->sub && (t->sub->flags & TF_FUNC))
		return t->sub;
	return 0;
}

/*
 * Compatibility as the call needs it.
 *
 * Types are interned, so identity is the rule.  The exception is the
 * generic object pointer: the library declares malloc void * and a
 * hundred K&R declarations say char *, and both are one call.  A
 * function value goes through fnCompatible, so a callback's own
 * signature is still checked, and a byte is still not a word.
 */
static char
typeCompat(struct type *a, struct type *b)
{
	struct type *fa, *fb;

	if (a == b)
		return 1;
	if (!a || !b)
		return 0;
	fa = fnUnder(a);
	fb = fnUnder(b);
	if (fa || fb)
		return fa && fb && fnCompatible(fa, fb);
	if ((a->flags & (TF_POINTER | TF_ARRAY)) &&
	    (b->flags & (TF_POINTER | TF_ARRAY))) {
		if (a->sub == voidtype || b->sub == voidtype)
			return 1;
		return a->sub == b->sub;
	}
	return 0;
}

/*
 * Are these two function signatures the same?  The return type, the
 * variadic flag and the parameter types, all by identity: every part
 * of a signature is interned - fnType() for functions, getType() for
 * pointers and arrays, the static table for primitives - so two
 * functions of one signature are one type node.  Parameter names are
 * not consulted; they are not part of the type.
 *
 * An empty parameter list is K&R silence ("extern short adds();")
 * and matches any signature.  That is not "takes no arguments": a
 * function that really takes none says (void), and an empty list
 * here is a list with one void entry.
 *
 * This is the check the calling convention needs.  The first
 * argument rides in A when the prototype says it is an 8-bit
 * scalar, and the answer comes back in A or HL depending on its
 * type; two signatures that disagree are two conventions, and a
 * call through the wrong one is wrong code, not a loose match.
 */
char
fnCompatible(struct type *t1, struct type *t2)
{
	unsigned char i;
	struct type *a1, *a2;

	if (t1 == t2)
		return 1;
	if (!t1 || !t2)
		return 0;
	if (!(t1->flags & TF_FUNC) || !(t2->flags & TF_FUNC))
		return 0;
	if (!typeCompat(t1->sub, t2->sub))
		return 0;
	/*
	 * Silence says nothing about the parameters at all.
	 */
	if (!t1->elem || !t2->elem)
		return 1;
	/*
	 * The ellipsis starts the optional part of the definition.  What
	 * a call must supply is the fixed prefix; a pointer type that
	 * stops there can hold a variadic function, and two declarations
	 * that agree about the required parameters agree, one spelling
	 * the optional tail and one not.  On this machine it is not even
	 * an ABI question: the first-argument convention comes from the
	 * first declared parameter, and the tail is on the stack either
	 * way.  What the ellipsis does is let a callee read past the
	 * fixed list; it does not change what a fixed call looks like.
	 */
	/*
	 * The lists are the required parameters.  They must match
	 * exactly; only the ellipsis flag may differ, because it
	 * describes an optional tail the other spelling simply does not
	 * mention.
	 */
	for (i = 0; ; i++) {
		a1 = fnArgType(t1, i);
		a2 = fnArgType(t2, i);
		if (!typeCompat(a1, a2))
			return 0;
		if (!a1)
			return 1;
	}
}

/*
 * Do two type trees name the same type?
 *
 * Identity is the fast path and what the interner makes common, but a
 * function type is built fresh from its parameter list (fnParams) and
 * never interned, so a pointer to one has a fresh sub each time it is
 * spelled.  Walk the tree instead: a function compares its return
 * (loosely, by size, as sameRet does), a pointer its target, an array
 * its element and extent.  Anything else is a primitive or an
 * aggregate, each keyed to a single node, so a bare mismatch means
 * different.
 */
char
sameType(struct type *a, struct type *b)
{
	if (a == b || !a || !b)
		return 1;
	if ((a->flags & TF_FUNC) && (b->flags & TF_FUNC))
		return sameRet(a->sub, b->sub);
	if ((a->flags & TF_POINTER) && (b->flags & TF_POINTER))
		return sameType(a->sub, b->sub);
	if ((a->flags & TF_ARRAY) && (b->flags & TF_ARRAY))
		return a->count == b->count && sameType(a->sub, b->sub);
	return 0;
}

/*
 * Find or create a type in the unified type system
 *
 * Implements type sharing where two variables of the same type have
 * identical type pointers. This ensures zero redundancy in the type tree
 * and enables fast type comparison using pointer equality.
 *
 * Type sharing rules:
 *   - Primitive types (int, char, etc.): Always shared
 *   - Pointers: Shared if flags and subtype match
 *   - Arrays: Shared if flags, subtype, and count match
 *   - Functions: Shared if flags, return type, and parameters match
 *   - Struct/union: NOT shared (TF_AGGREGATE) - each definition is unique
 *
 * Incomplete types:
 *   - Arrays with count=-1 are marked TF_INCOMPLETE
 *   - Forward-declared structs are TF_INCOMPLETE until definition parsed
 *   - Incomplete types may be updated in place when completed
 *
 * Size calculation:
 *   - Arrays: sub->size * count (0 if incomplete)
 *   - Pointers: TS_PTR constant (2 bytes on this architecture)
 *   - Other types: Size set during type creation
 *
 * Parameters:
 *   flags - Type flags (TF_POINTER, TF_ARRAY, TF_FUNC, TF_AGGREGATE, etc.)
 *   sub   - Subtype pointer (element type for array/pointer, return for function)
 *   count - Array element count (-1 for incomplete arrays)
 *
 * Returns:
 *   Pointer to type (existing if found, new if created)
 */
/*
 * How big is this type, as a number rather than as the byte the type
 * node has room for.  struct type keeps size in an unsigned char,
 * which covers everything a register can hold and not an array:
 * "unsigned char buf[512]" records 0, and sizeof answered 0.  The
 * element count survives, so an array is worked out from that.
 *
 * pass1's own lexBuf is 512 bytes, so read(fd, lexBuf, sizeof lexBuf)
 * asked for none and the compiler read an empty file and said nothing
 * - which is what this was found by.
 */
/*
 * A type whose size nothing has settled.
 *
 * void is the permanent case - it has no size by definition, and a
 * pointer to it is the generic pointer, not a pointer to something
 * one byte long.  A struct only forward-declared and an array with no
 * extent are the temporary ones: a later declaration may complete
 * them, and until it does neither sizeof nor pointer arithmetic has
 * an answer.
 *
 * Asking anyway is a constraint violation, not something to compute
 * around.  Both callers used to compute around it and get 0.
 */
int
incomplete(struct type *t)
{
	if (!t)
		return 1;
	if (t == voidtype)
		return 1;
	if (t->flags & TF_INCOMPLETE)
		return 1;
	if ((t->flags & TF_ARRAY) && t->count <= 0)
		return 1;
	return 0;
}

int
typesize(struct type *t)
{
	if (!t)
		return 0;
	/*
	 * An array carries TF_POINTER as well as TF_ARRAY - it decays -
	 * so the pointer bit says nothing here.  A real array is the one
	 * with a count, which an incomplete one ("char buf[]" as a
	 * parameter) does not have.
	 */
	if ((t->flags & TF_ARRAY) && t->sub && t->count > 0)
		return typesize(t->sub) * t->count;
	return t->size;
}

struct type *
getType(
    char flags,             // TF_whatever
    struct type *sub,       // subtype (return type for functions)
    int count)              // if array, length
{
    struct type *t;
#ifdef DEBUG
    int depth = 0;
#endif

    /*
     * search through types to see if we have a permissive match
     * Cycle protection: limit iterations to prevent infinite loop
     *
     * NOTE: Aggregate types (struct/union) are NOT shared because each
     * struct/union definition is unique with its own member list and size.
     * Only share non-aggregate types (pointers, arrays, functions, primitives).
     */
    if (!(flags & TF_AGGREGATE)) {
        for (t = types; t 
#ifdef DEBUG
&& depth < 1000
#endif
; t = t->next
#ifdef DEBUG
, depth++
#endif
) {
            if ((t->flags == flags) && (t->sub == sub)) {
                /*
                 * For arrays, also check count to distinguish different
                 * array sizes
                 */
                if ((flags & TF_ARRAY) && (t->count != count)) {
                    continue;
                }
                /*
                 * For functions, variadic is part of flags so it matches
                 * automatically
                 */
                return t;
            }
        }

#ifdef DEBUG
        if (depth >= 1000) {
            fdprintf(2, "WARNING: type cache search hit depth limit,"
                " possible cycle in types list\n");
        }
#endif
    }

    t = (struct type *)permalloc(sizeof(*t));
    t->sub = sub;
    t->flags = flags;
    t->count = count;
    if (t->count == -1) {
        t->flags |= TF_INCOMPLETE;
    }

    /* Calculate size for arrays and pointers */
    if (flags & TF_ARRAY) {
        if (sub && count > 0) {
            /*
             * Truncated on purpose - size is a byte, which is enough
             * for anything a register holds and not enough for an
             * array.  count is kept, so typesize() below can work the
             * real number out when someone needs it.
             */
            t->size = sub->size * count;
        } else {
            /*
             * Incomplete array - when used in expressions, decays to
             * pointer so size = TS_PTR for argument promotion
             */
            t->size = (flags & TF_POINTER) ? TS_PTR : 0;
        }
    } else if (flags & TF_POINTER) {
        t->size = TS_PTR;  // pointer size constant
    }

    t->next = types;
    types = t;

    return t;
}


/*
 * Parse basic type keywords into a primitive type
 *
 * Handles C's complex type keyword syntax where unsigned, short, and long
 * can be prefixes or standalone type names. Combines multiple keywords to
 * determine the final primitive type.
 *
 * Valid combinations:
 *   - char, short, int, long: Base types with optional unsigned
 *   - unsigned alone: Defaults to unsigned int (2 bytes)
 *   - unsigned char/short/long/int: Unsigned variants
 *   - float, double: Floating point (unsigned not allowed)
 *   - void: No size (unsigned not allowed)
 *
 * Type resolution:
 *   - Tracks unsignedness (0 or UN_SIGNED offset)
 *   - Tracks length (BYTES1/2/4 for char/short/long)
 *   - Tracks misc types (void/float/double offset)
 *   - Computes index into basictype table: unsignedness + length + misc
 *   - Returns corresponding type from names array (installed by initbasictype)
 *
 * Invalid combinations:
 *   - short long (conflicting length)
 *   - unsigned void/float/double (type qualifier mismatch)
 *
 * Type qualifiers:
 *   - const, volatile: Recognized but ignored by this compiler
 *
 * Returns:
 *   Primitive type pointer, or NULL if no type keywords found
 */
struct type *
parsebasic()
{
    char unsignedness = 0;
    char length = 0;
    char misc = 0;

    while (1) {
		switch (cur.type) {

		case CHAR:
			gettoken();
			length = BYTES1 + 1;
			goto done;

		case LONG:
			gettoken();
			if (length) {
				gripe(ER_T_PT);
			}
			length = BYTES4 + 1;
			continue;

		case SHORT:
			gettoken();
			if (length) {
				gripe(ER_T_PT);
			}
			length = BYTES2 + 1;
			continue;

		case INT:
			gettoken();
			if (!length) {
				length = BYTES2 + 1;
			}
			goto done;

        case UNSIGNED:
            gettoken();
            unsignedness = UN_SIGNED;
            continue;


		case VOID:
			gettoken();
			if (length + unsignedness) {
				gripe(ER_T_PT);
				length = 0;
			}
			misc += MISC_BASIC;
			goto done;

		default:
            // no type, no prefixes, unrecognized keyword. stop parsing type.
			if ((length + unsignedness) == 0) {
				return 0;
			}
			goto done;
		}
	}
done:
    if (unsignedness && (length == 0)) {    // naked unsigned
        length = BYTES2 + 1;
    }
    if (length) length--;
    return basicnames[unsignedness + length + misc].type;
}

/*
 * typedef handling and sametype() would go here
 * These require more infrastructure (new_type, normalizetype, etc.)
 * that doesn't exist yet, so they're removed for now.
 */

/*
 * Parse a base type (primitive, typedef, struct, union, or enum)
 *
 * This is the main type parser that handles all base types before
 * declarators add pointers, arrays, and functions. Returns when a complete
 * base type has been parsed.
 *
 * Type forms handled:
 *   - Primitive types: int, char, void, etc. (via parsebasic)
 *   - Typedef names: Previously declared type aliases
 *   - Struct/union: With or without tag, with or without body
 *   - Enum: With or without tag, with or without enumerator list
 *
 * Struct/union processing:
 *   - Forward declarations: struct foo; (creates incomplete type)
 *   - Definitions: struct foo { ... }; (creates complete type)
 *   - Anonymous: struct { ... } x; (no tag)
 *   - Member parsing: Calls declInternal for each member
 *   - Bitfield packing: Packs bitfields into 16-bit words
 *   - Size calculation: Sum of member sizes (struct) or max (union)
 *
 * Enum processing:
 *   - All enums are unsigned char (1 byte)
 *   - Enumerators are named constants in global namespace
 *   - Values default to sequential (0, 1, 2...) or explicit (= expr)
 *   - Tag is optional and ignored (just for documentation)
 *
 * Incomplete types:
 *   - Forward-declared struct/union without body
 *   - Marked TF_INCOMPLETE until definition found
 *   - Updated in place when body parsed later
 *
 * Returns:
 *   Type pointer for parsed base type, or NULL if no type found
 */
struct type *
getbasetype()
{
    struct type *t;
    struct name *n;
    /*
     * unsigned int, not long: offsets and enum values fit in 16 bits,
     * and zc3 miscompiles uchar -> ulong promotions (the loaded byte
     * gets clobbered), which broke every member offset and enum value.
     */
    unsigned int off = 0;
    unsigned short s;	/* tag id, 0 = untagged */
    int bitoff_accum;
    struct type *member_type;
    struct name *member;
    unsigned char is_union;

    t = parsebasic();
    if (t) {
        return t;
    }
    if ((cur.type != STRUCT) && (cur.type != UNION)) {
        return 0;
    }

    /*
     * struct or union [name] [ { members } ]
     */
    if (cur.type == STRUCT || cur.type == UNION) {
        is_union = (cur.type == UNION);
        gettoken();
        n = 0;
        s = 0;

        // optional struct/union tag name
        if (cur.type == SYM) {
            s = cur.v.id;
            n = findName(s, 1);  // look for existing tag
            gettoken();

            // if found and already complete, skip body if present and return it
            if (n && !(n->type->flags & TF_INCOMPLETE)) {
                // In phase 2, the body tokens still need to be consumed
                if (cur.type == BEGIN) {
                    unsigned char depth = 1;
                    gettoken();  // consume '{'
                    while (depth > 0 && cur.type != E_O_F) {
                        if (cur.type == BEGIN) depth++;
                        else if (cur.type == END) depth--;
                        gettoken();
                    }
                }
                return n->type;
            }
        }

        // must have a definition if no tag or if forward reference
        if (cur.type != BEGIN) {
            if (n) {
                return n->type;  // forward reference - tag already exists
            }
            // Forward declaration of a new tag (e.g., typedef struct S S_t;)
            // Create an incomplete type
            if (s) {
                t = getType(TF_AGGREGATE | TF_INCOMPLETE |
                    (is_union ? TF_UNION : 0), 0, 0);
                t->size = 0;
                n = newName(s, is_union ? kutag : kstag, t, 1);
                return t;
            }
            // No tag name and no definition - error
            gripe(ER_T_ED);
            return 0;
        }

        // create or reuse the struct/union type
        if (n && (n->type->flags & TF_INCOMPLETE)) {
            // Reuse existing incomplete type to maintain pointer identity
            t = n->type;
            t->flags &= ~TF_INCOMPLETE;  // will be completed below
            if (is_union)
                t->flags |= TF_UNION;
            t->size = 0;
        } else {
            // Create new type
            t = getType(TF_AGGREGATE | (is_union ? TF_UNION : 0), 0, 0);
            t->size = 0;
        }

        if (s) {
            // create or update the tag
            if (!n) {
                n = newName(s, is_union ? kutag : kstag, t, 1);
            } else if (n->type != t) {
                // Only update if we created a new type (shouldn't happen now)
                n->type = t;
            }
            /* Stack buffer automatically freed */
        }

        // parse member list: { type name; ... }
        match(BEGIN);
        off = 0;  // offset for struct members
        /*
         * bit offset within current word for bitfield packing
         */
        bitoff_accum = 0;
        while (cur.type != END && cur.type != E_O_F) {
            member_type = 0;

            /*
             * parse member declaration (struct_elem=true to avoid global
             * namespace pollution). Loop handles comma-separated declarators
             * sharing the same base type: int a, *b, c[3];
             */
            do {
                member = declare(&member_type, 1);
                if (!member) {
                    // skip to semicolon or end
                    while (cur.type != SEMI && cur.type != END &&
                           cur.type != E_O_F) {
                        gettoken();
                    }
                    break;
                }

                // add to member list
                member->next = t->elem;
                t->elem = member;

                // Set kind if not already set (bitfields are already marked)
                if (member->kind != kbitfield) {
                    member->kind = kelem;
                }

                // calculate offset and size
                if (is_union) {
                    member->w.m.offset = 0;
                    if (member->type->size > t->size)
                        t->size = member->type->size;
                } else {
                    // Handle bitfield packing
                    if (member->kind == kbitfield) {
                        // Bitfield - pack into current word
                        /*
                         * Check if bitfield fits in current word (assume
                         * 16-bit words for now)
                         */
                        if (bitoff_accum + member->w.m.width > 16) {
                            // Move to next word
                            off += 2;  // Advance to next word (2 bytes)
                            bitoff_accum = 0;
                        }

                        member->w.m.offset = off;
                        member->w.m.bitoff = bitoff_accum;
                        bitoff_accum += member->w.m.width;

                        // Update struct size if we're using a new word
                        if (off + 2 > t->size) {
                            t->size = off + 2;
                        }
                    } else {
                        // Regular member - reset bitfield accumulator
                        if (bitoff_accum > 0) {
                            /*
                             * Finish current bitfield word before adding
                             * regular member
                             */
                            off += 2;
                            bitoff_accum = 0;
                        }

                        member->w.m.offset = off;
                        /* typesize, not the byte-wide size field: a
                         * member array past 255 bytes has a wrapped
                         * size and used to contribute nothing - the
                         * switch stack in pass2 laid out 48 bytes for
                         * 8K and every case value past the tenth
                         * stomped the statics behind it */
                        off += typesize(member->type);
                        t->size = off;
                    }
                }
            } while (cur.type == COMMA && (gettoken(), 1));

            // expect semicolon after member(s)
            if (cur.type == SEMI) {
                gettoken();
            } else if (cur.type != END) {
                gripe(ER_T_ED);
            }
        }
        match(END);

        // mark as complete
        t->flags &= ~TF_INCOMPLETE;

        /*
         * size and member offsets are shorts now, so the layout is
         * good to 65535 and this only has to catch what would wrap
         * one.  It was 127, which is the (ix+d) window - but that
         * window was never what bounded a struct.  pass2 already
         * forms the address with 16-bit arithmetic when a member
         * sits past the displacement, the same way it does for a
         * local array too big for (iy+d), so "p->member" at offset
         * 202 comes out as "ld hl,(_p) / ld de,202" and is correct.
         *
         * The check reads the WIDE accumulator, which is the point:
         * the field it is protecting is exactly the one that cannot
         * be trusted to have kept the value.
         */
        if (off > 65535L) {
            gripe(ER_T_SB);
        }

        return t;
    }

    gripe(ER_T_UT);
    return 0;
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
