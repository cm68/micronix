/*
 * the tty driver's module header (sys/ovl.h)
 *
 * sys/ttyhdr.c
 *
 * The first bytes of the tty module's page.  The driver is the resident
 * console's line discipline plus the MultIO front end and the SS1 back
 * ends, and it serves char major 1; it fills cvec and leaves bvec alone.
 *
 * init is ttyinit (sys/multio.c): it publishes the page's entry points
 * to the resident half (sys/ttyres.c) for the interrupt stubs, which
 * arrive with no page of ours mapped.  The state itself - mttys, the
 * cooking buffer, p->tty - stays resident and is named from here.
 *
 * name is a character list rather than the string it reads as; sys/ovl.h
 * says why, and it is the same reason this file is one struct.
 */
#include <types.h>
#include <sys/sys.h>
#include <sys/con.h>
#include <sys/ovl.h>

extern struct ciovec ttycvec;
extern int ttyinit();

struct ovlhdr ttyhdr = { 0, &ttyinit, 0, 0, 0, &ttycvec, 0, 0,
	{ 't', 't', 'y' }, 0, 1 };

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
