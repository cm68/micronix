/*
 * we want a squeaky-clean type system
 * this compiler has an agenda to do operations in as small an integer as 
 * possible. this means that we might even get the wrong answer sometimes.  
 * we don't do the standard thing of doing word arithmetic on bytes just 
 * so we don't get overflows. that's slow and big.  don't be slow and big.
 *
 * there is no redundancy in the type tree, so two variables of the same
 * type have identical type pointers, even if one of them was declared 
 * with a typedef.
 *
 * names go out of scope, but types don't, so we don't need a ref count.
 *
 * we to add the primitive types to the global scope.
 *
 * the purpose of this is to unify basic type and typedef handling
 * example: typedef unsigned char byte 
 *          gets a name entry that points at the primitive
 * 
 * scope is handled by pushing names onto the name stack for open, 
 * and popping for close.  very simple and fast
 *
 * some example types:
 * int f          - short
 * int *f         - pointer -> short
 * int *f[4]      - array(4) -> pointer -> short
 * int f()        - function -> short
 * int *f()       - function -> pointer -> short
 * int (*f)()     - pointer -> function -> short
 * int (*f[4])()  - array(4) -> pointer -> function -> short
 * int *(*f)()    - pointer -> function -> pointer -> short
 *
 * some other interesting cases:
 * int *pi, i;
 * typedef int *pi;  pi *ppi, pi; 
 */


#include "p1core.h"
#include "p1type.h"
#include "p1name.h"
#include "p1lex.h"

/*
 * Static basic types - chained together, never freed
 */
struct type basictypes[] = {
    { 1, 0, 0, 0, 0,           0 },              // 0 _char_
    { 2, 0, 0, 0, 0,           &basictypes[0] }, // 1 _short_
    { 4, 0, 0, 0, 0,           &basictypes[1] }, // 2 _long_
    { 1, 0, 0, 0, TF_UNSIGNED, &basictypes[2] }, // 3 _uchar_
    { 2, 0, 0, 0, TF_UNSIGNED, &basictypes[3] }, // 4 _ushort_
    { 4, 0, 0, 0, TF_UNSIGNED, &basictypes[4] }, // 5 _ulong_
    { 0, 0, 0, 0, 0,           &basictypes[5] }, // 6 _void_
};
#define N_BASIC (sizeof(basictypes)/sizeof(basictypes[0]))

struct type *types = &basictypes[N_BASIC-1];
struct type *chartype = &basictypes[0];
struct type *inttype = &basictypes[1];
struct type *longtype = &basictypes[2];
struct type *uchartype = &basictypes[3];
struct type *ushorttype = &basictypes[4];
struct type *ulongtype = &basictypes[5];
struct type *voidtype = &basictypes[6];

/*
 * Static name entries for basic types - never freed
 * Chained via 'chain' field: [8]->[7]->...->[0]->NULL
 * Fields: name, type, chain - remaining fields zero by elision
 */
struct name basicnames[] = {
    { 0, &basictypes[0], 0 },
    { 0, &basictypes[1], &basicnames[0] },
    { 0, &basictypes[2], &basicnames[1] },
    { 0, &basictypes[3], &basicnames[2] },
    { 0, &basictypes[4], &basicnames[3] },
    { 0, &basictypes[5], &basicnames[4] },
    { 0, &basictypes[6], &basicnames[5] },
};

#ifdef DEBUG
/* spellings for typeName - the entries themselves have no names */
static char *basicnm[] = {
    "char", "short", "long", "uchar", "ushort", "ulong", "void",
};
#endif

struct name *names = &basicnames[6];  /* head of chain, most recent first */

#ifdef DEBUG
char *typeBitdefs[] = {
		"AGGREGATE", "INCOMPLETE", "UNSIGNED",
        "FUNC", "POINTER", "ARRAY", "?", "OLD"
};
void dumpType(struct type *t, int lv);

/*
 * Recover a printable name for a type.  Types don't carry names;
 * basic types index into basicnames[], and tags/typedefs are found
 * by scanning the symbol chain for an entry that points at t.
 */
char *
typeName(struct type *t)
{
    struct name *n;

    if (isBasicType(t))
        return basicnm[t - basictypes];
    for (n = names; n; n = n->chain)
        if (n->type == t && n->is_tag)
            return nameOf(n->id);
    return "unnamed";
}

void
dumpName(struct name *n)
{
	fdprintf(2,"dumpName: ");
	if (!n) { printf("null\n"); return; }
	fdprintf(2,"%s (%s)", nameOf(n->id), n->is_tag ? "tag" : "decl");
	if (n->sclass) {
		fdprintf(2," sclass=");
		if (n->sclass & SC_EXTERN) printf("extern ");
		if (n->sclass & SC_STATIC) printf("static ");
		if (n->sclass & SC_REGISTER) printf("register ");
		if (n->sclass & SC_AUTO) printf("auto ");
		if (n->sclass & SC_TYPEDEF) printf("typedef ");
	}
	fdprintf(2,"\n");
	dumpType(n->type, 0);
    fdprintf(2,"\toffset: %d bitoff: %d width: %d\n",
        n->w.m.offset, n->w.m.bitoff, n->w.m.width);
}

void
dumpType(struct type *t, int lv)
{
    struct name *param;
    unsigned char param_count = 0;
    int i;

    if (!t) return;
    if (lv > 20) {
        fdprintf(2,"\t... (max depth)\n");
        return;
    }
    for (i = 0; i < lv; i++) fdprintf(2,"\t");

    if (t->flags & TF_FUNC) {
        if (t->count) {
            struct farg *fa;
            for (fa = (struct farg *)t->elem; fa; fa = fa->next)
                param_count++;
        } else {
            for (param = t->elem; param; param = param->next)
                param_count++;
        }
        fdprintf(2,"function: flags %x (%s) params %d\n",
            t->flags, bitdef(t->flags, typeBitdefs), param_count);
        if (t->sub) {
            for (i = 0; i <= lv; i++) fdprintf(2,"\t");
            fdprintf(2,"returns:\n");
            dumpType(t->sub, lv + 2);
        }
    } else {
        fdprintf(2,"name %s flags %x (%s) count %x\n",
            typeName(t),
            t->flags, bitdef(t->flags, typeBitdefs), t->count);
        dumpType(t->sub, ++lv);
    }
}
#endif

/*
 * Check if a type is a static basic type (not to be freed)
 */
int
isBasicType(struct type *t)
{
    return t >= &basictypes[0] && t <= &basictypes[N_BASIC-1];
}
