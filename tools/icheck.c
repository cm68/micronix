
#include "fs.h"

#define	NINODE	16*64

#define	NB	10

struct	dsknod	inode[NINODE];

#define	BMAPSIZE	8192
#define	MAXBLK		(BMAPSIZE * 8)

union {
        struct sup sbl;
        char buf[512];
} sblu;
#define sblock sblu.sbl

int	sflg;

int	nfile;
int	nspcl;
int	nlarg;
int	nvlarg;
int	nindir;
int	nvindir;
int	ndir;
int	nused;
int	nfree;
int	ino;
int	ndup;
int	blist[10] = { -1 };
int	nerror;
unsigned char	bmap[BMAPSIZE];

main(argc, argv)
char **argv;
{
	register char **p;
	register int n, *lp;

	while (--argc) {
		argv++;
		if (**argv=='-') switch ((*argv)[1]) {
		case 'v':
			verbose++;
			continue;
		case 's':
			sflg++;
			continue;

		case 'b':
			lp = blist;
			while (lp < &blist[NB-1] && (n = number(argv[1]))) {
				*lp++ = n;
				argv++;
				argc--;
			}
			*lp++ = -1;
			continue;

		default:
			printf("Bad flag\n");
		}
		check(*argv);
	}
	return(nerror);
}

check(file)
char *file;
{
	int i, j, k;

	image = open(file, sflg?2:0);
	if (image < 0) {
		printf("cannot open %s\n", file);
		nerror |= 04;
		return;
	}
	printf("%s:\n", file);
	nfile = 0;
	nspcl = 0;
	nlarg = 0;
	nvlarg = 0;
	nindir = 0;
	nvindir = 0;
	ndir = 0;
	nused = 0;
	nfree = 0;
	ndup = 0;

	/* clear the bitmap */
	for (i = 0; i < BMAPSIZE; i++)
		bmap[i] = 0;
	sync();

	/* process all the inodes */
	readblk(1, &sblock);
	for (ino = 1; ino < sblock.isize * I_PER_BLK; ino++) {
		pass1(iget(ino));
	}

	sync();
	readblk(1, &sblock);
	/* rebuild the freelist */
	if (sflg) {
		makefree();
		return;
	}

	if (verbose > 2) printf("freehead: %d\n", sblock.free[0]);

	/* check for freelist dups */
	while(i = alloc()) {
		if (chk(i, 0, "free"))
			break;
		nfree++;
	}
	if (ndup) {
		printf("%d dups in free\n", ndup);
		nerror |= 02;
	}

	/* count the allocated blocks in the bitmap */
	j = 0;
	for (i = 0; i < BMAPSIZE; i++) {
		for (k = bmap[i]; k; k >>= 1)
			j++;
	}
	if (j != sblock.fsize - sblock.isize + 3)
		printf("missing %5d %5d\n", j, sblock.fsize - sblock.isize + 3);
	printf("spcl  %6d\n", nspcl);
	printf("files %6d\n", nfile);
	printf("large %6d\n", nlarg);
	if (nvlarg)
		printf("huge  %6d\n", nvlarg);
	printf("direc %6d\n", ndir);
	printf("indir %6l\n", nindir);
	if (nvindir)
		printf("indir2%6d\n", nvindir);
	printf("used  %6d\n", nused);
	printf("free  %6d\n", nfree);
	close(image);
}

pass1(struct dsknod *ip)
{
	UINT buf[256];	// indirect block
	UINT vbuf[256];	// double indirect
	register i, j;

	if (!(ip->mode & IALLOC))
		return;

	if (ip->mode & IIO) {
		nspcl++;
		return;
	}

	if ((ip->mode&ITYPE) == IDIR)
		ndir++;
	else
		nfile++;

	if (verbose) idump(ino, ip);

	if ((ip->mode & ILARGE) != 0) {
		nlarg++;
		for(i=0; i<7; i++)
		if (ip->addr[i] != 0) {
			nindir++;
			if (chk(ip->addr[i], i * 256, "indirect"))
				continue;
			readblk(ip->addr[i], buf);
			if (verbose) secdump(buf);
			for(j=0; j<256; j++)
			if (buf[j] != 0)
				chk(buf[j], i*256+j,"data (large)");
		}
		if (ip->addr[7]) {
			nvlarg++;
			if (chk(ip->addr[7], 7 * 256,"indirect"))
				return;
			readblk(ip->addr[7], buf);
			if (verbose) secdump(buf);
			for(i=0; i<256; i++)
			if (buf[i] != 0) {
				nvindir++;
				if (chk(buf[i], (7+i) * 256, "2nd indirect"))
					continue;
				readblk(buf[i], vbuf);
				if (verbose) secdump(vbuf);
				for(j=0; j<256; j++)
				if (vbuf[j])
					chk(vbuf[j], (7+i) * 256 + j, "data (very large)");
			}
		}
		return;
	}
	for(i=0; i<8; i++) {
		if (ip->addr[i] != 0)
			chk(ip->addr[i], i, "data (small)");
	}
}

/*
 * check to see if a block is in the allocation bitmap
 */
chk(blkno, lblkno, mesg)
UINT blkno;
UINT lblkno;
char *mesg;
{
	unsigned char mask;
	int index;
	int i;

	if (ino)
		nused++;

	if (blkno < sblock.isize + 2 || blkno >= sblock.fsize) {
		printf("%d bad; inode=%d, lblkno=%d class=%s\n", blkno, ino, lblkno, mesg);
		return(1);
	}

	mask = 1 << (blkno & 0x07);
	index = blkno >> 3;
	if (index > sizeof(bmap)) printf("lose: blkno %d bigger than bitmap\n", blkno);

	if (bmap[index] & mask) {
		printf("%d dup; inode=%d, lblkno=%d class=%s\n", blkno, ino, lblkno, mesg);
		ndup++;
	}
	bmap[index] |= mask;
	for (i=0; blist[i] != -1; i++)
		if (blkno == blist[i])
			printf("%d arg; inode=%d, lblkno=%d class=%s\n", blkno, ino, lblkno, mesg);
	return(0);
}

alloc()
{
	register b, i;
	UINT buf[256];

	i = --sblock.nfree;
	if (i < 0 || i >= 100) {
		printf("bad freeblock\n");
		return(0);
	}
	b = sblock.free[i];

	if (b == 0)
		return(0);

	if (sblock.nfree <= 0) {
		
		readblk(b, &buf);
		if (verbose > 2) secdump(&buf);
		sblock.nfree = buf[0];
		for(i=0; i<100; i++)
			sblock.free[i] = buf[i+1];
	}
	return(b);
}

bfree(blkno)
{
	register i;
	UINT buf[256];

	if (sblock.nfree >= 100) {
		buf[0] = sblock.nfree;
		for(i=0; i<100; i++)
			buf[i+1] = sblock.free[i];
		sblock.nfree = 0;
		writeblk(blkno, buf);
	}
	sblock.free[sblock.nfree++] = blkno;
}

makefree()
{
	register i;

	sblock.nfree = 0;
	sblock.ninode = 0;
	sblock.flock = 0;
	sblock.ilock = 0;
	sblock.time = 0;
	bfree(0);
	for(i=sblock.fsize-1; i>=sblock.isize+2; i--) {
		if ((bmap[(i>>3)] & (1<<(i&0x7)))==0)
			bfree(i);
	}
	writeblk(1, &sblock);
	close(image);
	sync();
	return;
}
