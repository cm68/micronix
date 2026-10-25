#
/*
 * UNIX shell
 *
 * S. R. Bourne
 * Bell Telephone Laboratories
 *
 */

#include	"defs.h"


PROC VOID	gsort();

#define ARGMK	01

INT		errno;
STRING		sysmsg[];

/* fault handling */
#define ENOMEM	12
#define ENOEXEC 8
#define E2BIG	7
#define ENOENT	2
#define ETXTBSY 26



/* service routines for `execute' */

VOID	initio(iop)
	IOPTR		iop;
{
	REG STRING	ion;
	REG INT		iof, fd;

	IF iop
	THEN	iof=iop->iofile;
		ion=mactrim(iop->ioname);
		IF *ion ANDF (flags&noexec)==0
		THEN	IF iof&IODOC
			THEN	subst(chkopen(ion),(fd=tmpfil()));
				close(fd); fd=chkopen(tmpout); unlink(tmpout);
			ELIF iof&IOMOV
			THEN	IF eq(minus,ion)
				THEN	fd = -1;
					close(iof&IOUFD);
				ELIF (fd=stoi(ion))>=USERIO
				THEN	failed(ion,badfile);
				ELSE	fd=dup(fd);
				FI
			ELIF (iof&IOPUT)==0
			THEN	fd=chkopen(ion);
			ELIF iof&IOAPP ANDF (fd=open(ion,1))>=0
			THEN	lseek(fd, 0L, 2);
			ELSE	fd=create(ion);
			FI
			IF fd>=0
			THEN	rename(fd,iof&IOUFD);
			FI
		FI
		initio(iop->ionxt);
	FI
}

STRING	getpath(s)
	STRING		s;
{
	REG STRING	path;
	IF any('/',s)
	THEN	return(nullstr);
	ELIF (path = pathnod.namval)==0
	THEN	return(defpath);
	ELSE	return(cpystak(path));
	FI
}

INT	pathopen(path, name)
	REG STRING	path, name;
{
	REG UFD		f;

	REP path=catpath(path,name);
	PER (f=open(curstak(),0))<0 ANDF path DONE
	return(f);
}

STRING	catpath(path,name)
	REG STRING	path;
	STRING		name;
{
	/* leaves result on top of stack */
	REG STRING	scanp = path;
	REG STRING	argp = locstak();

	WHILE *scanp ANDF *scanp!=COLON DO *argp++ = *scanp++ OD
	IF scanp!=path THEN *argp++='/' FI
	IF *scanp==COLON THEN scanp++ FI
	path=(*scanp ? scanp : 0); scanp=name;
	WHILE (*argp++ = *scanp++) DONE
	return(path);
}

LOCAL STRING	xecmsg;
LOCAL STRING	*xecenv;

VOID	execa(at)
	STRING		at[];
{
	REG STRING	path;
	REG STRING	*t = at;

	IF (flags&noexec)==0
	THEN	xecmsg=notfound; path=getpath(*t);
		namscan(exname);
		xecenv=setenv();
		WHILE path=execs(path,t) DONE
		failed(*t,xecmsg);
	FI
}

LOCAL STRING	execs(ap,t)
	STRING		ap;
	REG STRING	t[];
{
	REG STRING	p, prefix;

	prefix=catpath(ap,t[0]);
	trim(p=curstak());

	sigchk();
	execve(p, &t[0] ,xecenv);
	SWITCH errno IN

	    case ENOEXEC:
		flags=0;
		comdiv=0; ioset=0;
		clearup(); /* remove open files and for loop junk */
		IF input THEN close(input) FI
		close(output); output=2;
		input=chkopen(p);

		/* set up new args */
		setargs(t);
		longjmp(subshell,1);

	    case ENOMEM:
		failed(p,toobig);

	    case E2BIG:
		failed(p,arglist);

	    case ETXTBSY:
		failed(p,txtbsy);

	    default:
		xecmsg=badexec;
	    case ENOENT:
		return(prefix);
	ENDSW
}

