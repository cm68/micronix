/*	FREE A LINKED LIST
 *	copyright (c) 1978 by Whitesmiths, Ltd.
 */
#include <std.h>

struct list *frelst(p, plast)
	FAST struct {
		TEXT *next;
		} *p, *plast;
	{
	while (p && p != plast)
		p = wsfree(p, p->next);
	return (p);
	}
