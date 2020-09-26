/********************************************************/
/*							*/
/*		Q/C V3.2 Disk I/O Library		*/
/*							*/
/*     Copyright (c) 1984 Quality Computer Systems	*/
/*							*/
/*			03/21/84			*/
/********************************************************/

#include "qstdio.h"

/*
 * If PORTABLE is defined, then only C versions of library routines
 * are used. Otherwise, assembler versions are used where available.

#define PORTABLE 1
*/

/* Library parameters */

#define MAXARG	20	/* max no. of command line arguments */
#define MAXLINE 81	/* maximum size of console input line */
#define NFILES	10	/* max no. of files that can be open */
#define MAXINT	32767	/* maximum size integer */
#define NSECTS	4	/* no. of CP/M records to buffer */
#define SECSIZE 128	/* size of CP/M record */
#define BUFSIZE (NSECTS * SECSIZE) /* buffer size for buffered I/O */

/* Miscellaneous CP/M constants */

#define CPMFCBSIZE 36
#define DBUFF	0x80
#define CPMERR	0xFF
#define CPMEOF	0x1A
#define CTLC	0x3	/* ^C */

/* CP/M function numbers */

#define CONIN	1
#define CONOUT	2
#define LSTOUT	5
#define PRINTS	9
#define CONBUF	10
#define CONSTAT 11
#define OPENF	15
#define CLOSEF	16
#define DELETEF 19
#define MAKEF	22
#define SETDMA	26
#define READR	33
#define GTFSIZE 35
#define WRITERZ 40

/* I/O control blocks (iob's) for buffered files
 * struct _iob {
 *	char	_flag;		status flag
 *	char	*_pch;		ptr to curr char in buffer
 *	int	_cnt;		no. of chars remaining in buffer
 *	char	*_buf;		address of buffer
 *	int	_bufsize;	size of buffer
 *	char	_fd;		file descriptor (fd) for low-level I/O
 *	};
 */

/* I/O flag masks */

#define READ	01
#define WRITE	02
#define APPEND	04
#define BINARY	010
#define BUF	020
#define USERBUF 040
#define FEOF	0100
#define FERR	0200

/* File control block for low-level I/O */

struct fcb {
	char	flag;
	unsigned file_size;
	struct cpmfcb {
		char	drive;
		char	filename[8];
		char	filetype[3];
		char	extent;
		int	reserved;
		char	rec_cnt;
		char	dir[16];
		char	curr_rec;
		unsigned rand_rec;
		char	overflow;
		} cpm_fcb;
	} *_fcb[NFILES];	/* file control block pointers */

FILE	*stdin = CONIN, 	/* default to console */
	*stdout = CONOUT,
	*stderr = CONOUT,
	_iob[NFILES];		/* I/O blocks (iob) for buffered I/O */
int	_ungch = EOF;		/* hold unget character for stdin */

/*
 * Memory allocator
 */
struct _header {
	struct _header	*_next; /* next free block */
	unsigned	_bsize; /* size of block */
	};
typedef struct _header HEADER;

extern char	*_free; 	/* free space address */
unsigned	_moat = 1000;	/* moat between free space and stack */
HEADER		_base,		/* empty free space list */
		*_allocp;	/* last allocated block */

/* Parse command line and set standard files (does redirection) */
_rshell()
	{
	static	argc,
		argv[MAXARG],
		argflag = 0,	/* not in an argument */
		cmdcnt,
		c;
	static char
		*cmdline = 0x80, /* CP/M command line at 80H */
		*pargs,
		*ptr;
	char args[128], infile[15], outfile[15];
	FILE *fopen();

	cmdcnt = *cmdline++;	/* first byte is length */
	pargs = args;		/* point to args */
	*pargs =		/* set command = null string */
	infile[0] =		/* no redirection of standard files */
	outfile[0] = '\0';
	argv[argc++] = pargs;	/* record pointer to 1st arg */
	while (cmdcnt--) {
		c = *cmdline++;
		switch (argflag) {
		case 0: 		/* not in an arg */
			if (isspace(c))
				continue; /* still not in an arg */
			if (argc == MAXARG) {
				fputs("Too many command line args\n", stderr);
				exit(1);
				}
			argflag = c;	/* set type of arg */
			if (c == '<')	/* redirect stdin */
				ptr = infile;
			else if (c == '>') /* redirect stdout */
				ptr = outfile;
			else {		/* just an arg */
				argv[argc++] = pargs + 1;
				if (c != '"')	/* if not a string */
					break;	/* record char */
				}
			continue;	/* skip this character */
		case '<':		/* record redirection */
		case '>':		/* file name */
			if (isspace(c))
				c = argflag = 0; /* end of name */
			*ptr++ = c;
			continue;
		case '"':		/* in a string */
			if (c == '"')
				c = argflag = 0; /* end of string */
			break;
		default:
			if (isspace(c))
				c = argflag = 0; /* end of arg */
			break;
			}
		*++pargs = c;
		}
	if (argflag == '<' || argflag == '>') /* end last arg */
		*ptr = '\0';		/* a redirected filename */
	else
		*++pargs = '\0';	/* a normal arg */
	if (infile[0] != '\0') {	/* check for redirection */
		if ((stdin = fopen(infile, "r")) == NULL)
			cantopen(infile);
		}
	if (outfile[0] != '\0') {
		if ((stdout = fopen(outfile, "w")) == NULL)
			cantopen(outfile);
		}
	main(argc, argv);
	exit(0);
	}
