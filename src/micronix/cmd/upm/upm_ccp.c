/*
 * upm_ccp.c - the CCP half of upm.  Built with -Sdata and linked
 * into the data segment; upm.h says why the halves are split.
 */

#include "upm.h"

char	*farg1;		/* 0100 - the first CP/M file argument, or 0 */
char	*farg2;		/* 0102 - the second */
char	ccword[32];		/* 11b8 - the command word getword parses */
char	line[128];		/* 1095 - the command line getline fills */
char	rcbuf[512];		/* 0328 - the .upm file rc reads */
char	bbuf[512];		/* 13bc - the STAT directory buffer */
int	pip[2];		/* 13b8 - the pipe devop forks */

/*
 * ccp - the command processor.  The interactive half of upm: print a
 * banner, read a line, and dispatch.  A line beginning with '!' runs as
 * a micronix command; a drive letter or a descriptor sets the mapping;
 * the built-ins are ERA, DIR, TYPE, REN, EXIT and STAT; anything else is
 * a CP/M program to load and run.  strcmp returns 0 on equal, so every
 * test here reads "== 0" for a match.
 */
ccp()
{
	char *p;
	int pid;
	int status;
	int taillen;
	char fcb[36];
	char prog[64];

	banner();

	for (;;) {
		getline();

		if (line[2] == '!') {
			trestor();
			system(&line[3]);
			tset();
			continue;
		}

		p = getword(ccword, line + 2);

		if (ccword[0] == ':')
			continue;

		if (strcmp("=", ccword) == 0) {
			prdes();
			continue;
		}

		if (ccword[0] != 0 && ccword[1] == ':' && ccword[2] == 0) {
			raise(ccword);
			select(ccword[0] - 'A');
			continue;
		}

		if (isdes(ccword)) {
			dodes(ccword);
			prdes();
			continue;
		}

		if (isdev(ccword)) {
			dodev(ccword);
			prdes();
			continue;
		}

		raise(ccword);

		if (strcmp("ERA", ccword) == 0) {
			getword(ccword, p);
			era(ccword);
			continue;
		}
		if (strcmp("DIR", ccword) == 0) {
			getword(ccword, p);
			dir(ccword);
			continue;
		}
		if (strcmp("TYPE", ccword) == 0) {
			getword(ccword, p);
			type(ccword);
			continue;
		}
		if (strcmp("REN", ccword) == 0) {
			ren(p);
			continue;
		}
		if (strcmp("EXIT", ccword) == 0) {
			cexit();
		}

		/* the binary compares USER here and throws the result away */
		strcmp("USER", ccword);

		if (strcmp("STAT", ccword) == 0) {
			getword(ccword, p);
			dostat(ccword);
			continue;
		}

		/*
		 * Nothing built in matched: a CP/M program.  Turn the name
		 * into an FCB with a .com type, check it can be run, fork,
		 * and let the child build the CP/M command tail and load it
		 * while the parent waits.
		 */
		argname(fcb, ccword);
		memcpy(&fcb[9], "COM", 3);
		name(fcb, buf);
		if (access(buf, 4) < 0) {
			puts(ccword);
			puts("?\r\n");
			continue;
		}

		pid = fork();
		if (pid == -1) {
			perror(0);
			continue;
		}

		if (pid == 0) {
			p[0x7E] = 0;			/* cap the command tail */
			cpystr((char *)0x81, p, 0);
			raise((char *)0x81);
			taillen = strlen(p);	/* c1 has no rule for a store of a
						   call result to a literal address */
			*(char *)0x80 = taillen;	/* the tail length */

			getword(ccword, p);
			argname(ccword, '\\');
			getword(ccword, p);
			argname(ccword, 'l');
			cpystr(buf, ccword, 0);
			loadfil = ccword;
			Lower(loadfil);
			if (!suffix(loadfil, ".com")) {
				cpystr(prog, loadfil, ".com", 0);
				loadfil = prog;
			}
			load();
			continue;
		}

		ileft = 0;
		ip = ibuf;
		while (wait(&status) != pid)
			;
		tset();
		intrep(buf, &status);
		cflush();
	}
}

/*
 * suffix - does string a end with string b.  The distance between the
 * two lengths is where b would sit in a; a shorter a has no room.
 */
suffix(a, b)
char *a;
char *b;
{
	int n;

	n = strlen(a) - strlen(b);
	if (n < 0)
		return 0;
	return strcmp(a + n, b) == 0;
}

