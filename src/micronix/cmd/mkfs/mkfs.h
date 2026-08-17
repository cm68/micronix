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
#define NDRIVE      5

/*
 * The drives, from specs[] in sys/mw.c.  static here so the driver and
 * the worker each carry their own copy; the table is five drives and
 * this is simpler than a third object for them.
 *
 *	tracks	heads	sectors		blocks
 *	153	4	17		10404	5 meg
 *	306	4	17		20808	10 meg
 *	306	6	17		31212	16 meg
 *	640	6	17		65280	32 meg
 *	733	5	17		62305	40 meg
 */
static UINT dtracks[NDRIVE] = { 153, 306, 306, 640, 733 };
static UINT dheads[NDRIVE] = { 4, 4, 6, 6, 5 };
static UINT dsecs[NDRIVE] = { 17, 17, 17, 17, 17 };

extern char *pname;

/* K&R declarations: ccc predates prototypes, and the definitions are
 * K&R too, so the arguments get the default promotion on both sides. */
void die();
void domkfs();

/* the block i/o, defined by the driver (raw device) or by mnix (image) */
void rdblk();
void wrblk();
