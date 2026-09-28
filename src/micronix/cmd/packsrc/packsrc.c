/*
 * packsrc - pack a tree into tar files, right-sized to a volume
 *
 * cmd/packsrc/packsrc.c
 *
 *	packsrc <tree> <outdir> <budget-blocks>
 *
 * <tree> is a staged tree - the layout that lands under /usr/src, with
 * cmd/ lib/ sys/ and the makefile at its root.  <budget> is what one
 * tar may take, in 512 byte blocks, and is meant to be a volume's
 * usable size, so that a tar is something that goes on a disk more or
 * less whole.  <outdir> gets list.N and src.tar.N for each tar, N from
 * 1: the list is the members, relative to <tree>, and src.tar.N is the
 * tar of them.
 *
 * THE POLICY.  The unit is one directory's own files, and a leaf
 * directory - one with no subdirectories - is never split.  Units are
 * taken in sorted order and accumulated into one tar until the tar is
 * overfull, at which point the last unit is backed off, the tar is
 * closed without it, and it starts the next one.  One pass, no search:
 * the size of a candidate is COMPUTED, from the file sizes, rather
 * than found by building the tar and measuring it.  Backing off the
 * last unit is the whole of the search.
 *
 * A unit alone over budget is reported and goes on its own rather than
 * being split.  That is the policy being kept and not a failure: the
 * caller sized the budget, and a directory bigger than a volume is a
 * thing to hear about rather than to quietly cut in half.
 *
 * The tar is written here rather than by tar(1), because the size is
 * what decides everything and the size is exact: a 512 byte header and
 * the data rounded to 512, plus the two zero blocks that end an
 * archive.  So nothing is built to be measured, and there is no tar to
 * depend on.  The format is the classic V7 one that cmd/tar and mnix
 * both read, and the fields are spelled the way cmd/tar/tar.c spells
 * them, so an archive here and one the tree's own tar writes are the
 * same bytes for the same file - see tarhdr.  Member names carry no
 * directory entries and no leading "./", because mnix's tar x makes the
 * parents a member needs.
 *
 * Dual build, same as lz4.c: ccc for the guest (packsrc), the host cc
 * for bin/mxpacksrc, from this one source.  Two things differ and both
 * are behind #ifdef linux: the host has readdir and a 32 bit st_size,
 * micronix has neither - a micronix directory is a file of 16 byte
 * records, read here the way find reads it, and a file size is 24 bits
 * across st_size0 and st_size1.
 *
 * vim: tabstop=4 shiftwidth=4 noexpandtab:
 */

#include <types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#ifdef linux
#include <dirent.h>
#include <sys/stat.h>
#else
#include <sys/fs.h>
#include <sys/stat.h>
#include <sys/dir.h>
#endif

#define	PATHMAX	512
#define	TBLOCK	512
#define	DIRENT	16		/* sizeof(struct dir), both formats */

#ifdef linux
#define	FSIZE(sb)	((long)(sb).st_size)
#else
#define	FSIZE(sb)	((long)(sb).st_size1 + ((long)(sb).st_size0 << 16))
#endif

#define	roundup(n, b)	((((n) + (b) - 1) / (b)) * (b))

static void	die(char *what, char *arg);
static void	under(char *dst, char *rel);

/*
 * ---- the directory read, one entry at a time ----
 */

#ifdef linux

typedef DIR DIRW;

static int
diropen(DIRW **wp, char *path)
{
	*wp = opendir(path);
	return *wp != NULL;
}

static int
dirnext(DIRW *w, char *name)
{
	struct dirent *de;

	for (;;) {
		de = readdir(w);
		if (de == NULL)
			return 0;
		if (strcmp(de->d_name, ".") == 0 ||
		    strcmp(de->d_name, "..") == 0)
			continue;
		strcpy(name, de->d_name);
		return 1;
	}
}

static void
dirclose(DIRW *w)
{
	closedir(w);
}

#else

struct dirw {
	int fd;
	struct dir d;
};

typedef struct dirw DIRW;

static int
diropen(DIRW **wp, char *path)
{
	DIRW *w;
	int fd;

	fd = open(path, 0);
	if (fd < 0)
		return 0;
	w = (DIRW *)malloc(sizeof(DIRW));
	if (w == NULL) {
		close(fd);
		return 0;
	}
	w->fd = fd;
	*wp = w;
	return 1;
}

