/*
 * initializer value streaming, split from init.c: the walk that
 * prices and emits one initializer value at a time.
 */

#include <stdlib.h>
#include "p1core.h"
#include "p1expr.h"
#include "p1type.h"
#include "p1name.h"
#include "p1lex.h"

extern unsigned short curFuncId;	/* outast.c */

/*
 * Find struct member at given offset (members are linked in reverse order)
 *
 * The LAST match, not the first, because the list is reverse declaration
 * order: the last entry carrying an offset is the one declared first.
 *
 * For a struct that is the same member either way - offsets are unique,
 * so there is only one match.  For a UNION every member sits at offset
 * 0, and the difference is the whole answer: C says a brace initializer
 * for a union initializes its FIRST member, and taking the first match
 * took the LAST one declared.
 *
 *	union v { int i; char b[4]; } u = { 0x1234 };
 *
 * picked b, a char[4], and streamed 0x1234 into it as though four bytes
 * of array were a long.  It came to the right SIZE by luck and put the
 * value in the wrong shape.
 */
struct name *
findMemberOff(struct name *members, int offset)
{
    struct name *found = NULL;

    while (members) {
        if (members->w.m.offset == offset)
            found = members;
        members = members->next;
    }
    return found;
}

/*
 * The width the leading value of a scalar initializer goes in: the
 * size of the first scalar inside the object.
 *
 * An aggregate initialised with a scalar is not C - see the streaming
 * of one below - but it has to be given SOME width, and the width the
 * object's first element would have taken is the one that agrees with
 * the braced spelling of the same thing.
 */
static int
leadWidth(struct type *t)
{
    struct name *m;
    int guard = 16;		/* a type cannot nest forever */

    while (t && guard--) {
        if (t->flags & TF_ARRAY) {
            t = t->sub;
        } else if (t->flags & TF_AGGREGATE) {
            m = findMemberOff(t->elem, 0);
            if (!m)
                break;
            t = m->type;
        } else {
            break;
        }
    }
    return (t && t->size) ? (int)t->size : 2;
}

/*
 * One aggregate's worth of values out of a list that left the inner
 * braces out.
 *
 *	struct s x[2] = { 1,2, 3,4 };
 *
 * is the members of x[0] and then those of x[1], and C has always
 * allowed it to be written that way.
 *
 * Without this the whole struct type was handed to the scalar path
 * once per value, and an aggregate meeting a scalar there means the
 * Whitesmiths "fill the object" idiom - so each value was written at
 * the width of the entire struct and padded out to it.  An array of
 * two-member structs came out twice its size, with a member's worth
 * of zeroes after every value, while the code reading it indexed by
 * sizeof and found the wrong member.  A single-member struct is the
 * one shape where the two widths agree, so those looked right.
 *
 * The shell's operator table is eight of these.  isop() matched none
 * of them, getword() then stopped on a metacharacter it would not
 * consume, and the parser turned over forever on the first line
 * holding a pipe or a redirect.
 */
static void
streamElided(struct type *t)
{
    struct name *m;
    unsigned short off;
    int emitted;

    off = 0;
    emitted = 0;
    m = findMemberOff(t->elem, 0);
    while (m && cur.type != END) {
        streamInitVal(m->type);
        emitted += (int)m->type->size;
        off += m->type->size;
        m = findMemberOff(t->elem, off);
        if (!m || cur.type != COMMA)
            break;
        gettoken();
    }
    /*
     * Short of members named, the rest of the object is still there -
     * the same reason the braced form pads below.
     */
    while (emitted < (int)t->size) {
        asmDb(0);
        emitted++;
    }
}

/*
 * Stream an initializer value directly to assembly output
 * Used for static/global initializers to avoid building expression trees
 * Returns count of top-level elements (for array size inference)
 */
