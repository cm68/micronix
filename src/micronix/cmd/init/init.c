/*
 * init - process 1
 *
 * cmd/init/init.c
 *
 * THIS IS A RECONSTRUCTION.  There is no surviving source for /etc/init;
 * this file was written from the disassembly of the /etc/init binary on the
 * Micronix 1.6 standalone - see init.dist, init.dis, init.ctl and README
 * beside this file.
 * Every function below corresponds to one function in that binary, in the
 * same order, and the odd bits are reproduced rather than tidied away,
 * because the odd bits are the ones that say what the system expected.
 * Where the disassembly did not tell me something, the comment says so.
 *
 * The original was compiled by Whitesmith's C, so a few things are visible
 * in the object that a reader should know about:
 *
 *  - Whitesmith's has exactly three register variables, kept in three fixed
 *    memory cells that the function prologue saves and the epilogue restores.
 *    Functions that use them are marked 'register' here.
 *  - a lot of what look like locals are in static storage, laid out in
 *    source order, function by function.  Those are written 'static' below.
 *    It costs nothing on a Z80 (absolute addressing is shorter than
 *    frame-relative) and init has a small stack, but it does mean that
 *    readttys() is not reentrant even though a SIGHUP can land in it.
 *  - buffer sizes are recovered from the stack frame sizes, so they are
 *    exact even when they look silly (exists() really does put a 512 byte
 *    buffer on the stack to hold a 36 byte struct stat).
 *
 * What it does, in order: closes and defuses everything it inherited, opens
 * /dev/root and /dev/swap so they can never be unmounted out from under it,
 * makes sure /etc/utmp and /etc/mtab exist, stamps a boot record into
 * /usr/adm/wtmp, resets every terminal named in /etc/ttys, and runs a single
 * user console session.  When that session exits it runs /etc/rc, reads
 * /etc/ttys, forks a login per enabled line, and then sits in wait() forever
 * respawning them.  SIGHUP re-reads /etc/ttys; SIGTERM shuts the machine down.
 */
#include <types.h>
#include <stdio.h>
#include <sys/fs.h>
#include <sys/stat.h>
#include <sys/sgtty.h>
#include <sys/dir.h>
#include <sys/signal.h>
#include <sys/access.h>
#include <utmp.h>
#include <mtab.h>
#include <errno.h>

extern int errno;

/*
 * the two signal handlers, named before they are defined
 */
extern reload();
extern shutdown();

/*
 * NOPEN is the kernel's per-process open file limit; it lives in a kernel
 * header this program has no business including, and the object just has
 * the constant 16 in it, twice.
 */
#define NOPEN   16

/*
 * one line of /etc/ttys.  The binary lays this out as exactly nine bytes
 * with no padding, which is how you can tell the fields apart in the
 * disassembly: t_login is at offset 5, straddling a word boundary.
 *
 * /etc/ttys looks like
 *      ttyA    9600    login
 *      ttyB    9600
 *      ttyC    1200    lpr     lst
 * name first, then any mix of a baud rate and the mode keywords below.
 * Only a line with the word 'login' on it gets a process.  Words that are
 * neither a baud rate nor a known keyword are ignored, which is why the
 * 'lpr' and 'lst' on the ttyC line do nothing at all - the third column is
 * not a program name to this init, whatever it may have been to another one.
 */
struct ttyent {
	char *t_name;               /* 0: "/dev/ttyA", malloc'd */
	int t_pid;                  /* 2: pid of the login on it, 0 if none */
	char t_speed;               /* 4: B9600 &c from sgtty.h */
	int t_login;                /* 5: nonzero if this line wants a login */
	int t_mode;                 /* 7: sgtty mode word */
};

#define NTTYS   32              /* the table is 288 bytes of data segment */

/*
 * console device, as handed to mknod when /dev/console has to be recreated.
 * The mknod builtin in the emergency shell below builds the same word as
 * (major << 8) | minor, so this is major 1, minor 1.
 */
#define CONSDEV 0x0101

/*
 * Three mode bits that /etc/ttys can set have no name in <sys/sgtty.h>,
 * because the terminal driver invented them for Micronix and only
 * <sys/tty.h> - which is the kernel's - spells them out.  They are the
 * old v6 form-feed and carriage-return delay bits, appropriated.
 */
#define M_ALL8      0040000     /* keep all 8 bits of input: "data8" */
#define M_CBREAK    0020000     /* raw with rub, quit, start, stop */
#define M_MORE      0010000     /* pause after 23 lines of output */

char *downmsg = "Micronix is down.\r\n";

