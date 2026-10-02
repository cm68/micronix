# include "dar.h"

/*
 * zero a cluster
 */

gzero (a)
	{
	extern write ();

	char *b;

	b = calloc (1, d->bpg);

	gio (a, b, write);

	cfree (b);
	}



