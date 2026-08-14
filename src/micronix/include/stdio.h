#define	BUFSIZ		512
/*
 * Six left three for the program, because stdin, stdout and stderr
 * hold the first three for the whole life of it.  wsld wants more
 * than that on its own: it keeps every input open at once - crt0.o,
 * the objects, libc.a, libu.a - and the output as well, and it fell
 * over on libu.a with "cannot open" for a file that was there and
 * readable.  fopen has no descriptor left to fail on at that point;
 * it never gets as far as open.
 *
 * A FILE is eight bytes, so this costs 48 bytes of bss in every
 * program.  The 512 byte buffer behind one is only allocated when a
 * file is actually opened, so nothing pays for slots it does not use.
 */
#define	_NFILE		12
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
/*
 * _IORW IS ITS OWN BIT, and used to be the OR of the two above it.
 * That cannot work: fseek asks "is this stream read-write" with it and
 * then clears _IOREAD and _IOWRT to leave the direction undecided, so
 * using the answer destroyed it.  The first seek worked and the stream
 * never came back - once a read or a write had claimed the direction,
 * the next seek saw a single bit and concluded the stream had never
 * been read-write at all.
 *
 * It takes 0200, which was _IOBINARY.  That flag recorded the "b" of a
 * mode string and NOTHING EVER TESTED IT - fputc.s and fgetc.s each say
 * so, where the CP/M \r\n translation used to be.  A file written here
 * is read back here and there is nothing to translate.
 */
#define	_IORW		0200	/* 0x80 */
#define	_IONBF		04	/* 0x04 */
#define	_IOMYBUF	010	/* 0x08 */
#define	_IOEOF		020	/* 0x10 */
#define	_IOERR		040	/* 0x20 */
#define	_IOSTRG		0100	/* 0x40 */

#ifndef NULL
#define	NULL		(void *)0
#endif

#define	FILE		struct _iobuf
#define	EOF		(-1)

/* whence values for fseek.  unistd.h has them too, for lseek */
#ifndef SEEK_SET
#define	SEEK_SET	0
#define	SEEK_CUR	1
#define	SEEK_END	2
#endif

#define	getc(p)		fgetc(p)
#define	getchar()	getc(stdin)
#define	putc(x,p)	fputc(x,p)
#define	putchar(x)	putc(x,stdout)
#define	feof(p)		(((p)->_flag&_IOEOF)!=0)
#define	ferror(p)	(((p)->_flag&_IOERR)!=0)
#define	fileno(p)	((uchar)p->_file)

extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

FILE *		fopen();
FILE *		freopen();
FILE *		fdopen();
long		ftell();
/*
 * A prototype, and it has to be one: fseek takes a long, and almost
 * every caller in the tree writes fseek(fp, 0, SEEK_SET) with a plain
 * int.  Undeclared, that pushed two bytes where the body reads four,
 * so the offset was built from the offset and the whence, and whence
 * itself came off the stack from beyond the arguments.  lseek saw a
 * whence it did not recognise, returned EINVAL without ever issuing
 * the seek, and fseek reported failure - silently, since hardly
 * anyone checks it.  asz assembled into a temp file, seeked back to
 * copy it into the object, and copied nothing.
 */
int		fseek(FILE *, long, int);
char *		fgets();
char *		_bufallo();
int _flsbuf();
/*
 * int, not char: both return the character read or written, or EOF,
 * which is -1 - a value a char cannot hold apart from a data byte of
 * 0xff.  The bodies have always returned the full word in HL; the
 * char spelling made every "!= EOF" test depend on the sign-extension
 * of a truncated return.
 */
int fgetc();
int fputc();
int printf(char *, ...);
int fprintf(FILE *, char *, ...);
int sprintf(char *, char *, ...);

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
