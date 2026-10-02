# include "dar.h"

/*
 * set the d structure pointer
 * a is the file descript
 */

findsize ()	
	{
	static unsigned s;

/*
 * measure the device
 */

	s = devsize (fd);


/*
 * scan the table
 */

	for (d = disk; d->bpv; d++)
		if (d->bpv == s)
			return;

putstr (STDERR, "Device size ", itoa (s), ": Unknown format\n", NULL);

	exit (NO);
	}
