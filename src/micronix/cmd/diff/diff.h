/*
 * diff - common declarations
 *
 * cmd/diff/diff.h
 *
 * Ported from 2.11BSD usr/src/bin/diff.  The includes are micronix's
 * set rather than the PDP-11's: there is no sys/param.h, sys/stat.h
 * embeds struct dsknod so sys/fs.h has to come first, and signal()
 * lives in sys/signal.h.  sys/dir.h and dirent.h are left out here -
 * only diffdir.c walks directories, and it pulls them in itself.
 */
#include <types.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/fs.h>
#include <sys/stat.h>
#include <sys/signal.h>

/*
 * A micronix inode splits the file size into two words - d_size0 (the
 * high half) and d_size1 - where v7 has one long st_size.  fsize()
 * glues them back.  This is the same macro du uses.
 */
#define	fsize(sp)	(((long)(sp)->st_size0 << 16) + (sp)->st_size1)

/*
 * Output format options
 *
 * The flags below are char, not int: every one is a boolean, an
 * exit status, or a five-value mode, and on this machine a byte is
 * their honest size.  context is the exception and stays int - it
 * comes off the command line through atoi and takes part in line
 * arithmetic.
 *
 * opt was an exception too, and is not any more.  As a char it lost
 * its match against D_EDIT, which is -1, and diff -e printed the
 * text of its script with none of the ed command lines: switch (opt)
 * left the byte in A and the dispatch zero-extended it, so 0x00ff
 * went up against the 0xffff in the case table.  The == spellings
 * next to it were right the whole time, which is what made it look
 * like it only happened in company.  ccc ab6c40a promotes a switch
 * control the way C says to, and opt is a char like the rest.
 */
char	opt;

#define	D_NORMAL	0	/* Normal output */
#define	D_EDIT		-1	/* Editor script out */
#define	D_REVERSE	1	/* Reverse editor script */
#define	D_CONTEXT	2	/* Diff with context */
#define	D_IFDEF		3	/* Diff with merged #ifdef's */
#define	D_NREVERSE	4	/* Reverse ed script with numbered
				   lines and no trailing . */

char	tflag;			/* expand tabs on output */

/*
 * Algorithm related options
 */
char	hflag;			/* -h, use halfhearted DIFFH */
char	bflag;			/* ignore blanks in comparisons */
char	wflag;			/* totally ignore blanks in comparisons */
char	iflag;			/* ignore case in comparisons */

/*
 * Options on hierarchical diffs.
 */
char	lflag;			/* long output format with header */
char	rflag;			/* recursively trace directories */
char	sflag;			/* announce files which are same */
char	*start;			/* do file only if name >= this */

/*
 * Variables for -I D_IFDEF option.
 */
char	wantelses;		/* -E */
char	*ifdef1;		/* String for -1 */
char	*ifdef2;		/* String for -2 */
char	*endifname;		/* What we will print on next #endif */
char	inifdef;

/*
 * Variables for -c context option.
 */
int	context;		/* lines of context to be printed */

/*
 * State for exit status.
 */
char	status;
char	anychange;
char	*tempfile;		/* used when comparing against std input */

/*
 * Variables for diffdir.
 */
char	**diffargv;		/* option list to pass to recursive diffs */

/*
 * Input file names.
 * With diffdir, file1 and file2 are allocated BUFSIZ space,
 * and padded with a '/', and then efile0 and efile1 point after
 * the '/'.
 */
char	*file1, *file2, *efile1, *efile2;
struct	stat stb1, stb2;

char	*malloc(), *talloc(), *ralloc();
char	*savestr(), *splice();
char	*mktemp(), *copytemp(), *rindex();
char	*calloc(), *realloc(), *ctime();
int	done();
int	execv();

extern	char diffh[], diff[], pr[];
