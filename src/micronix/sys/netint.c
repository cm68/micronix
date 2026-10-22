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
