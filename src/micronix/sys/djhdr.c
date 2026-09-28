/*
 * The floppy driver's header (sys/ovl.h), in an object of its own.
 *
 * A module is its header first, at OVLBASE, because that is where the
 * kernel reads it (ovlattach, sys/ovl.c) - and which bytes land there is
 * the link's decision, not this file's.  A module is linked -Sdata,
 * which folds every section into the data segment, and an object's data
 * is placed in the order the objects are named on the link line.  So the
 * header is alone in an object that is named first, and nothing this
 * file could grow can push it off the front: the 28 bytes below are all
 * there is.
 *
 * Splitting it out is what that costs, and it is why the entries it
 * points at are objects of the driver's own rather than statics inside
 * it (sys/dj.c, sys/mwhdr.c's siblings) - an address a header names has
 * to be visible to the object holding the header.
 *
 * Everything here is a name and nothing is a call: the entries, the line
 * the handler is on, the tick the watchdog wants, and the command block
 * djinit() reaches through ovldata().  The kernel is linked naming no
 * driver symbol and this names no kernel one; that seam is the whole
 * reason a kernel can be built with no disk drivers in it.
 *
 * data is the command block rather than something djinit() computes,
 * because djinit is resident kernel code and can name nothing inside a
 * module - not a function, and a data object no more than a function.
 * The block is in the driver's own segment, so its address is a link
 * time constant here and only here.  The kernel never looks inside it;
 * all it does with the address is hand it back to the driver.
 *
 * name is a character list rather than the string it reads as; sys/ovl.h
 * says why, and it is the same reason this file is one struct.
 */
#include <types.h>
#include <sys/sys.h>
#include <sys/buf.h>
#include <sys/con.h>
#include <sys/proc.h>
#include <sys/dj.h>
#include <sys/ovl.h>

extern int djopen(), djclose(), djstrat(), djioctl();
extern int djinit(), djtick(), djint();
extern unsigned char djcomm[];
extern struct biovec djbvec;

struct ovlhdr djhdr = { 2, &djinit, &djtick, (char *) djcomm,
	&djbvec, 0, DJINT, &djint, { 'd', 'j', 'd', 'm', 'a' } };

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
