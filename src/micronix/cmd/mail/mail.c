/*
 * mail [user ...]
 *
 * v7 mail (usr/src/cmd/mail.c), ported to micronix.
 *
 * cmd/mail/mail.c
 *
 * The v7 source is the one followed.  The 2.11BSD mail in
 * extra/2.11/pdp11/usr/src/bin/mail.c is a rewrite of it, and three
 * of the things it changed are the reason it is not the base here.
 * It hands the sending half to sendmail - bulkmail() execs
 * _PATH_SENDMAIL - and micronix has no sendmail and no mailer of any
 * kind; the v7 design, where mail opens the recipient's spool file
 * and appends the letter to it and `m user` re-runs mail for a
 * forward, is the one that fits a system that delivers its own mail.
 * It locks with flock(2), which micronix has not got, so v7's lock -
 * the user-execute bit as a flag - is kept.  And its default save
 * file is $HOME/mbox, where micronix's getenv(3) reads an ENVIRON
 * file rather than the shell's exported variables; the v7 port saves
 * to plain "mbox", which is what the recovered mail.1 on the
 * distribution disk describes.
 *
 * What the port took out:
 *
 *	utmp.h		#included by v7 and never used.  Nothing in mail
 *			reads or writes utmp; the records are login's
 *			and who's business.  The include is gone rather
 *			than carried dead.
 *
 *	getlogin()	not in this libc.  v7 asked it first, checked the
 *			answer against getpwnam, and fell back to
 *			getpwuid(getuid()); with no getlogin the port goes
 *			straight to the fallback.
 *
 *	whoami.h	the 1982 header's sysname, which v7 used for the
 *			"remote from" line.  Micronix keeps no system
 *			name, and 2.11BSD's gethostname has nothing to
 *			answer with either, so the port uses the constant
 *			at thissys below.
 *
 *	umask(MAILMODE)	not a system call here.  v7 created a mailbox
 *			under a umask of ~0644, that is, with mode 0644.
 *			A file creat makes is 0666, so the port cuts a
 *			mailbox it had to create back to 0644 and leaves
 *			one that was already there at the mode it has.
 *
 *	popen()		not in this libc, and neither is fdopen, so
 *			there is no way to hand a FILE to a pipe.  v7's
 *			sendrmt() used popen to run `mail user` for a
 *			local forward, or `uux - sys!rmail user` for a
 *			remote one, with the letter on the child's
 *			standard input.  The port writes the letter to a
 *			second temporary file and lets the shell do the
 *			redirection - "mail user < /tmp/mbXXXXXX" -
 *			through system(3), which this libc does have.  A
 *			remote address still spells out its uux command
 *			line, but micronix has no uucp, so that one fails
 *			at the shell; that is the honest report for a
 *			machine that cannot reach another one.
 *
 *	chown()		takes an owner packed as uid | gid << 8 and sets
 *			both at once, so the two arguments v7 passed are
 *			one here.
 *
 * And one name had to change.  v7's signal handler is called
 * delete(); ccc's backend keeps an operator of that name and reads the
 * call to it as a malformed expression - "bad op (not fn)".  It is
 * onintr() here, which is what ed.c calls the same handler; 2.11BSD
 * renamed this one too, to delex().
 *
 * There is one more thing the port had to say differently and it is
 * not a missing routine, it is a present one that does not behave the
 * way v7 assumed:
 *
 *	fopen(name, "w")	does not truncate.  This tree's freopen
 *			treats "w" and "a" as the same case and seeks a
 *			file that already exists to its end, and there is
 *			no truncate(2) to do it by hand.  creat(2) does
 *			truncate an existing file and leaves its mode
 *			alone, so a creat to empty the file and then an
 *			fopen to write it is how this tree asks for "w".
 *			cmd/ld/ld.c opens its output the same way, for
 *			the same reason; see fopenw() below.  Without it
 *			copyback() would append every kept letter to the
 *			mailbox a second time instead of rewriting it.
 *
 *	mktemp()	names a file from a template of trailing X's, and
 *			this one and v7's do not agree on how many.  v7
 *			took any number, so its "/tmp/maXXXXX" was unique
 *			per process; this tree's wants six and gives the
 *			template back unchanged otherwise, which made the
 *			parent's working copy and the child mail that
 *			`m user` starts share one fixed name.  The
 *			template carries a sixth X here; lettmp's own
 *			comment has the rest.
 *
 * What it no longer has to say differently:
 *
 *	struct passwd	the tree's fields are name, uid and gid where
 *			v7's were pw_name, pw_uid and pw_gid, and uid and
 *			gid are one byte each.  getpwnam and getpwuid are
 *			in libc now - see getpwent (3) - though mail only
 *			ever called them and never read /etc/passwd
 *			itself.  The record comes back in a static area
 *			that the next call overwrites, and send() calls
 *			getpwnam, so the login name is copied into
 *			my_name[] here as 2.11BSD copies it; v7 kept the
 *			pointer and got away with it.
 *
 *	struct stat	micronix has no st_size and no st_ctime.  The
 *			size is 24 bits across st_size0 and st_size1 and
 *			is spelled out where it is read.  The ctime the
 *			lock's sixty-second staleness test wants is
 *			st_mtime here, the only time the inode carries;
 *			the kernel stamps it when the mailbox is written
 *			but its chmod does not stamp one, so a lock taken
 *			and abandoned reads as stale on the write time
 *			alone.  That makes the test weaker than v7's and
 *			not stronger.
 *
 * vim: tabstop=8 shiftwidth=8 noexpandtab:
 */

