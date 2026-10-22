/*
 * The network disk's module header (sys/ovl.h), in an object of its own.
 *
 * The card is port-mapped and its ND half needs no bring-up, but the
 * module carries the network functions too (sys/net.c, block major 6
 * minor 1): ndstart() is where their probe and handler join the attach
 * loop, with the segment the module was placed in.  The ND half has one
 * thing to say on its own - op-done - which arrives as the interrupt
 * this header names, and its probe is an ND_READ of block 0: a tickle
 * with no side effect (sys/INTR.md).
 *
 * name is a character list rather than the string it reads as;
 * sys/ovl.h says why.
 */
#include <types.h>
#include <sys/sys.h>
#include <sys/buf.h>
#include <sys/con.h>
#include <sys/proc.h>
#include <sys/nd.h>
#include <sys/ovl.h>

extern int ndopen(), ndclose(), ndstrat();
extern int ndint(), ndtick(), ndprobe(), ndstart();
extern struct biovec ndbvec;

struct ovlhdr ndhdr = { 6, &ndstart, &ndtick, 0, &ndbvec, 0, &ndprobe, &ndint,
	{ 'n', 'd' }, 1 };

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
