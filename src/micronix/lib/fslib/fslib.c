/*
 * micronix filesystem access library - the native driver half
 *
 * lib/fslib.c
 *
 * Just the block device and the superblock.  The inode, block and
 * directory logic lives in fslibfunc.c, shared with the host build;
 * this only defines readblk/writeblk and open/close, which is the whole
 * difference between the two halves.  The kernel's block driver does
 * the rotation, so a block is a seek and a read.
 */

#include <types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/fs.h>
#include <fslib.h>

#define BSIZE	512

/*
 * The filesystem handle, passed around as a struct super *.  The
 * superblock is the first member so its fields line up, exactly as the
 * host fslib's struct image does it.
 */
struct uxfs {
	struct super sb;
	int fd;
};

int
readblk(struct super *fs, int blkno, char *buf)
{
	struct uxfs *u = (struct uxfs *)fs;

	if (blkno == 0) {
		memset(buf, 0, BSIZE);
		return 0;
	}
	if (lseek(u->fd, blkno * (long) BSIZE, 0) < 0)
		lose("read seek");
	if (read(u->fd, buf, BSIZE) != BSIZE)
		lose("read");
	return 0;
}

int
writeblk(struct super *fs, int blkno, char *buf)
{
	struct uxfs *u = (struct uxfs *)fs;

	if (blkno == 0) {
		lose("write block 0");
		return 0;
	}
	if (lseek(u->fd, blkno * (long) BSIZE, 0) < 0)
		lose("write seek");
	if (write(u->fd, buf, BSIZE) != BSIZE)
		lose("write");
	return 0;
}

int
openfsrw(char *name, struct super **fsp, int writable)
{
	struct uxfs *u = malloc(sizeof(struct uxfs));

	u->fd = open(name, writable ? 2 : 0);
	if (u->fd < 0) {
		free(u);
		return -1;
	}
	readblk((struct super *)u, 1, (char *)&u->sb);
	*fsp = (struct super *)u;
	return 0;
}

int
openfs(char *name, struct super **fsp)
{
	return openfsrw(name, fsp, 0);
}

void
closefs(struct super *fs)
{
	struct uxfs *u = (struct uxfs *)fs;

	if (fs->s_fmod)
		writeblk(fs, 1, (char *)&u->sb);
	close(u->fd);
	free(u);
}
