#
/*
 * the shell's tentative globals, defined once
 *
 * v7 relied on the linker merging common symbols; this linker does not,
 * so defs.h declares them extern and they live here.
 */

#include	"defs.h"

INT		ioset;
IOPTR		iotemp;
IOPTR		iopend;

INT		dolc;
STRING		*dolv;
DOLPTR		argfor;
ARGPTR		gchain;

INT		wdval;
INT		wdnum;
ARGPTR		wdarg;
INT		wdset;
BOOL		reserv;

STRING		cmdadr;
STRING		exitadr;
STRING		dolladr;
STRING		pcsadr;
STRING		pidadr;

STRING		tmpnam;
INT		serial;

INT		peekc;
STRING		comdiv;

INT		flags;

jmp_buf		subshell;
jmp_buf		errshell;

BOOL		trapnote;
BOOL		backnote;

CHAR		jobline[JOBNM];
INT		joblinep;
BOOL		jobsnap;

INT		exitval;
BOOL		execbrk;
INT		loopcnt;
INT		breakcnt;
