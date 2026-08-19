#ifndef lint
static char sccsid[] = "@(#)run.c	4.5 12/4/84";
#endif

#include "awk.def"
#include "awk.h"
#include "stdio.h"
#define RECSIZE BUFSIZ

#define FILENUM	10
struct
{
	FILE *fp;
	int type;
	char *fname;
} files[FILENUM];
FILE *popen();

extern obj execute(), nodetoobj(), fieldel(), dopa2(), gettemp();
extern obj (*proctab[])();
#define PA2NUM	29
int pairstack[PA2NUM], paircnt;
node *winner = (node *)NULL;
#define MAXTMP 20
cell tmps[MAXTMP];
static cell nullval ={EMPTY,EMPTY,0L,NUM,0};
obj	true	= objmk(0, OBOOL, BTRUE);
obj	false	= objmk(0, OBOOL, BFALSE);

run()
{
	register int i;

	execute(winner);

	/* Wait for children to complete if output to a pipe. */
	for (i=0; i<FILENUM; i++)
		if (files[i].fp && files[i].type == '|')
			pclose(files[i].fp);
}

obj execute(u) node *u;
{
	register obj (*proc)();
	obj x;
	node *a;
	extern char *printname[];

	if (u==(node *)NULL)
		return(true);
	for (a = u; ; a = a->nnext) {
		if (cantexec(a))
			return(nodetoobj(a));
		if (a->ntype==NPA2)
			proc=dopa2;
		else {
			if (notlegal(a->nobj))
				error(FATAL, "illegal statement %o", a);
			proc = proctab[a->nobj-FIRSTTOKEN];
		}
		x = (*proc)(a->narg,a->nobj);
		if (isfld(x)) fldbld();
		if (isexpr(a))
			return(x);
		/* a statement, goto next statement */
		if (isjump(x))
			return(x);
		if (a->nnext == (node *)NULL)
			return(x);
		tempfree(x);
	}
}

obj program(a, n) node **a;
{
	obj x;

	if (a[0] != NULL) {
		x = execute(a[0]);
		if (isexit(x))
			return(true);
		if (isjump(x))
			error(FATAL, "unexpected break, continue or next");
		tempfree(x);
	}
	while (getrec()) {
		x = execute(a[1]);
		if (isexit(x)) break;
		tempfree(x);
	}
	tempfree(x);
	if (a[2] != NULL) {
		x = execute(a[2]);
		if (isbreak(x) || isnext(x) || iscont(x))
			error(FATAL, "unexpected break, continue or next");
		tempfree(x);
	}
	return(true);
}

obj awkgetline()
{
	obj x;

	x = gettemp();
	setfval(objptr(x), itof(getrec()));
	return(x);
}

obj array(a,n) node **a;
{
	obj x, y;
	extern obj arrayel();

	x = execute(a[1]);
	y = arrayel(a[0], x);
	tempfree(x);
	return(y);
}

obj arrayel(a,b) node *a; obj b;
{
	char *s;
	cell *x;
	int i;
	obj y;

	s = getsval(objptr(b));
	x = (cell *) a;
	if (!(x->tval&ARR)) {
		strfree(x->sval);
		x->tval &= ~STR;
		x->tval |= ARR;
		x->sval = (char *) makesymtab();
	}
	y = objmk(setsymtab(s, tostring(""), 0L, STR|NUM, x->sval), objtype(y), objsub(y));
	y = objmk(objptr(y), OCELL, objsub(y));
	y = objmk(objptr(y), objtype(y), CVAR);
	return(y);
}

obj matchop(a,n) node **a;
{
	obj x;
	char *s;
	int i;

	x = execute(a[0]);
	if (isstr(x)) s = objptr(x)->sval;
	else	s = getsval(objptr(x));
	tempfree(x);
	i = match(a[1], s);
	if (n==MATCH && i==1 || n==NOTMATCH && i==0)
		return(true);
	else
		return(false);
}

obj boolop(a,n) node **a;
{
	obj x, y;
	int i;

	x = execute(a[0]);
	i = istrue(x);
	tempfree(x);
	switch (n) {
	default:
		error(FATAL, "unknown boolean operator %d", n);
	case BOR:
		if (i) return(true);
		y = execute(a[1]);
		i = istrue(y);
		tempfree(y);
		if (i) return(true);
		else return(false);
	case AND:
		if ( !i ) return(false);
		y = execute(a[1]);
		i = istrue(y);
		tempfree(y);
		if (i) return(true);
		else return(false);
	case NOT:
		if (i) return(false);
		else return(true);
	}
}

