/* micronix.
 * this emulates the micronix user mode
 *
 * Copyright (c) 2018, Curt Mayer
 * do whatever you want, just don't claim you wrote it.
 * warrantee:  madness!  nope.
 *
 * plugs into the z80emu code from:
 * Copyright (c) 2012, 2016 Lin Ke-Fong
 * Copyright (c) 2012 Chris Pressey
 *
 * This code is free, do whatever you want with it.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <time.h>
#include "z80emu.h"
#include "z80user.h"
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>

typedef unsigned int ULONG;
typedef unsigned char UCHAR;
typedef unsigned short UINT;
#include "../kernel/inode.h"
#include "../include/obj.h"

#define	LISTLINES	8
#define	STACKTOP	0xffff
#define MAXIMUM_STRING_LENGTH   100

static int	do_exec(char *name, char**argv);
extern void dumpmem(unsigned char (*get)(int a), int addr, int len);
static void	emulate();

#define	DEFROOT	"../filesystem"

int debug_terminal;
int am_root;
int mypid;

char curdir[100] = "";
char *rootdir = 0;

char *initfile[] = {
	"/bin/sh",
	0
};

#define	V_SYS	(1 << 0)	/* trace system calls */
#define	V_DATA	(1 << 1)	/* dump bulk data from system calls */
#define	V_EXEC	(1 << 2)	/* exec args */
#define	V_INST	(1 << 3)	/* instructions */
#define	V_ASYS	(1 << 4)	/* even small syscalls */	
#define	V_SYS0	(1 << 5)	/* dump raw syscall args */
#define	V_ERROR	(1 << 6)	/* perror on system calls */

char *vopts[] = {
	"V_SYS", "V_DATA", "V_EXEC", "V_INST", "V_ASYS", "V_SYS0", "V_ERROR", 0
};
int verbose;

MACHINE	context;

unsigned short brake;
int breakpoint;

struct MACHINE *cp;

char namebuf[PATH_MAX];
char workbuf[PATH_MAX];

/*
 * translate our sim filename into a native filename
 * by prepending the root - we need to be a little smart
 * about this because relative paths need to first have
 * the current working directory prepended
 * this needs to be hella complicated because chroot is privileged - wtf
 */
char *
fname(char *orig)
{
	/* empty path is . */
	if (strlen(orig) == 0) {
		strcpy(workbuf, rootdir);
	} else if (*orig == '/') {
		sprintf(workbuf, "%s/%s", rootdir, orig);
	} else {
		sprintf(workbuf, "%s/%s/%s", rootdir, curdir, orig);
	}
	realpath(workbuf, namebuf);
	return (namebuf);
}

void
pid()
{
	printf("%d: ", mypid);
}

char *
get_symname(unsigned short addr)
{
	return 0;
}

unsigned short
reloc(unsigned short addr)
{
	return 0;
}

void
put_word(unsigned short addr, unsigned short value)
{
	cp->memory[addr] = value;
	cp->memory[addr+1] = value >> 8;
}

void
put_byte(unsigned short addr, unsigned char value)
{
	cp->memory[addr] = value;
}

unsigned short
get_word(unsigned short addr)
{
	return cp->memory[addr] + (cp->memory[addr+1] << 8);
}

unsigned char
get_byte(unsigned short addr)
{
	return cp->memory[addr];
}

static void
push(unsigned short s)
{
	cp->state.registers.word[Z80_SP] -= 2;
	put_word(cp->state.registers.word[Z80_SP], s);
}

static unsigned short
pop()
{
	unsigned short i;

	i = get_word(cp->state.registers.word[Z80_SP]);
	cp->state.registers.word[Z80_SP] += 2;
	return (i);
}

#define	TTY_FD	64

FILE *mytty;

void
usage(char *complaint, char *p)
{
	int i;

	printf("%s", complaint);
	printf("usage: %s [<options>] [program [<program options>]]\n", p);
	printf("\t-r\trun as root\n");
	printf("\t-t\topen a debug terminal window\n");
	printf("\t-d <root dir>\n");
	printf("\t-b\t\tstart with breakpoint\n");
	printf("\t-v <verbosity>\n");
	printf("\t-s [<syscall>[=<count>]\n");
	for (i = 0; vopts[i]; i++) {
		printf("\t%x %s\n", 1 << i, vopts[i]);
	}       
	exit(1);	
}

/* system calls to stop on */
char stop[64];

