#define	INODES_START	2
#define	I_PER_BLK	16
#define	NOPEN	16
#define	NSIG	16

#define	INODE_TO_BLK(i)	(INODES_START + (i / I_PER_BLK))
#define	INODE_OFF(i)	(i % I_PER_BLK)

typedef unsigned char UCHAR;
typedef unsigned short UINT;
typedef unsigned int ULONG;

#include "../include/inode.h"
#include "../include/sup.h"

#include <fcntl.h>
#include <time.h>

struct dir {
	UINT inum;
	UCHAR name[14];
};

extern int verbose;
extern int image;
extern char *mytime();
extern struct dsknod *iget();

extern void lose(char *s);
extern int readblk(int blkno, char *buf);
extern int fileread(struct dsknod *ip, int offset, char *buf);
extern int filewrite(struct dsknod *ip, int offset, char *buf);
extern void idump(int ino, struct dsknod *ip);
extern void dump(unsigned char *buf, int size);
extern void dumpsb(struct sup *sb);
extern void secdump(unsigned char *buf);
extern UINT secmap(UINT blkno);
extern int bmap(struct dsknod *ip, int offset, int alloc);
extern int lookup(struct dsknod *ip, char *name);
extern struct dir *getdirent(struct dsknod *ip, int index);
