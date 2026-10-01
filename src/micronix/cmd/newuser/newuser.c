/*
 * newuser
 *
 * cmd/newuser/newuser.c
 *
 * A reconstruction of /bin/newuser off the Micronix 1.6 distribution -
 * inode 29 of 1012-8_dist_2.IMD, 17526 bytes, mode 4711 - from its
 * disassembly.  There is no source for it.  README says what the binary
 * is and how each function below was read out of it; the addresses in
 * the comments are the binary's.
 *
 * It adds one account.  It asks for a login name and a group, asks for
 * a comment, takes the lowest uid from 11 up that /etc/passwd has not
 * used, makes the home directory /a/<name> and gives it to the new user,
 * and writes the new line into /etc/passwd - and, when the group had to
 * be made, into /etc/group.  The account has no password: passwd(1)
 * sets one.
 *
 * Both files are held in memory as linked lists and written back whole.
 * That is the original's method and it is kept, and so is the lock it
 * takes round the rewrite: /etc/passwdL is made as a second name for
 * /etc/passwd itself, so that link() is the test-and-set, and the one
 * byte read and written back through it is what stamps the file with
 * the time the lock was taken.  A lock older than a minute is taken to
 * belong to a dead process and is broken.
 *
 * What was not carried over as it stood:
 *
 * The exit status was inverted.  The 1982 library's exit passes its
 * argument straight to the kernel, and the program says exit(1) when it
 * has finished and exit(0) on every error, so a script saw failure as
 * success and the reverse.  It is 0 for an account added and 1 for
 * anything else here.  The duplicate-name path called exit with no
 * argument at all, which left whatever was on the stack as the status.
 *
 * End of file on the keyboard ended nothing.  The line reader gave back
 * an empty string, which is not a valid name, which asked again, for
 * ever.  It gives up here.
 *
 * The /etc/passwd reader is libc's getpwent, which is the same module -
 * the binary carries its own copy of it at 15ee - and the directory
 * creation is libc's mkdir, which is the mknod and the two links the
 * binary does by hand at 0c61.  The binary's chown, link and the rest
 * were the 1982 system call stubs and are libu's here.
 *
 * Where the original gave up with the lock still held - it left it to
 * the minute to time out - this puts the lock down first.
 *
 * Two routines in the binary are called by nothing and are not carried
 * over.  One, at 0264, prints "Removing <name> from the system.  OK? ",
 * reads the answer, and drops the account from the list; the program
 * has no way to ask for it, and its strings are the only trace.  The
 * other, at 0bfb, is the group lookup the other way round - a number to
 * a name, or to the number as text when there is no such group.
 *
 * vim: tabstop=8 shiftwidth=8 noexpandtab:
 */

#include <types.h>
#include <stdio.h>
#include <pwd.h>
#include <errno.h>
#include <sys/signal.h>
#include <sys/fs.h>
#include <sys/stat.h>

#define	PASSWD	"/etc/passwd"
#define	LOCK	"/etc/passwdL"
#define	GROUP	"/etc/group"
#define	HOME	"/a/"
#define	SHELL	"/bin/sh"

#define	FIRSTUID	11
#define	STALE		60L

/*
 * /etc/passwd as a list.  The entry is a pointer, and the removal
 * routine that nothing calls left it NULL for an account that was
 * dropped, which is why the walkers below all look.
 */
struct pnode {
	struct pnode *next;
	struct passwd *ent;
};

/*
 * /etc/group as a list.  The gid is one byte, which is what a group
 * number is on this machine; the members stay the one string they are
 * in the file.  The binary's node is nine bytes in this order.
 */
struct gnode {
	unsigned char gid;
	char *name;
	char *pass;
	char *mem;
	struct gnode *next;
};

struct pnode *pwlist;
struct gnode *grlist;
char	*grfile	= GROUP;
char	grdirty;
FILE	*pwfp;
FILE	*grfp;