obj relop(a,n) node **a;
{
	int i;
	obj x, y;

	x = execute(a[0]);
	y = execute(a[1]);
	if (objptr(x)->tval&NUM && objptr(y)->tval&NUM) {
		i = fcmp(objptr(x)->fval, objptr(y)->fval);
	} else {
		i = strcmp(getsval(objptr(x)), getsval(objptr(y)));
	}
	tempfree(x);
	tempfree(y);
	switch (n) {
	default:
		error(FATAL, "unknown relational operator %d", n);
	case LT:	if (i<0) return(true);
			else return(false);
	case LE:	if (i<=0) return(true);
			else return(false);
	case NE:	if (i!=0) return(true);
			else return(false);
	case EQ:	if (i==0) return(true);
			else return(false);
	case GE:	if (i>=0) return(true);
			else return(false);
	case GT:	if (i>0) return(true);
			else return(false);
	}
}

tempfree(a) obj a;
{
	if (!istemp(a)) return;
	strfree(objptr(a)->sval);
	objptr(a)->tval = 0;
}

obj gettemp()
{
	int i;
	obj x;
	cell *tp;

	for (i=0; i<MAXTMP; i++)
		if (tmps[i].tval==0)
			break;
	if (i==MAXTMP)
		error(FATAL, "out of temporaries in gettemp");
	tp = &tmps[i];
	x = objmk(tp, OCELL, CTEMP);
	tmps[i].nval = EMPTY;
	tmps[i].sval = EMPTY;
	tmps[i].fval = 0L;
	tmps[i].tval = NUM;
	tmps[i].nextval = 0;
	return(x);
}

obj indirect(a,n) node **a;
{
	obj x;
	int m;
	cell *fieldadr();

	x = execute(a[0]);
	m = getfval(objptr(x));
	tempfree(x);
	x = objmk(fieldadr(m), objtype(x), objsub(x));
	x = objmk(objptr(x), OCELL, objsub(x));
	x = objmk(objptr(x), objtype(x), CFLD);
	return(x);
}

obj substr(a, nnn) node **a;
{
	char *s, temp;
	obj x;
	int k, m, n;

	x = execute(a[0]);
	s = getsval(objptr(x));
	k = strlen(s) + 1;
	tempfree(x);
	x = execute(a[1]);
	m = getfval(objptr(x));
	if (m <= 0)
		m = 1;
	else if (m > k)
		m = k;
	tempfree(x);
	if (a[2] != nullstat) {
		x = execute(a[2]);
		n = getfval(objptr(x));
		tempfree(x);
	}
	else
		n = k - 1;
	if (n < 0)
		n = 0;
	else if (n > k - m)
		n = k - m;
	dprintf("substr: m=%d, n=%d, s=%s\n", m, n, s);
	x = gettemp();
	temp = s[n+m-1];	/* with thanks to John Linderman */
	s[n+m-1] = '\0';
	setsval(objptr(x), s + m - 1);
	s[n+m-1] = temp;
	return(x);
}

obj sindex(a, nnn) node **a;
{
	obj x;
	char *s1, *s2, *p1, *p2, *q;

	x = execute(a[0]);
	s1 = getsval(objptr(x));
	tempfree(x);
	x = execute(a[1]);
	s2 = getsval(objptr(x));
	tempfree(x);

	x = gettemp();
	for (p1 = s1; *p1 != '\0'; p1++) {
		for (q=p1, p2=s2; *p2 != '\0' && *q == *p2; q++, p2++)
			;
		if (*p2 == '\0') {
			setfval(objptr(x), itof(p1 - s1 + 1));	/* origin 1 */
			return(x);
		}
	}
	setfval(objptr(x), 0L);
	return(x);
}

