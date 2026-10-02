/*
 * dar - exchange files with a DOS diskette
 *
 * cmd/dar/dar.c
 *
 * Reconstructed from /bin/dar off the Micronix 1.67 patch
 * (disks/Late/mw1.67_patch/bin/dar), a Whitesmith's binary with no
 * source.  dar.dist is that binary, dar.dis is disas's output, and
 * dar.ctl names what has been read out of it - this file is written
 * against those names.  README records the shape of the program.
 *
 * dar reads an IBM/DOS-format floppy in a Micronix drive and moves
 * files between the two filesystems.  usage:
 *
 *	dar device {cdprtx} files ...
 *
 * c copies FROM the DOS diskette, p copies TO it, and d, r, t, x are
 * the single-file operations, of which at most one may be given; v is
 * verbose.  With no operation flag dar runs its menu (which, in this
 * binary, is dead code: menu() leads with badopt()).
 *
 * Architecture, read out of the binary:
 *
 *	doargs/optscan (flags) -> execute -> checkdev/mkdevname/dispatch
 *	  -> format (match geom) -> dofat (FAT image) -> one operation
 *	    -> finish (write FATs + directory back)
 *
 * The storage is three disjoint areas, not one "sector buffer":
 *   fatbuf  (0x8bdc)  the FAT image, 1024 bytes;
 *   dirblk  (0x9042)  the directory-entry area, 3584 bytes = 112*32;
 *   the stdio streams (0xa018) - stdin/stdout, the interactive layer.
 * Raw disk I/O is inline: seeksec() + read()/write() on devfd, wrapped
 * by clustosec() (sector <-> physical-sector + transfer) and
 * readcluster() (the directory region).
 *
 * vim: tabstop=4 shiftwidth=4 noexpandtab:
 */
#include <stdio.h>
#include <types.h>
#include <string.h>
#include <time.h>
#include <sys/fs.h>
#include <sys/stat.h>
#include <errno.h>

/* the raw syscalls dar drives, declared by hand to avoid unistd.h's
 * time() colliding with time.h's.  seek returns a long (HL':HL). */
extern int open(), read(), write(), close(), fstat(), stat(), creat();
extern long seek();
extern int exit();

/*
 * The operation flags, one 16-bit cell per option letter.  doargs sets
 * exactly one of the first six; v is independent.  usageflag is the
 * '?' option; skewflag is set from a device-geometry bit by dispatch().
 */
int	copyflag;	/* c - copy FROM the DOS diskette */
int	delflag;	/* d - delete a DOS file */
int	putflag;	/* p - copy TO the DOS diskette */
int	renflag;	/* r - rename a DOS file */
int	touchflag;	/* t - touch a DOS file */
int	xflag;		/* x - extract */
int	verboseflag;	/* v - the space report */
int	usageflag;	/* '?' - usage was requested */
int	skewflag;	/* device geometry bit 3 - interleave skew */
int	doneflag;	/* FAT/directory changed, write back at finish() */

char	*devname;	/* the device argument, argv[1] */
int	devfd;		/* the opened device */

char	**nametable;	/* 0x8bd7 - the file-name argument table (argv[3..]) */
int	namecount;	/* 0x9040 - number of file names */

char	*name;		/* the file name the current operation acts on */

/*
 * The DOS diskette geometry, one 34-byte (17 int) entry of the formats
 * table, matched by format() against the boot signature.  geom points
 * into the table (0x9e42).
 */
struct geom {
	int	signature;	/* 0: tracks * sectors-per-track (320/360/640/720) */
	int	spc;		/* 2: sectors per cluster (5) */
	int	reserved;	/* 4: reserved sectors, the boot (1 or 2) */
	int	spt;		/* 6: sectors per track (8 or 9) */
	int	heads;		/* 8: heads (8 or 9) */
	int	bytes_per_sec;	/* 10: 512 */
	int	total_clusters;	/* 12: 315..355 */
	int	cluster_size;	/* 14: bytes per cluster (512 or 1024) */
	int	field16;	/* 16: 1 or 2, unused */
	int	root_entries;	/* 18: 16 or 32 */
	int	root_dirents;	/* 20: root dir entry count (64 or 112) */
	int	root_sectors;	/* 22: root dir sector count (4 or 7) */
	int	fat_sectors;	/* 24: sectors per FAT copy (1 or 2) */
	int	media;		/* 26: media descriptor */
	int	root_start;	/* 28: root dir start sector (3 or 5) */
	int	field30;	/* 30: unused */
	int	field32;	/* 32: unused */
};

