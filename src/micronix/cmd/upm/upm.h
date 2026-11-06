/*
 * upm - CP/M for Micronix
 *
 * cmd/upm/upm.c
 *
 * THIS IS A RECONSTRUCTION.  There is no surviving source for /bin/upm;
 * this file is written from the disassembly of the /bin/upm binary on the
 * Micronix 1.6 standalone - see upm.dis, upm.ctl and README beside this
 * file.  Every function below corresponds to one function in that binary,
 * in the same order, and the odd bits are reproduced rather than tidied
 * away, because the odd bits are the ones that say what the system
 * expected.  Where the disassembly did not tell me something, the comment
 * says so.
 *
 * upm is Whitesmith's C - most functions open with a CALL to c.ent or
 * c.ents, the frame helper - so the calling convention is that one:
 * arguments are pushed right to left and are read at DE+4, DE+6, ...;
 * the result comes back in BC.  The hand-written bits are the crt
 * (upmhead), the CP/M entry bridge (entry/endentr), the BIOS jump
 * table (bios), and the interrupt handler (tint); those are not C and
 * are reproduced in upm.s, not here.
 *
 * The layout is backwards from every other program on this machine: the
 * data segment loads at 0100 and the text at d645.  The CP/M TPA a .com
 * is loaded into lives at the bottom of the address space, and upm's own
 * code sits above it.
 *
 * The BDOS dispatcher is cpm, which reads the function number out of its
 * own frame and indexes cpmtab - forty words, one handler per CP/M 2.2
 * function - after storing the argument in arg2.  Handlers that want a
 * stack frame read the argument off the frame cpm pushed it onto; the
 * small ones read arg2.  Both conventions are real; which one a handler
 * uses is visible in whether it opens with a c.ent/c.ents CALL.
 */
/*
 * upm.h - what the two halves of upm share.
 *
 * upm is linked backwards on purpose.  Its text sits high and its data
 * starts at 0x0100, the CP/M TPA base, so a transient program loaded
 * over it is free to clobber the CCP - the part of upm a warm boot
 * re-reads off the system tracks.  That division is a property of the
 * link, not of the source: the resident half is linked with -Stext and
 * the CCP half with -Sdata, each folded to a single segment, and
 * -Ttext=0xd500 / -Tdata=0x0100 put them where the layout wants them.
 *
 * So the CCP has to be its own object, and this header is what the two
 * objects share: <types.h>, the structs, the function prototypes, and
 * the variables each half defines for the other.
 *
 * upm_res.c is the resident half; upm_ccp.c is the CCP.  A definition
 * belongs in whichever half owns its segment; only declarations belong
 * here.
 */
/*
 * upm - CP/M for Micronix
 *
 * cmd/upm/upm.c
 *
 * THIS IS A RECONSTRUCTION.  There is no surviving source for /bin/upm;
 * this file is written from the disassembly of the /bin/upm binary on the
 * Micronix 1.6 standalone - see upm.dis, upm.ctl and README beside this
 * file.  Every function below corresponds to one function in that binary,
 * in the same order, and the odd bits are reproduced rather than tidied
 * away, because the odd bits are the ones that say what the system
 * expected.  Where the disassembly did not tell me something, the comment
 * says so.
 *
 * upm is Whitesmith's C - most functions open with a CALL to c.ent or
 * c.ents, the frame helper - so the calling convention is that one:
 * arguments are pushed right to left and are read at DE+4, DE+6, ...;
 * the result comes back in BC.  The hand-written bits are the crt
 * (upmhead), the CP/M entry bridge (entry/endentr), the BIOS jump
 * table (bios), and the interrupt handler (tint); those are not C and
 * are reproduced in upm.s, not here.
 *
 * The layout is backwards from every other program on this machine: the
 * data segment loads at 0100 and the text at d645.  The CP/M TPA a .com
 * is loaded into lives at the bottom of the address space, and upm's own
 * code sits above it.
 *
 * The BDOS dispatcher is cpm, which reads the function number out of its
 * own frame and indexes cpmtab - forty words, one handler per CP/M 2.2
 * function - after storing the argument in arg2.  Handlers that want a
 * stack frame read the argument off the frame cpm pushed it onto; the
 * small ones read arg2.  Both conventions are real; which one a handler
 * uses is visible in whether it opens with a c.ent/c.ents CALL.
 */

#include <types.h>
#include <hitech.h>
#include <stdio.h>
#include <string.h>

/*
 * The rest of the globals - files, fcb, disktab, tabs, ntab, the
 * buffers - come in with the FCB layer, whose functions are the only
 * thing that names them.
 */

