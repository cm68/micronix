/*
 * The network card's socket module header (sys/ovl.h).
 *
 * Char major 6, /dev/net0 and the sockets it clones (sys/net.c).  It is
 * a page of its own, so the socket face costs nothing when the module is
 * not placed.
 *
 * nettick() is the wakeup a card with no interrupt line needs; the line
 * itself is discovered by the attach loop from the probe named here, the
 * same way every other driver's is.
 */
#include <types.h>
#include <sys/sys.h>
#include <sys/con.h>
#include <sys/ovl.h>

extern INT16 netstart(), netprobe(), netint(), nettick();
extern struct ciovec netcvec;

struct ovlhdr nethdr = { 0, &netstart, &nettick, 0, 0, &netcvec,
	&netprobe, &netint, { 'n', 'e', 't' }, 0, 6 };

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
