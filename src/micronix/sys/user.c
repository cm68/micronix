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
 * would be cloned per process too.
 *
 * u is linked first among the objects that fill USERSEG: GNUmakefile
 * links "user.rel" from user.o, then the leaf code (mem.o, inout.o, and
 * later the libccc runtime) at -Tdata=0xf000, so u sits at the base of
 * the segment and the pure-text leaf code fills the space after it.
 * That code is assembled as .data, not .text, because the loader reads
 * text and then data - text placed here would be overwritten by the
 * data pass.  The rest of the segment stays empty: the HD-DMA loader's
 * stack parks at 0xffff, so the image must not reach the top of the page.
 */

struct user u = 0;

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