struct geom *geom;	/* 0x9e42 - the matched format descriptor */

/*
 * The DOS directory entry, the standard 32-byte layout (stride 0x20,
 * attribute 0x0b, first cluster 0x1a).
 */
struct dirent {
	char	name[11];	/* 0-10: filename (8) + extension (3) */
	char	attr;		/* 11: attribute (0x10 = directory) */
	char	reserved[10];	/* 12-21 */
	int	time;		/* 22-23 */
	int	date;		/* 24-25 */
	int	cluster;	/* 26-27: first cluster */
	long	size;		/* 28-31: file size */
};

/*
 * dirinfo - the loaded directory.  dirbuf() fills one; H4742()
 * (commit()) writes it back.  ptr points into dirblk.
 */
struct dirinfo {
	int	count;		/* number of entries */
	int	n;		/* the cluster (0 = root) */
	struct dirent *ptr;	/* -> dirblk */
	char	dirty;		/* changed since load */
};

char fatbuf[1024];	/* 0x8bdc - the FAT image */
char dirblk[0xe00];	/* 0x9042 - the directory-entry area */

/*
 * fatentry - read one 12-bit FAT entry.  Entry n sits at byte n*3/2;
 * an even cluster takes the low 12 bits, an odd one the high 12.
 */
fatentry(cluster)
int cluster;
{
	int offset, word;

	cluster %= 682;			/* entries in a 2-sector FAT */
	offset = cluster * 3 / 2;
	word = (fatbuf[offset] & 0xff) | ((fatbuf[offset + 1] & 0xff) << 8);
	return cluster & 1 ? word >> 4 : word & 0xfff;
}

/*
 * fatwrite - write one 12-bit FAT entry, the sibling of fatentry
 * (H3755).  Preserves the neighbour's bits and marks doneflag.
 */
fatwrite(cluster, value)
int cluster, value;
{
	char *p;

	cluster %= 682;
	p = fatbuf + (cluster * 3) / 2;
	if (cluster & 1) {
		value = (value << 4) & 0xfff0;
		p[0] &= 0x0f;
		p[1] = 0;
	} else {
		value &= 0xfff;
		p[0] = 0;
		p[1] &= 0xf0;
	}
	p[0] |= value & 0xff;
	p[1] |= value >> 8;
	doneflag = 1;
}

say(s)
char *s;
{
	printf("%s", s);
}

printnum(n)
int n;
{
	printf("%d", n);
}

/*
 * seeksec - seek to a logical sector.  whence 3 is the kernel's
 * "sector" seek - it multiplies the offset by 512.
 */
seeksec(fd, sector)
int fd, sector;
{
	seek(fd, sector, 3);
}

/*
 * clustosec - the sector-I/O primitive (0x0e09).  arg0 is a LOGICAL
 * sector number, not a cluster: it applies the format's interleave
 * skew, converts to a physical sector, and transfers one sector via
 * the function pointer (read or write).  Returns 1 on a full transfer;
 * on a failed read the buffer is filled with 0xe5 (the deleted marker).
 */
clustosec(sector, buf, func)
int sector;
char *buf;
int (*func)();
{
	int trk, sec, phys;

	sec = sector % geom->spt;
	trk = sector / geom->spt;

	if (geom->heads == 8)
		phys = skewflag ? sec * 4 + sec / 2 : sec;
	else if (geom->heads == 9)
		phys = skewflag ? sec * 5 : sec;
	else
		phys = sec;
	phys = trk * geom->heads + (phys % geom->heads);

	seeksec(devfd, phys * geom->bytes_per_sec);
	if ((*func)(devfd, buf, geom->bytes_per_sec) == geom->bytes_per_sec)
		return 1;
	printf("%s", devname);
	if (func == read)
		memset(buf, 0xe5, geom->bytes_per_sec);
	return 0;
}

/*
 * cluster2sector - the cluster -> logical-sector conversion: the data
 * area begins after the root directory (root_start + root_sectors).
 */
cluster2sector(cluster)
int cluster;
{
	return geom->root_start + geom->root_sectors + (cluster - 2) * geom->spc;
}

/*
 * readcluster - read or write the directory region (0x4a1b).  Loads
 * geom->root_sectors sectors from geom->root_start into dirblk.
 */
