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

typedef unsigned int ULONG;
typedef unsigned char UCHAR;
typedef unsigned short UINT;
#include "../kernel/inode.h"
#include "../include/obj.h"

#define	STACKTOP	0xffff
#define MAXIMUM_STRING_LENGTH   100

static int	do_exec(char *name, char**argv);
static void	emulate();

#define	DEFROOT	"../filesystem"

int mypid;

char curdir[100] = "";
char *rootdir = 0;

char *initfile[] = {
	"/bin/sh",
	0
};

int inst;
int verbose;
int trace;
MACHINE	context;

unsigned short brake;

struct MACHINE *cp;

/*
 * translate our sim filename into a native filename
 * by prepending the root - we need to be a little smart
 * about this because relative paths need to first have
 * the current working directory prepended
 */
char *fname(char *orig)
{
	char namebuf[PATH_MAX];
	if (*orig == '/') {
		sprintf(namebuf, "%s/%s", rootdir, orig);
	} else {
		sprintf(namebuf, "%s/%s/%s", rootdir, curdir, orig);
	}
	if (verbose > 2) printf("fname: %s new: %s\n", orig, namebuf);
	return (namebuf);
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

int 
main(int argc, char **argv)
{
	char *progname = argv[0];
	char *d;
	char **argvec;
	int i;

	setvbuf(stdout, 0, _IONBF, 0);

	while (--argc) {
		argv++;
		if (**argv == '-') {
			while (*++*argv) switch (**argv) {
			case 'h':
				printf("usage: %s [<options>] program\n", 
					progname);
				printf("\t-d <root dir>\n");
				printf("\t-i trace instructions\n");
				printf("\t-t trace system calls\n");
				printf("\t-v increase verbosity\n");
				exit(1);	
			case 'd':
				--argc;
				rootdir = *++argv;
				if (**argv) while ((*argv)[1])
					(*argv)++;
				break;
			case 'i':
				inst++;
				break;
			case 't':
				trace++;
				break;
			case 'v':
				verbose++;
				break;
			default:
				printf("bad flag %c\n", (**argv));
				break; 
			}
		} else {
			break;
		}
	}
	if (!argc) {
		argc = 1;
		argv = initfile;
	}

	if (!rootdir) {
		rootdir = DEFROOT;
	}

	if (*rootdir != "/") {
		d = malloc(PATH_MAX);
		getwd(d);
		strcat(d, "/");
		strcat(d, rootdir);
		rootdir = d;
	}
	if (verbose+trace+inst) { 
		printf("verbose %d trace %d inst %d\n", verbose, trace, inst);
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
	mypid = getpid();

        cp->state.pc = pop();
	emulate();
        return EXIT_SUCCESS;
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

	/* count our args from the null-terminated list */
	for (argc = 0; argv[argc]; argc++) {
		if (verbose > 2) {
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

	if (verbose) {
	printf("exec header: magic: %x conf: %x symsize: %d text: %d data: %d bss: %d heap: %d textoff: %x dataoff: %x\n",
		header.ident, header.conf, 
		header.table, header.text, header.data, 
		header.bss, header.heap, 
		header.textoff, header.dataoff);
	}

	fread(cp->memory + header.textoff, 1, header.text, file);
	fread(cp->memory + header.dataoff, 1, header.data, file);

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
	if (verbose > 1) dumpmem(&get_byte, cp->state.registers.word[Z80_SP], 256);
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

	pid();
	format_instr(cp->state.pc, outbuf, &get_byte, &get_symname, &reloc);
	printf("%04x: %-20s ", cp->state.pc, outbuf);
	printf("pc:%04x a:%02x bc:%04x de:%04x hl:%04x sp:%04x (%04x) status:%d break:%04x ",
		cp->state.pc,
		cp->state.registers.byte[Z80_A],
		cp->state.registers.word[Z80_BC],
		cp->state.registers.word[Z80_DE],
		cp->state.registers.word[Z80_HL],
		cp->state.registers.word[Z80_SP],
		get_word(cp->state.registers.word[Z80_SP]),
		cp->state.status,
		brake);

	f = cp->state.registers.byte[Z80_F];
	
	if (f & Z80_C_FLAG) printf("C"); else  printf(" ");
	if (f & Z80_N_FLAG) printf("N"); else  printf(" ");
	if (f & Z80_X_FLAG) printf("X"); else  printf(" ");
	if (f & Z80_H_FLAG) printf("H"); else  printf(" ");
	if (f & Z80_Y_FLAG) printf("Y"); else  printf(" ");
	if (f & Z80_Z_FLAG) printf("Z"); else  printf(" ");
	if (f & Z80_S_FLAG) printf("S"); else  printf(" ");
	printf("\n");
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

	do {
		if (inst) {
			dumpcpu();
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
int
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
	if (verbose > 1) dumpmem(&getsim, df->buffer, df->bufsize);	
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

	if (trace && (verbose > 1)) {
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

	if (verbose > 1) dumpmem(&get_byte, sc, argbytes[code] + 1);

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

	if (trace && verbose) {
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
		if (trace) {
			pid(); printf("exit %04x\n", fd);
		}
		exit(fd);
		break;

	case 2:		/* fork */
		if (trace) {
			pid(); printf("fork\n");
		}
		i = fork();
		if (i) {
			if (trace) {
				pid(); printf("child pid=%d\n", i);
			}
			cp->state.registers.word[Z80_HL] = i;
			push(pop() + 3);
		} else {
			mypid = getpid();
			if (trace) {
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
		if (trace) {
			pid(); printf("read(%d, %04x, %d) = %d\n", 
				fd, arg1, arg2, i);
		}
		if (verbose > 1) dumpmem(&get_byte, arg1, i);
		cp->state.registers.word[Z80_HL] = i;
		carry_clear();
		break;

	case 4: /* write (hl), buffer, len */
		i = write(fd, &cp->memory[arg1], arg2);
		if (trace) {
			pid(); printf("write(%d, %04x, %d) = %d\n", 
				fd, arg1, arg2, i);
		}
		if (verbose > 1) dumpmem(&get_byte, arg1, arg2);
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
				if (fd == -1) {
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
		if (trace) {
			pid(); printf("open(\"%s\", %04x)=%d\n", fn, arg2, fd);
		}
		break;

	case 6:	/* close */
		if (trace) {
			pid(); printf("close %d\n", fd);
		}
		dirclose(fd);
		close(fd);
		carry_clear();
		break;

	case 7: /* wait */
		if (trace) {
			pid(); printf("wait\n", fd);
		}
		if ((i = wait(&fd)) == -1) {
			pid(); printf("no children\n");
			carry_set();
			break;
		}
		if (trace) {
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
		if (trace) {
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
		if (trace) {
			pid(); printf("XXX - link(%s, %s)\n", 
				fn, fname(&cp->memory[arg2]));
		}
		carry_set();
		break;

	case 10:	/* unlink <file> */
		i = unlink(fn);
		if (trace) {
			pid(); printf("unlink(%s) = %d\n", fn, i);
		}
		if (i != 0) {
			carry_set();
		} else {
			carry_clear();
		}
		break;

	case 11: /* exec */
		if (trace) {
			pid(); printf("exec(%s %04x)\n", fn, arg2);
			if (verbose) dumpmem(&get_byte, arg2, 32);
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
		if (trace) {
			pid(); printf("chdir(%s)\n", fn);
		}
		strcpy(curdir, fn);
		break;
	case 13:	/* time */
		i = time(0);
		cp->state.registers.word[Z80_DE] = i & 0xffff;
		cp->state.registers.word[Z80_HL] = (i >> 16) & 0xffff;
		carry_clear();
		if (trace) {
			pid(); printf("r_time %x %04x %04x\n", 
				i, (i >> 16) & 0xffff, i & 0xffff);
		}
		break;
	case 14:	/* mknod <name> mode dev (dev == 0) for dir */
		fn = fname(&cp->memory[arg1]);
		if (trace) {
			pid();
			printf("XXX - mknod(%s, %o, %x)\n", fn, arg2, arg3);
		}
		carry_set();
		break;

	case 15:	/* chmod <name> <mode> */
		fn = fname(&cp->memory[arg1]);
		if (trace) {
			pid();
			printf("XXX - chmod(%s, %o)\n", fn, arg2);
		}
		carry_set();
		break;

	case 16:	/* chown <name> <mode> */
		fn = fname(&cp->memory[arg1]);
		if (trace) {
			pid();
			printf("XXX - chown(%s %04x)\n", fn, arg2);
		}
		carry_set();
		break;

	case 17:	/* sbrk <addr> */
		if (trace) {
			pid();
			printf("sbrk(%04x)\n", arg1);
		}
		brake = arg1;
		carry_clear();
		break;

	case 18:	/* stat fn buf */
	case 28:	/* fstat fd buf */
		if (code == 28) {
			if (trace) {
				pid(); printf("fstat(%d, %04x)\n", fd, arg1);
			}
			i = fstat(fd, &sbuf);
			if ((df = dirget(fd))) {
				sbuf.st_size = df->end;
			}
			arg2 = arg1;
		} else {
			if (trace) {
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
			if (verbose > 1)
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
		if (trace) {
			pid(); printf("seek(%d, %d, %d) = i\n", 
				fd, arg1, arg2, i);
		}
		cp->state.registers.word[Z80_HL] = (i >> 16) & 0xffff;
		carry_clear();
		break;

	case 20:	/* getpid */
		if (trace) {
			pid(); printf("getpid\n");
		}
		cp->state.registers.word[Z80_HL] = getpid();
		carry_clear();
		break;

	case 21:	/* mount */
		if (trace) {
			pid(); printf("XXX - mount(%s %s)\n", 
				fn, fname(&cp->memory[arg2]));
		}
		carry_set();
		break;

	case 22:	/* umount */
		if (trace) {
			pid(); printf("XXX - umount(%s)\n", fn);
		}
		carry_set();
		break;

	case 23:	/* setuid */
		if (trace) {
			pid(); printf("setuid %d\n", fd);
		}
		carry_set();
		break;

	case 24:	/* getuid */
		if (trace) {
			pid(); printf("getuid\n");
		}
		cp->state.registers.word[Z80_HL] = getuid();
		carry_clear();
		break;

	case 25:	/* stime */
		if (trace) {
			pid(); printf("XXX - stime(%04x, %04x)\n", 
				fd, cp->state.registers.word[Z80_DE]);
		}
		carry_set();
		break;

	case 31:	/* stty */
		if (trace) {
			pid(); printf("XXX - stty %d %04x\n", fd, arg1);
		}
		carry_set();
		break;

	case 32:	/* gtty */
		if (trace) {
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
		if (trace) {
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
		if (trace) {
		}
	case 34:	/* nice */
		if (trace) {
			pid(); printf("nice\n");
		}
		carry_clear();
		break;

	case 35:	/* sleep */
		if (trace) {
			pid(); printf("sleep %d\n", fd);
		}
		sleep(fd);
		carry_clear();
		break;

	case 36:	/* sync */
		if (trace) {
			pid(); printf("sync\n");
		}
		carry_clear();
		break;

	case 37:	/* kill <pid in hl> signal */
		if (trace) {
			pid(); printf("XXX - kill %d %d\n", fd, arg1);
		}
		carry_set();
		break;

	case 41:
		i = dup(fd);
		if (trace) {
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
		if (trace) {
			pid();
			printf("pipe() ");
		}
		if ((i = pipe(p))) {
			if (trace) printf("failed\n");
			carry_set();
		} else {
			if (trace) printf(" = (in: %d, out: %d)\n",
				p[0], p[1]);
			cp->state.registers.word[Z80_HL] = p[0];
			cp->state.registers.word[Z80_DE] = p[1];
			carry_clear();
		}
		break;

	case 48:
		i = arg1;
		if (i > 15) i = 0;
		if (trace) {
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