char rootfd;                    /* /dev/root, held open forever */
char swapfd;                    /* /dev/swap, likewise */
int nttys;
struct ttyent ttys[NTTYS];

/*
 * The first thing init does is disown everything it was started with.
 * closeall() is deliberately both halves at once - ignore the signal and
 * close the descriptor - so that nothing the bootstrap left behind can
 * reach us.  Note that this closes 0, 1 and 2, which is why the two opens
 * that follow land on descriptors 0 and 1: holding the root and swap
 * devices open is the point, the descriptor numbers are incidental.
 */
main()
{
	closeall();

	rootfd = open("/dev/root", 0);
	swapfd = open("/dev/swap", 0);

	mkempty("/etc/utmp");
	mkempty("/etc/mtab");

	bootrecord();
	resetttys();

	while (console() == 0)
		;

	/* the single user session is over; forget who was logged in */
	mkempty("/etc/utmp");

	multi();
}

/*
 * multi user.  runrc() waits for /etc/rc, reload() builds the tty table and
 * starts the logins, and mainloop() never returns.
 */
multi()
{
	runrc();
	reload();
	mainloop();
}

/*
 * start one line if it wants starting.  A line that is not enabled, or that
 * already has a live child, is left alone; that is the whole of the
 * respawn policy, and it is why spawnall() can simply be called again after
 * every death.
 */
spawn(t)
struct ttyent *t;
{
	int pid;

	if (t->t_login == 0)
		return;
	if (t->t_pid != 0)
		return;

	pid = fork();
	if (pid == -1)
		return;
	if (pid == 0)
		startlogin(t);      /* does not return */
	else
		t->t_pid = pid;
}

/*
 * the child of spawn().  opentty() has already put the line on 0, 1 and 2
 * and set it to a sane default; here we only override the speed and the
 * mode with what /etc/ttys asked for.  The erase and kill characters
 * opentty() set are left alone - /etc/ttys has no syntax for them.
 */
startlogin(t)
struct ttyent *t;
{
	struct sgtty sg;

	opentty(t->t_name);

	gtty(0, &sg);
	sg.ispeed = t->t_speed;
	sg.ospeed = t->t_speed;
	sg.mode = t->t_mode;
	stty(0, &sg);

	exec("/bin/login", "login", "-t", t->t_name, 0);

	perror("/bin/login");
	exit(0);
}

/*
 * Nothing calls this.  It is in the binary, complete, and no call site
 * anywhere refers to it - presumably a debugging aid that outlived its use.
 * It is reproduced because it shows the shape the author had in mind for
 * complaining about something without risking a block on the console open.
 */
consmsg(s)
char *s;
{
	char fd;

	if (fork() != 0)
		return;
	fd = open("/dev/console", 1);
	if (fd < 0)
		exit(0);
	putstr(fd, s, "\n", 0);
	exit(1);
}

/*
 * the single user console session.  Returns 1 when the session has ended;
 * main() loops on it, so a return of 0 would mean "go round again", but the
 * only path out of the parent returns 1.  The child never returns at all.
 *
 * The interesting part is the recovery loop: if /dev/console cannot be
 * opened, init assumes /dev does not exist and builds it - the directory,
 * its . and .. links, and the console node - then syncs and tries again.
 * On a machine whose only shell is on the console this is the difference
 * between a bootable disk and a paperweight.
 */
console()
{
	int pid;
	int status;

	for (;;) {
		pid = fork();
		if (pid != -1)
			break;
		sleep(1);
	}

	if (pid != 0) {
		/* only now is it safe to be told to shut down */
		signal(SIGTERM, shutdown);
		while (wait(&status) != pid)
			;
		return 1;
	}

	while (opentty("/dev/console") != 1) {
		mknod("/dev", S_IFDIR | 0777, 0);
		link("/dev", "/dev/.");
		link("/", "/dev/..");
		mknod("/dev/console", S_IFCHR | 0777, CONSDEV);
		sync();
	}

	catfile("/etc/signon");

	/*
	 * Single user is a root shell with no password - unless root
	 * actually has one, in which case you have to log in for it.  That
	 * is what rootpasswd() is deciding.  Anything missing or broken on
	 * the way to /bin/login drops through to the shell instead, which
	 * is the right failure direction for a machine you are trying to
	 * repair.
	 */
	if (access("/bin/sh", A_EXEC) < 0)
		perror("/bin/sh");
	else if (access("/bin/login", A_EXEC) < 0)
		perror("/bin/login");
	else if (access("/etc/passwd", A_READ) < 0)
		perror("/etc/passwd");
	else if (rootpasswd())
		exec("/bin/login", "login", "-t", "/dev/console", 0);

	exec("/bin/sh", "-sh", 0);

	/* no shell on the disk at all: fall back on the one built in here */
	minishell();
	exit(0);
}

