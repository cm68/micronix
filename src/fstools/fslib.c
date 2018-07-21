/*
 * common code for micronix filesystem maintenance utilities
 * brute force for everything
 */
#include "fs.h"
#include <stdio.h>

int spt = 15;
int altsec = 1;
int image;
int verbose;

/*
 * do sector skew
 */
int 
secmap(int blkno)
{
	int trk = blkno / spt;
	int sec = blkno % spt;

	if (altsec) {
		sec <<= 1;
		if (!(spt & 1) && sec >= spt) {
			sec++;
		}
		sec %= spt;
	}
	return (trk * spt + sec);
}

void
lose(char *s)
{
	fprintf(stderr, s);
	exit(1);
}

char *itype[] = {
	"IFREG",
	"CDEV",
	"DIR",
	"BDEV"
};

struct sup *fs;
struct dsknod *ip;
struct dir *dp;
char dbuf[512];

/*
 * a kinda cute hexdump
 */
void
secdump(unsigned char *buf)
{
	int i;
	int c;
	unsigned char cb[17];

	cb[0] = cb[16] = 0;

	for (i = 0; i < 512; i++) {
		if (i % 16 == 0) printf("%s\n%3x: ", cb, i);
		c = buf[i];
		printf("%02x ", c);
		if (c < ' ' || c >= 127) c = '.';
		cb[i % 16] = c;
	}
	printf("\n");
}

/*
 * print out a directory block
 */
void
dirdump(char *buf, int size)
{
	struct dir *dp;

	if (verbose) fprintf(stderr, "dirdump: %d %x\n", size, buf);
	if (size > 512) size = 512;
	for (dp = (struct dir *)buf; dp < (struct dir *)&buf[size]; dp++) {
		printf("%5d: %14s\n", dp->inum, dp->name);
	}
}

/*
 * get the contents of a block
 */
int
readblk(int blkno, char *buf)
{
	int realblk;

	realblk = secmap(blkno);
	if (verbose > 1) fprintf(stderr, "readblk: %d -> %d\n", blkno, realblk);

	if (lseek(image, 512 * realblk, SEEK_SET) < 0) 
		lose("readblk seek");
	if (read(image, buf, 512) != 512) 
		lose("readblk");
	return 0;
}

/*
 * scribble the contents of a block
 */
int
writeblk(int blkno, char *buf)
{
	int realblk;

	realblk = secmap(blkno);
	if (verbose > 1) fprintf(stderr, "writeblk: %d -> %d\n", blkno, realblk);

	if (lseek(image, 512 * realblk, SEEK_SET) < 0) 
		lose("writeblk seek");
	if (write(image, buf, 512) != 512) 
		lose("writeblk");
	return 0;
}

char timebuf[20];

/*
 * micronix timestamps seem to longs in a bizarre byte order
 */
char *
mytime(ULONG t)
{
#ifndef lame
	sprintf(timebuf, "%08x", t);
	return timebuf;
#else
	time_t t1;

	t1 = ((t >> 16) & 0xffff) | ((t & 0xffff) << 16);

	return ctime(&t1);
#endif
}

struct dsknod dinode;
/*
 * read an inode, given the inum
 */
struct dsknod *
iget(int inum)
{
	int iblk;
	int offset;

	inum--;

	iblk = secmap(inum / I_PER_BLK + INODES_START);
	offset = iblk * 512 + (inum % I_PER_BLK) * 32;
	lseek(image, offset, SEEK_SET);
	if (verbose > 2) fprintf(stderr, "iget: %d %x\n", inum, offset);

	if (read(image, &dinode, sizeof(dinode)) < 0) 
		lose("inode read");
	return &dinode;
}

blocklist(UINT *buf)
{
	int i;
	for (i = 0; i < 256; i++) {
		if ((i % 16) == 0) {
			printf("\n");
		}
		printf("%5d ", buf[i]);
	}
	printf("\n");
}

/*
 * print out an inode, raw
 */
void
idump(int ino, struct dsknod *ip)
{
	int i;
	int j;
	unsigned int ft;
	unsigned int size;
	UINT indir[256];
	UINT dindir[256];

	if (verbose > 1) fprintf(stderr, "idump %d\n", i);

	if (!(ip->mode & IALLOC))
		return;

	ft = (ip->mode & ITYPE) >> 13;
	printf("inode %5d: %6o %d %s\n", ino, ip->mode, ft, itype[ft & 3]);

	size = (ip->size0 << 16) + ip->size1;

	printf("\tmode: %o links: %d uid: %d gid: %d size: %d %s\n",
		ip->mode, ip->nlinks, ip->uid, ip->gid, 
		size, ip->mode & ILARGE ? "LARGE" : "");
	
 	printf("\trtime: %s ", mytime(ip->rtime));
 	printf("wtime: %s\n", mytime(ip->wtime));

	if (ip->mode & IIO) {
		int maj = (ip->addr[0] >> 8) & 0xff;
		int min = ip->addr[0] & 0xff;

		printf("\tmaj: %d min: %d\n", maj, min);
	} else {
		printf("\tblocks:\n\t\t");
		for (i = 0; i < 8; i++) {
			printf("%d ", ip->addr[i]);
			if (ip->mode & ILARGE) {
				if (!ip->addr[i]) continue;
				printf("indirect\n");
				readblk(ip->addr[i], indir);
				blocklist(indir);
			}
		}
		if ((ip->mode & ILARGE) && ip->addr[7]) {
			printf("double indirect\n");
			readblk(ip->addr[7], dindir);
			blocklist(dindir);
			for (i = 0; i < 256; i++) {
				printf("indirect %d\n", i);
				readblk(dindir[i], indir);
				blocklist(indir);
			}
		}
		printf("\n");
	}
	if ((ip->mode & ITYPE) == IDIR) {
		for (i = 0; i < 8; i++) {
			if (i * 512 > size) break;
			readblk(ip->addr[i], dbuf);
			dirdump(dbuf, size - (i * 512));
		}
	}
}

struct dir *
dread(struct dsknod *ip, int off)
{
        register b;
        static ibuf[256];
        static char buf[512];

        if (off > (ip->size1 + (ip->size0 << 16))) return (0);
        if ((off&0777)==0) {
                if (off==0177000) {
                        printf("Monstrous directory\n");
                        return(0);
                }
                if ((ip->mode&ILARGE)==0) {
                        if (off>=010000 || (b = ip->addr[off>>9])==0)
                                return(0);
                        readblk(b, buf);
                } else {
                        if (off==0) {
                                if (ip->addr[0]==0)
                                        return(0);
                                readblk(ip->addr[0], ibuf);
                        }
                        if ((b = ibuf[(off>>9)&0177])==0)
                                return(0);
                        readblk(b, buf);
                }
        }
        return(&buf[off&0777]);
}

int
number(as)
char *as;
{
        register n, c;
        register char *s;

        s = as;
        n = 0;
        while ((c = *s++) >= '0' && c <= '9') {
                n = n*10+c-'0';
        }
        return(n);
}

