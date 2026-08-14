#include "dextern"
#include "files"

/*
 * compute an array with the beginnings of productions yielding 
 * given nonterminals The array pres points to these lists
 */
cpres()
{
	int i, j, c;
	pres = yalloc(nnonter + 1);
	NTLOOP(i) {
		c = i + NTBASE;
		pres[i] = mem;
		fatfl = 0;				/* make undefined  symbols  nonfatal */
		PLOOP(0, j) {
			if (*prdptr[j] == c)
				*mem++ = prdptr[j] + 1;
		}
		if (pres[i] == mem) {
			error("nonterminal %s not defined!", nontrst[i].name);
		}
	}
	pres[i] = mem;
	fatfl = 1;
	if (nerrors) {
		summary();
		exit(1);
	}
}

#ifdef DEBUG
int indebug;
#endif
/*
 * compute an array with the first of nonterminals
 */
cpfir()
{
	register *p, **s, i, **t, ch, changes;

	zzcwp = &wsets[nnonter];
	pfirst = yalloc(nnonter + 1);
	NTLOOP(i) {
		aryfil(wsets[i].ws, tbitset, 0);
		t = pres[i + 1];
		for (s = pres[i]; s < t; ++s) {	/* initially fill the sets */
			for (p = *s; (ch = *p) > 0; ++p) {
				if (ch < NTBASE) {
					SETBIT(wsets[i].ws, ch);
					break;
				} else if (!pempty[ch - NTBASE])
					break;
			}
		}
	}

	/* now, reflect transitivity */

	changes = 1;
	while (changes) {
		changes = 0;
		NTLOOP(i) {
			t = pres[i + 1];
			for (s = pres[i]; s < t; ++s) {
				for (p = *s; (ch = (*p - NTBASE)) >= 0; ++p) {
					changes |= setunion(wsets[i].ws, wsets[ch].ws);
					if (!pempty[ch])
						break;
				}
			}
		}
	}

	NTLOOP(i) pfirst[i] = flset(wsets[i].ws);
#ifdef DEBUG
	if (!indebug)
		return;
	if ((foutput != NULL)) {
		NTLOOP(i) {
			fprintf(foutput, "\n%s: ", nontrst[i].name);
			prlook(pfirst[i]);
			fprintf(foutput, " %d\n", pempty[i]);
		}
	}
#endif
}

/*
 * sorts last state,and sees if it equals earlier ones. returns state number
 */
state(c)
{					
	int s, size1, size2;
	register i;
	struct item *p1, *p2, *k, *l, *q1, *q2;
	p1 = pstate[nstate];
	p2 = pstate[nstate + 1];
	if (p1 == p2)
		return (0);				/* null state */
	/* sort the items */
	for (k = p2 - 1; k > p1; k--) {	/* make k the biggest */
		for (l = k - 1; l >= p1; --l)
			if (l->pitem > k->pitem) {
				s = k->pitem;
				k->pitem = l->pitem;
				l->pitem = s;
				s = k->look;
				k->look = l->look;
				l->look = s;
			}
	}
	size1 = p2 - p1;			/* size of state */

	for (i = (c >= NTBASE) ? ntstates[c - NTBASE] : tstates[c]; i != 0;
		 i = mstates[i]) {
		/* get ith state */
		q1 = pstate[i];
		q2 = pstate[i + 1];
		size2 = q2 - q1;
		if (size1 != size2)
			continue;
		k = p1;
		for (l = q1; l < q2; l++) {
			if (l->pitem != k->pitem)
				break;
			++k;
		}
		if (l != q2)
			continue;
		/* found it */
		pstate[nstate + 1] = pstate[nstate];	/* delete last state */
		/* fix up lookaheads */
		if (nolook)
			return (i);
		for (l = q1, k = p1; l < q2; ++l, ++k) {
			SETLOOP(s) clset.lset[s] = l->look->lset[s];
			if (setunion(clset.lset, k->look->lset)) {
				tystate[i] = MUSTDO;
				/* register the new set */
				l->look = flset(&clset);
			}
		}
		return (i);
	}
	/* state is new */
	if (nolook)
		error("yacc state/nolook error");
	pstate[nstate + 2] = p2;
	if (nstate + 1 >= stsize)
		error("too many states");
	if (c >= NTBASE) {
		mstates[nstate] = ntstates[c - NTBASE];
		ntstates[c - NTBASE] = nstate;
	} else {
		mstates[nstate] = tstates[c];
		tstates[c] = nstate;
	}
	tystate[nstate] = MUSTDO;
	return (nstate++);
}

