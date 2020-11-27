/*
 * sys/file.h
 * 
 * open file structure
 */
struct file {
    UINT8 mode;                 /* IREAD, IWRITE, PIPE == IEXEC */
    UINT8 count;
    struct inode *inode;
    UINT32 rwptr;
} flist[];

#define PIPE	IEXEC           /* must include inode.h before this */

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