#include <types.h>
#include <stdio.h>
#include <pwd.h>
#include <sys/fs.h>
#include <sys/stat.h>
#include <sys/signal.h>
#include <setjmp.h>

/*copylet flags */
	/*remote mail, add rmtmsg */
#define	REMOTE	1
	/* zap header and trailing empty line */
#define	ZAP	3
#define	ORDINARY 2
#define	FORWARD	4
#define	LSIZE	256
#define	MAXLET	300	/* maximum number of letters */
/*
 * v7 spelled this ~0644 and handed it to umask.  There is no umask
 * here, so it is the mode of a mailbox this program had to create.
 */
#define	MAILMODE 0644		/* mode of created mail */

/*
 * A file's size.  micronix keeps it in 24 bits, a high byte and a low
 * word, where v7 had one st_size; packsrc and tail spell it the same
 * way.
 */
#define	FSIZE(st)	((long)(st).st_size1 + ((long)(st).st_size0 << 16))

/*
 * The system's name, for the "remote from" line.  v7 read it out of
 * <whoami.h>; micronix keeps none, so it is a constant.
 */
char	thissys[] = "micronix";

char	line[LSIZE];
char	resp[LSIZE];
struct let {
	long	adr;
	char	change;
} let[MAXLET];
int	nlet	= 0;
char	lfil[50];
long	iop;
/*
 * v7's template is "/tmp/maXXXXX", five X's.  v7's mktemp substitutes
 * every trailing X it finds, however many there are, so five was
 * enough there and the name came out unique to the process.  This
 * tree's mktemp - lib/libc/mktemp.c - wants exactly six and hands the
 * template back untouched when it does not find them, so five would
 * leave BOTH the copy of the mailbox being read and the identical
 * template in the child that `m user` starts named /tmp/maXXXXX.  The
 * child unlinks that name in its own main() and creates it again for
 * its own letter, and the parent's copy of the mailbox goes with it:
 * the parent then rewrites the mailbox out of a file whose first
 * block has been freed and reused, and the letters it kept come out
 * as NUL bytes.  A sixth X is the whole of the fix.
 */
char	lettmp[] = "/tmp/maXXXXXX";
char	fwdnam[] = "/tmp/mbXXXXXX";
char	maildir[] = "/usr/spool/mail/";
char	mailfile[] = "/usr/spool/mail/xxxxxxxxxxxxxxxxxxxxxxx";
char	dead[] = "dead.letter";
char	forwmsg[] = " forwarded\n";
char	*curlock;
int	lockerror;
FILE	*tmpf;
FILE	*malf;
/*
 * The name on the "From " line.  v7 held a pointer into getpwuid's
 * static record, which the getpwnam in send() then overwrote; this is
 * a copy, as 2.11BSD made it.
 */
char	my_name[64];
int	error;
int	locked;
int	changed;
int	forward;
char	from[] = "From ";
int	flgf;
int	flgp;
int	delflg = 1;
jmp_buf	sjbuf;

char	*getarg();
char	*index();
char	*ctime();
char	*mktemp();
char	*strncpy();
long	time();
int	system();

