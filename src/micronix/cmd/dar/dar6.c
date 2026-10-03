# include "dar.h"

/*
 * zero a cluster
 */

gzero (a)
	{
	char *b;

	b = calloc (1, d->bpg);

	gio (a, b, WRITE);

	cfree (b);
	}