/*
 * read /etc/ttys into ttys[].
 *
 * This is called again on every SIGHUP, so it is written to merge rather
 * than rebuild: every existing entry is marked not-wanted first, and a line
 * that names a tty already in the table updates it in place and keeps the
 * pid, so a running login survives a reread.  Entries whose line has
 * disappeared from the file simply stay marked not-wanted and are never
 * respawned again; they are not removed, because their t_pid may still be
 * live and mainloop() has to be able to find it.
 */
readttys()
{
	static int login;
	static char speed;
	/* two bytes of data segment sit here that nothing in the object
	 * ever refers to - a variable that was declared and not used */
	static int n;
	static int mode;
	static char *cursor;
	static char *name;
	static char *tok;
	static FILE *fp;
	static struct ttyent *p;

	char path[512];
	char word[512];
	char line[512];

	tok = word;

	for (p = ttys; p < &ttys[nttys]; p++)
		p->t_login = 0;

	fp = fopen("/etc/ttys", "read");
	if (fp == 0)
		return;

	for (;;) {
		fgets(line, 512, fp);
		if (fp->_flag & (_EOF | _ERR)) {
			fclose(fp);
			return;
		}

		cursor = line;
		login = 0;
		mode = CRMOD | ECHO | TABS;
		speed = B9600;
		name = 0;

		for (;;) {
			cursor = getword(cursor, tok);
			if (*tok == 0)
				break;

			/*
			 * a word that reads as a baud rate is a baud rate;
			 * the first word that is not is the tty name; every
			 * word after that is either a mode keyword or is
			 * silently ignored.
			 */
			n = speedcode(tok);
			if (n >= 0) {
				speed = n;
				continue;
			}
			if (name == 0) {
				name = savestr(tok);
				continue;
			}
			if (equal("login", tok)) {
				login = 1;
				continue;
			}
			if (equal("cts", tok)) {
				mode |= SHAKE;
				continue;
			}
			if (equal("hardhand", tok)) {
				mode |= SHAKE;
				continue;
			}
			if (equal("shake", tok)) {
				mode |= SHAKE;
				continue;
			}
			if (equal("raw", tok)) {
				mode |= RAW;
				continue;
			}
			if (equal("nl", tok)) {
				mode &= ~CRMOD;
				continue;
			}
			if (equal("-crmod", tok)) {
				mode &= ~CRMOD;
				continue;
			}
			if (equal("-echo", tok)) {
				mode &= ~ECHO;
				continue;
			}
			if (equal("tabs", tok)) {
				mode &= ~TABS;
				continue;
			}
			if (equal("data8", tok)) {
				mode |= M_ALL8;
				continue;
			}
			if (equal("cbreak", tok)) {
				mode |= M_CBREAK;
				continue;
			}
			if (equal("more", tok)) {
				mode |= M_MORE;
				continue;
			}
		}

		if (name == 0)
			continue;           /* a blank line */

		/* a bare name means a name in /dev */
		if (*name != '/') {
			concat(path, "/dev/", name, 0);
			free(name);
			name = savestr(path);
		}

		for (p = ttys; p < &ttys[nttys]; p++) {
			if (equal(p->t_name, name)) {
				p->t_login = login;
				p->t_speed = speed;
				p->t_mode = mode;
				free(name);
				break;
			}
		}
		if (p == &ttys[nttys]) {
			/*
			 * Nothing bounds this against NTTYS.  A long enough
			 * /etc/ttys walks off the end of the table and into
			 * whatever data follows it.
			 */
			p->t_name = name;
			p->t_pid = 0;
			p->t_login = login;
			p->t_speed = speed;
			p->t_mode = mode;
			nttys++;
		}

		/* this is the stat("/dev/ttyA") you see in a syscall trace */
		if (exists(p->t_name) == 0)
			p->t_login = 0;
	}
}

/*
 * make name be descriptors 0, 1 and 2 and put it in a known state.
 *
 * The dup(dup(open())) is not a flourish: the descriptors were all closed
 * by closefds() a line earlier, so the open lands on 0 and the two dups on
 * 1 and 2.  Only the last result is checked, which is enough - if the open
 * failed the dups fail too.
 *
 * 0600 root-owned is what a getty would do: nobody can read your keystrokes
 * off the line between logins.
 */