/*
 * Forward declarations.  ccc wants to know a name is a function before
 * it will take the function's address, and several of these are called
 * above their definitions - onintr is handed to setsig in main, and
 * fopenw is called from copyback above it.  v7's compiler did not ask;
 * ed.c declares its handlers for the same reason.
 */
int	cat();
int	copyback();
int	copylet();
int	copymt();
int	done();
FILE	*fopenw();
int	isfrom();
int	lock();
int	onintr();
int	printmail();
int	send();
int	sendmail();
int	sendrmt();
int	setsig();
int	unlock();

main(argc, argv)
int argc;
char **argv;
{
	register int i;
	char sobuf[BUFSIZ];
	struct passwd *pw;

	setbuf(stdout, sobuf);
	mktemp(lettmp);
	unlink(lettmp);
	/*
	 * v7 asked getlogin() first and dropped to this when it had no
	 * answer; there is no getlogin() in this libc, so this is all
	 * of it.
	 */
	pw = getpwuid(getuid());
	if (pw == NULL)
		strcpy(my_name, "???");
	else {
		strncpy(my_name, pw->name, sizeof(my_name) - 1);
		my_name[sizeof(my_name) - 1] = '\0';
	}
	if (setjmp(sjbuf))
		done();
	/*
	 * v7 counted to twenty because that was v7's NSIG; this one is
	 * sixteen, and signal() refuses a number outside it, so the
	 * loop is bounded by the header.
	 *
	 * SIGTINT is left out, and that is not tidiness.  Signal 7 here
	 * is not v7's SIGEMT; it is "a character has arrived at the
	 * terminal", which the tty driver sends on input (sys/tty.c)
	 * and which usersim raises whenever fd 0 has something to read.
	 * Caught by a handler that longjmps back to the prompt, it
	 * fires on the arrival of exactly the line the prompt is
	 * waiting for: the read is abandoned before it can take the
	 * character, the line is never consumed, and mail re-prompts
	 * for as long as the input is there.  login.c skips signal 7
	 * for the same reason - see the "left alone" in
	 * catch_signals().  With SIGTINT caught the loop above spins at
	 * the ? prompt; without it, it waits.
	 */
	for (i = 1; i < NSIG; i++)
		if (i != SIGTINT)
			setsig(i, onintr);
	tmpf = fopen(lettmp, "w");
	if (tmpf == NULL) {
		fprintf(stderr, "mail: cannot open %s for writing\n", lettmp);
		done();
	}
	if (argv[0][0] != 'r' &&	/* no favors for rmail*/
	   (argc == 1 || argv[1][0] == '-'))
		printmail(argc, argv);
	else
		sendmail(argc, argv);
	done();
}

setsig(i, f)
int i;
int (*f)();
{
	if (signal(i, SIG_IGN) != SIG_IGN)
		signal(i, f);
}

