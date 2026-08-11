/*
 * permanent allocation arena.
 *
 * objects that live until exit (symbol tables, type nodes, interned
 * strings, macro definitions) skip malloc's per-block header and
 * free list: carve them out of big malloc'd chunks instead.  nothing
 * handed out here can ever be freed.  chunk bases chain through the
 * first word so the arena stays walkable (and provably reachable
 * under valgrind).  running out of memory is fatal.
 *
 * permalloc: pointer-aligned and zeroed (structs, pointer arrays)
 * permdup:   unaligned string copy, packed tight
 */
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*
 * 128, halved three times from 1024: the grab size is also the
 * granularity of the last allocation before the sbrk guard says no,
 * and on the 64K machine each halving has bought back a source file
 * that died within a chunk of fitting - first cpp on a big header
 * closure, then cpp on pass1/expr.c once the typedef filter moved
 * in, then cpp again when constant folding moved in.
 */
#define PCHUNK 128

static char *pnext;			/* cursor into current chunk */
static unsigned int pleft;	/* bytes left in current chunk */
static char *pchain;		/* chunk bases, linked through word 0 */

static void
pgrab(unsigned int n)
{
	unsigned int chunk;
	char *p;

	n += sizeof(char *);
	chunk = n > PCHUNK ? n : PCHUNK;
	p = malloc(chunk);
	if (p == 0) {
		write(2, "out of memory\n", 14);
		exit(1);
	}
	*(char **)p = pchain;
	pchain = p;
	pnext = p + sizeof(char *);
	pleft = chunk - sizeof(char *);
}

char *
permalloc(unsigned int n)
{
	char *p;
	unsigned int pad;

	pad = (unsigned int)(sizeof(char *) - 1) & -(unsigned int)pnext;
	if (n + pad > pleft) {
		pgrab(n);
		pad = 0;
	}
	p = pnext + pad;
	pnext = p + n;
	pleft -= n + pad;
	memset(p, 0, n);
	return p;
}

char *
permdup(char *s)
{
	unsigned int n = strlen(s) + 1;
	char *p;

	if (n > pleft)
		pgrab(n);
	p = pnext;
	pnext += n;
	pleft -= n;
	return strcpy(p, s);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
