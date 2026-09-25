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
    struct buf *b_hash;         /* hash chain link */
};

extern struct buf blist[];      /* boot headers; textpad.s links blist last */

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
 * Window routines (uio.c).  A buffer's 512 bytes live in an unmapped
 * physical segment and are reached by mapping that segment into the
 * BUFSEG window: bhold() returns the buffer's address in the window and
 * brel() gives the window back.  Holds stack, and they must be short -
 * never sleep with one held (sleep() panics on it), and don't reach a
 * buffer header, or anything that maps the window for its own use, while
 * one is up.
 *
 * 0xd000 is the page mem.s borrows for user memory and puts back, which
 * is what lets the buffer pool carry headers there; the pool stops at the
 * window's own page (see expand_bufs) because a header there would be
 * read back as buffer data while a hold is up.  The superblock needs no
 * window - getsb() keeps it in the kernel's own memory.
 */
extern char *bhold();           /* map a buffer's data, return its address */
extern void brel();             /* put back the segment the hold saved */
extern int bwin();              /* the raw mapping, without the stack */

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
