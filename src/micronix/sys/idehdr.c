/*
 * The IDE driver's header (sys/ovl.h), in an object of its own.
 *
 * A module is its header first, at OVLBASE, because that is where the
 * kernel reads it (ovlattach, sys/ovl.c) - and which bytes land there is
 * the link's decision, not this file's.  A module is linked -Sdata,
 * which folds every section into the data segment, and an object's data
 * is placed in the order the objects are named on the link line.  So the
 * header is alone in an object that is named first, and nothing this
 * file could grow can push it off the front: the 28 bytes below are all
 * there is.  That is why the entries it points at are an object of the
 * driver's own rather than statics inside it (sys/ide.c) - an address a
 * header names has to be visible to the object holding the header.
 *
 * No tick - this controller answers when it is spoken to.  No init
 * either: VI2 is already unmasked by the board bring-up and the drive's
 * own reset belongs at open, where it is, so there is nothing to switch
 * on before this driver is reachable.  Nothing here reaches the driver's
 * data, so there is no data structure to point at yet.
 *
 * name is a character list rather than the string it reads as; sys/ovl.h
 * says why, and it is the same reason this file is one struct.
 */
#include <types.h>
#include <sys/sys.h>
#include <sys/buf.h>
#include <sys/con.h>
#include <sys/proc.h>
#include <sys/ide.h>
#include <sys/ovl.h>

extern int ideopen(), ideclose(), idestrat();
extern int ideint();
extern struct biovec idebvec;

struct ovlhdr idehdr = { 4, 0, 0, 0, &idebvec, 0, IDEINT, &ideint,
	{ 'i', 'd', 'e' } };

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
