/*
 * in-core structure describing mounted filesystems
 *
 * include/sys/mount.h
 * Changed: <2021-12-23 14:27:56 curt>
 */

/*
 * The fsize and isize are copied here from the superblock
 * to make them more accessible.  bfree and ifree are the live
 * free-block and free-inode counts, kept real by the allocators
 * and copied back into the superblock at sync time - they are the
 * numbers df reports, and the superblock is their backing store.
 */
struct mount {
    UINT dev;                   /* device number */
    struct inode *inode;        /* inode on which mounted */
    UINT8 ronly;                /* non zero if read only */
    UINT fsize;                 /* see sup.h */
    UINT isize;                 /* see sup.h */
    UINT bfree;                 /* live free-block count */
    UINT ifree;                 /* live free-inode count */
} mlist[];

struct mount *mlook();          /* find a mount entry by device number */
void syncsuper();               /* copy the live counts into the superblocks */

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
