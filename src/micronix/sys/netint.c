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
 *
 * netlock is the same argument for the card's command channel: one
 * command is on the wire at a time, and the sleeper on that lock must be
 * an address no module's page can alias.  netready is the readable set
 * select(2) needs, and select is resident too: the driver sets a bit
 * when an event or data is waiting for that socket.
 */
#include <types.h>
#include <sys/net.h>

int netchan;

/* one command on the card at a time (sys/net.c) */
int netlock;

/* per socket: something a reader should ask for */
UINT8 netready[8];

/*
 * The card's interrupt line, for select(2).  The driver finds the line
 * at attach time and normally keeps it in its own page; a selector has
 * to reopen the line after the handler masks it, and select is resident.
 * 0xffff is "not attached yet": line 0 is a real line, so zero will not
 * do.
 */
UINT netline = 0xffff;
