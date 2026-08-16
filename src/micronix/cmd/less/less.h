/*
 * Standard include file for "less".
 *
 * cmd/less/less.h
 *
 * THE PORT'S CONFIGURATION IS HERE, not in the makefile.  The
 * original took a dozen -D flags from a makefile its install script
 * generated; this tree's makefiles are uniform and say nothing about
 * any one program, so the choices live where they can be read:
 *
 *	VOID 1		ccc has void
 *	off_t		micronix has no off_t; lseek takes and
 *			returns long
 *	TERMIO 0	and no sgtty either - screen.c is rewritten
 *			for the micronix tty driver and an ANSI
 *			terminal, and reads neither flag
 *	SIGSETMASK 0	no sigsetmask, and no job control to want it
 *	REGCMP 0	no regcmp in libc
 *	RECOMP 0	no re_comp either - both 0 selects the
 *			built-in plain substring match
 *	SHELL_ESCAPE 1	libc has system()
 *	EDITOR 1	and the tree has vi
 *	GLOB 0		no popen
 *	LOGFILE 0	the -l machinery is more code than the
 *			machine has room to spend on it
 *	ONLY_RETURN 0	any key continues past an error
 *	XENIX 0
 *	HELPFILE	/usr/lib/less.help, which install ships
 */
#define	VOID		1
#define	off_t		long
#define	TERMIO		0
#define	SIGSETMASK	0
#define	REGCMP		0
#define	RECOMP		0
#define	SHELL_ESCAPE	1
#define	EDITOR		1
#define	EDIT_PGM	"vi"
#define	GLOB		0
#define	LOGFILE		0
#define	ONLY_RETURN	0
#define	XENIX		0
#define	HELPFILE	"/usr/lib/less.help"

/*
 * Language details.
 */
#if !VOID
#define	void  int
#endif
#define	public		/* PUBLIC FUNCTION */

/*
 * Special types and constants.
 */
typedef long		POSITION;
/*
 * {{ Warning: if POSITION is changed to other than "long",
 *    you may have to change some of the printfs which use "%ld"
 *    to print a variable of type POSITION. }}
 */

#define	NULL_POSITION	((POSITION)(-1))

#define	EOF		(0)
#define	NULL		(0)

/* How quiet should we be? */
#define	NOT_QUIET	0	/* Ring bell at eof and for errors */
#define	LITTLE_QUIET	1	/* Ring bell only for errors */
#define	VERY_QUIET	2	/* Never ring bell */

/* How should we prompt? */
#define	PR_SHORT	0	/* Prompt with colon */
#define	PR_MEDIUM	1	/* Prompt with message */
#define	PR_LONG		2	/* Prompt with longer message */

/* How should we handle backspaces? */
#define	BS_SPECIAL	0	/* Do special things for underlining and bold */
#define	BS_NORMAL	1	/* \b treated as normal char; actually output */
#define	BS_CONTROL	2	/* \b treated as control char; prints as ^H */

/* Special chars used to tell put_line() to do something special */
#define	UL_CHAR		'\201'	/* Enter underline mode */
#define	UE_CHAR		'\202'	/* Exit underline mode */
#define	BO_CHAR		'\203'	/* Enter boldface mode */
#define	BE_CHAR		'\204'	/* Exit boldface mode */

#define	CONTROL(c)		((c)&037)
#define	SIGNAL(sig,func)	signal(sig,func)

/* Library function declarations */
off_t lseek();

#include "funcs.h"
