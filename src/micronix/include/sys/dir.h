struct dir {
	UCOUNT ino;
	TEXT name[14];
};

/*
 * v7 compatibility
 */
#define	d_ino	ino
#define	d_name	name
#define	direct	dir

