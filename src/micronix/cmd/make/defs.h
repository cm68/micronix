/*
 * make - the v7 make, ported to micronix
 *
 * cmd/make/defs
 *
 * The shared header, kept named "defs" as in v7; every translation
 * unit pulls it in with  #include "defs".  The headers are this
 * tree's own: ccc's system directory holds a CP/M stat.h and the
 * Whitesmith stdio.h, and neither is what a micronix command wants
 * (see GNUmakefile.inc).
 *
 * TIMETYPE is long because micronix time() and st_mtime are 32 bits;
 * a 16-bit int would roll the clock over in eighteen hours.
 */

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <types.h>
#include <sys/fs.h>
#include <sys/stat.h>
#include <sys/dir.h>
#include <unistd.h>

#define SHELLCOM "/bin/sh"
typedef long TIMETYPE;

/* define FSTATIC to be static on systems with C compilers
   supporting file-static; otherwise define it to be null
*/
#define FSTATIC static

#define NO 0
#define YES 1

#define unequal strcmp
#define HASHSIZE 509
#define NLEFTS 128
#define NCHARS 500
#define NINTS  250
#define INMAX 1500
/*
 * OUTMAX is docom()'s command buffer, and it is a stack local: every
 * byte of it is a byte the stack cannot use.  It was 2500, and with
 * this tree's makefile - whose longest command is 307 characters -
 * that cost more than the margin between the heap and the stack:
 * doname's expansion had the break at 0xe92b, and docom's 2500 pushed
 * subst's frame below it.  1024 still holds the longest command twice
 * over and gives the stack the room.
 */
#define OUTMAX 1024
#define QBUFMAX 1500

#define ALLDEPS  1
#define SOMEDEPS 2

#define META 01
#define TERMINAL 02
extern char funny[128];


#define ALLOC(x) (struct x *) ckalloc(sizeof(struct x))

/*
 * signal numbers and dispositions, from sys/signal.h.  Declared here
 * rather than including <signal.h> so the constants are plain shorts
 * that match libunix's signal(short, short), not the function-pointer
 * casts of the POSIX header.
 */
#define SIG_DFL 0
#define SIG_IGN 1
#define SIGINT  2
#define SIGQUIT 3
extern short signal(int sig, short handler);

extern int sigivalue;
extern int sigqvalue;
extern int waitpid;
extern int dbgflag;
extern int prtrflag;
extern int silflag;
extern int noexflag;
extern int keepgoing;
extern int noruleflag;
extern int touchflag;
extern int questflag;
extern int ndocoms;
extern int ignerr;
extern int okdel;
extern int inarglist;
extern char *prompt;
extern char junkname[ ];



struct nameblock
	{
	struct nameblock *nxtnameblock;
	char *namep;
	struct lineblock *linep;
	int done;
	int septype;
	TIMETYPE modtime;
	};

extern struct nameblock *mainname ;
extern struct nameblock *firstname;

struct lineblock
	{
	struct lineblock *nxtlineblock;
	struct depblock *depp;
	struct shblock *shp;
	};
extern struct lineblock *sufflist;

struct depblock
	{
	struct depblock *nxtdepblock;
	struct nameblock *depname;
	};

struct shblock
	{
	struct shblock *nxtshblock;
	char *shbp;
	};

struct varblock
	{
	struct varblock *nxtvarblock;
	char *varname;
	char *varval;
	int noreset;
	int used;
	};
extern struct varblock *firstvar;

struct pattern
	{
	struct pattern *nxtpattern;
	char *patval;
	};
extern struct pattern *firstpat;

struct opendir
	{
	struct opendir *nxtopendir;
	FILE * dirfc;
	char *dirn;
	};
extern struct opendir *firstod;


struct chain
	{
	struct chain *nextp;
	char *datap;
	};

char *copys(char *), *concat(char *, char *, char *), *subst(char *, char *);
int *ckalloc(int);
struct nameblock *srchname(char *), *makename(char *);
TIMETYPE exists(char *);
extern int intrupt(void);
