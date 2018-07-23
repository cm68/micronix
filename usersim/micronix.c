/* micronix.
 * this emulates the micronix user mode
 *
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

typedef unsigned int ULONG;
typedef unsigned char UCHAR;
typedef unsigned short UINT;
#include "../kernel/inode.h"
#include "../include/obj.h"

#define MAXIMUM_STRING_LENGTH   100

static int	do_exec(char *name, char**argv);
static void	emulate();

#define	DEFROOT	"../filesystem"

int mypid;

char *curdir = "/";
char *rootdir = 0;

char *initfile[] = {
	"/etc/init",
	0
};

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

int 
main(int argc, char **argv)
{
	char *d;
	char **argvec;
	int i;

	while (--argc) {
		argv++;
		if (**argv == '-') {
			while (*++*argv) switch (**argv) {
			case 'd':
				--argc;
				rootdir = *++argv;
				if (**argv) while ((*argv)[1])
					(*argv)++;
				break;
			case 't':
				trace++;
				break;
			case 'v':
				verbose++;
				trace++;
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
	if (verbose) { 
		printf("verbose %d\n", verbose);
		printf("emulating %s with root %s\n", argv[0], rootdir);
	}

	cp = &context;
        Z80Reset(&cp->state);

        cp->state.pc = 0x100;

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

	emulate();
        return EXIT_SUCCESS;
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

unsigned short
get_word(int addr)
{
	return cp->memory[addr & 0xffff] + 
		(cp->memory[(addr+1) & 0xffff] << 8);
}

unsigned char
get_byte(int addr)
{
	return cp->memory[addr & 0xffff];
}

static void
pushs(unsigned short s)
{
	// printf("push %04x\n", s);
	cp->memory[--cp->state.registers.word[Z80_SP]] = (s >> 8) & 0xff;
	cp->memory[--cp->state.registers.word[Z80_SP]] = s & 0xff;
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

	if (verbose > 1) {
	printf("exec header: magic: %x conf: %x symsize: %d text: %d data: %d bss: %d heap: %d textoff: %x dataoff: %x\n",
		header.ident, header.conf, 
		header.table, header.text, header.data, 
		header.bss, header.heap, 
		header.textoff, header.dataoff);
	}

	fread(cp->memory + header.textoff, 1, header.text, file);
	fread(cp->memory + header.dataoff, 1, header.data, file);

        fclose(file);

	cp->memory[8] = 0x76;
        cp->state.pc = header.textoff;
        cp->state.registers.word[Z80_SP] = 0xefff;
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
	pushs(0xffff);
	for (i = 0; i < argc; i++) {
		pushs(ao[argc - (i+1)]);
	}
	pushs(argc);
	if (verbose > 1) dumpmem(&get_byte, cp->state.registers.word[Z80_SP], 256);
	return 0;
}

unsigned char
getsim(int addr)
{
	return *(unsigned char *)addr;
}

dumpinst()
{
	char outbuf[40];

	format_instr(cp->state.pc, outbuf, &get_byte, &get_symname, &reloc);
	printf("%04x: %-30s ", cp->state.pc, outbuf);
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
	printf("pc:%04x a:%02x bc:%04x de:%04x hl:%04x sp:%04x status:%d break:%04x ",
		cp->state.pc,
		cp->state.registers.byte[Z80_A],
		cp->state.registers.word[Z80_BC],
		cp->state.registers.word[Z80_DE],
		cp->state.registers.word[Z80_HL],
		cp->state.registers.word[Z80_SP],
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
		if (verbose > 2) {
			pid();
			dumpinst();
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
	unsigned short calladdr;
	unsigned short sp = cp->state.registers.word[Z80_SP];
	unsigned char bytes = 0;	/* amount we need to adjust ret */
	char indirect = 0;
	unsigned short addr, addr2;
	struct stat sbuf;
	struct inode *ip;
	int i;
	int fd;
	struct dirfd *df;
	char *fn;
	char **argvec;

	if (verbose > 1) {
		pid();
		dumpcpu();
	}
	calladdr = get_byte(sp) + (get_byte(sp+1) << 8) - 1;

	if ((code = get_byte(calladdr)) != 0xcf) {
		pid();
		dumpcpu();
		printf("halt no syscall %d %x!\n", code, calladdr);
		return;
	}
	bytes++;
	code=get_byte(calladdr + bytes);

	if (code == 0) { /* need to fetch the address */
		bytes += 2;
		indirect++;
		calladdr = get_byte(calladdr+2) + (get_byte(calladdr+3) << 8);
		if ((code = get_byte(calladdr)) != 0xcf) {
			printf("indir no syscall %d %x!\n", code, calladdr);
		}
		code = get_byte(calladdr+1);
	}

	/* let's build some common args */
	addr = get_byte(calladdr+2) + (get_byte(calladdr+3) << 8);
	addr2 = get_byte(calladdr+4) + (get_byte(calladdr+5) << 8);
	fd = cp->state.registers.word[Z80_HL];

	if (verbose) {
		pid();
		printf("%ssyscall %d %04x %04x %04x\n", 
			indirect ? "indirect " : "", code,
			fd, addr, addr2);
	}

	switch (code) {
	case 0:
		pid();
		printf("double indirect! yarg!\n");
		break;		
	case 1: /* exit */
		if (trace) {
			pid();
			printf("exit %04x %04x\n", fd, addr);
		}
		exit(0);
		break;
	case 2: /* fork */
		if (trace) {
			pid();
			printf("fork\n");
		}
		i = fork();
		if (i) {
			if (trace) {
				pid();
				printf("child pid=%d\n", i);
			}
			cp->state.registers.word[Z80_HL] = i;
			i = get_byte(sp) + get_byte(sp+1) << 8;
			i += 3;
			cp->memory[sp] = i & 0xff;
			cp->memory[sp+1] = (i >> 8) & 0xff;
		} else {
			mypid = getpid();
			if (trace) {
				pid();
				printf("child process\n", i);
			}
			cp->state.registers.word[Z80_HL] = 0;
		}
		carry_clear();
		break;
	case 3: /* read */
		if (trace) {
			pid();
			printf("read fd:%d %04x %04x\n", fd, addr, addr2);
		}
		if ((df = dirget(fd))) {
			i = df->bufsize - df->offset;
			if (addr2 < i) {
				i = addr2;
			}
			bcopy(&df->buffer[df->offset], &cp->memory[addr], i);
			df->offset += i;
		} else {
			i = read(fd, &cp->memory[addr], addr2);
		}
		if (verbose > 1) dumpmem(&get_byte, addr, i);
		bytes += 4;
		cp->state.registers.word[Z80_HL] = i;
		carry_clear();
		break;
	case 4: /* write */
		if (trace) {
			pid();
			printf("write fd:%d %04x %04x\n", fd, addr, addr2);
		}
		if (verbose > 1) dumpmem(&get_byte, addr, addr2);
		i = write(fd, &cp->memory[addr], addr2);
		cp->state.registers.word[Z80_HL] = i;
		bytes += 4;
		carry_clear();
		break;
	case 5: /* open */
		if (trace) {
			pid();
		}
		fn = fname(&cp->memory[addr]);
		bytes += 4;
		switch (addr2) {
		case 0:
			addr2 = O_RDONLY;
			break;
		case 1:
			addr2 = O_WRONLY;
			break;
		case 2:
			addr2 = O_RDWR;
			break;
		default:
			printf("open busted mode %s %04x %04x\n",
				&cp->memory[addr], addr, addr2);
			break;
		}
		if (!stat(fn, &sbuf)) {
			if (S_ISDIR(sbuf.st_mode)) {
				fd = dirsnarf(fn);
				if (fd == -1) {
					goto lose;
				}
			} else { 
				fd = open(fn, addr2);
			}
			cp->state.registers.word[Z80_HL] = fd;
			carry_clear();
		} else {
			perror("stat failed\n");
			lose:
			cp->state.registers.word[Z80_HL] = errno;
			carry_set();
		}
		if (trace) printf("open(\"%s\", %04x = %d\n", fn, addr2, fd);
		break;
	case 6:	/* close */
		if (trace) {
			pid();
			printf("close %d\n", fd);
		}
		dirclose(fd);
		close(fd);
		bytes += 2;
		break;
	case 11: /* exec */
		fn = fname(&cp->memory[addr]);
		if (trace) {
			pid();
			printf("exec %s %04x\n", fn, addr2);
			dumpmem(&get_byte, addr2, 32);
		}
		/* let's count our args */
		i = 0;
		while (get_word(addr2 + (i * 2))) i++;
		argvec = malloc((i + 1) * sizeof (char *));
		i = 0;
		for (i = 0; (addr = get_word(addr2 + (i * 2))); i++) {
			argvec[i] = strdup(&cp->memory[addr]);
		}
		argvec[i] = 0;	
		if (do_exec(fn, argvec)) {
			carry_set();
			break;
		}
		for (i = 0; argvec[i]; i++) {
			free(argvec[i]);
		}
		free(argvec);
		carry_clear();
		break;

	case 12: /* chdir */
		fn = fname(&cp->memory[addr]);
		if (trace) {
			pid();
			printf("chdir %s\n", fn);
		}
		curdir = fn;
		bytes += 2;
		break;
	case 13:	/* r_time */
		i = time(0);
		cp->state.registers.word[Z80_DE] = i & 0xffff;
		cp->state.registers.word[Z80_HL] = (i >> 16) & 0xffff;
		carry_clear();
		if (trace) {
			pid();
			printf("r_time %x %04x %04x\n", 
				i, (i >> 16) & 0xffff, i & 0xffff);
		}
		break;
	case 17:	/* sbrk */
		if (trace) {
			pid();
			printf("sbrk %04x\n", addr);
		}
		brake = addr;
		carry_clear();
		bytes += 2;
		break;	
	case 18:
	case 28:
		if (code == 28) {
			if (trace) {
				pid();
				printf("fstat(%d, %04x)\n", fd, addr);
			}
			i = fstat(fd, &sbuf);
			if ((df = dirget(fd))) {
				sbuf.st_size = df->end;
			}
			addr2 = addr;
		} else {
			fn = fname(&cp->memory[addr]);
			if (trace) {
				pid();
				printf("stat(\"%s\", %04x)\n", fn, addr2);
			}
			i = stat(fn, &sbuf);
		}
		if (i) {
			perror("stat failed");
			cp->state.registers.word[Z80_HL] = errno;
			carry_set();
		} else {
			ip = (struct inode *)&cp->memory[addr2];
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
				dumpmem(&get_byte, addr2, 36);
			carry_clear();
		}
		bytes += 2;
		break;
	case 19:
		if (trace) {
			pid();
			printf("seek fd:%d %04x %04x\n", fd, addr, addr2);
		}
		i = (short)addr;
		if (addr2 > 2) {
			i *= 512;
			addr2 -= 3;
		}
		if ((df = dirget(fd))) {
			switch (addr2) {
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
			switch (addr2) {
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
		cp->state.registers.word[Z80_HL] = (i >> 16) & 0xffff;
		carry_clear();
		bytes += 2;
		break;
	case 20:
		if (trace) {
			pid();
			printf("getpid fd:%d %04x %04x\n", fd, addr, addr2);
		}
		cp->state.registers.word[Z80_HL] = getpid();
		carry_clear();
		break;
	case 24:
		if (trace) {
			pid();
			printf("getuid %04x %04x\n", fd, addr, addr2);
		}
		cp->state.registers.word[Z80_HL] = getuid();
		carry_clear();
		break;
	case 36:
		if (trace) {
			pid();
			printf("sync\n");
		}
		carry_clear();
		break;
	case 32:
		if (trace) {
			pid();
			printf("gtty %04x\n", addr);
		}
		bytes += 2;
		carry_clear();
		break;
	case 48:
		i = addr;
		if (i > 15) i = 0;
		if (trace) {
			pid();
			printf("signal %s %04x %04x\n", 
				signame[i], addr, addr2);
		}
		carry_clear();
		break;
	default:
		pid();
		printf("unrecognized syscall %d %x\n", code, code);
		carry_set();
		break;
	}
	
	/* this is a return */
	if (indirect) {
		bytes = 3;
	}
	cp->state.pc = get_byte(sp) + (get_byte(sp+1) << 8) + bytes;
	cp->state.registers.word[Z80_SP] += 2;
	cp->state.status = 0;
}
