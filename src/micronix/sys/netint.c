/*
 * The network driver's wait event, resident.
 *
 * sys/netint.c
 *
 * The driver is part of the ND module (sys/net.c), so its code and data
 * live in one page at OVLBASE.  The event a blocked ioctl sleeps on is
 * the one piece that cannot: every module's data is addressed at
 * OVLBASE+offset, so an event in the driver's page would be the same
 * address as another module's data and wakeup() could not tell them
 * apart.  Resident bss is one address for the machine.
 */
#include <types.h>

int netchan;

/*
 * The card's interrupt line, for select(2).  The driver finds the line
 * at attach time and normally keeps it in its own page; a selector has
 * to reopen the line after the handler masks it, and select is resident.
 * 0xffff is "not attached yet": line 0 is a real line, so zero will not
 * do.
 */
UINT netline = 0xffff;
