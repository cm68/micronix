/*
 * sys/dir.h
 * 
 * on disk directory format
 */
struct dir {
    UINT ino;
    char name[14];
};

/*
 * v7 compatibility
 */
#define	d_ino	ino
#define	d_name	name
#define	direct	dir

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