/*
 * The record is 16 bytes and the name in it is 14, not necessarily
 * ended: what is copied out is ended here.
 */
static int
dirnext(DIRW *w, char *name)
{
	int i;

	for (;;) {
		if (read(w->fd, (char *)&w->d, DIRENT) != DIRENT)
			return 0;
		if (w->d.d_ino == 0)
			continue;
		if (w->d.d_name[0] == '.' &&
		    (w->d.d_name[1] == '\0' ||
		     (w->d.d_name[1] == '.' && w->d.d_name[2] == '\0')))
			continue;
		for (i = 0; i < 14; i++)
			name[i] = w->d.d_name[i];
		name[14] = '\0';
		return 1;
	}
}

static void
dirclose(DIRW *w)
{
	close(w->fd);
	free((char *)w);
}

#endif

/*
 * ---- what a tar costs ----
 */

/*
 * Blocks a file of n data blocks occupies, which is bmap's arithmetic
 * and not n: eight addresses in the inode are direct, the ninth is one
 * indirect block, and past 1792 a second level is added one block per
 * 256.  A tar is a file like any other and this is what it costs the
 * volume the budget is counted in.
 */
static long
fcost(long n)
{
	if (n <= 8)
		return n;
	if (n <= 1792)
		return n + 1;
	return n + 2 + (n - 1792 + 255) / 256;
}

/*
 * ---- the tar ----
 */

/*
 * n octal digits, right justified, blanks to the left.  Written by hand
 * rather than with printf because the widths are the format's and not a
 * choice.
 */
static void
ofield(char *p, int n, long v)
{
	int i;

	for (i = n - 1; i >= 0; i--) {
		if (v != 0) {
			p[i] = (char)('0' + (int)(v & 7));
			v >>= 3;
		} else
			p[i] = ' ';
	}
}

/*
 * A V7 header.  Every field here is spelled the way cmd/tar/tar.c spells
 * it, so that the two writers produce the same bytes for the same file
 * and an archive can be compared against one tar(1) made without
 * allowing for anything but the owner.
 *
 * That means blanks and not zeroes, and a blank where a NUL would be
 * expected: tar.c fills the block with NULs and then sprints "%6o " at
 * each of these, so an eight byte mode comes out "   600 " with the
 * seventh byte still NUL behind it, and a twelve byte size comes out
 * "       4200 " with no NUL at all - the blank is what ends the field.
 * The checksum is the one field whose format has no trailing blank, so
 * it keeps the blank the summation put there and takes the NUL from the
 * sprint right after the six digits.
 *
 * The checksum itself is the unsigned sum of the 512 header bytes.  A
 * signed sum is what tar.c's "i += *cp" computes on a machine whose char
 * is signed, and it would differ - but only for a header holding a byte
 * over 0177, and a name, a mode and a size are all ASCII, so the two
 * agree on everything either produces.
 *
 * The owner is the one field that is deliberately not tar.c's.  tar.c
 * writes whoever the file belongs to here, which on a built tree is
 * whoever ran the build - uid 1750, say - and that uid means nothing on
 * the machine the disk is installed on.  A distribution's files are
 * root's, and root is what an install as the super-user would set them
 * to anyway, so the field is zero.  An archive therefore compares equal
 * to one tar(1) wrote except in uid, gid and the checksum over them.
 */
static void
tarhdr(int fd, char *name, long size, int mode, long mtime)
{
	static char h[TBLOCK];
	unsigned int sum;
	int i, n;

	memset(h, 0, TBLOCK);
	n = strlen(name);
	if (n > 100)
		n = 100;
	memcpy(h, name, n);
	ofield(h + 100, 6, mode & 07777);
	h[106] = ' ';
	ofield(h + 108, 6, 0);			/* uid */
	h[114] = ' ';
	ofield(h + 116, 6, 0);			/* gid */
	h[122] = ' ';
	ofield(h + 124, 11, size);
	h[135] = ' ';
	ofield(h + 136, 11, mtime);
	h[147] = ' ';
	memset(h + 148, ' ', 8);		/* chksum, as spaces */
	sum = 0;
	for (i = 0; i < TBLOCK; i++)
		sum += (unsigned char)h[i];
	ofield(h + 148, 6, (long)sum);
	h[154] = '\0';
	if (write(fd, h, TBLOCK) != TBLOCK)
		die("write error on", "the tar");
}

