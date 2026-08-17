/*
 * interface to the fs library
 *
 * include/fslib.h
 *
 * This is the portable half of the interface: the inode, block and
 * directory functions, shared with the host fslib.  The driver half -
 * readblk, writeblk, openfs, closefs - is what differs between the host
 * (an image file) and micronix (a block device).
 */

#define	INODES_START	2
#define	I_PER_BLK	16

/*
 * an in-memory inode: the on-disk inode wrapped with its inumber and
 * the filesystem it belongs to.  iget returns a pointer to the ondisk
 * member, and the wrapper is reached back through it.
 */
struct i_node {
	struct dsknod ondisk;
	struct super *fs;
	int inum;
};

extern int openfs(char *name, struct super **f);
extern int openfsrw(char *name, struct super **f, int writable);
extern void closefs(struct super *f);

extern void lose(char *s);
extern int readblk(struct super *f, int blkno, char *buf);
extern int writeblk(struct super *f, int blkno, char *buf);

extern struct dsknod *iget(struct super *f, int inum);
extern void iput(struct dsknod *ip);
extern void ifree(struct dsknod *ip);

extern int filesize(struct dsknod *ip);
extern int bmap(struct dsknod *ip, int offset, int alloc);
extern int balloc(struct super *f);
extern void bfree(struct super *f, int b);
extern void iblkfree(struct super *f, UINT *bp);
extern void filefree(struct dsknod *ip);

extern struct dir *getdirent(struct dsknod *ip, int index);