opentty(name)
char *name;
{
	struct sgtty sg;

	closefds();
	defaultsigs();

	if (dup(dup(open(name, 2))) < 0)
		return 0;

	gtty(0, &sg);
	sg.mode = CRMOD | ECHO | TABS;
	sg.ispeed = sg.ospeed = B9600;
	sg.erase = 010;             /* ^H */
	sg.kill = 030;              /* ^X */
	stty(0, &sg);

	chown(name, 0);
	chmod(name, 0600);
	return 1;
}

savestr(s)
char *s;
{
	char *p;

	p = malloc(strlen(s) + 1);
	concat(p, s, 0);
	return p;
}

/*
 * The stat buffer here is 512 bytes on the stack for a structure that is
 * 36; that is what the frame size says and it is not a rounding of mine.
 */
exists(name)
char *name;
{
	char sb[512];

	if (stat(name, sb) < 0)
		return 0;
	return 1;
}

/*
 * (re)read /etc/ttys and act on it.  Also the SIGHUP handler, which is why
 * it brackets itself with ignoresigs()/catchsigs() - a second hangup
 * arriving in the middle of rebuilding the table would be fatal.
 */
reload()
{
	ignoresigs();
	readttys();
	resetttys();
	spawnall();
	catchsigs();
}

/*
 * run /etc/rc to completion.  Nothing happens if there is no /etc/rc, and
 * nothing happens if the fork fails - init just carries on without it.
 */
runrc()
{
	static int pid;
	static int status;

	if (exists("/etc/rc") == 0)
		return;

	pid = fork();
	if (pid == -1)
		return;

	if (pid != 0) {
		while (wait(&status) != pid)
			;
		return;
	}

	closefds();
	defaultsigs();
	opentty("/dev/console");
	exec("/bin/sh", "sh", "/etc/rc", 0);
	exit(0);
}

/*
 * make sure name exists and is empty.  If it is already there it is left
 * exactly as it is - which is how /etc/mtab survives a reboot with stale
 * entries in it, and why shutdown() has to cope with unmounting things
 * that are not mounted.
 */
mkempty(name)
char *name;
{
	if (exists(name))
		return;
	close(creat(name, 0666));
	sync();
}

ignoresigs()
{
	signal(SIGHUP, SIG_IGN);
	signal(SIGTERM, SIG_IGN);
}

catchsigs()
{
	signal(SIGHUP, reload);
	signal(SIGTERM, shutdown);
}

spawnall()
{
	static struct ttyent *p;

	for (p = ttys; p < &ttys[nttys]; p++)
		spawn(p);
}

/*
 * NOPEN is 16, so this covers every descriptor and every signal a process
 * can have.  signal(0, ...) is not a signal and the kernel rejects it; the
 * loop starts at 0 anyway because it is really a descriptor loop that got
 * the signal call folded into it.
 */
closeall()
{
	static int i;

	for (i = 0; i < NSIG; i++) {
		signal(i, SIG_IGN);
		close(i);
	}
}

/*
 * where init spends the rest of its life.  A death we cannot account for -
 * an orphan inherited from somebody, or a child of runrc() - just goes
 * round again.  Note that spawnall() is called rather than spawn(t): a
 * SIGHUP may have enabled other lines while we were blocked in wait().
 */
mainloop()
{
	register struct ttyent *t;
	int status;

	for (;;) {
		t = findpid(wait(&status));
		if (t == 0)
			continue;
		logout(t);
		spawnall();
	}
}

/*
 * Nothing calls this either.  It answers "which entry in /dev is this
 * device", by number, one-based, and returns 0 if it is not there.
 *
 * Two things about it: the dev buffer starts six bytes below the allocated
 * frame, which is fine only because this function uses the register-saving
 * prologue and that has already moved the stack down by six; and the middle
 * of the three comparisons tests one file's major against the other's
 * minor, which cannot be what was meant.  It has never mattered, because
 * nothing calls it.
 */
ttynumber(name)
char *name;
{
	register char *p;
	static char fd;
	static struct direct *dp;
	static int n;

	char path[38];
	struct stat sb;
	struct stat db;
	char dir[17];

	p = name;
	if (*p == '/')
		concat(path, p, 0);
	else
		concat(path, "/dev/", p, 0);

	if (stat(path, &sb) < 0)
		return 0;

	fd = open("/dev", 0);
	if (fd < 0)
		return 0;

	n = 1;
	for (;;) {
		if (read(fd, dir, 16) != 16) {
			n = 0;
			break;
		}
		dp = (struct direct *) dir;
		if (dp->d_ino == 0) {
			n++;
			continue;
		}
		dir[16] = 0;         /* d_name is 14 bytes and not terminated */
		concat(path, "/dev/", dp->d_name, 0);
		if (stat(path, &db) < 0) {
			n++;
			continue;
		}
		if (sb.st_ino != db.st_ino) {
			n++;
			continue;
		}
		if (sb.dev_u.dev_b.major_b != db.dev_u.dev_b.minor_b) {
			n++;
			continue;
		}
		if (sb.dev_u.dev_b.minor_b != db.dev_u.dev_b.minor_b) {
			n++;
			continue;
		}
		break;
	}
	close(fd);
	return n;
}

