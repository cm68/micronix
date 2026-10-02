# include "dar.h"


isfile (a)
	FAST TEXT *a;
	{
	INTERN UCOUNT i;

	if (nfiles <= 0)
		return YES;

	for (i = 0; i < nfiles; i++)
		{
		if (rflag) /* for replacement, names must match exactly */
			{
			if (cmpstr (a, files[i]))
				{
				return i;
				}
			}
		else
			{
			if (match (a, files [i]))
				{
				return i;
				}
			}
		}


	return -1;
	}
