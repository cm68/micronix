# include "dar.h"

/*
 * copy string from a to b 
 */

scopy (a, b)
	char *a, *b;
	{
	cpystr (b, a, NULL);
	}

	char *
save (a)
	char *a;
	{
	char *b;

	b = alloc (lenstr (a) + 1);

	cpystr (b, a, NULL);

	return b;
	}

lc (a)
	{
	return tolower (a);
	}
