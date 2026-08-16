#include <stdlib.h>
#include "p1core.h"
#include "p1expr.h"
#include "p1type.h"
#include "p1name.h"
#include "p1lex.h"

struct type *redeclOld;	/* the type a reused entry already had */

/*
 * Parse pointer prefix and build pointer type chain
 *
 * Processes zero or more '*' tokens to construct a chain of pointer types.
 * Each '*' wraps the previous type in a pointer type, building from right
 * to left in the type hierarchy.
 *
 * Type construction:
 *   - char **      -> pointer to pointer to char
 *   - int ***      -> pointer to pointer to pointer to int
 *
 * Precedence with arrays/functions:
 *   - Pointers bind tighter than arrays: int *arr[10] is array of pointers
 *   - Function pointers need parens: int (*fptr)() is pointer to function
 *
 * Parameters:
 *   basetype - The base type to wrap with pointer(s)
 *
 * Returns:
 *   Type pointer representing the complete pointer chain, or basetype if no '*'
 *
 * Side effects:
 *   - Consumes '*' and qualifier tokens from input stream
 */
struct type *
parsePtrPfx(struct type *t)
{
    while (cur.type == STAR) {
        gettoken();
        t = getType(TF_POINTER, t, 0);
    }
    return t;
}

/*
 * Create function parameter name entry for type signature
 *
 * Allocates and initializes a name structure to represent a function parameter
 * in the function type's elem list. These entries serve dual purposes:
 *   1. Store parameter types in function type signature
 *   2. Provide names for parsefunc() to create namespace entries
 *
 * Parameter name handling:
 *   - Always duplicates name string to prevent dangling pointers
 *   - Anonymous parameters get empty string "" (not NULL)
 *   - compatFnTyp() ignores names when comparing signatures
 *
 * Level and scope:
 *   - Sets level to lexlevel+1 (function body scope)
 *   - Parameters become visible when parsefunc() processes function body
 *   - Temporary entries in type->elem, real namespace entries created later
 *
 * Parameters:
 *   name - Parameter name (can be NULL for anonymous, will be duplicated)
 *   type - Parameter type
 *
 * Returns:
 *   Newly allocated name entry with funarg kind
 *
 * Side effects:
 *   - Allocates memory for name structure and duplicates name string
 */
struct name *
createPrmEnt(unsigned short id, struct type *type)
{
    struct name *arg = (struct name *)galloc(sizeof(*arg));
    arg->id = id;
    arg->type = type;
    arg->level = lexlevel + 1;
    arg->is_tag = 0;
    arg->kind = kfunarg;
    return arg;
}

/*
 * Replace a declaration's parameter name entries with compact
 * type-only nodes (struct farg), marking the shape with count = 1.
 * Chases pointers, arrays and return types so function pointer
 * declarators and functions returning function pointers get slimmed
 * too, and recurses into parameter types for fn-pointer parameters.
 * Safe to call on any type; no-op for non-functions and for types
 * already slimmed.  Never called on a definition's own type.
 */
void
slimFnArgs(struct type *t)
{
    struct name *p, *pnext;
    struct farg *fa, *tail;

    while (t && !(t->flags & TF_FUNC)) {
        if (t->flags & (TF_POINTER | TF_ARRAY)) {
            t = t->sub;
        } else {
            return;
        }
    }
    if (!t || t->count) {
        return;
    }
    t->count = 1;           /* elem is now a farg list */
    p = t->elem;
    t->elem = NULL;
    tail = NULL;
    while (p) {
        pnext = p->next;
        slimFnArgs(p->type);    /* fn-pointer parameter */
        fa = (struct farg *)permalloc(sizeof(*fa));
        fa->type = p->type;
        fa->next = NULL;
        if (tail) {
            tail->next = fa;
        } else {
            t->elem = (struct name *)fa;
        }
        tail = fa;
        free(p);
        p = pnext;
    }
    slimFnArgs(t->sub);     /* function returning function pointer */
}

/*
 * declare()'s phases, one worker apiece.  It was the largest single
 * function in c0 - seventeen hundred instructions - and this compiler
 * does no lifetime analysis, by design: locals of phases that can
 * never overlap still shared one frame and two registers.  The
 * function boundary is the lifetime analysis.
 */

/*
 * The name being declared: struct member, redeclaration, shadow or
 * new entry, plus the bitfield width if one follows.  cur is on the
 * SYM coming in and past the bitfield (if any) going out.
 */
