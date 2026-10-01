/*
 * cu device [-t] [-s speed]
 *
 * v7 cu (usr/src/cmd/cu.c), ported to micronix.
 *
 * cmd/cu/cu.c
 *
 * Cu opens a terminal line to another machine and gives you the two
 * halves of the conversation: a child process that reads the line and
 * writes it to your terminal, and this one, which reads your terminal
 * and writes it to the line.  A line that begins with ~ is a command
 * to cu and not to the machine on the other end - see wr(), rd() and
 * dopercen() below, which are v7's and are unchanged.
 *
 * WHAT THE PORT HAD TO SAY DIFFERENTLY.
 *
 *	the auto-call unit	v7's cu is written around a dialer: it
 *			takes a telephone number, opens an ACU
 *			(/dev/cua0), forks a process to hold the line
 *			open while it dials /dev/cul0, waits, and then
 *			picks the call up.  micronix has no such device
 *			and no cu (1) that dials: the page gives the
 *			LINE as cu's argument - "cu /dev/ttyC -s 300" -
 *			and the modem is expected to be up already.  So
 *			conn() and its ACU are gone, the first argument
 *			is the line to open, -l and -a are gone with
 *			them, and the connection is a plain open.  The
 *			driver waits for carrier at open for a line that
 *			wants it, which is where the waiting moved to.
 *
 *	ioctl(TIOCGETP/TIOCSETP, fd, &buf)	there is no terminal ioctl
 *			in this tree - the only ioctl is the block-device
 *			one - so every one of them is gtty and stty.
 *
 *	ioctl(TIOCEXCL/TIOCHPCL)	are gone.  Exclusive use and
 *			drop-DTR-on-close are not things this driver
 *			offers; HUP, the mode bit that reads like the
 *			second one, is tested by nothing in sys/tty.c.
 *
 *	struct sgttyb		is <sys/sgtty.h>'s struct sgtty, whose
 *			fields are ispeed, ospeed, erase, kill and mode
 *			where v7's are sg_ispeed, sg_ospeed, sg_erase,
 *			sg_kill and sg_flags.  The same six bytes, under
 *			the names the header and the rest of the tree
 *			use.
 *
 *	parity			v7 sets EVENP|ODDP on the line as it opens
 *			it.  The driver has no parity to set: sgtty.h
 *			names EVEN and ODD and no file under sys/ ever
 *			reads either bit.  So the mode word is built up
 *			from zero instead, and what cu asks for is the
 *			part that matters - raw input.
 *
 *	a bug: v7 clears the flags for raw mode with
 *
 *			stbuf.sg_flags &= ECHO|CRMOD;
 *
 *			which KEEPS those two bits and drops every other
 *			one - the opposite of what the comment beside it
 *			says and of what the f==2 case does.  It is
 *			written here as &= ~(ECHO|CRMOD), which is what
 *			the 4.1BSD line became.
 *
 *	the hangup at the end	is a close.  v7 sets the speed to 0 to
 *			drop the line; a speed of 0 is a rate on this
 *			driver (stty (2) lists it) and not a hangup, and
 *			there is nothing else to ask for one with.
 *
 *	the argument scan	v7 broke out of it at the first argument
 *			it did not know and carried on as though it had
 *			not been given, so "-q" was ignored.  Here an
 *			argument that is not an option is the usage line
 *			and the command stops, and a -s that is not a
 *			speed is reported before anything is opened.
 *
 * WHAT IS v7's AND STAYS: the escape set (~. to quit, ~~ for a line
 * that starts with ~, ~<file to send, ~! and ~$ for a local command,
 * ~%put and ~%take, ~>file to divert what comes back), and the reader
 * and writer loops that implement them.  v7 writes sizeof(CRLF) -
 * three bytes, NUL and all - where it means the two characters; that
 * is left alone rather than halved here, since it costs nothing on a
 * terminal and cu cannot be tested in this tree.
 *
 * vim: tabstop=8 shiftwidth=8 noexpandtab:
 */

#include <types.h>		/* sgtty.h speaks in UINT8s */
#include <stdio.h>
#include <sys/signal.h>
#include <sys/sgtty.h>

#define CRLF "\r\n"
#define wrc(ds) write(ds,&c,1)

char	*usage = "usage: cu device [-t] [-s speed]";
char	*lspeed	= "300";

int	ln;			/* the descriptor for the line */
char	tkill, terase;		/* the current input kill & erase */
char	c;

/*
 *	read one character, with the parity bit taken off.  The write
 *	side is the wrc macro above, since it is one statement.
 */

rdc(ds)
{

	ds=read(ds,&c,1);
	c &= 0177;
	return (ds);
}

int	intr;

/*
 *	^C arrived in the middle of a diversion
 */

sig2()
{

	signal(SIGINT, SIG_IGN);
	intr = 1;
}

int	set14;

/*
 *	sleep that an alarm can cut short
 */

xsleep(n)
{

	xalarm(n);
	pause();
	xalarm(0);
}

