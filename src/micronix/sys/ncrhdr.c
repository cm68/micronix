/*
 * The SCSI host adapter's header (sys/ovl.h), in an object of its own.
 *
 * A module is its header first, at OVLBASE, because that is where the
 * kernel reads it (ovlattach, sys/ovl.c) - and which bytes land there is
 * the link's decision, not this file's.  A module is linked -Sdata,
 * which folds every section into the data segment, and an object's data
 * is placed in the order the objects are named on the link line.  So the
 * header is alone in an object that is named first, and nothing this file
 * could grow can push it off the front: the 28 bytes below are all there
 * is.  That is why the entries it points at are an object of the driver's
 * own rather than statics inside it (sys/ncr.c) - an address a header
 * names has to be visible to the object holding the header.
 *
 * init is the presence probe (sys/ncrinit.c), and it is an init that
 * answers a question rather than switching something on.  A 5380 that is
 * not in the machine leaves its ports floating, the probe is what
 * notices, and its non-zero return is the driver saying it is not here:
 * ovlplace registers nothing, major 5 goes on answering as nodev does,
 * and the page is spent.  Nothing has been taken when init runs, so there
 * is nothing to give back.
 *
 * No tick - this controller answers when it is spoken to, and the one
 * asynchronous thing it has to say (the bus going free) is an interrupt.
 * No data either: the probe reads a register and computes nothing, and
 * what the driver keeps - boards[] and scs[] (sys/ncr.c) - is nobody
 * else's business, so there is no structure to point at.
 *
 * name is a character list rather than the string it reads as; sys/ovl.h
 * says why, and it is the same reason this file is one struct.
 */
#include <types.h>
#include <sys/sys.h>
#include <sys/buf.h>
#include <sys/con.h>
#include <sys/proc.h>
#include <sys/ncr.h>
#include <sys/ovl.h>

extern int ncropen(), ncrclose(), ncrstrat();
extern int ncrinit(), ncrint();
extern struct biovec ncrbvec;

struct ovlhdr ncrhdr = { 5, &ncrinit, 0, 0, &ncrbvec, 0, NCRINT, &ncrint,
	{ 'n', 'c', 'r', '5', '3', '8', '0' } };

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
