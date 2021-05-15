/* pathalias -- by steve bellovin, as told to peter honeyman */
#ifndef lint
static char	*sccsid = "@(#)mem.c	9.1 87/10/04";
#endif

#include "def.h"

/* exports */
extern void freelink(), wasted(), nomem();
extern long allocation();
long Ncount;

/* imports */
extern char *sbrk();
extern char *Netchars;
extern int Vflag;
extern void die();
#ifdef TMPFILES
extern off_t lseek();
extern int fdnode, fdlink, fdname;
extern node *getnode(), nullnode;
extern link *getlink(), nulllink;
#else /*TMPFILES*/

/* privates */
static link *Lcache;
#endif /*TMPFILES*/
static unsigned int Memwaste;

#ifndef TMPFILES
p_link
newlink()
{	register link *rval;

	if (Lcache) {
	 	rval = Lcache;
		Lcache = Lcache->l_next;
		strclear((char *) rval, sizeof(link));
	} else if ((rval = (link * ) calloc(1, sizeof(link))) == 0)
		nomem();
	return rval;
}
#else /*TMPFILES*/
p_link
newlink()
{
	p_link l_seq;

	/*
	 * We find the end of the file, use it to get the sequence number of
	 * the link (which we use instead of an offset, since the offset would
	 * have to be long and the sequence number can be short), and
	 * finally zero out the appropriate spot in the link temporary file.
	 * We then fill in and return the sequence number.
	 */
	l_seq = lseek(fdlink, (off_t) 0, L_XTND) / sizeof(link);

	if (write(fdlink, (char *) &nulllink, sizeof(link)) < 0) {
		perror("newlink");
		exit(1);
	}
	getlink(l_seq)->l_seq = l_seq;

	return l_seq;
}
#endif /*TMPFILES*/

/* caution: this destroys the contents of l_next */
/*ARGSUSED*/
void
freelink(l)
p_link l;
{
#ifndef TMPFILES
	l->l_next = Lcache;
	Lcache = l;
#else  /*TMPFILES*/
#endif /*TMPFILES*/
}

#ifndef TMPFILES
p_node
newnode()
{
	register node *rval;

	if ((rval = (node * ) calloc(1, sizeof(node))) == 0)
		nomem();
	Ncount++;
	return rval;
}
#else /*TMPFILES*/
p_node
newnode()
{
	register p_node n_seq;

	/*
	 * We find the end of the file, use it to get the sequence number of
	 * the node (which we use instead of an offset, since the offset would
	 * have to be long and the sequence number can be short), and
	 * finally zero out the appropriate spot in the node temporary file.
	 * We then fill in and return the sequence number.
	 */
	n_seq = lseek(fdnode, (off_t) 0, L_XTND) / sizeof(node);

	if (write(fdnode, (char *) &nullnode, sizeof(node)) < 0) {
		perror("newnode");
		exit(1);
	}
	getnode(n_seq)->n_seq = n_seq;

	Ncount++;
	return n_seq;
}
#endif /*TMPFILES*/

char	*
strsave(s)
	char *s;
{	register char *r;

	if ((r = malloc((unsigned) strlen(s) + 1)) == 0)
		nomem();
	(void) strcpy(r, s);
	return r;
}

#ifndef strclear
void
strclear(str, len)
	register char *str;
	register long len;
{
	while (--len >= 0)
		*str++ = 0;
}
#endif /*strclear*/

p_node	*
newtable(size)
	long size;
{	register p_node *rval;

	if ((rval = (p_node *) calloc(1, (unsigned int) size * sizeof(p_node))) == 0) 
		nomem();
	return rval;
}

/*ARGSUSED*/
freetable(t, size)
	p_node *t;
	long size;
{
#ifdef MYMALLOC
	addtoheap((char *) t, size * sizeof(p_node));
#else
	free((char *) t);
#endif
}

void
nomem()
{
	static char epitaph[128];

	sprintf(epitaph, "out of memory (%ldk allocated)", allocation());
	die(epitaph);
}

/* data space allocation -- main sets `dataspace' very early */
long
allocation()
{
	static char *dataspace;
	long rval;

	if (dataspace == 0) {		/* first time */
		dataspace = sbrk(0);	/* &end? */
		return 0;
	}
#ifndef TMPFILES
	rval = (sbrk(0) - dataspace)/1024;
#else /*TMPFILES*/
	rval = (long) sbrk(0)/1024;	/* break is useful on small machines */
#endif /*TMPFILES*/
	if (rval < 0)			/* funny architecture? */
		rval = -rval;
	return rval;
}

/* how much memory has been wasted? */
void
wasted()
{
	if (Memwaste == 0)
		return;
	vprintf(stderr, "memory allocator wasted %ld bytes\n", Memwaste);
}

#ifdef MYMALLOC

/* use c library malloc/calloc here, and here only */
#undef malloc
#undef calloc
extern char *malloc(), *calloc();

/* allocate in MBUFSIZ chunks.  4k works ok (less 16 for malloc quirks). */
#define MBUFSIZ (4 * 1024 - 16)

/* 
 * mess with ALIGN at your peril.  longword (== 0 mod 4)
 * alignment seems to work everywhere.
 */

#define ALIGN 2

typedef struct heap heap;
struct heap {
	heap *h_next;
	long h_size;
};

static heap *Mheap;	/* not to be confused with a priority queue */

addtoheap(p, size)
	char *p;
	long size;
{	int adjustment;
	heap *pheap;

	/* p is aligned, but it doesn't hurt to check */
	adjustment = align(p);
	p += adjustment;
	size -= adjustment;

	if (size < 1024)
		return;		/* can't happen */
	pheap = (heap *) p;	/* pheap is shorthand */
	pheap->h_next = Mheap;
	pheap->h_size = size;
	Mheap = pheap;
}

/*
 * buffered malloc()
 *	returns space initialized to 0.  calloc isn't used, since
 *	strclear can be faster.
 *
 * free is ignored, except for very large objects,
 * which are returned to the heap with addtoheap(). 
 */

char	*
mymalloc(n)
	register unsigned int n;
{	static unsigned int size; /* how much do we have on hand? */
	static char *mstash;	  /* where is it? */
	register char *rval;

	if (n >= 1024) {		/* for hash table */
		rval = malloc(n);	/* aligned */
		if (rval)
			strclear(rval, n);
		return rval;
	}

	n += align((char *) n);	/* keep everything aligned */

	if (n > size) {
		Memwaste += size;	/* toss the fragment */
		/* look in the heap */
		if (Mheap) {
			mstash = (char *) Mheap;	/* aligned */
			size = Mheap->h_size;
			Mheap = Mheap->h_next;
		} else {
			mstash = malloc(MBUFSIZ);	/* aligned */
			if (mstash == 0) {
				size = 0;
				return 0;
			}
			size = MBUFSIZ;
		}
		strclear(mstash, size);		/* what if size > 2^16? */
	}
	rval = mstash;
	mstash += n;
	size -= n;
	return rval;
}

/*
 * what's the (mis-)alignment of n?  return the complement of
 * n mod 2^ALIGN
 */
align(n)
	char *n;
{	register int abits;	/* misalignment bits in n */

	abits = (int) n & ~(0xff << ALIGN) & 0xff;
	if (abits == 0)
		return 0;
	return (1 << ALIGN) - abits;
}

#endif /*MYMALLOC*/
