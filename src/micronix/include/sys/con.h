/*
 * sys/con.h - the device driver switch tables
 */

/*
 * A block io vector is a list of 3 driver addresses for one major device: an
 * open, close, and strategy routine. Biosw is an an array of such vectors
 * indexed by major device numbers.
 */
struct biovec {
    int (*open) ();
    int (*close) ();
    int (*strat) ();
} biosw[];

/*
 * Device names for diagnostics
 */
char *devname[];

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
 * Globals initialized in con.c
 */
UINT nbdev;                     /* number of block devices */
UINT ncdev;                     /* number of character devices */
UINT rootdev;                   /* device number of root device */
UINT swapdev;                   /* device number of swap device */
UINT swapsize;                  /* number of swap blocks */
UINT swapaddr;                  /* number of first swap block */

/*
 * Note: if swapdev == rootdev, then swapaddr is set to rootdev's fsize
 */

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
