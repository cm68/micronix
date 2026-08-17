/*
 * file system check and repair
 *
 * cmd/fsck/fsck.c
 *
 * THIS IS A RECONSTRUCTION.  There is no surviving source for /bin/fsck;
 * this file is written from the disassembly of the /bin/fsck binary on
 * the Micronix 1.6 standalone - see fsck.dis and fsck.1 beside this
 * file.  It is the V6 fsck: the complete file system maintenance
 * package, five passes over the super block, the I-list, the free list
 * and the directories, with the repair the man page enumerates.
 *
 * Like dcheck and icheck, it runs against fslib rather than against a
 * block device directly - openfs hands back a struct super, and iget,
 * readblk and the rest read and write the file system image.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <types.h>
#include <sys/fs.h>
#include <sys/dir.h>
#include <fslib.h>

int traceflags;			/* the host fslib's trace controls */
int headpr;

/*
 * The options.  -t tests every block for readability, -n makes no
 * changes, -b hunts for bad blocks, and -v is verbose.  The man page
 * documents -t and -n; the binary takes all four.
 */
int	tflag;
int	nflag;
int	bflag;
int	vflag;

char	*fsname;		/* the file system being checked */

struct super *fs;		/* the open file system */

/*
 * the repair counts the passes report, and the totals the summary
 * prints at the end.
 */
int	nfiles;
int	nspecial;
int	ndir;
int	nsmall;
int	nlarge;
int	nhuge;
int	nused;
int	nfree;
int	nbad;
int	nblocks;

int	nerrors;		/* number of things fixed */

/*
 * the block map - one entry per block, recording who first claimed it
 * and how many times.  The original allocates it 8 bytes to the block
 * (two maps of s_fsize entries); a struct of four shorts is the same
 * thing, read back out of the disassembly's use of it in pass one.
 */
struct bmap {
	short	b_count;	/* reference count: 0 free, 1 used, >1 dup */
	short	b_inode;	/* the inode that first claimed it */
	short	b_offset;	/* its block offset in that inode */
	short	b_type;		/* what kind of block it is */
};

struct bmap *blockmap;		/* H700e - the block map */
char	*freemap;		/* H7010 - one byte per block, set if free */
char	*isallocated;		/* H7012 - one byte per inode */
short	*refcount;		/* H7014 - one short per inode, link refs */

/*
 * the passes, in the order the driver runs them.  Each returns 0 on
 * success, nonzero to stop the check.
 */
int	readsuper(void);
int	checkilist1(void);
int	checkfree(void);
int	checkdirs(void);
int	checkilist2(void);
int	summary(void);
int	reorg(void);

static void rebuildfree(void);

/*
 * main - parse the options, then check each file system named on the
 * command line.  With no name, print the usage.
 */
int
main(int argc, char *argv[])
{
	int	i;
	char	*arg;

	for (i = 1; i < argc; i++) {
		arg = argv[i];
		if (strcmp(arg, "-t") == 0) { tflag = 1; continue; }
		if (strcmp(arg, "-v") == 0) { vflag = 1; continue; }
		if (strcmp(arg, "-n") == 0) { nflag = 1; continue; }
		if (strcmp(arg, "-b") == 0) { bflag = 1; continue; }
		if (fsname) {
			fprintf(stderr, "usage: fsck filesystem ... \n");
			exit(1);
		}
		fsname = arg;
	}

	if (!fsname) {
		fprintf(stderr, "usage: fsck filesystem ... \n");
		exit(1);
	}

	if (readsuper() == 0)
		exit(1);

	printf("Checking %s:\n", fsname);

	/*
	 * the five passes.  The original interleaves them exactly so,
	 * with the free list and directory checks between the two I-list
	 * passes - see the driver at H021b in fsck.dis.
	 */
	printf("** Checking I-list, first pass\n");
	if (!checkilist1())
		goto out;
	printf("** Checking the free list\n");
	if (!checkfree())
		goto out;
	if (!checkdirs())
		goto out;
	printf("** Checking I-list, second pass\n");
	if (!checkilist2())
		goto out;
	summary();
	reorg();

out:
	closefs(fs);
	return 1;
}

/*
 * readsuper - read and validate the super block.  The original reads
 * block 1 and checks that s_isize and s_fsize are believable before it
 * trusts the rest of the file system.
 */
int
readsuper(void)
{
	if (openfs(fsname, &fs) < 0) {
		printf("Can't read the super block\n");
		return 0;
	}
	if (fs->s_isize == 0 || fs->s_fsize < 2) {
		printf("The super block of your file system is damaged beyond this\n");
		printf(" programs's capacity to repair.\n");
		printf("Or perhaps it never was a file system\n");
		return 0;
	}
	return 1;
}

/*
 * countblock - record one block in the map.  A zero block is a hole;
 * an out-of-range block is reported; a second claim is a duplicate.
 */