int 
main(int argc, char **argv)
{
	char *progname = *argv++;
	char *s;
	char **argvec;
	int i;
	char *ttyname;

	argc--;
	
	while (argc) {
		s = *argv;

		/* end of flagged options */
		if (*s++ != '-')
			break;

		argv++;
		argc--;

		/* s is the flagged arg string */
		while (*s) {
			switch (*s++) {
			case 'h':
				usage("", progname);
				break;
			case 't':
				debug_terminal = 1;
				break;
			case 'r':
				am_root = 1;
				break;
			case 'd':
				if (!argc--) {
					usage("directory not specified\n", progname);
				}
				rootdir = *argv++;
				break;
			case 'v':
				if (!argc--) {
					usage("verbosity not specified \n", progname);
				}
				verbose = strtol(*argv++, 0, 0);
				break;
			case 's':
				if (!argc--) {
					usage("stop syscall list not specified \n", progname);
				}
				s = *argv++;
				i = strtol(s, &s, 0);
				if ((i > sizeof(stop)) || (i < 0))
					continue;
				stop[i] = 1;
				break;
			case 'b':
				breakpoint++;
				break;
			default:
				printf("bad flag %c\n", (*s));
				break; 
			}
		}
	}
	if (!argc) {
		argc = 1;
		argv = initfile;
	}

	if (!rootdir) {
		rootdir = DEFROOT;
	}
	mypid = getpid();

	/*
	 * we might be piping the simulator.  let's get an open file for our debug output
	 * and monitor functions.  finally, let's make sure the file descriptor is out of
	 * range of the file descriptors our emulation uses.
	 * this is so that we can debug interactive stuff that might be writing/reading from
	 * stdin, and we want all our debug output to go to a different terminal, one that
	 * isn't running a shell.  
	 * also, if we specified to open a debug window, let's connect the emulator's file
	 * descriptors to an xterm or something.
	 */
	if (debug_terminal) {
		char *cmd = malloc(100);
		int pipefd[2];

		pipe(&pipefd);
		sprintf(cmd, 
			"tty > /proc/%d/fd/%d ; while test -d /proc/%d ; do sleep 1 ; done",
			mypid, pipefd[1], mypid);
		printf("command %s\n", cmd);
		if (fork()) {
			ttyname = malloc(100);
			i = read(pipefd[0], ttyname, 100);
			if (i == -1) {
				perror("pipe");
			}
			ttyname[strlen(ttyname)-1] = 0;
		} else {
			execlp("xterm", "xterm", "-e", "bash", "-c", cmd, (char *)0); 
		}
	} else {
		ttyname = "/dev/tty";
	}
	printf("ttyname $%s$\n", ttyname);	
	mytty = fopen(ttyname, "r+");
	if (!mytty) {
		perror(ttyname);
		exit(errno);
	}
	dup2(fileno(mytty), TTY_FD);
	mytty = fdopen(TTY_FD, "r+");
	setvbuf(mytty, 0, _IOLBF, 0);
	stdout = stderr = stdin = mytty;

	/* if our rootdir is relative, we need to make it absolute */
	if (*rootdir != '/') {
		getcwd(workbuf, sizeof(workbuf));
		strcat(workbuf, "/");
		strcat(workbuf, rootdir);
		realpath(workbuf, namebuf);
		rootdir = strdup(namebuf);
	}
	if (verbose) {
		printf("verbose %x ",verbose);
		for (i = 0; vopts[i]; i++) {
			if (verbose & (1 << i)) {
				printf("%s ", vopts[i]);
			}
		}
		printf("\n");
		printf("emulating %s with root %s\n", argv[0], rootdir);
	}

	cp = &context;
        Z80Reset(&cp->state);

	/* build argvec */
	argvec = malloc((argc + 1) * sizeof(char *));
	for (i = 0; i < argc; i++) {
		argvec[i] = argv[i];
	}
	argvec[i] = 0;

	if (do_exec(fname(argvec[0]), argvec)) {
		return (EXIT_FAILURE);
	}
	free(argvec);

        cp->state.pc = pop();
	emulate();
        return EXIT_SUCCESS;
}

struct symbol {
	char *name;
	int value;
	char type;
	struct symbol *next;
};
struct symbol *syms;

char *
lookup_sym(int value)
{
	struct symbol *s;

	for (s = syms; s; s = s->next) {
		if (s->value == value)
			return s->name;
	}
	return 0;
}

void
add_sym(char *name, int type, int value)
{
	struct symbol *s;

	// printf("add_sym %x %x %s\n", type,  value, name);
	s = malloc(sizeof(*s));
	s->name = strdup(name);
	s->value = value;
	s->type = type;
	s->next = syms;
	syms = s;	
}

/*
 * this is the exec function - slightly different from the standard unix
 */
static int 
do_exec(char *name, char **argv)
{
        FILE   	*file;
	struct obj header;
	int i;
	int ai;
	unsigned short *ao;
	int argc;
	struct fsym {
		unsigned short v;
		unsigned char t;
		char name[9];
	} fsym;

	if (verbose & V_EXEC) {
		pid(); printf("exec %s\n", name);
	}
	/* count our args from the null-terminated list */
	for (argc = 0; argv[argc]; argc++) {
		if (verbose & V_EXEC) {
			printf("arg %d = %s\n", argc, argv[argc]);
		}
	}

        if ((file = fopen(name, "rb")) == NULL) {
                printf("Can't open file %s!\n", name);
		return(errno);
        }
        fseek(file, 0, SEEK_SET);
	fread(&header, 1, sizeof(header), file);
	if (header.ident != OBJECT) {
		fseek(file, 0, SEEK_END);
		header.text = ftell(file);
		header.textoff = 0x100;
		header.dataoff = header.textoff + header.text;
		header.data = 0;
		header.bss = 0;
		header.heap = 0;
		header.table = 0;
		fseek(file, 0, SEEK_SET);
	}

	if (verbose & V_EXEC) {
	printf("exec header: magic: %x conf: %x symsize: %d text: %d data: %d bss: %d heap: %d textoff: %x dataoff: %x\n",
		header.ident, header.conf, 
		header.table, header.text, header.data, 
		header.bss, header.heap, 
		header.textoff, header.dataoff);
	}

	fread(cp->memory + header.textoff, 1, header.text, file);
	fread(cp->memory + header.dataoff, 1, header.data, file);
	if (header.table) {
		// printf("got %d symbols\n", header.table);
		for (i = 0; i < header.table / sizeof(fsym); i++) {
			fread(&fsym, 1, sizeof(fsym), file);
			add_sym(fsym.name, fsym.t, fsym.v);			
		}
		// printf("read %d symbols\n", i);
	}
        fclose(file);

        cp->state.registers.word[Z80_SP] = STACKTOP;
	put_byte(8, 0x76);

        cp->state.pc = header.textoff;
	cp->is_done = 0;
	brake = header.dataoff + header.data + header.bss + header.heap;

	ao = malloc(argc * sizeof(*ao));

	/* now, copy the args to argv and the stack */
	for (i = 0; i < argc; i++) {
		ai = argc - (i + 1);
		ao[ai] = cp->state.registers.word[Z80_SP] 
			- (strlen(argv[ai]) + 1);
		cp->state.registers.word[Z80_SP] = ao[ai];
		// printf("copyout %s to %04x\n", argv[ai], ao[ai]);
		strcpy(&cp->memory[ao[ai]], argv[ai]);
	}
	push(0xffff);
	for (i = 0; i < argc; i++) {
		push(ao[argc - (i+1)]);
	}
	push(argc);
	push(header.textoff);	
	/* if (verbose & V_DATA) dumpmem(&get_byte, cp->state.registers.word[Z80_SP], 256); */
	return 0;
}