extern int errno;
char	*strdup();

/*
 * Write to the terminal a line at a time, unbuffered: the prompts have
 * no newline and must be there before the read.  (08c6)
 */
say(s)
	char *s;
{
	write(1, s, strlen(s));
}

/*
 * A prompt and the line that answers it, newline gone.  (0941)
 */
prompt(msg, buf)
	char *msg, *buf;
{
	register char *p;
	int n;

	say(msg);
	n = read(0, buf, 511);
	if (n <= 0) {
		say("\n");
		exit(1);
	}
	buf[n] = '\0';
	for (p = buf; *p; p++)
		if (*p == '\n')
			*p = '\0';
}

/*
 * Two letters at least, and lower case a to z and nothing else.  The
 * message says "alphabetic"; this is what it means.  (01c2)
 */
goodname(s)
	register char *s;
{
	if (strlen(s) < 2)
		return 0;
	for (; *s; s++)
		if (*s < 'a' || *s > 'z')
			return 0;
	return 1;
}

/*
 * Ask until the name will do.  (016f)
 */
getname(buf)
	char *buf;
{
	for (;;) {
		prompt("Name of user: ", buf);
		if (goodname(buf))
			return;
		say("User names must be alphabetic and at least 2 characters long\n");
	}
}

/*
 * The lock.
 *
 * An old lock is broken before it is waited for.  Its age is how long
 * since the inode was written, and because the lock is the password
 * file's own inode the write that stamps it is the one lock() makes -
 * so the age of a live lock is the time it has been held.  (14d6)
 */
stale(path)
	char *path;
{
	struct stat sb;
	long now, age;

	if (stat(path, &sb) < 0)
		return 0;
	time(&now);
	age = now - sb.st_mtime;
	if (age < 0)
		age = -age;
	return age > STALE;
}

exists(path)
	char *path;
{
	struct stat sb;

	return stat(path, &sb) >= 0;
}

lock()
{
	char c;
	int fd;

	for (;;) {
		if (stale(LOCK))
			unlink(LOCK);
		if (link(PASSWD, LOCK) < 0) {
			sleep(1);
			continue;
		}
		fd = open(LOCK, 2);
		read(fd, &c, 1);
		seek(fd, 0, 0);
		write(fd, &c, 1);
		close(fd);
		if (exists(LOCK))
			return;
		sleep(1);
	}
}

unlock()
{
	unlink(LOCK);
}

/*
 * Give up: the lock goes down first.
 */
quit()
{
	unlock();
	exit(1);
}

/*
 * Ignore signals 1 through 14, so that the rewrite is not left half
 * done.  (09cb)
 */
ignore()
{
	register int i;

	for (i = 1; i < 15; i++)
		signal(i, SIG_IGN);
}

/*
 * /etc/passwd, in.  Every field of every entry is copied, because
 * getpwent's record is overwritten by the next call.  The list is built
 * newest first and written back by recursion, which puts it right way
 * round.  (056c, 0681)
 */
addpw(p)
	struct passwd *p;
{
	register struct pnode *n;

	n = (struct pnode *)calloc(1, sizeof *n);
	n->ent = (struct passwd *)calloc(1, sizeof *n->ent);
	memcpy(n->ent, p, sizeof *p);
	n->ent->name = strdup(p->name);
	n->ent->passwd = strdup(p->passwd);
	n->ent->person = strdup(p->person);
	n->ent->dir = strdup(p->dir);
	n->ent->shell = strdup(p->shell);
	n->next = pwlist;
	pwlist = n;
}

readpw()
{
	register struct passwd *p;

	pwlist = NULL;
	while ((p = getpwent()) != NULL)
		addpw(p);
	endpwent();
}

/*
 * A name or a number already in the passwd list.  (08ee, 0f0c)
 */
namefound(name)
	char *name;
{
	register struct pnode *n;

	for (n = pwlist; n; n = n->next)
		if (strcmp(n->ent->name, name) == 0)
			return 1;
	return 0;
}