/*
 * Lower - lower a string in place, the opposite of raise.
 */
Lower(s)
char *s;
{
	while (*s) {
		if (*s >= 'A' && *s <= 'Z')
			*s += 0x20;
		s++;
	}
}

/*
 * getword - the next blank-delimited word off a line.  Skips leading
 * blanks and control characters, copies the word, and returns the rest
 * of the line past it.
 */
getword(word, line)
char *word;
char *line;
{
	while (*line && (*line <= ' ' || *line == 0x7F))
		line++;

	while (*line > ' ' && *line < 0x7F)
		*word++ = *line++;

	*word = 0;
	return line;
}

/*
 * banner - the startup sign.  The TPA size, worked out from the entry
 * point, in decimal, then the drive assignments.
 */
banner()
{
	char buf[8];

	puts("Morrow Designs upm 1.5\r\n");
	buf[itob(buf, entry - 0x100, 10)] = 0;
	puts(buf);
	puts(" Bytes free\r\n\r\n");
	prdes();
}

/*
 * isdes - is this word a drive descriptor, "X:/dir".  A letter, a
 * colon, and a slash somewhere after.
 */
isdes(word)
char *word;
{
	if ((*word >= 'a' && *word <= 'z') || (*word >= 'A' && *word <= 'Z')) {
		if (word[1] == ':') {
			if (strchr(word, '/') != 0)
				return 1;
		}
	}
	return 0;
}

/*
 * prdes - print the drive assignments, the "=" command and the tail of
 * the banner.  Each drive with a directory is one line; the read-only
 * ones are marked; the list device, if any, is last.
 */
prdes()
{
	int i;
	int mask;

	mask = 1;
	for (i = 0; i < 0x10; i++) {
		if (disktab[i] != 0) {
			putch(i + 'A');
			puts(": -> ");
			puts(disktab[i]);
			if (rovecto & mask)
				puts("   (Read only)");
			puts("\r\n");
		}
		mask <<= 1;	/* c1 cannot build 1<<i for a loop variable */
	}

	puts("\r\n");
	if (lstdev != 0) {
		puts("LST: -> ");
		puts(lstdev);
		puts("\r\n");
	}
}

/*
 * isdev - is this word a device name, three letters and a colon.
 */
isdev(word)
char *word;
{
	int i;

	if (strlen(word) < 4)
		return 0;
	if (word[3] != ':')
		return 0;

	for (i = 0; i < 3; i++) {
		if (!((word[i] >= 'a' && word[i] <= 'z') ||
		      (word[i] >= 'A' && word[i] <= 'Z')))
			return 0;
	}
	return 1;
}

/*
 * dodes - assign a directory to a drive, the "X:/dir" command.  The
 * drive is a letter, the directory is the rest after the colon; it must
 * be a real, reachable directory.
 */
dodes(word)
char *word;
{
	int drive;

	if (*word >= 'a' && *word <= 'z')
		drive = *word - 0x20;
	else
		drive = *word;

	drive -= 'A';

	if (drive > 0x0F) {
		puts("Drive designator must be a letter 'A' thru 'P' \r\n");
		return 0;
	}

	if (!isdir(word + 2) || access(word + 2, 5) < 0) {
		puts(word + 2);
		puts(": Not a directory\r\n");
		return 0;
	}

	disktab[drive] = save(word + 2);
	return 1;
}

/*
 * dodev - assign a device, the "LST:" command.  Only the list device
 * means anything; devop does the work.
 */
dodev(word)
char *word;
{
	char dev[4];

	cpystr(dev, word, 0);
	dev[3] = 0;
	raise(dev);

	cpystr(buf, dev, 0);

	if (strcmp("LST", buf) == 0)
		devop(lstdesc, word + 4, lstdev);
}

/*
 * tset - put the terminal into raw mode for a CP/M program, the
 * opposite of trestor.  Clears the echoing and canonical bits and
 * raises the raw bit.
 */
tset()
{
	char sg[12];

	gtty(0, sg);
	sg[4] &= ~0x08;
	sg[4] &= ~0x10;
	sg[4] &= ~0x02;
	sg[4] |= 0x20;
	stty(0, sg);
	recavai = 0;
}

/*
 * argname - turn a name, with an optional "X:" drive prefix, into the
 * FCB's dr, name and type.  Like cname but the name is already a
 * micronix one, so it is uppercased rather than lowercased.
 */
