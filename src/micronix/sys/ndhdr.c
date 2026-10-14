/*
 * The network disk's module header (sys/ovl.h), in an object of its own.
 *
 * The card is port-mapped and always present in the simulator, so there
 * is no presence probe and no init: init is zero and the driver is
 * registered the moment it is placed.  It has one thing to say on its
 * own - op-done - which arrives as the one interrupt this header names.
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
	{ 'n', 'd' }, 1 };

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