/* Parse command line but don't do redirection */
_shell()
	{
#ifdef PORTABLE
	static	argc,
		argv[MAXARG],
		argflag = 0,	/* not in an argument */
		cmdcnt,
		c;
	static char
		*cmdline = 0x80, /* CP/M command line at 80H */
		*pargs,
		*ptr;
	char args[128];

	cmdcnt = *cmdline++;	/* first byte is length */
	pargs = args;		/* point to args */
	*pargs = '\0';		/* set command = null string */
	argv[argc++] = pargs;	/* record pointer to 1st arg */
	while (cmdcnt--) {
		c = *cmdline++;
		switch (argflag) {
		case 0: 		/* not in an arg */
			if (isspace(c))
				continue; /* still not in an arg */
			if (argc == MAXARG) {
				bdos1(PRINTS,"Too many command line args\r\n$");
				_exit();
				}
			argflag = c;	/* set type of arg */
			argv[argc++] = pargs + 1;
			if (c != '"')	/* if not a string */
				break;	/* record char */
			continue;	/* skip this character */
		case '"':		/* in a string */
			if (c == '"')
				c = argflag = 0; /* end of string */
			break;
		default:
			if (isspace(c))
				c = argflag = 0; /* end of arg */
			break;
			}
		*++pargs = c;
		}
	*++pargs = '\0';		/* end last arg */
	main(argc, argv);
	_exit();
#else
#asm 8080
TAB	EQU	9
BLANK	EQU	32
QUOTE	EQU	34
INARG	EQU	1
INSTR	EQU	2
	EXTRN	main?,?ult,r?1?
	LXI	H,-128
	DAD	SP
	SPHL		;reserve space for argv strings
	SHLD	r?1?	;remember where
	XCHG		;DE points to argv strings
	LXI	B,1	;initialize argc
	PUSH	B
	XRA	A
	STAX	D	;argv[0] points to null string
	INX	D
	MOV	C,A	;zero inarg and instring flags
	LXI	H,80H	;CP/M command line
	MOV	B,M	;size of command line
?@sh0:	INX	H	;next command char
	DCR	B
	JM	?@sh6	;end of command line
	MOV	A,C
	CPI	INSTR	;in a string?
	JNZ	?@sh1
	MOV	A,M
	CPI	QUOTE	;end of string?
	JMP	?@sh2
?@sh1:	CPI	INARG	;in an arg?
	JNZ	?@sh3
	CALL	?@sh9	;white space?
?@sh2:	JNZ	?@sh5
	XRA	A	;argv string terminator
	MOV	C,A	;turn off flag
	JMP	?@sh5
?@sh3:	CALL	?@sh9	;white space?
	JZ	?@sh0	;ignore
	XTHL		;retrieve argc
	INX	H	;count arg
	XTHL
	CPI	QUOTE
	JNZ	?@sh4
	MVI	C,INSTR
	JMP	?@sh0
?@sh4:	MVI	C,INARG
?@sh5:	STAX	D
	INX	D
	JMP	?@sh0
?@sh6:	XRA	A
	STAX	D	;terminate last arg
	POP	B	;retrieve argc
?@sh7:	DCX	D
	PUSH	D
	LHLD	r?1?	;beginning of args
	CALL	?ult	;end of argv area?
	POP	D
	JNZ	?@sh8
	LDAX	D
	CPI	0
	JNZ	?@sh7
	INX	D
	PUSH	D	;record in argv
	DCX	D
	JMP	?@sh7
?@sh8:	INX	D
	PUSH	D	;argv[0]
	PUSH	B	;argc
	LXI	H,2
	DAD	SP	;address of argv
	PUSH	H	;argv
	CALL	main?
	JMP	0	;reboot CP/M
?@sh9:	MOV	A,M	;chk for white space
	CPI	TAB
	RZ
	CPI	BLANK
#endasm
#endif
	}
cantopen(filename)
char *filename;
	{
	fputs("Can't open: ", stderr);
	fputs(filename, stderr);
	exit(1);
	}
/* Cleanup routine - close files, etc. */
exit(error)
	{
	register FILE *fp;
	register n;

	for (fp = _iob, n = 0; n < NFILES; ++fp, ++n)
		fclose(fp);		/* close any output files */
	if (error)
		unlink("A:$$$.SUB");	/* era submit file, if any */
	_exit();			/* reboot CP/M */
	}
/* Immediate exit to CP/M with no cleanup */
_exit()
	{
	bdos1(0, 0);
	}
/* Get a string from stdin */
char *gets(s)
char *s;
	{
	char *fgets(), *index(), *p;

	if (fgets(s, MAXLINE, stdin) == NULL)
		return NULL;
	if (p = index(s, '\n')) /* strip newline if present */
		*p = '\0';
	return s;
	}
/* Put a string to stdout */
puts(s)
char *s;
	{
	while(*s)
		putchar(*s++);
	putchar('\n');		/* always append newline */
	}
/* Read formatted input from stdin */
scanf(nargs)
	{
	return (_scan(stdin, &nargs + nargs));
	}
/* Read formatted input from a file */
fscanf(nargs)
	{
	register *p;

	p = &nargs + nargs;		/* first arg (file ptr) */
	return (_scan(*p, p - 1));	/* _scan does the work */
	}