char *format(s,a) char *s; node *a;
{
	char *buf, *p, fmt[200], *t, *os;
	obj x;
	int flag = 0;
	awkfloat xf;

	os = s;
	p = buf = (char *)malloc(RECSIZE);
	while (*s) {
		if (*s != '%') {
			*p++ = *s++;
			continue;
		}
		if (*(s+1) == '%') {
			*p++ = '%';
			s += 2;
			continue;
		}
		for (t=fmt; (*t++ = *s) != '\0'; s++)
			if (*s >= 'a' && *s <= 'z' && *s != 'l')
				break;
		*t = '\0';
		if (t >= fmt + sizeof(fmt))
			error(FATAL, "format item %.20s... too long", os);
		switch (*s) {
		case 'f': case 'e': case 'g':
			flag = 1;
			break;
		case 'd':
			flag = 2;
			if(*(s-1) == 'l') break;
			*(t-1) = 'l';
			*t = 'd';
			*++t = '\0';
			break;
		case 'o': case 'x':
			flag = *(s-1)=='l' ? 2 : 3;
			break;
		case 'c':
			flag = 3;
			break;
		case 's':
			flag = 4;
			break;
		default:
			flag = 0;
			break;
		}
		if (flag == 0) {
			sprintf(p, "%s", fmt);
			p += strlen(p);
			continue;
		}
		if (a == NULL)
			error(FATAL, "not enough arguments in printf(%s)", os);
		x = execute(a);
		a = a->nnext;
		if (flag != 4)	/* watch out for converting to numbers! */
			xf = getfval(objptr(x));
		if (flag==1) ftoa(p, xf, 6);
		else if (flag==2) sprintf(p, fmt, (long)ftoi(xf));
		else if (flag==3) sprintf(p, fmt, (int)ftoi(xf));
		else if (flag==4) sprintf(p, fmt, objptr(x)->sval==NULL ? "" : getsval(objptr(x)));
		tempfree(x);
		p += strlen(p);
		s++;
	}
	*p = '\0';
	return(buf);
}

obj asprintf(a,n) node **a;
{
	obj x;
	node *y;
	char *s;

	y = a[0]->nnext;
	x = execute(a[0]);
	s = format(getsval(objptr(x)), y);
	tempfree(x);
	x = gettemp();
	objptr(x)->sval = s;
	objptr(x)->tval = STR;
	return(x);
}

obj arith(a,n) node **a;
{
	awkfloat i,j;
	obj x,y,z;

	x = execute(a[0]);
	i = getfval(objptr(x));
	tempfree(x);
	if (n != UMINUS) {
		y = execute(a[1]);
		j = getfval(objptr(y));
		tempfree(y);
	}
	z = gettemp();
	switch (n) {
	default:
		error(FATAL, "illegal arithmetic operator %d", n);
	case ADD:
		i = fadd(i, j);
		break;
	case MINUS:
		i = fsub(i, j);
		break;
	case MULT:
		i = fmul(i, j);
		break;
	case DIVIDE:
		if (j == 0)
			error(FATAL, "division by zero");
		i = fdiv(i, j);
		break;
	case MOD:
		if (j == 0)
			error(FATAL, "division by zero");
		i = fmod(i, j);
		break;
	case UMINUS:
		i = fneg(i);
		break;
	}
	setfval(objptr(z), i);
	return(z);
}

obj incrdecr(a, n) node **a;
{
	obj x, z;
	int k;
	awkfloat xf;

	x = execute(a[0]);
	xf = getfval(objptr(x));
	k = (n == PREINCR || n == POSTINCR) ? 1 : -1;
	if (n == PREINCR || n == PREDECR) {
		setfval(objptr(x), fadd(xf, itof(k)));
		return(x);
	}
	z = gettemp();
	setfval(objptr(z), xf);
	setfval(objptr(x), fadd(xf, itof(k)));
	tempfree(x);
	return(z);
}


obj assign(a,n) node **a;
{
	obj x, y;
	awkfloat xf, yf;

	x = execute(a[0]);
	y = execute(a[1]);
	if (n == ASSIGN) {	/* ordinary assignment */
		if ((objptr(y)->tval & (STR|NUM)) == (STR|NUM)) {
			setsval(objptr(x), objptr(y)->sval);
			objptr(x)->fval = objptr(y)->fval;
			objptr(x)->tval |= NUM;
		}
		else if (objptr(y)->tval & STR)
			setsval(objptr(x), objptr(y)->sval);
		else if (objptr(y)->tval & NUM)
			setfval(objptr(x), objptr(y)->fval);
		tempfree(y);
		return(x);
	}
	xf = getfval(objptr(x));
	yf = getfval(objptr(y));
	switch (n) {
	case ADDEQ:
		xf = fadd(xf, yf);
		break;
	case SUBEQ:
		xf = fsub(xf, yf);
		break;
	case MULTEQ:
		xf = fmul(xf, yf);
		break;
	case DIVEQ:
		if (yf == 0)
			error(FATAL, "division by zero");
		xf = fdiv(xf, yf);
		break;
	case MODEQ:
		if (yf == 0)
			error(FATAL, "division by zero");
		xf = fmod(xf, yf);
		break;
	default:
		error(FATAL, "illegal assignment operator %d", n);
		break;
	}
	tempfree(y);
	setfval(objptr(x), xf);
	return(x);
}

