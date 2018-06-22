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
extern int number();
extern struct dir *dread(struct dsknod *ip, int off);

