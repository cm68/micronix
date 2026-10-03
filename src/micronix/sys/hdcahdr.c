/*
 * The HDCA's module header (sys/ovl.h), in an object of its own.
 *
 * The controller is port-mapped and always present on the M10/M20/M26,
 * so there is no presence probe and no init: init is zero and the
 * driver is registered the moment it is placed.  There is no periodic
 * work either - the controller answers when it is spoken to, and the
 * two things it says on its own (op done and seek done) arrive as the
 * one interrupt this header names.
 *
 * name is a character list rather than the string it reads as;
 * sys/ovl.h says why.
 */
#include <types.h>
#include <sys/sys.h>
#include <sys/buf.h>
#include <sys/con.h>
#include <sys/proc.h>
#include <sys/hdca.h>
#include <sys/ovl.h>

extern int hdopen(), hdclose(), hdstrat();
extern int hdint();
extern struct biovec hdbvec;

struct ovlhdr hdcahdr = { 1, 0, 0, 0, &hdbvec, 0, HDCAINT, &hdint,
	{ 'h', 'd', 'c', 'a' } };

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