/*
 * a login on t has died.  Give the line back to root, append a logout
 * record to wtmp, clear the utmp slot, and mark the entry free so that the
 * next spawnall() restarts it.
 *
 * The utmp slot is found by device, not by name: for every record in the
 * file we stat /dev/<its line> and compare the device number that the
 * kernel keeps in the first block address of a special file's inode.  That
 * way a line that is reachable under two names still only occupies one
 * slot.
 *
 * The original frame has twenty more bytes in it than the code touches -
 * most likely a 'struct utmp' declared here and then abandoned in favour
 * of the static one.
 */
logout(t)
struct ttyent *t;
{
	static char fd;
	static char *p;
	static struct utmp u;

	char path[24];
	struct stat sb;
	struct stat sb2;

	if (t == 0)
		return;

	chown(t->t_name, 0);
	chmod(t->t_name, 0666);

	fd = open("/usr/adm/wtmp", 1);
	if (fd >= 0) {
		setmem(&u, sizeof u, 0);
		time(&u.time);

		/* walk to just past the last slash: "/dev/ttyA" -> "ttyA" */
		p = t->t_name;
		while (any('/', p))
			p++;
		concat(u.tty, p, 0);

		/* an all-zero name field is a logout */
		seek(fd, 0, 2);
		write(fd, &u, sizeof u);
		close(fd);
	}

	if (stat(t->t_name, &sb) < 0) {
		t->t_pid = 0;
		return;
	}
	fd = open("/etc/utmp", 2);
	if (fd < 0) {
		t->t_pid = 0;
		return;
	}

	while (read(fd, &u, sizeof u) == sizeof u) {
		concat(path, "/dev/", u.tty, 0);
		if (stat(path, &sb2) < 0)
			continue;
		if (sb.st_addr[0] != sb2.st_addr[0])
			continue;
		seek(fd, -(sizeof u), 1);
		setmem(&u, sizeof u, 0);
		write(fd, &u, sizeof u);
		break;
	}
	close(fd);
	t->t_pid = 0;
}

/*
 * stamp the boot into /usr/adm/wtmp.  A record with an empty line and a
 * name of "~" is the v6 convention for a reboot; last(1) reads it that way.
 *
 * This is the one place init writes without looking: the descriptor from
 * the open is never tested, so on a system with no /usr mounted yet the
 * seek and the write both go to -1 and quietly fail.  Which is, in fairness,
 * exactly the behaviour you want here - there is nothing useful to do about
 * it and nowhere to complain to this early.
 */
bootrecord()
{
	static int fd;
	static struct utmp u;

	setmem(&u, sizeof u, 0);
	time(&u.time);
	u.name[0] = '~';

	fd = open("/usr/adm/wtmp", 1);
	seek(fd, 0, 2);
	write(fd, &u, sizeof u);
	close(fd);
}

findpid(pid)
int pid;
{
	struct ttyent *p;

	if (pid == 0)
		return 0;
	for (p = ttys; p < &ttys[nttys]; p++)
		if (p->t_pid == pid)
			return p;
	return 0;
}

/*
 * SIGTERM: take the machine down.
 *
 * There is no process table to walk, so it signs off by killing every
 * possible pid except its own - 255 down to 1 - and then reaps until there
 * are no children left, running the normal logout bookkeeping for each one
 * so that utmp and wtmp are correct on the disk we are about to unmount.
 *
 * Then it unmounts everything /etc/mtab claims is mounted, drops its own
 * holds on the root and swap devices, prints its epitaph on the console and
 * sleeps forever.  It never exits: a dead process 1 is a panic.
 *
 * Note that the mtab record is unmounted by its special-file name with
 * "/dev/" pasted on the front, over the top of the record's own directory
 * field - the read buffer and the concat destination are the same buffer.
 */
