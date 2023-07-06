/*
 * the whitesmith's stdio implementation
 *
 * /include/stdio.h

 * Modified: <2023-07-05 23:06:12 curt>

 * vim: tabstop=4 shiftwidth=4 expandtab:
 */

#define	_BUFSIZE	512
#define	 BUFSIZ		512
#define	_NFILE		16

typedef struct _iobuf {
	char *_ptr;
	int _cnt;
	char *_base;
	int _flag;
	int _fd;
	long _offset;
	int _pid;
} FILE;

extern FILE _iob[_NFILE];

extern _exit();
extern exit();

#define	stdin	(&_iob[0])
#define	stdout	(&_iob[1])
#define	stderr	(&_iob[2])

#define	_READ	01
#define	_WRITE	02
#define	_UNBUF	04
#define	_BIGBUF	010
#define	_EOF	020
#define	_ERR	040
#define	_UNSEEK 0100			/* the _fd is not seekable */
#define	_PIPE	0200
#define	_TTY	0400			/* _fd corresponds to a tty */
#define	_ALLOC  01000			/* buffer obtained through alloc */

#define	getc(p) (((--(p)->_cnt) >= 0) ? (*(p)->_ptr++ & 0377) : (_fillbuf (p)))

#define	getchar() getc(stdin)

#define	putc(x,p) \
	{\
	if (--(p)->_cnt >= 0)\
		*(p)->_ptr++ = (x);\
	else\
		_flushbuf ((x & 0377), (p));\
	if ((x) == '\n' && (p)->_flag & (_TTY | _UNSEEK))\
		fflush (p);\
	}

#define	putchar(x)	putc(x,stdout)

#define	fflush(a)	(_flushbuf(EOF,(a)))

#define isalpha(a) (isupper(a) || islower(a))
#define isupper(a) ('A' <= (a) && (a) <= 'Z')
#define islower(a) ('a' <= (a) && (a) <= 'z')
#define isdigit(a) ('0' <= (a) && (a) <= '9')
#define isspace(a) ((a) == ' ' || (a) == '\t' || (a) == '\n')
#define toupper(a) (islower(a) ? ((a) + 'A' - 'a') : (a))
#define tolower(a) (isupper(a) ? ((a) + 'a' - 'A') : (a))

/*
 * system parameters
 */

#define STDIN	0
#define STDOUT	1
#define STDERR	2
#define YES		1
#define NO		0
#define NULL	0
#define BUFSIZE	512
#define READ	0
#define WRITE	1
#define UPDATE	2
#define EOF		-1
#define BYTMASK	0377

/*
 *  macros
 */

#define abs(x)		((x) < 0 ? (-(x)) : (x))
#define iswhite(c)	((c) <= ' ' || 0177 <= (c))	/* ASCII ONLY */
#define isblack(a)	(!iswhite(a))
#define max(x, y)	(((x) < (y)) ? (y) : (x))
#define min(x, y)	(((x) < (y)) ? (x) : (y))

#define feof(a) ((a)->_flag & (_EOF | _ERR))
#define ferror(a) ((a)->_flag & _ERR)
#define fileno(a) ((a)->_fd)
#define clearerr(a) ((a)->_flag &= ~ERR)
