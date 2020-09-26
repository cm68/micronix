/*
 * sup.h 
 */

/*
 * Structure of super block
 * (block 1 on a file device)
 */
struct sup
{
    UINT isize;                 /* number of blocks devoted to ilist */
    UINT fsize;                 /* largest file block + 1 */
    UINT nfree;                 /* number of free blocks in free[] */
    UINT free[100];             /* free block numbers */
    UINT ninode;                /* number of free inumbers in inode[] */
    UINT inode[100];            /* free inode numbers */
    char flock;                 /* irrelevant on disk */
    char ilock;                 /* irrelevant on disk */
    ULONG time;                 /* time of last umount */
};

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
