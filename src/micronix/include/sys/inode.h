/*
 * the in-core inode
 * 
 * this include file is strictly for the use of the kernel. 
 *
 * include/sys/inode.h
 * Changed: <2021-12-23 14:26:57 curt>
 */

struct inode {
    struct stat statb;          /* returned to user with stat(2) */

    UINT8 flags;                /* see below */
    UINT8 count;                /* count of in-core references */
    struct mount *mount;        /* mounted on this inode */
    UINT32 size;                /* for computational convenience */
    UINT time;                  /* "time" of last reference */
};

/*
 * access macros so the kernel can get simply at the composite type. note
 * that the stat struct contains a disk inode, so it's 2 deep. even deeper if
 * you count the dev_t.
 */
#define i_dev   statb.dev_u.dev_s
#define i_minor statb.dev_u.dev_b.minor_b
#define i_major statb.dev_u.dev_b.major_b
#define i_inum  statb.ino
#define i_mode  statb.d.d_mode
#define i_nlink statb.d.d_nlink
#define i_uid   statb.d.d_uid
#define i_gid   statb.d.d_gid
#define i_size0 statb.d.d_size0
#define i_size1 statb.d.d_size1
#define i_addr  statb.d.d_addr
#define i_rtime statb.d.d_atime
#define i_mtime statb.d.d_mtime
#define i_flags flags
#define i_count count
#define i_mount mount
#define i_size  size
#define i_time  time

/*
 * Flag bits
 */
#define IBUSY	1               /* changing: do not access */
#define IMOD	2               /* inode has been modified */
#define IWANT	4               /* wakeup address of inode on release */
#define IRONLY	8               /* device is mounted read-only */
#define IWRLOCK 16              /* do not open for writing */
#define IPIPE	32              /* delay disk write of full blocks */

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