obj cat(a,q) node **a;
{
	obj x,y,z;
	int n1, n2;
	char *s;

	x = execute(a[0]);
	y = execute(a[1]);
	getsval(objptr(x));
	getsval(objptr(y));
	n1 = strlen(objptr(x)->sval);
	n2 = strlen(objptr(y)->sval);
	s = (char *) malloc(n1 + n2 + 1);
	strcpy(s, objptr(x)->sval);
	strcpy(s+n1, objptr(y)->sval);
	tempfree(y);
	z = gettemp();
	objptr(z)->sval = s;
	objptr(z)->tval = STR;
	tempfree(x);
	return(z);
}

obj pastat(a,n) node **a;
{
	obj x;

	if (a[0]==nullstat)
		x = true;
	else
		x = execute(a[0]);
	if (istrue(x)) {
		tempfree(x);
		x = execute(a[1]);
	}
	return(x);
}

obj dopa2(a,n) node **a;
{
	obj x;

	if (pairstack[n]==0) {
		x = execute(a[0]);
		if (istrue(x))
			pairstack[n] = 1;
		tempfree(x);
	}
	if (pairstack[n] == 1) {
		x = execute(a[1]);
		if (istrue(x))
			pairstack[n] = 0;
		tempfree(x);
		x = execute(a[2]);
		return(x);
	}
	return(false);
}

obj aprintf(a,n) node **a;
{
	obj x;

	x = asprintf(a,n);
	if (a[1]==NULL) {
		printf("%s", objptr(x)->sval);
		tempfree(x);
		return(true);
	}
	redirprint(objptr(x)->sval, (int)a[1], a[2]);
	return(x);
}

obj split(a,nnn) node **a;
{
	obj x;
	cell *ap;
	register char *s, *p;
	char *t, temp, num[5];
	register int sep;
	int n, flag;

	x = execute(a[0]);
	s = getsval(objptr(x));
	tempfree(x);
	if (a[2] == nullstat)
		sep = **FS;
	else {
		x = execute(a[2]);
		sep = getsval(objptr(x))[0];
		tempfree(x);
	}
	ap = (cell *) a[1];
	freesymtab(ap);
	dprintf("split: s=|%s|, a=%s, sep=|%c|\n", s, ap->nval, sep);
	ap->tval &= ~STR;
	ap->tval |= ARR;
	ap->sval = (char *) makesymtab();

	n = 0;
	if (sep == ' ')
		for (n = 0; ; ) {
			while (*s == ' ' || *s == '\t' || *s == '\n')
				s++;
			if (*s == 0)
				break;
			n++;
			t = s;
			do
				s++;
			while (*s!=' ' && *s!='\t' && *s!='\n' && *s!='\0');
			temp = *s;
			*s = '\0';
			sprintf(num, "%d", n);
			if (isnumber(t))
				setsymtab(num, tostring(t), fatof(t), STR|NUM, ap->sval);
			else
				setsymtab(num, tostring(t), 0L, STR, ap->sval);
			*s = temp;
			if (*s != 0)
				s++;
		}
	else if (*s != 0)
		for (;;) {
			n++;
			t = s;
			while (*s != sep && *s != '\n' && *s != '\0')
				s++;
			temp = *s;
			*s = '\0';
			sprintf(num, "%d", n);
			if (isnumber(t))
				setsymtab(num, tostring(t), fatof(t), STR|NUM, ap->sval);
			else
				setsymtab(num, tostring(t), 0L, STR, ap->sval);
			*s = temp;
			if (*s++ == 0)
				break;
		}
	x = gettemp();
	objptr(x)->tval = NUM;
	objptr(x)->fval = n;
	return(x);
}

obj ifstat(a,n) node **a;
{
	obj x;

	x = execute(a[0]);
	if (istrue(x)) {
		tempfree(x);
		x = execute(a[1]);
	}
	else if (a[2] != nullstat) {
		tempfree(x);
		x = execute(a[2]);
	}
	return(x);
}

obj whilestat(a,n) node **a;
{
	obj x;

	for (;;) {
		x = execute(a[0]);
		if (!istrue(x)) return(x);
		tempfree(x);
		x = execute(a[1]);
		if (isbreak(x)) {
			x = true;
			return(x);
		}
		if (isnext(x) || isexit(x))
			return(x);
		tempfree(x);
	}
}

obj forstat(a,n) node **a;
{
	obj x;

	tempfree(execute(a[0]));
	for (;;) {
		if (a[1]!=nullstat) {
			x = execute(a[1]);
			if (!istrue(x)) return(x);
			else tempfree(x);
		}
		x = execute(a[3]);
		if (isbreak(x)) {	/* turn off break */
			x = true;
			return(x);
		}
		if (isnext(x) || isexit(x))
			return(x);
		tempfree(x);
		tempfree(execute(a[2]));
	}
}

