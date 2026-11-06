/*
 * s.h - macro definitions for the screen editor
 */

#define	NULL	0

/*
 * Functions the files share whose first parameter is a character, or
 * whose result is one.  The declaration makes the byte convention
 * visible across translation units; without it the caller would pass
 * the word convention and ld would refuse the link.
 */
int	operator(char, int, int);
unsigned char	isalnum(unsigned char);		/* lib.c */
unsigned char	isspace(unsigned char);
unsigned char	isdigit(unsigned char);
unsigned char	isprint(unsigned char);
unsigned char	isalpha(unsigned char);		/* libc */
unsigned char	islower(unsigned char);
unsigned char	isupper(unsigned char);
char	k_lastcmd(void);

#define CR		'\r'    /* sent by <return> key */
#define ctrl(x)		(x & 037)       /* control character 'x' */
#define ESCAPE		27      /* end-of-insertion character */
#define MAXTEXT		1000    /* maximum length of a line */
#define TAB_WIDTH	8       /* columns per tab character */

#define abs(x)		((x > 0) ? x : -(x))
#define max(x,y)	(x > y) ? x : y
#define min(x,y)	(x < y) ? x : y

/*
 * for an unknown command, ring the bell 
 */
/*
 * #define UNKNOWN putc(7, stderr) 
 */
#define	UNKNOWN		bell()

/*
 * malloc was defined to alloc here, which is what v7 called it.  This
 * library has malloc and no alloc, so every malloc() in s compiled to
 * a call to a function that does not exist.
 *
 * #define	malloc(x) alloc(x)
 */
#define	getchar() getc(stdin)

#define	FIXEDSCREEN

#ifdef FIXEDSCREEN
#define	nrows	24
#define ncols	80
#else
extern int nrows;
extern int ncols;
#endif

/* cross-file entry points */
int b_setcur(int, int);
int b_gets(int, char *);
int b_getcur(int *, int *);
int b_getmark(int *, int *);
int b_insert(int, char *);
int b_setline(int);
int b_delete(int, int);
int b_lineid(int);
int b_newcmd(int);
int s_savemsg(char *, int);
int s_errmsg(char *, int);
char * ckalloc(int);
int s_keyboard(int);
int scr_move(int, int);
int k_donext(char *);
