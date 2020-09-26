/************************************************
 *						*
 * Q/C V3.1 Disk I/O Library			*
 * Copyright (c) 1983 Quality Computer Systems	*
 *						*
 * 07/21/83					*
 *						*
 * cp/m specific stuff				*
 *						*
 ************************************************/

#include "qstdio.h"

/* Library parameters */

#define MAXARG	20		/* max no. of command line arguments */
#define MAXLINE 81		/* maximum size of console input line */
#define NFILES	10		/* max no. of files that can be open */
#define NSECTS	4		/* no. of CP/M records to buffer */
#define SECSIZE 128		/* size of CP/M record */
#define BUFSIZE (NSECTS * SECSIZE)	/* buffer size for buffered I/O */

/* Miscellaneous CP/M constants */

#define CPMFCBSIZE 36
#define DBUFF	0x80
#define CPMERR	0xFF
#define CPMEOF	0x1A
#define CTLC	0x3		/* ^C */

/* CP/M function numbers */

#define CONIN	1
#define CONOUT	2
#define LSTOUT	5
#define PRINTS	9
#define CONBUF	10
#define OPENF	15
#define CLOSEF	16
#define DELETEF 19
#define READF	20
#define WRITEF	21
#define MAKEF	22
#define SETDMA	26

/*
 * I/O control blocks (iob's) for buffered files struct _iob { char _flag;
 * tatus flag char	*_pch;		ptr to curr char in buffer int	_cnt;
 * o. of chars remaining in buffer char *_buf;		address of buffer int
 * _bufsize;	size of buffer char	_fd;		file descriptor (fd)
 * for low-level I/O };
 */

/* I/O flag masks */

#define READ	01
#define WRITE	02
#define BINARY	04
#define BUF	010
#define USERBUF 020
#define FEOF	040
#define FERR	0100

/* File control block for low-level I/O */

#define FCBSIZE (CPMFCBSIZE + 1)/* size of a Q/C file control block */
struct fcb {
    char	    flag;
    char	    cpmfcb [CPMFCBSIZE];
}              *_fcb[NFILES];	/* file control block pointers */

FILE           *stdin = CONIN,	/* default to console */
               *stdout = CONOUT, *stderr = CONOUT, _iob[NFILES];	/* I/O blocks (iob) for
									 * buffered I/O */
int		_ungch = EOF;	/* hold unget character for stdin */
extern char    *_free;		/* free space address */
unsigned	_moat = 1000;	/* moat between free space and stack */

