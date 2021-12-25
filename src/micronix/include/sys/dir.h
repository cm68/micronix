/*
 * on disk directory format
 *
 * include/sys/dir.h
 * Changed: <2021-12-23 14:20:40 curt>
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