#ifdef DEBUG
int pidebug;					/* debugging flag for putitem */
#endif
putitem(ptr, lptr)
int *ptr;
struct looksets *lptr;
{
	int *jip;
	struct item *j;
	register size;

#ifdef DEBUG
	if (pidebug && (foutput != NULL)) {
		fprintf(foutput, "putitem(%s), state %d\n", writem(ptr), nstate);
	}
#endif
	j = pstate[nstate + 1];
	j->pitem = ptr;
	if (!nolook)
		j->look = flset(lptr);
	pstate[nstate + 1] = ++j;
	jip = j;
	if ((size = jip - mem0) >= zzmemsz) {
		zzmemsz = size;
		if (size >= memsiz)
			error("out of state space");
	}
}

/*
 * mark nonterminals which derive the empty string
 */
cempty()
{	

	int i, *p;

	/* set pempty to 0 */
	pempty = yalloc(nnonter);
	aryfil(pempty, nnonter + 1, 0);
	PLOOP(1, i) {
		if (prdptr[i][1] <= 0) {	/* i is empty production */
			pempty[*prdptr[i] - NTBASE] = 1;
		}
	}

	/* now, all explicitly empty nonterminals marked with pempty = 1 */

	/* loop as long as we keep finding nontrivial empty nonterminals */

again:
	PLOOP(1, i) {
		if (pempty[*prdptr[i] - NTBASE] == 0) {	/* not known to be empty */
			for (p = prdptr[i] + 1;
				 *p >= NTBASE && pempty[*p - NTBASE] != 0; ++p);
			if (*p < 0) {		/* we have a nontrivially empty nonterminal */
				pempty[*prdptr[i] - NTBASE] = 1;
				goto again;		/* got one ... try for another */
			}
		}
	}
}

#ifdef DEBUG
int gsdebug;
#endif
/*
 * generate the states
 */
stagen()
{	

	int i, j, c;
	struct wset *p, *q;

	/* initialize */

	nstate = 0;
	pstate[0] = pstate[1] = mem;
	aryfil(clset.lset, tbitset, 0);
	putitem(prdptr[0] + 1, &clset);
	tystate[0] = MUSTDO;
	nstate = 1;
	pstate[2] = pstate[1];

	aryfil(amem, actsiz, 0);

	/* now, the main state generation loop */

more:
	SLOOP(i) {
		if (tystate[i] != MUSTDO)
			continue;
		tystate[i] = DONE;
		aryfil(temp1, nnonter + 1, 0);
		/* take state i, close it, and do gotos */
		closure(i);
		WSLOOP(wsets, p) {		/* generate goto's */
			if (p->flag)
				continue;
			p->flag = 1;
			c = *(p->pitem);
			if (c <= 1) {
				if (pstate[i + 1] - pstate[i] <= p - wsets)
					tystate[i] = MUSTLOOKAHEAD;
				continue;
			}
			/* do a goto on c */
			WSLOOP(p, q) {
				if (c == *(q->pitem)) {	/* this item contributes to the goto */
					putitem(q->pitem + 1, q->ws);
					q->flag = 1;
				}
			}
			if (c < NTBASE) {
				state(c);		/* register new state */
			} else {
				temp1[c - NTBASE] = state(c);
			}
		}
#ifdef DEBUG
		if (gsdebug && (foutput != NULL)) {
			fprintf(foutput, "%d: ", i);
			NTLOOP(j) {
				if (temp1[j])
					fprintf(foutput, "%s %d, ", nontrst[j].name, temp1[j]);
			}
			fprintf(foutput, "\n");
		}
#endif
		indgo[i] = apack(&temp1[1], nnonter - 1) - 1;
		goto more;				/* we have done one goto; do some more */
	}
	/* no more to do... stop */
}

