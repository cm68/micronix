#include <setjmp.h>

/*
 * ${}
 * `command`
 * blank interpretation
 * quoting
 * glob
 */
char **
eval(ap, f)
register char **ap;
int f;
{
	struct wdblock *wb;
	char **wp;
	char **wf;
	jmp_buf ev;

	wp = NULL;
	wb = NULL;
	wf = NULL;
	if (newenv(setjmp(errpt = ev)) == 0) {
		while (*ap && isassign(*ap))
			expand(*ap++, &wb, f & ~DOGLOB);
		if (flag['k']) {
			for (wf = ap; *wf; wf++) {
				if (isassign(*wf))
					expand(*wf, &wb, f & ~DOGLOB);
			}
		}
		for (wb = addword((char *)0, wb); *ap; ap++) {
			if (!flag['k'] || !isassign(*ap))
				expand(*ap, &wb, f & ~DOKEY);
		}
		wb = addword((char *)0, wb);
		wp = getwords(wb);
		quitenv();
	} else
		gflg = 1;
	return(gflg? (char **)NULL: wp);
}

/*
 * Make the exported environment from the exported
 * names in the dictionary. Keyword assignments
 * will already have been done.
 */
char **
makenv()

{
	register struct wdblock *wb;
	register struct var *vp;

	wb = NULL;
	for (vp = vlist; vp; vp = vp->next)
		if (vp->status & EXPORT)
			wb = addword(vp->name, wb);
	wb = addword((char *)0, wb);
	return(getwords(wb));
}

char *
evalstr(cp, f)
register char *cp;
int f;
{
	struct wdblock *wb;

	wb = NULL;
	if (expand(cp, &wb, f)) {
		if (wb == NULL || wb->w_nword == 0 || (cp = wb->w_words[0]) == NULL)
			cp = "";
		DELETE(wb);
	} else
		cp = NULL;
	return(cp);
}

static int
expand(cp, wbp, f)
register char *cp;
register struct wdblock **wbp;
int f;
{
	jmp_buf ev;

	gflg = 0;
	if (cp == NULL)
		return(0);
	if (!anys("$`'\"", cp) &&
	    !anys(ifs->value, cp) &&
	    ((f&DOGLOB)==0 || !anys("[*?", cp))) {
		cp = strsave(cp, areanum);
		if (f & DOTRIM)
			unquote(cp);
		*wbp = addword(cp, *wbp);
		return(1);
	}
	if (newenv(setjmp(errpt = ev)) == 0) {
		PUSHIO(aword, cp, strchar);
		e.iobase = e.iop;
		while ((cp = blank(f)) && gflg == 0) {
			e.linep = cp;
			cp = strsave(cp, areanum);
			if ((f&DOGLOB) == 0) {
				if (f & DOTRIM)
					unquote(cp);
				*wbp = addword(cp, *wbp);
			} else
				*wbp = glob(cp, *wbp);
		}
		quitenv();
	} else
		gflg = 1;
	return(gflg == 0);
}

/*
 * Blank interpretation and quoting
 */
static char *
blank(f)
int f;
{
	register c, c1;
	register char *sp;
	int scanequals, foundequals;

	sp = e.linep;
	scanequals = f & DOKEY;
	foundequals = 0;

loop:
	switch (c = subgetc('"', foundequals)) {
	case 0:
		if (sp == e.linep)
			return(0);
		*e.linep++ = 0;
		return(sp);

	default:
		if (f & DOBLANK && any(c, ifs->value))
			goto loop;
		break;

	case '"':
	case '\'':
		scanequals = 0;
		if (INSUB())
			break;
		for (c1 = c; (c = subgetc(c1, 1)) != c1;) {
			if (c == 0)
				break;
			if (c == '\'' || !any(c, "$`\""))
				c |= QUOTE;
			*e.linep++ = c;
		}
		c = 0;
	}
	unget(c);
	if (!letter(c))
		scanequals = 0;
	for (;;) {
		c = subgetc('"', foundequals);
		if (c == 0 ||
		    f & (DOBLANK && any(c, ifs->value)) ||
		    (!INSUB() && any(c, "\"'"))) {
		        scanequals = 0;
			unget(c);
			if (any(c, "\"'"))
				goto loop;
			break;
		}
		if (scanequals)
			if (c == '=') {
				foundequals = 1;
				scanequals  = 0;
			}
			else if (!letnum(c))
				scanequals = 0;
		*e.linep++ = c;
	}
	*e.linep++ = 0;
	return(sp);
}

/*
 * Get characters, substituting for ` and $
 */