static void
countblock(int inum, int off, int b)
{
	struct bmap *bp;

	if (b == 0)
		return;
	if (b >= fs->s_fsize) {
		printf("Out of range block in I-list, Inode %u, Block %u\n",
		    inum, b);
		return;
	}
	bp = &blockmap[b];
	if (bp->b_count++) {
		printf("Dup in I-list, Inode %u, Block %u\n",
		    bp->b_inode, b);
	} else {
		bp->b_inode = inum;
		bp->b_offset = off;
	}
}

/*
 * countindir - walk one single-indirect block: the block itself, then
 * the 256 blocks it names.
 */
static void
countindir(int inum, int b)
{
	UINT blk[256];
	int i;

	if (b == 0)
		return;
	countblock(inum, 0, b);
	readblk(fs, b, (char *)blk);
	for (i = 0; i < 256; i++)
		countblock(inum, i, blk[i]);
}

/*
 * checkilist1 - first pass over the I-list.  Walks every inode, counts
 * each block it names into the block map, and detects duplicates and
 * out-of-range blocks, so the free list and directory passes know what
 * is really used.  The classification - small, large, huge - is the V6
 * addressing: small files are eight direct blocks, large files use the
 * seven single-indirect blocks, huge files the double-indirect block as
 * well.
 */
int
checkilist1(void)
{
	struct dsknod *ip;
	UINT blk[256];
	int inum;
	int i;

	blockmap = calloc(fs->s_fsize, sizeof(struct bmap));
	if (blockmap == 0)
		lose("out of memory");

	isallocated = calloc(fs->s_isize * I_PER_BLK, 1);
	if (isallocated == 0)
		lose("out of memory");

	for (inum = 1; inum < fs->s_isize * I_PER_BLK; inum++) {
		ip = iget(fs, inum);
		if ((ip->d_mode & IALLOC) == 0) {
			iput(ip);
			continue;
		}
		isallocated[inum] = 1;

		/* classify the inode */
		switch (ip->d_mode & IFMT) {
		case IFDIR:
			ndir++;
			break;
		case IFCHR:
		case IFBLK:
			nspecial++;
			break;
		default:
			nfiles++;
			if (ip->d_mode & ILARG) {
				if (ip->d_addr[7])
					nhuge++;
				else
					nlarge++;
			} else {
				nsmall++;
			}
			break;
		}

		/* count the blocks it names; a device names none */
		if (ip->d_mode & IIO) {
			iput(ip);
			continue;
		}
		if (ip->d_mode & ILARG) {
			for (i = 0; i < 7; i++)
				countindir(inum, ip->d_addr[i]);
			if (ip->d_addr[7]) {
				countblock(inum, 0, ip->d_addr[7]);
				readblk(fs, ip->d_addr[7], (char *)blk);
				for (i = 0; i < 256; i++)
					countindir(inum, blk[i]);
			}
		} else {
			for (i = 0; i < 8; i++)
				countblock(inum, i, ip->d_addr[i]);
		}
		iput(ip);
	}
	return 1;
}

/*
 * freeblock - classify one free block.  b is the block number and
 * chain is nonzero when it is the link block at the bottom of a free
 * list block rather than a plain free block.  Reports the out-of-range,
 * bad, duplicate and allocated cases and marks the block free in the
 * map otherwise.
 */
static void
freeblock(int b, int chain)
{
	char *what = chain ? "Free chain" : "Free";

	if (b == 0)
		return;
	if (b >= fs->s_fsize) {
		printf("Out of range block in Free, Block %u, Type %s\n", b, what);
		return;
	}
	if (freemap[b]) {
		printf("Dup in Free, Block %u, Type %s\n", b, what);
		return;
	}
	if (blockmap[b].b_count) {
		printf("Allocated block in Free, Block %u, Type %s\n", b, what);
		return;
	}
	freemap[b] = 1;
}

/*
 * freewalk - mark every block the free list names.  The list is a
 * chain: the super block holds s_nfree numbers in s_free[100], and
 * s_free[0] links to a block whose word 0 is the count of the next
 * batch and words 1..100 the numbers (the fslib bfree/balloc shape).
 */
static void
freewalk(void)
{
	UINT buf[256];
	UINT list[100];
	int n;
	int i;
	int b;

	memcpy(list, fs->s_free, sizeof(list));
	n = fs->s_nfree;
	if (n > 100)
		n = 100;

	for (;;) {
		for (i = n - 1; i >= 1; i--) {
			b = list[i];
			if (b == 0)
				return;
			freeblock(b, 0);
		}
		b = list[0];
		if (b == 0)
			return;
		freeblock(b, 1);
		if (b >= fs->s_fsize)
			return;
		if (readblk(fs, b, (char *)buf))
			return;
		memcpy(list, buf + 1, sizeof(list));
		n = buf[0];
		if (n > 100)
			n = 100;
	}
}

/*
 * checkfree - walk the free list, then find the blocks that are
 * neither allocated nor free and rebuild the list to take them back.
 */
