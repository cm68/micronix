/*
 * the device driver switch tables
 *
 * include/sys/con.h
 * Changed: <2021-12-23 14:18:35 curt>
 */

/*
 * A block io vector is a list of 4 driver addresses for one major device:
 * an open, close, and strategy routine, and the one that runs a raw
 * command block (include/sys/ioctl.h). Biosw is an an array of such
 * vectors indexed by major device numbers.
 *
 * The fourth entry is the block side's answer to the character switch's
 * mode() below, and it is a separate entry rather than a ride on that one
 * because a block device is not a character device: the mode() hook is
 * reached through ciosw[] and a disk has no row there. A driver whose bus
 * has no command block to run - the HD-DMA board is the one left -
 * declares 0, and a call against it answers ENOTTY rather than reaching
 * anything.
 */
struct biovec {
    int (*open) ();
    int (*close) ();
    int (*strat) ();
    int (*ioctl) ();
} biosw[];

/*
 * Device names for diagnostics
 */
char devname[][11];

/*
 * Convention: major device 0 is reserved for NODEV
 */
#define NODEV	0

/*
 * Character device switch
 */
struct ciovec {
    int (*open) ();
    int (*close) ();
    int (*read) ();
    int (*write) ();
    int (*mode) ();
} ciosw[];

/*
 * Macros for accessing the major and minor device numbers.
 */
#define minor(dev)	((dev) & 0377)
#define major(dev)	((UINT16)(dev) >> 8)

/*
 * The minor number of a disk, cut into the three things its driver wants
 * from it: which drive, which row of its geometry table, and which slice
 * (sys/dlabel.h).  The slice sits at the top so that every device number
 * in use before slices existed keeps its meaning and names slice 'a'; the
 * price is that a driver's table is eight rows rather than sixty four.
 * The drive bits below these are the driver's own - two for mw.c's four
 * drives, one for the ide card's two.
 */
#define devslice(dev)	((minor(dev) >> 5) & 7)
#define devtype(dev)	((minor(dev) >> 2) & 7)

/*
 * Globals initialized in con.c
 */
extern UINT nbdev;              /* number of block devices */
extern UINT ncdev;              /* number of character devices */
extern UINT rootdev;            /* device number of root device */
extern UINT swapdev;            /* device number of swap device */
extern UINT swapsize;           /* number of swap blocks */
extern UINT swapaddr;           /* number of first swap block */

/*
 * Note: if swapdev == rootdev, then swapaddr is set to rootdev's fsize
 */

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
