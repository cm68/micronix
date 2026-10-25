/*
 * the memory device's module header (sys/ovl.h)
 *
 * sys/memhdr.c
 *
 * Major 2, /dev/mem.  It is a character device with no hardware behind
 * it and no state to point at: the read and write entries move the
 * caller's buffer through the window (sys/memdev.c), which is why there
 * is no init, probe or interrupt.
 *
 * name is a character list rather than the string it reads as;
 * sys/ovl.h says why.
 */
#include <types.h>
#include <sys/sys.h>
#include <sys/con.h>
#include <sys/ovl.h>

extern struct ciovec memcvec;

struct ovlhdr memhdr = { 10, 0, 0, 0, 0, &memcvec, 0, 0,
	{ 'm', 'e', 'm' } };

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
