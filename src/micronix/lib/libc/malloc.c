/*
 * malloc
 *
 */

#ifdef debug
#define ASSERT(p) if(!(p))botch("p");else
botch(s)
char *s;
{
	printf("assertion botched: %s\n",s);
	abort();
}
#else
#define ASSERT(p)
#endif

/*	C storage allocator for Z80 and other 8 bit machines
 *	circular first-fit strategy
 *	works with noncontiguous, but monotonically linked, arena
 *	each block is preceded by a ptr to the (pointer of) 
 *	the next following block and a busy flag
 *	bit in flag is 1 for busy, 0 for idle
 *	gaps in arena are merely noted as busy blocks
 *	last block of arena (pointed to by alloct) is empty and
 *	has a pointer to first
 *	idle blocks are coalesced during space search
 *
*/
#define	BLOCK	(43*sizeof(struct store))	/* 129 bytes: the granule
						 * is also the last grab before
						 * the sbrk guard says no */
#define BUSY 1
#define NULL 0
#define	testbusy(p)	((p).flag & BUSY)
#define	sbusy(p)	(p).flag |= BUSY
#define	cbusy(p)	(p).flag &= ~BUSY

struct store
{
	struct store *	ptr;
	char		flag;
};

static struct store	allocs[2];	/*initial arena*/
static struct store *	allocp;		/*search ptr*/
static struct store *	alloct;		/*arena top*/
static struct store	allocx;		/* for realloc */
char *			sbrk();

/*
 * __malloc is the allocator: it returns 0 when it cannot find the
 * space.  Almost nothing wants that - see malloc below - but the
 * entry point is here for anything that genuinely can carry on
 * without the memory it asked for.
 */
char *
__malloc(nw)
unsigned nw;
{
	register struct store *p, *q;
	static unsigned temp;	/*coroutines assume no auto*/

	if(allocs[0].ptr==(struct store *)0) {	/*first time*/
		alloct = allocs[0].ptr = &allocs[1];
		allocp = allocs[1].ptr = &allocs[0];
		sbusy(allocs[0]);
		sbusy(allocs[1]);
	}
	nw = ((nw - 1 + sizeof(struct store)*2)/sizeof(struct store)) * sizeof(struct store);
	ASSERT(allocp>=allocs && allocp<=alloct);
	ASSERT(allock());
	for(p=allocp; ; ) {
		for(temp=0; ; ) {
			if(!testbusy(*p)) {
				while(!testbusy(*(q=p->ptr))) {
					ASSERT(q>p&&q<alloct);
					p->ptr = q->ptr;
				}
				if(q>=(struct store *)((char *)p+nw) && (struct store *)((char *)p+nw)>=p)
					goto found;
			}
			q = p;
			p = p->ptr;
			if(p>q)
				ASSERT(p<=alloct);
			else if(q!=alloct || p!=allocs) {
				ASSERT(q==alloct&&p==allocs);
				return(NULL);
			} else if(++temp>1)
				break;
		}
		temp = ((nw+sizeof(struct store)-1+BLOCK)/BLOCK)*BLOCK;
		q = (struct store *)sbrk(0);
		if((struct store *)((char *)q+temp) < q) {
			return(NULL);
		}
		q = (struct store *)sbrk(temp);
		if((int)q == -1) {
			return(NULL);
		}
		ASSERT(q>alloct);
		alloct->ptr = q;
		if(q!=alloct+1)
			sbusy(*alloct);
		else
			cbusy(*alloct);
		alloct = q->ptr = (struct store *)((char *)q+temp-sizeof(struct store));;
		alloct->ptr = allocs;
		sbusy(*alloct);
		cbusy(*q);
	}
found:
	allocp = (struct store *)((char *)p + nw);
	ASSERT(allocp<=alloct);
	if(q>allocp) {
		/* member copies: ccc has no struct assignment */
		allocx.ptr = allocp->ptr;
		allocx.flag = allocp->flag;
		allocp->ptr = p->ptr;
		allocp->flag = 0;
	}
	p->ptr = allocp;
	sbusy(*p);
	return((char *)(p+1));
}

