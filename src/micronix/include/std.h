/*
 * pseudo storage classes
 */

/* awful brain damage */

#ifdef notdef

# define AFAST	register
# define FAST	register
# define GLOBAL	extern
# define IMPORT	extern
# define INTERN	static
# define LOCAL	static

/*
 *  pseudo types
 */

typedef char TEXT, TINY;
typedef double DOUBLE;
typedef float FLOAT;
typedef int ARGINT, BOOL, VOID;
typedef long LONG;
typedef short BITS, COUNT, FILE;
typedef unsigned BYTES;
typedef unsigned char UTINY;
typedef unsigned long ULONG;
typedef unsigned short UCOUNT;
#endif

/*
 * system parameters
 */

# define STDIN	0
# define STDOUT	1
# define STDERR	2
# define YES		1
# define NO		0
# define NULL	0
# define BUFSIZE	512
# define BWRITE	-1
# define READ	0
# define WRITE	1
# define UPDATE	2
# define EOF		-1
# define BYTMASK	0377

/*
 *	macros
 */

# define abs(x)		((x) < 0 ? (-(x)) : (x))
# define isalpha(c)	(islower(c) || isupper(c))
# define isdigit(c)	('0' <= (c) && (c) <= '9')
# define islower(c)	('a' <= (c) && (c) <= 'z')
# define isupper(c)	('A' <= (c) && (c) <= 'Z')
# define iswhite(c)	((c) <= ' ' || 0177 <= (c)) /* ASCII ONLY */
# define isblack(a)	(!iswhite(a))
# define max(x, y)	(((x) < (y)) ? (y) : (x))
# define min(x, y)	(((x) < (y)) ? (x) : (y))


# define tolower(c)	(isupper(c) ? ((c) + ('a' - 'A')) : (c))
# define toupper(c)	(islower(c) ? ((c) - ('a' - 'A')) : (c))

/*
 * the file IO structure
 * used by the whitesmith's flavor of stdio
 */
typedef struct fio {
	UINT8 _fd;
	UINT _nleft;
	UINT _fmode;
	char *_pnext;
	char *_buf;
	UINT _size;	/* buffer size */
} FIO;
