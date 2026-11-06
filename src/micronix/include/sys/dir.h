#ifndef _SYS_DIR_H
#define _SYS_DIR_H

/*
 * on disk directory format
 *
 * include/sys/dir.h
 * Changed: <2021-12-23 14:20:40 curt>
 */
struct dir {
    UINT16 ino;
    UINT8 name[14];
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

#endif
