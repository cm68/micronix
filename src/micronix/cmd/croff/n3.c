#include "tdef.h"
#include "t.h"
#include "tw.h"

/*
troff3.c

macro and string routines, storage allocation
*/

#define NBLIST 256	/*allocation list*/
#define BLK  128	/*alloc block words*/
/* BLK*NBLIST=32768 words */

extern int ch;
extern int ibf;
extern int nextb;
extern char *enda;
extern int lgf;
extern int copyf;
extern int ch0;
extern int ip;
extern int app;
extern int ds;
extern int nlflg;
extern int *nxf;
extern int *argtop;
extern int *ap;
extern int nchar;
extern int *frame;
extern int *stk;
extern int pendt;
extern int rchar;
extern int dilev;
extern int *dip;
extern int nonumb;
extern int lt;
extern int nrbits;
extern int nform;
extern int fmt[];
extern int oldmn;
extern int newmn;
extern int macerr;
extern int apptr;
extern int offset;
extern int aplnk;
extern int diflg;
extern int woff;
extern int roff;
extern int wbfi;
extern int po;
extern int *cp;
extern int xxx;
int pagech '%';
int strflg;
extern struct contab {
	int rq;
	int (*f)();
}contab[NM];
int blist[NBLIST];
int wbuf[BLK];
int rbuf[BLK];
static char Sccsid[] "@(#)n3.c	1.3 of 3/29/77";

