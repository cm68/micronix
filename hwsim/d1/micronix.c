/* 
 * micronix system call tracer
 *
 * Copyright (c) 2018, Curt Mayer
 * do whatever you want, just don't claim you wrote it.
 * warrantee:  madness!  nope.
 */

#define	_GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <string.h>
#include <sys/time.h>
#include <signal.h>
#include "sim.h"

extern byte fubyte(word addr);
extern word fuword(word addr);

char *signame[] = {
	"bogus",
	"hup",
	"int",
	"quit",
	"ill",
	"trace",
	"bg",
	"termio",
	"fpe",
	"kill",
	"bus",
	"segv",
	"sysarg",
	"pipeerr",
	"alarm",
	"term"
};

char bounce[1000];
int bnum = 0;

char *
getname(word addr)
{
	char *s, *r;
	
	r = s = &bounce[bnum * 500];
	bnum = (bnum + 1) % 2;

	while ((*s++ = fubyte(addr++)))
		;
		
	return r;
}

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
void 
syscall_at(word sc)
{
	unsigned char code;
	char indirect = 0;

	word fd;		/* from hl */
	word arg1;	/* first arg */
	word arg2;	/* second arg */
	word arg3;	/* third arg */
	word arg4;	/* fourth arg */

	char *fn;
	char *fn2;
	unsigned short ret;
	struct syscall *sp;

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
	if (((fubyte(sc)) != 0xcf) && (fubyte(sc - 2) != 0xcd)) {
		printf("halt no syscall %x!\n", sc);
		return;
	}

	/* get the function code */
	code = fubyte(sc + 1);

	/* this is an indirect call - the argument points at a syscall */
	if (code == 0) {
		indirect++;
		sc = fuword(sc + 2);
		if ((code = fubyte(sc)) != 0xcf) {
			printf("indir no syscall %d %x!\n", code, sc);
			return;
		}
		code = fubyte(sc + 1);
	}

	sp = &syscalls[code];
	printf("micronix syscall %10s %3d ", sp->name, z80_get_reg16(hl_reg));
	dumpmem(&fubyte, sc, sp->argbytes + 1);

	if (sp->flag & (SF_ARG1|SF_NAME)) arg1 = fuword(sc+2);
	if (sp->flag & (SF_ARG2|SF_NAME2)) arg2 = fuword(sc+4);
	if (sp->flag & SF_ARG3) arg3 = fuword(sc+6);
	if (sp->flag & SF_ARG4) arg3 = fuword(sc+8);
	if (sp->flag & SF_FD) fd = z80_get_reg16(hl_reg);
	if (sp->flag & SF_NAME) fn = getname(arg1);
	if (sp->flag & SF_NAME2) fn2 = getname(arg2);
#ifdef notdef

	/* what's with these zero length writes? */
	if ((code == 4) && (arg2 == 0)) {
		goto nolog;
		verbose = -1;
		breakpoint = 1;
	}

	if ((verbose & V_SYS) && (!(sp->flag & SF_SMALL) || (verbose & V_ASYS))) {
		pid(); fprintf(mytty,"%s(", sp->name);
		i = sp->flag & (SF_FD|SF_ARG1|SF_NAME|SF_ARG2|SF_NAME2|SF_ARG2|SF_ARG3|SF_ARG4);
#define F(b, f, a) \
	if (i & (b)) { i ^= (b) ; fprintf(mytty,f,a,i?",":""); }
		F(SF_FD, "%d%s", fd);
		F(SF_ARG1, "%04x%s", arg1);
		F(SF_NAME, "\"%s\"%s", fn);
		F(SF_ARG2, "%04x%s", arg2);
		F(SF_NAME2, "\"%s\"%s", fn2);
		F(SF_ARG3, "%04x%s", arg3);
		F(SF_ARG4, "%04x%s", arg4);
		fprintf(mytty,") ");
	}
nolog:
#endif

	switch (code) {
	case 0:	/* double indirect is a no-op */
		printf("double indirect syscall!\n");
		break;		

	case 1:		/* exit (hl) */
		printf("exit %d\n", fd);
		break;

	case 2:		/* fork */
		printf("fork\n");
		break;

	case 3: /* read (hl), buffer, len */
		printf("read(%d, %x, %d)\n", fd, arg1, arg2);
		break;

	case 4: /* write (hl), buffer, len */
		printf("write(%d, %x, %d)\n", fd, arg1, arg2);
		break;

	case 5: /* open */
		printf("open(\"%s\", %x)\n", fn, arg2);
		break;

	case 6:	/* close */
		printf("close(%d)\n", fd);
		break;
	case 7: /* wait */
		printf("wait(%d)\n", fd);
		break;

	case 8:	/* creat <name> <mode> */
		printf("creat(\"%s\", %x)\n", fn, arg2);
		break;

	case 9:	/* link <old> <new> */
		printf("link(\"%s\", \"%s\")\n", fn, fn2);
		break;

	case 10:	/* unlink <file> */
		printf("unlink(\"%s\")\n", fn);
		break;

	case 11: /* exec */
		printf("exec(\"%s\")\n", fn);
		break;
#ifdef notdef

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
#endif
	case 15:	/* chmod <name> <mode> */
		printf("chmod(\"%s\", %x)\n", fn, arg2);
		break;

	case 16:	/* chown <name> <mode> */
		printf("chown(\"%s\", %x)\n", fn, arg2);
		break;
#ifdef notdef
	case 17:	/* sbrk <addr> */
		ret = brake;
		brake = arg1;
		carry_clear();
		break;
#endif
	case 18:	/* stat fn buf */
		printf("stat(\"%s\", %x\n", fn, arg2);
		break;
	case 28:	/* fstat fd buf */
		printf("fstat(%d, %x\n", fd, arg2);
		break;
	case 19:	/* seek fd where mode */
		printf("seek(%d, %d, %d)\n", fd, arg1, arg2);
		break;

#ifdef notdef
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
		if (verbose & V_SYS) {
			tty_dump("stty", arg1);
		}
		tcgetattr(fd, &ti);
		settimode(get_word(arg1+4));
		ti.c_cc[VERASE] = get_byte(arg1+2);
		ti.c_cc[VKILL] = get_byte(arg1+3);
		if (verbose & V_SYS) {
			ti_dump();
		}
		tcsetattr(fd, TCSANOW, &ti);
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
		if (verbose & V_SYS) {
			tty_dump("gtty", arg1);
			// ti_dump();
		}
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
#endif

	case 48:	/* set signal handler */
		printf("signal(%d, %x)\n", arg1, arg2);
		break;
	default:
		printf("unrecognized syscall %d %x\n", code, code);
		break;
	}
}
#ifdef notdef
unsigned char pchars[16];
int pcol;

dp()
{
        int i;
        char c;

        for (i = 0; i < pcol; i++) {
                c = pchars[i];
                if ((c <= 0x20) || (c >= 0x7f)) c = '.';
                fprintf(mytty, "%c", c);
        }
        fprintf(mytty, "\n");
}

dumpmem(unsigned char (*readbyte)(void *addr), void *addr, unsigned short len)
{
        int i;
        pcol = 0;

        while (len) {
                if (pcol == 0) fprintf(mytty, "%04x: ", addr);
                fprintf(mytty, "%02x ", pchars[pcol] = (*readbyte)(addr++));
                len--;
                if (pcol++ == 15) {
                        dp();
                        pcol = 0;
                }
        }
        if (pcol != 0) {
                for (i = pcol; i < 16; i++) fprintf(mytty, "   ");
                dp();
        }
}
#endif
