/*
 * the portable half of the fs library - shared by the host and micronix
 *
 * lib/fslibfunc.c
 *
 * Inode and block management, directory walking, and the free lists.
 * Everything here reads and writes through readblk/writeblk, which the
 * caller defines: the host fslib drives an image file, the micronix one
 * drives /dev/m5a.  This is the mkfs worker/driver split again.
 */

#include <types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fs.h>
#include <sys/dir.h>
#include <fslib.h>

/* the inode cache: one block of the ilist, remembered between iget */
static struct dsknod inodeblk[I_PER_BLK];
static int inblkno = -1;

/* the indirect-block scratch, for bmap */
static UINT iblk[256];
static int iblkno = -1;
static UINT iiblk[256];
static int iiblkno = -1;

/* the directory-entry cache, for getdirent */
static struct dir dirbuf[32];
static int dblk = -1;

void
lose(char *s)
{
	printf("%s\n", s);
	exit(1);
}

struct dsknod *
iget(struct super *fs, int inum)
{
	int iblk;
	struct i_node *ip = malloc(sizeof(struct i_node));

	ip->inum = inum;
	ip->fs = fs;
	inum--;
	iblk = INODES_START + (inum / I_PER_BLK);
	if (inblkno != iblk) {
		inblkno = iblk;
		readblk(fs, inblkno, (char *)inodeblk);
	}
	memcpy(&ip->ondisk, &inodeblk[inum % I_PER_BLK], sizeof(struct dsknod));
	return &ip->ondisk;
}

void
iput(struct dsknod *dp)
{
	struct i_node *ip = (struct i_node *)dp;
	int inum = ip->inum;

	inum--;
	if (inblkno != INODES_START + (inum / I_PER_BLK)) {
		inblkno = INODES_START + (inum / I_PER_BLK);
		readblk(ip->fs, inblkno, (char *)inodeblk);
	}
	memcpy(&inodeblk[inum % I_PER_BLK], &ip->ondisk, sizeof(struct dsknod));
	writeblk(ip->fs, inblkno, (char *)inodeblk);
}

void
ifree(struct dsknod *dp)
{
	struct i_node *ip = (struct i_node *)dp;

	free(ip);
}

int
filesize(struct dsknod *ip)
{
	return (ip->d_size0 << 16) + ip->d_size1;
}

struct dir *
getdirent(struct dsknod *dp, int i)
{
	int b;
	struct i_node *ip = (struct i_node *)dp;

	if (i * sizeof(struct dir) > filesize(dp))
		return 0;
	b = bmap(dp, i * sizeof(struct dir), 0);
	if (dblk != b) {
		dblk = b;
		readblk(ip->fs, dblk, (char *)dirbuf);
	}
	return &dirbuf[i % 32];
}

/*
 * allocate a block from the free list
 */
int
balloc(struct super *fs)
{
	int b, i;
	UINT buf[256];

	i = --fs->s_nfree;
	if (i < 0 || i >= 100) {
		printf("bad freeblock\n");
		return 0;
	}
	b = fs->s_free[i];
	if (b == 0) {
		printf("no space\n");
		return 0;
	}
	if (fs->s_nfree <= 0) {
		readblk(fs, b, (char *)buf);
		fs->s_nfree = buf[0];
		for (i = 0; i < 100; i++)
			fs->s_free[i] = buf[i + 1];
		memset(buf, 0, sizeof(buf));
		writeblk(fs, b, (char *)buf);
	}
	fs->s_fmod = 1;
	return b;
}

void
bfree(struct super *fs, int blkno)
{
	int i;
	UINT buf[256];

	if (fs->s_nfree >= 100) {
		buf[0] = fs->s_nfree;
		for (i = 0; i < 100; i++)
			buf[i + 1] = fs->s_free[i];
		fs->s_nfree = 0;
		writeblk(fs, blkno, (char *)buf);
	}
	fs->s_free[fs->s_nfree++] = blkno;
	fs->s_fmod = 1;
}