shutdown()
{
	static struct mtab m;

	int status;
	int mypid;
	int i;

	ignoresigs();
	sync();

	mypid = getpid();
	for (i = 255; i != 0; i--) {
		if (i == mypid)
			continue;
		kill(i, SIGKILL);
	}
	sync();

	while ((i = wait(&status)) != -1 || errno != ECHILD) {
		logout(findpid(i));
		sync();
	}

	chdir("/");
	i = open("/etc/mtab", 0);
	while (read(i, &m, sizeof m) == sizeof m) {
		concat(m.directory, "/dev/", m.special, 0);
		umount(m.directory);
	}
	close(i);

	i = open("/dev/console", 1);
	close(rootfd);
	close(swapfd);
	sync();
	write(i, downmsg, strlen(downmsg));
	sync();

	for (;;)
		sleep(3600);
}

/*
 * The shell of last resort, reached only when exec of /bin/sh failed in
 * console().  It is enough to put a /bin/sh back on the disk, and no more:
 * no pipes, no redirection, no globbing, no quoting.  The prompt is "&& ".
 *
 * It ignores every signal for the duration, so a stray interrupt cannot
 * kill process 1 while you are repairing the machine; the child restores
 * the defaults before it execs.
 *
 * av is 512 pointers because line is 512 bytes: one word per character is
 * the worst case, so the word list can never overflow.
 */
minishell()
{
	char line[512];
	char *cp;
	char *av[512];
	char **ap;
	char *msg;
	char *prompt;
	int i;
	int pid;
	int status;
	int cmd;

	for (i = 1; i < NSIG - 1; i++)
		signal(i, SIG_IGN);

	msg = "You need to install /bin/login !\n";
	prompt = "&& ";

	print(msg);

	for (;;) {
		print(prompt);
		i = read(0, line, 512);
		if (i <= 0)
			return;
		line[i] = 0;

		/* split on anything that is not a printing character */
		cp = line;
		ap = av;
		for (;;) {
			while (*cp && (*cp <= ' ' || *cp >= 0177))
				*cp++ = 0;
			if (*cp == 0)
				break;
			*ap++ = cp;
			while (*cp > ' ' && *cp < 0177)
				cp++;
		}
		*ap = 0;
		if (ap == av)
			continue;

		cmd = lookup(av[0]);
		if (cmd != 0) {
			docmd(cmd, av);
			continue;
		}

		pid = fork();
		if (pid == -1) {
			perror(av[0]);
			continue;
		}
		if (pid != 0) {
			while (wait(&status) != pid)
				;
			continue;
		}

		for (i = 1; i < NSIG - 1; i++)
			signal(i, SIG_DFL);

		/*
		 * This does not do what it looks like it does.  exec() takes
		 * its argument vector from the address of its second
		 * argument on the stack, execl-fashion, so handing it av
		 * builds an argv of { av, ... } rather than av itself.  The
		 * exec fails or the child gets nonsense for argv[0]; either
		 * way the perror below is what actually runs.
		 */
		exec(av[0], av);
		perror(av[0]);
		exit(0);
	}
}

/*
 * the builtin table lives in the data segment as { name, code } pairs.
 * cd and chdir share a code, as they should.
 */
struct builtin {
	char *b_name;
	int b_code;
} builtins[] = {
	"cd", 1,
	"chdir", 1,
	"cp", 11,
	"sync", 2,
	"dir", 3,
	"exit", 4,
	"mkdir", 5,
	"mknod", 6,
	"mount", 7,
	"era", 8,
	"type", 9,
	"umount", 10,
	0, 0
};

lookup(word)
char *word;
{
	struct builtin *p;

	for (p = builtins; p->b_name != 0; p++)
		if (equal(word, p->b_name))
			return p->b_code;
	return 0;
}

/*
 * The compiler turned this into a jump table, so the numbering above is
 * load-bearing and the cases are in that order in the object.
 */
docmd(code, av)
int code;
char **av;
{
	switch (code) {

	case 1:                     /* cd, chdir */
		if (chdir(av[1]) < 0)
			perror(av[1]);
		break;

	case 2:                     /* sync */
		sync();
		break;

	case 3:                     /* dir */
		if (av[1] != 0)
			dirf(av[1]);
		else
			dirf(".");
		break;

	case 4:                     /* exit */
		/* no argument: whatever was in the return register is the
		 * exit status.  Harmless here - nothing waits for init. */
		exit();
		break;

	case 5:                     /* mkdir */
		mkdirf(av[1]);
		break;

	case 6:                     /* mknod */
		mknodf(av);
		break;

	case 7:                     /* mount */
		if (mount(av[1], av[2], 0) < 0)
			perror(0);
		break;

	case 8:                     /* era */
		av++;
		if (unlink(*av) < 0)
			perror(*av);
		break;

	case 9:                     /* type */
		typef(av[1]);
		break;

	case 10:                    /* umount */
		av++;
		if (umount(*av) < 0)
			perror(*av);
		break;

	case 11:                    /* cp */
		cpf(av[1], av[2]);
		break;
	}
}