struct name *
symDecl(struct type *prefix, unsigned char struct_elem)
{
    struct name *nm;

    redeclOld = NULL;

    if (struct_elem) {
        /*
         * struct members: create name but DON'T add to
         * global names[] array
         */
        nm = (struct name *)galloc(sizeof(*nm));
        nm->id = cur.v.id;
        nm->type = prefix;
        nm->level = lexlevel;
        nm->is_tag = 0;
        nm->kind = kelem;  /* will be struct/union member */
        nm->w.m.offset = 0;
        nm->w.m.width = 0;
        nm->w.m.bitoff = 0;
        nm->next = 0;
        nm->u.init = 0;
#ifdef DEBUG
        if (VERBOSE(V_SYM)) {
            fdprintf(2, "struct_elem: %s (not added to names[])\n",
                     nameOf(nm->id));
        }
#endif
    } else {
        /* normal variable: add to global names[] array */
        /* Check if this name already exists at this scope */
        struct name *existing = findName(cur.v.id, 0);
        if (existing && existing->level == lexlevel) {
            /*
             * Name exists at current scope - check if it's a
             * function prototype
             */
            /*
             * A name can reach here with no type when an earlier
             * declaration was malformed - the entry was made and
             * then abandoned.  Treat it as a redeclaration rather
             * than dereferencing the missing type.
             */
            if (existing->type &&
                (existing->type->flags & TF_FUNC) && !existing->u.locals) {
                /* Reuse existing function declaration (prototype) */
                nm = existing;
                redeclOld = existing->type;
                /*
                 * Update type to the new one (definition may have
                 * full param list)
                 */
                /* But keep the existing name structure */
            } else if (lexlevel == 1 && existing->type &&
                       !existing->is_tag && existing->kind == kvar) {
                /*
                 * A file-scope variable declared again.  C allows it:
                 * one of them may say how big it is and the other need
                 * not, which is how an array is declared ahead of the
                 * definition that sizes it -
                 *
                 *	char foo[];
                 *	char foo[20];
                 *
                 * Reuse the entry as the function case above does and
                 * let the declarator finish; the two types are weighed
                 * against each other at the end, where both are known.
                 * Before this the second declaration made a second
                 * name and the tree said "dup name".
                 */
                nm = existing;
                redeclOld = existing->type;
            } else {
                /* Not a redeclaration we know how to merge */
                nm = newName(cur.v.id, kvar, prefix, 0);
            }
        } else if (existing && existing->level < lexlevel) {
            /*
             * Name exists at outer scope - this is shadowing.
             * Assign static_id so cc2 can distinguish variables.
             * Emitted as L<id> (not S<id> which is for statics).
             */
            nm = newName(cur.v.id, kvar, prefix, 0);
            nm->static_id = ++shadowCtr;
        } else {
            /* New name - create it */
            nm = newName(cur.v.id, kvar, prefix, 0);
            /*
             * Anything declared inside a nested block gets a
             * distinct name too, shadowing or not.  All of a
             * function's locals are hoisted into one list, and two
             * sibling blocks each declaring "b" would arrive there
             * as the same name - pass2 binds by name and would give
             * both the first one's slot.
             */
            if (lexlevel > 2 && !nm->is_tag)
                nm->static_id = ++shadowCtr;
        }
    }
    if (nm && lexlevel >= 2)
        nm->w.r.blkid = curblk();
    gettoken();

    if (cur.type == COLON) {    // check for bitfield
        gettoken();
        if (cur.type != NUMBER && cur.type != LNUMBER) {
            gripe(ER_D_BD);
        } else if (cur.v.numeric > MAXBITS) {
            gripe(ER_D_BM);
        } else {
            nm->kind = kbitfield;
            nm->w.m.width = cur.v.numeric;
        }
        gettoken();
    }
    return nm;
}

/*
 * A function-pointer parameter, "(*name)(args)", its opening LPAR
 * consumed.  Returns the finished parameter type; the name, if one
 * was given, lands in *namep.
 */
struct type *
prmFnPtr(struct type *param_type, unsigned short *namep)
{
    if (cur.type == STAR) {
        // (*) or (*name) - pointer to function
        gettoken();
        // Optional name inside (*)
        if (cur.type == SYM) {
            *namep = cur.v.id;
            gettoken();
        }
        expect(RPAR, ER_D_FA);
        // Now parse function parameter list
        if (cur.type == LPAR) {
            struct type *fn_type;
            struct name *inner_arg, *inner_tail;
            fn_type = (struct type *)permalloc(sizeof(*fn_type));
            fn_type->flags = TF_FUNC;
            fn_type->sub = param_type;  // return type
            inner_tail = NULL;
            gettoken();  // consume (
            // Parse inner function's parameters
            while (cur.type != RPAR && cur.type != E_O_F) {
                struct type *inner_base, *inner_type;
                inner_base = getbasetype();
                if (!inner_base) {
                    if (cur.type == COMMA) {
                        gettoken();
                        continue;
                    }
                    break;
                }
                inner_type = parsePtrPfx(inner_base);
                // Skip optional inner parameter name
                if (cur.type == SYM)
                    gettoken();
                inner_arg = createPrmEnt(0, inner_type);
                inner_arg->next = NULL;
                if (inner_tail)
                    inner_tail->next = inner_arg;
                else
                    fn_type->elem = inner_arg;
                inner_tail = inner_arg;
                if (cur.type == COMMA) {
                    gettoken();
                    continue;
                }
                break;
            }
            expect(RPAR, ER_D_FA);
            // Result: pointer to function type
            param_type = getType(TF_POINTER, fn_type, 0);
        } else {
            // Just (*) without function params - pointer type
            param_type = getType(TF_POINTER, param_type, 0);
        }
    } else {
        // Unexpected token after ( - try to recover
        gripe(ER_D_FA);
        // Skip to matching )
        while (cur.type != RPAR && cur.type != E_O_F)
            gettoken();
        if (cur.type == RPAR)
            gettoken();
    }
    return param_type;
}

/*
 * One parameter declaration.  Returns the entry, or NULL when the
 * list is malformed and the caller should stop.
 */
