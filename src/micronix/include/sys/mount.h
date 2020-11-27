/*
 * sys/mount.h
 */

/*
 * Mount structure. The fsize and isize are copied here from the superblock
 * to make them more accessible.
 */
struct mount {
    UINT dev;                   /* device number */
    struct inode *inode;        /* inode on which mounted */
    UINT8 ronly;                /* non zero if read only */
    UINT fsize;                 /* see sup.h */
    UINT isize;                 /* see sup.h */
} mlist[];

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
