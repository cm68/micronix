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

#include <types.h>
#include <sys/fs.h>
#include <sys/dir.h>
#include <fslib.h>

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
 * checkilist1 - first pass over the I-list.  Counts each block the
 * inodes name, so the free list and the directory checks know what is
 * really used.
 */
int
checkilist1(void)
{
	printf("** Checking I-list, first pass\n");
	/* TODO: read the .dis from H289e */
	return 1;
}

/*
 * checkfree - walk the free list.  The original rebuilds it from
 * scratch if it finds a duplicate, an allocated block, or a bad block
 * in the chain.
 */
int
checkfree(void)
{
	/* TODO: read the .dis from H287f */
	return 1;
}

/*
 * checkdirs - walk the directories, fixing link counts, orphans, and
 * the "." and ".." links.
 */
int
checkdirs(void)
{
	/* TODO: read the .dis from H2fdc */
	return 1;
}

/*
 * checkilist2 - second pass over the I-list.  Reconciles the directory
 * link counts against the inode link counts.
 */
int
checkilist2(void)
{
	/* TODO: read the .dis from H2ab9 */
	return 1;
}

/*
 * summary - the final report: files, directories, blocks used and free.
 */
int
summary(void)
{
	printf("%u files, %u special, %u directories, %u small, %u large, %u huge\n",
	    nfiles, nspecial, ndir, nsmall, nlarge, nhuge);
	printf("%u used, %u free, %u bad\n", nused, nfree, nbad);
	return 1;
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
		/* TODO: read the .dis from H4672 */
	}
	return 1;
}
