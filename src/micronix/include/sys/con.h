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
    INT16 (*open) (INT16, INT16);
    INT16 (*close) (INT16, INT16);
    INT16 (*strat) (struct buf *);
    INT16 (*ioctl) (INT16, INT16, char *, struct buf *);
} biosw[];

/*
 * Convention: major device 0 is reserved for NODEV
 */
#define NODEV	0

/*
 * Character device switch
 */
struct ciovec {
    INT16 (*open) (INT16, INT16);
    INT16 (*close) (INT16, INT16);
    INT16 (*read) (INT16);
    INT16 (*write) (INT16);
    INT16 (*ioctl) (INT16, INT16, char *);            /* the terminal's requests live here:
                                 * stty/gtty are TIOCSETA/TIOCGETA rather
                                 * than a hook of their own */
} ciosw[];

/*
 * Macros for accessing the major and minor device numbers.
 */
#define minor(dev)	((dev) & 0377)
#define major(dev)	((UINT8)((dev) >> 8))

/*
 * The minor number of a disk, cut into the two things its driver wants
 * from it: which drive, and which slice (sys/dlabel.h).  The slice sits
 * at the top so that every device number in use before slices existed
 * keeps its meaning and names slice 'a'; the three bits below it are
 * spare, since the drive type the minor once named is gone - a drive's
 * geometry now comes from its label, not a table row.  The drive bits
 * below the slice are the driver's own - two for mw.c's four drives, one
 * for the ide card's two.
 */
#define devslice(dev)	((minor(dev) >> 5) & 7)

/*
 * Globals initialized in con.c
 */
extern UINT16 nbdev;              /* number of block devices */
extern UINT16 ncdev;              /* number of character devices */
extern UINT16 rootdev;            /* device number of root device */
extern UINT16 swapdev;            /* device number of swap device */
extern UINT16 swapsize;           /* number of swap blocks */
extern UINT16 swapaddr;           /* number of first swap block */

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