/*
 * mkdir the hard way, because there is no mkdir system call: make the
 * node, then link . and .. into it by hand.  Between the mknod and the
 * second link the directory is malformed, which is exactly why mkdir(1)
 * had to be privileged on these systems.
 *
 * The parent is found by truncating the path at its last slash; a relative
 * path's parent is "." by definition.
 */
mkdirf(name)
char *name;
{
	char dot[32];
	char dotdot[32];
	char parent[32];
	char *p;

	concat(dot, name, "/.", 0);
	concat(dotdot, name, "/..", 0);
	concat(parent, name, 0);

	if (*name == '/') {
		p = parent;
		while (any('/', p))
			p++;
		*p = 0;
	} else {
		concat(parent, ".", 0);
	}

	if (mknod(name, S_IFDIR | 0777, 0) < 0) {
		perror(name);
		return;
	}
	if (link(name, dot) < 0) {
		perror(dot);
		return;
	}
	if (link(parent, dotdot) < 0) {
		perror(dotdot);
		return;
	}
}

/*
 * ls, near enough: read the directory as a file, print the name out of
 * each live entry.  d_name is 14 bytes and need not be terminated, hence
 * the copy into a 15 byte buffer.
 */
dirf(name)
char *name;
{
	char dir[16];
	char n;
	char fd;
	char entry[15];
	struct direct *dp;

	fd = open(name, 0);
	if (fd < 0) {
		perror(name);
		return;
	}

	for (;;) {
		n = read(fd, dir, 16);
		if (n != 16) {
			close(fd);
			return;
		}
		dp = (struct direct *) dir;
		if (dp->d_ino == 0)
			continue;
		movblock(entry, dp->d_name, 14);
		entry[14] = 0;
		print(entry);
		print("\n");
	}
}

/*
 * mknod name {b|c} major minor.  The device word is built as
 * (major << 8) | minor, which is the same layout the kernel keeps in a
 * special file's first block address.
 */
mknodf(av)
char **av;
{
	int mode;
	int dev;

	if (av[1] == 0 || av[2] == 0 || av[3] == 0 || av[4] == 0)
		goto usage;
	if (isnumber(av[3]) == 0)
		goto usage;
	if (isnumber(av[4]) == 0)
		goto usage;

	switch (*av[2]) {
	case 'b':
		mode = S_IFBLK | 0777;
		break;
	case 'c':
		mode = S_IFCHR | 0777;
		break;
	default:
		goto usage;
	}

	dev = (atoi(av[3]) << 8) | atoi(av[4]);
	if (mknod(av[1], mode, dev) < 0)
		perror(av[1]);
	return;

usage:
	putstr(2, "usage: mknod name {bc} major minor\n", 0);
}

isnumber(s)
char *s;
{
	while (*s) {
		if (*s < '0' || *s > '9')
			return 0;
		s++;
	}
	return 1;
}

print(s)
char *s;
{
	register char *p;

	p = s;
	write(1, p, strlen(p));
}

typef(name)
char *name;
{
	static int n;
	static char fd;

	char buf[512];

	fd = open(name, 0);
	if (fd < 0) {
		perror(name);
		return;
	}
	for (;;) {
		n = read(fd, buf, 512);
		if (n <= 0) {
			close(fd);
			return;
		}
		write(1, buf, n);
	}
}

/*
 * cp.  0666 on the target regardless of the source, and a short write is
 * reported against the target name.
 */
cpf(from, to)
char *from;
char *to;
{
	static char in;
	static char out;
	static int n;

	char buf[512];

	if (from == 0 || to == 0)
		return;
	if (*from == 0 || *to == 0)
		return;

	in = open(from, 0);
	if (in < 0) {
		perror(from);
		return;
	}
	out = creat(to, 0666);
	if (out < 0) {
		close(in);
		perror(to);
		return;
	}

	for (;;) {
		n = read(in, buf, 512);
		if (n < 0) {
			perror(from);
			break;
		}
		if (n == 0)
			break;
		if (write(out, buf, n) != n) {
			perror(to);
			break;
		}
	}
	close(in);
	close(out);
}

/*
 * copy the next printing-character run out of s into buf, and return where
 * to carry on from.  Anything outside ' '..0176 is a separator, so tabs and
 * the newline that fgets leaves on the line both end a word.
 */
