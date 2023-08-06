/*
 * STRUCTURES FOR PASS 0
 * copyright (c) 1978 by Whitesmiths, Ltd.
 *
 * pp/int0.h
 *
 * Changed: <2023-06-28 00:37:06 curt>
 */

#ifdef LONGNAME
#define LENNAME	32
#else
#define LENNAME	8
#endif

/*
 *	the types
 */
#define PCHCON	1
#define PEOL	2
#define PIDENT	3
#define PNUM	4
#define PPUNCT	5
#define PSTRING	6

/*
 *	the keywords
 */
#define PDEFINE	10
#define PELSE	11
#define PENDIF	12
#define PIF		13
#define PIFDEF	14
#define PIFNDEF	15
#define PINCLUD	16
#define PLINE	17
#define PSHARP	18
#define PUNDEF	19

/*
 *	the linked list of included files
 */
struct incl {
	struct incl *next;
	char *fname;
	short nline;
	FILE *file;
};

/*
 *	the predefined keywords
 */
struct pretab {
	char *prename;
	int pretype;
};

/*
 *	the linked list of tokens
 */
struct tlist {
	struct tlist *next;
	int type;
	char *white;
	unsigned short nwhite;
	char *text;
	unsigned short ntext;
};

/*
 *	the argument list structure
 */
struct alist {
	struct alist *next;
	struct tlist *astart;
	struct tlist *aend;
};

/*
 *	the symbol table definition entry
 */
#define NHASH	128
#define STRSIZE	512

struct def {
	struct def *next;
	unsigned short dnlen;
	char *defn;
	char dname[LENNAME];
};

/*
 * ordered include path.
 */
struct incpath {
    struct incpath *next;
    char *path;
};

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