/*
 * The CP/M 2.2 BDOS function table, forty words.  Read out of the .dis
 * with the function number as the index:
 *
 *   0  cexit     system reset     20 rseq     read sequential
 *   1  cconin    console input    21 wseq     write sequential
 *   2  echo      console output   22 cmake    make file
 *   3  getch     reader input     23 rename   rename file
 *   4  putch     punch output     24 clogin   login vector
 *   5  clist     list output      25 ccurdisk  current disk
 *   6  cdirio    direct cons I/O  26 cdma     set DMA
 *   7  cgetio    get I/O byte     27 null     get alloc addr
 *   8  csetio    set I/O byte     28 cprotect  write protect
 *   9  cprs      print string     29 cgetro   get read-only vec
 *  10  readbuf   read console buf 30 setatt   set file attributes
 *  11  cconstat   console status   31 getparm  get disk params
 *  12  cversion   version          32 cuser    set/get user
 *  13  creset    reset disk       33 rrand    read random
 *  14  select    select disk      34 wrand    write random
 *  15  copen     open file        35 csize    compute size
 *  16  cclose    close file       36 setrand  set random rec
 *  17  cfirst    search first     37 null     reset drive
 *  18  cnext     search next      38 null     access drive
 *  19  delete    delete file      39 null     free drive
 *
 * The odd four - 3 is getch not crdr, 2 is echo not cconout - are
 * how the binary actually dispatches console output and reader input, so
 * they are reproduced as they are, not as a CP/M manual would name them.
 */

/*
 * Every handler and every helper the dispatcher and the small handlers
 * reach, in no order but before first use.  The FCB layer and the CCP
 * are reconstructed further down and are only declared here.
 */
extern int cexit(void);
extern int echo(int);
extern int getch();
extern int putch(int);
extern int cdirio(void);
extern int cprs(void);
extern int readbuf(char *);
extern int select(int);
extern int copen(struct fcb *);
extern int setatt(struct fcb *);
extern int cclose(struct fcb *);
extern int delete(struct fcb *);
extern int cmake(struct fcb *);
extern int rename();
extern int csize(struct fcb *);
extern int setrand(struct fcb *);
extern int cread(int, struct fcb *);
extern int cwrite(int, struct fcb *);
extern int cflush(void);
extern int lput(int);
extern int search(int, struct fcb *, char *);

/*
 * ---------------------------------------------------------------------
 * The FCB layer.  copen down through cseek is one sitting-next-to-
 * another block of C in the binary, and this is where the CP/M file
 * control block and the micronix file descriptor meet.
 *
 * The FCB is not a standard CP/M one.  The first sixteen bytes are the
 * ordinary dr/name/ft/ex/s1/s2/rc, but the disk map (0x10-0x1f) has
 * been pressed into service to hold the micronix state a file needs:
 * the descriptor at 0x19, the size at 0x1a, the record count at 0x1c
 * and the random record at 0x1e.  Those offsets are read out of the
 * functions below; they are what copen writes and cread reads.
 */
struct fcb {
	uchar	dr;		/* 0x00 - drive */
	char	name[8];	/* 0x01 - filename */
	char	ft[3];		/* 0x09 - filetype; ft[0] carries R/O */
	uchar	ex;		/* 0x0c - extent */
	uchar	s1;		/* 0x0d */
	uchar	s2;		/* 0x0e */
	uchar	rc;		/* 0x0f - records in this extent */
	uchar	dm[8];		/* 0x10 - the first eight disk-map bytes */
	uchar	bc;		/* 0x18 - last record byte count; 0 = full */
	char	fd;		/* 0x19 - the micronix file descriptor,
				   -1 (0xff) when closed, so signed */
	ushort	size;		/* 0x1a - size in bytes */
	ushort	nrec;		/* 0x1c - number of 128-byte records */
	ushort	currec;		/* 0x1e - current record number; 0 means uncertain */
	uchar	cr;		/* 0x20 - current record in a sequential op,
				   filled in by the user, normally zero */
	uchar	r0;		/* 0x21 - CP/M random record, low */
	uchar	r1;		/* 0x22 - CP/M random record, mid */
	uchar	r2;		/* 0x23 - CP/M random record, high */
};

extern int cpystr(char *, char *);
extern int selerr(int);
extern int lc(int);

extern int uniqize(struct fcb *);
extern int name(struct fcb *, char *);
extern int fsize(char *);
extern int openfile(char *);
extern int setrc(struct fcb *);
extern int closefile(int);
extern int isdir(char *);
extern int closename(char *);
extern int unlink();