/*
 * Map a file's byte offset to a block number, allocating along the way
 * if alloc is set.  The v6 scheme: seven single indirect blocks, one
 * double, and the inode holds eight addresses either way.
 */
int
bmap(struct dsknod *dp, int offset, int alloc)
{
	struct i_node *ip = (struct i_node *)dp;
	int lblk = offset / 512;
	UINT *aa;
	int iindex;
	int i;

	if (!(dp->d_mode & ILARG)) {
		if (lblk <= 7) {
			if ((dp->d_addr[lblk] == 0) && alloc)
				dp->d_addr[lblk] = balloc(ip->fs);
			return dp->d_addr[lblk];
		} else if (!alloc) {
			return 0;
		}
		/* convert to ILARG */
		iblkno = balloc(ip->fs);
		memset((char *)iblk, 0, sizeof(iblk));
		for (i = 0; i < 8; i++) {
			iblk[i] = dp->d_addr[i];
			dp->d_addr[i] = 0;
		}
		dp->d_addr[0] = iblkno;
		dp->d_mode |= ILARG;
		writeblk(ip->fs, iblkno, (char *)iblk);
	}

	iindex = lblk / 256;
	if (iindex >= 7) {
		if (dp->d_addr[7] == 0) {
			if (!alloc)
				return 0;
			iiblkno = dp->d_addr[7] = balloc(ip->fs);
			memset((char *)iiblk, 0, sizeof(iiblk));
			writeblk(ip->fs, iiblkno, (char *)iiblk);
		}
		if (iiblkno != dp->d_addr[7]) {
			iiblkno = dp->d_addr[7];
			readblk(ip->fs, iiblkno, (char *)iiblk);
		}
		aa = iiblk;
		iindex -= 7;
	} else {
		aa = dp->d_addr;
	}

	if (aa[iindex] == 0) {
		if (!alloc)
			return 0;
		aa[iindex] = balloc(ip->fs);
		if (aa == dp->d_addr)
			writeblk(ip->fs, iblkno, (char *)iblk);
		else
			writeblk(ip->fs, iiblkno, (char *)iiblk);
	}

	if (iblkno != aa[iindex]) {
		iblkno = aa[iindex];
		readblk(ip->fs, iblkno, (char *)iblk);
	}

	if ((iblk[lblk % 256] == 0) && alloc) {
		iblk[lblk % 256] = balloc(ip->fs);
		writeblk(ip->fs, iblkno, (char *)iblk);
	}
	return iblk[lblk % 256];
}

/*
 * free all 256 blocks in an indirect block, then the block itself
 */
void
iblkfree(struct super *fs, UINT *bp)
{
	int i;

	if (!*bp)
		return;
	readblk(fs, *bp, (char *)iblk);
	for (i = 0; i < 256; i++) {
		if (iblk[i]) {
			bfree(fs, iblk[i]);
			iblk[i] = 0;
		}
	}
	writeblk(fs, *bp, (char *)iblk);
	bfree(fs, *bp);
	*bp = 0;
}

/*
 * free all the blocks in a file
 */
void
filefree(struct dsknod *dp)
{
	struct i_node *ip = (struct i_node *)dp;
	int i;

	if (dp->d_mode & IIO)
		return;
	if (!(dp->d_mode & ILARG)) {
		for (i = 0; i < 8; i++) {
			if (dp->d_addr[i]) {
				bfree(ip->fs, dp->d_addr[i]);
				dp->d_addr[i] = 0;
			}
		}
		goto done;
	}
	iblkno = -1;
	iiblkno = -1;
	for (i = 0; i < 7; i++)
		iblkfree(ip->fs, &dp->d_addr[i]);
	if (dp->d_addr[7]) {
		readblk(ip->fs, dp->d_addr[7], (char *)iiblk);
		for (i = 0; i < 256; i++)
			iblkfree(ip->fs, &iiblk[i]);
		writeblk(ip->fs, dp->d_addr[7], (char *)iiblk);
	}
done:
	dp->d_mode &= ~ILARG;
	dp->d_size0 = 0;
	dp->d_size1 = 0;
	iput(dp);
}
