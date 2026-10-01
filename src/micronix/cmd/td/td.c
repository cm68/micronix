/*
 * td - tree dump
 *
 * cmd/td/td.c
 *
 * A reconstruction of /bin/td off the Micronix 1.6 distribution - inode
 * 21 of 1012-8_dist_2.IMD, 21326 bytes, mode 4711 - from its
 * disassembly.  There is no source for it.  README says how it was
 * read, and what the file that stood here before it got wrong; the
 * addresses in the comments are the binary's.
 *
 * td [-v] [-a] [-i] [-u] [-hN] [-dN] [source [destination]]
 *
 * It copies the tree under SOURCE to DESTINATION, which is a directory
 * or a block special file.  A directory is copied into.  A block
 * device is a file system: it is mounted on a directory made for the
 * purpose in /tmp, the tree is copied into that, and when the file
 * system fills up td asks for the next disk, which is mounted in turn
 * and the file that did not fit copied again from its start.  The
 * result is a set of ordinary file systems that hold the tree between
 * them, and that is what a restore is: cp.
 *
 *	-v	say what is done to each file
 *	-a	copy the files of more than a thousand blocks too, which
 *		are otherwise left behind
 *	-i	incremental: only the files changed since the last dump
 *		of this source, the time of which /etc/dtab keeps by the
 *		device and inode of the source directory
 *	-u	when the dump has finished, write its time into /etc/dtab
 *	-hN	only the files changed in the last N hours
 *	-dN	only the files changed in the last N days
 *
 * With no source it asks for one, and for the destination, and whether
 * the dump is to be full or incremental - and an incremental dump begun
 * that way updates /etc/dtab.  An option letter it does not know does
 * the same.  It will not run unless the standard input, output and
 * error are all terminals, because the media change is a conversation.
 *
 * The files copied are the regular ones, with their modes - the set-id
 * bits included - and their owners; the directories are made with
 * mode 0777 and are not given to anybody; special files are made again
 * with mknod when it is the super-user dumping, and are named and
 * passed over when it is not.  The destination is not copied into
 * itself, when it is in the source.
 *
 * A file left behind - older than the time asked for, or too big - is
 * reported with " - Not dumped." and when it is being talked to with
 * its name in front of that.  The name is conditional and the rest is
 * not, so a quiet dump prints the bare remainder of the line for every
 * file it passes over.  It is the original's, and so is this.
 *
 * What was not carried over as it stood:
 *
 * -hN and -dN could not have worked.  The number was converted by a
 * library routine that is handed 512 as the length of the string, and
 * works to the string's address plus that, which with the arguments at
 * the top of memory is a pointer that has wrapped, so every number came
 * out 0.  They do here.
 *
 * The exit status was inverted: 1 for a good dump and 0 for every
 * mistake, and the good dump is what updates /etc/dtab.  It is 0 and 1
 * here.
 *
 * With a source and no destination the original stat'ed a null pointer.
 * This asks, as it does when it has neither.
 *
 * vim: tabstop=8 shiftwidth=8 noexpandtab:
 */

#include <types.h>
#include <stdio.h>
#include <errno.h>
#include <sys/signal.h>
#include <sys/fs.h>
#include <sys/stat.h>
#include <sys/dir.h>

#define	DTAB		"/etc/dtab"
#define	BIGFILE		1000	/* blocks: the size that wants -a */
#define	PATHSZ		512
#define	BUFSZ		8192
#define	NOSIG		7	/* the one signal not touched */

extern int errno;

int	aflag;			/* -a       (5000) */
int	devdest;		/* the destination is a device  (5002) */
int	iflag;			/* -i       (5004) */
int	uflag;			/* -u       (5006) */
int	verbose;		/* -v       (5008) */
int	mounted;		/* a volume is mounted  (500a) */
char	*source;		/* the tree  (500c) */
char	*destdir;		/* where files go: the mount point, or the
				   destination itself  (500e) */
char	*destarg;		/* what was said for the destination  (5010) */
int	srclen;			/* strlen(source)  (5212) */
int	volume;			/* disks used so far  (5214) */
long	now;			/* the time the command began  (5216) */
long	since;			/* files older than this are left  (521a) */
struct	stat deststat;		/* who the destination directory is  (521e) */

