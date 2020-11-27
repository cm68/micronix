/*	HEADER FILE FOR PASCAL FILE I/O
 *	copyright (c) 1980 by Whitesmiths, Ltd.
 */

typedef struct {
	char *p_buf;
	int p_size;
	char p_mode;
	unsigned char p_fd;
} PFILE;

/*
 * modes
 */
#define P_EOF	0
#define P_EOLN	1
#define P_INVAL	2
#define P_VALID	3
#define P_WRITE	4
#define P_WROTE	5

#define NFILES	16
#define P_TFD	0200
