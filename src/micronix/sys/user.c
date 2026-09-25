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
 * The segment is filled by two link-time pieces: the pure-code leaf
 * objects (mem.o, inout.o, and later the libccc runtime) first, then
 * user.o, whose own storage is ustack below u.  GNUmakefile gives the
 * partial link -Tdata=0xf000, so the code starts at the base of the
 * segment, ustack follows it, and u sits at the top of the three.  That
 * code is assembled as .data, not .text, because the loader reads text
 * and then data - text placed here would be overwritten by the data
 * pass.
 *
 * Neither ustack nor u has a value to load, so both are .bss and the
 * file carries no bytes for them.  The loader's stack lives at the top
 * of the same page, so it runs straight over them; ustack is a stack
 * and does not care, and task0_body() zeroes u before it is used.  Only
 * the code has to stay below the loader's stack, which is what lets the
 * image run to the top of the page.
 *
 * ustack is the system stack.  Keeping it out of the struct puts the
 * register save area at the base of u (see proc.h), which is where the
 * firmware's trap entry leaves SP, and leaves one number to turn if the
 * page ever has to get smaller.
 */

char ustack[512];               /* the system stack, just below u */

struct user u;

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