struct name *
prmDecl(void)
{
    unsigned short nameid;
    struct type *basetype, *param_type;
    unsigned char psclass;
    struct name *arg;

    nameid = 0;

    // ANSI style: parse full type + declarator
    /*
     * register is the one storage class that means something
     * on a parameter, and it used to be consumed and thrown
     * away right here - the keyword parsed, the allocator
     * never heard about it.
     */
    psclass = parseSclass();

    /*
     * K&R implicit int, after a storage class - the same rule decl()
     * applies at file scope, at the one other place a storage class
     * can arrive with no type behind it:
     *
     *	f(a) register a; { }		f(a, b) register a; char *b; { }
     *
     * cpp's K&R pass turns those into "int f(register a)", which is
     * the ANSI spelling of the same thing and is what arrives here.
     * Without this the parameter had no base type, the list was
     * called malformed, and the parse gave up at the top of the
     * function - so a file lost every diagnostic after its first such
     * definition and reported thirty cascades instead.
     *
     * The test is what follows rather than what is missing, exactly
     * as at file scope: a declarator starts with a name, a star or a
     * parenthesis, and any of those means the type was left out.
     * Anything else is getbasetype()'s business.
     */
    basetype = 0;
    if (psclass && (cur.type == SYM || cur.type == STAR ||
        cur.type == LPAR))
        basetype = inttype;
    if (!basetype)
        basetype = getbasetype();
    if (!basetype) {
        gripe(ER_D_FA);
        return NULL;
    }

    // Parse pointer prefix
    param_type = parsePtrPfx(basetype);

    // Handle function pointer: type (*)(args) or type (*name)(args)
    if (cur.type == LPAR) {
        gettoken();
        param_type = prmFnPtr(param_type, &nameid);
    } else {
        /* get param name */
        if (cur.type == SYM) {
            nameid = cur.v.id;
            gettoken();
        }
    }

    // Handle array suffix (converts to pointer)
    if (cur.type == LBRACK) {
        struct expr *sz;
        gettoken();
        if (cur.type != RBRACK) {
            /* Array size (ignored for parameters) */
            sz = parseExpr(0);
            if (sz) FreeExpr(sz);
        }
        expect(RBRACK, ER_D_FA);
        /*
         * "T a[]" as a parameter is "T *a", so the decay ADDS a
         * pointer to whatever the declarator has built.  Taking sub
         * when there was one took a level OFF instead, so
         *
         *	int f(char *a[])
         *
         * came out as "char *a" rather than "char **a" - the same type
         * as "char a[]", and the tree said so: *a was a DEREF of width
         * byte, loading one byte of a two byte pointer.  Nothing here
         * is ever an array (parsePtrPfx only builds pointers, and a
         * parenthesised declarator went to prmFnPtr above), so there
         * is no array level to unwrap and sub was never the element
         * type - it was the thing the element points AT.
         *
         * It hid because the two widths agree for "int *a[]", which is
         * the spelling anyone tries first: pointer-to-int and
         * pointer-to-pointer are both two bytes, so only char and long
         * showed it.
         */
        param_type = getType(TF_POINTER, param_type, 0);
    }

    // Create parameter entry for type->elem with actual name
    arg = createPrmEnt(nameid, param_type);
    if (psclass & SC_REGISTER)
        arg->sclass = SC_REGISTER;
    return arg;
}

/*
 * The parameter list of a fresh declaration, cur just past the
 * opening LPAR.  Builds and returns the function type.
 */
struct type *
fnParams(struct type *prefix)
{
    struct type *suffix;
    struct name *arg, *param_tail;

    // Create a new function type (don't use getType() which caches types)
    // Function types need unique instances because we modify elem list
    suffix = (struct type *)permalloc(sizeof(*suffix));
    suffix->flags = TF_FUNC;
    suffix->sub = prefix ? prefix : inttype;

    param_tail = NULL;
    while (cur.type != RPAR && cur.type != E_O_F) {
        // Check for variadic ...
        if (cur.type == ELLIPSIS) {
            gettoken();
            suffix->flags |= TF_VARIADIC;
            break;  // exit parameter loop
        }
        arg = prmDecl();
        if (!arg)
            break;
        arg->next = NULL;
        if (param_tail) {
            param_tail->next = arg;
        } else {
            suffix->elem = arg;
        }
        param_tail = arg;

        // Handle comma or end of list
        if (cur.type == COMMA) {
            gettoken();
            continue;
        }
        if (cur.type != RPAR) {
            gripe(ER_D_FA);
            break;
        }
    }
    expect(RPAR, ER_D_FA);
    return suffix;
}

/*
 * Phase 2 never attaches the suffix (see "suffix && phase == 1" in
 * declare()): the name entry keeps its phase 1 type.  Building the
 * function type and its parameter entries again would only leak
 * them, so just consume the parameter list and move on.
 *
 * Except for a local function pointer.  "(*fp)()" in a body is
 * created afresh here - locals do not survive phase 1 - so it
 * arrives holding only the pointer that "(*fp)" made, with nothing
 * under it.  Without the function type that pointer points at
 * nothing, and every use that consults the type is wrong: the call
 * still works, because a call does not look, while "(*fp)()"
 * dereferences one time too many because it does.
 *
 * A reused entry already has its phase 1 type and is left alone -
 * that is what the pointer-with-no-target test distinguishes.  The
 * parameters are not wanted either way, which is the leak the skip
 * exists to avoid; only the type of the thing pointed at is.
 */