int
streamInitVal(struct type *type)
{
    int size = type ? type->size : 2;
    int count = 0;
    struct type *elem_type;
    struct name *member;       /* also used as np in expr branch */
    /*
     * member_offset walks a struct, and a struct is at most 255
     * bytes - type->size is itself a byte.  is_struct is a truth
     * value.  Byte locals cost one load where an int costs two,
     * and this function tests both in loops.
     */
    unsigned short member_offset;
    unsigned char is_struct;
    cstring str;
    int slen, arrlen, i, b;
    int totalsz, leadsz, emitted;
    char buf[20];              /* used as strname in STRING, buf in expr */
    struct expr *e;
    long val;

    if (cur.type == BEGIN) {
        /* Nested initializer list */
        elem_type = NULL;
        member = NULL;
        member_offset = 0;
        is_struct = 0;
        if (type && (type->flags & TF_AGGREGATE)) {
            is_struct = 1;
            member = findMemberOff(type->elem, 0);  /* First member at offset 0 */
            elem_type = member ? member->type : NULL;
        } else if (type && (type->flags & TF_ARRAY)) {
            elem_type = type->sub;  /* array - pass element type */
        }
        gettoken();  /* consume { */
        while (cur.type != END) {
            /*
             * An aggregate element whose braces were left out takes
             * one value per member, not one value for the whole of
             * it.  With braces it comes back here and walks itself.
             */
            if (elem_type && (elem_type->flags & TF_AGGREGATE) &&
                cur.type != BEGIN)
                streamElided(elem_type);
            else
                streamInitVal(elem_type);
            count++;
            /* Advance to next struct member by offset */
            if (is_struct && member) {
                member_offset += member->type->size;
                member = findMemberOff(type->elem, member_offset);
                elem_type = member ? member->type : NULL;
            }
            if (cur.type == COMMA)
                gettoken();
            else if (cur.type != END) {
                gripe(ER_S_SN);
                return count;
            }
        }
        gettoken();  /* consume } */
        /*
         * Pad out the rest of the struct.  An initializer that names
         * fewer members than the struct has still occupies all of it,
         * and the next element of an array of these begins at sizeof
         * - not where the initializers happened to stop.
         *
         * Without this, pass1's own basicnames[] - seven entries of
         * "{ name, type, chain }" out of a struct with a dozen fields
         * - sat 20 bytes apart in memory while every subscript was
         * computed against sizeof, which is 37.  basicnames[1] read
         * the middle of basicnames[0]'s name and came back null, so
         * the c0 that ccc built could not name a basic type: "int"
         * and "long" and "unsigned char" as parameters all answered
         * "fn array".  Index 0 worked, having no stride to get wrong.
         */
        if (is_struct && type) {
            while (member_offset < type->size) {
                asmDb(0);
                member_offset++;
            }
        } else if (type && (type->flags & TF_ARRAY) && type->count > 0 &&
                   count < type->count) {
            /*
             * And the rest of an array, for the same reason one
             * level up.  An initializer that names fewer elements
             * than the array has still occupies all of it, and what
             * is declared after it begins at the end of the array,
             * not where the initializers stopped.
             *
             * Without this, libc's "FILE _iob[_NFILE]" - three
             * entries initialised out of six - emitted 24 bytes of a
             * 48 byte array, and stdin/stdout/stderr, declared next,
             * were laid down on top of _iob[3..5].  Their pointer
             * values read back as those entries' flags, so fopen
             * found no free slot and every fopen in the system
             * returned null while freopen on a named entry worked.
             */
            b = typesize(type->sub) * (type->count - count);
            while (b-- > 0)
                asmDb(0);
        }
    } else if (cur.type == STRING) {
        /* String literal - check if initializing char array inline */
        str = cur.v.str;
        if (type && (type->flags & TF_ARRAY) && type->sub &&
            type->sub->size == 1 && str) {
            /* Emit string bytes inline for char array */
            slen = str[0];
            arrlen = type->count > 0 ? type->count : slen + 1;
            for (i = 0; i < arrlen; i++) {
                b = (i < slen) ? (unsigned char)str[i + 1] : 0;
                asmDb(b);
            }
            /* Skip the strN label emitted in phase 1 - keep counter in sync */
            globalStrCtr++;
            count = slen + 1;  /* Array size for char[] = "..." */
        } else {
            /* Pointer to string - just emit reference (data emitted in phase 1) */
            fmtstr(buf, "str%d", globalStrCtr++);
            asmDwSym(buf);
            count = 1;
        }
        cur.v.str = NULL;
        gettoken();
    } else {
        /*
         * Parse single expression, emit, free.
         *
         * Folded first.  This streams straight to assembly instead of
         * building an AST, so it never reached foldTree, and the test
         * just below asks for a CONST node - which an unfolded "a | b"
         * is not.  Every constant expression in a static initialiser
         * took the unsupported branch and was written as zero, at both
         * widths, while the same expression anywhere else folded fine.
         *
         * cpp's keyword tables are built out of "c | HI" entries, so
         * every skip byte in its trie was nought and only the one
         * directive spelled without them still matched.
         */
        /*
         * An AGGREGATE initialised with a scalar - "union u a = 0;",
         * "char image[512] = 0;", "struct cmd bad[128] = 0;".
         *
         * This is not C in any edition.  K&R's own book does not allow
         * a union to be initialized at all, and ANSI, which does, wants
         * a brace-enclosed initializer for the union's first member; a
         * bare scalar for an array or a struct is a constraint
         * violation everywhere.  Whitesmiths accepted it and read it as
         * "zero fill the object", and because it accepted it the idiom
         * is through every source of that period - including this
         * tree's own boot loader, where "union diskbuf disk0 = 0;" is a
         * 512 byte sector buffer.
         *
         * So it is accepted here and the object is filled.  What could
         * not stand was the SIZE: the value was emitted at the width of
         * a register and that was the whole object, so a 512 byte
         * buffer occupied two bytes and everything declared after it
         * was laid down inside it.  Nothing was said, the program
         * linked, and the first write through the buffer landed on the
         * next variable.  The Micronix boot loader read one inode block
         * and wrote 19 bytes of it over the disk spec it had just
         * range-checked against, so the read that passed the check
         * destroyed the limit the next one used.
         *
         * The same width bug reached VALID C through the braced form:
         * "{ 0 }" on a union streams the member, and a member that is
         * itself an array came through here and got two bytes.
         */
        totalsz = 0;
        leadsz = size;
        if (type && (type->flags & (TF_ARRAY | TF_AGGREGATE)) &&
            !((type->flags & TF_ARRAY) && type->count < 0)) {
            totalsz = typesize(type);
            leadsz = leadWidth(type);
        }
        emitted = 0;
        e = foldTree(parseExpr(OP_PRI_COMMA));
        if (e) {
            if (e->op == CONST) {
                val = (long)e->v;  /* e->v IS the value, not a pointer */
                emitted = leadsz;
                if (leadsz == 1)
                    asmDb((int)val);
                else if (leadsz == 4) {
                    /*
                     * Four bytes, HIGH word first, which is how
                     * everything that reads one expects to find it -
                     * see QLONG.md and NUXI.  A word was emitted for
                     * anything that was not a byte, so an initialised
                     * long global was half a long - the next variable
                     * sat where its other word belonged, and reading it
                     * back gave that variable in the top half.
                     */
                    asmDw((int)((val >> 16) & 0xffffL));
                    asmDw((int)(val & 0xffffL));
                } else {
                    asmDw((int)val);
                    emitted = 2;
                }
            } else if (e->op == SYM) {
                member = (struct name *)e->var;
                if (member->sclass & SC_STATIC)
                    staticName(buf, member->id,
                        member->level > 1 ? curFuncId : 0,
                        member->static_id);
                else
                    fmtstr(buf, "_%s", nameOf(member->id));
                asmDwSym(buf);
                emitted = 2;
            } else if (e->op == PLUS && e->left->op == SYM &&
                       e->right && (e->right->flags & E_CONST)) {
                /*
                 * The address of an element other than the first.
                 * "&arr[0]" folds to the bare symbol and was taken by
                 * the branch above; "&arr[n]" keeps the offset and is
                 * a PLUS, which matched neither and was written as
                 * zero.  The assembler takes label+offset, so say it.
                 *
                 * pass1's own type table is built this way:
                 *
                 *	{ 2, 0, 0, 0, 0, &basictypes[0] },
                 *	{ 4, 0, 0, 0, 0, &basictypes[1] },
                 *
                 * so every link past the first was null and inttype,
                 * longtype and voidtype were all zero.  The c0 that
                 * ccc built could not name a type it had not been
                 * given by a typedef.
                 */
                member = (struct name *)e->left->var;
                if (member->sclass & SC_STATIC)
                    {
                        char nb[32];
                        staticName(nb, member->id,
                            member->level > 1 ? curFuncId : 0,
                            member->static_id);
                        fmtstr(buf, "%s+%d", nb, (int)e->right->v);
                    }
                else
                    fmtstr(buf, "_%s+%d", nameOf(member->id),
                        (int)e->right->v);
                asmDwSym(buf);
                emitted = 2;
            } else {
                /* Unsupported initializer - emit zero */
                if (leadsz == 1) {
                    asmDb(0);
                    emitted = 1;
                } else {
                    asmDw(0);
                    emitted = 2;
                }
            }
            FreeExpr(e);
        }
        /*
         * and the rest of the object.  totalsz is 0 for a plain scalar,
         * so this costs nothing where nothing is needed.
         */
        while (emitted < totalsz) {
            asmDb(0);
            emitted++;
        }
        count = 1;
    }
    return count;
}