int	oldsig[16];		/* the dispositions found at the start */
char	srcbuf[PATHSZ];		/* what was typed  (5252) */
char	dstbuf[PATHSZ];		/* what was typed  (52d2) */
char	newdev[PATHSZ];		/* a device named at a media change (5806) */
char	*iobuf;			/* the copy buffer, from the heap: it is
				   8k and the file need not carry it */
int	bufsz = BUFSZ;
char	walkpath[PATHSZ];	/* the path the walk is at  (55ae) */

char	*strchr(), *strrchr();

int	catch();

/*
 * Signals.  They are all ignored while a disk is changed, and are
 * otherwise caught and made to clean up, if they were not ignored to
 * begin with.  (1058, 10b4)
 */
sigoff()
{
	register int i;

	for (i = 1; i <= 15; i++)
		if (i != NOSIG)
			oldsig[i] = signal(i, SIG_IGN);
}

sigon()
{
	register int i;

	for (i = 1; i <= 15; i++)
		if (i != NOSIG && oldsig[i] != SIG_IGN)
			signal(i, catch);
}

catch()
{
	cleanup(0);
}

/*
 * The messages go to the standard error unbuffered, as the original's
 * did, and the prompts and the answers to the standard output.  (0cbd,
 * 0ce2)
 */
errs(s)
	char *s;
{
	write(2, s, strlen(s));
}

errn(n)
	int n;
{
	char buf[12];

	sprintf(buf, "%d", n);
	errs(buf);
}

/*
 * Prompt and read the answer; end of file is the end.  (1133)
 */
ask(prompt, buf)
	char *prompt, *buf;
{
	fputs(prompt, stdout);
	fflush(stdout);
	if (gets(buf) == NULL)
		cleanup(0);
}

exists(path)
	char *path;
{
	struct stat sb;

	return stat(path, &sb) >= 0;
}

/*
 * The directory a path is in, and the last part of it.  (0d46, 0e95)
 */