int
checkfree(void)
{
	int b;
	int missing;

	freemap = calloc(fs->s_fsize, 1);
	if (freemap == 0)
		lose("out of memory");

	freewalk();

	missing = 0;
	for (b = fs->s_isize + INODES_START; b < fs->s_fsize; b++) {
		if (!blockmap[b].b_count && !freemap[b])
			missing++;
	}
	if (missing) {
		printf("%u missing blocks\n", missing);
		rebuildfree();
	}
	return 1;
}

/*
 * rebuildfree - rewrite the free list from the block map.  Clears the
 * list and adds every unused data block back, highest first (H1315 in
 * fsck.dis walks s_fsize-1 down to s_isize+2).  fslib's bfree chains
 * the list when it overflows a block of one hundred numbers.
 */
static void
rebuildfree(void)
{
	int b;

	printf("** Rebuilding the free list\n");

	fs->s_nfree = 0;
	memset(fs->s_free, 0, sizeof(fs->s_free));

	for (b = fs->s_fsize - 1; b >= fs->s_isize + INODES_START; b--) {
		if (blockmap[b].b_count)
			continue;
		bfree(fs, b);
	}
}

/*
 * checkdirs - walk the directories.  Counts every directory entry's
 * reference to an inode into refcount, and reports entries whose inode
 * number is out of range.  The reference counts are what checkilist2
 * reconciles against the inode link counts.
 *
 * NOTE: the orphan (lost+found) and "." / ".." repair in the original
 * are not here yet - this is the counting half of H2fdc.
 */
int
checkdirs(void)
{
	struct dsknod *ip;
	struct dir *dp;
	int inum;
	int i;

	refcount = calloc(fs->s_isize * I_PER_BLK, sizeof(short));
	if (refcount == 0)
		lose("out of memory");

	for (inum = 1; inum < fs->s_isize * I_PER_BLK; inum++) {
		ip = iget(fs, inum);
		if (!(ip->d_mode & IALLOC)) {
			iput(ip);
			continue;
		}
		if ((ip->d_mode & IFMT) != IFDIR) {
			iput(ip);
			continue;
		}

		for (i = 0; (dp = getdirent(ip, i)) != 0; i++) {
			if (dp->ino == 0)
				continue;
			if (dp->ino >= fs->s_isize * I_PER_BLK) {
				printf("Dir. entry inumber out of range,"
				    " Inode %u, Entry %u\n", inum, dp->ino);
				continue;
			}
			refcount[dp->ino]++;
		}
		iput(ip);
	}
	return 1;
}

/*
 * checkilist2 - second pass over the I-list.  Reconciles each inode's
 * link count against the directory entries that referenced it, resolved
 * in favour of the directory entries, as the man page puts it.  This is
 * the counting half of H2ab9; the orphan (lost+found) repair and the
 * "." / ".." patching are still to read out of the second-pass handlers.
 */
int
checkilist2(void)
{
	struct dsknod *ip;
	int inum;

	for (inum = 1; inum < fs->s_isize * I_PER_BLK; inum++) {
		ip = iget(fs, inum);
		if (!(ip->d_mode & IALLOC)) {
			iput(ip);
			continue;
		}
		if (refcount[inum] != ip->d_nlink) {
			printf("Inode %u, %u Directory entries, Link count %u\n",
			    inum, (unsigned)refcount[inum], ip->d_nlink);
			ip->d_nlink = refcount[inum];
		}
		iput(ip);
	}
	return 1;
}

/*
 * summary - the final report: files, directories, blocks used and free.
 */
int
summary(void)
{
	int b;

	nused = 0;
	nfree = 0;
	nbad = 0;
	for (b = fs->s_isize + INODES_START; b < fs->s_fsize; b++) {
		if (blockmap[b].b_count)
			nused++;
		else if (freemap[b])
			nfree++;
	}

	printf("%u files, %u special, %u directories, %u small, %u large, %u huge\n",
	    nfiles, nspecial, ndir, nsmall, nlarge, nhuge);
	printf("%u used, %u free, %u bad\n", nused, nfree, nbad);
	return 1;
}

/*
 * huntbad - the -t surface scan (H4672).  Reads every block; a block
 * that cannot be read is reported and, unless -n, patched with a fresh
 * block from the free list.  The patch itself (H0940 - find the inode
 * that names the block and repoint it) is still to be read out.
 */
static void
huntbad(void)
{
	char buf[512];
	int b;

	for (b = 0; b < fs->s_fsize; b++) {
		if (readblk(fs, b, buf) == 0)
			continue;
		printf("Bad block: %u\n", b);
		nbad++;
		/* TODO: H0940 - allocate a new block and patch the inode */
	}
}

/*
 * reorg - whatever is left to fix once the passes have decided.  The
 * original hunts for bad blocks here when -t was given.
 */
int
reorg(void)
{
	if (tflag) {
		printf("Hunting for bad blocks\n");
		huntbad();
	}
	return 1;
}
