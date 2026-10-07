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

/*
 * No media table.  mkfs makes a filesystem on whatever the device already
 * is: the geometry comes off the label, and the medium is not named again
 * here.  The one program that still knows drive shapes is mwformat - and,
 * on the host, mnix's initialize, which stands in for it.
 */

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