extern int read();
extern int open(char *, int, int);
extern int close();
extern int write();
extern int match(char *, char *);
extern int cname(char *, char *);
extern int uc(int);
extern int cseek(int, struct fcb *);
extern int advance(int, struct fcb *);
extern int seek(unsigned char fd, int offset, int whence);
extern int stat();
extern int chmod();
extern int isunique(char *);
extern int fexists(char *);
extern int creat();
extern int link();
extern int suspend(void);
extern int cfill(void);
extern int gtty(int, struct sgtty *);
extern int stty(int, struct sgtty *);
extern int _signal();
extern int dup();
extern int execl();
extern int pipe();
extern void _exit(int);
extern int clean(char *);
extern int banner(void);
extern int getline(void);
extern int getword(char *, char *);
extern int tabecho(void);
extern int taberase(void);
extern int erase(int);
extern int rbecho(int);
extern int rub(void);
extern int putb(char *, int);
extern int wboot(void);
extern int go(void);
extern int space(void);
extern int tabpos(void);
extern int itob(char *, ushort, ushort);
extern int entry();
extern int tint();
extern char bios[];
extern int isarg(char *);
extern int global(struct fcb *, char *);
extern int isplain(char *);
extern int issel(char *);
extern int devop(uchar *, char *, char **);
extern int puts(char *);
extern int prdes(void);
extern int isdes(char *);
extern int dodes(char *);
extern int isdev(char *);
extern int dodev(char *);
extern int era(char *);
extern int dir(char *);
extern int type(char *);
extern int ren(char *);
extern int dostat(char *);
extern int system(char *);
extern int trestore(void);
extern int tset(void);
extern int argname(struct fcb *, char *);
extern int access(char *, int);
extern int fork(void);
extern void perror(char *);
extern int cpystr(char *, char *);
extern int suffix(char *, char *);
extern int Lower(char *);
extern int load(void);
extern int wait(int *);
extern int intrep(char *, int *);
extern int raise(char *);

/*
 * The SAVE allocator.  A small first-fit free list over the TPA, used by
 * the save built-in and by selerr to keep strings.  A node is four
 * bytes - a next pointer and a size in four-byte units - and base is
 * the sentinel the list starts from.
 */
struct node {
	struct node *next;
	ushort size;
};

extern struct tramp {
	char v[6];
} jtab[];

/*
 * The shared variables.  Each is defined in the half that owns
 * its segment; this is the declaration the other half sees.
 */
extern uchar lstdesc;
extern char *lstdev;
extern uchar *op;
extern uchar *ip;
extern uchar *lp;
extern uchar obuf[32];
extern uchar ibuf[16];
extern uchar lbuf[32];
extern uchar sgtty[6];
extern ushort dma;
extern uchar iobyte;
extern uchar call;
extern ushort col;
extern uchar ntab;
extern uchar tabs[12];
extern ushort logvector;
extern uchar user;
extern ushort rovector;
extern uchar curdrive;
extern uchar ileft;
extern uchar lleft;
extern uchar oleft;
extern uchar recavail;
extern ushort verbose;
extern uchar loaded;
extern int errno;
extern ushort arg2;
extern char *loadfile;
extern ushort ro;
extern char buf[64];
extern char *disktab[16];
extern struct fcb fcb;
extern char dirbuf[18];
extern char statbuf[36];
extern struct node base;
extern struct node *allocp;
extern char savebuf[512];
extern short stab[15];
extern char *farg1;
extern char *farg2;
extern char ccword[32];
extern char line[128];
extern char rcbuf[512];
extern char bbuf[512];
extern int pip[2];
extern char *intstat[];
extern uchar statdriv;
extern char subfile[64];
extern int subfd;
extern int subflag;
extern int sublines;

/*
 * The resident half is two translation units (upm_res.c and upm_res2.c):
 * the whole of it overflows c0's 64K on the machine that has to rebuild
 * it.  These are the resident globals both units name; upm_res.c defines
 * them.
 *
 * The deferred last record.  A CP/M write is a full 128-byte record, but
 * the CP/M 3 byte-count extension lets the final record be short, so upm
 * holds the most recent record back and writes it on the next write or on
 * close.  pending is a copy of the DMA record, because the program may
 * reuse its DMA buffer as soon as the write returns.
 */
extern char pending[128];
extern char pending_valid;
extern struct fcb *pending_fcb;
extern ushort pending_rec;

/*
 * The open-file table.  One four-byte entry per descriptor: the device
 * and inode the descriptor is on, read out of struct stat when the file
 * was opened.  closename matches a name by stat'ing it and looking for
 * its dev/inode here.
 */
struct fileent {
	ushort	dev;
	ushort	ino;
};
extern struct fileent files[16];
