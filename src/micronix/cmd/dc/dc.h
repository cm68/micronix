#define FATAL 0
#define NFATAL 1
#define BLK sizeof(struct blk)
#define PTRSZ sizeof(int *)
#define HEADSZ 1024
#define STKSZ 100
#define RDSKSZ 100
#define TBLSZ 256
#define ARRAYST 0241
#define MAXIND 2048
#define NL 1
#define NG 2
#define NE 3
#define length(p) ((p)->wt-(p)->beg)
#define rewind(p) (p)->rd=(p)->beg
#define create(p)	(p)->rd = (p)->wt = (p)->beg
#define fsfile(p)	(p)->rd = (p)->wt
#define truncate(p)	(p)->wt = (p)->rd
#define sfeof(p)	(((p)->rd==(p)->wt)?1:0)
#define sfbeg(p)	(((p)->rd==(p)->beg)?1:0)
#define sungetc(p,c)	*(--(p)->rd)=c
#ifdef interdata
#define NEGBYTE 0200
#define MASK (-1 & ~0377)
#define sgetc(p)	( ((p)->rd==(p)->wt) ? EOF :( ((*(p)->rd & NEGBYTE) != 0) ? ( *(p)->rd++ | MASK): *(p)->rd++ ))
#define slookc(p)	( ((p)->rd==(p)->wt) ? EOF :( ((*(p)->rd & NEGBYTE) != 0) ? (*(p)->rd | MASK) : *(p)->rd ))
#define sbackc(p)	( ((p)->rd==(p)->beg) ? EOF :( ((*(--(p)->rd) & NEGBYTE) != 0) ? (*(p)->rd | MASK): *(p)->rd ))
#endif
#ifndef interdata
/*
 * The (int) cast on the character read is a port addition and not
 * v7's.  A block is bytes, and the byte -1 is how this dc spells the
 * sign of a negative number, so every read of one has to arrive
 * sign extended.
 *
 * ccc does that for the cast and for a plain assignment, but not for
 * a ?: whose other arm is a small positive constant: the arms are
 * one byte each, one signed and one not, the result keeps the
 * unsigned arm's type, and a 0xff read through it comes back 255
 * instead of -1.  "sfeof(p)?0:sgetc(p)", which dc writes a dozen
 * times, is exactly that shape, and the digits of a negative number
 * were arriving as 255 - 5 minus 3 printed three bytes of garbage.
 *
 * Casting here fixes all twelve at once and leaves dc's own source
 * alone.  "%d" of (int)cbuf[0] with cbuf[0] = -1 is -1 where the
 * bare ?: gives 255; see the note in dc.c.
 */
#define sgetc(p)	(((p)->rd==(p)->wt)?EOF:(int)*(p)->rd++)
#define slookc(p)	(((p)->rd==(p)->wt)?EOF:(int)*(p)->rd)
#define sbackc(p)	(((p)->rd==(p)->beg)?EOF:(int)*(--(p)->rd))
#endif
#define sputc(p,c)	{if((p)->wt==(p)->last)more(p); *(p)->wt++ = c; }
#define salterc(p,c)	{if((p)->rd==(p)->last)more(p); *(p)->rd++ = c; if((p)->rd>(p)->wt)(p)->wt=(p)->rd;}
/*
 * sunputc was a macro here - (*(p)->rd = --(p)->wt) - and is a
 * function in this port.  ccc's code generator has no rule for the
 * shape the macro makes when its result is assigned to an int, a
 * byte dereference of an assignment through a pointer, and it drops
 * the statement with "4 expression(s) that no rule could build code
 * for".  The function is v7's own, commented out further down dc.c;
 * see the note there.
 */
int	sunputc();
#define zero(p)	for(pp=(p)->beg;pp<(p)->last;)*pp++='\0'
#define OUTC(x) {printf("%c",x); if(--count == 0){printf("\\\n"); count=ll;} }
#define TEST2	{if((count -= 2) <=0){printf("\\\n");count=ll;}}
#define EMPTY if(stkerr != 0){printf("stack empty\n"); continue; }
#define EMPTYR(x) if(stkerr!=0){pushp(x);printf("stack empty\n");continue;}
#define EMPTYS if(stkerr != 0){printf("stack empty\n"); return(1);}
#define EMPTYSR(x) if(stkerr !=0){printf("stack empty\n");pushp(x);return(1);}
#define error(p)	{printf(p); continue; }
#define errorrt(p)	{printf(p); return(1); }
struct blk {
	char	*rd;
	char	*wt;
	char	*beg;
	char	*last;
};
struct	blk *hfree;
struct	blk *getwd();
struct	blk *lookwd();
struct	blk *getdec();
struct	blk *morehd();

struct	blk *arg1, *arg2;
int	svargc;
char	savk;
char	**svargv;
int	dbg;
int	ifile;
FILE	*curfile;
struct	blk *scalptr, *basptr, *tenptr, *inbas;
struct	blk *sqtemp, *chptr, *strptr, *divxyz;
struct	blk *stack[STKSZ];
struct	blk **stkptr,**stkbeg;
struct	blk **stkend;
int	stkerr;
int	lastchar;
struct	blk *readstk[RDSKSZ];
struct	blk **readptr;
struct	blk *rem;
int	k;
struct	blk *irem;
int	skd,skr;
struct	blk *pop(),*readin(),*add0(),*mult();
struct	blk *scalint();
struct	blk *removc();
struct	blk *add(),*div(),*removr();
struct	blk *exp();
struct	blk *sqrt();
struct	blk *salloc(),*copy();
struct	blk *scale();
int	neg;
struct	sym {
	struct	sym *next;
	struct	blk *val;
} symlst[TBLSZ];
struct	sym *stable[TBLSZ];
struct	sym *sptr,*sfree;
struct	wblk {
	struct blk **rdw;
	struct blk **wtw;
	struct blk **begw;
	struct blk **lastw;
};
FILE	*fsave;
long	rel;
long	nbytes;
long	all;
long	headmor;
long	obase;
int	fw,fw1,ll;
int	(*outdit)();
int	bigot(),hexot();
int	logo;
int	log10;
int	count;
char	*pp;
int	(*signal())();
int	onintr();
char	*malloc();
char	*nalloc();
char	*realloc();
char	*dummy;
