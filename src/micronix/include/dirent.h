/*
 * data structure returned by readdir
 * part of the opendir library
 *
 * /include/dirent.h
 *
 * Changed: <2021-12-23 14:48:12 curt>
 */

/*
 * an open directory
 */
struct dirhandle {
	short fd;
	struct dir d;
	char pad;					/* null termination for length 14 file
								 * name */
};

typedef struct dirhandle DIR;

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
