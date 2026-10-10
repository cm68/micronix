/*
 * The add-on MultIO's slave 8259 bring-up, split out of cus.c so that the
 * polled and the CAS-vectored variants are the whole of what differs
 * between the two two-PIC kernels.  Both objects are built from this one
 * file - slavepic_polled.o with -DN8259_POLLED=1, slavepic_vectored.o
 * with -DN8259_POLLED=0 - and the highmem link picks one (sys/GNUmakefile).
 * A driver never sees this: it claims its line by name (INT_*), and the
 * only question this decides is how the slave's interrupt reaches the
 * master - polled out of the ISR, or CAS-vectored.
 *
 * Init-only, so it folds into highmem.o and is reclaimed as buffer
 * headers once cus() has returned - the resident kernel's text is full,
 * and the slave bring-up has no business holding a page of it.
 */
#include <types.h>
#include <sys/ints.h>

#define SBASE   0x58
#define SSELECT (SBASE + 7)
#define SPIC0   (SBASE + 4)
#define SPIC1   (SBASE + 5)
#define SPICONLY 0x08

#if N8259 == 2
/*
 * Bring the slave 8259 up: the add-on MultIO's PIC at 0x58.  The init
 * sequence is the master's own (minit, inits.s) save for the cascade:
 * ICW1 clears the single bit (there is a slave, so it is not standalone),
 * ICW3 names the slave id as the master's cascade line (CASCADE_IR).
 * Everything is masked; a driver unmasks its line with inton().
 *
 * A polled slave is never acknowledged, so its vector base is zero.  A
 * CAS-cascaded slave supplies its own vector: the three-entry table in
 * sys/slavevec.s is copied down to its 32-byte boundary here, the way
 * minit relocates the master's vectors, and ICW2/ICW1's VECL name it.
 */
int
slave_pic_init()
{
#if N8259_POLLED == 0
    extern char slave_vectors[];
    int addr = (int) slave_vectors;     /* the address, as a plain int */
    int base = addr & ~0x1f;            /* its 32-byte boundary */
    char *bp = (char *) base;
    char *src = slave_vectors;
    int i;

    for (i = 0; i < 12; i++)            /* the 3 entries, onto the boundary */
        bp[i] = src[i];
#endif

    out(SSELECT, SPICONLY);     /* select the slave PIC */
#if N8259_POLLED == 0
    out(SPIC0, 0x1d | (base & 0xe0));   /* ICW1: slave, VECL = base bits 7-5 */
    out(SPIC1, (base >> 8) & 0xff);     /* ICW2: vector base high byte */
#else
    out(SPIC0, 0x1d);           /* ICW1: level, 4-byte, slave (not single), ICW4 */
    out(SPIC1, 0x00);           /* ICW2: no vector, the slave is polled */
#endif
    out(SPIC1, CASCADE_IR);     /* ICW3: slave id = the master's cascade line */
    out(SPIC1, 0x00);           /* ICW4: normal EOI, slave, 8080 mode */
    out(SPIC1, 0xff);           /* OCW1: mask everything; inton() opens lines */
}
#endif