/* for processes to be waited for */
#define MAXP 20
LOCAL INT	pwlist[MAXP];
LOCAL INT	pwc;

postclr()
{
	REG INT		*pw = pwlist;

	WHILE pw <= &pwlist[pwc]
	DO *pw++ = 0 OD
	pwc=0;
}

VOID	post(pcsid)
	INT		pcsid;
{
	REG INT		*pw = pwlist;

	IF pcsid
	THEN	WHILE *pw DO pw++ OD
		IF pwc >= MAXP-1
		THEN	pw--;
		ELSE	pwc++;
		FI
		*pw = pcsid;
	FI
}

VOID	await(i)
	INT		i;
{
	INT		rc=0, wx=0;
	INT		w;
	INT		ipwc = pwc;

	post(i);
	WHILE pwc
	DO	REG INT		p;
		REG INT		sig;
		INT		w_hi;

		BEGIN
		   REG INT	*pw=pwlist;
		   p=wait(&w);
		   WHILE pw <= &pwlist[ipwc]
		   DO IF *pw==p
		      THEN *pw=0; pwc--;
		      ELSE pw++;
		      FI
		   OD
		END

		IF p > 0 THEN jobreaped(p) FI

		IF p == -1
		THEN	/*
			 * ^Z: the terminal has told the shell to take the
			 * foreground pipeline back.  Stop it and record it;
			 * the job ids, names and policy are all ours.
			 */
			IF backnote
			THEN	backnote = 0;
				jobstop();
				rc = 0;
				wx = 0;
			FI
			continue;
		FI

		w_hi = (w>>8)&LOBYTE;

		IF sig = w&0177
		THEN	IF sig == 0177	/* ptrace! return */
			THEN	prs("ptrace: ");
				sig = w_hi;
			FI
			IF sysmsg[sig]
			THEN	IF i!=p ORF (flags&prompt)==0 THEN prp(); prn(p); blank() FI
				prs(sysmsg[sig]);
				IF w&0200 THEN prs(coredump) FI
			FI
			newline();
		FI

		IF rc==0
		THEN	rc = (sig ? sig|SIGFLG : w_hi);
		FI
		wx |= w;
	OD

	IF wx ANDF flags&errflg
	THEN	exitsh(rc);
	FI
	exitval=rc; exitset();
}

/* ========	job control	======== */

/*
 * A job is the pipeline the shell was running when ^Z interrupted its
 * wait.  The kernel keeps only per-process switches (sys/job.c); ids,
 * grouping, names and every policy decision are here, so the two do
 * not have to agree about what a job is.
 */
#define MAXJOBS	6		/* detached pipelines remembered at once */
#define JSTOP	1		/* stopped by ^Z */
#define JBG	2		/* running, terminal input closed */
#define JFG	3		/* in the foreground now */

LOCAL struct jobent {
	INT	jid;		/* 0 = free */
	INT	npids;
	INT	state;
	INT	pids[MAXP];
	CHAR	name[JOBNM];
} jobtab[MAXJOBS];

LOCAL INT	curjob = -1;	/* the entry jobfg is waiting for */

LOCAL VOID
jobname(j)
	REG struct jobent *j;
{
	REG INT	i;

	FOR i=0; i<JOBNM-1 ANDF jobline[i]; i++
	DO j->name[i] = jobline[i] OD
	j->name[i] = 0;
}

/*
 * A child has been reaped, so it is no longer part of any job.
 */
LOCAL VOID
jobreaped(p)
	INT	p;
{
	REG INT		i, k;
	REG struct jobent *j;

	IF p > 0
	THEN	FOR i=0; i<MAXJOBS; i++
		DO	j = &jobtab[i];
			IF j->jid
			THEN	FOR k=0; k<j->npids; k++
				DO IF j->pids[k] == p
				   THEN	WHILE k < j->npids-1
					DO j->pids[k] = j->pids[k+1]; k++ OD
					j->npids--;
					IF j->npids == 0
					THEN	IF curjob == i THEN curjob = -1 FI
						j->jid = 0;
					FI
					return;
				   FI
				OD
			FI
		OD
	FI
}

