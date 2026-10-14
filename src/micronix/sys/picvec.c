/*
 * The 8259 vector-table relocation, shared by the three PIC objects.
 *
 * sys/picvec.c
 *
 * Every 8259's vector table is copied down to its 32-byte boundary before
 * the ICWs name it; the three PIC objects all do the same move, so it
 * lives here once.  It runs at boot only, so it folds into highmem.o and
 * is reclaimed as buffer headers.
 */
#include <types.h>

/*
 * Copy a vector table down to its 32-byte boundary and return that
 * boundary.  The table sits with slack above it so there is somewhere to
 * go; an entry is a jp to an absolute address, which the copy does not
 * disturb.
 */
vecalign(src)
    char *src;
{
    register int base, i;
    register char *bp, *sp;

    base = (int) src & ~0x1f;
    bp = (char *) base;
    sp = src;
    for (i = 0; i < 32; i++)
        bp[i] = sp[i];
    return (base);
}
