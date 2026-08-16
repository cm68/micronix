/*
 * static and global initializer parsing, split from decl.c so no
 * single translation unit carries both halves of declaration handling.
 */

#include "p1core.h"
#include "p1expr.h"
#include "p1type.h"
#include "p1name.h"
#include "p1lex.h"

extern unsigned short curFuncId;	/* outast.c */

/*
 * Parse a variable initializer for static/global variables
 *
 * Streams assembly directly without building trees. Auto initializers
 * are handled by cpp (transformed to assignment statements).
 */
void
doInitlzr(struct name *v)
{
    char fullname[32];
    char strname[16];

    gettoken(); /* consume = token */

    /*
     * A UNION initializer, in any spelling.
     *
     * K&R's book does not allow one at all - initializers are for
     * scalars, arrays and structs, and a union is none of those.  ANSI
     * later admitted the braced form for the union's FIRST member, and
     * the bare "= 0" that Whitesmiths read as "zero fill" was never C
     * anywhere.  Both were accepted here and both came out two bytes
     * long, so a 512 byte sector buffer had two bytes of storage and
     * everything declared after it was laid down inside it.
     *
     * Refused rather than sized, because there is nothing to gain by
     * accepting it: a union DECLARED and not initialized goes to bss
     * at its full size, and crt0 clears bss, so
     *
     *	union diskbuf disk0;
     *
     * already means what "= 0" was trying to say.  The gripe is in the
     * emitting phase only, so it is said once rather than once per
     * walk of the declarator.
     */
    if (phase != 1 && v && v->type && (v->type->flags & TF_UNION))
        gripe(ER_D_UI);

    /* Phase 1: skip tokens, emit string data */
    if (phase == 1) {
        if (cur.type == BEGIN) {
            /* Struct/array init - emit STRING data in phase 1 */
            unsigned char depth = 1;
            gettoken();  /* consume initial { before loop */
            while (depth > 0 && cur.type != E_O_F) {
                if (cur.type == BEGIN)
                    depth++;
                else if (cur.type == END)
                    depth--;
                else if (cur.type == STRING) {
                    /* Emit string data now for pointer-to-string fields */
                    cstring str = cur.v.str;
                    if (str) {
                        unsigned char slen = str[0];
                        fmtstr(strname, "str%d", globalStrCtr++);
                        setSeg(SEG_TEXT);
                        asmLabel(strname);
                        asmDbStr((unsigned char *)str + 1, slen);
                    }
                }
                if (depth > 0)
                    gettoken();
            }
            gettoken();  /* consume final } */
        } else if (cur.type == STRING) {
            /*
             * A string initialising a POINTER needs a literal to point
             * at.  One initialising a char array does not: phase 2
             * streams the bytes into the array itself and burns a
             * counter value where this label would have been.  Emitting
             * it anyway put a second copy of every such string in the
             * text segment with nothing referring to it - six bytes for
             * "hello", and rather more for a table of them.
             *
             * The test is streamInitVal's, spelled the same way, because
             * the two have to agree about which strings get a label.
             */
            cstring str = cur.v.str;
            if (str) {
                unsigned char slen = str[0];
                struct type *t = v ? v->type : (struct type *)0;
                int inarray = t && (t->flags & TF_ARRAY) &&
                    t->sub->size == 1;

                fmtstr(strname, "str%d", globalStrCtr++);
                if (!inarray) {
                    setSeg(SEG_TEXT);
                    asmLabel(strname);
                    asmDbStr((unsigned char *)str + 1, slen);
                }
            }
            gettoken();
        } else {
            parseExpr(15);  /* skip expression */
        }
        return;
    }

    /* Phase 2: emit assembly directly */
    setSeg(SEG_DATA);

    /* Build label: globals get ::, statics get : */
    if (v->sclass & SC_STATIC) {
        char *p = staticName(fullname, v->id,
            v->level > 1 ? curFuncId : 0,
            v->static_id);
        *p++ = ':';
        *p = 0;
    } else
        fmtstr(fullname, "_%s::", nameOf(v->id));
    asmLine(fullname);

    /* Stream initializer and fix array size if needed */
    {
        int count = streamInitVal(v->type);
        if ((v->type->flags & TF_ARRAY) && v->type->count == -1)
            v->type = getType(TF_ARRAY|TF_POINTER, v->type->sub, count);
    }
}
