#
/*
 *	UNIX shell
 *
 *	S. R. Bourne
 *	Bell Telephone Laboratories
 *
 */

#include	"defs.h"

/* the library header costs c0 heap in a 64K process; name what this file calls */
extern char *sbrk(int);

setbrk(incr)
{
	REG BYTPTR	a=sbrk(incr);
	brkend=a+incr;
	return(a);
}