struct type *
skipParams(struct name *nm, struct type *prefix)
{
    struct type *suffix = NULL;
    unsigned char pdepth = 1;

    /*
     * The same is true of a function DECLARED in a body - "extern
     * short _pnum(), _fnum();", K&R's way of saying a routine does
     * not return int.  It is created afresh here too, so there is no
     * phase 1 type to keep and the bare return type is all it has.  A
     * short does not decay, so the reference came out as DEREF(SYM):
     * the call loaded the first two bytes of the routine's own code
     * and jumped through them.  doprnt declares _pnum this way, which
     * is what took printf("%d") off into the weeds.
     *
     * A pointer counts, and the two pointer shapes are told apart by
     * what is under it, not here: "(*fp)()" arrives as a pointer to
     * nothing and declare() turns this suffix into pointer-to-
     * function, while "char *malloc()" arrives as a pointer to char
     * and becomes a function returning it.  qsort declares malloc
     * that way, so it called whatever the first two bytes of malloc
     * pointed at and sorted nothing.
     *
     * Inside a body only.  At file scope the entry a definition
     * reuses is not always the one that carries the function type,
     * and building a fresh one here turned "short (*gfp)()" into a
     * function rather than a pointer to one - so assigning to it was
     * no longer assigning to an object at all.
     */
    /*
     * The exception to the file-scope rule above is a name phase 1
     * threw away.  A declaration nothing refers to is not kept (see
     * declaration()), so there is no phase 1 entry for phase 2 to
     * reuse and the worry that stops us building one here - that the
     * entry a definition reuses may not be the one carrying the
     * function type - cannot arise: nothing refers to this name, so
     * nothing reuses it either.  Without this the fresh entry keeps
     * the bare return type, and "char *fopen();" in a file that never
     * calls fopen was emitted as two bytes of bss, defining in every
     * object what it was only declaring.
     */
    /*
     * An ARRAY of them - "int (*tab[2])();" - is the same shape with a
     * dimension in the middle: the element is the pointer to nothing.
     * The array-ness made it fail both tests above, so phase 2 built
     * no suffix, the local kept "array of pointer to nothing", and
     * "(*tab[0])()" was a deref of something pfxStar could not tell
     * was a function pointer.  It added a load, and the call went to
     * whatever the first two bytes of the function held.  A global was
     * fine - it keeps its phase 1 type - so this only bit locals, and
     * it bit them silently.
     */
    if (nm->type &&
        (((nm->type->flags & TF_POINTER) && !nm->type->sub) ||
         ((nm->type->flags & TF_ARRAY) && nm->type->sub &&
          (nm->type->sub->flags & TF_POINTER) && !nm->type->sub->sub) ||
         ((lexlevel > 1 || idOnce(nm->id)) &&
          !(nm->type->flags & (TF_FUNC | TF_ARRAY))))) {
        suffix = (struct type *)permalloc(sizeof(*suffix));
        suffix->flags = TF_FUNC;
        suffix->sub = prefix ? prefix : inttype;
    }
    while (pdepth && cur.type != E_O_F) {
        if (cur.type == LPAR)
            pdepth++;
        else if (cur.type == RPAR)
            pdepth--;
        if (pdepth)
            gettoken();
    }
    expect(RPAR, ER_D_FA);
    return suffix;
}

/*
 * Parse a complete C declarator and create symbol table entry
 *
 * This is the core declaration parser that handles all C declarator syntax:
 * pointers, arrays, functions, and their combinations with proper precedence.
 * It creates name entries in the symbol table (or struct members) with
 * complete type information.
 *
 * Declarator grammar (simplified):
 *   declarator = prefix_declarator suffix_declarator*
 *   prefix_declarator = '*'* ('(' declarator ')' | identifier)
 *   suffix_declarator = '[' const_expr? ']' | '(' params ')'
 *
 * Precedence rules (postfix binds tighter than prefix):
 *   - int *p[10]    -> array of 10 pointers to int
 *   - int (*p)[10]  -> pointer to array of 10 ints
 *   - int *f()      -> function returning pointer to int
 *   - int (*f)()    -> pointer to function returning int
 *
 * Parameter modes:
 *   btp         - Base type pointer (in/out parameter)
 *   struct_elem - If true, creates struct member (not added to names[])
 *
 * Type assembly:
 *   - Base type comes from caller (int, char, struct foo, etc.)
 *   - Prefix modifiers (pointers) wrap base type
 *   - Suffix modifiers (arrays, functions) become new outer type
 *   - Final type attached to name entry
 *
 * Function parameter handling:
 *   - ANSI style only (K&R style is converted by cpp preprocessor)
 *   - Parameters stored in type->elem as linked list
 *   - Variadic functions detected (...) and marked with TF_VARIADIC
 *
 * Array handling:
 *   - Size can be omitted: int arr[] (size -1, completed later)
 *   - Arrays get both TF_ARRAY and TF_POINTER for decay semantics
 *   - Multi-dimensional arrays supported: int m[10][20]
 *
 * Struct member mode:
 *   - struct_elem=1: Creates name but doesn't add to names[] array
 *   - Avoids polluting global namespace with member names
 *   - Members linked via next pointer in struct type's elem list
 *
 * Bitfield support:
 *   - Detected by ':' after identifier: unsigned flags : 3;
 *   - Width stored in name->w.m.width field
 *   - Kind changed to bitfield
 *
 * Redeclaration handling:
 *   - Function forward declarations: Reuses existing name entry
 *   - Other redeclarations: Creates new entry (error reported elsewhere)
 *
 * Parameters:
 *   btp         - Pointer to base type (can be NULL, updated if type seen)
 *   struct_elem - 1 for struct members, 0 for normal variables/functions
 *
 * Returns:
 *   Name entry with complete type, or NULL on error
 *
 * Side effects:
 *   - May update *btp if type keywords encountered
 *   - Consumes declarator tokens from input stream
 *   - Creates name entry (added to names[] unless struct_elem=1)
 *   - For functions: allocates parameter name entries
 */