uidused(uid)
	register int uid;
{
	register struct pnode *n;

	for (n = pwlist; n; n = n->next)
		if (n->ent != NULL && n->ent->uid == uid)
			return 1;
	return 0;
}

/*
 * The lowest free uid from 11, or 0 when there is none.  0 is root's
 * and is never free, so it is the answer that cannot be a uid.  (0f89)
 */
newuid()
{
	register int uid;

	for (uid = FIRSTUID; uid < 256; uid++)
		if (!uidused(uid))
			return uid;
	say("No more user ID numbers available!\n");
	return 0;
}

/*
 * /etc/passwd, out.  (0709, 075b)
 */
putpw(n)
	register struct pnode *n;
{
	register struct passwd *e;

	if (n == NULL)
		return;
	putpw(n->next);
	e = n->ent;
	if (e == NULL)
		return;
	fprintf(pwfp, "%s:%s:%d:%d:%s:%s:%s\n", e->name, e->passwd,
	    e->uid, e->gid, e->person, e->dir, e->shell);
}

writepw()
{
	pwfp = fopen(PASSWD, "w");
	if (pwfp == NULL) {
		perror(PASSWD);
		quit();
	}
	putpw(pwlist);
	fclose(pwfp);
}

/*
 * /etc/group, in.  A line is name:password:gid:members, and one that
 * has fewer than three colons is not a group and is passed over.
 * (0a04, 113d)
 */
addgr(name, pass, gid, mem)
	char *name, *pass, *mem;
	int gid;
{
	register struct gnode *g;

	g = (struct gnode *)malloc(sizeof *g);
	g->name = strdup(name);
	g->pass = strdup(pass);
	g->gid = gid;
	g->mem = strdup(mem);
	g->next = grlist;
	grlist = g;
}

readgr()
{
	char buf[512];
	register char *p;
	char *pass, *gid, *mem;
	FILE *fp;

	grlist = NULL;
	fp = fopen(grfile, "r");
	if (fp == NULL) {
		perror(grfile);
		quit();
	}
	while (fgets(buf, sizeof buf, fp) != NULL) {
		for (p = buf; *p; p++)
			if (*p == '\n')
				*p = '\0';
		for (p = buf; *p && *p != ':'; p++)
			;
		if (*p == '\0')
			continue;
		*p++ = '\0';
		pass = p;
		for (; *p && *p != ':'; p++)
			;
		if (*p == '\0')
			continue;
		*p++ = '\0';
		gid = p;
		for (; *p && *p != ':'; p++)
			;
		if (*p == '\0')
			continue;
		*p++ = '\0';
		mem = p;
		for (; *p && *p != ':'; p++)
			;
		*p = '\0';
		addgr(buf, pass, atoi(gid), mem);
	}
	fclose(fp);
}

/*
 * Show the group names.  The list is newest first, so the recursion
 * puts them in the order of the file.  (0fd4)
 */
showgr(g)
	register struct gnode *g;
{
	if (g == NULL)
		return;
	showgr(g->next);
	say("\t");
	say(g->name);
	say("\n");
}

/*
 * The group a name is, and its number through gidp.  (0b96)
 */
findgr(name, gidp)
	char *name;
	char *gidp;
{
	register struct gnode *g;

	for (g = grlist; g; g = g->next)
		if (strcmp(g->name, name) == 0) {
			*gidp = g->gid;
			return 1;
		}
	return 0;
}

/*
 * The lowest group number from 1 that no group has.  (11f9)
 */
newgid()
{
	register int gid;
	register struct gnode *g;

	for (gid = 1; gid < 256; gid++) {
		for (g = grlist; g && g->gid != gid; g = g->next)
			;
		if (g == NULL)
			return gid;
	}
	say("Out of group numbers\n");
	quit();
}

/*
 * The number of a group by name, making the group if there is none -
 * which is what makes /etc/group want writing.  (1594)
 */