/*
 * n bytes of zeros.  n need not be a block: a member's data is rounded
 * out to one, which is a partial block whenever the size was not a
 * multiple of 512, and the two that end the archive are whole ones.
 */
static void
zeros(int fd, long n)
{
	static char z[TBLOCK];
	long i;
	int c;

	for (i = 0; i < n; i += c) {
		c = (int)(n - i > (long)TBLOCK ? (long)TBLOCK : n - i);
		if (write(fd, z, c) != c)
			die("write error on", "the tar");
	}
}

static long
tarbytes(long size)
{
	return TBLOCK + roundup(size, TBLOCK);
}

/*
 * One member: its header, its data, and the zeros that round the data
 * out to a block.  Returns what it added, so the caller can report a
 * size that is the file's and not a recount.
 */
static long
tarmember(int fd, char *path, long size, int mode, long mtime)
{
	static char buf[4096];
	char full[PATHMAX];
	int f, n;
	long left;

	tarhdr(fd, path, size, mode, mtime);
	under(full, path);
	f = open(full, 0);
	if (f < 0)
		die("cannot open", full);
	left = size;
	while (left > 0) {
		n = (int)(left > (long)sizeof buf ? (long)sizeof buf : left);
		n = read(f, buf, n);
		if (n <= 0)
			die("read error on", full);
		if (write(fd, buf, n) != n)
			die("write error on", path);
		left -= n;
	}
	close(f);
	n = (int)(size % TBLOCK);
	if (n != 0)
		zeros(fd, TBLOCK - n);
	return tarbytes(size);
}

/*
 * ---- messages ----
 */

static void
die(char *what, char *arg)
{
	fprintf(stderr, "packsrc: %s: %s\n", what, arg);
	exit(1);
}

static void
note(char *buf)
{
	printf("%s\n", buf);
}

/*
 * ---- the tree ----
 */

static char *tree;
static char *out;
static long budget;

static char **dirs;			/* every directory, sorted */
static int ndirs, adirs;

struct member {
	char *path;			/* relative to the tree */
	long size;
	int mode;
	long mtime;
};

static struct member *mem;		/* the tar being built */
static int nmem, amem;

static struct member *unit;		/* one directory's own files */
static int nun, aun;

static int ntar;
static long nblocks;

/*
 * The path of a relative name under the tree, and the relative name of
 * a leaf under a relative directory.  The root is "." and its files
 * are named bare: a member is "cmd/foo.c", never "./cmd/foo.c".
 */
static void
under(char *dst, char *rel)
{
	if (strcmp(rel, ".") == 0)
		strcpy(dst, tree);
	else
		sprintf(dst, "%s/%s", tree, rel);
}

static void
sub(char *dst, char *rel, char *name)
{
	if (strcmp(rel, ".") == 0)
		strcpy(dst, name);
	else
		sprintf(dst, "%s/%s", rel, name);
}

/*
 * realloc(NULL, n) is C's, but not every realloc here is that careful,
 * so the first allocation is a malloc.
 */
static char *
grown(char *p, int n)
{
	if (p == NULL)
		return (char *)malloc(n);
	return (char *)realloc(p, n);
}

static void
adddir(char *rel)
{
	if (ndirs >= adirs) {
		adirs = adirs ? adirs * 2 : 64;
		dirs = (char **)grown((char *)dirs, adirs * sizeof(char *));
		if (dirs == NULL)
			die("out of memory", rel);
	}
	dirs[ndirs] = (char *)malloc(strlen(rel) + 1);
	if (dirs[ndirs] == NULL)
		die("out of memory", rel);
	strcpy(dirs[ndirs], rel);
	ndirs++;
}

static void
addunit(char *rel, long size, int mode, long mtime)
{
	struct member *m;

	if (nun >= aun) {
		aun = aun ? aun * 2 : 64;
		unit = (struct member *)grown((char *)unit,
		    aun * sizeof(struct member));
		if (unit == NULL)
			die("out of memory", rel);
	}
	m = &unit[nun++];
	m->path = (char *)malloc(strlen(rel) + 1);
	if (m->path == NULL)
		die("out of memory", rel);
	strcpy(m->path, rel);
	m->size = size;
	m->mode = mode;
	m->mtime = mtime;
}

