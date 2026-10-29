/*
 * The network disk's module header (sys/ovl.h), in an object of its own.
 *
 * The ND half has one thing to say - op-done - which arrives as the
 * interrupt this header names, and its probe is an ND_READ of block 0: a
 * tickle with no side effect (sys/INTR.md).  The card's socket face is a
 * page of its own (sys/nethdr.c, sys/net.c), char major 6; the two
 * namespaces share the number and nothing else.
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
extern int ndint(), ndtick(), ndprobe();
extern struct biovec ndbvec;

struct ovlhdr ndhdr = { 6, 0, &ndtick, 0, &ndbvec, 0, &ndprobe, &ndint,
	{ 'n', 'd' }, 1, 0 };

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