unsigned char
getsim(int addr)
{
	return *(unsigned char *)addr;
}

void
carry_set()
{
	cp->state.registers.byte[Z80_F] |= Z80_C_FLAG;
}

void
carry_clear()
{
	cp->state.registers.byte[Z80_F] &= ~Z80_C_FLAG;
}

void
dumpcpu()
{
	unsigned char f;
	char outbuf[40];
	char fbuf[9];
	char *s;
	int i;

	strcpy(fbuf, "        ");

	format_instr(cp->state.pc, outbuf, &get_byte, &lookup_sym, &reloc);
	s = lookup_sym(cp->state.pc);
	if (s) {
		fprintf(stdout,"%s\n", s);
	}
	fprintf(stdout,"%04x: %-20s ", cp->state.pc, outbuf);

	f = cp->state.registers.byte[Z80_F];
	
	if (f & Z80_C_FLAG) fbuf[0] = 'C';
	if (f & Z80_N_FLAG) fbuf[1] = 'N';
	if (f & Z80_X_FLAG) fbuf[2] = 'X';
	if (f & Z80_H_FLAG) fbuf[3] = 'H';
	if (f & Z80_Y_FLAG) fbuf[4] = 'Y';
	if (f & Z80_Z_FLAG) fbuf[5] = 'Z';
	if (f & Z80_C_FLAG) fbuf[6] = 'S';

	fprintf(stdout,
	" %s a:%02x bc:%04x de:%04x hl:%04x ix:%04x iy:%04x sp:%04x tos:%04x brk:%04x\n",
		fbuf, 
		cp->state.registers.byte[Z80_A],
		cp->state.registers.word[Z80_BC],
		cp->state.registers.word[Z80_DE],
		cp->state.registers.word[Z80_HL],
		cp->state.registers.word[Z80_IX],
		cp->state.registers.word[Z80_IY],
		cp->state.registers.word[Z80_SP],
		get_word(cp->state.registers.word[Z80_SP]),
		brake);
}

/*
 * breakpoints and watchpoints are handled using the same data structure
 */
struct point {
	unsigned short addr;
	int value;
	struct point *next;
};

struct point *breaks;
struct point *watches;

int
watchpoint_hit()
{
	struct point *p;
	int n;

	for (p = watches; p; p = p->next) {
		if (p->value == -1) {
			p->value = get_byte(p->addr);
		}
		n = get_byte(p->addr);
		if (n != p->value) {
			fprintf("value %02x at %04x changed to %02x\n",
				p->value, p->addr, n);
			p->value = n;
			return (1);
		}
	}
	return (0);
}

struct point *
point_at(struct point **head, unsigned short addr, struct point **pp)
{
	struct point *p;
	if (pp)
		*pp = 0;
	for (p = *head; p; p = p->next) {
		if (p->addr ==addr) {
			break;
		}
		if (pp) {
			*pp = p;
		}
	}
	return p;
}

int lastaddr = -1;

