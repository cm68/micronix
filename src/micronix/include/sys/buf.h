/*
 * Block io buffer header
 *
 * include/sys/buf.h
 * Changed: <2021-12-23 14:18:06 curt>
 */
struct buf {
    UINT8 flags;                /* see below */
    UINT dev;                   /* device number */
    UINT blk;                   /* block number */
    UINT count;                 /* number of bytes to transfer */
    char *data;                 /* memory address */
    UINT8 xmem;                 /* extended address */
    struct buf *forw;           /* for use by strategy routine */
    struct buf *back;           /* for use by strategy routine */
    UINT cyl;                   /* for use by strategy routine */
    UINT8 error;                /* error return */
    UINT time;                  /* "time" of last access */
} blist[];

/*
 * flag bits - originally in octal, but screw that.
 */
#define BWRITE	0x00            /* mneumonic only */
#define ASYNC	0x00            /* mneumonic only */
#define BREAD	0x01            /* read the disk */
#define BBUSY	0x02            /* busy -- do not access */
#define BSYNC	0x04            /* syncronous io -- wait for completion */
#define BDELWRI 0x08            /* write out before using */
#define BDONE	0x10            /* io done -- data is valid */
#define BLOCK	0x20            /* locked in core -- do not take */
#define BWANT	0x40            /* wake up &buf on release */
#define BERROR	0x80            /* error on last io transfer */

/*
 * xmem value
 */
#define KERNEL	0               /* extended address of kernel memory */

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