struct name *
declare(struct type **btp, unsigned char struct_elem)
{
    struct name *nm;
    struct type *t, *prefix, *suffix, *rt;
    unsigned long i;
    unsigned long dims[MAXDIM];
    unsigned char ndim;
    /*
     * Set when this level read a parenthesised declarator.  It is what
     * tells "(*p)[8]" from "*p[8]": the brackets belong to a different
     * level in each, and the anonymous pointer they meet looks the
     * same either way.  See the pointer-to-array case below.
     */
    unsigned char paren;

    paren = 0;
    suffix = 0;

    nm = 0;

    /*
     * this will be primitive, enum, struct/union
     */
    t = getbasetype();
    if (t && *btp) {
        gripe(ER_T_DT);
        t = 0;
    }
    if (t) {
        *btp = t;
    }
    prefix = *btp;

    prefix = parsePtrPfx(prefix);

    // parenthesed type definition does precedence
    if (cur.type == LPAR) {
        paren = 1;
        gettoken();
        rt = 0;
        nm = declare(&rt, struct_elem);       // recurse
        expect(RPAR, ER_D_DP);
        if (*btp && rt) {
            gripe(ER_T_DT);
            rt = 0;
        }
        if (rt && !nm) {
            *btp = rt;
        }
    }

    if (cur.type == RPAR) {
        if (!nm) {
            for (t = prefix; t->sub; t = t->sub) {
                t->sub = *btp;
                *btp = prefix;
            }
        }
        return nm;
    }

    if (cur.type == SYM) {      // symbol name
        if (nm) {
            gripe(ER_D_MV);
        }
        nm = symDecl(prefix, struct_elem);
    }

    /*
     * Dimensions are read left to right and the type has to be built
     * from the right: "char a[3][4]" is 3 arrays of 4 chars, so the
     * LAST bracket is the innermost type and the first is the
     * outermost.
     *
     * Wrapping each new dimension around the one before built it the
     * other way up - 4 arrays of 3.  The total is the same, which is
     * why sizeof(a) was right and nothing complained, but sizeof(a[0])
     * came back as the outer dimension and every row address was
     * scaled by it.  a[1] was three bytes along a four-byte row.
     */
    ndim = 0;
    while (cur.type == LBRACK) {        // array
        gettoken();
        if (cur.type == RBRACK) {
            i = -1;
        } else {
            parseConst(RBRACK);
            i = constVal;
        }
        if (ndim < MAXDIM)
            dims[ndim++] = i;
        else
            gripe(ER_D_AD);
        expect(RBRACK, ER_D_AD);
    }
    while (ndim) {
        /*
         * Arrays have both TF_ARRAY and TF_POINTER flags for array decay
         * semantics
         */
        prefix = getType(TF_ARRAY|TF_POINTER, prefix, dims[--ndim]);
        /* Store array type in suffix so it gets assigned to nm->type */
        suffix = prefix;
    }

    if (cur.type == LPAR) {     // ( <func_arg>[,<func_arg>]*. )
        gettoken();
        if (suffix) {
            gripe(ER_D_FA);
            suffix = 0;
        }
        if (phase == 2)
            suffix = skipParams(nm, prefix);
        else
            suffix = fnParams(prefix);
    }

    if ((cur.type != ASSIGN) && (cur.type != BEGIN) &&
        (cur.type != COMMA) && (cur.type != SEMI) && (cur.type != RPAR)) {
#ifdef DEBUG
        fprintf(stderr, "token: %d $%c$\n", cur.type, cur.type);
#endif
        gripe(ER_D_UT);
        nm = 0;
    }
    if (!nm) {
        return 0;
    }

    /*
     * Handle suffix types (function or array) and connect to nm
     *
     * For function pointers like void (*fp)(args):
     *   - nm->type is pointer with sub=NULL (from recursion with (*fp))
     *   - suffix is function type
     *   - Need: pointer -> function type
     *   - Create new pointer type (don't modify cached type)
     *
     * For regular functions like void foo(args):
     *   - nm->type is the base type (void)
     *   - suffix is function type
     *   - Need: nm->type = suffix (function type, with sub=return type)
     */
    if (suffix && (phase == 1 ||
        ((suffix->flags & TF_ARRAY) &&
         !(nm->type && (nm->type->flags & TF_ARRAY))) ||
        ((suffix->flags & TF_FUNC) && nm->type &&
         (nm->type->flags & TF_POINTER) && !nm->type->sub) ||
        /* and the same behind a dimension: int (*tab[2])() */
        ((suffix->flags & TF_FUNC) && nm->type &&
         (nm->type->flags & TF_ARRAY) && nm->type->sub &&
         (nm->type->sub->flags & TF_POINTER) && !nm->type->sub->sub) ||
        /*
         * A function DECLARED inside a body - "extern short _pnum(),
         * _fnum();", K&R's way of saying a routine does not return
         * int.  Locals are freed after phase 1 and re-created in
         * phase 2 from the base type alone, so the "()" was dropped
         * and the name came back as a plain short.  A short does not
         * decay, so the reference became DEREF(SYM) and the call
         * loaded the first two bytes of the routine's own code and
         * jumped through them.  Same shape as the array case above,
         * and the same fix; only where the name carries no
         * function-ness of its own, which is what the phase 2
         * caution below is about.
         */
        /*
         * ...but NOT a name that is already a pointer to a function.
         * That carries no TF_FUNC of its own either - the func-ness is
         * one level down, in the sub - so it answered this test and
         * fell through to the plain "nm->type = suffix" below, which
         * replaced pointer-to-function with the bare function type.
         *
         * A file-scope declarator is walked twice, and the two walks
         * see different things: the first meets the anonymous pointer
         * left by "(*fp)" and wraps the "()" suffix into it correctly,
         * the second meets the finished pointer-to-function and
         * flattened it.  Then the redeclaration check compared the two
         * and said so, which is why
         *
         *	int (*fp)();
         *
         * on a line by itself - no second declaration anywhere - was
         * "redecl disagrees".  extern, a local, a parameter and an
         * array of them all took other paths and were fine.
         */
        ((suffix->flags & TF_FUNC) && nm->type &&
         !(nm->type->flags & (TF_FUNC | TF_ARRAY)) &&
         !((nm->type->flags & TF_POINTER) && nm->type->sub &&
           (nm->type->sub->flags & TF_FUNC))))) {
        /*
         * Function suffixes are only applied in phase 1: in phase 2 a
         * reused nm already has its type correctly set, and re-applying
         * would overwrite pointer-to-function with just the function
         * type.  Array suffixes must be applied in phase 2 as well when
         * the name doesn't already carry one: locals are freed after
         * phase 1, so phase 2 re-creates them with only the base type
         * and would lose the array-ness.  Reused entries (globals) keep
         * their phase 1 type - it has sizes inferred from initializers.
         *
         * A local function pointer falls between these and is still
         * wrong: "short (*fp)();" inside a body reaches here with the
         * "()" suffix in phase 1 and without it in phase 2, so the
         * phase 2 entry keeps the bare pointer from "(*fp)" and never
         * learns what it points at.  Calling it works - the call does
         * not consult the type - but "(*fp)()" derefs one time too
         * many, and at file scope or through a typedef both spellings
         * are fine.  The suffix is missing before this point rather
         * than being rejected here; adding a case for it here has no
         * effect.
         */
        if (nm->type && (nm->type->flags & TF_POINTER) &&
            !(nm->type->flags & TF_ARRAY) && (suffix->flags & TF_FUNC) &&
            !nm->type->sub && suffix->sub != nm->type) {
            /*
             * Function pointer: create proper pointer-to-function type.
             * This case arises from recursive parsing of (*fp)(args) where
             * nm->type is an anonymous pointer (sub=NULL) from parsing (*fp)
             * and the (args) suffix was parsed at THIS outer level.
             * (When suffix->sub == nm->type the suffix was built in the
             * same level as the `*` - that's `*name(args)` inside parens,
             * a function returning a pointer, handled below.)
             * For functions returning pointers like int *func(), nm->type->sub
             * is the base type (int), so we just use suffix directly.
             */
            nm->type = getType(TF_POINTER, suffix, 0);
        } else if (paren && nm->type && (nm->type->flags & TF_POINTER) &&
                   !(nm->type->flags & TF_ARRAY) &&
                   (suffix->flags & TF_ARRAY) && !nm->type->sub) {
            /*
             * A pointer to an ARRAY, "char (*buffer)[512]".  The same
             * shape as the function pointer just above and it needs
             * the same answer: the recursion on "(*buffer)" left an
             * anonymous pointer with no sub, and this level's "[512]"
             * is what it points at.
             *
             * ONLY when this level read the parentheses, which is what
             * "paren" is for.  Without that test it also caught the
             * inner half of "unsigned (*slist[8])()" - an array of
             * eight function pointers, which is what proc.h keeps its
             * signal dispositions in.  There the "[8]" is at the same
             * level as the "*" and means an array OF pointers; here
             * the brackets are outside the parentheses and mean a
             * pointer TO an array.  The anonymous pointer looks
             * identical in both, so the brackets' level is the only
             * thing that separates them.
             *
             * Without this it fell to the plain "nm->type = suffix"
             * at the bottom and the whole declaration came out as a
             * bare array of 512 char.  An array is not an lvalue, so
             * the symptom was not a wrong type but a rejected
             * program:
             *
             *  char (*buffer)[512];
             *  buffer = 0;             "need lvalue"
             *  buffer = &blist[n];     "need lvalue"
             *  buffer[1][2]            "bad index"
             *
             * even for an assignment as plain as "= 0".  The comment
             * on the array-of-function-pointers case below says the
             * quiet version of this is the worse one; here it is the
             * other way round, and the kernel's main.c has not
             * compiled because of it - binit() keeps its buffer pool
             * as exactly this type.
             *
             * Rebuilt rather than having its sub plugged, for the
             * reason given below: an array type comes from getType
             * and is shared, so writing through it would retype every
             * other array of the same shape.
             */
            nm->type = getType(TF_POINTER, suffix, 0);
        } else if (nm->type && (nm->type->flags & TF_ARRAY) &&
                   (suffix->flags & TF_FUNC) && nm->type->sub &&
                   (nm->type->sub->flags & TF_POINTER) &&
                   !(nm->type->sub->flags & TF_ARRAY) &&
                   !nm->type->sub->sub) {
            /*
             * An ARRAY of function pointers, "int (*tab[2])()".  The
             * same shape as the scalar above with a dimension in the
             * middle: the recursion read "(*tab[2])" and produced an
             * array of the ANONYMOUS pointer, and this level's "()" is
             * what that pointer points at.
             *
             * Rebuilding rather than plugging sub, because an array
             * type comes from getType and is shared - writing through
             * it would retype every other array of the same shape.
             *
             * Without this the array fell past every case here to the
             * plain "nm->type = suffix" below and the whole
             * declaration became a bare function type.  At file scope
             * that then disagreed with itself on the second walk and
             * said "redecl disagrees"; as a local it was quieter and
             * worse - the type said "array of pointer to int", so
             * "(*tab[0])()" derefed the function's address as though
             * it were a pointer variable, loaded the first two bytes
             * of the function's own code and called THAT.  It compiled
             * clean and ran away.
             */
            nm->type = getType(TF_ARRAY | TF_POINTER,
                               getType(TF_POINTER, suffix, 0),
                               nm->type->count);
        } else if (paren && nm->type && (nm->type->flags & TF_POINTER) &&
                   !(nm->type->flags & TF_ARRAY) &&
                   (suffix->flags & TF_ARRAY) && nm->type->sub &&
                   (nm->type->sub->flags & TF_ARRAY)) {
            /*
             * The second walk of a pointer to an array, which already
             * has the type the first walk gave it.  Leave it alone.
             *
             * A file-scope declarator is read twice and the two walks
             * meet different things: the first the anonymous pointer
             * from "(*buffer)", the second the finished
             * pointer-to-array.  That carries no TF_ARRAY of its own -
             * the array-ness is one level down in the sub - so it
             * reaches here and, with nothing to catch it, fell to the
             * plain "nm->type = suffix" at the bottom and was
             * flattened back to a bare array.  Then the two walks
             * disagreed and said so.
             *
             * Answered here and not by refusing entry above, because
             * the condition up there is shared with the array of
             * function pointers - "unsigned (*slist[8])()", which is
             * what proc.h keeps signal dispositions in - and tightening
             * it broke that instead.  Doing nothing in the right case
             * is cheaper than deciding not to arrive.
             */
            ;
        } else if (nm->type && (nm->type->flags & TF_FUNC) &&
                   nm->type->sub && (nm->type->sub->flags & TF_POINTER) &&
                   !(nm->type->sub->flags & TF_ARRAY) &&
                   !nm->type->sub->sub && (suffix->flags & TF_FUNC)) {
            /*
             * Function returning a function pointer:
             * void (*signal(args))(args2).  The recursion produced
             * fn(args) returning an incomplete pointer; this level's
             * (args2) completes it: return type = ptr -> fn(args2).
             * nm->type is a unique permalloc'd function type, so
             * plugging its sub is safe (never a cached type).
             */
            nm->type->sub = getType(TF_POINTER, suffix, 0);
        } else {
            nm->type = suffix;
        }
    }

    /*
     * A function redeclared as returning something else.
     *
     * An unknown name called as a function is an int-returning one -
     * that is the language, and pfxSym says so.  What is not allowed
     * is for a later declaration to disagree, and nothing checked:
     * the entry was reused and its type quietly overwritten, so
     *
     *	caller() { return f() + 1; }	implies int f()
     *	long f() { ... }
     *
     * compiled without a word, the caller reading a long return as an
     * int and taking HL for the whole of it.
     *
     * Return type only.  compatFnTyp compares parameter lists too and
     * is too strict to use here: a K&R declaration names no parameters
     * - "extern short adds();" ahead of "short adds(a, b)" - and the
     * two are meant to agree.  It is the return type that decides how
     * a call site reads the answer, and the return type that was being
     * silently replaced.
     */
    if (redeclOld && nm->type) {
        if ((nm->type->flags & TF_FUNC) && (redeclOld->flags & TF_FUNC)) {
            if (!sameRet(redeclOld->sub, nm->type->sub))
                gripe(ER_D_RD);
        } else if ((nm->type->flags & TF_ARRAY) &&
                   (redeclOld->flags & TF_ARRAY)) {
            /*
             * Two spellings of the same array.  They have to agree
             * about the element; about the extent, either may be
             * silent, and a stated one wins.  Two different stated
             * ones do not agree at all.
             */
            if (redeclOld->sub != nm->type->sub)
                gripe(ER_D_RD);
            else if (nm->type->count <= 0 && redeclOld->count > 0)
                nm->type = redeclOld;
            else if (nm->type->count > 0 && redeclOld->count > 0 &&
                     nm->type->count != redeclOld->count)
                gripe(ER_D_RD);
        } else if (redeclOld != nm->type) {
            gripe(ER_D_RD);
        }
    }

    return nm;
}

