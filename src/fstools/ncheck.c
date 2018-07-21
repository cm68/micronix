
#include "fs.h"

#define NINODE	16*64

union {
	struct sup sbl;
	char buf[512];
} sblu;
#define	sblock sblu.sbl

struct	dsknod	inode[NINODE];

int	sflg;
int	aflg;
#define	NI	20
#define	NDIRS	787

int pass1(), pass2(), pass3();

int	iilist[NI] = { -1};
struct	htab {
	int	hino;
	int	hpino;
	char	hname[14];
} htab[NDIRS];
int	nhent=	10;
int	(*pass[])()=	{ pass1, pass2, pass3 };
char	*lasts;
int	ino;
int	nerror;
int	nfiles;
struct htab *lookup();

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

		case 'a':
			aflg++;
			continue;

		case 'i':
			lp = iilist;
			while (lp < &iilist[NI-1] && (n = number(argv[1]))) {
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
	register i, j, pno;

	image = open(file, 0);
	if (image < 0) {
		printf("cannot open %s\n", file);
		return;
	}
	printf("%s:\n", file);
	sync();
	readblk(1, &sblock);
	nfiles = sblock.isize*I_PER_BLK;
	for (i=0; i<NDIRS; i++)
		htab[i].hino = 0;
	for (pno=0; pno<3; pno++) {
		for (ino = 1; ino < nfiles; ino++) {
			(*pass[pno])(iget(ino));
		}
	}
}

pass1(struct dsknod *ip)
{
	if ((ip->mode&IALLOC)==0 || (ip->mode&ITYPE)!=IDIR)
		return;
	lookup(ino, 1);
}

pass2(ip)
struct dsknod *ip;
{
	register doff;
	register struct htab *hp;
	register struct dir *dp;
	int i;

	if ((ip->mode&IALLOC)==0 || (ip->mode&ITYPE)!=IDIR)
		return;

	for (doff = 0; (dp = dread(ip, doff)) != 0; doff += 16) {
		if (dp->inum==0)
			continue;
		if ((hp = lookup(dp->inum, 0)) == 0)
			continue;
		if (dotname(dp))
			continue;
		hp->hpino = ino;
		for (i=0; i<14; i++)
			hp->hname[i] = dp->name[i];
	}
}

pass3(ip)
struct dsknod *ip;
{
	register doff;
	register struct dir *dp;
	register int *ilp;

	if ((ip->mode&IALLOC)==0 || (ip->mode&ITYPE)!=IDIR)
		return;

	for (doff = 0; (dp = dread(ip, doff)) != 0; doff += 16) {
		if (dp->inum==0)
			continue;
		if (aflg==0 && dotname(dp))
			continue;
		for (ilp=iilist; *ilp >= 0; ilp++)
			if (*ilp == dp->inum)
				break;
		if (ilp > iilist && *ilp!=dp->inum)
			continue;
		printf("%d	", dp->inum);
		pname(ino, 0);
		printf("/%.14s\n", dp->name);
	}
}

dotname(struct dir *dp)
{
	if (dp->name[0]=='.')
		if (dp->name[1]==0 || dp->name[1]=='.' && dp->name[2]==0)
			return(1);
	return(0);
}

pname(i, lev)
{
	register struct htab *hp;

	if (i==1)
		return;
	if ((hp = lookup(i, 0)) == 0) {
		printf("???");
		return;
	}
	if (lev > 10) {
		printf("...");
		return;
	}
	pname(hp->hpino, ++lev);
	printf("/%.14s", hp->hname);
}

struct htab *
lookup(i, ef)
{
	register struct htab *hp;

	for (hp = &htab[i%NDIRS]; hp->hino;) {
		if (hp->hino==i)
			return(hp);
		if (++hp >= &htab[NDIRS])
			hp = htab;
	}
	if (ef==0)
		return(0);
	if (++nhent >= NDIRS) {
		printf("Out of core-- increase NDIRS\n");
		exit(1);
	}
	hp->hino = i;
	return(hp);
}
