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

/*
 * Claim the boot blocks for an inode.
 *
 * installboot is the repair half of what mkfs's domkfs does when it
 * reserves the boot area: it builds the /boot directory and the
 * /boot/<name> file (bootmw on a hard disk, bootdj on a floppy) whose
 * indirect block lists blocks first through
 * first+nblk-1, so those blocks are owned by an inode and the free list
 * cannot hand them out.  It does NOT write the boot itself - the blocks
 * keep whatever a rom-bootable loader or dd put there - so it runs on a
 * floppy whose boot is already installed and which has only lost the
 * inode that protects it.
 *
 * The caller (icheck -i, fsck -i) rebuilds the free list afterwards; the
 * blocks this inode now owns come out of that rebuild allocated rather
 * than free.  The inode allocation below is a private copy (bootialloc)
 * rather than the host fslib's ialloc, because the two must not share a
 * name and this one works against the shared, portable interface.
 */

/*
 * allocate a free inode, as the kernel's ialloc does: pop the
 * superblock's free inode list, refilling it from the ilist when it is
 * empty.
 */
static int
bootialloc(struct super *fs)
{
    char buf[512];
    struct dsknod *dp;
    int iblk, n, inum;

    if (fs->s_ninode <= 0) {
        for (iblk = INODES_START; iblk < INODES_START + fs->s_isize;
            iblk++) {
            readblk(fs, iblk, buf);
            inum = (iblk - INODES_START) * I_PER_BLK + 1;
            for (n = I_PER_BLK, dp = (struct dsknod *)buf; n;
                n--, dp++, inum++)
                if (!(dp->d_mode & IALLOC) && fs->s_ninode < 100)
                    fs->s_inode[fs->s_ninode++] = inum;
            if (fs->s_ninode >= 100)
                break;
        }
    }
    if (fs->s_ninode <= 0)
        return 0;
    inum = fs->s_inode[--fs->s_ninode];
    {
        /* the free inode list may be stale on a disk we are repairing;
         * refuse to hand out an inode that is already allocated */
        struct dsknod *dp = iget(fs, inum);
        if (dp->d_mode & IALLOC) {
            ifree(dp);
            return 0;
        }
        ifree(dp);
    }
    fs->s_fmod = 1;
    return inum;
}

/*
 * Build the /boot directory and the boot file (named by `name`) that
 * own the boot blocks, exactly as mkfs builds them: a directory nothing can walk
 * into (mode 0) holding a file nothing can open (mode 0), one indirect
 * block naming first..first+nblk-1.  Returns 0 on success, -1 on
 * failure, and 1 when the boot was already installed.
 */
int
installboot(struct super *fs, int first, int nblk, char *name)
{
    struct dsknod *ip;
    struct dir *de;
    UINT ind[256];
    char buf[512];      /* the /boot directory block */
    char rbuf[512];     /* the root directory block */
    int indblk, dirblk;
    int dirino, filino;
    int b, entries, slot, i;

    if (first < 0 || nblk <= 0 || nblk > 256) {
        printf("installboot: %d boot blocks at %d is not representable\n",
            nblk, first);
        return -1;
    }

    /*
     * The boot can sit past the end of the filesystem: a floppy keeps it
     * in reserved tracks in front, and the roll maps those to the blocks
     * just past s_fsize, which the superblock therefore counts as absent.
     * Bring them in-band - grow the filesystem to take the boot, so the
     * blocks become real filesystem blocks and the free-list rebuild the
     * caller runs next sees them as allocated, not missing.
     */
    if (first + nblk > fs->s_fsize) {
        fs->s_fsize = first + nblk;
        fs->s_fmod = 1;
    }

    /* link /boot into the root directory, unless it is already there */
    ip = iget(fs, 1);
    if ((ip->d_mode & IFMT) != IFDIR) {
        ifree(ip);
        printf("installboot: inode 1 is not a directory\n");
        return -1;
    }
    b = bmap(ip, 0, 0);
    readblk(fs, b, rbuf);
    entries = filesize(ip) / sizeof(struct dir);
    slot = entries;
    for (i = 0; i < entries && i < 32; i++) {
        de = (struct dir *)rbuf + i;
        if (de->ino && strncmp(de->name, "boot", 14) == 0) {
            ifree(ip);
            printf("installboot: boot already installed\n");
            return 1;
        }
        if (de->ino == 0 && slot == entries)
            slot = i;
    }
    if (slot >= 32) {
        ifree(ip);
        printf("installboot: root directory is full\n");
        return -1;
    }

    /* the indirect block naming the boot blocks */
    indblk = balloc(fs);
    if (indblk == 0) {
        ifree(ip);
        return -1;
    }
    memset((char *)ind, 0, sizeof(ind));
    for (i = 0; i < nblk; i++)
        ind[i] = first + i;
    writeblk(fs, indblk, (char *)ind);

    /* two inodes: /boot and the boot file */
    dirino = bootialloc(fs);
    filino = bootialloc(fs);
    if (dirino == 0 || filino == 0) {
        ifree(ip);
        printf("installboot: out of inodes\n");
        return -1;
    }

    /* the /boot directory: ".", "..", and the boot file */
    dirblk = balloc(fs);
    if (dirblk == 0) {
        ifree(ip);
        return -1;
    }
    memset(buf, 0, sizeof(buf));
    de = (struct dir *)buf;
    de[0].ino = dirino;    strcpy(de[0].name, ".");
    de[1].ino = 1;         strcpy(de[1].name, "..");
    de[2].ino = filino;    strncpy(de[2].name, name, 14);
    writeblk(fs, dirblk, buf);

    /* the /boot directory inode, mode 0 */
    {
        struct dsknod *dip = iget(fs, dirino);
        memset((char *)dip, 0, sizeof(struct dsknod));
        dip->d_mode = IALLOC | IFDIR;
        dip->d_nlink = 2;
        dip->d_size1 = 3 * sizeof(struct dir);
        dip->d_addr[0] = dirblk;
        iput(dip);
        ifree(dip);
    }

    /* the boot file inode, owning the boot blocks */
    {
        struct dsknod *fip = iget(fs, filino);
        memset((char *)fip, 0, sizeof(struct dsknod));
        fip->d_mode = IALLOC | ILARG;
        fip->d_nlink = 1;
        fip->d_size0 = nblk >> 7;
        fip->d_size1 = (nblk & 0177) * 512;
        fip->d_addr[0] = indblk;
        iput(fip);
        ifree(fip);
    }

    /* the root entry: "boot" -> dirino */
    de = (struct dir *)rbuf + slot;
    de->ino = dirino;
    memset(de->name, 0, 14);
    strncpy(de->name, "boot", 14);
    writeblk(fs, b, rbuf);
    if (slot >= entries)
        ip->d_size1 += sizeof(struct dir);
    iput(ip);
    ifree(ip);

    /*
     * The inode and directory caches (inodeblk, dirbuf, iblk) may hold
     * copies of blocks this just rewrote through readblk/writeblk, since
     * installboot reads the root directory straight off the disk instead
     * of through getdirent.  Drop them so the caller's next iget and
     * getdirent read the disk, not a stale buffer.
     */
    inblkno = -1;
    dblk = -1;
    iblkno = -1;
    iiblkno = -1;

    printf("installboot: /boot/%.14s owns blocks %d..%d\n",
        name, first, first + nblk - 1);
    return 0;
}