/* Internal routine for scanf/fscanf */
_scan(fp, pargs)
register FILE *fp;
register char **pargs;
	{
	static char *fmt, *s;
	static count, c, f, quit, suppress, width,
		base, d, maxd, sign, value;

	fmt = *pargs;
	count = 0;
	quit = FALSE;
	while (f = *fmt++) {
		if (isspace(f)) 	/* skip white space in format */
			continue;
		if (f != '%') { 	/* must match next input string */
			while (isspace(c=getc(fp)))
				;
			if (f == c)
				continue;
			else {		/* doesn't match so put it back */
				ungetc(c, fp);
				break;
				}
			}
		if (*fmt == '*') {
			suppress = TRUE;
			++fmt;
			}
		else
			suppress = FALSE;
		width = (isdigit(*fmt))? _atoi(&fmt): -1;
		if (*fmt == 'h')	/* ignore short int modifier */
			++fmt;
		switch (chupper(*fmt++)) {
		case '%':
			while (isspace(c=getc(fp)))
				;
			if (c == '%')
				continue;
			else {
				quit = TRUE;
				break;
				}
		case 'X':
			base = 16;
			maxd = 'F';
			sign = 1;
			goto getnum;
		case 'O':
			base = 8;
			maxd = '7';
			sign = 1;
			goto getnum;
		case 'D':
			base = 10;
			maxd = '9';
			if (c == '-') {
				sign = -1;
				c = getc(fp);
				}
			else
				sign = 1;
		getnum:
			while (isspace(c=getc(fp)))
				;
			d = chupper(c);
			if (isalnum(d) && d <= maxd) {
				if (!suppress)
					++count;
				}
			else {
				quit = TRUE;
				break;
				}
			value = 0;
			while (width--) {
				d = chupper(c);
				if (isdigit(d) && d <= maxd)
					d -= '0';
				else if (isalpha(d) && d <= maxd)
					d -= ('A' - 10);
				else
					break;
				value = base * value + d;
				c = getc(fp);
				}
			if (!suppress)
				*(int *)*--pargs = sign * value;
			break;
		case 'C':
			if ((c=getc(fp)) == EOF)
				break;
			if (suppress) {
				if (width == -1)
					continue; /* no char to unget */
				while (width--) {
					if ((c=getc(fp)) == EOF)
						break;
					}
				break;
				}
			++count;
			if (width == -1) { /* get single char */
				**--pargs = c;
				continue;
				}
			else {		/* get string */
				s = *--pargs;
				while (width--) {
					*s++ = c;
					if ((c=getc(fp)) == EOF)
						break;
					}
				*s = '\0';
				}
			break;
		case 'S':
			while (isspace(c=getc(fp)))
				;
			if (c == EOF)
				break;
			if (!suppress) {
				++count;
				s = *--pargs;
				}
			while (width--) {
				if (!suppress)
					*s++ = c;
				if (isspace(c=getc(fp)) || c == EOF)
					break;
				}
			if (!suppress)
				*s = '\0';
			break;
		default:		/* can't figure this out */
			quit = TRUE;
			break;
			}
		if (c == EOF)
			break;
		else
			ungetc(c, fp);	/* put back unused character */
		if (quit == TRUE)
			break;
		}
	return ((count == 0 && c == EOF)? EOF: count);
	}
/* Short version of printf to save space */
qprintf(nargs)
	{
	register char **arg, *fmt;
	register c, base;
	char s[7], *itob();

	arg = (char **)&nargs + nargs;
	fmt = *arg;
	while (c = *fmt++) {
		if (c != '%') {
			putchar(c);
			continue;
			}
		switch (c = *fmt++) {
		case 'c':
			putchar(*--arg);
			continue;
		case 'd':
			base = -10;
			goto prt;
		case 'o':
			base = 8;
			goto prt;
		case 'u':
			base = 10;
			goto prt;
		case 'x':
			base = 16;
		prt:
			fputs(itob(*--arg, s, base), stdout);
			continue;
		case 's':
			fputs(*--arg, stdout);
			continue;
		default:
			putchar(c);
			continue;
			}
		}
	}
/* Formatted print routine */
printf(nargs)
	{
	int putc();		/* function _fmt will use as output */
	_fmt(putc, stdout, &nargs + nargs);
	}
/* Write formatted output to a file */
fprintf(nargs)
	{
	int putc();		/* function _fmt will use as output */
	register *p;

	p = &nargs + nargs;	/* address of first arg (file ptr) */
	_fmt(putc, *p, p - 1);	/* _fmt does the formatting */
	}
/* Get character from standard input file */
getchar()
	{
	return getc(stdin);
	}
/* Put character to standard output file */
putchar(c)
	{
	return putc(c, stdout);
	}
