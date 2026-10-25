/*
 * the ports device's module header (sys/ovl.h)
 *
 * sys/iohdr.c
 *
 * Major 3, /dev/io: the same driver as major 2 (sys/memdev.c), which
 * takes the iorw() path for it.  The page carries its own copy of
 * memdev.o and its own memcvec, so a module placed here is self
 * contained; the two headers name the one driver, not each other.
 *
 * name is a character list rather than the string it reads as;
 * sys/ovl.h says why.
 */
#include <types.h>
#include <sys/sys.h>
#include <sys/con.h>
#include <sys/ovl.h>

extern struct ciovec memcvec;

struct ovlhdr iohdr = { 3, 0, 0, 0, 0, &memcvec, 0, 0,
	{ 'i', 'o' } };

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
