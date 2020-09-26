/*
 * user.c 
 */
#include "sys.h"
#include "proc.h"

/*
 * Location of user structure.
 * Must be compiled with the -x7 option to place
 * the u structure in with the code. Must be linked
 * into the code at an address that places it
 * entirely within one of the Decision's memory-
 * mapped 4K segments. Everything else in that
 * segment must be pure code, so that different
 * user-segments can be mapped in to switch processes.
 * The USEG definition in sys.h must give the offset into
 * the Decision's memory map for that segment.
 */

struct user u = 0;

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