/*
 * Every directory in the tree, the root first as ".".  What a directory
 * holds is asked separately, so nothing is kept here but the names.
 */
static void
collectdirs(char *rel)
{
	DIRW *w;
	char name[256];
	char child[PATHMAX];
	char full[PATHMAX];
	struct stat sb;

	adddir(rel);
	under(full, rel);
	if (!diropen(&w, full))
		die("cannot open directory", full);
	while (dirnext(w, name)) {
		sub(child, rel, name);
		under(full, child);
		if (stat(full, &sb) < 0)
			continue;
		if ((sb.st_mode & S_IFMT) == S_IFDIR)
			collectdirs(child);
	}
	dirclose(w);
}

/*
 * One directory's own files - not its subdirectories, which are units
 * of their own.
 */
static void
unitof(char *rel)
{
	DIRW *w;
	char name[256];
	char child[PATHMAX];
	char full[PATHMAX];
	struct stat sb;

	nun = 0;
	under(full, rel);
	if (!diropen(&w, full))
		die("cannot open directory", full);
	while (dirnext(w, name)) {
		sub(child, rel, name);
		under(full, child);
		if (stat(full, &sb) < 0)
			continue;
		if ((sb.st_mode & S_IFMT) != S_IFREG)
			continue;
		addunit(child, FSIZE(sb), sb.st_mode & 07777, sb.st_mtime);
	}
	dirclose(w);
}

/*
 * ---- sorting, shell sort, so nothing is asked of the library that the
 * two systems might answer differently ----
 */

static void
sortstr(char **a, int n)
{
	int gap, i, j;
	char *t;

	for (gap = n / 2; gap > 0; gap /= 2)
		for (i = gap; i < n; i++)
			for (j = i - gap; j >= 0; j -= gap) {
				if (strcmp(a[j], a[j + gap]) <= 0)
					break;
				t = a[j];
				a[j] = a[j + gap];
				a[j + gap] = t;
			}
}

/*
 * ---- the tar being built ----
 */

static void
truncmem(int n)
{
	int i;

	for (i = n; i < nmem; i++) {
		free(mem[i].path);
		mem[i].path = NULL;
	}
	nmem = n;
}

/*
 * What the tar being built costs the volume: the members, their
 * headers, and the two zero blocks that end the archive.
 */
static long
blocks(void)
{
	long t;
	int i;

	t = 2 * (long)TBLOCK;
	for (i = 0; i < nmem; i++)
		t += tarbytes(mem[i].size);
	return fcost(t / TBLOCK);
}

static void
emit(void)
{
	char path[PATHMAX];
	char buf[256];
	FILE *lf;
	int fd, i;
	long blk;

	blk = blocks();
	ntar++;
	nblocks += blk;

	sprintf(path, "%s/list.%d", out, ntar);
	lf = fopen(path, "w");
	if (lf == NULL)
		die("cannot write", path);
	for (i = 0; i < nmem; i++)
		fprintf(lf, "%s\n", mem[i].path);
	fclose(lf);

	sprintf(path, "%s/src.tar.%d", out, ntar);
	fd = creat(path, 0666);
	if (fd < 0)
		die("cannot create", path);
	for (i = 0; i < nmem; i++)
		tarmember(fd, mem[i].path, mem[i].size, mem[i].mode,
		    mem[i].mtime);
	zeros(fd, 2 * (long)TBLOCK);
	close(fd);

	sprintf(buf, "src.tar.%d: %d files, %ld blocks of %ld (%ld%%)",
	    ntar, nmem, blk, budget, 100 * blk / budget);
	if (blk > budget)
		sprintf(buf + strlen(buf), " - OVER");
	note(buf);

	truncmem(0);
}

static void
appendunit(void)
{
	struct member *m;
	int i;

	for (i = 0; i < nun; i++) {
		if (nmem >= amem) {
			amem = amem ? amem * 2 : 256;
			mem = (struct member *)grown((char *)mem,
			    amem * sizeof(struct member));
			if (mem == NULL)
				die("out of memory", unit[i].path);
		}
		m = &mem[nmem++];
		m->path = (char *)malloc(strlen(unit[i].path) + 1);
		if (m->path == NULL)
			die("out of memory", unit[i].path);
		strcpy(m->path, unit[i].path);
		m->size = unit[i].size;
		m->mode = unit[i].mode;
		m->mtime = unit[i].mtime;
	}
}

