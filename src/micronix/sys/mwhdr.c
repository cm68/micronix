/*
 * The HD-DMA driver's header (sys/ovl.h), in an object of its own.
 *
 * A module is its header first, at OVLBASE, because that is where the
 * kernel reads it (ovlattach, sys/ovl.c) - and which bytes land there is
 * the link's decision, not this file's.  A module is linked -Sdata,
 * which folds every section into the data segment, and an object's data
 * is placed in the order the objects are named on the link line.  So the
 * header is alone in an object that is named first, and nothing this
 * file could grow can push it off the front: the 28 bytes below are all
 * there is.  That is why the entries it points at are an object of the
 * driver's own rather than statics inside it (sys/mw.c) - an address a
 * header names has to be visible to the object holding the header.
 *
 * The init is mwinit, whose whole job is to remember the segment the
 * kernel placed the page in (sys/mw.c).  It is not the controller's
 * bring-up - that is still the reset at open - but the driver cannot be
 * handed the address of its own command block without it: the block is a
 * data object in this module, the address the driver computes for it is
 * the window address it appears at, and the board follows a physical one.
 * Being module code, mwinit names the block itself, so nothing here
 * reaches the driver's data and the data field stays 0.
 *
 * The tick is mwcheck, the controller check that used to arm a timer of
 * its own and now rides the resident tick (sys/ovl.c).
 *
 * name is a character list rather than the string it reads as; sys/ovl.h
 * says why, and it is the same reason this file is one struct.
 */
#include <types.h>
#include <sys/sys.h>
#include <sys/buf.h>
#include <sys/con.h>
#include <sys/proc.h>
#include <sys/mw.h>
#include <sys/ovl.h>

extern int mwopen(), mwclose(), mwstrat();
extern int mwcheck(), mwint();
extern int mwinit();
extern struct biovec mwbvec;

struct ovlhdr mwhdr = { 3, &mwinit, &mwcheck, 0, &mwbvec, 0, MWINT, &mwint,
	{ 'h', 'd', 'd', 'm', 'a' } };

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