xalarm(n)
{

	set14=n;
	alarm(n);
}

/*
 *	the alarm handler: rearm for a second at a time, so that a long
 *	pause is not ended early by the second alarm.
 */

sig14()
{

	signal(SIGALRM, sig14);
	if (set14) alarm(1);
}

int	dout;
int	nhup;

/*
 *	main: open the line, set its speed, put it in raw mode, and
 *	spawn a child to read it and write to this terminal while this
 *	process reads the terminal and writes to the line.
 */

main(ac, av)
	char *av[];
{
	int fk;
	int speed;
	char *dev;
	struct sgtty stbuf;

	signal(SIGALRM, sig14);
	if (ac < 2) {
		prf(usage);
		exit(8);
	}
	dev = av[1];
	av += 2;
	ac -= 2;
	for (; ac > 0; av++) {
		if (equal(*av, "-t")) {
			dout = 1;
			--ac;
			continue;
		}
		if (ac < 2) {
			prf(usage);
			exit(8);
		}
		if (equal(*av, "-s"))
			lspeed = *++av;
		else {
			prf(usage);
			exit(8);
		}
		ac -= 2;
	}
	/*
	 * the argument scan is checked before the line is touched, so
	 * that a -s that is not a speed is reported without opening
	 * anything.  v7 tested the devices first.
	 */
	switch (atoi(lspeed)) {
	case 110:
		speed = B110; break;
	case 134:
		speed = B134; break;
	case 150:
		speed = B150; break;
	case 300:
		speed = B300; break;
	case 1200:
		speed = B1200; break;
	default:
		prf("Unknown speed: %s", lspeed);
		exit(9);
	}
	if (!exists(dev))
		exit(9);
	ln = open(dev, 2);
	if (ln < 0) {
		prf("Can't open %s", dev);
		exit(9);
	}
	stbuf.ispeed = speed;
	stbuf.ospeed = speed;
	stbuf.mode = 0;
	if (!dout)
		stbuf.mode |= RAW;
	stty(ln, &stbuf);
	prf("Connected");
	if (dout)
		fk = -1;
	else
		fk = fork();
	nhup = (int)signal(SIGINT, SIG_IGN);
	if (fk == 0) {
		rd();
		prf("\007Lost carrier");
		exit(3);
	}
	mode(1);
	wr();
	mode(0);
	kill(fk, SIGKILL);
	wait((int *)NULL);
	prf("Disconnected");
	exit(0);
}

/*
 *	wr: write to remote: 0 -> line.
 *	~.	terminate
 *	~<file	send file
 *	~!	local login-style shell
 *	~!cmd	execute cmd locally
 *	~$proc	execute proc locally, send output to line
 *	~%cmd	execute builtin cmd (put and take)
 */

wr()
{
	int ds,fk,lcl,x;
	char *p,b[600];

	for (;;) {
		p=b;
		while (rdc(0) == 1) {
			if (p == b) lcl=(c == '~');
			if (p == b+1 && b[0] == '~') lcl=(c!='~');
			if (c == 0) c=0177;
			if (!lcl) {
				if (wrc(ln) == 0) {
					prf("line gone"); return;
				}
			}
			if (lcl) {
				if (c == 0177) c=tkill;
				if (c == '\r' || c == '\n') goto A;
				if (!dout) wrc(0);
			}
			*p++=c;
			if (c == terase) {
				p=p-2;
				if (p<b) p=b;
			}
			if (c == tkill || c == 0177 || c == '\r' || c == '\n') p=b;
		}
		return;
A:
		if (!dout) echo("");
		*p=0;
		switch (b[1]) {
		case '.':
		case '\004':
			return;
		case '!':
		case '$':
			fk = fork();
			if (fk == 0) {
				close(1);
				dup(b[1] == '$'? ln:2);
				close(ln);
				mode(0);
				if (!nhup) signal(SIGINT, SIG_DFL);
				if (b[2] == 0) execl("/bin/sh","-",0);
				else execl("/bin/sh","sh","-c",b+2,0);
				prf("Can't execute shell");
				exit(~0);
			}
			if (fk!=(-1)) {
				while (wait(&x)!=fk);
			}
			mode(1);
			if (b[1] == '!') echo("!");
			else {
				if (dout) echo("$");
			}
			break;
		case '<':
			if (b[2] == 0) break;
			if ((ds=open(b+2,0))<0) {
				prf("Can't divert %s",b+1);
				break;
			}
			intr=x=0;
			mode(2);
			if (!nhup) signal(SIGINT, sig2);
			while (!intr && rdc(ds) == 1) {
				if (wrc(ln) == 0) {
					x=1;
					break;
				}
			}
			signal(SIGINT, SIG_IGN);
			close(ds);
			mode(1);
			if (x) return;
			if (dout) echo("<");
			break;
		case '%':
			dopercen(&b[2]);
			break;
		default:
			prf("Use `~~' to start line with `~'");
		}
		continue;
	}
}