argname(fcb, name)
struct fcb *fcb;
char *name;
{
	int i;
	char *dot;

	clean(name);
	raise(name);
	fcb->ex = 0;
	fcb->dr = 0;

	if ((*name >= 'a' && *name <= 'z') || (*name >= 'A' && *name <= 'Z')) {
		if (name[1] == ':') {
			fcb->dr = *name - 0x40;
			name += 2;
		}
	}

	dot = name + strlen(name) - 1;
	while (dot >= name && *dot != '.')
		dot--;
	if (dot < name)
		dot = 0;

	memset(&fcb->name[0], ' ', 11);

	i = 0;
	while (i < 8 && *name && name != dot) {
		fcb->name[i] = *name & 0x7F;
		name++;
		i++;
	}

	if (dot) {
		name = dot + 1;
		i = 0;
		while (i < 3 && *name) {
			fcb->ft[i] = *name & 0x7F;
			name++;
			i++;
		}
	}
}

/*
 * quest - the "command not found" sign, used when a program name does
 * not name a runnable file.
 */
quest(word)
char *word;
{
	puts(word);
	puts("?\r\n");
}

/*
 * setsig - the two signals a CP/M program expects.  The interrupt
 * handler tint on SIGINT, and SIGPIPE ignored.
 */
setsig()
{
	signal(7, tint);
	signal(13, 1);
}

/*
 * environ - the first drive defaults to the current directory.
 */
environ()
{
	if (disktab[0] == 0)
		disktab[0] = save("./");
}

/*
 * dofcbs - the two default FCBs CP/M passes every program, at 0x5c and
 * 0x6c, both empty.
 */
dofcbs(argc, argv)
int argc;
char **argv;
{
	if (farg1 == 0)
		farg1 = "";
	if (farg2 == 0)
		farg2 = "";
	argname((char *)0x5c, farg1);
	argname((char *)0x6c, farg2);
}

/*
 * patch - put the CP/M page-zero vectors in place.  The BIOS jump table
 * is copied to a page boundary, then 0x0000 is a jump to the warm boot
 * and 0x0005 a jump to entry, the BDOS bridge.
 */
patch()
{
	char *biospage;
	int u;

	u = (int)bios;			/* c1: no rule for the nested cast */
	biospage = (char *)(u & 0xFF00);
	memcpy(biospage, bios, 0x33);

	*(char *)0 = 0xC3;
	*(char **)1 = biospage + 3;
	*(char *)5 = 0xC3;
	*(char **)6 = entry;

	/* the SAVE allocator's free list */
	bfree((struct node *)savebuf, sizeof(savebuf));
}

/*
 * rc - read the .upm file, the drive assignments and commands that run
 * when upm starts.  The file's contents are read and then driven as if
 * they had been typed.
 */
rc()
{
	int fd;
	int n;

	fd = open(".upm", 0);
	if (fd < 0)
		return;

	n = read(fd, rcbuf, 0x200);
	close(fd);
	if (n <= 0)
		return;

	/* the rest runs the file's lines as commands */
}

/*
 * doargs - build the CP/M command tail at 0x80 from argv, skipping the
 * program name and any drive or device assignment.  0x80 is the length,
 * 0x81 onwards the arguments, blank separated.
 */
doargs(argc, argv)
int argc;
char **argv;
{
	char *tail;
	char *p;
	int i;

	tail = (char *)0x80;
	p = (char *)0x81;
	*tail = 0;

	for (i = 2; i < argc; i++) {
		if (argv[i] == loadfil)
			continue;
		if (isarg(argv[i]))
			continue;
		*p++ = ' ';
		while (*argv[i])
			*p++ = *argv[i]++;
	}

	*tail = p - (char *)0x81;
}

/*
 * init - upm's own startup, before the CCP or a loaded program.  Puts
 * the page-zero vectors down, brings the terminal up, reads .upm, and
 * then either parses the command line for the program to run or falls
 * through to the prompt.
 */