/*
 * ---- the partition ----
 *
 * The other job.  What goes on the disks unpacked - the runtime, the man
 * pages, anything that is not a tar - has to be sorted into volumes too,
 * and a volume is a tighter budget than a tar's: it holds blocks AND
 * inodes, and directories are neither free nor negligible.  /usr/man/man1
 * has 224 names, a micronix directory entry is 16 bytes, and 32 entries
 * fill a block.
 *
 * This is dist/pack.awk written out in C, so that the tooling is the
 * tree's own.  The awk has four functions - fcost, nb, chainof and walk
 * - and this is those four with the same arithmetic, so that the two
 * place every file on the same disk.  The awk is 145 lines and this is
 * longer; what the length buys is that it needs no awk.  Micronix awk is
 * 2.11BSD awk, which has no user-defined functions, no ternary and no
 * sub(), so pack.awk cannot run on micronix at all - and the shell
 * cannot drive it either, having no if and no while.
 *
 * The unit is one directory's own files, the unit the tar mode uses.  A
 * unit goes on a disk whole; when it will not fit even on an empty disk
 * it goes a file at a time instead, because on the five inch media most
 * of them will not - /bin is 4200 blocks against 2151 - and a directory
 * that cannot be placed whole still has to be placed.
 *
 * A disk number below zero is a disk with nothing on it.  Pricing a unit
 * asks whether it would fit alone, which must not disturb the disk being
 * filled, so the question is asked of a disk that does not exist: the
 * commit flag is clear, so nothing is written, and the lookups below
 * answer "not there" and "no entries" without a place to put them.
 */

struct pfile {				/* one file of a unit */
	char	*path;
	int	cost;			/* fcost of its blocks */
};

struct punit {				/* one directory's own files */
	char	*name;			/* "." for a file at the root */
	long	blk;			/* the unit in data blocks */
	struct pfile *f;
	int	nf, af;
};

struct pent {				/* a directory on one disk */
	char	*path;
	int	ent;			/* entries, "." and ".." among them */
};

struct pdisk {
	long	blk;
	int	ino;
	struct pent *e;
	int	ne, ae;
};

static struct punit *unit_tab;
static int nunit, aunit;

static struct pdisk *disk_tab;
static int adisk;

static int newdirs;			/* what the last walk would create */

static char *
strsave(char *s)
{
	char *p;

	p = (char *)malloc(strlen(s) + 1);
	if (p == NULL)
		die("out of memory", s);
	strcpy(p, s);
	return (p);
}

static int
pfind(char *name)
{
	int i;

	for (i = 0; i < nunit; i++)
		if (strcmp(unit_tab[i].name, name) == 0)
			return i;
	return -1;
}

static int
padd(char *name)
{
	struct punit *u;

	if (nunit >= aunit) {
		aunit = aunit ? aunit * 2 : 64;
		unit_tab = (struct punit *)grown((char *)unit_tab,
		    aunit * sizeof(struct punit));
		if (unit_tab == NULL)
			die("out of memory", name);
	}
	u = &unit_tab[nunit];
	u->name = strsave(name);
	u->blk = 0;
	u->f = NULL;
	u->nf = 0;
	u->af = 0;
	return nunit++;
}

/*
 * One line of the manifest: a size, whitespace, and the path the file
 * lands at, relative to the image's root.  The path is the rest of the
 * line and not the second field, so that a name with a blank in it is
 * one file rather than two.
 */
static void
addfile(char *path, long size)
{
	char d[PATHMAX];
	char *s;
	struct punit *u;
	struct pfile *f;
	int i, cost;

	strcpy(d, path);
	s = strrchr(d, '/');
	if (s == NULL)
		strcpy(d, ".");
	else
		*s = '\0';

	i = pfind(d);
	if (i < 0)
		i = padd(d);
	u = &unit_tab[i];

	cost = (int)fcost((size + TBLOCK - 1) / TBLOCK);
	u->blk += cost;
	if (u->nf >= u->af) {
		u->af = u->af ? u->af * 2 : 16;
		u->f = (struct pfile *)grown((char *)u->f,
		    u->af * sizeof(struct pfile));
		if (u->f == NULL)
			die("out of memory", path);
	}
	f = &u->f[u->nf++];
	f->path = strsave(path);
	f->cost = cost;
}