grpnum(name)
	char *name;
{
	static char gid;

	if (findgr(name, &gid))
		return gid;
	gid = newgid();
	addgr(name, "", gid, "");
	grdirty = 1;
	return gid;
}

/*
 * /etc/group, out - if the group list changed.  (12c3, 1318)
 */
putgr(g)
	register struct gnode *g;
{
	if (g == NULL)
		return;
	putgr(g->next);
	fprintf(grfp, "%s:%s:%d:%s\n", g->name, g->pass, g->gid, g->mem);
}

writegr()
{
	if (!grdirty)
		return;
	grfp = fopen(GROUP, "w");
	if (grfp == NULL) {
		perror(GROUP);
		quit();
	}
	putgr(grlist);
	fclose(grfp);
}

/*
 * Ask which group the new user is in.  The groups are listed first, and
 * the answer is held to the same rule as a login name: it is either one
 * of them or a new one.  (10bf)
 */
askgroup()
{
	char buf[512];

	readgr();
	for (;;) {
		say("Group names:\n");
		showgr(grlist);
		say("You may select one from above or create a new group.\n");
		prompt("Select a group for the new user: ", buf);
		if (goodname(buf))
			return strdup(buf);
		say("group names must be lower case, at least 2 letters long.\n");
	}
}

/*
 * The directory a path is in.  A path with no slash is in ".", and
 * trailing slashes do not count; the root's is the root.  (0e04)
 */
char *
dirname(path)
	char *path;
{
	static char buf[512];
	register char *end;

	if (strchr(path, '/') == NULL)
		return ".";
	strcpy(buf, path);
	end = buf + strlen(buf) - 1;
	while (buf < end && *end == '/')
		*end-- = '\0';
	while (buf < end && *end != '/')
		*end-- = '\0';
	while (buf < end && *end == '/')
		*end-- = '\0';
	return buf;
}

/*
 * mkdir -p.  The parents are made first, root's own, and only the
 * last is handed over to the new user.  (0c61)
 */
mkpath(path)
	char *path;
{
	char parent[512];

	if (*path == '\0' || strcmp(path, "/") == 0 || strcmp(path, ".") == 0)
		return;
	strcpy(parent, dirname(path));
	mkpath(parent);
	if (exists(path))
		return;
	if (access(parent, 2) < 0 || mkdir(path, 0777) != 0)
		perror(path);
}

/*
 * The whole job, once there is a name.  (03a5)
 */
adduser(name)
	char *name;
{
	char buf[512];
	register char *p;
	struct passwd pw;
	char *group;
	int uid, gid;

	memset(&pw, 0, sizeof pw);
	pw.name = strdup(name);
	group = askgroup();
	sprintf(buf, "%s%s", HOME, name);
	pw.dir = strdup(buf);
	pw.shell = SHELL;
	pw.passwd = "";
	say("(This entry is optional.\n");
	prompt("Comment (Real name and/or phone number and/or office, etc): ", buf);
	pw.person = strdup(buf);

	/*
	 * The colon is the file's separator and a comment cannot carry one.
	 */
	for (p = pw.person; *p; p++)
		if (*p == ':')
			*p = ';';

	ignore();
	lock();
	readpw();
	readgr();
	if (namefound(pw.name)) {
		say(pw.name);
		say(": User name already exists\n");
		quit();
	}
	uid = newuid();
	gid = grpnum(group);
	if (uid == 0)
		quit();
	pw.uid = uid;
	pw.gid = gid;
	addpw(&pw);
	mkpath(pw.dir);
	chown(pw.dir, uid | (gid << 8));
	writepw();
	writegr();
	say("New user added.\n");
	unlock();
}

main()
{
	char name[512];

	if (getuid() != 0) {
		errno = EPERM;
		perror("newuser");
		exit(1);
	}
	getname(name);
	adduser(name);
	exit(0);
}
