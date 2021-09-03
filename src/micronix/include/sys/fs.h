/*
 * sys/fs.h
 *
 * this is modified from the stock fs.h to align it with v6
 *      field names and defines now match
 * filesystem superblock - found on block 1 of every bdev
 */
struct super {
    UINT s_isize;               /* number of inode blocks */
    UINT s_fsize;               /* largest file block + 1 */
    UINT s_nfree;               /* number of free blocks in free[] */
    UINT s_free[100];           /* the free block list */
    UINT s_ninode;              /* number of free inumbers in inode[] */
    UINT s_inode[100];          /* the free inode list */

    UINT8 s_flock;              /* mounted read only */
    UINT8 s_ilock;
    UINT8 s_fmod;               /* dirty */

    UINT32 s_time;              /* last umount time */
};

/*
 * the on-disk inode - these fields have the d_ prefix to
 * allow the in-memory inode to have the i_ prefix reference
 * the included structure.
 */
struct dsknod {
    UINT d_mode;                /* what kind of inode */
    UINT8 d_nlink;              /* link count */
    UINT8 d_uid;                /* user id of owner */
    UINT8 d_gid;                /* group id of owner */
    UINT8 d_size0;              /* high byte of size */
    UINT d_size1;               /* low word */
    UINT d_addr[8];             /* block list of file */
    UINT32 d_atime;             /* time of last read */
    UINT32 d_mtime;             /* time of last write */
};

#define IALLOC		0100000     /* inode is allocated */

#define ILARG		0010000     /* large file addressing */

#define IFMT		0060000     /* inode type */
#define     IFREG		0000000 /* a file */
#define     IIO 		0020000 /* io nodes have this set */
#define     IFCHR		0020000 /* cdev */
#define     IFDIR		0040000 /* directory */
#define     IFBLK		0060000 /* bdev */

#define ISUID		0004000     /* set uid */
#define ISGID		0002000     /* set gid */
#define ISVTX       0001000     /* sticky */

#define IPERM		0000777     /* permissions masks */
#define     IREAD   0000004
#define     IWRITE  0000002
#define     IEXEC   0000001
/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
