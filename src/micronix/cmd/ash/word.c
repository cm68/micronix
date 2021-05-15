/* -------- word.c -------- */
/* #include "sh.h" */
/* #include "word.h" */

#define	NSTART	16	/* default number of words to allow for initially */

struct wdblock *
newword(nw)
register int nw;
{
	register struct wdblock *wb;

	wb = (struct wdblock *) space(sizeof(*wb) + nw*sizeof(char *));
	wb->w_bsize = nw;
	wb->w_nword = 0;
	return(wb);
}

struct wdblock *
addword(wd, wb)
char *wd;
register struct wdblock *wb;
{
	register struct wdblock *wb2;
	register nw;

	if (wb == NULL)
		wb = newword(NSTART);
	if ((nw = wb->w_nword) >= wb->w_bsize) {
		wb2 = newword(nw * 2);
		memcopy((char *)wb2->w_words, (char *)wb->w_words, nw*sizeof(char *));
		wb2->w_nword = nw;
		DELETE(wb);
		wb = wb2;
	}
	wb->w_words[wb->w_nword++] = wd;
	return(wb);
}

char **
getwords(wb)
register struct wdblock *wb;
{
	register char **wd;
	register nb;

	if (wb == NULL)
		return((char **)NULL);
	if (wb->w_nword == 0) {
		DELETE(wb);
		return((char **)NULL);
	}
	wd = (char **) space(nb = sizeof(*wd) * wb->w_nword);
	memcopy((char *)wd, (char *)wb->w_words, nb);
	DELETE(wb);	/* perhaps should done by caller */
	return(wd);
}

_PROTOTYPE(int (*func), (char *, char *));
int	globv;

void
glob0(a0, a1, a2, a3)
char *a0;
unsigned a1;
int a2;
_PROTOTYPE(int (*a3), (char *, char *));
{
	func = a3;
	globv = a2;
	glob1(a0, a0 + a1 * a2);
}

void
glob1(base, lim)
char *base, *lim;
{
	register char *i, *j;
	int v2;
	char *lptr, *hptr;
	int c;
	unsigned n;


	v2 = globv;

top:
	if ((n=(int)(lim-base)) <= v2)
		return;
	n = v2 * (n / (2*v2));
	hptr = lptr = base+n;
	i = base;
	j = lim-v2;
	for(;;) {
		if (i < lptr) {
			if ((c = (*func)(i, lptr)) == 0) {
				glob2(i, lptr -= v2);
				continue;
			}
			if (c < 0) {
				i += v2;
				continue;
			}
		}

begin:
		if (j > hptr) {
			if ((c = (*func)(hptr, j)) == 0) {
				glob2(hptr += v2, j);
				goto begin;
			}
			if (c > 0) {
				if (i == lptr) {
					glob3(i, hptr += v2, j);
					i = lptr += v2;
					goto begin;
				}
				glob2(i, j);
				j -= v2;
				i += v2;
				continue;
			}
			j -= v2;
			goto begin;
		}


		if (i == lptr) {
			if (lptr-base >= lim-hptr) {
				glob1(hptr+v2, lim);
				lim = lptr;
			} else {
				glob1(base, lptr);
				base = hptr+v2;
			}
			goto top;
		}


		glob3(j, lptr -= v2, i);
		j = hptr -= v2;
	}
}

void
glob2(i, j)
char *i, *j;
{
	register char *index1, *index2, c;
	int m;

	m = globv;
	index1 = i;
	index2 = j;
	do {
		c = *index1;
		*index1++ = *index2;
		*index2++ = c;
	} while(--m);
}

void
glob3(i, j, k)
char *i, *j, *k;
{
	register char *index1, *index2, *index3;
	int c;
	int m;

	m = globv;
	index1 = i;
	index2 = j;
	index3 = k;
	do {
		c = *index1;
		*index1++ = *index3;
		*index3++ = *index2;
		*index2++ = c;
	} while(--m);
}

char *
memcopy(ato, from, nb)
register char *ato, *from;
register int nb;
{
	register char *to;

	to = ato;
	while (--nb >= 0)
		*to++ = *from++;
	return(ato);
}