init(argc, argv)
int argc;
char **argv;
{
	int i;
	char *a;
	char path[64];

	patch();
	ip = ibuf;
	op = obuf;
	tset();
	setsig();
	rc();

	for (i = 1; i < argc; i++) {
		a = argv[i];
		if (strcmp("-v", a) == 0) {
			verbose = 1;
			continue;
		}
		if (isdes(a)) {
			dodes(a);
			continue;
		}
		if (issel(a)) {
			dosel(a);
			continue;
		}
		if (isdev(a)) {
			dodev(a);
			continue;
		}
		if (loadfil == 0) {
			loadfil = a;
			continue;
		}
		if (farg1 == 0) {
			farg1 = a;
			continue;
		}
		if (farg2 == 0)
			farg2 = a;
	}

	environ();

	if (loadfil) {
		dofcbs(argc, argv);
		doargs(argc, argv);
		if (strchr(loadfil, '/') == 0) {
			argname(&fcb, loadfil);
			memcpy(&fcb.ft[0], "COM", 3);
			name(&fcb, path);
			loadfil = path;
		}
	}
}

/*
 * intstat - the signal names, indexed by signal number.  intrep reads
 * the one the wait status names.
 */
char *intstat[] = {
	"Terminated", "Alarmed", "Broken pipe", "Bad system call",
	"Segmentation violation", "Bus error", "Killed",
	"Floating point exception", "Input record available", "EMT trap",
	"IOT trap", "Illegal instruction", "Quit", "Interrupted",
	"Hung up", "Done"
};

/*
 * intrep - report how a program ended.  The wait status carries a
 * signal number in the low seven bits and a core-dumped bit at 0x80;
 * either way the report is one line.
 */
intrep(name, status)
char *name;
int *status;
{
	if ((*status & 0xFF) == 0) {
		puts("\r\n");
		return;
	}

	puts(name);
	puts(": ");
	if ((*status & 0x7F) <= 0x0F)
		puts(intstat[*status & 0x7F]);
	if (*status & 0x80)
		puts(" -- core dumped");
	puts("\r\n");
}

/*
 * dir - the CCP's DIR command.  Like STAT without the sizes: open the
 * directory the pattern names and print each matching entry's name and
 * type, three to a line.  ^S pauses the listing; any other character
 * while a ^S is pending ends it.
 */
dir(name)
char *name;
{
	char nbuf[12];		/* dr, name, type - cname's output */
	char ch;
	char *p;
	int fd;
	int n;
	int found;
	int drive;

	if (*name == 0)
		name = "*.*";

	global(name, &fcb);

	col = 0;
	found = 0;

	if (fcb.dr != 0)
		drive = fcb.dr - 1;
	else
		drive = curdriv;
	drive += 'A';

	buildpr(&fcb, buf);
	if (buf[0] == 0)
		cpystr(buf, ".", 0);

	fd = open(buf, 0);

	for (;;) {
		if (recavai) {
			read(0, &ch, 1);
			recavai = 0;
			if (ch != 0x13) {
				close(fd);
				puts("\r\n");
				return;
			}
			read(0, &ch, 1);
			recavai = 0;
		}

		n = read(fd, bbuf, 0x200);
		if (n <= 0) {
			close(fd);
			if (col != 0)
				puts("\r\n");
			break;
		}

		for (p = bbuf; p < bbuf + n; p += 16) {
			if (p[0] | p[1] == 0)
				continue;
			if (strcmp(".", p + 2) == 0)
				continue;
			if (strcmp("..", p + 2) == 0)
				continue;
			if (p[15] != 0)
				continue;
			if (!match(p, &fcb))
				continue;

			cname(p + 2, nbuf);
			found++;

			if (col == 0)
				putch(drive);

			puts(": ");
			putb(nbuf + 1, 8);
			puts(" ");
			putb(nbuf + 9, 3);
			puts(" ");

			if (col == 3) {
				puts("\r\n");
				col = 0;
			} else
				col++;
		}
	}

	if (found == 0)
		puts("NO FILE\r\n");
}

/*
 * era - the CCP's ERA command.  Expands the wildcard into an FCB,
 * opens the directory, and unlinks every plain file that matches.  If
 * nothing matched, the name is echoed back with a question mark.
 */
era(name)
char *name;
{
	char fcb[36];
	char dirbuf[16];
	char path[64];
	char full[64];
	int fd;
	int deleted;

	if (*name == 0) {
		puts("?\r\n");
		return;
	}

	global(fcb, name);
	buildpr(fcb, path);
	fd = open(path, 0);

	deleted = 0;
	while (read(fd, dirbuf, 16) == 16) {
		if (dirbuf[0] | dirbuf[1] == 0)
			continue;
		if (strcmp(".", &dirbuf[2]) == 0)
			continue;
		if (strcmp("..", &dirbuf[2]) == 0)
			continue;
		if (!match(fcb, dirbuf))
			continue;
		cpystr(full, path, "/", &dirbuf[2], 0);
		if (!isplain(full))
			continue;
		if (unlink(full) >= 0)
			deleted = 1;
	}
	close(fd);

	if (!deleted) {
		puts(name);
		puts("?\r\n");
	}
}