int
monitor()
{
	struct point *p, *prev, **head;
	char cmdline[100];
	char l;
	char c;
	int i;
	int delete;
	char *s;

	while (1) {
	more:
		fprintf(stdout, "%d >>> ", mypid);
		s = fgets(cmdline, sizeof(cmdline), stdin);
		if (*s) {
			s[strlen(s)-1] = 0;
		}
		c = *s++;
		while (*s && (*s == ' ')) s++;
		head = &breaks;
		switch(c) {
		case 'c':
			c = 1;
			while (*s && (*s == ' ')) s++;
			if (!*s) {
				for (i = 0; i < sizeof(stop); i++) {
					if ((i % 16) == 0) fprintf(stdout,"\n%02d: ", i);
					fprintf(stdout,"%03d ", stop[i]);
				}
				fprintf(stdout, "\n");
			}
			if (*s == '-') {
				s++;
				c = 0;
			}
			if (*s) {
				i = strtol(s, &s, 16);
			}
			if (i) {
				if (i < 0) {
					i = -i;
					c = 0;
				}
				if (i < sizeof(stop)) {
					stop[i] = c;
				}
			}
			break;
		case 'd':
			while (*s && (*s == ' ')) s++;
			if (*s) {
				i = strtol(s, &s, 16);
			} else {
				if (lastaddr == -1) {
					i = cp->state.registers.word[Z80_SP];
				} else {
					i = lastaddr;
				}
			}
			dumpmem(&get_byte, i, 256);
			lastaddr = (i + 256) & 0xfff;
			break;
		case 'l':
			while (*s && (*s == ' ')) s++;
			if (*s) {
				i = strtol(s, &s, 16);
			} else {
				if (lastaddr == -1) {
					i = cp->state.pc;
				} else {
					i = lastaddr;
				}
			}
			for (l = 0; l < LISTLINES; l++) {
				c = format_instr(i, cmdline, &get_byte, &lookup_sym, &reloc);
				s = lookup_sym(i);
				if (s) {
					fprintf(stdout, "%s\n", s);
				}
				fprintf(stdout, "%04x: %-20s\n", i, cmdline);
				i += c;
				lastaddr = i & 0xfff;
			}
			break;
		case 'r':
			dumpcpu();
			break;
		case 's':
			dumpcpu();
			return (1);
		case 'g':
			return (0);
		case 'q':
			exit(1);
			return (0);
		case 'w':	/* w [-] <addr> <addr> ... */
			head = &watches;
		case 'b':	/* b [-] <addr> <addr> ... */
			delete = 0;
			i = -1;
			if (*s == '-') {
				s++;
				delete = 1;
			}
			while (*s) {
				i = strtol(s, &s, 16);
				p = point_at(head, i, &prev);
				if (p && delete) {
					if (prev) {
						prev->next = p->next;
					} else {
						*head = p->next;
					}
					free(p);
				} else if ((!p) && (!delete)) {
					p = malloc(sizeof(*p));
					p->addr = i;
					p->next = *head;
					*head = p;
				}
				while (*s && (*s == ' ')) s++;
			}
			if (i == -1) {
				if (delete) {
					while ((p = *head)) {
						*head = p->next;
						free(p);
					}
				} else {
					for (p = *head; p; p = p->next) {
						fprintf(stdout, "%04x\n", p->addr);
					}
				}
			}
			break;
		case '?':
		case 'h':
			fprintf(stdout, "commands:\n");
			fprintf(stdout, "l <addr> :list\n");
			fprintf(stdout, "d <addr> :dump memory\n");
			fprintf(stdout, "r dump cpu state\n");
			fprintf(stdout, "g: continue\n");
			fprintf(stdout, "s: single step\n");
			fprintf(stdout, "q: exit\n");
			fprintf(stdout, "b [-] <nnnn> ... :breakpoint\n");
			fprintf(stdout, "w [-] <nnnn> ... :watchpoint\n");
			fprintf(stdout, "c [-] <nn> :system call trace\n");
			break;
		default:
			fprintf(stdout, "unknown command %c\n", c);
			break;
		case 0:
			break;
		}
	}
}

/*
 * this is the actual micronix emulator that emulates all the system calls
 * of micronix. initially, it starts with exec of the named file, and then 
 * jumps to the emulation
 */
static void 
emulate()
{
	unsigned char *ip;
	int i;
	
	do {
		if (watchpoint_hit()) {
			breakpoint = 1;
		}
		if (point_at(&breaks, cp->state.pc, 0)) {
			if (point_at(&breaks, cp->state.pc, 0)) {
				pid();
				printf("break at %04x\n", cp->state.pc);
			}
			breakpoint = 1;
		}
		if (breakpoint) {
			breakpoint = monitor();
		}
		if (verbose & V_INST) {
			pid(); dumpcpu();
		}
		/*
                 * the second arg is the number of cycles we are allowing 
                 * the emulator to run
                 */
                Z80Emulate(&cp->state, 1, &context);
		if (cp->state.status == Z80_STATUS_HALT) {
			SystemCall(&context);
		}
	} while (!cp->is_done);
}


/*
 * we have to be very shady about directories, since the v6 directory is directly read, and
 * the abstract read directory stuff is far in the future.  so what we do when we open a
 * directory, we translate the entire directory into V6 form and keep the buffer around
 * until the directory is closed.
 */
struct dirfd {
	DIR *dp;
	int fd;
	unsigned char *buffer;
	int bufsize;
	int offset;
	struct dirfd *next;
	int end;
} *opendirs;

/* a version 6 directory entry */
struct v6dir {
	UINT inum;
	char name[14];
};

#define	DIRINC	16

/*
 * open a directory and return the file descriptor
 */
unsigned short
dirsnarf(char *name)
{
	DIR *dp;
	struct dirent *de;
	struct dirfd *df;
	int i;
	struct v6dir *v;
	
	dp = opendir(name);
	if (!dp) return -1;	

	df = malloc(sizeof(struct dirfd));
	df->fd = dirfd(dp);
	df->dp = dp;
	df->bufsize = 0;
	df->buffer = 0;
	df->offset = 0;
	df->end = 0;
	v = (struct v6dir *)df->buffer;
	df->next = opendirs;
	opendirs = df;

	while (1) {
		de = readdir(dp);
		if (!de) break;
		if (df->end + sizeof(*v) > df->bufsize) {
			df->bufsize += DIRINC;
			df->buffer = realloc(df->buffer, df->bufsize);
			bzero(&df->buffer[df->end], DIRINC);
		}
		v = (struct v6dir *)&df->buffer[df->end];
		v->inum = (UINT)(((de->d_ino >> 16) ^ de->d_ino) & 0xffff);
		strncpy(v->name, de->d_name, 14);
		v++;
		df->end += sizeof(*v);
	}
	if (verbose & V_DATA) dumpmem(&getsim, df->buffer, df->bufsize);	
	return (df->fd);
}

