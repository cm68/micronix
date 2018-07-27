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

int mypid;

char curdir[100] = ".";
char *rootdir = 0;

char *initfile[] = {
	"/bin/sh",
	0
};

#define	V_SYS	1	/* trace system calls */
#define V_MEM	2	/* dump out reads/writes */
#define	V_SYS1	4	/* trace system call low level */
#define	V_EXEC	8	/* exec args */
#define	V_INST	16	/* instructions */
	
char *vopts[] = {
	"V_SYS", "V_MEM", "V_SYS1", "V_EXEC", "V_INST", 0
};
int verbose;

MACHINE	context;

unsigned short brake;
int breakpoint;

struct MACHINE *cp;

char namebuf[PATH_MAX];

/*
 * translate our sim filename into a native filename
 * by prepending the root - we need to be a little smart
 * about this because relative paths need to first have
 * the current working directory prepended
 */
char *fname(char *orig)
{
	if (*orig == '/') {
		sprintf(namebuf, "%s/%s", rootdir, orig);
		return (namebuf);
	}
	return (orig);
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
	printf("\t-d <root dir>\n");
	printf("\t-b\t\tstart with breakpoint\n");
	printf("\t-v <verbosity>\n");
	for (i = 0; vopts[i]; i++) {
		printf("\t%x %s\n", 1 << i, vopts[i]);
	}       
	exit(1);	
}