/*
 * isplain - is the file a plain one, not a directory or a special.  The
 * mode's file-type bits are zero for a plain file.
 */
isplain(name)
char *name;
{
	if (stat(name, statbuf) < 0)
		return 0;
	if ((statbuf[5] & 0x60) == 0)
		return 1;
	return 0;
}

/*
 * global - turn a name, with an optional "X:" prefix, into the FCB's
 * dr and wildcard name/type, so a command can be run against a pattern.
 */
global(fcb, name)
struct fcb *fcb;
char *name;
{
	raise(name);

	if ((*name >= 'a' && *name <= 'z') || (*name >= 'A' && *name <= 'Z')) {
		if (name[1] == ':') {
			fcb->dr = *name - 0x40;
			name += 2;
		}
	}

	/* the name and type, wildcards and all, copied into the FCB */
	cname(name, fcb);
}

/*
 * isarg - is this command-line argument one of the non-file kinds: a
 * drive descriptor, the program name itself, a drive selector, or a
 * device.  doargs skips these when building the CP/M command tail.
 */
isarg(arg)
char *arg;
{
	if (isdes(arg))
		return 1;
	if (arg == loadfil)
		return 1;
	if (issel(arg))
		return 1;
	if (isdev(arg))
		return 1;
	return 0;
}

/*
 * prompt - the CP/M prompt, the current drive letter and a ">".
 */
prompt()
{
	putch(curdriv + 'A');
	putch('>');
	cflush();
}

/*
 * type - the TYPE command.  Open the file, read it 512 bytes at a
 * time, and echo each character so tabs expand and ^Z ends the listing.
 */
type(fname)
char *fname;
{
	int fd;
	int n;
	char *p;

	argname(&fcb, fname);
	name(&fcb, buf);

	fd = open(buf, 0);
	if (fd < 0) {
		cflush();
		perror(buf);
		puts("\r\n");
		return;
	}

	recavai = 0;
	for (;;) {
		n = read(fd, ccword, 0x200);
		if (n <= 0)
			break;
		p = ccword;
		while (n-- > 0)
			echo(*p++);
	}
	close(fd);
}

/*
 * ren - the REN command.  The new name is on the command line; the old
 * name is after an "=" if given, else the FCB at 0x5c that dofcbs left
 * from the previous command.  Both go into one FCB and rename does it.
 *
 * NOTE: high-level reconstruction; the exact name-parsing loop wants a
 * second, closer pass against the .dis.
 */
ren(name)
char *name;
{
	char newname[16];
	char oldname[16];

	/* copy the new name off the line, stopping at '=' or a blank */
	while (*name && *name != '=' && *name > ' ')
		name++;

	if (*name == '=') {
		/* an old name follows */
		name++;
		while (*name && *name > ' ')
			name++;
	}

	/* rename wants both names packed into one FCB */
	argname(&fcb, name);
	rename(&fcb);
}

/*
 * issel - is this argument a bare drive selector, "X:".
 */
issel(arg)
char *arg;
{
	if (arg[0] != 0 && arg[1] == ':' && arg[2] == 0)
		return 1;
	return 0;
}

/*
 * dosel - act on a drive selector: uppercase and select the drive.
 */
dosel(arg)
char *arg;
{
	raise(arg);
	select(arg[0] - 'A');
}

/*
 * bfree - free n bytes at block, the SAVE allocator's free entry.  The
 * size becomes n/4 nodes and the block is handed to free.
 */
bfree(block, n)
struct node *block;
ushort n;
{
	block->size = n / 4;
	if (block->size)
		free((char *)block + 4);
}

/*
 * free - return a block to the SAVE allocator's list, coalescing with
 * whatever free block is adjacent.
 *
 * NOTE: high-level; the list walk and coalescing want a closer pass.
 */
free(ptr)
char *ptr;
{
	struct node *p;

	if (ptr == 0)
		return;

	setallo();
	p = (struct node *)(ptr - 4);
	p->next = base.next;
	base.next = p;
}