/*
 * Detect if current token starts a type cast expression
 *
 * Distinguishes between type casts and parenthesized expressions when
 * parsing '(' token. This is critical for correctly parsing cast expressions
 * vs. grouped expressions:
 *
 * Disambiguation examples:
 *   - (int)x      -> type cast (starts with type keyword)
 *   - (foo)x      -> type cast if foo is typedef, expression otherwise
 *   - (x + y)     -> parenthesized expression (starts with identifier/expression)
 *   - (int*)p     -> type cast (starts with type keyword)
 *
 * Detection strategy:
 *   1. Check for C type keywords (int, char, struct, etc.)
 *   2. Check if SYM token is a typedef name in symbol table
 *
 * Typedef handling:
 *   - Looks up identifier in names[] array
 *   - Checks if kind is tdef (typedef)
 *   - Enables casts with user-defined type names: (foo_t)x
 *
 * Used by:
 *   - Expression parser when encountering '(' to decide parse path
 *   - Cast expression parsing (parseTypeName)
 *
 * Returns:
 *   1 if current token could start a type cast
 *   0 if current token starts a parenthesized expression
 */
char
isCastStart(void)
{
    /* Check for type keywords */
    if (isTypeToken(cur.type)) {
        return 1;
    }

    /* typedef names cannot reach here: cpp dissolves them */
    return 0;
}

