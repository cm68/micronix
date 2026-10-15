/*
 * The CompuPro DISK 1 / DISK 1A driver's header (sys/ovl.h), in an object
 * of its own.
 *
 * A module is its header first, at OVLBASE, because that is where the
 * kernel reads it (ovlattach, sys/ovl.c) - and which bytes land there is
 * the link's decision, not this file's.  A module is linked -Sdata, which
 * folds every section into the data segment, and an object's data is
 * placed in the order the objects are named on the link line.  So the
 * header is alone in an object that is named first, and nothing this file
 * could grow can push it off the front.  That is why the entries it points
 * at are an object of the driver's own rather than statics inside it
 * (sys/disk1.c).
 *
 * There is no init: the board fetches nothing out of this module.  The
 * command goes to the chip a byte at a time through the data register
 * and the data phase address goes to the glue's own register, so there
 * is no command block in memory and no page or physical address for an
 * init to learn.
 *
 * The probe is disk1probe, which makes the card raise its interrupt by
 * completing a RECALIBRATE, and the handler is disk1int.  The line they
 * are wired to is not named here: sys/probe.c discovers it at boot.
 *
 * The tick is disk1check, the watchdog that turns a board which has
 * stopped answering into an error rather than a hang.
 *
 * name is a character list rather than the string it reads as; sys/ovl.h
 * says why, and it is the same reason this file is one struct.
 */
#include <types.h>
#include <sys/sys.h>
#include <sys/buf.h>
#include <sys/con.h>
#include <sys/proc.h>
#include <sys/disk1.h>
#include <sys/ovl.h>

extern int disk1open(), disk1close(), disk1strat(), disk1ioctl();
extern int disk1probe(), disk1int(), disk1check();
extern struct biovec disk1bvec;

struct ovlhdr disk1hdr = { 9, 0, &disk1check, 0, &disk1bvec, 0,
	&disk1probe, &disk1int, { 'd', 'i', 's', 'k', '1' }, 8 };

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