/* Open buffered file */
FILE *fopen(filename, mode)
register char *filename;
char *mode;
	{
	register FILE *fp;
	register n, fd, binmode, rwmode;

	for (fp = _iob, n = 0; n < NFILES; ++fp, ++n)
		if (!(fp->_flag & (READ|WRITE|APPEND)))
			break;		/* found empty iob */
	if (n >= NFILES)
		return NULL;		/* no empty iob's */

	binmode = (chupper(*(mode + 1)) == 'B'); /* binary I/O? */
	switch (chupper(*mode)) {
	case 'R':
		rwmode = READ;
		fd = open(filename, 0);
		break;
	case 'W':
		rwmode = WRITE;
		/* check for output to printer (LST:) */
		if (chupper(filename[0]) == 'L' &&
		    chupper(filename[1]) == 'S' &&
		    chupper(filename[2]) == 'T' && filename[3] == ':')
			fd = LSTOUT;
		else
			fd = creat(filename, 0644); /* 0644 = UNIX mode */
		break;
	case 'A':
		rwmode = WRITE;
		if ((fd=open(filename, 1)) == -1)
			fd = creat(filename, 0644);
		else if (binmode)
			seekr(fd, 0, 2); /* start after last rec */
		else
			rwmode = (WRITE|APPEND);
		break;
	default:			/* bad mode */
		return NULL;
		}
	if (fd == -1)
		return NULL;
	if (binmode)
		rwmode |= BINARY;
	fp->_pch = fp->_buf;	/* remember an earlier buffer if any */
	fp->_flag |= rwmode;
	fp->_cnt = (rwmode & WRITE)? MAXINT: 0;
	fp->_fd = fd;
	return fp;
	}
/* Close buffered file */
fclose(fp)
register FILE *fp;
	{
	if (fp->_fd == LSTOUT) {	/* close the printer */
		fp->_flag &= BUF;	/* remember if buffer allocated */
		return 0;
		}
	if (fp->_flag & WRITE		/* if it's an output file */
	   && !(fp->_flag & APPEND)) {	/* not just opened for append */
		if (!(fp->_flag & BINARY)) /* if not binary I/O */
			putc(CPMEOF, fp); /* write a ^Z */
		fflush(fp);		/* write last buffer */
		}
	fp->_flag &= BUF;		/* remember if buffer allocated */
	return close(fp->_fd);		/* free file descriptor */
	}
/* Get string from buffered file */
char *fgets(s, maxsize, fp)
char *s;
FILE *fp;
	{
	register int c, size;
	register char *ps, *q;

	ps = s;
	if (fp == CONIN) {		/* use CP/M func #10 for console */
		*ps = maxsize;		/* set buffer size for CP/M */
		bdos1(CONBUF, ps);	/* get the line from console */
		bdos1(CONOUT, '\n');	/* echo LF after CR */
		q = ps + 2;		/* beginning of line */
		size = ps[1];		/* find out how long line is */
		while (size--)		/* move line to start of buffer */
			*ps++ = *q++;
		*ps = '\n';		/* append newline */
		*(ps + 1) = '\0';	/* terminate line */
		return (*s == CPMEOF) ? NULL: s;
		}
	while (--maxsize > 0 && (c = getc(fp)) != EOF)
		if ((*ps++ = c) == '\n')
			break;
	*ps = '\0';
	return ((c == EOF && ps == s) ? NULL : s);
	}
/* Put string to buffered file */
fputs(s, fp)
char *s;
FILE *fp;
	{
	register int c;

	while (c = *s++)
		putc(c, fp);
	}
/* Read data items from a file */
fread(p, size, nitems, fp)
char *p;
FILE *fp;
	{
	register i, cnt;

	for (cnt = 0; cnt < nitems; ++cnt)
		for (i = 0; i < size; ++i)
			if ((*p++ = getc(fp)) == EOF)
				return (i == 0)? cnt: 0;
	return cnt;
	}
/* Write data items to a file */
fwrite(p, size, nitems, fp)
char *p;
FILE *fp;
	{
	register i, cnt;

	for (cnt = 0; cnt < nitems; ++cnt) {
		for (i = 0; i < size; ++i) {
			putc(*p++, fp);
			if (ferror(fp))
				return 0;
			}
		}
	return cnt;
	}
/* Read a word from a file */
getw(fp)
FILE *fp;
	{
	register unsigned low, high;

	if ((low=getc(fp)) == EOF || (high=getc(fp)) == EOF)
		return EOF;
	else
		return ((high << 8) | low);
	}
/* Write a word to a file */
putw(word, fp)
unsigned word;
FILE *fp;
	{
	if (putc(word & 0xFF, fp) == EOF || putc(word >> 8, fp) == EOF)
		return EOF;
	else
		return (word);
	}
/* Get character from buffered file */
getc(fp)
register FILE *fp;
	{
	register c;

	if (fp == CONIN) {		/* input from console? */
		if ((c = _ungch) != EOF) /* unget char ready? */
			_ungch = EOF;	/* use it and clear */
		else
			if ((c = bdos1(CONIN, 0)) == '\r') /* chg CR */
				bdos1(CONOUT, c = '\n'); /* to newline */
		return (c == CPMEOF) ? EOF: c;
		}
	if (fp->_cnt > 0) {
		c = (*fp->_pch++ & 0xFF); /* defeat sign extension */
		--fp->_cnt;
		}
	else
		c =  _fill(fp);
	if (fp->_flag & BINARY) 	/* no tampering if binary I/O */
		return c;
	if (c == '\r')			/* throw away CR */
		c = getc(fp);
	if (c == CPMEOF) {
		fp->_cnt = 0;
		fp->_flag |= FEOF;
		return EOF;
		}
	return ((c == EOF)? EOF: c & 0x7F);
	}