/*
 * Stop the pids still in the wait list and make them a job.
 *
 * The wait list is not compact - reaped pids leave holes - so it is
 * scanned whole.  The pids that refuse to stop have already gone and
 * are not recorded; jobs will reap them when it next looks.
 */
LOCAL VOID
jobstop()
{
	REG INT		i, pid;
	REG struct jobent *j;
	INT		stopped = 0;
	INT		newname = 0;

	IF pwc
	THEN	IF curjob >= 0 ANDF jobtab[curjob].jid
		THEN	j = &jobtab[curjob];
		ELSE	FOR i=0; i<MAXJOBS; i++
			DO IF jobtab[i].jid == 0 THEN break FI OD
			IF i >= MAXJOBS
			THEN	prs("jobs: table full"); newline();
				FOR i=0; i<MAXP; i++
				DO IF pwlist[i] > 0
				   THEN jobctl(pwlist[i], JOBSTOP);
				   FI
				OD
				FOR i=0; i<MAXP; i++ DO pwlist[i] = 0 OD
				pwc = 0;
				return;
			FI
			j = &jobtab[i];
			j->jid = i+1;
			newname = 1;
		FI

		j->npids = 0;
		FOR i=0; i<MAXP; i++
		DO	pid = pwlist[i];
			IF pid > 0 ANDF jobctl(pid, JOBSTOP) == 0
			THEN	j->pids[j->npids++] = pid;
				stopped++;
			FI
		OD
		FOR i=0; i<MAXP; i++ DO pwlist[i] = 0 OD
		pwc = 0;

		IF stopped
		THEN	IF newname THEN jobname(j) FI
			j->state = JSTOP;
			prc('['); prn(j->jid); prs("] Stopped ");
			prs(j->name); newline();
		ELSE	j->jid = 0;
			IF curjob >= 0 THEN curjob = -1 FI
		FI
	FI
}

/*
 * Reap the job's dead members.
 *
 * A pid the kernel says is not alive is a zombie - it has exited but
 * its parent has not collected it - and wait() returns at once.  It is
 * the shell's own child, and await() reports every pid it reaps to
 * jobreaped(), so a pid still here cannot have been collected already.
 */
LOCAL VOID
jobpoll(j)
	REG struct jobent *j;
{
	REG INT		k, p, pid;
	INT		w;

	LOOP	pid = 0;
		FOR k=0; k<j->npids; k++
		DO IF kill(j->pids[k], 0) < 0
		   THEN pid = j->pids[k]; break;
		   FI
		OD
		IF pid == 0 THEN break FI
		p = wait(&w);
		IF p > 0
		THEN	jobreaped(p);
		ELSE	jobreaped(pid);	/* no child left to reap */
		FI
	POOL
}

LOCAL struct jobent *
jobfind(id)
	INT	id;
{
	REG INT	i;

	FOR i=0; i<MAXJOBS; i++
	DO IF jobtab[i].jid == id THEN return(&jobtab[i]) FI OD
	return(0);
}

VOID
joblist()
{
	REG INT		i;
	REG struct jobent *j;

	FOR i=0; i<MAXJOBS; i++
	DO	j = &jobtab[i];
		IF j->jid
		THEN	jobpoll(j);
			IF j->jid
			THEN	prc('['); prn(j->jid); prs("] ");
				IF j->state == JSTOP
				THEN	prs("Stopped ");
				ELSE	prs("Running ");
				FI
				prs(j->name); newline();
			FI
		FI
	OD
}

VOID
jobfg(id)
	INT	id;
{
	REG INT		i;
	REG struct jobent *j;

	IF (j = jobfind(id)) == 0
	THEN	prs("fg: no such job"); newline();
		exitval = 1;
		return;
	FI
	jobpoll(j);
	IF j->jid == 0
	THEN	prs("fg: job finished"); newline();
		exitval = 1;
		return;
	FI

	FOR i=0; i<MAXP; i++ DO pwlist[i] = 0 OD
	pwc = 0;
	FOR i=0; i<j->npids; i++
	DO	post(j->pids[i]);
		jobctl(j->pids[i], JOBCONT|JOBTTYIN);
	OD
	j->state = JFG;
	curjob = j - jobtab;
	backnote = 0;
	await(0);
	curjob = -1;
}

