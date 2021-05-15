/* -------- glob.c -------- */
/* #include "sh.h" */

/*
 * glob
 */

#define	scopy(x) strsave((x), areanum)
#define	BLKSIZ	512
#define	NDENT	((BLKSIZ+sizeof(struct direct)-1)/sizeof(struct direct))

static	struct wdblock	*cl, *nl;
static	char	spcl[] = "[?*";

struct wdblock *
glob(cp, wb)
char *cp;
struct wdblock *wb;
{
	register i;
	register char *pp;

	if (cp == 0)
		return(wb);
	i = 0;
	for (pp = cp; *pp; pp++)
		if (any(*pp, spcl))
			i++;
		else if (!any(*pp & ~QUOTE, spcl))
			*pp &= ~QUOTE;
	if (i != 0) {
		for (cl = addword(scopy(cp), (struct wdblock *)0); anyspcl(cl); cl = nl) {
			nl = newword(cl->w_nword*2);
			for(i=0; i<cl->w_nword; i++) { /* for each argument */
				for (pp = cl->w_words[i]; *pp; pp++)
					if (any(*pp, spcl)) {
						globname(cl->w_words[i], pp);
						break;
					}
				if (*pp == '\0')
					nl = addword(scopy(cl->w_words[i]), nl);
			}
			for(i=0; i<cl->w_nword; i++)
				DELETE(cl->w_words[i]);
			DELETE(cl);
		}
		for(i=0; i<cl->w_nword; i++)
			unquote(cl->w_words[i]);
		glob0((char *)cl->w_words, cl->w_nword, sizeof(char *), xstrcmp);
		if (cl->w_nword) {
			for (i=0; i<cl->w_nword; i++)
				wb = addword(cl->w_words[i], wb);
			DELETE(cl);
			return(wb);
		}
	}
	wb = addword(unquote(cp), wb);
	return(wb);
}

void
globname(we, pp)
char *we;
register char *pp;
{
	register char *np, *cp;
	char *name, *gp, *dp;
	int dn, j, n, k;
	struct direct ent[NDENT];
	char dname[NAME_MAX+1];
	struct stat dbuf;

	for (np = we; np != pp; pp--)
		if (pp[-1] == '/')
			break;
	for (dp = cp = space((int)(pp-np)+3); np < pp;)
		*cp++ = *np++;
	*cp++ = '.';
	*cp = '\0';
	for (gp = cp = space(strlen(pp)+1); *np && *np != '/';)
		*cp++ = *np++;
	*cp = '\0';
	dn = open(dp, 0);
	if (dn < 0) {
		DELETE(dp);
		DELETE(gp);
		return;
	}
	dname[NAME_MAX] = '\0';
	while ((n = read(dn, (char *)ent, sizeof(ent))) >= sizeof(*ent)) {
		n /= sizeof(*ent);
		for (j=0; j<n; j++) {
			if (ent[j].d_ino == 0)
				continue;
			strncpy(dname, ent[j].d_name, NAME_MAX);
			if (dname[0] == '.')
				if (*gp != '.')
					continue;
			for(k=0; k<NAME_MAX; k++)
				if (any(dname[k], spcl))
					dname[k] |= QUOTE;
			if (gmatch(dname, gp)) {
				name = generate(we, pp, dname, np);
				if (*np && !anys(np, spcl)) {
					if (stat(name,&dbuf)) {
						DELETE(name);
						continue;
					}
				}
				nl = addword(name, nl);
			}
		}
	}
	close(dn);
	DELETE(dp);
	DELETE(gp);
}

/*
 * generate a pathname as below.
 * start..end1 / middle end
 * the slashes come for free
 */
static char *
generate(start1, end1, middle, end)
char *start1;
register char *end1;
char *middle, *end;
{
	char *p;
	register char *op, *xp;

	p = op = space((int)(end1-start1)+strlen(middle)+strlen(end)+2);
	for (xp = start1; xp != end1;)
		*op++ = *xp++;
	for (xp = middle; (*op++ = *xp++) != '\0';)
		;
	op--;
	for (xp = end; (*op++ = *xp++) != '\0';)
		;
	return(p);
}

static int
anyspcl(wb)
register struct wdblock *wb;
{
	register i;
	register char **wd;

	wd = wb->w_words;
	for (i=0; i<wb->w_nword; i++)
		if (anys(spcl, *wd++))
			return(1);
	return(0);
}

static int
xstrcmp(p1, p2)
char *p1, *p2;
{
	return(strcmp(*(char **)p1, *(char **)p2));
}