/* Internal routine to load the buffer for getc */
_fill(fp)
register FILE *fp;	 /* pointer to iob for file */
	{
	register cnt;

	if (fp->_flag & FEOF)
		return EOF;
	cnt = (_chkbuf(fp) == -1) ? -1:
		read(fp->_fd, fp->_buf, fp->_bufsize);
	if (cnt <= 0) { 		/* did we read anything? */
		fp->_flag |= ((cnt==0)? FEOF: FERR);
		return EOF;
		}
	fp->_cnt = cnt - 1;		/* # chars remaining */
	fp->_pch = fp->_buf + 1;	/* point to 2nd char in buffer */
	return (*fp->_buf & 0xFF);	/* return 1st char in buffer */
	}
/* Put character to buffered file */
putc(c, fp)
register c;
register FILE *fp;
	{
	if (fp == CONOUT) {		/* to console? */
		if (c == '\n')
			bdos1(CONOUT, '\r');
		bdos1(CONOUT, c);
		return c;
		}
	if (c == '\n' && !(fp->_flag & BINARY)) /* normal I/O only */
		if (putc('\r', fp) == EOF)
			return EOF;
	if (fp->_fd == LSTOUT) {	/* to printer? */
		bdos1(LSTOUT, c);
		return c;
		}
	if (fp->_cnt >= fp->_bufsize) { /* buffer full? */
		if (fflush(fp) == EOF)
			return EOF;
		}
	++fp->_cnt;
	return (*fp->_pch++ = c);
	}
/* Write file buffer */
fflush(fp)
register FILE *fp;
	{
	register nwrite, cnt, fd;
	register char *buf;
	struct fcb *_gfcb();

	fd = fp->_fd;
	if (fd == LSTOUT)		/* to printer? */
		return 0;
	if (!(fp->_flag & WRITE)	/* not open for output? */
	   || _chkbuf(fp) == -1)	/* can't get buffer? */
		fp->_flag |= FERR;
	if (fp->_flag & FERR)
		return EOF;
	if (fp->_flag & APPEND) {	/* position file at CP/M EOF */
		seekr(fd, -1, 2);
		bdos1(SETDMA, buf=fp->_buf);
		bdos1(READR, &((_gfcb(fd))->cpm_fcb));
		bdos1(SETDMA, DBUFF);
		for (cnt = 0; cnt < SECSIZE; ++cnt)
			if (buf[cnt] == CPMEOF)
				break;
		fp->_cnt = cnt; 	/* bytes up to CP/M EOF */
		fp->_pch = buf + cnt;	/* next byte to be written */
		fp->_flag &= ~APPEND;
		return 0;
		}
	if (fp->_cnt == MAXINT) {	/* first time thru */
		fp->_cnt = 0;
		return 0;
		}
	nwrite = ((fp->_cnt + (SECSIZE-1)) / SECSIZE) * SECSIZE;
	fp->_cnt = 0;			/* buffer is empty */
	if (write(fd, fp->_pch=fp->_buf, nwrite) < nwrite) {
		fp->_flag |= FERR;
		return EOF;
		}
	return 0;
	}
_chkbuf(fp)
register FILE *fp;
	{
	char *sbrk();
	if (!(fp->_flag & (BUF|USERBUF))) { /* buffer allocated? */
		fp->_bufsize = BUFSIZE;
		if ((int)(fp->_pch=fp->_buf=sbrk(BUFSIZE)) == -1) {
			fp->_flag |= FERR;
			return -1;
			}
		else
			fp->_flag |= BUF;
		}
	return 0;
	}
/* Push character back onto buffered file */
ungetc(c, fp)
register FILE *fp;
	{
	if (fp == CONIN) {		/* stdin */
		if (_ungch != EOF)
			return EOF;	/* only one pushback */
		else
			return (_ungch = c);
		}
	if ((fp->_flag & READ)		/* must be input */
	   && fp->_pch > fp->_buf) {	/* with room to unget */
		++fp->_cnt;
		return (*--fp->_pch = c);
		}
	else
		return EOF;
	}
/* User-provided file buffer */
setbuf(fp, buffer)
FILE *fp;
char *buffer;
	{
	if ((fp->_flag & (BUF|USERBUF)) /* buffer already allocated? */
	    || buffer == NULL)		/* no buffer supplied? */
		return;
	fp->_bufsize = BUFSIZE;
	fp->_pch = fp->_buf = buffer;
	fp->_flag |= USERBUF;
	}
/* Set size of user-provided buffer */
setbsize(fp, size)
FILE *fp;
	{
	if (fp->_flag & BUF)	/* system buffer allocated? */
		return;
	else if (size /= SECSIZE)
		fp->_bufsize = size * SECSIZE;
	else			/* can't use less than one sector */
		fp->_flag &= ~USERBUF;
	}
/* Check for end-of-file */
feof(fp)
FILE *fp;
	{
	return (fp->_flag & FEOF);
	}
/* Check file error status */
ferror(fp)
FILE *fp;
	{
	return (fp->_flag & FERR);
	}
/* Clear file error status */
clearerr(fp)
FILE *fp;
	{
	fp->_flag &= ~FERR;
	}
/* Get file descriptor number */
fileno(fp)
FILE *fp;
	{
	return (fp->_fd);
	}
