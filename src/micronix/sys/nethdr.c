/*
 * The network card's socket module header (sys/ovl.h).
 *
 * Char major 6, /dev/net0 and the sockets it clones (sys/net.c).  This
 * is a page of its own: the card's socket face and the network disk
 * (block major 6, sys/nd.c) share a number across two namespaces and
 * nothing else, so neither costs the other page space.
 *
 * nettick() is the wakeup a card with no interrupt line needs; the line
 * itself is discovered by the attach loop from the probe named here, the
 * same way every other driver's is.
 */
#include <types.h>
#include <sys/sys.h>
#include <sys/con.h>
#include <sys/ovl.h>

extern int netstart(), netprobe(), netint(), nettick();
extern struct ciovec netcvec;

struct ovlhdr nethdr = { 0, &netstart, &nettick, 0, 0, &netcvec,
	&netprobe, &netint, { 'n', 'e', 't' }, 0, 6 };

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
