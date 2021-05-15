#define	INODES_START	2
#define	I_PER_BLK	16
#ifdef notdef
#define	NOPEN	16
#define	NSIG	16
#endif

/*
 * this is here because the emulation and host utilities need to access
 * both the host stat structure and the micronix stat structure,
 * and there are namespace collisions.
 */
struct statb {
	UINT16 dev;
	UINT16 inum;
	struct dsknod d;
};

#define	INODE_TO_BLK(i)	(INODES_START + (i / I_PER_BLK))
#define	INODE_OFF(i)	(i % I_PER_BLK)

extern int openfs(char *name, struct super **f);
extern int openfsrw(char *name, struct super **f, int writable);
extern void closefs(struct super *f);

extern char *mytime();

extern struct dsknod *iget(struct super *f, int inum);
extern void iput(struct dsknod *ip);
extern void ifree(struct dsknod *ip);

extern void lose(char *s);
extern int readblk(struct super *f, int blkno, char *buf);
extern int writeblk(struct super *f, int blkno, char *buf);
extern int fileread(struct dsknod *ip, int offset, char *buf);
extern int filewrite(struct dsknod *ip, int offset, char *buf);
extern void idump(struct dsknod *ip);
extern void isummary(char *name, struct dsknod *ip);
extern void dump(unsigned char *buf, int size);
extern void dumpsb(struct super *sb);
extern void secdump(unsigned char *buf);
extern UINT secmap(struct super *fs, UINT blkno);
extern int bmap(struct dsknod *ip, int offset, int alloc);
extern struct dsknod *namei(struct super *f, char *name);
extern struct dir *getdir(struct dsknod *ip);
extern struct dir *getdirent(struct dsknod *ip, int index);
extern int balloc(struct super *f);
extern void bfree(struct super *f, int b);
extern void filefree(struct dsknod *ip);
extern void fileunlink(struct super *f, char *name);
extern struct dsknod *filecreate(struct super *f, char *name);
extern int dircreate(struct super *f, char *name);
extern int dirrm(struct super *f, char *name);