/* Delete CP/M file */
unlink(filename)
char *filename;
	{
	struct cpmfcb cpmfcb;

	if (makfcb(filename, &cpmfcb) != -1)
		return (bdos1(DELETEF, &cpmfcb) == CPMERR) ? -1: 0;
	return -1;
	}
/* Open CP/M file */
open(filename, rwmode)
char *filename;
register rwmode;
	{
	register struct fcb *fcb;
	register struct cpmfcb *cpmfcb;
	struct fcb *_gfcb();
	register int fd;

	if (rwmode < 0 || rwmode > 2)
		return -1;
	fd = _gfd();			/* get a file descriptor */
	if ((fcb=_gfcb(fd)) == NULL)	/* get file control blk for fd */
		return -1;
	cpmfcb = &fcb->cpm_fcb;
	if (makfcb(filename, cpmfcb) == -1
	   || bdos1(OPENF, cpmfcb) == CPMERR)
		return -1;
	fcb->flag = rwmode + 1; 	/* flag: R=01, W=02, R/W=03 */
	bdos1(GTFSIZE, cpmfcb); 	/* get file size */
	fcb->file_size = cpmfcb->rand_rec;
	cpmfcb->rand_rec = 0;
	return fd;
	}
/* Create CP/M file */
creat(filename, pmode) /* p[rotection] mode - ignored by CP/M */
char *filename;
	{
	register struct fcb *fcb;
	register struct cpmfcb *cpmfcb;
	struct fcb *_gfcb();
	register int fd;

	fd = _gfd();			/* get a file descriptor */
	if ((fcb=_gfcb(fd)) == NULL)	/* get file control blk for fd */
		return -1;
	cpmfcb = &fcb->cpm_fcb;
	if (makfcb(filename, cpmfcb) == -1)
		return -1;		/* bad filename */
	bdos1(DELETEF, cpmfcb); 	/* delete any existing file */
	if (bdos1(MAKEF, cpmfcb) == CPMERR)
		return -1;		/* CP/M can't create filename */
	fcb->flag = WRITE;
	fcb->file_size = 0;
	return fd;
	}
/* Read CP/M logical records */
read(fd, buf, n)
register n;
char *buf;
	{
	register int i, nread;
	register struct fcb *fcb;
	register struct cpmfcb *cpmfcb;
	struct fcb *_gfcb();

	if (getkey() == CTLC)		/* abort run if ^C was typed */
		_exit();
	if ((fcb = _gfcb(fd)) == NULL	/* fd OK? */
	   || (!(fcb->flag & READ))	/* open for input? */
	   || (n % SECSIZE))		/* multiple of CP/M sector? */
		return -1;
	if (fcb->flag & (FEOF|FERR))
		return 0;		/* nothing read - at EOF */
	n /= SECSIZE;			/* convert to CP/M sectors */
	cpmfcb = &fcb->cpm_fcb;
	for (nread = 0; nread < n; ++nread) {
		bdos1(SETDMA, buf);	/* set DMA to our buffer */
		switch (bdos1(READR, cpmfcb)) {
		case 1: 		/* unwritten data */
		case 4: 		/* chk for EOF */
			if (cpmfcb->rand_rec >= fcb->file_size)
				fcb->flag |= FEOF;
			else for (i=0; i<SECSIZE; ++i)
				*buf++ = 0;
			break;
		default:		/* misc errors */
			fcb->flag |= FERR;
			break;
		case 0:
			buf += SECSIZE; /* next sector in our buffer */
			break;
			}
		if (fcb->flag & (FEOF|FERR))
			break;
		++cpmfcb->rand_rec;
		}
	bdos1(SETDMA, DBUFF);		/* reset CP/M default buffer */
	return (nread * SECSIZE);	/* convert to bytes */
	}
/* Write CP/M logical records */
write(fd, buf, n)
register n;
char *buf;
	{
	register int nwritten;
	register struct fcb *fcb;
	register struct cpmfcb *cpmfcb;
	struct fcb *_gfcb();

	if (getkey() == CTLC)		/* abort run if ^C was typed */
		_exit();
	if ((fcb = _gfcb(fd)) == NULL	/* fd OK? */
	   || (!(fcb->flag & WRITE))	/* open for output? */
	   || n % SECSIZE		/* multiple of CP/M sector? */
	   || fcb->flag & FERR) 	/* previous error? */
		return -1;
	n /= SECSIZE;			/* convert to CP/M sectors */
	cpmfcb = &fcb->cpm_fcb;
	for (nwritten = 0; nwritten < n; ++nwritten) {
		bdos1(SETDMA, buf);	/* set DMA to our buffer */
		if (bdos1(WRITERZ, cpmfcb)) { /* check for error */
			fcb->flag |= FERR;
			break;
			}
		++cpmfcb->rand_rec;
		buf += SECSIZE; 	/* next sector in our buffer */
		}
	if (cpmfcb->rand_rec > fcb->file_size)
		fcb->file_size = cpmfcb->rand_rec;
	bdos1(SETDMA, DBUFF);		/* reset CP/M default buffer */
	return (nwritten * SECSIZE);	/* convert to bytes */
	}
/* Close CP/M file */
close(fd)
	{
	register struct fcb *fcb;
	struct fcb *_gfcb();
	register fcbflag;

	if ((fcb = _gfcb(fd)) == NULL)
		return -1;
	fcbflag = fcb->flag;		/* save fcb flag */
	fcb->flag = 0;			/* mark fcb closed */
	if (fcbflag & FERR)		/* has error occurred? */
		return -1;
	if (fcbflag & WRITE)		/* was file open for write? */
		if (bdos1(CLOSEF, &fcb->cpm_fcb) == CPMERR)
			return -1;	/* CP/M can't close */
	return 0;
	}