struct dirfd *
dirget(int fd)
{
	struct dirfd *n;
	for (n = opendirs; n; n = n->next) {
		if (n->fd == fd) {
			return (n);
		}
	}
	return (0);
}

void
dirclose(int fd)
{
	struct dirfd *n, *p;
	p = 0;

	for (n = opendirs; n; n = n->next) {
		if (n->fd == fd)
			break;
		p = n;
	}
	if (!n) {
		return;
	}
	if (p) {
		p->next = n->next;
	} else {
		opendirs = n->next;
	}
	closedir(n->dp);
	free(n->buffer);
	free(n);
}

char *signame[] = {
	"bogus",
	"hup",
	"int",
	"quit",
	"ill",
	"trace",
	"bg",
	"rec",
	"fpe",
	"kill",
	"bus",
	"segv",
	"sysarg",
	"pipeerr",
	"alarm",
	"term"
};

/*
 * the number of bytes to adjust the return address on the stack by.
 * since the rst1 already advanced to point at the function code, 1 has
 * already been advanced over. we minimally need to bump again by 1, for
 * the function code.
 */
struct syscall {
	char argbytes;
	char *name;
	short flag;
#define	SF_NAME		1
#define	SF_NAME2	2
#define	SF_FD		4
#define	SF_ARG1		8
#define	SF_ARG2		16
#define	SF_ARG3		32
#define	SF_ARG4		64
#define	SF_BUF		128
#define	SF_SMALL	256
} syscalls[] = {
/* 0  */	{3, "indir", SF_ARG1 },
/* 1  */	{1, "exit", SF_FD },
/* 2  */	{1, "fork", 0 },
/* 3  */	{5, "read", SF_FD|SF_ARG1|SF_ARG2|SF_BUF },
/* 4  */	{5, "write", SF_FD|SF_ARG1|SF_ARG2|SF_BUF },
/* 5  */	{5, "open", SF_NAME|SF_ARG2 },
/* 6  */	{1, "close", SF_FD },
/* 7  */	{1, "wait", 0 },
/* 8  */	{5, "creat", SF_NAME|SF_ARG2 },
/* 9  */	{5, "link", SF_NAME|SF_NAME2 },
/* 10  */	{3, "unlink", SF_NAME },
/* 11  */	{5, "exec", SF_NAME|SF_ARG2 },
/* 12  */	{3, "chdir", SF_NAME },
/* 13  */	{1, "time", 0 },
/* 14  */	{7, "mknod", SF_NAME|SF_ARG2|SF_ARG3 },
/* 15  */	{5, "chmod", SF_NAME|SF_ARG2 }, 
/* 16  */	{5, "chown", SF_NAME|SF_ARG2 },
/* 17  */	{3, "sbrk", SF_ARG1|SF_SMALL },
/* 18  */	{5, "stat", SF_NAME|SF_ARG2 },
/* 19  */	{5, "seek", SF_FD|SF_ARG1|SF_ARG2 },
/* 20  */	{1, "getpid", 0 },
/* 21  */	{7, "mount", SF_NAME|SF_NAME2|SF_ARG3 },
/* 22  */	{3, "umount", SF_NAME },
/* 23  */	{1, "setuid", 0 },
/* 24  */	{1, "getuid", 0 },
/* 25  */	{1, "stime", 0 },
/* 26  */	{7, "ptrace", SF_ARG1|SF_ARG2|SF_ARG3 },/* pid addr req */
/* 27  */	{1, "alarm", 0 },
/* 28  */	{3, "fstat", SF_FD|SF_ARG1 },
/* 29  */	{1, "pause", 0 },
/* 30  */	{1, "bad", 0 },
/* 31  */	{3, "stty", SF_FD|SF_ARG1 },
/* 32  */	{3, "gtty", SF_FD|SF_ARG1 },
/* 33  */	{5, "access", SF_NAME|SF_ARG2 },
/* 34  */	{1, "nice", 0 },
/* 35  */	{1, "sleep", SF_FD },
/* 36 */	{1, "sync", 0 },
/* 37  */	{3, "kill", SF_FD|SF_ARG1 },
/* 38  */	{1, "csw", 0 },
/* 39  */	{1, "ssw", 0 },
/* 40 */	{1, "bad", 0 },	
/* 41 */	{1, "dup", SF_FD },
/* 42 */	{1, "pipe", 0 },
/* 43 */	{3, "times", 0 },
/* 44 */	{9, "profil", SF_ARG1|SF_ARG2|SF_ARG3|SF_ARG4 },
/* 45 */	{1, "bad", 0 },
/* 46 */	{1, "bad", 0 },
/* 47 */	{1, "bad", 0 },
/* 48 */	{5, "signal", SF_ARG1|SF_ARG2 },
/* 49 */	{3, "lock", SF_FD|SF_ARG1 },
/* 50 */	{1, "unlock", SF_FD },
};

char *filename;

/*
 * micronix system calls are done using the RST8 instruction, which
 * is a one-byte call instruction to location 8, which has a halt instruction
 * placed there by the exec call.
 *
 * what we do is look on the stack for the instruction following the RST8,
 * and get the code there, which is the system call number
 *
 * if the code is 0, the next 2 bytes are the address of another syscall
 * descriptor, which starts with a rst8 byte
 *
 * in any event, we need to adjust the return address on the stack to
 * to skip over the system call args. and return to after the syscall.
 */
