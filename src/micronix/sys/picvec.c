/*
 * The 8259 vector-table relocation.
 *
 * sys/picvec.c
 *
 * Each 8259's vector table is copied to its 32-byte home in low memory -
 * the master's at 0x200, a CAS slave's at 0x220 - before the ICWs name
 * it.  It runs at boot only, so it folds into highmem.o and is reclaimed
 * as buffer headers.
 */
#include <types.h>

/*
 * Copy an 8-entry vector table to dest, the 32-byte boundary its ICW2
 * will name.  An entry is a jp to an absolute address, which the copy
 * does not disturb.
 */
vecalign(src, dest)
    char *src;
    UINT dest;
{
    register int i;
    register char *bp, *sp;

    bp = (char *) dest;
    sp = src;
    for (i = 0; i < 32; i++)
        bp[i] = sp[i];
    return (dest);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