/* Position file for random I/O */
seekr(fd, offset, mode)
unsigned offset;
	{
	struct fcb *_gfcb(), *fcb;
	register struct cpmfcb *cpmfcb;

	if ((fcb = _gfcb(fd)) == NULL)
		return -1;
	cpmfcb = &fcb->cpm_fcb;
	switch (mode) {
	case 2: 			/* from EOF */
		cpmfcb->rand_rec = fcb->file_size;
	case 1: 			/* from current rec */
		cpmfcb->rand_rec += offset;
		break;
	case 0: 			/* from beginning of file */
		cpmfcb->rand_rec = offset;
		break;
	default:
		return -1;
		}
	fcb->flag &= ~FEOF;		/* if we saw EOF, forget it */
	return 0;
	}
/* Report current record number in file */
unsigned tellr(fd)
	{
	struct fcb *fcb, *_gfcb();

	if ((fcb = _gfcb(fd)) == NULL)
		return 0;
	else
		return (fcb->cpm_fcb.rand_rec);
	}
/* Internal routine to get next available file descriptor */
_gfd()
	{
	register fd;
	struct fcb *_gfcb(), *fcb;

	fd = 6; 	/* skip stdin, stdout, stderr, CP/M LST: */
	while ((fcb = _gfcb(fd)) != NULL) {
		if (fcb->flag == 0)
			return fd;	/* fcb not in use */
		++fd;
		}
	return -1;			/* all fcb's in use */
	}
/* Internal routine - get file control block for file descriptor fd */
struct fcb *_gfcb(fd)
register fd;
	{
	char *sbrk();
	register struct fcb *fcb;

	fd -= 6;	/* skip stdin, stdout, stderr, CP/M LST: */
	if (fd >= 0 && fd < NFILES) {
		if ((fcb = _fcb[fd]) == NULL) { /* need space? */
			fcb = (struct fcb *) sbrk(sizeof(struct fcb));
			if ((int)fcb == -1)
				return NULL;
			else {
				fcb->flag = 0;
				_fcb[fd] = fcb;
				}
			}
		return fcb;
		}
	return NULL;
	}
/* Build a CP/M file control block (fcb) */
#ifdef PORTABLE
makfcb(filename, fcb)
register char *filename, *fcb;
	{
	register int i, c, padchar;

	if (filename[1] != ':') 	/* check for drive designator */
		*fcb++ = 0;		/* default drive */
	else {
		if (!isalpha(*filename)) /* drive must be alpha */
			return -1;
		*fcb++ = chupper(*filename) - ('A' - 1);
		filename += 2;		/* move past ':' */
		}
	padchar = ' ';		/* pad character for short names */
	for (i=1; i<=8; ++i) {		/* copy file name to fcb */
		if ((c = chupper(*filename++)) == '\0' || c == '.')
			break;		/* end of file name */
		if (iscntrl(c)) 	/* no control chars */
			return -1;
		if (c == '*') { 	/* wild card character? */
			padchar = '?';	/* fill rest of name with ? */
			break;
			}
		*fcb++ = c;
		}
	for (; i<=8; ++i)		/* pad short name */
		*fcb++ = padchar;
	if (c != 0 && c != '.') 	/* if not end of file name... */
		if ((c = chupper(*filename++)) != 0 && c != '.')
			return -1;	/* next char must be end */
	if (c)				/* if more chars, get 1st */
		c = chupper(*filename++); /* char of file extension */
	padchar = ' ';			/* reset pad character */
	for (; i<=11; ++i) {		/* copy file extension to fcb */
		if (c == '\0')
			break;
		if (iscntrl(c))
			return -1;
		if (c == '*') {
			padchar = '?';
			break;
			}
		*fcb++ = c;
		c = chupper(*filename++);
		}
	if ((c != 0 && i == 11) 	/* if full 11 char filename... */
		|| c == '*')		/* or ended with wild card char */
		c = chupper(*filename); /* see what next char is */
	if (c != 0)
		return -1;		/* file name too long */
	for (; i<=11; ++i)		/* pad short extension */
		*fcb++ = padchar;
	for (; i<CPMFCBSIZE; ++i)
		*fcb++ = 0;		/* fill rest of fcb with zero */
	return 0;
	}
#else

