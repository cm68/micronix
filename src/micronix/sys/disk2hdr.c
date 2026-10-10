/*
 * The CompuPro DISK 2 driver's header (sys/ovl.h), in an object of its own.
 *
 * A module is its header first, at OVLBASE, because that is where the
 * kernel reads it (ovlattach, sys/ovl.c) - and which bytes land there is
 * the link's decision, not this file's.  A module is linked -Sdata, which
 * folds every section into the data segment, and an object's data is
 * placed in the order the objects are named on the link line.  So the
 * header is alone in an object that is named first, and nothing this file
 * could grow can push it off the front.  That is why the entries it points
 * at are an object of the driver's own rather than statics inside it
 * (sys/disk2.c).
 *
 * There is no init: the DISK 2 has no in-memory command block the board
 * fetches - the host talks through the registers and the SELECTOR CHANNEL
 * takes the DMA address straight from the buffer - so there is no page or
 * physical address for an init to learn.  There is no interrupt either:
 * the driver polls the status register.  The tick is disk2check, the
 * watchdog that turns a silent board into NOTRDY rather than a hang.
 *
 * name is a character list rather than the string it reads as; sys/ovl.h
 * says why, and it is the same reason this file is one struct.
 */
#include <types.h>
#include <sys/sys.h>
#include <sys/buf.h>
#include <sys/con.h>
#include <sys/proc.h>
#include <sys/disk2.h>
#include <sys/ovl.h>

extern int disk2open(), disk2close(), disk2strat();
extern int disk2check();
extern struct biovec disk2bvec;

struct ovlhdr disk2hdr = { 7, 0, &disk2check, 0, &disk2bvec, 0, 0, 0,
	{ 'd', 'i', 's', 'k', '2' }, 4 };

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