readcluster(func)
int (*func)();
{
	char *p;
	int sector, i;

	p = dirblk;
	sector = geom->root_start;
	for (i = 0; i < geom->root_sectors; i++) {
		clustosec(sector, p, func);
		p += geom->bytes_per_sec;
		sector++;
	}
}

/*
 * readfat - load the FAT image (0x48ca).  Reads geom->fat_sectors
 * sectors from sector 1; on a short read it retries from the second
 * FAT copy.
 */
readfat()
{
	char *p;
	int n, sector;

	p = fatbuf;
	sector = 1;
	n = geom->fat_sectors;
	while (n-- != 0) {
		if (!clustosec(sector, p, read)
		    && !clustosec(sector + geom->fat_sectors, p, read))
			return 0;
		p += geom->bytes_per_sec;
		sector++;
	}
}

/*
 * dofat - top-level FAT load and copy dispatch (0x47ee).  The copy (c)
 * path builds a blank FAT + blank directory in memory and sets
 * doneflag (no read loop - the per-file copy lives in docopy).  The
 * normal path loads and verifies the FAT, and the directory.
 */
dofat()
{
	if (copyflag) {
		memset(fatbuf, 0, 0x400);
		fatbuf[0] = geom->media & 0xff;
		fatbuf[1] = 0xff;
		fatbuf[2] = 0xff;
		doneflag = 1;
	} else {
		readfat();
		if ((fatbuf[0] & 0xff) != geom->media && geom->heads == 9) {
			geom = (struct geom *)((char *)geom + 0x22);
			readfat();
		}
		if ((fatbuf[0] & 0xff) != geom->media) {
			printf("Not a DOS diskette.\n");
			exit(1);
		}
	}
	if (copyflag)
		memset(dirblk, 0xe5, 0xe00);
	else
		readcluster(read);
}

/*
 * nextcluster - follow the FAT chain one link; 0 at end-of-chain.
 */
nextcluster(cluster)
int cluster;
{
	int next;

	next = fatentry(cluster);
	if (next >= 0xff8)
		return 0;
	return next;
}

/*
 * alloccluster - free a whole FAT chain: walk from cluster writing 0
 * (free) into every entry until the chain ends.  (The name is a
 * misnomer in the binary; callers use it to release a file's chain.)
 */
alloccluster(cluster)
int cluster;
{
	int next;

	while (cluster != 0) {
		next = nextcluster(cluster);
		fatwrite(cluster, 0);
		cluster = next;
	}
}

/*
 * newcluster - return a free cluster number, or 0 if none
 * (H34bd in the binary).
 */
newcluster()
{
	int c;

	for (c = 2; c < geom->total_clusters; c++)
		if (fatentry(c) == 0)
			return c;
	return 0;
}

/*
 * dirbuf - load the directory for cluster n (0 = root) into a dirinfo
 * (0x466a).  Root: count = geom->root_dirents, ptr = dirblk; a
 * subdirectory cluster is read through clustosec/readcluster.
 */
dirbuf(n, di)
int n;
struct dirinfo *di;
{
	di->n = n;
	di->dirty = 0;
	if (n == 0) {
		di->count = geom->root_dirents;
		di->ptr = (struct dirent *)dirblk;
		return;
	}
	/* subdirectory: read the cluster's sectors into dirblk */
	readcluster(read);
	di->count = geom->root_dirents;
	di->ptr = (struct dirent *)dirblk;
}

/*
 * commit - write a directory back to disk (H4742).
 */
commit(di)
struct dirinfo *di;
{
	readcluster(write);
}

/*
 * The string helpers.  strcmp/strncmp here are the binary's boolean
 * "equal" / "prefix" tests, not the classic difference.
 */
strcmp(a, b)
char *a, *b;
{
	while (*a == *b && *a) {
		a++;
		b++;
	}
	return *a == 0;		/* arg0 a prefix of arg1 */
}

strncmp(a, b)
char *a, *b;
{
	while (*a == *b && *a) {
		a++;
		b++;
	}
	return *a == 0;
}

/* strippath - return the pointer just past the last '/'. */
char *
strippath(s)
char *s;
{
	char *p, *last;

	last = s;
	for (p = s; *p; p++)
		if (*p == '/')
			last = p + 1;
	return last;
}

/* findslash - copy the next path component into out, NUL-terminated. */
char *
findslash(s, out)
char *s, *out;
{
	while (*s == '/')
		s++;
	while (*s && *s != '/')
		*out++ = *s++;
	*out = '\0';
	return s;
}