caseig(){
	register i;

	offset = 0;
	if((i = copyb()) != '.')control(i,1);
}
casern(){
	register i,j;

	lgf++;
	skip();
	if(((i=getrq())==0) || ((oldmn=findmn(i)) < 0))return;
	skip();
	clrmn(findmn(j=getrq()));
	if(j)contab[oldmn].rq = (contab[oldmn].rq & MMASK) | j;
}
caserm(){
	lgf++;
	skip();
	clrmn(findmn(getrq()));
}
caseas(){
	app++;
	caseds();
}
caseds(){
	ds++;
	casede();
}
caseam(){
	app++;
	casede();
}
casede(){
	register i, savoff, req;

	if(dip->op)wbfl();
	req = '.';
	lgf++;
	skip();
	if((i=getrq())==0)goto de1;
	if((offset=finds(i)) == 0)goto de1;
	if(ds)copys();
		else req = copyb();
	wbfl();
	clrmn(oldmn);
	if(newmn)contab[newmn].rq = i | MMASK;
	if(apptr){
		savoff = offset;
		offset = apptr;
		wbt(IMP);
		offset = savoff;
	}
	offset = dip->op;
	if(req != '.')control(req,1);
de1:
	ds = app = 0;
	return;
}
findmn(i)
int i;
{
	register j;

	for(j=0;j<NM;j++){
		if(i == (contab[j].rq & ~MMASK))break;
	}
	if(j==NM)j = -1;
	return(j);
}
clrmn(i)
int i;
{
	if(i >= 0){
		if(contab[i].rq & MMASK)free(contab[i].f);
		contab[i].rq = contab[i].f = 0;
	}
}
finds(mn)
int mn;
{
	register i, savip;

	oldmn = findmn(mn);
	newmn = apptr = aplnk = 0;
	if(app && (oldmn >= 0) && (contab[oldmn].rq & MMASK)){
			savip = ip;
			ip = contab[oldmn].f;
			oldmn = -1;
			while((i=rbf()) != 0);
			apptr = ip;
			if(!diflg)ip = incoff(ip);
			nextb = ip;
			ip = savip;
	}else{
		for(i=0;i<NM;i++){
			if(contab[i].rq == 0)break;
		}
		if((i==NM) ||
		   (nextb = alloc()) == 0){
			app = 0;
			if(macerr++ > 1)done2(02);
			prstr("Too many string/macro names.\n");
			edone(04);
			return(offset = 0);
		}
			contab[i].f = nextb;
		if(!diflg){
			newmn = i;
			if(oldmn == -1)contab[i].rq = -1;
		}else{
			contab[i].rq = mn | MMASK;
		}
	}

	app = 0;
	return(offset = nextb);
}
skip(){
	register i;

	while(((i=getch()) & CMASK) == ' ');
	ch=i;
	return(nlflg);
}
copyb()
{
	register i, j, k;
	int ii, req, state, savoff;

	if(skip() || !(j=getrq()))j = '.';
	req = j;
	k = j>>BYTE;
	j =& BMASK;
	copyf++;
	flushi();
	nlflg = 0;
	state = 1;
	while(1){
		i = (ii = getch()) & CMASK;
		if(state == 3){
			if(i == k)break;
			if(!k){
				ch = ii;
				i = getach();
				ch = ii;
				if(!i)break;
			}
			state = 0;
			goto c0;
		}
		if(i == '\n'){
			state = 1;
			nlflg = 0;
			goto c0;
		}
		if((state == 1) && (i == '.')){
			state++;
			savoff = offset;
			goto c0;
		}
		if((state == 2) && (i == j)){
			state++;
			goto c0;
		}
		state = 0;
c0:
		if(offset)wbf(ii);
	}
	if(offset){
		wbfl();
		offset = savoff;
		wbt(0);
	}
	copyf--;
	return(req);
}
copys()
{
	register i;

	copyf++;
	if(skip())goto c0;
	if(((i=getch()) & CMASK) != '"')wbf(i);
	while(((i=getch()) & CMASK) != '\n')wbf(i);
c0:
	wbt(0);
	copyf--;
}
alloc()
{
	register i;
	int j;

	for(i=0;i<NBLIST;i++){
		if(blist[i] == 0)break;
	}
	if(i==NBLIST){
		return(nextb=0);
	}else{
		blist[i] = -1;
		if((j = boff(i)) < NEV*EVS)return(nextb = 0);
		return(nextb = j);
	}
}
free(i)
int i;
{
	register j;

	while((blist[j = blisti(i)]) != -1){
		i = blist[j];
		blist[j] = 0;
	}
	blist[j] = 0;
}
boff(i)
int i;
{
	return(NEV*EVS + i*BLK);
}
wbt(i)
int i;
{
	wbf(i);
	wbfl();
}
wbf(i)
int i;
{
	register j;

	if(!offset)return;
	if(!woff){
		woff = offset;
		wbfi = 0;
	}
	wbuf[wbfi++] = i;
	if(!((++offset) & (BLK-1))){
		wbfl();
		if(blist[j = blisti(--offset)] == -1){
			if(alloc() == 0){
				prstr("Out of temp file space.\n");
				done2(01);
			}
			blist[j] = nextb;
		}
		offset = blist[j];
	}
	if(wbfi >= BLK)wbfl();
}
wbfl(){
	if(woff == 0)return;
	seek(ibf, woff<<1, 0);
	write(ibf, &wbuf, wbfi<<1);
	if((woff & (~(BLK-1))) == (roff & (~(BLK-1))))roff = -1;
	woff = 0;
}
blisti(i)
int i;
{
	return((i-NEV*EVS)/(BLK));
}
rbf(){
	register i;

	if((i=rbf0(ip)) == 0){
		if(!app)i = popi();
	}else{
		ip = incoff(ip);
	}
	return(i);
}
rbf0(p)
int p;
{
	register i;

	if((i = (p & (~(BLK-1)))) != roff){
		roff = i;
		seek(ibf, roff<<1, 0);
		if(read(ibf, &rbuf, BLK<<1) == 0)return(0);
	}
	return(rbuf[p & (BLK-1)]);
}
incoff(p)
int p;
{
	register j;
	if(!((j = (++p)) & (BLK-1))){
		if((j = blist[blisti(--p)]) == -1){
			prstr("Bad storage allocation.\n");
			done2(-5);
		}
	}
	return(j);
}
popi(){
	register int *p;

	if(frame == stk)return(0);
	if(strflg)strflg--;
	p = nxf = frame;
	*p++ = 0;
	frame = *p++;
	ip = *p++;
	nchar = *p++;
	rchar = *p++;
	pendt = *p++;
	ap = *p++;
	cp = *p++;
	ch0 = *p++;
	return(*p);
}
pushi(newip)
int newip;
{
	register int *p;

	if((enda - (STKSIZE<<1)) < nxf)setbrk(DELTA);
	p = nxf;
	p++; /*nargs*/
	*p++ = frame;
	*p++ = ip;
	*p++ = nchar;
	*p++ = rchar;
	*p++ = pendt;
	*p++ = ap;
	*p++ = cp;
	*p++ = ch0;
	*p++ = ch;
	cp = nchar = rchar = pendt = ap = ch0 = ch = 0;
	frame = nxf;
	if(*nxf == 0) nxf =+ STKSIZE;
		else nxf = argtop;
	return(ip = newip);
}
setbrk(x)
char *x;
{
	register char *i;
	char *sbrk();

	if((i = sbrk(x)) == -1){
		prstrfl("Core limit reached.\n");
		edone(0100);
	}else{
		enda = i + x;
	}
	return(i);
}
getsn(){
	register i;

	if((i=getach()) == 0)return(0);
	if(i == '(')return(getrq());
		else return(i);
}
setstr(){
	register i;

	lgf++;
	if(((i=getsn()) == 0) ||
	   ((i=findmn(i)) == -1) ||
	   !(contab[i].rq & MMASK)){
		lgf--;
		return(0);
	}else{
		if((enda-2) < nxf)setbrk(DELTA);
		*nxf = 0;
		strflg++;
		lgf--;
		return(pushi(contab[i].f));
	}
}
collect()
{
	register i;
	register int *strp;
	int *argpp, *argppend;
	int quote, *savnxf, *lim;

	copyf++;
	*nxf = 0;
	if(skip())goto rtn;
	savnxf = nxf;
	lim = nxf =+ 20*STKSIZE;
	strflg = 0;
	if((argppend = strp = (argpp = savnxf+STKSIZE) + 9) > enda)setbrk(DELTA);
	for(i=8; i>=0; i--)argpp[i] = 0;
	while((argpp != argppend) && (!skip())){
		*argpp++ = strp;
		quote = 0;
		if(((i = getch()) & CMASK) == '"')quote++;
			else ch = i;
		while(1){
			i = getch();
			if( nlflg ||
			  ((!quote) && ((i & CMASK) == ' ')))break;
			if(quote && ((i & CMASK) == '"') &&
			  (((i=getch()) & CMASK) != '"')){
				ch = i;
				break;
			}
			*strp++ = i;
			if(strflg && (strp >= lim)){
				prstrfl("Macro argument too long.\n");
				copyf--;
				edone(004);
			}
			if((enda-4) <= strp)setbrk(DELTA);
		}
		*strp++ = 0;
	}
	nxf = savnxf;
	*nxf = argpp - nxf - STKSIZE;
	argtop = strp;
rtn:
	copyf--;
}
seta()
{
	register i;

	if(((i = (getch() & CMASK) - '0') > 0) &&
		(i <= 9) && (i <= *frame))ap = *(i + frame + STKSIZE -1);
}
caseda(){
	app++;
	casedi();
}
casedi(){
	register i, j;

	lgf++;
	if(skip() || ((i=getrq()) == 0)){
		if(dip->op > 0)wbt(0);
		if(dilev > 0){
			v.dn = dip->dnl;
			v.dl = dip->maxl;
			dip = &d[--dilev];
			offset = dip->op;
		}
		goto rtn;
	}
	if(++dilev == NDI){
		--dilev;
		prstr("Cannot divert.\n");
		edone(02);
	}
	if(dip->op)wbt(0);
	diflg++;
	dip = &d[dilev];
	dip->op = finds(i);
	dip->curd = i;
	clrmn(oldmn);
	for(j=1; j<=10; j++)dip[j] = 0;	/*not op and curd*/
rtn:
	app = 0;
	diflg = 0;
}
casedt(){
	lgf++;
	dip->dimac = dip->ditrap = dip->ditf = 0;
	skip();
	dip->ditrap = vnumb(0);
	if(nonumb)return;
	skip();
	dip->dimac = getrq();
}
casetl(){
	register i, j;
	int w1, w2, w3, begin, delim;
	extern width(), pchar();

	dip->nls = 0;
	skip();
	if(dip->op)wbfl();
	if((offset = begin = alloc()) == 0)return;
	if((delim = getch()) & MOT){
		ch = delim;
		delim = '\'';
	}else delim =& CMASK;
	if(!nlflg)
		while(((i = getch()) & CMASK) != '\n'){
			if((i & CMASK) == delim)i = IMP;
			wbf(i);
		}
	wbf(IMP);wbf(IMP);wbt(0);

	w1 = hseg(width,begin);
	w2 = hseg(width,0);
	w3 = hseg(width,0);
	offset = dip->op;
#ifdef NROFF
	if(!offset)horiz(po);
#endif
	hseg(pchar,begin);
	if(w2 || w3)horiz(j=quant((lt - w2)/2-w1,HOR));
	hseg(pchar,0);
	if(w3){
		horiz(lt-w1-w2-w3-j);
		hseg(pchar,0);
	}
	newline(0);
	if(*dip){if(dip->dnl > dip->hnl)dip->hnl = dip->dnl;}
	else{if(v.nl > dip->hnl)dip->hnl = v.nl;}
	free(begin);
}
casepc(){
	pagech = chget(IMP);
}
hseg(f,p)
int (*f)();
int *p;
{
	register acc, i;
	static int *q;

	acc = 0;
	if(p)q = p;
	while(1){
		i = rbf0(q);
		q = incoff(q);
		if(!i || (i == IMP))return(acc);
		if((i & CMASK) == pagech){
			nrbits = i & ~CMASK;
			nform = fmt[findr('%')];
			acc =+ fnumb(v.pn,f);
		}else acc =+ (*f)(i);
	}
}
casepm(){
	register i, k;
	register char *p;
	int j, xx, cnt, kk, tot;
	char pmline[10];

	kk = cnt = 0;
	tot = !skip();
	for(i = 0; i<NM; i++){
		if(!((xx = contab[i].rq) & MMASK))continue;
		p = pmline;
		j = contab[i].f;
		k = 1;
		while((j = blist[blisti(j)]) != -1)k++;
		cnt++;
		kk =+ k;
		if(!tot){
			*p++ = xx & 0177;
			if(!(*p++ = (xx >> BYTE) & 0177))*(p-1) = ' ';
			*p++ = ' ';
			kvt(k,p);
			prstr(pmline);
		}
	}
	if(tot || (cnt > 1)){
		kvt(kk,pmline);
		prstr(pmline);
	}
}
kvt(k,p)
int k;
char *p;
{
	if(k>=100)*p++ = k/100 + '0';
	if(k>=10)*p++ = (k%100)/10 + '0';
	*p++ = k%10 + '0';
	*p++ = '\n';
	*p = 0;
}