/*
 * Parse type name in cast expression without requiring identifier
 *
 * Parses abstract type names used in cast expressions, sizeof, and other
 * contexts where a type is specified without a variable name. This is
 * distinct from normal declarations which require an identifier.
 *
 * Abstract declarators supported:
 *   - Simple types: int, char, struct foo
 *   - Pointer types: char *, int **, void ***
 *   - Typedef names: size_t, my_type_t
 *
 * Abstract declarators NOT YET supported:
 *   - Arrays: int[], int[10]
 *   - Function pointers: int (*)(void), void (*)(int, int)
 *   - Complex combinations: int (*[])(void)
 *
 * TODO: Full abstract declarator support would require:
 *   - Parsing (*) for pointer-to-array/function
 *   - Parsing [] and () without identifiers
 *   - Proper precedence handling
 *
 * Current limitations:
 *   - Only handles base types and pointer prefixes
 *   - No array dimensions in casts: (int[])x doesn't work
 *   - No function pointer casts: (int(*)(void))x doesn't work
 *
 * Default type:
 *   - If no type specified, defaults to int
 *   - Enables implicit int casts (rare in practice)
 *
 * Examples:
 *   (int)x           -> parses "int"
 *   (char *)p        -> parses "char *" (pointer to char)
 *   (unsigned long)v -> parses "unsigned long"
 *   (foo_t)x         -> parses typedef "foo_t"
 *
 * Parameters:
 *   None (reads from current token stream)
 *
 * Returns:
 *   Type structure representing the parsed type name
 *
 * Side effects:
 *   - Consumes type tokens from input stream
 *   - Does NOT consume closing ')' of cast
 */