int 
main(int argc, char **argv)
{
	char *progname = argv[0];
	char *s;
	char **argvec;
	int i;

	/*
	 * we might be piping the simulator.  let's get an open file for our debug output
	 * and monitor functions.  finally, let's make sure the file descriptor is out of
	 * range of the file descriptors our emulation uses.
	 */
	mytty = fopen("/dev/tty", "r+");
	dup2(fileno(mytty), TTY_FD);
	mytty = fdopen(TTY_FD, "r+");
	setvbuf(mytty, 0, _IONBF, 0);
	stdout = stderr = stdin = mytty;
	argc--;
	
	while (argc) {
		argc--;
		argv++;

		s = *argv;

		/* end of flagged options */
		if (*s++ != '-')
			break;

		while (*s) {
			switch (*s++) {
			case 'h':
				usage("", progname);
			case 'd':
				if (!argc--) {
					usage("directory not specified\n", progname);
				}
				rootdir = *++argv;
				s = "";
				break;
			case 'v':
				if (!argc--) {
					usage("verbosity not specified \n", progname);
				}
				verbose = strtol(*++argv, 0, 0);
				s = "";
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

	/* if our rootdir is relative, we need to make it absolute */
	if (*rootdir != '/') {
		s = malloc(PATH_MAX);
		getwd(s);
		strcat(s, "/");
		strcat(s, rootdir);
		rootdir = s;
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

	strcpy(curdir, rootdir);
	chdir(rootdir);

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
	mypid = getpid();

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
struct symbols *syms;

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

	printf("add_sym %x %x %s\n", type,  value, name);
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
		fseek(file, 0, SEEK_SET);
	}

	if (verbose & V_SYS1) {
	printf("exec header: magic: %x conf: %x symsize: %d text: %d data: %d bss: %d heap: %d textoff: %x dataoff: %x\n",
		header.ident, header.conf, 
		header.table, header.text, header.data, 
		header.bss, header.heap, 
		header.textoff, header.dataoff);
	}

	fread(cp->memory + header.textoff, 1, header.text, file);
	fread(cp->memory + header.dataoff, 1, header.data, file);
	if (header.table) {
		printf("got %d symbols\n", header.table);
		for (i = 0; i < header.table / sizeof(fsym); i++) {
			fread(&fsym, 1, sizeof(fsym), file);
			add_sym(&fsym.name, fsym.t, fsym.v);			
		}
		printf("read %d symbols\n", i);
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
	if (verbose & V_SYS1) dumpmem(&get_byte, cp->state.registers.word[Z80_SP], 256);
	return 0;
}

unsigned char
getsim(int addr)
{
	return *(unsigned char *)addr;
}

carry_set()
{
	cp->state.registers.byte[Z80_F] |= Z80_C_FLAG;
}

carry_clear()
{
	cp->state.registers.byte[Z80_F] &= ~Z80_C_FLAG;
}

pid()
{
	printf("%d: ", mypid);
}

dumpcpu()
{
	unsigned char f;
	char outbuf[40];
	char *s;

	format_instr(cp->state.pc, outbuf, &get_byte, &lookup_sym, &reloc);
	s = lookup_sym(cp->state.pc);
	if (s) {
		printf("%s\n", s);
	}
	printf("%04x: %-20s ", cp->state.pc, outbuf);

	f = cp->state.registers.byte[Z80_F];
	
	if (f & Z80_C_FLAG) printf("C"); else  printf(" ");
	if (f & Z80_N_FLAG) printf("N"); else  printf(" ");
	if (f & Z80_X_FLAG) printf("X"); else  printf(" ");
	if (f & Z80_H_FLAG) printf("H"); else  printf(" ");
	if (f & Z80_Y_FLAG) printf("Y"); else  printf(" ");
	if (f & Z80_Z_FLAG) printf("Z"); else  printf(" ");
	if (f & Z80_S_FLAG) printf("S"); else  printf(" ");

	printf(" a:%02x bc:%04x de:%04x hl:%04x ix:%04x iy:%04x sp:%04x tos:%04x brk:%04x ",
		cp->state.registers.byte[Z80_A],
		cp->state.registers.word[Z80_BC],
		cp->state.registers.word[Z80_DE],
		cp->state.registers.word[Z80_HL],
		cp->state.registers.word[Z80_IX],
		cp->state.registers.word[Z80_IY],
		cp->state.registers.word[Z80_SP],
		get_word(cp->state.registers.word[Z80_SP]),
		brake);

	printf(" \n");
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
			printf("value %02x at %04x changed to %02x\n",
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

void
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
		printf("%d >>> ", mypid);
		s = fgets(cmdline, sizeof(cmdline), stdin);
		if (*s) {
			s[strlen(s)-1] = 0;
		}
		c = *s++;
		while (*s && (*s == ' ')) s++;
		head = &breaks;
		switch(c) {
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
					printf("%s\n", s);
				}
				printf("%04x: %-20s\n", i, cmdline);
				i += c;
				lastaddr = i & 0xfff;
			}
			break;
		case 'r':
			dumpcpu();
			break;
		case 's':
			dumpcpu();
			return;
		case 'g':
			breakpoint = 0;
			return;
		case 'q':
			exit(1);
			return;
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
						printf("%04x\n", p->addr);
					}
				}
			}
			break;
		case '?':
		case 'h':
			printf("commands:\n");
			printf("l <addr> :list\n");
			printf("d <addr> :dump memory\n");
			printf("p dump cpu state\n");
			printf("g: continue\n");
			printf("s: single step\n");
			printf("q: exit\n");
			printf("b [-] <nnnn> ... :breakpoint\n");
			printf("w [-] <nnnn> ... :watchpoint\n");
			break;
		default:
			printf("unknown command %c\n", c);
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
			monitor();
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
	v = df->buffer;
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
	if (verbose & V_SYS1) dumpmem(&getsim, df->buffer, df->bufsize);	
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
char argbytes[] = {
	3, 1, 1, 5,	/* 0  - indir, exit, fork, read */
	5, 5, 1, 1,	/* 4  - write, open, close, wait */
	5, 5, 3, 5,	/* 8  - creat, link, unlink, exec */
	3, 1, 7, 5,	/* 12 - chdir, time, mknod, chmod */
	5, 3, 5, 5,	/* 16 - chown, sbrk, stat, seek */
	1, 7, 3, 1,	/* 20 - getpid, mount, umount, setuid */
	1, 1, 7, 1,	/* 24 - getuid, stime, ptrace, alarm */
	3, 1, 1, 3,	/* 28 - fstat, pause, badcall, stty */
	3, 5, 1, 1,	/* 32 - gtty, access, nice, sleep */
	1, 3, 1, 1,	/* 36 - sync, kill, csw, ssw */
	1, 1, 1, 3,	/* 40 - badcall, dup, pipe, times */
	9, 1, 1, 1,	/* 44 - profil, badcall, badcall, badcall */
	5, 3, 1 	/* 48 - signal, lock, unlock */
};

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

	struct stat sbuf;
	int i;
	int p[2];
	
	struct inode *ip;
	struct dirfd *df;
	char *fn;
	char **argvec;
	struct termios ti;

	if (verbose & V_SYS) {
		pid();
		dumpcpu();
	}

	/* pop the return address from the stack */
	sc = pop();
	push(sc);

	/* make sure that we came here from a rst1 */
	if ((code = get_byte(sc - 1)) != 0xcf) {
		pid();
		dumpcpu();
		printf("halt no syscall %d %x!\n", code, sc);
		exit(1);
	}

	/* get the function code */
	code = get_byte(sc);

	/* this is an indirect call - the argument points at a syscall */
	if (code == 0) {
		indirect++;
		sc = get_word(sc + 1);
		if ((code = get_byte(sc)) != 0xcf) {
			printf("indir no syscall %d %x!\n", code, sc);
		}
		code = get_byte(sc+1);
	}

	if (verbose & V_SYS1) dumpmem(&get_byte, sc, argbytes[code] + 1);

	/* let's build some common args */
	arg1 = get_word(sc+2);
	arg2 = get_word(sc+4);
	arg3 = get_word(sc+6);
	fd = cp->state.registers.word[Z80_HL];

	/* file name building */
	switch (code) {
	case 5: case 8: case 9: case 10: case 11: case 12: 
	case 14: case 15: case 16: case 18: case 21: case 22:
	case 33:
		fn = fname(&cp->memory[arg1]);
		break;
	default:
		break;
	}

	if (verbose & V_SYS1) {
		pid();
		printf("%ssyscall %d %04x %04x %04x\n", 
			indirect ? "indirect " : "", 
			code, fd, arg1, arg2);
	}

	/* let's fixup the return address from the table */
	push(pop() + argbytes[indirect ? 0 : code]);

	switch (code) {
	case 0:	/* double indirect is a no-op */
		pid(); printf("double indirect syscall!\n");
		break;		

	case 1:		/* exit (hl) */
		if (verbose & V_SYS) {
			pid(); printf("exit %04x\n", fd);
		}
		exit(fd);
		break;

	case 2:		/* fork */
		if (verbose & V_SYS) {
			pid(); printf("fork\n");
		}
		i = fork();
		if (i) {
			if (verbose & V_SYS) {
				pid(); printf("child pid=%d\n", i);
			}
			cp->state.registers.word[Z80_HL] = i;
			push(pop() + 3);
		} else {
			mypid = getpid();
			if (verbose & V_SYS) {
				pid(); printf("child process\n", i);
			}
			cp->state.registers.word[Z80_HL] = 0;
		}
		carry_clear();
		break;

	case 3: /* read (hl), buffer, len */
		if ((df = dirget(fd))) {
			i = df->bufsize - df->offset;
			if (arg2 < i) {
				i = arg2;
			}
			bcopy(&df->buffer[df->offset], &cp->memory[arg1], i);
			df->offset += i;
		} else {
			i = read(fd, &cp->memory[arg1], arg2);
		}
		if (verbose & V_SYS) {
			pid(); printf("read(%d, %04x, %d) = %d\n", 
				fd, arg1, arg2, i);
		}
		if (verbose & V_SYS1) dumpmem(&get_byte, arg1, i);
		cp->state.registers.word[Z80_HL] = i;
		carry_clear();
		break;

	case 4: /* write (hl), buffer, len */
		i = write(fd, &cp->memory[arg1], arg2);
		if (verbose & V_SYS) {
			pid(); printf("write(%d, %04x, %d) = %d\n", 
				fd, arg1, arg2, i);
		}
		if (verbose & V_SYS1) dumpmem(&get_byte, arg1, arg2);
		cp->state.registers.word[Z80_HL] = i;
		carry_clear();
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
		if (!stat(fn, &sbuf)) {
			if (S_ISDIR(sbuf.st_mode)) {
				fd = dirsnarf(fn);
				if (fd == 0xffff) {
					goto lose;
				}
			} else { 
				fd = open(fn, arg2);
			}
			cp->state.registers.word[Z80_HL] = fd;
			carry_clear();
		} else {
			lose:
			cp->state.registers.word[Z80_HL] = errno;
			carry_set();
		}
		if (verbose & V_SYS) {
			pid(); printf("open(\"%s\", %04x)=%d\n", fn, arg2, fd);
		}
		break;

	case 6:	/* close */
		if (verbose & V_SYS) {
			pid(); printf("close %d\n", fd);
		}
		dirclose(fd);
		close(fd);
		carry_clear();
		break;

	case 7: /* wait */
		if (verbose & V_SYS) {
			pid(); printf("wait\n", fd);
		}
		if ((i = wait(&fd)) == -1) {
			if (verbose & V_SYS) { pid(); printf("no children\n"); }
			carry_set();
			break;
		}
		if (verbose & V_SYS) {
			pid(); printf("wait ret %x\n", fd);
		}
		cp->state.registers.byte[Z80_D] = WEXITSTATUS(fd);
		cp->state.registers.byte[Z80_E] = 0;
		if (WIFSIGNALED(fd)) {
			cp->state.registers.byte[Z80_D] = 1;
			cp->state.registers.byte[Z80_E] = WTERMSIG(fd);
		}
		cp->state.registers.word[Z80_HL] = i;
		carry_clear();
		break;

	case 8:	/* creat <name> <mode> */
		i = creat(fn, arg2);
		if (verbose & V_SYS) {
			pid(); printf("creat(%s, %o) = %d\n", 
				fn, arg2, i);
		}
		if (i == -1) {
			carry_set();
		} else {
			carry_clear();
		}
		cp->state.registers.word[Z80_HL] = i;
		break;

	case 9:	/* link <old> <new> */
		if (verbose & V_SYS) {
			pid(); printf("XXX - link(%s, %s)\n", 
				fn, fname(&cp->memory[arg2]));
		}
		carry_set();
		break;

	case 10:	/* unlink <file> */
		i = unlink(fn);
		if (verbose & V_SYS) {
			pid(); printf("unlink(%s) = %d\n", fn, i);
		}
		if (i != 0) {
			carry_set();
		} else {
			carry_clear();
		}
		break;

	case 11: /* exec */
		if (verbose & V_SYS) {
			pid(); printf("exec(%s %04x)\n", fn, arg2);
			if (verbose & V_SYS1) dumpmem(&get_byte, arg2, 32);
		}
		/* let's count our args */
		i = 0;
		while (get_word(arg2 + (i * 2))) i++;
		argvec = malloc((i + 1) * sizeof (char *));
		i = 0;
		for (i = 0; (arg1 = get_word(arg2 + (i * 2))); i++) {
			argvec[i] = strdup(&cp->memory[arg1]);
		}
		argvec[i] = 0;	
		if (do_exec(fn, argvec)) {
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
		fn = &cp->memory[arg1];
		if (*fn == '/') {
			fn = fname(&cp->memory[arg1]);
		}
		if (verbose & V_SYS) {
			pid(); printf("chdir(%s)\n", fn);
		}
		strcpy(curdir, fn);
		chdir(fn);
		break;
	case 13:	/* time */
		i = time(0);
		cp->state.registers.word[Z80_DE] = i & 0xffff;
		cp->state.registers.word[Z80_HL] = (i >> 16) & 0xffff;
		carry_clear();
		if (verbose & V_SYS) {
			pid(); printf("r_time %x %04x %04x\n", 
				i, (i >> 16) & 0xffff, i & 0xffff);
		}
		break;
	case 14:	/* mknod <name> mode dev (dev == 0) for dir */
		fn = fname(&cp->memory[arg1]);
		if (verbose & V_SYS) {
			pid();
			printf("XXX - mknod(%s, %o, %x)\n", fn, arg2, arg3);
		}
		carry_set();
		break;

	case 15:	/* chmod <name> <mode> */
		fn = fname(&cp->memory[arg1]);
		if (verbose & V_SYS) {
			pid();
			printf("XXX - chmod(%s, %o)\n", fn, arg2);
		}
		carry_set();
		break;

	case 16:	/* chown <name> <mode> */
		fn = fname(&cp->memory[arg1]);
		if (verbose & V_SYS) {
			pid();
			printf("XXX - chown(%s %04x)\n", fn, arg2);
		}
		carry_set();
		break;

	case 17:	/* sbrk <addr> */
		if (verbose & V_SYS) {
			pid();
			printf("sbrk(%04x)\n", arg1);
		}
		brake = arg1;
		carry_clear();
		break;

	case 18:	/* stat fn buf */
	case 28:	/* fstat fd buf */
		if (code == 28) {
			if (verbose & V_SYS) {
				pid(); printf("fstat(%d, %04x)\n", fd, arg1);
			}
			i = fstat(fd, &sbuf);
			if ((df = dirget(fd))) {
				sbuf.st_size = df->end;
			}
			arg2 = arg1;
		} else {
			if (verbose & V_SYS) {
				pid(); printf("stat(\"%s\", %04x)\n", fn, arg2);
			}
			i = stat(fn, &sbuf);
		}
		if (i) {
			perror("stat failed");
			cp->state.registers.word[Z80_HL] = errno;
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
			if (verbose & V_SYS1)
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
		if (verbose & V_SYS) {
			pid(); printf("seek(%d, %d, %d) = %d\n", 
				fd, arg1, arg2, i);
		}
		cp->state.registers.word[Z80_HL] = (i >> 16) & 0xffff;
		carry_clear();
		break;

	case 20:	/* getpid */
		if (verbose & V_SYS) {
			pid(); printf("getpid\n");
		}
		cp->state.registers.word[Z80_HL] = getpid();
		carry_clear();
		break;

	case 21:	/* mount */
		if (verbose & V_SYS) {
			pid(); printf("XXX - mount(%s %s)\n", 
				fn, fname(&cp->memory[arg2]));
		}
		carry_set();
		break;

	case 22:	/* umount */
		if (verbose & V_SYS) {
			pid(); printf("XXX - umount(%s)\n", fn);
		}
		carry_set();
		break;

	case 23:	/* setuid */
		if (verbose & V_SYS) {
			pid(); printf("setuid %d\n", fd);
		}
		carry_set();
		break;

	case 24:	/* getuid */
		if (verbose & V_SYS) {
			pid(); printf("getuid\n");
		}
		cp->state.registers.word[Z80_HL] = getuid();
		carry_clear();
		break;

	case 25:	/* stime */
		if (verbose & V_SYS) {
			pid(); printf("XXX - stime(%04x, %04x)\n", 
				fd, cp->state.registers.word[Z80_DE]);
		}
		carry_set();
		break;

	case 31:	/* stty */
		if (verbose & V_SYS) {
			pid(); printf("XXX - stty %d %04x\n", fd, arg1);
		}
		carry_set();
		break;

	case 32:	/* gtty */
		if (verbose & V_SYS) {
			pid(); printf("gtty %d %04x\n", fd, arg1);
		}
		if (tcgetattr(fd, &ti)) {
			carry_set();
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
		i = access(fn, i);
		if (verbose & V_SYS) {
			pid(); printf("access(%s, %o) = %d\n", 
				fn, arg2, i);
		}
		if (i == -1) {
			carry_set();
		} else {
			carry_clear();
		}
		cp->state.registers.word[Z80_HL] = i;
		break;
		if (verbose & V_SYS) {
		}
	case 34:	/* nice */
		if (verbose & V_SYS) {
			pid(); printf("nice\n");
		}
		carry_clear();
		break;

	case 35:	/* sleep */
		if (verbose & V_SYS) {
			pid(); printf("sleep %d\n", fd);
		}
		sleep(fd);
		carry_clear();
		break;

	case 36:	/* sync */
		if (verbose & V_SYS) {
			pid(); printf("sync\n");
		}
		carry_clear();
		break;

	case 37:	/* kill <pid in hl> signal */
		if (verbose & V_SYS) {
			pid(); printf("XXX - kill %d %d\n", fd, arg1);
		}
		carry_set();
		break;

	case 41:
		i = dup(fd);
		if (verbose & V_SYS) {
			pid();
			printf("dup(%d) = %d\n", fd, i);
		}
		if (i != -1) {
			cp->state.registers.word[Z80_HL] = i;
			carry_clear();
		} else {
			carry_set();
		}
		break;

	case 42:
		if (verbose & V_SYS) {
			pid();
			printf("pipe() ");
		}
		if ((i = pipe(p))) {
			if (verbose & V_SYS) printf("failed\n");
			carry_set();
		} else {
			if (verbose & V_SYS) printf(" = (in: %d, out: %d)\n",
				p[0], p[1]);
			cp->state.registers.word[Z80_HL] = p[0];
			cp->state.registers.word[Z80_DE] = p[1];
			carry_clear();
		}
		break;

	case 48:
		i = arg1;
		if (i > 15) i = 0;
		if (verbose & V_SYS) {
			pid();
			printf("signal(%s, %04x, %04x)\n", 
				signame[i], arg1, arg2);
		}
		carry_clear();
		break;
	default:
		pid();
		printf("unrecognized syscall %d %x\n", code, code);
		carry_set();
		break;
	}
	
	cp->state.pc = pop();
	cp->state.status = 0;
}