int
subgetc(ec, quoted)
register char ec;
int quoted;
{
	register char c;

again:
	c = getc(ec);
	if (!INSUB() && ec != '\'') {
		if (c == '`') {
			if (grave(quoted) == 0)
				return(0);
			e.iop->task = XGRAVE;
			goto again;
		}
		if (c == '$' && (c = dollar(quoted)) == 0) {
			e.iop->task = XDOLL;
			goto again;
		}
	}
	return(c);
}

/*
 * Prepare to generate the string returned by ${} substitution.
 */
static int
dollar(quoted)
int quoted;
{
	int otask;
	struct io *oiop;
	char *dolp;
	register char *s, c, *cp;
	struct var *vp;

	c = readc();
	s = e.linep;
	if (c != '{') {
		*e.linep++ = c;
		if (letter(c)) {
			while ((c = readc())!=0 && letnum(c))
				if (e.linep < elinep)
					*e.linep++ = c;
			unget(c);
		}
		c = 0;
	} else {
		oiop = e.iop;
		otask = e.iop->task;
		e.iop->task = XOTHER;
		while ((c = subgetc('"', 0))!=0 && c!='}' && c!='\n')
			if (e.linep < elinep)
				*e.linep++ = c;
		if (oiop == e.iop)
			e.iop->task = otask;
		if (c != '}') {
			err("unclosed ${");
			gflg++;
			return(c);
		}
	}
	if (e.linep >= elinep) {
		err("string in ${} too long");
		gflg++;
		e.linep -= 10;
	}
	*e.linep = 0;
	if (*s)
		for (cp = s+1; *cp; cp++)
			if (any(*cp, "=-+?")) {
				c = *cp;
				*cp++ = 0;
				break;
			}
	if (s[1] == 0 && (*s == '*' || *s == '@')) {
		if (dolc > 1) {
			/* currently this does not distinguish $* and $@ */
			/* should check dollar */
			e.linep = s;
			PUSHIO(awordlist, dolv+1, dolchar);
			return(0);
		} else {	/* trap the nasty ${=} */
			s[0] = '1';
			s[1] = 0;
		}
	}
	vp = lookup(s);
	if ((dolp = vp->value) == null) {
		switch (c) {
		case '=':
			if (digit(*s)) {
				err("cannot use ${...=...} with $n");
				gflg++;
				break;
			}
			setval(vp, cp);
			dolp = vp->value;
			break;

		case '-':
			dolp = strsave(cp, areanum);
			break;

		case '?':
			if (*cp == 0) {
				prs("missing value for ");
				err(s);
			} else
				err(cp);
			gflg++;
			break;
		}
	} else if (c == '+')
		dolp = strsave(cp, areanum);
	if (flag['u'] && dolp == null) {
		prs("unset variable: ");
		err(s);
		gflg++;
	}
	e.linep = s;
	PUSHIO(aword, dolp, quoted ? qstrchar : strchar);
	return(0);
}

/*
 * Run the command in `...` and read its output.
 */
static int
grave(quoted)
int quoted;
{
	register char *cp;
	register int i;
	int pf[2];

	for (cp = e.iop->argp->aword; *cp != '`'; cp++)
		if (*cp == 0) {
			err("no closing `");
			return(0);
		}
	if (openpipe(pf) < 0)
		return(0);
	if ((i = fork()) == -1) {
		closepipe(pf);
		err("try again");
		return(0);
	}
	if (i != 0) {
		e.iop->argp->aword = ++cp;
		close(pf[1]);
		PUSHIO(afile, remap(pf[0]), quoted? qgravechar: gravechar);
		return(1);
	}
	*cp = 0;
	/* allow trapped signals */
	for (i=0; i<=_NSIG; i++)
		if (ourtrap[i] && signal(i, SIG_IGN) != SIG_IGN)
			signal(i, SIG_DFL);
	dup2(pf[1], 1);
	closepipe(pf);
	flag['e'] = 0;
	flag['v'] = 0;
	flag['n'] = 0;
	cp = strsave(e.iop->argp->aword, 0);
	areanum = 1;
	freehere(areanum);
	freearea(areanum);	/* free old space */
	e.oenv = NULL;
	e.iop = (e.iobase = iostack) - 1;
	unquote(cp);
	talking = 0;
	PUSHIO(aword, cp, nlchar);
	onecommand();
	exit(1);
}

char *
unquote(as)
register char *as;
{
	register char *s;

	if ((s = as) != NULL)
		while (*s)
			*s++ &= ~QUOTE;
	return(as);
}
