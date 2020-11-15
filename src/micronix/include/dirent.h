/*
 * data structure returned by readdir
 * part of the opendir library
 */

typedef short ino_t;

struct dirent {
	ino_t d_ino;
	char d_name[14];
};

struct dirhandle {
	short fd;
	struct dirent d;
	char pad;
};

typedef struct dirhandle DIR;