char *
dirname(path)
	char *path;
{
	static char buf[PATHSZ];
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

char *
basename(path)
	register char *path;
{
	while (strchr(path, '/') != NULL)
		path++;
	return path;
}

/*
 * mkdir -p, by mknod and two links, with the parents made first.  This
 * is newuser's, and the library's before it.  (16ca)
 */
mkpath(path)
	char *path;
{
	char parent[PATHSZ];

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
 * Whether a device holds a file system, which it does if the first
 * block of the data is a directory that begins with "." and ".." both
 * of inode 1: the superblock is read for where that is.  (1a75)
 */
isfs(dev)
	char *dev;
{
	int fd;
	static struct super sb;
	struct dir e[2];

	fd = open(dev, 0);
	if (fd < 0) {
		perror(dev);
		return 0;
	}
	seek(fd, 1, 3);
	if (read(fd, &sb, 415) != 415) {
		close(fd);
		return 0;
	}
	seek(fd, sb.s_isize + 2, 3);
	if (read(fd, e, sizeof e) != sizeof e) {
		close(fd);
		return 0;
	}
	close(fd);
	if (e[0].ino != 1 || e[1].ino != 1)
		return 0;
	if (strcmp(e[0].name, ".") != 0 || strcmp(e[1].name, "..") != 0)
		return 0;
	return 1;
}

/*
 * /etc/dtab is a file of eight byte records - the device, the inode
 * and the time of the last dump.  (188e, 1977)
 */
struct dtab {
	unsigned dev;
	unsigned ino;
	long time;
};

dtfind(dev, ino)
	unsigned dev, ino;
{
	struct dtab r;
	int fd;

	fd = open(DTAB, 0);
	if (fd < 0) {
		perror(DTAB);
		cleanup(0);
	}
	while (read(fd, &r, sizeof r) == sizeof r) {
		if (r.dev == dev && r.ino == ino) {
			close(fd);
			since = r.time;
			return;
		}
	}
	close(fd);
	errs("Previous table entry not found - defaulting to full dump\n");
	since = 0;
}

dtput(r)
	struct dtab *r;
{
	struct dtab o;
	int fd;

	if (!exists(DTAB))
		close(creat(DTAB, 0777));
	chmod(DTAB, 0777);
	fd = open(DTAB, 2);
	if (fd < 0) {
		perror(DTAB);
		cleanup(0);
	}
	while (read(fd, &o, sizeof o) == sizeof o) {
		if (o.dev == r->dev && o.ino == r->ino) {
			seek(fd, -8, 1);
			break;
		}
	}
	if (write(fd, r, sizeof *r) != sizeof *r)
		perror(DTAB);
	close(fd);
}

/*
 * The end, by whatever way.  The disk is let go and the directory it
 * was mounted on taken away, and a dump that finished - and only that -
 * writes its time into /etc/dtab if it was asked to.  (1447)
 */
cleanup(ok)
	int ok;
{
	register int i;
	struct stat sb;
	struct dtab r;

	fflush(stdout);
	sigoff();
	for (i = 0; i < 16; i++)
		close(i);
	if (mounted && umount(destarg) < 0)
		perror(destarg);
	if (destdir && devdest)
		unlink(destdir);
	if (!ok)
		exit(1);
	if (stat(source, &sb) < 0) {
		perror(source);
		exit(1);
	}
	r.dev = sb.st_dev;
	r.ino = sb.st_ino;
	r.time = now;
	if (uflag)
		dtput(&r);
	exit(0);
}

/*
 * A name for the directory a device is mounted on: /tmp, the process
 * number, a dash and the clock, as it was.  (33ca)
 */
char *
mntname()
{
	static char name[PATHSZ];
	long t;

	time(&t);
	sprintf(name, "/tmp/%d-%ld", getpid(), t % 10000L);
	return name;
}

/*
 * Run a command line with the shell, as system() does, but with the
 * original's own complaint when the shell cannot be had: signals are
 * ignored in the parent while the child runs, and the child puts them
 * back before it execs.  (325d)
 */
shell(cmd)
	char *cmd;
{
	int old[16];
	register int i;
	int pid, w;

	for (i = 1; i <= 15; i++)
		old[i] = signal(i, SIG_IGN);
	pid = fork();
	if (pid == 0) {
		for (i = 1; i <= 15; i++)
			signal(i, old[i]);
		execl("/bin/sh", "sh", "-c", cmd, (char *)0);
		perror("sh");
		_exit(0);
	}
	if (pid != -1)
		while ((w = wait((int *)0)) != pid && w != -1)
			;
	for (i = 1; i <= 15; i++)
		signal(i, old[i]);
}

/*
 * Another disk.  The one mounted is let go of, the operator is asked
 * for the next, and it is mounted - unless it has no file system, in
 * which case one can be made.  The first disk comes through here as
 * well.  (12a6)
 */
newvolume()
{
	char buf[PATHSZ];
	char cmd[PATHSZ];
	int c;

	sigoff();
	if (mounted && umount(destarg) < 0) {
		perror(destarg);
		cleanup(0);
	}
	mounted = 0;
	sigon();
	for (;;) {
		errs("Insert new media and press RETURN to continue\n");
		errs("or type the name of a new device containing a file system.\n");
		ask("\t-> ", buf);
		if (buf[0]) {
			strcpy(newdev, buf);
			destarg = newdev;
		}
		while (!isfs(destarg)) {
			errs(destarg);
			errs(": Not a file system\n");
			ask("Do you want to make a file system on this device?", buf);
			c = buf[0];
			if (c >= 'a' && c <= 'z')
				c += 'A' - 'a';
			if (c != 'Y')
				goto again;
			strcpy(cmd, "mkfs ");
			strcat(cmd, destarg);
			shell(cmd);
		}
		sigoff();
		if (umount(destarg) < 0 && errno == EBUSY) {
			sigon();
			perror(destarg);
			errs("The device is mounted and someone else is using it just now!\n");
			continue;
		}
		if (mount(destarg, destdir, 0) < 0) {
			sigon();
			perror(destarg);
			continue;
		}
		if (volume) {
			errs("The previous file system should be labeled volume #");
			errn(volume);
			errs("\n");
		}
		mounted = 1;
		volume++;
		sigon();
		return;
again:		;
	}
}

/*
 * Whether this file is to be copied.  It is not if it is older than
 * the time to go back to, or if it is of more than a thousand blocks
 * and nothing was said.  (1587)
 */
wanted(name, sb)
	char *name;
	struct stat *sb;
{
	unsigned long size;
	unsigned blocks;

	size = ((unsigned long)sb->st_size0 << 16) + sb->st_size1;
	blocks = size >> 9;
	if (size & 0777)
		blocks++;
	if ((unsigned long)sb->st_mtime < (unsigned long)since ||
	    (blocks > BIGFILE && !aflag)) {
		if (verbose)
			errs(name);
		errs(" - Not dumped.\n");
		return 0;
	}
	if (verbose) {
		errs(name);
		errs(" - ");
		errn(blocks);
		errs(" Blocks\n");
	}
	return 1;
}

/*
 * Copy a file.  If the disk fills the next is asked for and the file is
 * begun again on it.  (0847)
 */
copyfile(from, to, sb)
	char *from, *to;
	struct stat *sb;
{
	struct stat ds;
	char dir[PATHSZ];
	int fi, fo, n;

	if (stat(to, &ds) >= 0) {
		if (access(to, 2) < 0) {
			perror(to);
			return 0;
		}
	} else {
		if (access(dirname(to), 2) < 0 && errno == EPERM) {
			perror(dirname(to));
			return 0;
		}
	}
	fi = open(from, 0);
	if (fi < 0) {
		perror(from);
		return 0;
	}
	for (;;) {
		strcpy(dir, dirname(to));
		mkpath(dir);
		fo = creat(to, sb->st_mode & 06777);
		if (fo < 0) {
			close(fi);
			perror(to);
			return 0;
		}
		chown(to, sb->st_uid | (sb->st_gid << 8));
		for (;;) {
			n = read(fi, iobuf, bufsz);
			if (n == 0) {
				close(fi);
				close(fo);
				return 1;
			}
			if (n < 0) {
				perror(from);
				close(fi);
				close(fo);
				return 0;
			}
			if (write(fo, iobuf, n) != n)
				break;
		}
		if (devdest && errno == ENOSPC) {
			puts("\007This file system is full!.\007");
			close(fo);
			unlink(to);
			newvolume();
			seek(fi, 0, 0);
			continue;
		}
		perror(to);
		close(fi);
		close(fo);
		return 0;
	}
}

/*
 * One thing found in the tree.  The answer is whether to go on into it
 * if it is a directory.  (0ace)
 */
visit(path, sb)
	char *path;
	struct stat *sb;
{
	char to[PATHSZ];
	char *name, *sep;
	int len;

	if (strcmp(path, source) == 0)
		return 1;
	name = basename(path);
	if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
		return 1;
	len = strlen(destdir);
	if (destdir[len - 1] == '/' || path[srclen] == '/')
		sep = "";
	else
		sep = "/";
	strcpy(to, destdir);
	strcat(to, sep);
	strcat(to, path + srclen);
	if (sb->st_dev == deststat.st_dev && sb->st_ino == deststat.st_ino)
		return 0;

	switch (sb->st_mode & S_IFMT) {
	case S_IFREG:
		if (wanted(path, sb))
			copyfile(path, to, sb);
		return 1;
	case S_IFDIR:
		mkpath(to);
		return 1;
	}
	if (getuid() != 0) {
		if (verbose) {
			errs(path);
			errs(": Special file not copied\n");
		}
		return 1;
	}
	if (verbose) {
		errs(path);
		errs(" - Special file\n");
	}
	if (mknod(to, sb->st_mode, sb->st_addr[0]) < 0)
		perror(to);
	return 1;
}

/*
 * The walk.  The path being visited is kept in one buffer and grown and
 * cut back in place; "." and ".." are visited like the rest and passed
 * over.  (0ed1)
 */
walk(end)
	char *end;
{
	struct stat sb;
	struct dir d;
	FILE *fp;
	char *p;
	int dots, skip;

	if (stat(walkpath, &sb) < 0)
		return;
	skip = !visit(walkpath, &sb);
	if ((sb.st_mode & S_IFMT) != S_IFDIR)
		return;
	if (skip)
		return;
	fp = fopen(walkpath, "r");
	if (fp == NULL)
		return;
	for (;;) {
		if (fread(&d, 16, 1, fp) != 1)
			break;
		if (d.ino == 0)
			continue;
		dots = strcmp(d.name, ".") == 0 || strcmp(d.name, "..") == 0;
		if (dots)
			continue;
		p = end;
		if (end[-1] != '/')
			*p++ = '/';
		strcpy(p, d.name);
		walk(p + strlen(p));
		*end = '\0';
	}
	fclose(fp);
}

/*
 * The questions asked when there is nothing on the command line.
 * (06ef)
 */
interact()
{
	char ans[PATHSZ];
	int c;

	verbose = 1;
	puts("Td is used to dump an entire tree of files and directories.");
	ask("Source directory: ", srcbuf);
	if (srcbuf[0] == '\0')
		cleanup(0);
	ask("Destination device: ", dstbuf);
	if (dstbuf[0] == '\0')
		cleanup(0);
	source = srcbuf;
	destarg = dstbuf;
	for (;;) {
		errs("You can dump all the files in the directory or\n");
		errs("only those files that have changed since\n");
		errs("the last incremental dump of this directory\n");
		ask("Do you want a full or an incremental dump (F/I) ? ", ans);
		c = ans[0];
		if (c >= 'a' && c <= 'z')
			c += 'A' - 'a';
		if (c == 'F')
			return;
		if (c == 'I') {
			iflag = 1;
			uflag = 1;
			return;
		}
	}
}

/*
 * The time to go back to, moved back by n times a unit; the first to
 * say starts from now.  (0164)
 */
goback(n, unit)
	long n, unit;
{
	if (since == 0)
		since = now;
	since -= n * unit;
}

/*
 * One argument that begins with a dash, a letter at a time.  (0164)
 */
options(s)
	register char *s;
{
	long n, unit;

	for (s++; *s; s++) {
		switch (*s) {
		case 'v':
			verbose = 1;
			break;
		case 'a':
			aflag = 1;
			break;
		case 'i':
			iflag = 1;
			break;
		case 'u':
			uflag = 1;
			break;
		case 'h':
		case 'd':
			unit = *s == 'h' ? 3600L : 86400L;
			n = 0;
			while (s[1] >= '0' && s[1] <= '9')
				n = n * 10 + (*++s - '0');
			goback(n, unit);
			break;
		default:
			interact();
			break;
		}
	}
}

/*
 * The command line, and the work to do before the walk begins: what
 * is to be dumped, and where.  (03b1)
 */
setup(argc, argv)
	int argc;
	char *argv[];
{
	register int i;
	struct stat sb;
	char *a;

	if (!isatty(0) || !isatty(1) || !isatty(2)) {
		puts("This program must be run interactively to allow media change.");
		cleanup(0);
	}
	time(&now);
	sigoff();
	sigon();
	for (i = 1; i < argc; i++) {
		a = argv[i];
		if (a[0] == '-')
			options(a);
		else if (source == NULL)
			source = a;
		else if (destarg == NULL)
			destarg = a;
	}
	if (source == NULL || destarg == NULL)
		interact();
	if (stat(source, &sb) < 0) {
		perror(source);
		cleanup(0);
	}
	if (iflag)
		dtfind(sb.st_dev, sb.st_ino);
	if (stat(destarg, &sb) < 0) {
		perror(destarg);
		cleanup(0);
	}
	switch (sb.st_mode & S_IFMT) {
	case S_IFBLK:
		devdest = 1;
		destdir = mntname();
		if (mknod(destdir, 040777, 0) < 0) {
			perror(destdir);
			cleanup(0);
		}
		break;
	case S_IFDIR:
		devdest = 0;
		destdir = destarg;
		break;
	default:
		errs("Dump destination must be either block special or directory.\n");
		cleanup(0);
	}
	if (devdest)
		newvolume();
}

main(argc, argv)
	int argc;
	char *argv[];
{
	while ((iobuf = malloc(bufsz)) == NULL && (bufsz >>= 1) >= 512)
		;
	if (iobuf == NULL) {
		errs("td: Out of memory\n");
		exit(1);
	}
	setup(argc, argv);
	if (!exists(source)) {
		errno = ENOENT;
		perror(source);
		cleanup(0);
	}
	stat(destdir, &deststat);
	srclen = strlen(source);
	strcpy(walkpath, source);
	walk(walkpath + srclen);
	if (verbose)
		puts("Tree dump complete.");
	cleanup(1);
}