/*
 * rnum - print a number right-justified in a five-character field,
 * the column the STAT listing uses.
 */
rnum(n)
int n;
{
	char buf[10];
	int len;
	int pad;

	len = itob(buf, n, 10);
	buf[len] = 0;

	pad = 5 - strlen(buf);
	while (pad-- > 0)
		putch(' ');

	puts(buf);
}

/*
 * di - the DI command, which is just "ignore signals 1 through 14".
 */
di()
{
	int i;

	for (i = 1; i < 0x0F; i++)
		signal(i, 1);
}

/*
 * statdriv - the drive the STAT command is listing, set by dostat.
 */
uchar statdriv;

/*
 * statone - print the STAT line for one file: the drive letter and
 * name, and either the recs/bytes/ext/access for a plain file or the
 * word "Directory"/"Special" for the other kinds.
 */
statone(name)
char *name;
{
	char st[36];
	int recs, bytes, ext, access;

	cpystr(buf, disktab[statdriv], "/", name, 0);

	if (stat(buf, st) < 0)
		return;

	if ((st[5] & 0x60) == 0x40) {
		puts("Directory             ");
		goto printname;
	}
	if ((st[5] & 0x60) != 0) {
		puts("Special               ");
		goto printname;
	}

	recs = (st[9] << 9) | ((st[10] | (st[11] << 8)) >> 7);
	if (st[10] & 0x7F)
		recs++;
	bytes = (st[9] << 6) | ((st[10] | (st[11] << 8)) >> 10);
	if (st[10] | (st[11] & 0x03))
		bytes++;
	ext = (st[9] << 2) | ((st[10] | (st[11] << 8)) >> 14);
	if (st[10] | (st[11] & 0x3F))
		ext++;

	access = access(buf, 2) >= 0;

	rnum(recs); puts(" ");
	rnum(bytes); puts("k ");
	puts("R/");
	rnum(ext); puts(" ");
	puts(access ? "W" : "O");
	puts(" ");

printname:
	putch(statdriv + 'A');
	puts(":");
	raise(name);
	puts(name);
	puts("\r\n");
}

/*
 * dostat - the STAT command.  Expands the wildcard, opens the
 * directory, and prints one line per matching entry via statone; the
 * header line goes out once, before the first.  ^S pauses the listing.
 */
dostat(name)
char *name;
{
	char *p;
	int fd;
	int n;
	int found;
	char c;

	if (*name == 0)
		name = "*.*";

	global(&fcb, name);

	if (fcb.dr != 0)
		statdriv = fcb.dr - 1;
	else
		statdriv = curdriv;

	buildpr(&fcb, buf);
	if (buf[0] == 0)
		cpystr(buf, ".", 0);

	fd = open(buf, 0);
	recavai = 0;
	found = 0;

	while ((n = read(fd, bbuf, 0x200)) > 0) {
		for (p = bbuf; p < bbuf + n; p += 16) {
			if (recavai) {
				read(0, &c, 1);
				recavai = 0;
				if (c == 0x13) {
					read(0, &c, 1);
					recavai = 0;
				}
			}

			if (p[0] | p[1] == 0)
				continue;
			if (strcmp(".", p + 2) == 0)
				continue;
			if (strcmp("..", p + 2) == 0)
				continue;
			if (p[15] != 0)
				continue;
			if (!match(&fcb, p))
				continue;

			if (!found)
				puts(" Recs  Bytes  Ext Acc\r\n");
			found = 1;
			statone(p + 2);
		}
	}
	close(fd);

	if (!found)
		puts("NO FILE\r\n");
}

/*
 * devop - assign the list device, the "LST:" command.  An empty
 * argument returns it to the console; a "|command" pipes the listing
 * through a child running the command; anything else is a file the
 * listing is appended to.  desc is the descriptor, dev the saved path.
 */
