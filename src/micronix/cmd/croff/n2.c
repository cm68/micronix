#include "tdef.h"
#include "t.h"

/*
troff2.c

output, cleanup
*/

extern char obuf[OBUFSZ];
extern char *obufp;
extern int dilev;
extern int *dip;
extern int eschar;
extern int tlss;
extern int tflg;
extern int ascii;
extern int print;
extern char trtab[];
extern int waitf;
extern char ptname[];
extern int ptid;
extern int offset;
extern int em;
extern int ds;
extern int ip;
extern int mflg;
extern int woff;
extern int nflush;
extern int lgf;
extern int app;
extern int nfo;
extern int donef;
extern int *frame;
extern int *stk;
extern int *pendw;
extern int nofeed;
extern int trap;
extern int ttys[3];
extern int quiet;
extern int pendnf;
extern int ndone;
extern int lead;
extern int ralss;
extern int paper;
extern int gflag;
extern int *nxf;
extern char *unlkp;
extern char *nextf[];
extern int pipeflg;
extern int ejf;
extern int no_out;
extern int level;
extern int xxx;
int toolate;
int error;
#ifndef NROFF
extern int acctf;
#endif
static char Sccsid[] "@(#)n2.c	1.8 of 5/13/77";

pchar(c)
int c;
{
	register i, j;

	if((i=c) & MOT){pchar1(i); return;}
	switch(j = i & CMASK){
		case 0:
		case IMP:
		case RIGHT:
		case LEFT:
			return;
		case HX:
			j = (tlss>>9) | ((i&~0777)>>3);
			if(i & 040000){
				j =& ~(040000>>3);
				if(j > dip->blss)dip->blss = j;
			}else{
				if(j > dip->alss)dip->alss = j;
				ralss = dip->alss;
			}
			tlss = 0;
			return;
		case LX:
			tlss = i;
			return;
		case PRESC:
			if(!dip->op)j = eschar;
		default:
			i = (trtab[j] & BMASK) | (i & ~CMASK);
	}
	pchar1(i);
}
pchar1(c)
int c;
{
	register i, j, *k;
	extern int chtab[];

	j = (i = c) & CMASK;
	if(dip->op){
		wbf(i);
		dip->op = offset;
		return;
	}
	if(!tflg && !print){
		if(j == '\n')dip->alss = dip->blss = 0;
		return;
	}
	if(no_out || (j == FILLER))return;
#ifndef NROFF
	if(ascii){
		if(i & MOT){
			oput(' ');
			return;
		}
		if(j < 0177){
			oput(i);
			return;
		}
		switch(j){
			case 0200:
			case 0210:
				oput('-');
				break;
			case 0211:
				oputs("fi");
				break;
			case 0212:
				oputs("fl");
				break;
			case 0213:
				oputs("ff");
				break;
			case 0214:
				oputs("ffi");
				break;
			case 0215:
				oputs("ffl");
				break;
			default:
				for(k=chtab; *++k != j; k++)
					if(*k == 0)return;
				oput('\\');
				oput('(');
				oput(*--k & BMASK);
				oput(*k >> BYTE);
		}
	}else
#endif
	ptout(i);
}
oput(i)
char i;
{
	*obufp++ = i;
	if(obufp == (obuf + OBUFSZ + ascii - 1))flusho();
}
oputs(i)
char *i;
{
	while(*i != 0)oput(*i++);
}
flusho(){
	if(!ascii)*obufp++ = 0;
	if(!ptid){
		while((ptid=open(ptname,1)) < 0){
			if(++waitf <=2)prstr("Waiting for Typesetter.\n");
			sleep(15);
		}
	}
	if(no_out == 0){
	toolate =+ write(ptid, obuf, obufp-obuf);
	}
	obufp = obuf;
}
done(x) int x;{
	register i;

	error =| x;
	level = 0;
	app = ds = lgf = 0;
	if(i=em){
		donef = -1;
		em = 0;
		if(control(i,0))reset();
	}
	if(!nfo)done3(0);
	mflg = 0;
	dip = &d[0];
	if(woff)wbt(0);
	if(pendw)getword(1);
	pendnf = 0;
	if(donef == 1)done1(0);
	donef = 1;
	ip = 0;
	frame = stk;
	nxf = frame + STKSIZE;
	if(!ejf)tbreak();
	nflush++;
	eject(0);
	reset();
}
done1(x) int x; {
	error =| x;
	if(v.nl){
		trap = 0;
		eject(0);
		reset();
	}
	if(nofeed){
		ptlead();
		flusho();
		done3(0);
	}else{
		if(!gflag)lead =+ TRAILER;
		done2(0);
	}
}
done2(x) int x; {
	register i;

	ptlead();
#ifndef NROFF
	if(!ascii){
		oput(T_INIT);
		oput(T_STOP);
		if(!gflag)for(i=8; i>0; i--)oput(T_PAD);
	}
#endif
	flusho();
	done3(x);
}
done3(x) int x;{
	error =| x;
	signal(SIGINT, 1);
	signal(SIGKILL,1);
	unlink(unlkp);
#ifdef NROFF
	twdone();
#endif
	if(quiet){
		ttys[2] =| ECHO;
		stty(0,ttys);
	}
	if(ascii)mesg(1);
#ifndef NROFF
	report();
#endif
	exit(error);
}
edone(x) int x;{
	frame = stk;
	nxf = frame + STKSIZE;
	ip = 0;
	done(x);
}
#ifndef NROFF
report(){
	register i;
	struct {int use; char uid;} a;

	if((ptid != 1) && paper ){
		seek(acctf,0,2);
		a.use = paper;
		a.uid = getuid();
		write(acctf,&a,3);
		close(acctf);
	}
}
#endif
#ifdef NROFF
casepi(){
	register i;
	int id[2];

	if(toolate || skip() || !getname() || (pipe(id) == -1) ||
	   ((i=fork()) == -1)){
		prstr("Pipe not created.\n");
		return;
	}
	ptid = id[1];
	if(i>0){
		close(id[0]);
		toolate++;
		pipeflg++;
		return;
	}
	close(0);
	dup(id[0]);
	close(id[1]);
	execl(nextf,nextf,0);
	prstr("Cannot exec: ");
	prstr(nextf);
	prstr("\n");
	exit(-4);
}
#endif