VOID
jobbg(id)
	INT	id;
{
	REG INT		i;
	REG struct jobent *j;

	IF (j = jobfind(id)) == 0
	THEN	prs("bg: no such job"); newline();
		exitval = 1;
		return;
	FI
	jobpoll(j);
	IF j->jid == 0
	THEN	prs("bg: job finished"); newline();
		exitval = 1;
		return;
	FI

	FOR i=0; i<j->npids; i++
	DO jobctl(j->pids[i], JOBCONT|JOBTTYOUT) OD
	j->state = JBG;
	prc('['); prn(j->jid); prs("] "); prs(j->name); prs(" &"); newline();
}

BOOL		nosubst;

trim(at)
	STRING		at;
{
	REG STRING	p;
	REG CHAR	c;
	REG CHAR	q=0;

	IF p=at
	THEN	WHILE c = *p
		DO *p++=c&STRIP; q |= c OD
	FI
	nosubst=q&QUOTE;
}

STRING	mactrim(s)
	STRING		s;
{
	REG STRING	t=macro(s);
	trim(t);
	return(t);
}

STRING	*scan(argn)
	INT		argn;
{
	REG ARGPTR	argp = Rcheat(gchain)&~ARGMK;
	REG STRING	*comargn, *comargm;

	comargn=getstak(BYTESPERWORD*argn+BYTESPERWORD); comargm = comargn += argn; *comargn = ENDARGS;

	WHILE argp
	DO	*--comargn = argp->argval;
		IF argp = argp->argnxt
		THEN trim(*comargn);
		FI
		IF argp==0 ORF Rcheat(argp)&ARGMK
		THEN	gsort(comargn,comargm);
			comargm = comargn;
		FI
		/* Lcheat(argp) &= ~ARGMK; */
		argp = Rcheat(argp)&~ARGMK;
	OD
	return(comargn);
}

LOCAL VOID	gsort(from,to)
	STRING		from[], to[];
{
	INT		k, m, n;
	REG INT		i, j;

	IF (n=to-from)<=1 THEN return FI

	FOR j=1; j<=n; j*=2 DONE

	FOR m=2*j-1; m/=2;
	DO  k=n-m;
	    FOR j=0; j<k; j++
	    DO	FOR i=j; i>=0; i-=m
		DO  REG STRING *fromi; fromi = &from[i];
		    IF cf(fromi[m],fromi[0])>0
		    THEN break;
		    ELSE STRING s; s=fromi[m]; fromi[m]=fromi[0]; fromi[0]=s;
		    FI
		OD
	    OD
	OD
}

/* Argument list generation */

INT	getarg(ac)
	COMPTR		ac;
{
	REG ARGPTR	argp;
	REG INT		count=0;
	REG COMPTR	c;

	IF c=ac
	THEN	argp=c->comarg;
		WHILE argp
		DO	count += split(macro(argp->argval));
			argp=argp->argnxt;
		OD
	FI
	return(count);
}

LOCAL INT	split(s)
	REG STRING	s;
{
	REG STRING	argp;
	REG ARGPTR	ap;
	REG INT		c;
	INT		count=0;

	LOOP	sigchk(); argp=locstak()+BYTESPERWORD;
		WHILE (c = *s++, !any(c,ifsnod.namval) && c)
		DO *argp++ = c OD
		IF argp==staktop+BYTESPERWORD
		THEN	IF c
			THEN	continue;
			ELSE	return(count);
			FI
		ELIF c==0
		THEN	s--;
		FI
		argp=endstak(argp); ap=(ARGPTR)argp;
		IF c=expand(ap->argval,0)
		THEN	count += c;
		ELSE	/* assign(&fngnod, argp->argval); */
			makearg(ap); count++;
		FI
		gchain=(ARGPTR)(Rcheat(gchain)|ARGMK);
	POOL
}
