/*
 * filesystem on disk data structures
 *
 * include/sys/fs.h
 * Changed: <2021-12-23 14:25:52 curt>
 */

/*
 *
 * filesystem superblock - found on block 1 of every bdev
 *
 * this is modified from the stock fs.h to align it with v6
 *      field names and defines now match
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

    /*
     * Spelled out, and it has to be.  The three bytes above leave the
     * struct at an odd offset, and s_time is 32 bits: a host compiler
     * aligns it to 4 and slides a byte of padding in here, ccc aligns
     * to 2 and does not.  That one byte was the whole difference
     * between mkfs writing s_time where the kernel reads it and
     * writing it one byte late - the kernel then read the padding plus
     * three bytes of the stamp, and its year, counting from 1970, ran
     * off the end of the 2-digit field.  Naming the byte puts it in
     * both layouts, at offset 412.
     */
    UINT8 s_pad;

    UINT32 s_time;              /* last umount time */
    UINT32 s_magic;             /* filesystem signature */
    UINT s_bfree;               /* total free blocks */
    UINT s_ifree;               /* total free inodes */
};

/*
 * The filesystem signature, stamped into the superblock by mkfs and
 * checked by df (and fsck) in one field instead of three block reads.
 * A 32-bit field, so it rides the same host/guest NUXI word order as
 * s_time above - see the swap in cmd/mkfs/mkfsfunc.c.
 */
#define FsMAGIC 0xDEADBEEFL

/*
 * The value as it sits on the disk.  A 32-bit field on a micronix disk
 * carries its high word at the lower address, which a host build's
 * little-endian long writes the other way up; a guest's long is already
 * in the disk's order.  INTEGER_32 (types.h) marks the host builds, so
 * only they swap - the same convention s_time uses in mkfsfunc.c.
 */
#ifdef INTEGER_32
#define FsMAGIC_DISK    (((UINT32) FsMAGIC >> 16) | ((UINT32) FsMAGIC << 16))
#else
#define FsMAGIC_DISK    FsMAGIC
#endif

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
