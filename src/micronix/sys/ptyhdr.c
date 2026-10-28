/*
 * the pty driver's module header (sys/ovl.h)
 *
 * sys/ptyhdr.c
 *
 * The first byte of the module's page.  The driver is a character
 * device, so it fills cvec and leaves bvec alone; there is no hardware
 * to find or interrupt, so init, probe, tick and intr are all zero and
 * the major is registered as soon as the page is placed.
 *
 * name is a character list rather than the string it reads as; sys/ovl.h
 * says why, and it is the same reason this file is one struct.
 */
#include <types.h>
#include <sys/sys.h>
#include <sys/con.h>
#include <sys/ovl.h>

extern struct ciovec ptycvec;

struct ovlhdr ptyhdr = { 0, 0, 0, 0, 0, &ptycvec, 0, 0,
	{ 'p', 't', 'y' }, 0, 5 };

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
