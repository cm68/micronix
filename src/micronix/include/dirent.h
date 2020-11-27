/*
 * data structure returned by readdir
 * part of the opendir library
 */

/*
 * an open directory
 */
struct dirhandle {
	int fd;
	struct dir d;
	char pad;			/* null termination for length 14 file name */
};

typedef struct dirhandle DIR;
