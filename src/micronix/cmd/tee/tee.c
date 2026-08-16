/*
 * tee-- pipe fitting
 *
 * 2.11BSD tee (tee.c 5.4, Berkeley 12/14/85), ported to micronix.
 *
 * cmd/tee/tee.c
 *
 * Nearly untouched.  The open for -a is v6 spelling - open(name,1)
 * and creat if that fails - which is what the original did anyway.
 * The ESPIPE check on stdout still works because the errno numbers
 * here are the v6 ones and ESPIPE is 29 in both worlds.  The local
 * puts() that writes to fd 2 stays: this program includes no stdio
 * and links nothing that wants the library's.
 *
 * The one real change is invisible: the two 8K buffers stay 8K,
 * because even on a 64K machine, sixteen of them are what is left
 * after four of code.
 *
 * vim: tabstop=8 shiftwidth=8 noexpandtab:
 */

#include <types.h>
#include <sys/fs.h>
#include <sys/stat.h>
#include <sys/signal.h>
#include <errno.h>

#define	BUFSIZ	8192
/*
 * bytes: a descriptor is -1 or 0..15, n counts at most twenty of
 * them, t never passes the argument count, and aflag is a truth.
 */
char openf[20] = { 1 };
char n = 1;
char t = 0;
char aflag;

char in[BUFSIZ];

char out[BUFSIZ];

extern int errno;
long	lseek();

main(argc,argv)
char **argv;
{
	register int r,w,p;
	struct stat buf;
	while(argc>1&&argv[1][0]=='-') {
		switch(argv[1][1]) {
		case 'a':
			aflag++;
			break;
		case 'i':
		case 0:
			signal(SIGINT, SIG_IGN);
		}
		argv++;
		argc--;
	}
	fstat(1,&buf);
	t = (buf.st_mode&S_IFMT)==S_IFCHR;
	if(lseek(1,0L,1)==-1&&errno==ESPIPE)
		t++;
	/* "argc-- > 1", and the space is load-bearing: this compiler's
	   lexer reads "-->" as minus arrow and loses the parse */
	while(argc-- > 1) {
		if(aflag) {
			openf[n] = open(argv[1],1);
			if(openf[n] < 0)
				openf[n] = creat(argv[1],0666);
			lseek(openf[n++],0L,2);
		} else
			openf[n++] = creat(argv[1],0666);
		if(stat(argv[1],&buf)>=0) {
			if((buf.st_mode&S_IFMT)==S_IFCHR)
				t++;
		} else {
			puts("tee: cannot open ");
			puts(argv[1]);
			puts("\n");
			n--;
		}
		argv++;
	}
	r = w = 0;
	for(;;) {
		for(p=0;p<BUFSIZ;) {
			if(r>=w) {
				if(t>0&&p>0) break;
				w = read(0,in,BUFSIZ);
				r = 0;
				if(w<=0) {
					stash(p);
					exit(0);
				}
			}
			out[p++] = in[r++];
		}
		stash(p);
	}
}

stash(p)
{
	int k;
	int i;
	int d;
	d = t ? 16 : p;
	for(i=0; i<p; i+=d)
		for(k=0;k<n;k++)
			write(openf[k], out+i, d<p-i?d:p-i);
}

puts(s)
char *s;
{
	while(*s)
		write(2,s++,1);
}
