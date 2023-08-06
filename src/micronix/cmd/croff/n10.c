#include "tdef.h"
#include "t.h"
#include "tw.h"
/*
nroff10.c

Device interfaces
*/

extern int lss;
extern char obuf[];
extern char *obufp;
extern int xfont;
extern int esc;
extern int lead;
extern int *dip;
extern int oline[];
extern int *olinep;
extern int ulfont;
extern int esct;
extern int sps;
extern int ics;
extern int ttysave;
extern int ttys[3];
extern char termtab[];
extern int ptid;
extern int waitf;
extern int pipeflg;
extern int eqflg;
extern int hflg;
extern int tabtab[];
extern int xxx;
int dtab;
int bdmode;
int plotmode;
static char Sccsid[] "@(#)n10.c	1.3 of 4/26/77";

ptinit(){
	register i;
	register char **p, *q;
	char *x[8];

	if((i = open(termtab,0)) < 0){
		prstr("Cannot open ");
		prstr(termtab);
		prstr("\n");
		exit(-1);
	}
	read(i,x,16);
	read(i,&t.bset,q = 2*(&t.zzz - &t.bset));
	x[2] =- q;
	q = setbrk(x[2]);
	seek(i,t.twinit+020,0);
	i = read(i,q,x[2]);
	q =- t.twinit;
	for(p = &t.twinit; p < &t.zzz; p++){
		if(*p)*p =+ q;else *p = &t.zzz;
	}
	sps = EM;
	ics = EM*2;
	dtab = 8 * t.Em;
	for(i=0; i<16; i++)tabtab[i] = dtab * (i+1);
	if(t.bset || t.breset){
		ttys[2] =& ~t.breset;
		ttys[2] =| t.bset;
		stty(1,ttys);
	}
	oputs(t.twinit);
	if(eqflg)t.Adj = t.Hor;
}
twdone(){
	obufp = obuf;
	oputs(t.twrest);
	flusho();
	if(pipeflg){
		close(ptid);
		wait(&waitf);
	}
	ttys[2] = ttysave;
	stty(1,ttys);
}
ptout(i)
int i;
{
	*olinep++ = i;
	if(olinep >= &oline[LNSIZE])olinep--;
	if((i&CMASK) != '\n')return;
	olinep--;
	lead =+ dip->blss + lss - t.Newline;
	dip->blss = 0;
	esct = esc = 0;
	if(olinep>oline){
		move();
		ptout1();
		oputs(t.twnl);
	}else{
		lead =+ t.Newline;
		move();
	}
	lead =+ dip->alss;
	dip->alss = 0;
	olinep = oline;
}
ptout1()
{
	register i, k;
	register char *codep;
	int *q, w, j, phyw;

	for(q=oline; q<olinep; q++){
	if((i = *q) & MOT){
		j = i & ~MOTV;
		if(i & NMOT)j = -j;
		if(i & VMOT)lead =+ j;
		else esc =+ j;
		continue;
	}
	if((k = (i & CMASK)) <= 040){
		switch(k){
			case ' ': /*space*/
				esc =+ t.Char;
				break;
		}
		continue;
	}
	codep = t.codetab[k-32];
	w = t.Char * (*codep++ & 0177);
	phyw = w;
	if(i&ZBIT)w = 0;
	if(*codep && (esc || lead))move();
	esct =+ w;
	if(i&074000)xfont = (i>>9) & 03;
	if(*t.bdon & 0377){
		if(!bdmode && (xfont == 2)){
			oputs(t.bdon);
			bdmode++;
		}
		if(bdmode && (xfont != 2)){
			oputs(t.bdoff);
			bdmode = 0;
		}
	}
	if(xfont == ulfont){
		for(k=w/t.Char;k>0;k--)oput('_');
		for(k=w/t.Char;k>0;k--)oput('\b');
	}
	while(*codep != 0){
		if(*codep & 0200){
			codep = plot(codep);
			oputs(t.plotoff);
			oput(' ');
		}else{
			if(plotmode)oputs(t.plotoff);
			oput(*codep++);
		}
	}
	if(!w)for(k=phyw/t.Char;k>0;k--)oput('\b');
	}
}
plot(x)
char *x;
{
	register int i;
	register char *j, *k;

	if(!plotmode)oputs(t.ploton);
	k = x;
	if((*k & 0377) == 0200)k++;
	for(; *k; k++){
		if(*k & 0200){
			if(*k & 0100){
				if(*k & 040)j = t.up; else j = t.down;
			}else{
				if(*k & 040)j = t.left; else j = t.right;
			}
			if(!(i = *k & 037))return(++k);
			while(i--)oputs(j);
		}else oput(*k);
	}
	return(k);
}
move(){
	register k;
	register char *i, *j;
	char *p, *q;
	int iesct, dt;

	iesct = esct;
	if(esct =+ esc)i = "\0"; else i = "\n\0";
	j = t.hlf;
	p = t.right;
	q = t.down;
	if(lead){
		if(lead < 0){
			lead = -lead;
			i = t.flr;
		/*	if(!esct)i = t.flr; else i = "\0";*/
			j = t.hlr;
			q = t.up;
		}
		if(*i & 0377){
			k = lead/t.Newline;
			lead = lead%t.Newline;
			while(k--)oputs(i);
		}
		if(*j & 0377){
			k = lead/t.Halfline;
			lead = lead%t.Halfline;
			while(k--)oputs(j);
		}
	}
	if(esc){
		if(esc < 0){
			esc = -esc;
			j = "\b";
			p = t.left;
		}else{
			j = " ";
			if(hflg)while((dt = dtab - (iesct%dtab)) <= esc){
				if(dt%t.Em)break;
				oput(TAB);
				esc =- dt;
				iesct =+ dt;
			}
		}
		k = esc/t.Em;
		esc = esc%t.Em;
		while(k--)oputs(j);
	}
	if((*t.ploton & 0377) && (esc || lead)){
		if(!plotmode)oputs(t.ploton);
		esc =/ t.Hor;
		lead =/ t.Vert;
		while(esc--)oputs(p);
		while(lead--)oputs(q);
		oputs(t.plotoff);
	}
	esc = lead = 0;
}
ptlead(){move();}
dostop(){
	int junk;

	flusho();
	read(2,&junk,1);
}
