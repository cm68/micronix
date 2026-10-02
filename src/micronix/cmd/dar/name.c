# include "dar.h"

/*
 * convert a DOS directory entry to a UNIX file name
 */

name (a, c)
	FAST struct entry *a;
	FAST TEXT *c;
	{
	INTERN TEXT z;
	INTERN UTINY i;

	for (i = 0; i < sizeof a->name; i++)
		{
		z = a->name[i];

		z &= 0177;		/* strip high bit */
		z = tolower (z);	/* lower case */

		if (z == ' ')		/* no more */
			break;
		
		if (z == '/')
			z = '|';

		*c++ = z;
		}

	for (i = 0; i < sizeof a->type; i++)
		{
		z = a->type[i];

		z &= 0177;		/* strip high bit */
		z = tolower (z);	/* lower case */

		if (z == ' ')		/* no more */
			break;

		if (!i)
			*c++ = '.';
		
		if (z == '/')
			z = '|';

		*c++ = z;
		}

	*c = NULL;
	}

/*
 * cname - transcribe a unix name into a entry block at b
 *	act as the user would
 */

cname (a, b)
	FAST TEXT *a;
	FAST struct entry *b;
	{
	FAST TEXT *per;
	INTERN TEXT z;
	INTERN UTINY i;

	a = tail (a);


/*
 * find the last period
 */

	for (per = a + lenstr (a) - 1; per >= a; per--)
		if (*per == '.')
			break;

	if (per < a)
		per = NULL;


	fill (b->name, sizeof b->name, ' ');
	fill (b->type, sizeof b->name, ' ');
	b->att = 0;
	fill (b->res,  sizeof b->res,   0 );

	for (i = 0; i < sizeof b->name; i++)
		{
		if (!*a || a == per)
			break;

		z = *a++;
		z &= 0377;
		z = toupper (z);

		b->name[i] = z;

		}

	if (per)
		{
		a = per + 1;

		for (i = 0; i < sizeof b->type; i++)
			{
			if (!*a)
				break;

			z = *a++;
			z &= 0377;
			z = toupper (z);

			b->type[i] = z;
			}
		}
	}

/*
 * Test to see if the given character is a valid DOS directory
 * name character.
 */

okchar (a)
	unsigned char a;
	{
	return a && !(a & 0200) && !islower (a);
	}

/*
 *	a is the file name
 *	b is the matching expression
 */

	BOOL
match (a, b)
	TEXT *a, *b;
	{
	if (*a == NULL && *b == NULL)		/* end simultaneously */
		return YES;

	if (*b == NULL)			/* pattern ends */
		return NO; 

	if (*b == '*')
		{
		TEXT *p;

		for (p = a + lenstr (a); a <= p; p--)
			if (match (p, b + 1))
				return YES;

		return NO;
		}

	if (*a == NULL)			/* text ends */
		return NO;

	if (omatch (a, b))
		return match (a + 1, nextpat (b));

	else
		return NO;
	}

	BOOL
omatch (a, b)
	FAST TEXT *a, *b;
	{
	BOOL sense;
	TEXT *start;

	if (*a != NULL && *b == '?')
		return YES;

	else if (*b == ESCAPE && b[1])
		return *a == b[1];

	else if (*b != '[')
		return *a == *b;

	b++;		/* [ .... ]    form */


	if (*b == NEGATE)
		{
		sense = NO;
		b++;
		}
	else
		{
		sense = YES;
		}


	start = b;		/* mark the starting pos. */



	for (;;)
		{
		if (*b == ']' || !*b)
			{
			return !sense;
			}

		else if (*b == ESCAPE && b[1])
			{
			if (*a == b[1])
				return sense;
			else
				b += 2;
			}

		else if	(
			*b == '-'
			&&
			b != start
			&&
			alphanum (b[1])
			&&
			alphanum (b[-1])
			)
			{
			if (b[-1] <= *a && *a <= b[1])
				return sense;

			else
				b += 2;
			}

		else
			{
			if (*a == *b)
				return sense;
			else
				b++;
			}
		}

	return NO;
	}

	TEXT *
nextpat (a)
	TEXT *a;
	{
	if (*a == ESCAPE && a[1])
		return a + 2;

	if (*a == '[')
		{
		a++;
		
		while (*a)
			{
			if (*a == ESCAPE && a[1])
				a += 2;

			else if (*a == ']')
				return a + 1;

			else
				a++;
			}

		return a;
		}

	return a + 1;
	}

	BOOL
alphanum (a)
	{
	return isdigit (a) || isalpha (a);
	}
