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
 * Copy a vector table to its home in low memory, 0x200 - the old map
 * image, now free.  The 8259's ICW2 names that fixed address, so there is
 * no 32-byte boundary to hunt for and no slack to leave in text.  An
 * entry is a jp to an absolute address, which the copy does not disturb.
 */
vecalign(src)
    char *src;
{
    register int i;
    register char *bp, *sp;

    bp = (char *) 0x200;
    sp = src;
    for (i = 0; i < 32; i++)
        bp[i] = sp[i];
    return (0x200);
}
