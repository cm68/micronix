# include "dar.h"

duptest ()
	{
	INTERN UCOUNT i, j;

	for (i = 0; i < nfiles; i++)
		{
		for (j = i + 1; j < nfiles; j++)
			{
			if (cmpstr (files[i], files[j]))
				{
				put (files[i]);
				put (": duplicate file name\n");
				exit (NO);
				}
			}
		}
	}


/*
 * is this a DOS formatted diskette ?
 * return YES or NO
 */

isdos ()
	{
	static struct entry *f;
	static m;
	unsigned char c;

	for (f = thedir; f < &thedir [d->epd]; f++)
		{
		c = *f->name;


		if (c == 0 || c == NOENT)
			continue;

		for (m = 0; m < sizeof f->name; m++)
			if (!okchar (f->name [m]))
				return NO;

		for (m = 0; m < sizeof f->type; m++)
			if (!okchar (f->type [m]))
				return NO;
		}

	return YES;
	}


	char *
itoa (a)
	{
	static char buf [16];

	buf[itob(buf, a, 10)] = 0;
	return buf;
	}

putnum (a)
	{
	put (itoa (a));
	}
