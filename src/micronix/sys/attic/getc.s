
	/*
/	 * c - list element
/	 *	2 bytes of pointer, 14 bytes of data
/	 */

/struct cblock
/	{
/	struct cblock *next;
/	char block[14];
/	};

   /*
/	* Terminal circular queue
/	*/
/struct que
/	{
/	UCHAR		count;
/	char	       *first;
/	char	       *last;
/	};

public  _getc
_getc:
	/
/getc (q)
/	register struct que *q;
/	{
/	fast UINT c;

	call _di;	/* di (); */
	
	sp => bc => hl <= hl <= bc;	/* hl = q; */

	a = *hl | a; jz L1;		/* if (q->count) */

	a = *(hl + 1) & 15; jnz L2;	/* if (q->first & 15) */

	hl - 1 => sp; call _qfree; sp => hl;


/	if (q->count && (((int) (q->first) & 15) || qfree (q)))
/		{
		L2:
		(*hl) - 1;
/		q->count--;


		hl + 1 => sp =a^ hl; a = *hl; hl + 1;

		bc = hl; 
		hl <= sp;

		*hl = c; *(hl + 1) = b;

		c = a;
		hl -1 -1;


/		c = *q->first++;
/		}
	else
		{
		L1:
		c = (-1);
		}


	a = *hl | a; jnz L3;
		/
		sp <= bc <= hl; call _qrelse; sp => hl => bc;
		/

/	if (q->count == 0)
/		qrelse(q);

	L3:
	call _ei;

/	ei ();

	ret;
/	return c;
	}


	static
qrelse(q)
	fast struct que *q;
	{
	static struct cblock *b;
	
	sp => bc => hl <= hl <= bc;

	hl => sp;

	hl =a^ hl;

	b = q->first;

	a = h | l; jz L4;

	if (b)
		{
		hl - 1;
		a = l & 0xf0 -> l;

/		b = ((int)b - 1) & ~15;

		hl + 1;

/		b++;

		
		bc = hl;
		sp => hl <= hl;

		*(hl + 1) = c;
		*(hl + 1) = b;

		*(hl + 1) = c;
		*(hl + 1) = b;
		


		q->first = b;
		q->last = b;

		call _qfree; 


/		qfree(q);
		}

	L4:

	sp => af;	 
	ret;
	}

	static
qfree (q)
	register struct que *q;
	{
	static struct cblock *new, *old;




	old = q->first;		/* the cblock to be freed */

	if (!old)
		{
		qinit (q);
		return NO;
		}

	old--;

	new = old->next;	/* save pointer */

	old->next = cfree;	/* free cblock */
	cfree = old;

	if (new)
		{
		q->first = new->block;
		return YES;
		}

	else
		{
		qinit (q);
		return NO;
		}
	}

qinit (q)
	register struct que *q;
	{
	q->count = 0;
	q->first = 0;
	q->last	 = 0;
	}