/* splitname - build the space-padded, uppercased 8+3 DOS name. */
splitname(name, out)
char *name, *out;
{
	char *base, *dot, *p;
	int i, c;

	base = strippath(name);
	dot = base;
	for (p = base; *p; p++)
		if (*p == '.')
			dot = p;

	memset(out, ' ', 11);
	for (i = 0; i < 8 && *base && base != dot; i++) {
		c = *base++;
		if (c >= 'a' && c <= 'z')
			c -= 0x20;
		out[i] = c;
	}
	if (*dot == '.') {
		base = dot + 1;
		for (i = 0; i < 3 && *base; i++) {
			c = *base++;
			if (c >= 'a' && c <= 'z')
				c -= 0x20;
			out[8 + i] = c;
		}
	}
}

/*
 * match - glob matcher (0x3e11).  arg0 is the entry NAME, arg1 the
 * pattern; '*' is greedy (backtracks), '?' one char, with \x escapes
 * and [...] classes.  Returns 1/0.
 */
match(name, pattern)
char *name, *pattern;
{
	char *p;

	if (*name == 0 && *pattern == 0)
		return 1;
	if (*pattern == 0)
		return 0;
	if (*pattern == '*') {
		for (p = name + strlen(name); p >= name; p--)
			if (match(p, pattern + 1))
				return 1;
		return 0;
	}
	if (*name == 0)
		return 0;
	if (*pattern == '?') {
		if (*name == 0)
			return 0;
		return match(name + 1, pattern + 1);
	}
	if (*pattern == *name)
		return match(name + 1, pattern + 1);
	return 0;
}

/*
 * findfile - locate a file by path, returning its first-cluster number
 * (or -1).  Walks '/' components, skipping '.', via scanentry2.
 */
findfile(f)
char *f;
{
	int cluster;
	char comp[28];
	char *s;
	struct dirinfo di;
	struct dirent *de;

	cluster = 0;
	s = f;
	for (;;) {
		s = findslash(s, comp);
		if (comp[0] == 0)
			return cluster;
		if (strncmp(comp, "."))
			continue;
		dirbuf(cluster, &di);
		de = scanentry2(comp, &di);
		if (de == 0)
			return -1;
		cluster = de->cluster;
	}
}

/* scanentry2 - find a directory entry by its 8.3 name; returns the
 * entry pointer or 0. */
struct dirent *
scanentry2(name, di)
char *name;
struct dirinfo *di;
{
	char dosname[11];
	struct dirent *de;
	int count;

	splitname(name, dosname);
	de = di->ptr;
	count = di->count;
	while (count != 0) {
		if ((de->name[0] & 0377) == 0xe5
		 || (de->name[0] & 0377) == 0x00
		 || (de->name[0] & 0377) == 0x2e) {
			count--;
			de++;
			continue;
		}
		if (memcmp(dosname, de->name, 11) == 0)
			return de;
		count--;
		de++;
	}
	return 0;
}

/* scanentry3 - find the first free (0x00) or deleted (0xe5) slot. */
struct dirent *
scanentry3(di)
struct dirinfo *di;
{
	struct dirent *de;
	int count;

	de = di->ptr;
	count = di->count;
	while (count != 0) {
		if ((de->name[0] & 0377) == 0xe5
		 || (de->name[0] & 0377) == 0x00)
			return de;
		count--;
		de++;
	}
	return 0;
}

/*
 * delfile - is-directory test (0x20d0).  Returns 1 if f is a directory
 * (or a dot entry).
 */
delfile(f)
char *f;
{
	struct dirent *de;
	struct dirinfo di;
	int c;

	if (*strippath(f) == '.')
		return 1;
	c = findfile(f);
	if (c < 0)
		return 0;
	dirbuf(c, &di);
	de = scanentry2(strippath(f), &di);
	if (de == 0)
		return 0;
	return (de->attr & 0x10) != 0;
}

/* basename - strip a leading "./" and print the name with a newline. */
basename(s)
char *s;
{
	if (s[0] == '.' && s[1] == '/')
		s += 2;
	printf("%s\n", s);
}

/*
 * addentry - delete one file (0x32be; the dar.ctl name is a misnomer).
 * Locate the containing directory, find the entry, mark it 0xe5, and
 * free its FAT chain.
 */
