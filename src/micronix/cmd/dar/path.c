# include "dar.h"

/*
 * return the tail of a path name
 */

	char *
tail (a)
	char *a;
	{
	while (element ('/', a))
		a++;

	return a;
	}

	char *
parent (a)
	FAST TEXT *a;
	{
	INTERN TEXT buf [PATHSIZE];
	TEXT *p;

	if (!element ('/', a))
		return ".";

	cpystr (buf, a, NULL);

	p = buf + lenstr (buf) - 1;	/* last character */

	while (buf < p && *p == '/')	/* remove trailing slashes */
		*p-- = NULL;

	while (buf < p && *p != '/')	/* remove trailing component */
		*p-- = NULL;

	while (buf < p && *p == '/')	/* remove trailing slashes */
		*p-- = NULL;

	return  buf;
	}


/*
 * extract the first compenent from a path name 
 */

	char *
getcomp (a, b)
	char *a, *b;
	{
	while (*a == '/')
		a++;

	while (*a && *a != '/')
		*b++ = *a++;

	*b = 0;
	return a;
	}
