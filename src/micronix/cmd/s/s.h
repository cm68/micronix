/*
 * s.h - macro definitions for the screen editor
 */

#define	NULL	0

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