static struct pdisk *
pdisk(int k)
{
	int old;

	if (k < 0)
		die("internal: a disk below zero", "the partition");
	if (k >= adisk) {
		old = adisk;
		while (k >= adisk)
			adisk = adisk ? adisk * 2 : 8;
		disk_tab = (struct pdisk *)grown((char *)disk_tab,
		    adisk * sizeof(struct pdisk));
		if (disk_tab == NULL)
			die("out of memory", "the disk table");
		memset(&disk_tab[old], 0, (adisk - old) * sizeof(struct pdisk));
	}
	return &disk_tab[k];
}

/*
 * Blocks a directory occupies at e entries.  Two of them are '.' and
 * '..'.  The root block is one of the two the superblock does not use,
 * so it is already paid for and never charged.
 */
static int
nb(char *d, int e)
{
	int c;

	c = (e + 2 + 31) / 32;
	if (d[0] == '.' && d[1] == '\0')
		c--;
	return c < 0 ? 0 : c;
}

#define	CHAINMAX	64
static char *chain[CHAINMAX];
static int nchain;

/*
 * The chain from d up to the root, d first and the root-most ancestor
 * last.  "." is not on it: the climb stops there.
 *
 * The names are kept on the heap rather than as one static array of
 * strings because that array would be PATHMAX squared - 32K, which the
 * assembler cannot emit as a single .ds.  The strings from the previous
 * call are freed at the top of this one, so a walk that runs once per
 * file of a manifest does not grow the heap by its depth each time.
 */
static int
chainof(char *d)
{
	int n;
	char buf[PATHMAX];
	char *s;

	while (nchain > 0)
		free(chain[--nchain]);

	n = 0;
	strcpy(buf, d);
	while (strcmp(buf, ".") != 0) {
		if (n >= CHAINMAX)
			die("directory nesting too deep", d);
		chain[n++] = strsave(buf);
		s = strrchr(buf, '/');
		if (s == NULL || s == buf)
			strcpy(buf, ".");
		else
			*s = '\0';
	}
	nchain = n;
	return n;
}

static int
dentfind(int k, char *path)
{
	int i;

	if (k < 0)
		return -1;
	for (i = 0; i < pdisk(k)->ne; i++)
		if (strcmp(pdisk(k)->e[i].path, path) == 0)
			return i;
	return -1;
}

static void
dadd(int k, char *path)
{
	struct pdisk *d;
	struct pent *e;

	d = pdisk(k);
	if (d->ne >= d->ae) {
		d->ae = d->ae ? d->ae * 2 : 32;
		d->e = (struct pent *)grown((char *)d->e,
		    d->ae * sizeof(struct pent));
		if (d->e == NULL)
			die("out of memory", path);
	}
	e = &d->e[d->ne++];
	e->path = strsave(path);
	e->ent = 0;
}

static int
dent(int k, char *path)
{
	int i;

	i = dentfind(k, path);
	return i < 0 ? 0 : pdisk(k)->e[i].ent;
}

static void
dentset(int k, char *path, int e)
{
	int i;

	i = dentfind(k, path);
	if (i < 0) {
		dadd(k, path);
		i = pdisk(k)->ne - 1;
	}
	pdisk(k)->e[i].ent = e;
}

/*
 * Price, and with docommit apply, putting f more entries in the
 * directory d on disk k.  Every directory between d and the root has to
 * exist for it to hold anything, so those are created and charged for
 * first, root-most first: a directory made a moment ago is already at
 * zero entries when its child is charged to it, and the arithmetic comes
 * out once.  newdirs is what this call would create, counted whether or
 * not docommit is set - which is why the caller reads it after pricing
 * the empty disk and again after pricing the file itself.
 */
