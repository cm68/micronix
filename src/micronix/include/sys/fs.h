/*
 * sys/fs.h
 * 
 * note:  this file was duplicated in the micronix 1.61 source as sup.h the v6
 * filesystem superblock - found on block 1 of every bdev
 */
struct super {
    UINT isize;                 /* number of inode blocks */
    UINT fsize;                 /* largest file block + 1 */
    UINT nfree;                 /* number of free blocks in free[] */
    UINT free[100];             /* the free block list */
    UINT ninode;                /* number of free inumbers in inode[] */
    UINT inode[100];            /* the free inode list */

    UINT8 flock;                /* mounted read only */
    UINT8 ilock;

    UINT32 time;                /* last umount time */
};

/*
 * the on-disk inode
 */
struct dsknod {
    UINT mode;                  /* what kind of inode */
    UINT8 nlink;                /* link count */
    UINT8 uid;                  /* user id of owner */
    UINT8 gid;                  /* group id of owner */
    UINT8 size0;                /* high byte of size */
    UINT size1;                 /* low word */
    UINT addr[8];               /* block list of file */
    UINT32 actime;              /* time of last read */
    UINT32 modtime;             /* time of last write */
};

#define D_ALLOC		0100000 /* inode is allocated */

#define D_IFMT		0060000 /* inode type */
#define D_IFREG		0000000 /* a file */
#define D_IIO		0020000 /* io nodes have this set */
#define D_IFCHR		0020000 /* cdev */
#define D_IFDIR		0040000 /* directory */
#define D_IFBLK		0060000 /* bdev */

#define D_LARGE		0010000 /* large file addressing */
#define D_ISUID		0004000 /* set uid */
#define D_ISGID		0002000 /* set gid */
#define D_1WRITE	0001000 /* exclusive write (!) */

#define D_PERM		0000777 /* permissions masks */

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
