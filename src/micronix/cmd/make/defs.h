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
#define NLEFTS 40
#define NCHARS 500
#define NINTS  250
#define INMAX 1500
#define OUTMAX 2500
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
 * that match libu's signal(short, short), not the function-pointer
 * casts of the POSIX header.
 */
#define SIG_DFL 0
#define SIG_IGN 1
#define SIGINT  2
#define SIGQUIT 3
extern short signal(int sig, short handler);

/*
 * The standard library has no declaration for these three in stdio.h,
 * so make's implicit use of them would not type-check.  Declared here
 * with the shapes libc's bodies actually have.
 */
extern int fread(char *, int, int, FILE *);
extern int fclose(FILE *);
extern int fflush(FILE *);

/*
 * The system calls make reaches directly, declared with the shapes
 * libu actually implements.  unistd.h is not pulled in because its
 * stat prototype takes a char * buffer and make passes a struct stat;
 * the ABI is the same pointer either way but the types are cleaner
 * spelled out here.
 */
extern int fork(void);
extern int wait(int *);
extern int execv(char *, char **);
extern int execl(char *, char *, char *, char *, char *);
extern long lseek(int, long, int);
extern int stat(char *, struct stat *);
extern int creat(char *, int);
extern int open(char *, int);
extern int read(int, char *, int);
extern int write(int, char *, int);
extern int close(int);
extern int unlink(char *);
extern int time(long *);
extern char access(char *, int);

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

char *copys(), *concat(), *subst();
int *ckalloc();
struct nameblock *srchname(), *makename();
TIMETYPE exists();
extern int intrupt();