printmail(argc, argv)
int argc;
char **argv;
{
	int flg, i, j, print;
	char *p;

	setuid(getuid());
	cat(mailfile, maildir, my_name);
	for (; argc>1; argv++, argc--) {
		if (argv[1][0]=='-') {
			if (argv[1][1]=='q')
				delflg = 0;
			else if (argv[1][1]=='p') {
				flgp++;
				delflg = 0;
			} else if (argv[1][1]=='f') {
				if (argc>=3) {
					strcpy(mailfile, argv[2]);
					argv++;
					argc--;
				}
			} else if (argv[1][1]=='r') {
				forward = 1;
			} else {
				fprintf(stderr, "mail: unknown option %c\n",
					argv[1][1]);
				done();
			}
		} else
			break;
	}
	malf = fopen(mailfile, "r");
	if (malf == NULL) {
		fprintf(stdout, "No mail.\n");
		return;
	}
	lock(mailfile);
	copymt(malf, tmpf);
	fclose(malf);
	fclose(tmpf);
	unlock();
	tmpf = fopen(lettmp, "r");

	changed = 0;
	print = 1;
	for (i = 0; i < nlet; ) {
		j = forward ? i : nlet - i - 1;
		if(setjmp(sjbuf)) {
			print=0;
		} else {
			if (print)
				copylet(j, stdout, ORDINARY);
			print = 1;
		}
		if (flgp) {
			i++;
			continue;
		}
		setjmp(sjbuf);
		fprintf(stdout, "? ");
		fflush(stdout);
		if (fgets(resp, LSIZE, stdin) == NULL)
			break;
		switch (resp[0]) {

		default:
			fprintf(stderr, "usage\n");
		case '?':
			print = 0;
			fprintf(stderr, "q\tquit\n");
			fprintf(stderr, "x\texit without changing mail\n");
			fprintf(stderr, "p\tprint\n");
			fprintf(stderr, "s[file]\tsave (default mbox)\n");
			fprintf(stderr, "w[file]\tsame without header\n");
			fprintf(stderr, "-\tprint previous\n");
			fprintf(stderr, "d\tdelete\n");
			fprintf(stderr, "+\tnext (no delete)\n");
			fprintf(stderr, "m user\tmail to user\n");
			fprintf(stderr, "! cmd\texecute cmd\n");
			break;

		case '+':
		case 'n':
		case '\n':
			i++;
			break;
		case 'x':
			changed = 0;
		case 'q':
			goto donep;
		case 'p':
			break;
		case '^':
		case '-':
			if (--i < 0)
				i = 0;
			break;
		case 'y':
		case 'w':
		case 's':
			flg = 0;
			if (resp[1] != '\n' && resp[1] != ' ') {
				fprintf(stdout, "illegal\n");
				flg++;
				print = 0;
				continue;
			}
			if (resp[1] == '\n' || resp[1] == '\0')
				cat(resp+1, "mbox", "");
			for (p = resp+1; (p = getarg(lfil, p)) != NULL; ) {
				malf = fopen(lfil, "a");
				if (malf == NULL) {
					fprintf(stdout,
					    "mail: cannot append to %s\n", lfil);
					flg++;
					continue;
				}
				copylet(j, malf, resp[0]=='w'? ZAP: ORDINARY);
				fclose(malf);
			}
			if (flg)
				print = 0;
			else {
				let[j].change = 'd';
				changed++;
				i++;
			}
			break;
		case 'm':
			flg = 0;
			if (resp[1] == '\n' || resp[1] == '\0') {
				i++;
				continue;
			}
			if (resp[1] != ' ') {
				fprintf(stdout, "invalid command\n");
				flg++;
				print = 0;
				continue;
			}
			for (p = resp+1; (p = getarg(lfil, p)) != NULL; )
				if (!sendrmt(j, lfil))	/* couldn't send it */
					flg++;
			if (flg)
				print = 0;
			else {
				let[j].change = 'd';
				changed++;
				i++;
			}
			break;
		case '!':
			system(resp+1);
			fprintf(stdout, "!\n");
			print = 0;
			break;
		case 'd':
			let[j].change = 'd';
			changed++;
			i++;
			if (resp[1] == 'q')
				goto donep;
			break;
		}
	}
   donep:
	if (changed)
		copyback();
}

