/*
 * the Udot
 *
 * sys/user.c 
 * Changed: <2022-01-04 11:33:16 curt>
 */
#include <types.h>
#include <sys/sys.h>
#include <sys/proc.h>

/*
 * Location of user structure.
 * The u structure lives entirely within one of the Decision's
 * memory-mapped 4K segments (USERSEG, see sys.h).  newmap() remaps
 * that segment per process, and fork()'s bankcopy() clones the whole
 * segment; so nothing else in the segment may be mutable data or it
 * would be cloned per process too.  The rest of the segment is padded
 * out below so the next data object begins on the next 4K boundary,
 * and u is linked first among the data objects so it sits at the base
 * of USERSEG; textpad.s pads the text so the data segment begins there.
 */

struct user u = 0;

/*
 * Pad the remainder of u's 4K segment.  The size is computed from
 * sizeof(struct user), so it tracks any change to proc.h.  It must be
 * initialised (not bss) so it stays in the data segment, adjacent to u.
 */
char u_segpad[4096 - sizeof(struct user)] = {0};

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
