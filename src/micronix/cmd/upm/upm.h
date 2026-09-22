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
 *   5  clist     list output      25 ccurdis  current disk
 *   6  cdirio    direct cons I/O  26 cdma     set DMA
 *   7  cgetio    get I/O byte     27 null     get alloc addr
 *   8  csetio    set I/O byte     28 cprotec  write protect
 *   9  cprs      print string     29 cgetro   get read-only vec
 *  10  readbuf   read console buf 30 null     set file attributes
 *  11  cconsta   console status   31 null     get disk params
 *  12  cversio   version          32 cuser    set/get user
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
extern int cexit();
extern int echo();
extern int getch();
extern int putch();
extern int cdirio();
extern int cprs();
extern int readbuf();
extern int select();
extern int copen();
extern int cclose();
extern int delete();
extern int cmake();
extern int rename();
extern int csize();
extern int setrand();
extern int cread();
extern int cwrite();
extern int cflush();
extern int lput();
extern int search();

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
	uchar	dm[9];		/* 0x10 - the first nine disk-map bytes */
	char	fd;		/* 0x19 - the micronix file descriptor,
				   -1 (0xff) when closed, so signed */
	ushort	size;		/* 0x1a - size in bytes */
	ushort	nrec;		/* 0x1c - number of 128-byte records */
	ushort	rrec;		/* 0x1e - random record, upm's own */
	uchar	cr;		/* 0x20 - current record within the extent */
	uchar	r0;		/* 0x21 - CP/M random record, low */
	uchar	r1;		/* 0x22 - CP/M random record, mid */
	uchar	r2;		/* 0x23 - CP/M random record, high */
};

extern int cpystr();
extern int selerr();
extern int lc();

extern int uniqize();
extern int name();
extern int fsize();
extern int openfil();
extern int setrc();
extern int closefi();
extern int isdir();
extern int closena();
extern int unlink();

extern int read();
extern int open();
extern int close();
extern int write();
extern int match();
extern int cname();
extern int uc();
extern int cseek();
extern int seqincr();
extern int seek();
extern int stat();
extern int isuniqu();
extern int fexists();
extern int creat();
extern int link();
extern int suspend();
extern int cfill();
extern int gtty();
extern int stty();
extern int _signal();
extern int dup();
extern int execl();
extern int pipe();
extern int _exit();
extern int clean();
extern int banner();
extern int getline();
extern int getword();
extern int tabecho();
extern int taberas();
extern int erase();
extern int rbecho();
extern int rub();
extern int putb();
extern int wboot();
extern int go();
extern int space();
extern int tabpos();
extern int itob();
extern int entry();
extern int tint();
extern char bios[];
extern int isarg();
extern int global();
extern int isplain();
extern int issel();
extern int devop();
extern int puts();
extern int prdes();
extern int isdes();
extern int dodes();
extern int isdev();
extern int dodev();
extern int era();
extern int dir();
extern int type();
extern int ren();
extern int dostat();
extern int system();
extern int trestor();
extern int tset();
extern int argname();
extern int access();
extern int fork();
extern int perror();
extern int cpystr();
extern int suffix();
extern int Lower();
extern int load();
extern int wait();
extern int intrep();
extern int raise();

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
extern ushort logvect;
extern uchar user;
extern ushort rovecto;
extern uchar curdriv;
extern uchar ileft;
extern uchar lleft;
extern uchar oleft;
extern uchar recavai;
extern ushort verbose;
extern uchar loaded;
extern int errno;
extern ushort arg2;
extern char *loadfil;
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
 * Not declared here, because only one half uses it:
 *   struct fileent files[16]	(upm_res.c)
 */