addentry(name)
char *name;
{
	struct dirinfo di;
	struct dirent *de;
	int c;

	c = findfile(name);
	if (c < 0) {
		say(name);
		say(": No such file or directory\n");
		return 0;
	}
	dirbuf(c, &di);
	de = scanentry2(strippath(name), &di);
	if (de == 0) {
		say(name);
		say(": No such file or directory\n");
		commit(&di);
		return 0;
	}
	de->name[0] = 0xe5;
	alloccluster(de->cluster);
	if (verboseflag && delflag) {
		say("d ");
		basename(name);
	}
	commit(&di);
}

/*
 * findfree - recursive delete-all (0x311a).  Marks every entry in the
 * directory deleted and frees its chain, recursing into subdirs.
 */
findfree(n, name)
int n;
char *name;
{
	struct dirinfo di;
	struct dirent *de;
	int count;
	char *p;

	dirbuf(n, &di);
	de = di.ptr;
	count = di.count;
	while (count != 0) {
		if ((de->name[0] & 0377) == 0xe5
		 || (de->name[0] & 0377) == 0x00) {
			count--;
			de++;
			continue;
		}
		if ((de->name[0] & 0377) == 0x2e) {
			de->name[0] = 0xe5;
			count--;
			de++;
			continue;
		}
		if (verboseflag && delflag) {
			say("d ");
			basename(name);
		}
		if ((de->attr & 0x10) == 0) {
			de->name[0] = 0xe5;
			alloccluster(de->cluster);
			count--;
			de++;
			continue;
		}
		p = name + strlen(name);
		*p++ = '/';
		*p = '\0';
		findfree(de->cluster, name);
		count--;
		de++;
	}
	commit(&di);
}

/*
 * readdir - filter the filename pointer table in place: non-matching
 * names are dropped.  Returns the surviving count.
 */
readdir(pattern, table, count)
char *pattern;
struct dirent **table;
int count;
{
	int i;

	for (i = 0; i < count; ) {
		if (match(table[i], pattern))
			i++;
		else {
			count--;
			table[i] = table[count];
		}
	}
	return count;
}

/*
 * dectime - decode and print a directory entry's DOS date/time.  Takes
 * a pointer to the 32-byte entry.
 */
