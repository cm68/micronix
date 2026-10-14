/*
 * The CompuPro DISK 3 driver's header (sys/ovl.h), in an object of its own.
 *
 * A module is its header first, at OVLBASE, because that is where the
 * kernel reads it (ovlattach, sys/ovl.c) - and which bytes land there is
 * the link's decision, not this file's.  A module is linked -Sdata, which
 * folds every section into the data segment, and an object's data is
 * placed in the order the objects are named on the link line.  So the
 * header is alone in an object that is named first, and nothing this file
 * could grow can push it off the front.  That is why the entries it points
 * at are an object of the driver's own rather than statics inside it
 * (sys/disk3.c) - an address a header names has to be visible to the
 * object holding the header.
 *
 * The init is disk3init, which remembers the segment the kernel placed the
 * page in - the IOPB is a data object in this module, the board follows a
 * physical address, and the driver has to be told which page that address
 * is.  Like the HD-DMA (sys/mw.c), the controller's reset is still at open;
 * the presence of the board is established there and by the watchdog.
 *
 * The tick is disk3check, the controller check that turns a silent board
 * into NOTRDY rather than a hang.
 *
 * name is a character list rather than the string it reads as; sys/ovl.h
 * says why, and it is the same reason this file is one struct.
 */
#include <types.h>
#include <sys/sys.h>
#include <sys/buf.h>
#include <sys/con.h>
#include <sys/proc.h>
#include <sys/disk3.h>
#include <sys/ovl.h>

extern int disk3open(), disk3close(), disk3strat();
extern int disk3init(), disk3int(), disk3check(), disk3probe();
extern struct biovec disk3bvec;

struct ovlhdr disk3hdr = { 6, &disk3init, &disk3check, 0, &disk3bvec, 0,
	&disk3probe, &disk3int, { 'd', 'i', 's', 'k', '3' }, 4 };

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