#include <errno.h>

extern int	write(int, void *, int);
extern void	exit(int);

/*
 * malloc does not come back empty.
 *
 * Nothing on a machine this size can do anything useful with a failed
 * allocation, and the evidence is that nobody even tried: cpp had
 * thirteen call sites and checked none of them, ld had thirteen and
 * checked none of them.  What that costs is not a null pointer
 * dereference and a core file - there is no core file here.  The first
 * write through the null lands in page zero, on top of the rst 08
 * syscall trap.  The next write() then does not trap; execution runs
 * forward through the zeroed page to 0x0100, which is the entry point,
 * and the program starts again.  It runs out of memory again, each
 * pass eating another frame, until the stack has walked down through
 * the heap.  The symptom is a program that has consumed the machine at
 * a call depth of four, with no diagnostic anywhere near the cause.
 *
 * So the check belongs here, once, rather than in every program that
 * remembers.  permalloc has always done this; now malloc agrees with
 * it.  Anything that really can recover calls __malloc and looks at
 * what it gets.
 */
char *
malloc(nw)
unsigned nw;
{
	char *p;

	p = __malloc(nw);
	if (p == NULL) {
		write(2, "out of memory\n", 14);
		exit(ENOMEM);
	}
	return p;
}

/*	freeing strategy tuned for LIFO allocation
*/
free(ap)
char *ap;
{
	register struct store *p;

	/*
	 * free(0) is a no-op, and C has said so since it was
	 * standardised.  This one stepped back over the header of the
	 * pointer it was not given and parked the allocation cursor
	 * there, so the next malloc handed out a block inside the
	 * previous one - the heap came apart quietly, a call or two
	 * later.  One test here is worth a guard at every call site,
	 * and there are forty-three of them.
	 */
	if (ap == 0)
		return;
	p = ((struct store *)ap)-1;
	ASSERT(p>allocs[1].ptr&&p<=alloct);
	ASSERT(allock());
	allocp = p;
	ASSERT(testbusy(*p));
	cbusy(*p);
	ASSERT(p->ptr > allocp && p->ptr <= alloct);
}

char *
realloc(p, nbytes)
char *		p;
unsigned short	nbytes;
{
	register struct store *	xp, * q;
	unsigned short		ons;
	unsigned short		ns;

	xp = (struct store *)p;
	ns = (nbytes + sizeof(struct store) - 1)/sizeof(struct store);
	ons = xp[-1].ptr - xp;
	if(testbusy(xp[-1]))
		free((char *)xp);
	if(!(q = (struct store *)malloc(nbytes)) || q == xp)
		return (char *)q;
	ns = q[-1].ptr - q;
	if(ons > ns)
		ons = ns;
	bmove((char *)xp, (char *)q, ons * sizeof(struct store));
	/*
	 * Both halves are inside the if.  This was one struct assignment
	 * until the copy was written out by hand - ccc has no structure
	 * assignment - and the second half was left outside, so it ran
	 * on every realloc and stored through an index that only means
	 * anything when the blocks overlap.  It landed in the text of
	 * whatever was loaded there; in c0 that was a write to 0xa446
	 * from inside realloc itself, and c0 could not compile any
	 * source with a switch big enough to grow the case array.
	 */
	if(q < xp && q+ns > xp) {
		q[q+ns-xp].ptr = allocx.ptr;
		q[q+ns-xp].flag = allocx.flag;
	}
	return (char *)q;
}


#ifdef	debug
showall()
{
	struct store *p, *q;
	int i, used = 0, free = 0;

	for(p = &allocs[0] ; p && p!= alloct ; p = q) {
		q = p->ptr;
		printf("%4.4x %5d %s\n", p, i = (unsigned)q - (unsigned)p,
			testbusy(*p) ? "BUSY" : "FREE");
		if(testbusy(*p))
			used += i;
		else
			free += i;
	}
	printf("%d used, %d free, %4.4x end\n", used, free, alloct);
}
#endif

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