static int
walk(int k, char *path, int f, int docommit)
{
	int n, i, delta, t;

	n = chainof(path);
	delta = 0;
	newdirs = 0;
	for (i = n - 1; i >= 0; i--) {
		char *a, *p;

		a = chain[i];
		p = (i == n - 1) ? "." : chain[i + 1];
		if (dentfind(k, a) >= 0)
			continue;
		delta += nb(a, 0);
		if (docommit) {
			dadd(k, a);
			pdisk(k)->ino++;
		}
		newdirs++;
		/*
		 * One more entry in the parent, so the parent grows by a
		 * block exactly when the count crosses a multiple of 32.
		 */
		t = dent(k, p);
		delta += (t + 34) / 32 - (t + 33) / 32;
		if (docommit)
			dentset(k, p, t + 1);
	}
	t = dent(k, path);
	delta += nb(path, t + f) - nb(path, t);
	if (docommit)
		dentset(k, path, t + f);
	return delta;
}

/*
 * One path to the disk's list file.  Only the list being written is
 * open: the disk number never decreases, so each list is written
 * through in one run, and holding all of them open at once runs the
 * library out of FILE slots - _NFILE is about a dozen, and a five inch
 * set is twenty five disks.  That is why the file is opened "w" rather
 * than "a": it is opened once, on the first path that belongs to it.
 */
static int plistopen = -1;
static FILE *plist;

static void
plistout(int k, char *path)
{
	char buf[PATHMAX];

	if (k != plistopen) {
		if (k < plistopen)
			die("internal: disks out of order, list", "the partition");
		if (plist != NULL)
			fclose(plist);
		sprintf(buf, "%s/list.%d", out, k);
		plist = fopen(buf, "w");
		if (plist == NULL)
			die("cannot write", buf);
		plistopen = k;
	}
	fprintf(plist, "%s\n", path);
}

static void
readmanifest(char *manifest)
{
	char line[PATHMAX + 64];
	FILE *mf;
	char *p, *nl;
	long size;

	mf = fopen(manifest, "r");
	if (mf == NULL)
		die("cannot read", manifest);
	while (fgets(line, sizeof(line), mf) != NULL) {
		size = atol(line);
		p = line;
		while (*p && *p != ' ' && *p != '\t')
			p++;			/* the size field */
		while (*p == ' ' || *p == '\t')
			p++;
		if (*p == '\n' || *p == '\0')
			continue;		/* no path on this line */
		nl = p + strlen(p);
		while (nl > p && (nl[-1] == '\n' || nl[-1] == '\r'))
			*--nl = '\0';
		addfile(p, size);
	}
	fclose(mf);
}

/*
 * The totals are kept in a local before being written back, and the disk
 * is fetched once into a pointer, so that the only arithmetic c1 sees is
 * long + long on two plain variables.  The same expression written with
 * pdisk(k)->blk on each side of the comparison is miscompiled: blk is a
 * long past a function call that returns a struct pointer, and the
 * compound assignment to it does not stick - the disk's block count
 * stays zero, the block budget never overflows, and every unit lands on
 * disk 1.  The inode count, which is an int, is unaffected, so the
 * symptom is a partition that splits on inodes alone.
 */
static void
place(long inos)
{
	struct pdisk *d;
	long delta, db, ec;
	int disk, i, j, nd;

	if (nunit == 0)
		die("nothing to place in", "the manifest");

	disk = 1;
	for (i = 0; i < nunit; i++) {
		struct punit *u = &unit_tab[i];

		ec = u->blk + (long)walk(-1, u->name, u->nf, 0);
		if (ec > budget || newdirs + u->nf > inos) {
			/*
			 * The unit will not fit on a disk of its own, so
			 * it goes a file at a time.
			 */
			for (j = 0; j < u->nf; j++) {
				d = pdisk(disk);
				db = d->blk;
				delta = (long)u->f[j].cost +
				    (long)walk(disk, u->name, 1, 0);
				if ((db + delta > budget ||
				     d->ino + newdirs + 1 > inos) && db > 0) {
					disk++;
					d = pdisk(disk);
					db = d->blk;
					delta = (long)u->f[j].cost +
					    (long)walk(disk, u->name, 1, 0);
				}
				nd = newdirs;
				walk(disk, u->name, 1, 1);
				d->blk = db + delta;
				d->ino += nd + 1;
				plistout(disk, u->f[j].path);
			}
			continue;
		}
		d = pdisk(disk);
		db = d->blk;
		delta = u->blk + (long)walk(disk, u->name, u->nf, 0);
		if (db + delta > budget ||
		    d->ino + newdirs + u->nf > inos) {
			disk++;
			d = pdisk(disk);
			db = d->blk;
			delta = u->blk + (long)walk(disk, u->name, u->nf, 0);
		}
		nd = newdirs;
		walk(disk, u->name, u->nf, 1);
		d->blk = db + delta;
		d->ino += nd + u->nf;
		for (j = 0; j < u->nf; j++)
			plistout(disk, u->f[j].path);
	}

	if (plist != NULL) {
		fclose(plist);
		plist = NULL;
		plistopen = -1;
	}
	for (i = 1; i <= disk; i++) {
		struct pdisk *d = pdisk(i);

		/*
		 * Both percentages are taken in long.  A micronix int is
		 * 16 bits, so 100 * ino overflows past 327 inodes and the
		 * disk reads as -1 per cent; blk escapes it only because
		 * it is a long to begin with.
		 */
		printf("disk %d: %5ld blocks (%ld%%), %5ld inodes (%ld%%)\n",
		    i, d->blk, (long)100 * d->blk / budget,
		    (long)d->ino, (long)100 * d->ino / inos);
	}
	printf("TOTAL %d disks\n", disk);
}

