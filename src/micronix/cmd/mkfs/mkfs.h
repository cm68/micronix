/*
 * shared between the mkfs command driver and the worker that mnix also
 * links.  The worker builds a filesystem on a block device described by
 * rdblk/wrblk, which each side defines differently: the driver opens the
 * raw unix device, mnix opens the simulated drive image.
 *
 * cmd/mkfs/mkfs.h
 */

#define BSIZE       512
#define SUPERBLK    1           /* the superblock */
#define INOSTART    2           /* first inode block */
#define IPERBLK     16          /* inodes to a block */
#define ROOTINO     1
#define NICFREE     100         /* free block list in the superblock */
#define NICINOD     100         /* free inode list in the superblock */
#define APERBLK     256         /* block numbers in an indirect block */
#define DEFBOOT     "/bootmw.bin"
#define NDRIVE      8

/*
 * The media this can make a filesystem on: the five hard disks from
 * specs[] in sys/mw.c and then the three diskettes, whose rows are
 * specs[] in sys/dj.c as well (the same geometry, and toff is the track
 * offset both carry).  static here so the driver and the worker each
 * carry their own copy; the table is eight rows and this is simpler than
 * a third object for them.
 *
 *	tracks	heads	sectors	toff		blocks	volume
 *	153	4	17	0		10404	5 meg
 *	306	4	17	0		20808	10 meg
 *	306	6	17	0		31212	16 meg
 *	640	6	17	0		65280	32 meg
 *	733	5	17	0		62305	40 meg
 *	77	1	15	2		 1155	8 inch, single sided
 *	77	2	15	2		 2310	8 inch, double sided
 *	40	2	10	2		  800	5 1/4 inch, double sided
 *
 * toff is the whole difference between the two label idioms and the only
 * thing distinguishing a diskette's row from a drive's.  A hard disk is
 * rolled: mw.c adds d_roll to blk / spc, so the filesystem wraps and the
 * boot lands at physical cylinder 0 from the inside.  A diskette is not:
 * dj.c adds toff and never rotates, so the boot is simply the sectors in
 * front of the filesystem and no block of the filesystem reaches them.
 * A row with toff therefore gets the slice idiom - d_roll 0, d_cyl0 0, a
 * slice table naming cylinder toff as 'a' - and a row without it gets
 * the roll idiom, exactly as before.  See putlabel() and sys/dlabel.h.
 *
 * The 5 1/4 single sided row (40/1/10, 380 blocks) is deliberately not
 * here: 194 K cannot hold a running system, and a medium that can only
 * be a data carrier is not worth a row.
 */
static UINT dtracks[NDRIVE] = { 153, 306, 306, 640, 733, 77, 77, 40 };
static UINT dheads[NDRIVE] = { 4, 4, 6, 6, 5, 1, 2, 2 };
static UINT dsecs[NDRIVE] = { 17, 17, 17, 17, 17, 15, 15, 10 };
static UINT dtoff[NDRIVE] = { 0, 0, 0, 0, 0, 2, 2, 2 };

/*
 * And the number of the first sector on a track, which is a property of
 * the medium and not of its shape: it says whether the sectors are
 * marked by index holes in the jacket - hard sectored, which counts from
 * zero - or written into the track, which is soft sectored and counts
 * from one.  The eight inch media are soft sectored and the 5 1/4 inch
 * double sided one is Morrow's hard sectored format; a hard disk does
 * not go through this at all, because its controller is told a sector
 * number rather than made to find one, and zero is what the driver's own
 * table expects.
 *
 * This has to be right.  A driver that numbers a soft sectored track
 * from zero asks for a sector the track does not have, and one that
 * numbers a hard sectored track from one reads the sector beside the
 * one it wanted - on every block, all the way to the end of the medium.
 * It is written into the label as d_firstsec, and the loader reads it
 * out of there, because it has no other way to be told: see
 * sys/dlabel.h and stand/boot/djio.c.
 */
static UINT dfirst[NDRIVE] = { 0, 0, 0, 0, 0, 1, 1, 0 };

extern char *pname;

/* K&R declarations: ccc predates prototypes, and the definitions are
 * K&R too, so the arguments get the default promotion on both sides. */
void die();
void domkfs();
/* declared because mnix calls it directly: a diskette's label lives in
 * device block 0, which the worker's wrblk cannot reach, so the driver
 * writes that one block itself through this. */
void putlabel();

/* the block i/o, defined by the driver (raw device) or by mnix (image) */
void rdblk();
void wrblk();