getword(s, buf)
char *s;
char *buf;
{
	register char *p;
	register char *q;

	p = s;
	q = buf;

	while (*p && (*p <= ' ' || *p >= 0177))
		p++;
	while (*p > ' ' && *p < 0177)
		*q++ = *p++;
	*q = 0;
	return p;
}

/*
 * a baud rate as written in /etc/ttys to the sgtty code for it, or -1 if
 * the string is not a baud rate at all - which is how readttys() tells a
 * speed from a keyword without having to know the keywords first.
 */
speedcode(s)
char *s;
{
	switch (atoi(s)) {
	case 50:
		return B50;
	case 75:
		return B75;
	case 110:
		return B110;
	case 134:
		return B134;
	case 150:
		return B150;
	case 200:
		return B200;
	case 300:
		return B300;
	case 600:
		return B600;
	case 1200:
		return B1200;
	case 1800:
		return B1800;
	case 2400:
		return B2400;
	case 4800:
		return B4800;
	case 9600:
		return B9600;
	case 19200:
		return B19200;
	}
	return -1;
}

/*
 * cat a file to standard output, silently doing nothing if it is not
 * there.  Used for /etc/signon, which is optional.
 */
catfile(name)
char *name;
{
	register char *p;
	static char fd;
	static int n;

	char buf[512];

	p = name;
	fd = open(p, 0);
	if (fd < 0)
		return;
	for (;;) {
		n = read(fd, buf, 512);
		if (n <= 0) {
			close(fd);
			return;
		}
		write(1, buf, n);
	}
}

closefds()
{
	static char i;

	for (i = 0; i < NOPEN; i++)
		close(i);
}

defaultsigs()
{
	static char i;

	for (i = 0; i < NSIG; i++)
		signal(i, SIG_DFL);
}

/*
 * does root have a password?
 *
 * Walks /etc/passwd looking for an entry whose third field is uid 0 - the
 * test is both that the field starts with '0' and that it converts to 0,
 * so "0" passes and "01" does not.  Returns 1 if such an entry has a
 * non-empty password field, 0 otherwise, and 0 if /etc/passwd cannot be
 * read at all.
 *
 * console() uses this to decide whether single user needs a login.  An
 * unreadable or root-less /etc/passwd therefore gives you a free root
 * shell, which is the deliberate choice: you cannot lock yourself out of
 * a machine by damaging the password file.
 */
rootpasswd()
{
	static int found;
	static char *p;
	static char *pw;
	static FILE *fp;

	char line[512];

	found = 0;
	fp = fopen("/etc/passwd", "read");
	if (fp == 0)
		return found;

	for (;;) {
		fgets(line, 512, fp);
		if (fp->_flag & (_EOF | _ERR))
			break;

		p = line;
		while (*p && *p != ':')     /* the login name */
			p++;
		if (*p == 0)
			continue;
		p++;

		pw = p;                     /* the password */
		while (*p && *p != ':')
			p++;
		if (*p == 0)
			continue;
		p++;

		if (*p != '0')              /* the uid */
			continue;
		if (atoi(p) != 0)
			continue;

		if (*pw != ':')
			found = 1;
		break;
	}

	fclose(fp);
	return found;
}

/*
 * put every enabled line into the state /etc/ttys asks for, without
 * starting anything on it.  Called once at boot and again out of reload().
 *
 * The whole thing runs in a child because opening a terminal blocks until
 * carrier, and init cannot afford to block; the parent waits for it anyway,
 * so what this really buys is that a wedged line kills the child rather
 * than wedging init - though only if something kills the child, which
 * nothing here does.
 *
 * The open of the line is written with one argument.  open() takes two, so
 * whatever happened to be on the stack above it becomes the mode; on the
 * read/write open that this wants to be, that is usually harmless, but it
 * is a bug and not an idiom.
 */
resetttys()
{
	static int pid;
	static int status;
	static int r;
	static struct ttyent *p;
	static char fd;
	static struct sgtty sg;

	pid = fork();
	if (pid == -1)
		return;

	if (pid != 0) {
		do {
			r = wait(&status);
			if (r == -1)
				return;
		} while (r != pid);
		return;
	}

	readttys();

	for (p = ttys; p < &ttys[nttys]; p++) {
		if (p->t_login == 0)
			continue;
		chmod(p->t_name, 0666);
		chown(p->t_name, 0);
		fd = open(p->t_name);
		gtty(fd, &sg);
		sg.ispeed = sg.ospeed = p->t_speed;
		sg.mode = p->t_mode;
		stty(fd, &sg);
		close(fd);
	}
	exit(1);
}

/*
 * vim: tabstop=4 shiftwidth=4 noexpandtab:
 */
