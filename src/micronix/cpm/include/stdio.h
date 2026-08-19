#define	BUFSIZ		512
/*
 * Six FILE slots: stdin, stdout and stderr hold the first three for
 * the life of the program, and three are left over for the files it
 * opens.  The FCB table underneath is cpm.h's MAXFILE (twelve), so
 * this is not the limit on files - a descriptor opened directly with
 * open() and read/written without a FILE does not use one of these.
 */
#define	_NFILE		6
# ifndef FILE
#define	uchar	unsigned char

extern	struct	_iobuf {
	char *		_ptr;
	int		_cnt;
	char *		_base;
	uchar		_flag;
	char		_file;
} _iob[_NFILE];

extern uchar _setup;

# endif

#define	_IOREAD		01	/* 0x01 */
#define	_IOWRT		02	/* 0x02 */
#define	_IORW		03	/* 0x03, _IOREAD|_IOWRT */
#define	_IONBF		04	/* 0x04 */
#define	_IOMYBUF	010	/* 0x08 */
#define	_IOEOF		020	/* 0x10 */
#define	_IOERR		040	/* 0x20 */
#define	_IOSTRG		0100	/* 0x40 */
/*
 * _IOBINARY records the "b" of a mode string.  Text is CP/M's default:
 * fgetc drops the \r of a \r\n pair and stops at a ctrl-Z, and fputc
 * expands \n back to \r\n.  A file written here is read back here, so
 * the translation round-trips - but a binary file (a compiler object,
 * a disk image) must be opened "rb" or the first 0x1a in it reads as
 * end of file.
 */
#define	_IOBINARY	0200	/* 0x80 */

#ifndef NULL
#define	NULL		(void *)0
#endif

#define	FILE		struct _iobuf
#define	EOF		(-1)

/*
 * stdin/stdout/stderr are the first three FILE slots, spelled as
 * macros rather than the pointers a Unix libc exports.  A program
 * that takes &stdout gets the address of a variable here, not the
 * FILE * itself; code that wants the pointer uses stdout, not &stdout.
 */
#define	stdin		(&_iob[0])
#define	stdout		(&_iob[1])
#define	stderr		(&_iob[2])

#define	getc(p)		fgetc(p)
#define	getchar()	getc(stdin)
#define	putc(x,p)	fputc(x,p)
#define	putchar(x)	putc(x,stdout)
#define	feof(p)		(((p)->_flag&_IOEOF)!=0)
#define	ferror(p)	(((p)->_flag&_IOERR)!=0)
#define	fileno(p)	((uchar)p->_file)

FILE *		fopen();
FILE *		freopen();
FILE *		fdopen();
long		ftell();
/*
 * A prototype, and it has to be one: fseek takes a long, and a caller
 * that writes fseek(fp, 0, SEEK_SET) with a plain int pushes two bytes
 * where the body reads four, so the offset is built from the offset
 * and the whence.  The same trap the Unix stdio.h documents.
 */
int		fseek(FILE *, long, int);
char *		fgets();
char *		_bufallo();
int fgetc();
int fputc();
int printf(char *, ...);
int fprintf(FILE *, char *, ...);
int sprintf(char *, char *, ...);

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