copyback()	/* copy temp or whatever back to /usr/spool/mail */
{
	register int i, n, c;
	int new = 0;
	struct stat stbuf;

	signal(SIGINT, SIG_IGN);
	signal(SIGHUP, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	lock(mailfile);
	stat(mailfile, &stbuf);
	if (FSIZE(stbuf) != let[nlet].adr) {	/* new mail has arrived */
		malf = fopen(mailfile, "r");
		if (malf == NULL) {
			fprintf(stdout, "mail: can't re-read %s\n", mailfile);
			done();
		}
		fseek(malf, let[nlet].adr, 0);
		fclose(tmpf);
		tmpf = fopen(lettmp, "a");
		fseek(tmpf, let[nlet].adr, 0);
		while ((c = fgetc(malf)) != EOF)
			fputc(c, tmpf);
		fclose(malf);
		fclose(tmpf);
		tmpf = fopen(lettmp, "r");
		let[++nlet].adr = FSIZE(stbuf);
		new = 1;
	}
	malf = fopenw(mailfile, MAILMODE);
	if (malf == NULL) {
		fprintf(stderr, "mail: can't rewrite %s\n", mailfile);
		done();
	}
	n = 0;
	for (i = 0; i < nlet; i++)
		if (let[i].change != 'd') {
			copylet(i, malf, ORDINARY);
			n++;
		}
	fclose(malf);
	if (new)
		fprintf(stdout, "new mail arrived\n");
	unlock();
}

/*
 * A stream open for writing on a file that must come out empty.
 *
 * micronix's fopen has no truncating mode: freopen's "w" and "a" are
 * the same case and both seek to the end of a file that already
 * exists, and there is no truncate(2) to empty it with.  creat(2)
 * does truncate, and leaves the mode of a file that exists alone, so
 * the two calls together are this tree's "w".  cmd/ld/ld.c opens its
 * output the same way.
 */
FILE *
fopenw(name, mode)
char *name;
int mode;
{
	close(creat(name, mode));
	return (fopen(name, "w"));
}

copymt(f1, f2)	/* copy mail (f1) to temp (f2) */
FILE *f1, *f2;
{
	long nextadr;

	nlet = nextadr = 0;
	let[0].adr = 0;
	while (fgets(line, LSIZE, f1) != NULL) {
		if (isfrom(line))
			let[nlet++].adr = nextadr;
		nextadr += strlen(line);
		fputs(line, f2);
	}
	let[nlet].adr = nextadr;	/* last plus 1 */
}

copylet(n, f, type)
int n;
FILE *f;
int type;
{
	int ch, k;

	fseek(tmpf, let[n].adr, 0);
	k = let[n+1].adr - let[n].adr;
	while(k-- > 1 && (ch=fgetc(tmpf))!='\n')
		if(type!=ZAP) fputc(ch,f);
	if(type==REMOTE)
		fprintf(f, " remote from %s\n", thissys);
	else if (type==FORWARD)
		fprintf(f, forwmsg);
	else if(type==ORDINARY)
		fputc(ch,f);
	while(k-->1)
		fputc(ch=fgetc(tmpf), f);
	if(type!=ZAP || ch!= '\n')
		fputc(fgetc(tmpf), f);
}

isfrom(lp)
register char *lp;
{
	register char *p;

	for (p = from; *p; )
		if (*lp++ != *p++)
			return(0);
	return(1);
}

sendmail(argc, argv)
int argc;
char **argv;
{

	time(&iop);
	fprintf(tmpf, "%s%s %s", from, my_name, ctime(&iop));
	iop = ftell(tmpf);
	flgf = 1;
	while (fgets(line, LSIZE, stdin) != NULL) {
		if (line[0] == '.' && line[1] == '\n')
			break;
		if (isfrom(line))
			fputs(">", tmpf);
		fputs(line, tmpf);
		flgf = 0;
	}
	fputs("\n", tmpf);
	nlet = 1;
	let[0].adr = 0;
	let[1].adr = ftell(tmpf);
	fclose(tmpf);
	if (flgf)
		return;
	tmpf = fopen(lettmp, "r");
	if (tmpf == NULL) {
		fprintf(stderr, "mail: cannot reopen %s\n", lettmp);
		return;
	}
	while (--argc > 0)
		if (!send(0, *++argv))	/* couldn't send to him */
			error++;
	if (error) {
		setuid(getuid());
		malf = fopenw(dead, MAILMODE);
		if (malf == NULL) {
			fprintf(stdout, "mail: cannot open %s\n", dead);
			fclose(tmpf);
			return;
		}
		copylet(0, malf, ZAP);
		fclose(malf);
		fprintf(stdout, "Mail saved in %s\n", dead);
	}
	fclose(tmpf);
}

/*
 * Send letter n to a name that has a "!" in it.
 *
 * v7 ran the delivery through popen and handed the letter to the
 * child's standard input.  There is no popen here and no fdopen to
 * build one on, so the letter goes to a temporary file and the shell
 * does the redirection instead.  A name with no "!" after a leading
 * one is local and means "forward this letter to that user": v7 ran
 * mail for it, and so does this, which is why a forward costs a
 * second process.
 *
 * rsys is bounded, which v7's was not: the name comes off the command
 * line and the loop that fills rsys had no end but the string's.
 */
sendrmt(n, name)
int n;
char *name;
{
	register char *p;
	char rsys[80], cmd[LSIZE + 128];
	register int local, sts;

	local = 0;
	if (*name=='!')
		name++;
	for(p=rsys; *name!='!'; *p++ = *name++)
		if (*name=='\0' || p >= &rsys[sizeof(rsys)-2]) {
			local++;
			break;
		}
	*p = '\0';
	if ((!local && *name=='\0') || (local && *rsys=='\0')) {
		fprintf(stdout, "null name\n");
		return(0);
	}
	mktemp(fwdnam);
	unlink(fwdnam);
	malf = fopen(fwdnam, "w");
	if (malf == NULL) {
		fprintf(stdout, "mail: cannot open %s\n", fwdnam);
		return(0);
	}
	copylet(n, malf, local? FORWARD: REMOTE);
	fclose(malf);
	if (local)
		sprintf(cmd, "mail %s < %s", rsys, fwdnam);
	else {
		if (index(name+1, '!'))
			sprintf(cmd, "uux - %s!rmail \\(%s\\) < %s",
				rsys, name+1, fwdnam);
		else
			sprintf(cmd, "uux - %s!rmail %s < %s",
				rsys, name+1, fwdnam);
	}
	setuid(getuid());
	sts = system(cmd);
	unlink(fwdnam);
	return(sts == 0);
}

send(n, name)	/* send letter n to name */
int n;
char *name;
{
	char file[50];
	register char *p;
	struct passwd *pw;
	struct stat stbuf;
	int existed;

	for(p=name; *p!='!' &&*p!='\0'; p++)
		;
	if (*p == '!')
		return(sendrmt(n, name));
	if ((pw = getpwnam(name)) == NULL) {
		fprintf(stdout, "mail: can't send to %s\n", name);
		return(0);
	}
	cat(file, maildir, name);
	/*
	 * v7 set the mode with umask(MAILMODE) around the open.  There
	 * is no umask here, so a mailbox that had to be created is cut
	 * back to the same mode afterwards; one that was already there
	 * is left alone.
	 */
	existed = (stat(file, &stbuf) == 0);
	malf = fopen(file, "a");
	if (malf == NULL) {
		fprintf(stdout, "mail: cannot append to %s\n", file);
		return(0);
	}
	if (!existed)
		chmod(file, MAILMODE);
	lock(file);
	chown(file, pw->uid | (pw->gid << 8));
	copylet(n, malf, ORDINARY);
	fclose(malf);
	unlock();
	return(1);
}

/*
 * The handler main installs for everything but SIGTINT.
 *
 * Two things about it differ from v7's delete(), and both are about
 * what the machine does rather than what mail does.
 *
 * The first is the argument.  v7's handler was handed the number of
 * the signal that fired and re-armed that one.  The kernel here does
 * not push a number: sig() in sys/sig.c pushes the interrupted pc and
 * points the pc at the handler, which leaves HL holding whatever the
 * interrupted code had in it.  A parameter reads as garbage - the
 * port's first cut printed 4963 for signal 7 - so this takes none,
 * and re-arms the whole set instead.  That is not belt and braces:
 * the same sig() clears the handler for every signal but SIGTINT as
 * it delivers it, so a handler that means to stay installed has to
 * put itself back.  ed.c's onintr() has no parameter for the same
 * reason.
 */
onintr()
{
	register int i;

	for (i = 1; i < NSIG; i++)
		if (i != SIGTINT)
			setsig(i, onintr);
	fprintf(stderr, "\n");
	if(delflg)
		longjmp(sjbuf, 1);
	done();
}

done()
{
	if(!lockerror)
		unlock();
	unlink(lettmp);
	exit(error+lockerror);
}

lock(file)
char *file;
{
	struct stat stbuf;

	if (locked || flgf)
		return;
	if (stat(file, &stbuf)<0)
		return;
	if (stbuf.st_mode&01) { 	/* user x bit is the lock */
		/*
		 * v7 read st_ctime here.  micronix keeps no change
		 * time, so this is the write time; see the note at the
		 * head of the file.
		 */
		if (stbuf.st_mtime+60 >= time((long *)0)) {
			fprintf(stderr, "%s busy; try again in a minute\n",
				file);
			lockerror++;
			done();
		}
	}
	locked = stbuf.st_mode & ~01;
	curlock = file;
	chmod(file, stbuf.st_mode|01);
}

unlock()
{
	if (locked)
		chmod(curlock, locked);
	locked = 0;
}

cat(to, from1, from2)
char *to, *from1, *from2;
{
	int i, j;

	j = 0;
	for (i=0; from1[i]; i++)
		to[j++] = from1[i];
	for (i=0; from2[i]; i++)
		to[j++] = from2[i];
	to[j] = 0;
}

char *getarg(s, p)	/* copy p... into s, update p */
register char *s, *p;
{
	while (*p == ' ' || *p == '\t')
		p++;
	if (*p == '\n' || *p == '\0')
		return(NULL);
	while (*p != ' ' && *p != '\t' && *p != '\n' && *p != '\0')
		*s++ = *p++;
	*s = '\0';
	return(p);
}