void SystemCall (MACHINE *cp)
{
	unsigned char code;
	unsigned short sc;
	char indirect = 0;

	unsigned short fd;	/* from hl */
	unsigned short arg1;	/* first arg */
	unsigned short arg2;	/* second arg */
	unsigned short arg3;	/* third arg */
	unsigned short arg4;	/* fourth arg */

	char *fn;
	char *fn2;
	unsigned short ret;

	struct stat sbuf;
	int i;
	int p[2];
	
	struct inode *ip;
	struct dirfd *df;
	char **argvec;
	struct termios ti;
	int stopnow;
	struct syscall *sp;

	stopnow = verbose;

	if (verbose & V_SYS0) {
		pid();
		dumpcpu();
	}

	/* pop the return address from the stack */
	sc = pop();
	push(sc);

	/*
	 * upm can't use the rst1, since cp/m uses that memory.
	 * so, it does a call (0xcd) to the halt instruction.
	 * all our other code uses rst1.
	 * so, either: sc[0] == 0xcf or sc[-2] == 0xcd
	 * the hack is that in the second case, sc[0] is part of
	 * the high part of the address of the halt.
	 */
	sc -= 1; 

	/* make sure that we came here from a rst1 */
	if (((get_byte(sc)) != 0xcf) && (get_byte(sc - 2) != 0xcd)) {
		pid();
		dumpcpu();
		printf("halt no syscall %x!\n", sc);
		exit(1);
	}

	/* get the function code */
	code = get_byte(sc + 1);

	/* this is an indirect call - the argument points at a syscall */
	if (code == 0) {
		indirect++;
		sc = get_word(sc + 2);
		if ((code = get_byte(sc)) != 0xcf) {
			printf("indir no syscall %d %x!\n", code, sc);
		}
		code = get_byte(sc + 1);
	}

	/* if this is a system call we are interested in, deal with it */
	if (stop[code]) {
		pid();
		dumpcpu();
		verbose = -1;
		breakpoint = 1;
	}

	sp = &syscalls[code];

	if (verbose & V_SYS0) {
		printf("%10s %3d ", sp->name, cp->state.registers.word[Z80_HL]);
		dumpmem(&get_byte, sc, sp->argbytes + 1);
	}

	if (sp->flag & (SF_ARG1|SF_NAME)) arg1 = get_word(sc+2);
	if (sp->flag & (SF_ARG2|SF_NAME2)) arg2 = get_word(sc+4);
	if (sp->flag & SF_ARG3) arg3 = get_word(sc+6);
	if (sp->flag & SF_ARG4) arg3 = get_word(sc+8);
	if (sp->flag & SF_FD) fd = cp->state.registers.word[Z80_HL];
	if (sp->flag & SF_NAME) fn = &cp->memory[arg1];
	if (sp->flag & SF_NAME2) fn2 = &cp->memory[arg2];

	if ((verbose & V_SYS) && (!(sp->flag & SF_SMALL) || (verbose & V_ASYS))) {
		pid(); printf("%s(", sp->name);
		i = sp->flag & (SF_FD|SF_ARG1|SF_NAME|SF_ARG2|SF_NAME2|SF_ARG2|SF_ARG3|SF_ARG4);
#define F(b, f, a) \
	if (i & (b)) { i ^= (b) ; printf(f,a,i?",":""); }
		F(SF_FD, "%d%s", fd);
		F(SF_ARG1, "%04x%s", arg1);
		F(SF_NAME, "\"%s\"%s", fn);
		F(SF_ARG2, "%04x%s", arg2);
		F(SF_NAME2, "\"%s\"%s", fn2);
		F(SF_ARG3, "%04x%s", arg3);
		F(SF_ARG4, "%04x%s", arg4);
		printf(") ");
	}

	/* let's fixup the return address from the table */
	push(pop() + syscalls[indirect ? 0 : code].argbytes);

	/* let's make the assumption that all calls fail */
	carry_set();

	// carry_clear();

	switch (code) {
	case 0:	/* double indirect is a no-op */
		pid(); printf("double indirect syscall!\n");
		break;		

	case 1:		/* exit (hl) */
		exit(fd);
		break;

	case 2:		/* fork */
		ret = fork();
		if (ret) {
			push(pop() + 3);
		} else {
			mypid = getpid();
		}
		carry_clear();
		break;

	case 3: /* read (hl), buffer, len */
		if ((df = dirget(fd))) {
			ret = df->bufsize - df->offset;
			if (arg2 < ret) {
				ret = arg2;
			}
			bcopy(&df->buffer[df->offset], &cp->memory[arg1], ret);
			df->offset += ret;
		} else {
			ret = read(fd, &cp->memory[arg1], arg2);
		}
		if (ret == -1) {
			ret = errno;
			carry_set();
		} else {
			carry_clear();
		}
		break;

	case 4: /* write (hl), buffer, len */
		ret = write(fd, &cp->memory[arg1], arg2);
		if (ret == -1) {
			ret = errno;
			carry_set();
		} else {
			carry_clear();
		}
		break;

	case 5: /* open */
		switch (arg2) {
		case 0:
			arg2 = O_RDONLY;
			break;
		case 1:
			arg2 = O_WRONLY;
			break;
		case 2:
			arg2 = O_RDWR;
			break;
		default:
			pid();
			printf("open busted mode %s %04x %04x\n",
				fn, arg1, arg2);
			break;
		}
		filename = fname(fn);
		if (!stat(filename, &sbuf)) {
			if (S_ISDIR(sbuf.st_mode)) {
				ret = dirsnarf(filename);
				if (ret == 0xffff) {
					ret = errno;
					printf("dirsnarf lose\n");
					goto lose;
				}
			} else { 
				ret = open(filename, arg2);
				if (ret == -1) {
					if (verbose & V_ERROR) perror(filename);
					goto lose;
				}
			}
			carry_clear();
		} else {
			if (verbose & V_ERROR) perror(filename);
			lose:
			ret = errno;
			carry_set();
		}
		break;

	case 6:	/* close */
		if (dirget(fd)) {
			dirclose(fd);
		} else {
			close(fd);
		}
		carry_clear();
		break;

	case 7: /* wait */
		if (verbose & V_SYS) {
			pid(); printf("wait\n");
		}
		if ((ret = wait(&fd)) == -1) {
			if (verbose & V_SYS) { pid(); printf("no children\n"); }
			carry_set();
			break;
		}
		if (verbose & V_SYS) {
			pid(); printf("wait ret %x %x\n", ret, fd);
		}
		cp->state.registers.byte[Z80_D] = WEXITSTATUS(fd);
		cp->state.registers.byte[Z80_E] = 0;
		if (WIFSIGNALED(fd)) {
			cp->state.registers.byte[Z80_D] = 1;
			cp->state.registers.byte[Z80_E] = WTERMSIG(fd);
		}
		carry_clear();
		break;

	case 8:	/* creat <name> <mode> */
		ret = creat(filename = fname(fn), arg2);
		if (ret == -1) {
			if (verbose & V_ERROR) perror(filename);
			ret = errno;
			carry_set();
		} else {
			carry_clear();
		}
		break;

	case 9:	/* link <old> <new> */
		/*
		 * special case code when doing a mkdir:  
		 * our applications put links to . and .. in the
		 * directory, and we need to ignore these system calls
		 */
		i = strlen(fn2);
		if ((strcmp(&fn2[i-3], "/..") == 0) || 
			(strcmp(&fn2[i-2], "/.") == 0)) {
			carry_clear();
			ret = 0;
			break;
		}
		filename = strdup(fname(fn));
		ret = link(filename, fname(fn2));
		if (ret != 0) {
			if (verbose & V_ERROR) perror(filename);
			ret = -1;
		} else {
			carry_clear();
		}
		free(filename);
		break;

	case 10:	/* unlink <file> */
		/*
		 * special case code when doing a rmdir
		 */
		i = strlen(fn);
		if ((strcmp(&fn[i-3], "/..") == 0) || 
		(strcmp(&fn[i-2], "/.") == 0)) {
			carry_clear();
			ret = 0;
			break;
		}
		ret = unlink(filename = fname(fn));
		if (ret != 0) {
			if (errno == EISDIR) {
				ret = rmdir(filename);
				if (ret == 0) {
					carry_clear();
					break;
				}
			}
			if (verbose & V_ERROR) perror(filename);
			ret = errno;
			carry_set();
		} else {
			carry_clear();
		}
		break;

	case 11: /* exec */
		/* let's count our args */
		i = 0;
		while (get_word(arg2 + (i * 2))) i++;
		argvec = malloc((i + 1) * sizeof (char *));
		i = 0;
		for (i = 0; (arg1 = get_word(arg2 + (i * 2))); i++) {
			argvec[i] = strdup(&cp->memory[arg1]);
		}
		argvec[i] = 0;	
		ret = do_exec(fname(fn), argvec);
		if (ret) {
			ret = errno;
			carry_set();
		} else {
			carry_clear();
		}
		for (i = 0; argvec[i]; i++) {
			free(argvec[i]);
		}
		free(argvec);
		break;

	case 12: /* chdir <ptr to name> */
		/* again, because of chroot being privileged, we need
		 * to do some pretty sleazy stuff */
		if (*fn == '/') {
			strcpy(namebuf, fn);
		} else {
			sprintf(namebuf, "%s/%s", curdir, fn);
		}
		sprintf(workbuf, "%s/%s", rootdir, namebuf);
		realpath(workbuf, namebuf);
		ret = stat(filename = workbuf, &sbuf);
		if (ret || !(S_ISDIR(sbuf.st_mode))) {
			ret = 20;
			carry_set();
		} else {
			/* we are good to go - strip out root again */
			if (strncmp(namebuf, rootdir, strlen(rootdir)) == 0) {
				strcpy(curdir, &namebuf[strlen(rootdir)]);
			} else {
				strcpy(curdir, "");
			}
			carry_clear();
		}
		break;
	case 13:	/* time */
		i = time(0);
		cp->state.registers.word[Z80_DE] = i & 0xffff;
		ret = (i >> 16) & 0xffff;
		carry_clear();
		break;
	case 14:	/* mknod <name> mode dev (dev == 0) for dir */
		if (arg3 == 0) {
			arg2 &= 0777;
			ret = mkdir(filename = fname(fn), arg2);
			if (ret == -1) {
				if (verbose & V_ERROR) perror(filename);
				ret = errno;
				carry_set();
			} else {
				carry_clear();
			}
		} else {
			carry_set();
		}
		break;

	case 15:	/* chmod <name> <mode> */
		carry_set();
		break;

	case 16:	/* chown <name> <mode> */
		carry_set();
		if (am_root) {
			carry_clear();
		}
		break;

	case 17:	/* sbrk <addr> */
		ret = brake;
		brake = arg1;
		carry_clear();
		break;

	case 18:	/* stat fn buf */
	case 28:	/* fstat fd buf */
		if (code == 28) {
			ret = fstat(fd, &sbuf);
			if ((df = dirget(fd))) {
				sbuf.st_size = df->end;
			}
			arg2 = arg1;
		} else {
			ret = stat(filename = fname(fn), &sbuf);
			if (ret) {
				if (verbose & V_ERROR) perror(filename);
			}
		}
		if (ret) {
			ret = errno;
			carry_set();
		} else {
			ip = (struct inode *)&cp->memory[arg2];
			ip->dev = sbuf.st_dev;
			ip->inum = sbuf.st_ino;
			ip->mode = sbuf.st_mode;
			ip->nlinks = sbuf.st_nlink;
			ip->uid = sbuf.st_uid;
			ip->gid = sbuf.st_gid;
			ip->size0 = sbuf.st_size >> 16;
			ip->size1 = sbuf.st_size & 0xffff;
			ip->rtime = sbuf.st_atime;
			ip->wtime = sbuf.st_mtime;
			if (verbose & V_DATA)
				dumpmem(&get_byte, arg2, 36);
			carry_clear();
		}
		break;

	case 19:	/* seek fd where mode */
		i = (short)arg1;
		if (arg2 > 2) {
			i *= 512;
			arg2 -= 3;
		}
		if ((df = dirget(fd))) {
			switch (arg2) {
			case 0:
				break;
			case 1:
				i += df->offset;
				break;
			case 2:
				i += df->end;
				break;
			}
			df->offset = i;
		} else {
			switch (arg2) {
			case 0:
				i = lseek(fd, i, SEEK_SET);
				break;
			case 1:
				i = lseek(fd, i, SEEK_CUR);
				break;
			case 2:
				i = lseek(fd, i, SEEK_END);
				break;
			}
		}
		ret = (i >> 16) & 0xffff;
		ret = 0;
		carry_clear();
		break;

	case 20:	/* getpid */
		ret = getpid();
		carry_clear();
		break;

	case 21:	/* mount */
		carry_set();
		break;

	case 22:	/* umount */
		carry_set();
		break;

	case 23:	/* setuid */
		carry_set();
		break;

	case 24:	/* getuid */
		ret = getuid();
		if (am_root) ret = 0;
		carry_clear();
		break;

	case 25:	/* stime */
		carry_set();
		break;

	case 31:	/* stty */
		carry_set();
		carry_clear();
		break;

	case 32:	/* gtty */
		if (tcgetattr(fd, &ti)) {
			/* perror("gtty"); */
			carry_set();
			carry_clear();	// we do gtty on everything!
			break;
		}
		i = 0;
		if (!(ti.c_iflag & ISTRIP))
			i |= 040000;
		if (ti.c_iflag & ICRNL)
			i |= 020;
		if (ti.c_lflag & ECHO)
			i |= 010;
		if (!(ti.c_lflag & ICANON))
			i |= 040;
		if (ti.c_oflag & TABDLY)
			i |= 002;

		put_byte(arg1, ti.c_ispeed);
		put_byte(arg1+1, ti.c_ospeed);
		put_byte(arg1+2, ti.c_cc[VERASE]);	
		put_byte(arg1+3, ti.c_cc[VKILL]);
		put_word(arg1+4, i);
		carry_clear();
		break;

	case 33:	/* access <name> <mode> */
		i = 0;
		if (arg2 & 4) i |= R_OK;
		if (arg2 & 2) i |= W_OK;
		if (arg2 & 1) i |= X_OK;
		ret = access(fname(fn), i);
		if (ret == -1) {
			carry_set();
		} else {
			carry_clear();
		}
		break;

	case 34:	/* nice */
		carry_clear();
		break;

	case 35:	/* sleep */
		sleep(fd);
		carry_clear();
		break;

	case 36:	/* sync */
		carry_clear();
		break;

	case 37:	/* kill <pid in hl> signal */
		carry_set();
		break;

	case 41:
		ret = dup(fd);
		if (ret == -1) {
			ret = 0;
			carry_set();
		} else {
			carry_clear();
		}
		break;

	case 42:
		if ((i = pipe(p))) {
			ret = errno;
			carry_set();
		} else {
			ret = p[0];
			cp->state.registers.word[Z80_DE] = p[1];
			carry_clear();
		}
		break;

	case 48:
		i = arg1;
		if (i > 15) i = 0;
		carry_clear();
		break;
	default:
		pid();
		printf("unrecognized syscall %d %x\n", code, code);
		carry_set();
		break;
	}
	
	cp->state.registers.word[Z80_HL] = ret;
	if ((verbose & V_SYS) && (!(sp->flag & SF_SMALL) || (verbose & V_ASYS))) {
		if ((code == 2) && (ret == 0)) { printf("\n%d: fork() ", mypid); }
		printf(" = %04x%s\n", ret, (cp->state.registers.byte[Z80_F] & Z80_C_FLAG) ? " FAILED" : "");
	}
	if ((verbose & V_DATA) && (sp->flag & SF_BUF)) {
		dumpmem(&get_byte, arg1, ret);
	}
	cp->state.pc = pop();
	cp->state.status = 0;
	verbose = stopnow;
	if ((verbose & V_SYS) && (cp->state.registers.word[Z80_HL] == 0xffff)) {
		printf("code = %d\n", code);
		dumpcpu();
	}
}