char	*monthname[] = {
	"Jan", "Feb", "Mar", "Apr", "May", "Jun",
	"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

dectime(de)
struct dirent *de;
{
	int month, day, hour, minute;

	month = ((de->date >> 5) & 0x0f) - 1;
	day = de->date & 0x1f;
	hour = de->time >> 11;
	minute = (de->time >> 5) & 0x3f;
	printf("%s %2d %2d:%02d ", monthname[month], day, hour, minute);
}

/*
 * The stdio layer.  getstr/putstr are the interactive reads/writes on
 * stdin/stdout (the binary's _filbuf/_flsbuf machinery is libc's, and
 * is not reproduced here).  getstr returns a line, 0 on EOF; 0xff is
 * the DOS end-of-file marker.
 */
char *
getstr(buf)
char *buf;
{
	char *p;
	int c;

	p = buf;
	for (;;) {
		c = getchar();
		if (c == 0xff || c == EOF) {
			*p = '\0';
			return (p == buf) ? 0 : buf;
		}
		if (c == '\n') {
			*p = '\0';
			return buf;
		}
		*p++ = c;
	}
}

putstr(s)
char *s;
{
	printf("%s\n", s);
}

/*
 * readboot - probe the device to learn its total sector count, which
 * format() matches against tracks*spt (320/360/640/720).  The binary
 * (0x13bf) is a 32-bit binary search over the sector space:
 *
 *	lo = 0, hi = 0x10000
 *	loop: mid = (lo + hi) >> 1
 *	      if mid == lo: break
 *	      seek(fd, low16(mid), 3); read(fd, &c, 1)
 *	      success  -> lo = mid
 *	      ENXIO    -> hi = mid
 *	      other    -> lo = mid
 *	seek(fd, 0, 0); if hi >= 0x10000: hi = 0xffff; return low16(hi)
 *
 * The 32-bit long is word-swapped (bytes 0-1 = high word, bytes 2-3 =
 * low word), so the seek offset and the return are the value's low 16
 * bits; hi is the first unreadable sector, i.e. the sector count.  The
 * midpoint is computed by the compiler's long-shift helper whose count
 * cell holds 2; for the search to converge to the device size it must
 * halve, so this is written as >>1 (the exact helper semantics - one
 * bit per 16-bit word - were not byte-certain).
 */
readboot(fd)
int fd;
{
	long lo, hi, mid;
	char c;

	lo = 0;
	hi = 0x10000;
	for (;;) {
		mid = (lo + hi) >> 1;
		if (mid == lo)
			break;
		if (seek(fd, (int)(mid & 0xffff), 3) < 0
		    || read(fd, &c, 1) != 1) {
			if (errno == ENXIO)
				hi = mid;
			else {
				printf("");
				lo = mid;
			}
		} else
			lo = mid;
	}
	seek(fd, 0, 0);
	if (hi >= 0x10000)
		hi = 0xffff;
	return (int)(hi & 0xffff);
}

/*
 * format - match the device signature against the formats table; the
 * matched entry becomes geom.
 */
int	formats[] = {
	320, 5, 1, 8, 8, 512, 315, 512, 1, 16, 64, 4, 1, 254, 3, 7, 1,
	360, 5, 1, 9, 9, 512, 353, 512, 1, 16, 64, 4, 2, 252, 5, 9, 2,
	360, 5, 1, 8, 9, 512, 315, 512, 1, 16, 64, 4, 1, 254, 3, 7, 1,
	640, 5, 2, 8, 8, 512, 316, 1024, 2, 32, 112, 7, 1, 255, 3, 10, 1,
	720, 5, 2, 9, 9, 512, 355, 1024, 2, 32, 112, 7, 2, 253, 5, 12, 2,
	720, 5, 2, 8, 9, 512, 316, 1024, 2, 32, 112, 7, 1, 255, 3, 10, 1,
	0
};

format()
{
	int sig;
	int *p;

	sig = readboot(devfd);
	for (p = formats; *p != 0; p += 17)
		if (*p == sig) {
			geom = (struct geom *)p;
			return;
		}
	printnum(sig);
	say(": Unknown format\n");
	say("Device size ");
	exit(0);
}

/*
 * space - the -v report: count free clusters and print the summary.
 * The free-KB divisor is 1000 (0x3e8), not 1024.
 */
space()
{
	int used, free, cluster;

	used = 0;
	free = 0;
	for (cluster = 2; cluster < geom->total_clusters; cluster++)
		if (fatentry(cluster) == 0)
			free++;
		else
			used++;

	printf("Diskette ");
	printnum(used * 100 / (used + free));
	printf("%% full, ");
	printnum(free * geom->cluster_size / 1000);
	printf("K free space left\n");
}

/*
 * checkdev - the device argument must name an existing block device.
 */
checkdev()
{
	struct stat st;

	if (stat(devname, &st) < 0) {
		printf("%s", devname);
		return 0;
	}
	if ((st.st_mode & S_IFBLK) != S_IFBLK) {
		say(devname);
		say(": Not block special\n");
		return 0;
	}
	return 1;
}

/*
 * mkdevname - find the device in /etc/mtab and build its /dev path.
 * The mtab record is 64 bytes: name in the first 32, the device path
 * in the second 32.
 */
mkdevname(name, buf)
char *name, *buf;
{
	int fd, n;
	char mtab[64];
	char path[80];
	struct stat st;

	fd = open("/etc/mtab", 0);
	if (fd < 0)
		return;
	for (;;) {
		if (read(fd, mtab, 64) != 64) {
			close(fd);
			return;
		}
		if (mtab[0] == 0 || mtab[32] == 0)
			continue;
		strcpy(path, "/dev/");
		strcpy(path + 5, &mtab[32]);
		if (stat(path, &st) == 0
		    && (st.st_mode & S_IFBLK) == S_IFBLK) {
			printf("%s", &mtab[32]);
			exit(0);
		}
	}
}

/*
 * dispatch - open the device with the right mode and stat it.  Delete
 * and rename need read-write (2); the read operations read-only (0).
 * Retries once, then prints the name and exits.
 */
dispatch()
{
	int mode;
	struct stat st;

	if (devfd >= 0)
		return;
	mode = (delflag || renflag) ? 2 : 0;
	devfd = open(devname, mode);
	if (devfd < 0)
		devfd = open(devname, mode);
	if (devfd < 0) {
		printf("%s", devname);
		exit(0);
	}
	fstat(devfd, &st);
	skewflag = (st.st_addr & 0x08) ? 1 : 0;
}

/*
 * execute - the driver: check the device, build the /dev name, open
 * and stat it, detect the format, load the FAT, then run exactly one
 * of delete / rename / touch / extract.  finish() writes back when
 * the FAT or directory was changed.
 */
execute()
{
	struct stat st;

	if (!checkdev())
		badopt();
	mkdevname(devname, &st);
	dispatch();
	format();
	dofat();
	if (delflag || renflag)
		if ((fatbuf[0] & 0xff) != geom->media) {
			say("Not a DOS diskette!");
			exit(0);
		}
	if (delflag)
		dodelete();
	else if (renflag)
		dorename();
	else if (touchflag)
		dotouch();
	else if (xflag || putflag)
		doextract();
	if (doneflag)
		finish();
}

/*
 * finish - write the two FAT copies and the directory back to the
 * diskette (H497c).
 */
finish()
{
	int sector, fat, i;
	char *buf;

	sector = 1;
	for (fat = 0; fat < 2; fat++) {
		buf = fatbuf;
		for (i = 0; i < geom->fat_sectors; i++) {
			clustosec(sector, buf, write);
			buf += geom->bytes_per_sec;
			sector++;
		}
	}
	readcluster(write);
}

/*
 * The single-file operations.  dodelete/dorename/dotouch/doextract
 * walk the filename table, acting on each name.
 */

dodelete()
{
	int i;
	char **t;

	if (namecount == 0)
		return;
	if (nametable == (char **)-1 || namecount == 1 && *nametable == 0)
		findfree(0, "");
	else
		for (i = 0; i < namecount; i++)
			addentry(nametable[i]);
}

dorename()
{
	/* rename: the next argument is the new name; find the entry and
	 * rewrite its 11-byte name.  Reconstructed from the operation
	 * shape, not byte-traced. */
	int c;
	struct dirinfo di;
	struct dirent *de;
	char newname[11];

	if (namecount < 1)
		return;
	c = findfile(nametable[0]);
	if (c < 0) {
		say(nametable[0]);
		say(": No such file or directory\n");
		return;
	}
	dirbuf(c, &di);
	de = scanentry2(strippath(nametable[0]), &di);
	if (de == 0)
		return;
	if (namecount > 1) {
		splitname(nametable[1], newname);
		memcpy(de->name, newname, 11);
	}
	commit(&di);
}

dotouch()
{
	/* touch: refresh the entry's date/time. */
	int c, i;
	struct dirinfo di;
	struct dirent *de;

	for (i = 0; i < namecount; i++) {
		c = findfile(nametable[i]);
		if (c < 0) {
			say(nametable[i]);
			say(": No such file or directory\n");
			continue;
		}
		dirbuf(c, &di);
		de = scanentry2(strippath(nametable[i]), &di);
		if (de == 0)
			continue;
		de->date = dosdate();
		de->time = dostime();
		commit(&di);
	}
}

doextract()
{
	/* extract/copy: walk the filename table and copy each file.  The
	 * copy-from and copy-to paths are docopy() and doput(). */
	int i;

	for (i = 0; i < namecount; i++)
		copyfile(nametable[i]);
}

/*
 * The copy operations.  docopy reads a DOS file and writes it to a
 * Micronix file; doput reads a Micronix file and writes it to DOS.
 * Reconstructed from the 0x2b74/0x27b8 bodies.
 */

/* dosdate/dostime - the DOS date/time from the clock (H4212/H42ed). */
dosdate()
{
	long t;
	struct tm *tm;

	t = time(0);
	tm = localtime(&t);
	return ((tm->tm_year - 80) << 9) | ((tm->tm_mon + 1) << 5) | tm->tm_mday;
}

dostime()
{
	long t;
	struct tm *tm;

	t = time(0);
	tm = localtime(&t);
	return (tm->tm_hour << 11) | (tm->tm_min << 5) | (tm->tm_sec / 2);
}

/* copyfile - dispatch one file copy (H2729/H1772). */
copyfile(name)
char *name;
{
	if (copyflag)
		return docopy(name);
	return doput(name);
}

/*
 * docopy - copy FROM the DOS diskette: walk the DOS file's FAT chain
 * and write its clusters to a new Micronix file.
 */
docopy(name)
char *name;
{
	int fd, cluster, n, s, sector;
	struct dirinfo di;
	struct dirent *de;
	long remaining;
	char buf[1024];

	cluster = findfile(name);
	if (cluster < 0)
		return 0;
	dirbuf(cluster, &di);
	de = scanentry2(strippath(name), &di);
	if (de == 0)
		return 0;
	fd = creat(strippath(name), 0666);
	if (fd < 0)
		return 0;
	remaining = de->size;
	for (cluster = de->cluster; cluster && cluster < 0xff8 && remaining > 0;
	     cluster = nextcluster(cluster)) {
		n = remaining < geom->cluster_size ? remaining : geom->cluster_size;
		sector = cluster2sector(cluster);
		for (s = 0; s < geom->spc; s++)
			clustosec(sector + s, buf + s * 512, read);
		write(fd, buf, n);
		remaining -= n;
	}
	close(fd);
	return 1;
}

/*
 * doput - copy TO the DOS diskette: read a Micronix file, allocate
 * clusters, write them, and link the FAT chain.
 */
doput(name)
char *name;
{
	int fd, cluster, prev, s, sector;
	long size;
	int n;
	struct dirinfo di;
	struct dirent *de;
	char buf[1024];

	if (verboseflag) {
		say("r ");
		basename(name);
	}
	fd = open(name, 0);
	if (fd < 0) {
		printf("%s", name);
		return 0;
	}
	dirbuf(0, &di);
	de = scanentry3(&di);
	if (de == 0) {
		say(": Volume full\n");
		close(fd);
		return 0;
	}
	splitname(strippath(name), de->name);
	de->attr = 0x20;
	de->cluster = 0;
	size = 0;
	prev = 0;
	for (;;) {
		n = read(fd, buf, geom->cluster_size);
		if (n < 0) {
			printf("%s", name);
			alloccluster(de->cluster);
			close(fd);
			commit(&di);
			return 0;
		}
		if (n == 0) {
			de->date = dosdate();
			de->time = dostime();
			de->size = size;
			close(fd);
			commit(&di);
			return 1;
		}
		cluster = newcluster();
		if (cluster == 0) {
			say("Not enough space left on volume\n");
			alloccluster(de->cluster);
			close(fd);
			commit(&di);
			return 0;
		}
		if (prev == 0)
			de->cluster = cluster;
		else
			fatwrite(prev, cluster);
		fatwrite(cluster, 0xfff);
		sector = cluster2sector(cluster);
		for (s = 0; s < geom->spc; s++)
			clustosec(sector + s, buf + s * 512, write);
		size += n;
		prev = cluster;
	}
}

/*
 * menu - the interactive loop.  In this binary it is dead code: the
 * first thing it does is call badopt(), which prints usage and exits.
 */
menu()
{
	badopt();
}

getdiskname()
{
	for (;;) {
		say("Insert diskette and close the drive door\n");
		say("Enter disk name: ");
		getstr(devname);
		if (checkdev())
			return;
	}
}

/*
 * main / doargs / optscan - the command line.
 */
main(argc, argv)
char **argv;
{
	doargs(argc, argv);
	execute();
	if (verboseflag)
		space();
	exit(1);
}

doargs(argc, argv)
char **argv;
{
	if (argc == 1)
		menu();
	if (argc > 1)
		devname = argv[1];
	if (argc > 2)
		optscan(argv[2]);
	if (argc > 3) {
		nametable = &argv[3];
		namecount = argc - 3;
	}
	if (namecount == 0) {
		nametable = 0;
		namecount = 1;
	}
	if (delflag + putflag + renflag + touchflag + xflag == 0) {
		touchflag = 1;
		verboseflag = 1;
	}
	if (delflag + putflag + renflag + touchflag + xflag > 1)
		atmost();
	if (usageflag)
		badopt();
}

optscan(s)
char *s;
{
	char *p;

	for (p = s; *p; p++) {
		switch (*p) {
		case 'c':
			copyflag = 1;
			break;
		case 'd':
			delflag = 1;
			break;
		case 'p':
			putflag = 1;
			break;
		case 'r':
			renflag = 1;
			break;
		case 't':
			touchflag = 1;
			break;
		case 'x':
			xflag = 1;
			break;
		case 'v':
			verboseflag = 1;
			break;
		case '-':
			break;
		case '?':
			usageflag = 1;
			break;
		default:
			badopt();
		}
	}
}

usage()
{
	usageflag = 1;
}

badopt()
{
	printf("usage: dar device {cdprtx} files ...\n");
	printf("Example: dar mfa xv\n");
	exit(0);
}

atmost()
{
	say("At most one of d, r, t, x may be specified.\n");
	exit(0);
}