obj instat(a, n) node **a;
{
	cell *vp, *arrayp, *cp, **tp;
	obj x;
	int i;

	vp = (cell *) a[0];
	arrayp = (cell *) a[1];
	if (!(arrayp->tval & ARR))
		error(FATAL, "%s is not an array", arrayp->nval);
	tp = (cell **) arrayp->sval;
	for (i = 0; i < MAXSYM; i++) {	/* this routine knows too much */
		for (cp = tp[i]; cp != NULL; cp = cp->nextval) {
			setsval(vp, cp->nval);
			x = execute(a[2]);
			if (isbreak(x)) {
				x = true;
				return(x);
			}
			if (isnext(x) || isexit(x))
				return(x);
			tempfree(x);
		}
	}
	return (true);
}

obj jump(a,n) node **a;
{
	obj x, y;

	x = objmk(objptr(x), OJUMP, objsub(x));
	switch (n) {
	default:
		error(FATAL, "illegal jump type %d", n);
		break;
	case EXIT:
		if (a[0] != 0) {
			y = execute(a[0]);
			errorflag = getfval(objptr(y));
		}
		x = objmk(objptr(x), objtype(x), JEXIT);
		break;
	case NEXT:
		x = objmk(objptr(x), objtype(x), JNEXT);
		break;
	case BREAK:
		x = objmk(objptr(x), objtype(x), JBREAK);
		break;
	case CONTINUE:
		x = objmk(objptr(x), objtype(x), JCONT);
		break;
	}
	return(x);
}

obj fncn(a,n) node **a;
{
	obj x;
	awkfloat u;
	int t;

	t = (int) a[0];
	x = execute(a[1]);
	if (t == FLENGTH)
		u = itof(strlen(getsval(objptr(x))));
	else if (t == FLOG)
		u = ilog(getfval(objptr(x)));
	else if (t == FINT)
		u = itof(ftrunc(getfval(objptr(x))));
	else if (t == FEXP)
		u = iexp(getfval(objptr(x)));
	else if (t == FSQRT)
		u = isqrt(getfval(objptr(x)));
	else
		error(FATAL, "illegal function type %d", t);
	tempfree(x);
	x = gettemp();
	setfval(objptr(x), u);
	return(x);
}

obj print(a,n) node **a;
{
	register node *x;
	obj y;
	char s[RECSIZE];

	s[0] = '\0';
	for (x=a[0]; x!=NULL; x=x->nnext) {
		y = execute(x);
		strcat(s, getsval(objptr(y)));
		tempfree(y);
		if (x->nnext==NULL)
			strcat(s, *ORS);
		else
			strcat(s, *OFS);
	}
	if (strlen(s) >= RECSIZE)
		error(FATAL, "string %.20s ... too long to print", s);
	if (a[1]==nullstat) {
		printf("%s", s);
		return(true);
	}
	redirprint(s, (int)a[1], a[2]);
	return(false);
}

obj nullproc() {}

obj nodetoobj(a) node *a;
{
	obj x;

	x = objmk((cell *) a->nobj, objtype(x), objsub(x));
	x = objmk(objptr(x), OCELL, objsub(x));
	x = objmk(objptr(x), objtype(x), a->subtype);
	if (isfld(x)) fldbld();
	return(x);
}

redirprint(s, a, b) char *s; node *b;
{
	register int i;
	obj x;

	x = execute(b);
	getsval(objptr(x));
	for (i=0; i<FILENUM; i++)
		if (files[i].fp && strcmp(objptr(x)->sval, files[i].fname) == 0)
			goto doit;
	for (i=0; i<FILENUM; i++)
		if (files[i].fp == 0)
			break;
	if (i >= FILENUM)
		error(FATAL, "too many output files %d", i);
	if (a == '|')	/* a pipe! */
		files[i].fp = popen(objptr(x)->sval, "w");
	else if (a == APPEND)
		files[i].fp = fopen(objptr(x)->sval, "a");
	else
		files[i].fp = fopen(objptr(x)->sval, "w");
	if (files[i].fp == NULL)
		error(FATAL, "can't open file %s", objptr(x)->sval);
	files[i].fname = tostring(objptr(x)->sval);
	files[i].type = a;
doit:
	fprintf(files[i].fp, "%s", s);
#ifndef gcos
	fflush(files[i].fp);	/* in case someone is waiting for the output */
#endif
	tempfree(x);
}