dopercen(line)
	register char *line;
{
	char *args[10];
	register narg, f;
	int rcount;

	for (narg = 0; narg < 10;) {
		while(*line == ' ' || *line == '\t')
			line++;
		if (*line == '\0')
			break;
		args[narg++] = line;
		while(*line != '\0' && *line != ' ' && *line != '\t')
			line++;
		if (*line == '\0')
			break;
		*line++ = '\0';
	}
	if (equal(args[0], "take")) {
		if (narg < 2) {
			prf("usage: ~%%take from [to]");
			return;
		}
		if (narg < 3)
			args[2] = args[1];
		wrln("echo '~>:'");
		wrln(args[2]);
		wrln(";tee /dev/null <");
		wrln(args[1]);
		wrln(";echo '~>'\n");
		return;
	} else if (equal(args[0], "put")) {
		if (narg < 2) {
			prf("usage: ~%%put from [to]");
			return;
		}
		if (narg < 3)
			args[2] = args[1];
		if ((f = open(args[1], 0)) < 0) {
			prf("cannot open: %s", args[1]);
			return;
		}
		wrln("stty -echo;cat >");
		wrln(args[2]);
		wrln(";stty echo\n");
		xsleep(5);
		intr = 0;
		if (!nhup)
			signal(SIGINT, sig2);
		mode(2);
		rcount = 0;
		while(!intr && rdc(f) == 1) {
			rcount++;
			if (c == tkill || c == terase)
				wrln("\\");
			if (wrc(ln) != 1) {
				xsleep(2);
				if (wrc(ln) != 1) {
					prf("character missed");
					intr = 1;
					break;
				}
			}
		}
		signal(SIGINT, SIG_IGN);
		close(f);
		if (intr) {
			wrln("\n");
			prf("stopped after %d bytes", rcount);
		}
		wrln("\004");
		xsleep(5);
		mode(1);
		return;
	}
	prf("~%%%s unknown\n", args[0]);
}

equal(s1, s2)
	register char *s1, *s2;
{

	while (*s1++ == *s2)
		if (*s2++ == '\0')
			return(1);
	return(0);
}

wrln(s)
	register char *s;
{

	while (*s)
		write(ln, s++, 1);
}

/*
 *	rd: read from remote: line -> 1
 *	catch:
 *	~>[>][:][file]
 *	stuff from file...
 *	~>	(ends diversion)
 */

rd()
{
	int ds,slnt;
	char *p,*q,b[600];

	p=b;
	ds=(-1);
	while (rdc(ln) == 1) {
		if (ds<0) slnt=0;
		if (!slnt) wrc(1);
		*p++=c;
		if (c!='\n') continue;
		q=p;
		p=b;
		if (b[0]!='~' || b[1]!='>') {
			if (*(q-2) == '\r') {
				q--;
				*(q-1)=(*q);
			}
			if (ds>=0) write(ds,b,q-b);
			continue;
		}
		if (ds>=0) close(ds);
		if (slnt) {
			write(1, b, q - b);
			write(1, CRLF, sizeof(CRLF));
		}
		if (*(q-2) == '\r') q--;
		*(q-1)=0;
		slnt=0;
		q=b+2;
		if (*q == '>') q++;
		if (*q == ':') {
			slnt=1;
			q++;
		}
		if (*q == 0) {
			ds=(-1);
			continue;
		}
		if (b[2]!='>' || (ds=open(q,1))<0) ds=creat(q,0644);
		lseek(ds, (long)0, 2);
		if (ds<0) prf("Can't divert %s",b+1);
	}
}

/*
 *	the state of the terminal at this end.  0 cooked, 1 raw, 2
 *	cooked with the echo and the cr-lf mapping off, which is what a
 *	diversion to a file wants.
 */

mode(f)
{

	struct sgtty stbuf;

	if (dout) return;
	if (gtty(0, &stbuf) < 0) {
		prf("Not a typewriter");
		exit(1);
	}
	tkill = stbuf.kill;
	terase = stbuf.erase;
	if (f == 0) {
		stbuf.mode &= ~RAW;
		stbuf.mode |= ECHO|CRMOD;
	}
	if (f == 1) {
		stbuf.mode |= RAW;
		stbuf.mode &= ~(ECHO|CRMOD);
	}
	if (f == 2) {
		stbuf.mode &= ~RAW;
		stbuf.mode &= ~(ECHO|CRMOD);
	}
	stty(0, &stbuf);
}

echo(s)
	char *s;
{
	char *p;

	for (p=s;*p;p++);
	if (p>s) write(0,s,p-s);
	write(0,CRLF, sizeof(CRLF));
}

prf(f, s)
	char *f;
	char *s;
{

	fprintf(stderr, f, s);
	fprintf(stderr, CRLF);
}

exists(devname)
	char *devname;
{

	if (access(devname, 0)==0)
		return(1);
	prf("%s does not exist", devname);
	return(0);
}