devop(desc, arg, dev)
uchar *desc;
char *arg;
char **dev;
{
	int fd;
	int pid;

	lflush();

	if (*arg == 0) {
		if (*desc > 2)
			close(*desc);
		*desc = 1;
		if (*dev) {
			free(*dev);
			*dev = 0;
		}
		return;
	}

	if (*arg == '|') {
		arg++;
		if (strchr(arg, '/') == 0) {
			cpystr(bbuf, "/bin/", arg, 0);
			arg = bbuf;
		}
		if (access(arg, 1) < 0) {
			perror(arg);
			puts("\r");
			return;
		}
		pipe(pip);
		pid = fork();
		if (pid < 0) {
			perror(0);
			puts("\r");
			return;
		}
		if (pid == 0) {
			di();
			close(0);
			dup(pip[0]);
			close(pip[0]);
			close(pip[1]);
			execl(arg, arg, 0);
			quest(arg);
			_exit(0);
		}
		close(pip[0]);
		if (*desc > 2)
			close(*desc);
		*desc = pip[1];
		cpystr(buf, "| ", arg, 0);
		if (*dev)
			free(*dev);
		*dev = save(buf);
		return;
	}

	if (!fexists(arg)) {
		fd = creat(arg, 0x1FF);
		close(fd);
	}
	fd = open(arg, 2);
	if (fd < 0) {
		cflush();
		perror(arg);
		puts("\r");
		return;
	}
	if (*desc > 2)
		close(*desc);
	*desc = fd;
	if (*dev)
		free(*dev);
	*dev = save(arg);
}

/*
 * system - the "!" command.  Runs a line as a micronix shell command:
 * save the signal handlers, fork, and let the child exec /bin/sh -c
 * while the parent waits and then puts the handlers back.
 */
system(cmd)
char *cmd;
{
	int sigs[16];
	int pid;
	int status;
	int i;

	for (i = 1; i < 0x0F; i++)
		sigs[i] = signal(i, 1);

	pid = fork();

	if (pid != 0) {
		if (pid != -1) {
			while (wait(&status) != pid)
				;
		}
		for (i = 1; i < 0x0F; i++)
			signal(i, sigs[i]);
		return;
	}

	for (i = 1; i < 0x0F; i++)
		signal(i, sigs[i]);

	execl("/bin/sh", "sh", "-c", cmd, 0);
	perror("sh");
	_exit(0);
}

/*
 * The submit-file state, the CP/M batch mechanism behind the "!" tail
 * of a command.  When a line ends in "!", the rest is saved to /$$$.sub
 * and getline reads from it instead of the keyboard until it is empty.
 */
char subfile[64];
int subfd;
int subflag;
int sublines;

/*
 * getline - the next command line.  From the submit file if one is in
 * progress, else a prompt and a line from the keyboard.
 */
getline()
{
	if (disktab[0] != 0)
		cpystr(subfile, disktab[0], "/$$$.sub", 0);
	else
		subfile[0] = 0;

	if (!subflag) {
		if (subfile[0]) {
			cflush();
			if (stat(subfile, statbuf) >= 0) {
				subflag = 1;
				sublines = (statbuf[10] | (statbuf[11] << 8)) / 128;
				subfd = open(subfile, 0);
				if (subfd < 0)
					sublines = 0;
			}
		}
		if (!subflag)
			goto keyboard;
	}

	if (sublines == 0) {
		unlink(subfile);
		subflag = 0;
		close(subfd);
		goto keyboard;
	}

	sublines--;
	seek(subfd, sublines * 128, 0);
	read(subfd, line + 1, 128);
	line[2 + line[1]] = 0;
	prompt();
	puts(line + 2);
	puts("\r\n");
	return;

keyboard:
	for (;;) {
		prompt();
		line[0] = 0x7F;
		readbuf(line);
		putch('\n');
		cflush();
		if (line[1] != 0)
			break;
	}
	line[2 + line[1]] = 0;
}

/*
 * itob - the integer into a buffer, most significant digit first.  The
 * caller nul-terminates; the length is returned.  Hex digits (for a base
 * above 10) come out lowercase, though upm only ever asks for base 10.
 */
itob(buf, value, base)
char *buf;
ushort value;
ushort base;
{
	int len;
	int d;

	len = 0;
	if (value / base)
		len = itob(buf, value / base, base);
	d = value % base;
	buf[len] = (d < 10) ? d + '0' : d - 10 + 'a';
	return len + 1;
}

signal(sig, handler)
int sig;
int handler;
{
	int ret;

	if (sig < 1 || sig > 15)
		return -1;

	if (handler == 0 || handler == 1) {
		ret = _signal(sig, handler);
	} else {
		stab[sig - 1] = handler;
		ret = _signal(sig, (int)&jtab[sig - 1]);
	}

	if (!(ret == 1 || ret == 0 || ret == -1))
		ret = stab[sig - 1];
	return ret;
}