static void
mkdirp(char *path)
{
	char buf[PATHMAX];
	char *p;

	if (path[0] == '\0')
		return;
	strcpy(buf, path);
	for (p = buf + 1; *p; p++) {
		if (*p != '/')
			continue;
		*p = '\0';
		mkdir(buf, 0777);
		*p = '/';
	}
	mkdir(buf, 0777);
}

/*
 * tar <tree> <outdir> <budget> - pack a source tree into tars that fit a
 * volume, one tar per volume, backing off a directory at a time.
 */
static void
do_tar(int argc, char **argv)
{
	char buf[256];
	int i;

	if (argc != 5) {
		fprintf(stderr,
		    "usage: packsrc tar <tree> <outdir> <budget-blocks>\n");
		exit(1);
	}
	tree = argv[2];
	out = argv[3];
	budget = atol(argv[4]);
	if (budget <= 0)
		die("bad budget", argv[4]);

	mkdirp(out);

	collectdirs(".");
	sortstr(dirs, ndirs);

	for (i = 0; i < ndirs; i++) {
		int save;

		unitof(dirs[i]);
		if (nun == 0)
			continue;

		/*
		 * The unit joins the tar being built; if that puts it
		 * over, the unit is backed off, what was there is closed
		 * without it, and the unit starts the next one.  An
		 * empty tar is never closed, so a unit too big for a
		 * disk of its own is not backed off into a tar of its
		 * own that is also too big - it stays, and is reported.
		 */
		save = nmem;
		appendunit();
		if (blocks() > budget && save > 0) {
			truncmem(save);
			emit();
			appendunit();
		}
	}
	if (nmem > 0)
		emit();

	if (ntar == 0) {
		fprintf(stderr, "packsrc: nothing to pack in\n");
		exit(1);
	}
	sprintf(buf, "%d tar files in %s, %ld blocks", ntar, out, nblocks);
	note(buf);
}

/*
 * part <manifest> <outdir> <budget-blocks> <budget-inodes> - sort the
 * files of a manifest into volumes, writing list.1, list.2 ... into the
 * outdir, one path per line.
 */
static void
do_part(int argc, char **argv)
{
	long inos;

	if (argc != 6) {
		fprintf(stderr,
		    "usage: packsrc part <manifest> <outdir> <blocks> <inodes>\n");
		exit(1);
	}
	out = argv[3];
	budget = atol(argv[4]);
	inos = atol(argv[5]);
	if (budget <= 0)
		die("bad block budget", argv[4]);
	if (inos <= 0)
		die("bad inode budget", argv[5]);

	mkdirp(out);
	readmanifest(argv[2]);
	place(inos);
}

int
main(argc, argv)
	int argc;
	char **argv;
{
	if (argc < 2) {
		fprintf(stderr,
		    "usage: packsrc tar <tree> <outdir> <budget-blocks>\n");
		fprintf(stderr,
		    "       packsrc part <manifest> <outdir> <blocks> <inodes>\n");
		exit(1);
	}
	if (strcmp(argv[1], "tar") == 0)
		do_tar(argc, argv);
	else if (strcmp(argv[1], "part") == 0)
		do_part(argc, argv);
	else {
		fprintf(stderr, "packsrc: unknown mode %s\n", argv[1]);
		exit(1);
	}
	return 0;
}
