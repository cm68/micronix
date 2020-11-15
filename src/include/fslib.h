#define	INODES_START	2
#define	I_PER_BLK	16
#define	NOPEN	16
#define	NSIG	16

#define	INODE_TO_BLK(i)	(INODES_START + (i / I_PER_BLK))
#define	INODE_OFF(i)	(i % I_PER_BLK)

#include <fcntl.h>
#include <time.h>

extern int openfs(char *name, struct sup **f);
extern void closefs(struct sup *f);

extern char *mytime();

extern struct dsknod *iget(struct sup *f, int inum);
extern void iput(struct dsknod *ip);
extern void ifree(struct dsknod *ip);

extern void lose(char *s);
extern int readblk(struct sup *f, int blkno, char *buf);
extern int writeblk(struct sup *f, int blkno, char *buf);
extern int fileread(struct dsknod *ip, int offset, char *buf);
extern int filewrite(struct dsknod *ip, int offset, char *buf);
extern void idump(struct dsknod *ip);
extern void isummary(char *name, struct dsknod *ip);
extern void dump(unsigned char *buf, int size);
extern void dumpsb(struct sup *sb);
extern void secdump(unsigned char *buf);
extern UINT secmap(struct sup *fs, UINT blkno);
extern int bmap(struct dsknod *ip, int offset, int alloc);
extern struct dsknod *namei(struct sup *f, char *name);
extern struct dir *getdirent(struct dsknod *ip, int index);
extern int balloc(struct sup *f);
extern void bfree(struct sup *f, int b);
extern void filefree(struct dsknod *ip);
extern void fileunlink(struct sup *f, char *name);
extern struct dsknod *filecreate(struct sup *f, char *name);
extern int dircreate(struct sup *f, char *name);
extern int dirrm(struct sup *f, char *name);

