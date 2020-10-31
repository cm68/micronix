/*
 * buf.h 
 */

/*
 * Block io buffer header
 */
struct buf
{
    UCHAR flags;                /* see below */
    UINT dev;                   /* device number */
    UINT blk;                   /* block number */
    UINT count;                 /* number of bytes to transfer */
    char *data;                 /* memory address */
    UCHAR xmem;                 /* extended address */
    struct buf *forw;           /* for use by strategy routine */
    struct buf *back;           /* for use by strategy routine */
    UINT cyl;                   /* for use by strategy routine */
    UCHAR error;                /* error return */
    UINT time;                  /* "time" of last access */
} blist[];

/*
 * flag bits
 */
#define BWRITE	0000            /* mneumonic only */
#define ASYNC	0000            /* mneumonic only */
#define BREAD	0001            /* read the disk */
#define BBUSY	0002            /* busy -- do not access */
#define BSYNC	0004            /* syncronous io -- wait for completion */
#define BDELWRI 0010            /* write out before using */
#define BDONE	0020            /* io done -- data is valid */
#define BLOCK	0040            /* locked in core -- do not take */
#define BWANT	0100            /* wake up &buf on release */
#define BERROR	0200            /* error on last io transfer */

/*
 * xmem value
 */
#define KERNEL	0               /* extended address of kernel memory */
/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