struct type *
parseTypeName(void)
{
    struct type *base_type, *result_type;

    /* Parse base type (int, char, struct foo, typedef, etc.) */
    base_type = getbasetype();
    if (!base_type) {
        /* No type specified - default to int */
        base_type = inttype;
    }

    /* Parse pointer prefix (*, **, etc.) */
    result_type = parsePtrPfx(base_type);

    /* Abstract function-pointer declarator: (*)(args) - as in the
     * SIG_IGN cast (void (*)(int))1.  The parameter types are only
     * consumed; calls through the pointer don't need them. */
    if (cur.type == LPAR) {
        gettoken();
        if (cur.type == STAR) {
            struct type *fn;
            unsigned long adims[MAXDIM];
            unsigned char andim;
            unsigned long av;

            gettoken();
            while (cur.type == STAR)    /* (**)() etc. */
                gettoken();
            expect(RPAR, ER_D_DP);
            /*
             * "(*)[n]" - a pointer to an ARRAY, which is the cast the
             * kernel's main.c needs for its buffer pool:
             *
             *  char (*buffer)[512];
             *  buffer = (char (*)[512]) &blist[nbuf];
             *
             * Only "(*)(args)" was read here, so a "[" after the
             * parentheses was left in the input and the type came out
             * as a pointer to a FUNCTION returning char.  The caller
             * then met the brackets it had not consumed and said
             * "expr paren" against the line the type was declared on,
             * which is a long way from where the trouble is.
             *
             * The dimensions are wrapped from the innermost out, the
             * same way declare() does it: "[3][4]" is 3 arrays of 4,
             * so the LAST bracket is the innermost type.
             */
            if (cur.type == LBRACK) {
                andim = 0;
                while (cur.type == LBRACK) {
                    gettoken();
                    if (cur.type == RBRACK) {
                        av = -1;
                    } else {
                        parseConst(RBRACK);
                        av = constVal;
                    }
                    if (andim < MAXDIM)
                        adims[andim++] = av;
                    else
                        gripe(ER_D_AD);
                    expect(RBRACK, ER_D_AD);
                }
                while (andim)
                    result_type = getType(TF_ARRAY | TF_POINTER,
                                          result_type, adims[--andim]);
                return getType(TF_POINTER, result_type, 0);
            }
            fn = (struct type *)permalloc(sizeof(*fn));
            fn->flags = TF_FUNC;
            fn->sub = result_type;      /* return type */
            if (cur.type == LPAR) {
                unsigned char d = 1;
                gettoken();
                while (d && cur.type != E_O_F) {
                    if (cur.type == LPAR)
                        d++;
                    else if (cur.type == RPAR)
                        d--;
                    if (d)
                        gettoken();
                }
                expect(RPAR, ER_D_DP);
            }
            result_type = getType(TF_POINTER, fn, 0);
        } else {
            gripe(ER_D_DP);
        }
    }

    return result_type;
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