#ifdef DEBUG
int cldebug;					/* debugging flag for closure */
#endif
closure(i)
{								/* generate the closure of state i */

	int c, ch, work, k;
	register struct wset *u, *v;
	int *pi;
	int **s, **t;
	struct item *q;
	register struct item *p;

	++zzclose;

	/* first, copy kernel of state i to wsets */

	cwp = wsets;
	ITMLOOP(i, p, q) {
		cwp->pitem = p->pitem;
		cwp->flag = 1;			/* this item must get closed */
		SETLOOP(k) cwp->ws[k] = p->look->lset[k];
		WSBUMP(cwp);
	}

	/* now, go through the loop, closing each item */

	work = 1;
	while (work) {
		work = 0;
		WSLOOP(wsets, u) {

			if (u->flag == 0)
				continue;
			c = *(u->pitem);	/* dot is before c */

			if (c < NTBASE) {
				u->flag = 0;
				continue;		/* only interesting case is where . is before nonterminal */
			}

			/* compute the lookahead */
			aryfil(clset.lset, tbitset, 0);

			/* find items involving c */

			WSLOOP(u, v) {
				if (v->flag == 1 && *(pi = v->pitem) == c) {
					v->flag = 0;
					if (nolook)
						continue;
					while ((ch = *++pi) > 0) {
						if (ch < NTBASE) {	/* terminal symbol */
							SETBIT(clset.lset, ch);
							break;
						}
						/* nonterminal symbol */
						setunion(clset.lset, pfirst[ch - NTBASE]);
						if (!pempty[ch - NTBASE])
							break;
					}
					if (ch <= 0)
						setunion(clset.lset, v->ws);
				}
			}

			/*  now loop over productions derived from c */

			c -= NTBASE;		/* c is now nonterminal number */

			t = pres[c + 1];
			for (s = pres[c]; s < t; ++s) {
				/* put these items into the closure */
				WSLOOP(wsets, v) {	/* is the item there */
					if (v->pitem == *s) {	/* yes, it is there */
						if (nolook)
							goto nexts;
						if (setunion(v->ws, clset.lset))
							v->flag = work = 1;
						goto nexts;
					}
				}

				/*  not there; make a new entry */
				if (cwp - wsets + 1 >= wssize)
					error("working set overflow");
				cwp->pitem = *s;
				cwp->flag = 1;
				if (!nolook) {
					work = 1;
					SETLOOP(k) cwp->ws[k] = clset.lset[k];
				}
				WSBUMP(cwp);

nexts:			;
			}

		}
	}

	/* have computed closure; flags are reset; return */

	if (cwp > zzcwp)
		zzcwp = cwp;
#ifdef DEBUG
	if (cldebug && (foutput != NULL)) {
		fprintf(foutput, "\nState %d, nolook = %d\n", i, nolook);
		WSLOOP(wsets, u) {
			if (u->flag)
				fprintf(foutput, "flag set!\n");
			u->flag = 0;
			fprintf(foutput, "\t%s", writem(u->pitem));
			prlook(u->ws);
			fprintf(foutput, "\n");
		}
	}
#endif
}

struct looksets *
flset(p)
struct looksets *p;
{
	/* decide if the lookahead set pointed to by p is known */
	/* return pointer to a perminent location for the set */

	register struct looksets *q;
	int j, *w;
	register *u, *v;

	for (q = &lkst[nlset]; q-- > lkst;) {
		u = p->lset;
		v = q->lset;
		w = &v[tbitset];
		while (v < w)
			if (*u++ != *v++)
				goto more;
		/* we have matched */
		return (q);
more:	;
	}
	/* add a new one */
	q = &lkst[nlset++];
	if (nlset >= lsetsz)
		error("too many lookahead sets");
	SETLOOP(j) {
		q->lset[j] = p->lset[j];
	}
	return (q);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