makfcb()
	{
#asm	8080
	PUSH	B	;save frame pointer
	LXI	H,7	;get args
	DAD	SP
	MOV	D,M	;filename
	DCX	H
	MOV	E,M
	DCX	H
	MOV	A,M	;fcb
	DCX	H
	MOV	L,M
	MOV	H,A
	INX	D
	LDAX	D	;2nd char of filename
	DCX	D
	CPI	58	;check for drive specifier (:)
	MVI	A,0	;preset logged drive
	JNZ	?FCB0
	LDAX	D	;get drive letter
	INX	D	;move past colon
	INX	D
	CALL	?FCB9	;convert to upper case
	SUI	64	;convert to drive number (A=1,B=2,etc)
	CPI	1
	JC	?FCB99	;<'A' no good
	CPI	17
	JNC	?FCB99	;>'P' no good
?FCB0:	MOV	M,A	;store drive number
	INX	H
	MVI	C,46	;end character can be '.'
	MVI	B,8	;max 8 characters for file name
	CALL	?FCB2	;put file name if fcb
	JC	?FCB99	;found error
	MVI	C,0	;only '\0' is valid end character
	MVI	B,3	;max 3 characters for file type
	CALL	?FCB2
	JC	?FCB99
	XRA	A	;fill rest of fcb with zeros
	MVI	B,24
?FCB1:	MOV	M,A
	INX	H
	DCR	B
	JNZ	?FCB1
	JMP	?FCB98	;normal end
;Put file name or file type in fcb
?FCB2:	CALL	?FCB4	;end of string?
	JNC	?FCB7
	CPI	42	;check for wildcard character '*'
	JNZ	?FCB3
	INX	D	;next character must be end (0 or .)
	CALL	?FCB4
	RC		;error
	MVI	C,63	;set pad character to '?'
	JMP	?FCB8
?FCB3:	CPI	32	;check for illegal characters (<' ')
	RC
	CPI	128	;'~'+1
	JNC	?FCB5
	CALL	?FCB9	;convert to upper case
	MOV	M,A
	INX	H
	INX	D
	DCR	B
	JNZ	?FCB2	;any more allowed?
;Check for end character and return carry set if not found
?FCB4:	LDAX	D
	ORA	A	;end of string?
	RZ
	CMP	C	;valid end character?
	JNZ	?FCB5
	INX	D
	RET
?FCB5:	STC
	RET
;Fill rest with pad character (' ' or '?')
?FCB7:	MVI	C,32
?FCB8:	MOV	M,C
	INX	H
	DCR	B
	JNZ	?FCB8
	RET
;Convert lower case to upper
?FCB9:	CPI	97	;'a'
	RC
	CPI	123	;'z'+1
	RNC
	SUI	32
	RET
;Good return
?FCB98: XRA	A
	MOV	L,A	;return value must not be -1
	MOV	H,A
	POP	B	;restore frame pointer
	RET
;Error return
?FCB99: LXI	H,-1	;return value is -1
	ORA	L
	POP	B
#endasm
	}
#endif
/* If a key was pressed, get the character */
getkey()
	{
	return (bdos1(CONSTAT, 0) & 01) ? bdos1(CONIN, 0): EOF;
	}
/*
 * Storage allocator based on Kernighan & Ritchie model but
 * simplified since there are no alignment requirements on
 * 8080/Z80 and obtaining free space under CP/M is cheap
 */
char *calloc(nelements, size)
unsigned nelements, size;
	{
	register char *malloc(), *p, *q;

	size *= nelements;
	if ((q = p = malloc(size)) != NULL) {
		while (size--)
			*q++ = 0;
		}
	return p;
	}
char *malloc(nbytes)
unsigned nbytes;
	{
	register HEADER *p, *q;
	register unsigned nunits;
	char *sbrk();

	nunits = 1 + (nbytes + (sizeof(HEADER) - 1)) / sizeof(HEADER);
	if ((q = _allocp) == NULL)	/* no free list */
		_base._next = _allocp = q = &_base;
	for (p = q->_next; ; q = p, p = p->_next) {
		if (p->_bsize >= nunits) {
			if (p->_bsize == nunits)
				q->_next = p->_next;
			else {
				p->_bsize -= nunits;
				p += p->_bsize;
				p->_bsize = nunits;
				}
			_allocp = q;
			return ((char *)(p+1));
			}
		if (p == _allocp) {	/* wrapped around */
			p = (HEADER *) sbrk(nunits * sizeof(HEADER));
			if ((int) p == -1)
				return NULL;
			p->_bsize = nunits;
			free((char *)(p+1)); /* add to free list */
			p = _allocp;
			}
		}
	}
free(pblk)
char *pblk;
	{
	register HEADER *p, *q;

	p = (HEADER *)pblk - 1; 	/* point to block header */
	for (q=_allocp; !(p>q && p<q->_next); q=q->_next)
		if (q >= q->_next && (p>q || p<q->_next))
			break;
	if (p+p->_bsize == q->_next) {	/* join to upper neighbor */
		p->_bsize += q->_next->_bsize;
		p->_next = q->_next->_next;
		}
	else
		p->_next = q->_next;
	if (q+q->_bsize == p) { 	/* join to lower neighbor */
		q->_bsize += p->_bsize;
		q->_next = p->_next;
		}
	else
		q->_next = p;
	_allocp = q;
	}
char *sbrk(size)
register unsigned size;
	{
	unsigned maxsbrk();

	if (maxsbrk() < size)
		return -1;	/* can't go into moat */
	_free += size;		/* move free space ptr */
	return (_free - size);
	}
/* Change size of moat between free space and stack */
unsigned moat(size)
unsigned size;
	{
	unsigned oldsize;

	oldsize = _moat;
	_moat = size;
	return oldsize;
	}
/* Compute memory left to be allocated */
unsigned maxsbrk()
	{
	auto char sp[1];	/* sp is on the stack , so sp = SP */
	return ((sp>_free+_moat) ? sp-(_free+_moat): 0);
	}
/* end of CDISKLIB.C */

close(fd)
	{
	register struct fcb *fcb;
	struct fcb *_gfcb();
	regi