/* Parse command line and set standard files (does redirection) */
_rshell()
{
    static	    argc, argv[MAXARG], argflag = 0,	/* not in an argument */
    		    cmdcnt   , c;
    static char
                   *cmdline = 0x80,	/* CP/M command line at 80H */
                   *pargs, *ptr;
    char	    args   [128], infile[15], outfile[15];
    FILE           *fopen();

    cmdcnt = *cmdline++;	/* first byte is length */
    pargs = args;		/* point to args */
    *pargs =			/* set command = null string */
	infile[0] =		/* no redirection of standard files */
	outfile[0] = '\0';
    argv[argc++] = pargs;	/* record pointer to 1st arg */
    while (cmdcnt--) {
	c = *cmdline++;
	switch (argflag) {
	case 0:		/* not in an arg */
	    if (isspace(c))
		continue;	/* still not in an arg */
	    if (argc == MAXARG) {
		fputs("Too many command line args\n", stderr);
		exit(1);
	    }
	    argflag = c;	/* set type of arg */
	    if (c == '<')	/* redirect stdin */
		ptr = infile;
	    else if (c == '>')	/* redirect stdout */
		ptr = outfile;
	    else {		/* just an arg */
		argv[argc++] = pargs + 1;
		if (c != '"')	/* if not a string */
		    break;	/* record char */
	    }
	    continue;		/* skip this character */
	case '<':		/* record redirection */
	case '>':		/* file name */
	    if (isspace(c))
		c = argflag = 0;/* end of name */
	    *ptr++ = c;
	    continue;
	case '"':		/* in a string */
	    if (c == '"')
		c = argflag = 0;/* end of string */
	    break;
	default:
	    if (isspace(c))
		c = argflag = 0;/* end of arg */
	    break;
	}
	*++pargs = c;
    }
    if (argflag == '<' || argflag == '>')	/* end last arg */
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
    static	    argc, argv[MAXARG], argflag = 0,	/* not in an argument */
    		    cmdcnt   , c;
    static char
                   *cmdline = 0x80,	/* CP/M command line at 80H */
                   *pargs, *ptr;
    char	    args   [128];

    cmdcnt = *cmdline++;	/* first byte is length */
    pargs = args;		/* point to args */
    *pargs = '\0';		/* set command = null string */
    argv[argc++] = pargs;	/* record pointer to 1st arg */
    while (cmdcnt--) {
	c = *cmdline++;
	switch (argflag) {
	case 0:		/* not in an arg */
	    if (isspace(c))
		continue;	/* still not in an arg */
	    if (argc == MAXARG) {
		bdos(PRINTS, "Too many command line args\r\n$");
		exit(0);
	    }
	    argflag = c;	/* set type of arg */
	    argv[argc++] = pargs + 1;
	    if (c != '"')	/* if not a string */
		break;		/* record char */
	    continue;		/* skip this character */
	case '"':		/* in a string */
	    if (c == '"')
		c = argflag = 0;/* end of string */
	    break;
	default:
	    if (isspace(c))
		c = argflag = 0;/* end of arg */
	    break;
	}
	*++pargs = c;
    }
    *++pargs = '\0';		/* end last arg */
    main(argc, argv);
    exit(0);
}
cantopen(filename)
    char           *filename;
{
    fputs("Can't open: ", stderr);
    fputs(filename, stderr);
    exit(1);
}
/* Cleanup routine - close files, etc. */
exit(error)
    int		    error;
{
    register FILE  *fp;
    register	    n;

    for (fp = _iob, n = 0; n < NFILES; ++fp, ++n)
	fclose(fp);		/* close any output files */
    if (error)
	unlink("A:$$$.SUB");	/* era submit file, if any */
    exit(0);			/* reboot CP/M */
}
/* Immediate exit to CP/M with no cleanup */
_exit()
{
    bdos(0, 0);
}
/* Get a string from stdin */
char           *
gets(s)
    char           *s;
{
    char           *fgets(), *index(), *p;

    if (fgets(s, MAXLINE, stdin) == NULL)
	return NULL;
    if (p = index(s, '\n'))	/* strip newline if present */
	*p = '\0';
    return s;
}
/* Put a string to stdout */
puts(s)
    char           *s;
{
    while (*s)
	putchar(*s++);
    putchar('\n');		/* always append newline */
}
/* Formatted print routine */
printf(nargs)
    int		    nargs;
{
    int		    putc   ();	/* function _fmt will use as output */
    _fmt(putc, stdout, &nargs + nargs);
}
/* Write formatted output to a file */
fprintf(nargs)
    int		    nargs;
{
    int		    putc   ();	/* function _fmt will use as output */
    register       *p;

    p = &nargs + nargs;		/* address of first arg (file ptr) */
    _fmt(putc, *p, p - 1);	/* _fmt does the formatting */
}
/* Get character from standard input file */
getchar()
{
    return getc(stdin);
}
/* Put character to standard output file */
putchar(c)
    int		    c;
{
    return putc(c, stdout);
}
/* Open buffered file */
FILE           *
fopen(filename, mode)
    char           *filename, *mode;
{
    register FILE  *fp;
    register	    n , fd, rwmode;
    char           *_upper();

    for (fp = _iob, n = 0; n < NFILES; ++fp, ++n)
	if (!(fp->_flag & (READ | WRITE)))
	    break;		/* found empty iob */
    if (n >= NFILES)
	return NULL;		/* no empty iob's */
    switch (chupper(*mode)) {
    case 'R':
	rwmode = READ;
	fd = open(filename, 0);
	break;
    case 'W':
	rwmode = WRITE;
	if (strcmp(_upper(filename), "LST:") == 0)
	    fd = LSTOUT;
	else
	    fd = creat(filename, 0644);	/* 0644 = UNIX mode */
	break;
    default:			/* bad mode */
	return NULL;
    }
    if (fd == ERROR)
	return NULL;
    if (chupper(*(mode + 1)) == 'B')	/* binary I/O? */
	rwmode |= BINARY;
    fp->_pch = fp->_buf;
    fp->_flag |= rwmode;
    fp->_cnt = 0;
    fp->_bufsize = BUFSIZE;
    fp->_fd = fd;
    return fp;
}
/* Close buffered file */
fclose(fp)
    register FILE  *fp;
{
    if (fp->_fd == LSTOUT) {	/* close the printer */
	fp->_flag &= BUF;	/* remember if buffer allocated */
	return 0;
    }
    if (fp->_flag & WRITE) {	/* if it's an output file... */
	if (!(fp->_flag & BINARY))	/* not binary I/O? */
	    putc(CPMEOF, fp);	/* write a ^Z */
	fflush(fp);		/* write last buffer */
    }
    fp->_flag &= BUF;		/* remember if buffer allocated */
    return close(fp->_fd);	/* free file descriptor */
}
/* Get string from buffered file */
char           *
fgets(s, maxsize, fp)
    char           *s;
    int		    maxsize;
    FILE           *fp;
{
    register int    c, size;
    register char  *ps, *q;

    ps = s;
    if (fp == CONIN) {		/* use CP/M func #10 for console */
	*ps = maxsize;		/* set buffer size for CP/M */
	bdos(CONBUF, ps);	/* get the line from console */
	bdos(CONOUT, '\n');	/* echo LF after CR */
	q = ps + 2;		/* beginning of line */
	size = ps[1];		/* find out how long line is */
	while (size--)		/* move line to start of buffer */
	    *ps++ = *q++;
	*ps = '\n';		/* append newline */
	*(ps + 1) = '\0';	/* terminate line */
	return (*s == CPMEOF) ? NULL : s;
    }
    while (--maxsize > 0 && (c = getc(fp)) != EOF)
	if ((*ps++ = c) == '\n')
	    break;
    *ps = '\0';
    return ((c == EOF && ps == s) ? NULL : s);
}
/* Put string to buffered file */
fputs(s, fp)
    char           *s;
    FILE           *fp;
{
    register int    c;

    while (c = *s++)
	putc(c, fp);
}
/* Get character from buffered file */
getc(fp)
    register FILE  *fp;
{
    register int    c;

    if (fp == CONIN) {		/* input from console? */
	if ((c = _ungch) != EOF)/* unget char ready? */
	    _ungch = EOF;	/* use it and clear */
	else if ((c = bdos(CONIN, 0)) == '\r')	/* chg CR */
	    bdos(CONOUT, c = '\n');	/* to newline */
	return (c == CPMEOF) ? EOF : c;
    }
    if (!(fp->_flag & READ)	/* open for READ? */
	||(fp->_flag & (FEOF | FERR)))	/* EOF or error? */
	return EOF;
    c = (--fp->_cnt >= 0) ? (*fp->_pch++ & 0xFF) : _fill(fp);
    if (fp->_flag & BINARY)	/* no tampering if binary I/O */
	return c;
    if (c == '\r')		/* throw away CR */
	c = getc(fp);
    if (c == CPMEOF || c == EOF) {
	fp->_flag |= FEOF;
	return EOF;
    }
    return (c & 0x7F);		/* make c positive */
}
/* Internal routine to load the buffer for getc */
_fill(fp)
    register FILE  *fp;		/* pointer to iob for file */
{
    register	    cnt;
    char           *malloc();

    if (!(fp->_flag & (BUF | USERBUF))) {	/* buffer allocated? */
	if ((fp->_buf = malloc(BUFSIZE)) == NULL) {
	    fp->_flag |= FERR;
	    return EOF;
	} else
	    fp->_flag |= BUF;
    }
    cnt = read(fp->_fd, fp->_buf, fp->_bufsize);
    if (cnt <= 0) {		/* did we read anything? */
	if (cnt == 0)		/* read 0 means EOF */
	    fp->_flag |= FEOF;
	else			/* read -1 means error */
	    fp->_flag |= FERR;
	fp->_cnt = 0;
	return EOF;
    }
    fp->_cnt = cnt - 1;		/* # chars remaining */
    fp->_pch = fp->_buf + 1;	/* point to 2nd char in buffer */
    return (*fp->_buf & 0xFF);	/* return 1st char in buffer */
}
/* Push character back onto buffered file */
ungetc(c, fp)
    int		    c;
    register FILE  *fp;
{
    if (fp == CONIN) {		/* stdin */
	if (_ungch != EOF)
	    return EOF;		/* only one pushback */
	else
	    return (_ungch = c);
    }
    if ((fp->_flag & READ)	/* must be input */
	&&fp->_pch > fp->_buf) {/* with room to unget */
	++fp->_cnt;
	return (*--fp->_pch = c);
    } else
	return EOF;
}
/* Put character to buffered file */
putc(c, fp)
    register int    c;
    register FILE  *fp;
{
    char           *malloc();

    if (fp == CONOUT) {		/* to console? */
	if (c == '\n')
	    putc('\r', fp);	/* chg newline to CR/LF */
	bdos(CONOUT, c);
	return c;
    }
    if (!(fp->_flag & WRITE))	/* open for output? */
	return EOF;
    if (c == '\n' && !(fp->_flag & BINARY))	/* normal I/O only */
	if (putc('\r', fp) == EOF)
	    return EOF;
    if (fp->_fd == LSTOUT) {	/* to printer? */
	bdos(LSTOUT, c);
	return c;
    }
    if (!(fp->_flag & (BUF | USERBUF))) {	/* buffer allocated? */
	if ((fp->_pch = fp->_buf = malloc(BUFSIZE)) == NULL) {
	    fp->_flag |= FERR;
	    return EOF;
	} else
	    fp->_flag |= BUF;
    }
    if (fp->_cnt >= fp->_bufsize) {	/* buffer full? */
	if (fflush(fp) == EOF)
	    return EOF;
    }
    ++fp->_cnt;
    return (*fp->_pch++ = c);
}
/* Write file buffer */
fflush(fp)
    register FILE  *fp;
{
    register	    nwrite;

    if (fp->_fd == LSTOUT)	/* to printer? */
	return 0;
    if (!(fp->_flag & WRITE) || fp->_flag & FERR)
	return EOF;
    nwrite = ((fp->_cnt + (SECSIZE - 1)) / SECSIZE) * SECSIZE;
    fp->_cnt = 0;		/* buffer is empty */
    if (write(fp->_fd, fp->_pch = fp->_buf, nwrite) < nwrite) {
	fp->_flag |= FERR;
	return EOF;
    }
    return 0;
}
/* User-provided file buffer */
setbuf(fp, buffer)
    FILE           *fp;
    char           *buffer;
{
    if ((fp->_flag & (BUF | USERBUF))	/* buffer already allocated? */
	||buffer == NULL)	/* no buffer supplied? */
	return;
    fp->_pch = fp->_buf = buffer;
    fp->_flag |= USERBUF;
}
/* Set size of user-provided buffer */
setbsize(fp, size)
    FILE           *fp;
    int		    size;
{
    if (fp->_flag & BUF)	/* system buffer already allocated? */
	return;
    else if (size /= SECSIZE)
	fp->_bufsize = size * SECSIZE;
    else			/* can't use less than one sector */
	fp->_flag &= ~USERBUF;
}
/* Check for end-of-file */
feof(fp)
    FILE           *fp;
{
    return (fp->_flag & FEOF);
}
/* Check file error status */
ferror(fp)
    FILE           *fp;
{
    return (fp->_flag & FERR);
}
/* Clear file error status */
clearerr(fp)
    FILE           *fp;
{
    fp->_flag &= ~FERR;
}
/* Get file descriptor number */
fileno(fp)
    FILE           *fp;
{
    return (fp->_fd);
}
/* Delete CP/M file */
unlink(filename)
    char           *filename;
{
    char	    cpmfcb [CPMFCBSIZE];

    if (makfcb(filename, cpmfcb) != -1)
	return (bdos(DELETEF, cpmfcb) == CPMERR) ? -1 : 0;
    return -1;
}
/* Open CP/M file */
open(filename, rwmode)
    char           *filename;
    register	    rwmode;
{
    register struct fcb *fcb;
    struct fcb     *_gfcb();
    register int    fd;

    if (rwmode < 0 || rwmode > 1/* check mode */
	|| (fd = _gfd()) == NULL)	/* get next available fd */
	return -1;		/* bad mode or max files open */
    fcb = _gfcb(fd);		/* get fcb for file descriptor fd */
    if (makfcb(filename, fcb->cpmfcb) == -1
	|| bdos(OPENF, fcb->cpmfcb) == CPMERR)
	return -1;		/* bad name or CP/M can't open */
    fcb->flag = rwmode + 1;	/* flag: R=01, W=02 */
    return fd;
}
/* Create CP/M file */
creat(filename, pmode)
    char           *filename;
    int		    pmode;	/* protection mode - ignored by CP/M */
{
    register struct fcb *fcb;
    struct fcb     *_gfcb();
    register int    fd;

    if ((fd = _gfd()) == NULL)	/* get next available fd */
	return -1;		/* max # of files open already */
    fcb = _gfcb(fd);		/* get fcb for file descriptor fd */
    if (makfcb(filename, fcb->cpmfcb) == -1)
	return -1;		/* bad filename */
    bdos(DELETEF, fcb->cpmfcb);	/* delete any existing file */
    if (bdos(MAKEF, fcb->cpmfcb) == CPMERR)
	return -1;		/* CP/M can't create filename */
    fcb->flag = WRITE;
    return fd;
}
/* Read CP/M logical records */
read(fd, buf, n)
    int		    fd;
    register	    n;
    char           *buf;
{
    register int    nread;
    register struct fcb *fcb;
    struct fcb     *_gfcb();

    if (getkey() == CTLC)	/* abort run if ^C was typed */
	_exit();
    if ((fcb = _gfcb(fd)) == NULL	/* fd OK? */
	|| (!(fcb->flag & READ))/* open for input? */
	||(n % SECSIZE))	/* multiple of CP/M sector? */
	return -1;
    if (fcb->flag & FEOF)
	return 0;		/* nothing read - at EOF */
    n /= SECSIZE;		/* convert to CP/M sectors */
    for (nread = 0; nread < n; ++nread) {
	bdos(SETDMA, buf);	/* set DMA to our buffer */
	if (bdos(READF, fcb->cpmfcb)) {	/* EOF if != zero */
	    fcb->flag |= FEOF;
	    break;
	}
	buf += SECSIZE;		/* next sector in our buffer */
    }
    bdos(SETDMA, DBUFF);	/* reset CP/M default buffer */
    return (nread * SECSIZE);	/* convert to bytes */
}
/* Write CP/M logical records */
write(fd, buf, n)
    int		    fd;
    register	    n;
    char           *buf;
{
    register int    nwritten;
    register struct fcb *fcb;
    struct fcb     *_gfcb();

    if (getkey() == CTLC)	/* abort run if ^C was typed */
	_exit();
    if ((fcb = _gfcb(fd)) == NULL	/* fd OK? */
	|| (!(fcb->flag & WRITE))	/* open for output? */
	||n % SECSIZE		/* multiple of CP/M sector? */
	|| fcb->flag & FERR)	/* previous error? */
	return -1;
    n /= SECSIZE;		/* convert to CP/M sectors */
    for (nwritten = 0; nwritten < n; ++nwritten) {
	bdos(SETDMA, buf);	/* set DMA to our buffer */
	if (bdos(WRITEF, fcb->cpmfcb)) {	/* check for error */
	    fcb->flag |= FERR;
	    break;
	}
	buf += SECSIZE;		/* next sector in our buffer */
    }
    bdos(SETDMA, DBUFF);	/* reset CP/M default buffer */
    return (nwritten * SECSIZE);/* convert to bytes */
}
/* Close CP/M file */
close(fd)
    int		    fd;
{
    register struct fcb *fcb;
    struct fcb     *_gfcb();
    register	    fcbflag;

    if ((fcb = _gfcb(fd)) == NULL)	/* fd OK? */
	return -1;
    fcbflag = fcb->flag;	/* save fcb flag */
    fcb->flag = 0;		/* mark fcb closed */
    if (fcbflag & FERR)		/* has error occurred? */
	return -1;
    if (fcbflag & WRITE)	/* was file open for write? */
	if (bdos(CLOSEF, fcb->cpmfcb) == CPMERR)
	    return -1;		/* CP/M can't close */
    return 0;
}
/* Internal routine to get next available file descriptor */
_gfd()
{
    register	    fd;
    struct fcb     *_gfcb(), *fcb;

    fd = 6;			/* skip stdin, stdout, stderr, CP/M LST: */
    while ((fcb = _gfcb(fd)) != NULL) {
	if (fcb->flag == 0)
	    return fd;		/* fcb not in use */
	++fd;
    }
    return -1;			/* all fcb's in use */
}
/* Internal routine - get file control block for file descriptor fd */
struct fcb     *
_gfcb(fd)
    register int    fd;
{
    char           *malloc();
    register struct fcb *fcb;

    fd -= 6;			/* skip stdin, stdout, stderr, CP/M LST: */
    if (fd >= 0 && fd < NFILES) {
	if ((fcb = _fcb[fd]) == NULL) {	/* need space? */
	    fcb = _fcb[fd] = (struct fcb *)malloc(FCBSIZE);
	    if (fcb != NULL)
		fcb->flag = 0;
	}
	return fcb;
    }
    return NULL;
}
/* Build a CP/M file control block (fcb) */
makfcb(filename, fcb)
    register char  *filename, *fcb;
{
    register int    i, c, padchar;
    char           *_upper();

    filename = _upper(filename);/* convert to upper case */
    if (filename[1] != ':')	/* check for drive designator */
	*fcb++ = 0;		/* default drive */
    else {
	if (!isalpha(*filename))/* drive must be alpha */
	    return -1;
	*fcb++ = *filename - ('A' - 1);
	filename += 2;		/* skip drive and ':' */
    }
    padchar = ' ';		/* pad character for short names */
    for (i = 1; i <= 8; ++i) {	/* copy file name to fcb */
	if ((c = *filename++) == '\0' || c == '.')
	    break;		/* end of file name */
	if (iscntrl(c))		/* no control chars */
	    return -1;
	if (c == '*') {		/* wild card character? */
	    padchar = '?';	/* fill rest of name with ? */
	    break;
	}
	*fcb++ = c;
    }
    for (; i <= 8; ++i)		/* pad short name */
	*fcb++ = padchar;
    if (c != 0 && c != '.')	/* if not end of file name... */
	if ((c = *filename++) != 0 && c != '.')
	    return -1;		/* next char must be end */
    if (c)			/* if more chars, get 1st */
	c = *filename++;	/* char of file extension */
    padchar = ' ';		/* reset pad character */
    for (; i <= 11; ++i) {	/* copy file extension to fcb */
	if (c == '\0')
	    break;
	if (iscntrl(c))
	    return -1;
	if (c == '*') {
	    padchar = '?';
	    break;
	}
	*fcb++ = c;
	c = *filename++;
    }
    if ((c != 0 && i == 11)	/* if full 11 char filename... */
	||c == '*')		/* or ended with wild card char */
	c = *filename;		/* see what next char is */
    if (c != 0)
	return -1;		/* file name too long */
    for (; i <= 11; ++i)	/* pad short extension */
	*fcb++ = padchar;
    for (; i < CPMFCBSIZE; ++i)
	*fcb++ = 0;		/* fill rest of fcb with zero */
    return 0;
}
/* Internal routine to convert a string to upper case */
char           *
_upper(s)
    char           *s;
{
    register char  *p;

    p = s;
    while (*p = chupper(*p))
	++p;
    return s;
}
/* If a key was pressed, get the character */
getkey()
{
    return (bdos(11, 0) & 01) ? bdos(1, 0) : EOF;
}

/* Return a pointer to requested size of memory */
char           *
malloc(size)
    register unsigned size;
{
    unsigned	    maxsbrk();

    if (maxsbrk() < size)
	return NULL;		/* can't go into moat */
    _free += size;		/* move free space ptr */
    return (_free - size);	/* beginning of allocated space */
}

/* Change the size of the moat between free space and stack */
unsigned
moat(size)
    unsigned	    size;
{
    unsigned	    oldsize;

    oldsize = _moat;
    _moat = size;
    return oldsize;
}

/* See how much memory is left that can be allocated */
unsigned
maxsbrk()
{
    auto char	    sp[1];	/* sp is on the stack , so sp = SP */
    return (sp - _free - _moat);
}

/* end of CDISKLIB.C */
