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
#include "util.h"

extern byte fubyte(word addr);
extern word fuword(word addr);

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

	word fd;	/* from hl */
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
	printf("micronix ");

	if (sp->flag & (SF_ARG1|SF_NAME)) arg1 = fuword(sc+2);
	if (sp->flag & (SF_ARG2|SF_NAME2)) arg2 = fuword(sc+4);
	if (sp->flag & SF_ARG3) arg3 = fuword(sc+6);
	if (sp->flag & SF_ARG4) arg3 = fuword(sc+8);
	if (sp->flag & SF_FD) fd = z80_get_reg16(hl_reg);
	if (sp->flag & SF_NAME) fn = getname(arg1);
	if (sp->flag & SF_NAME2) fn2 = getname(arg2);

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
		dumpmem(&fubyte, arg1, arg2);
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
	case 12: /* chdir <ptr to name> */
		printf("chdir(\"%s\")\n", fn);
		break;
	case 13:	/* time */
		printf("time()\n");
		break;
	case 14:	/* mknod <name> mode dev (dev == 0) for dir */
		printf("mknod(\"%s\", %x, %x)\n", fn, arg2, arg3);
		break;
	case 15:	/* chmod <name> <mode> */
		printf("chmod(\"%s\", %x)\n", fn, arg2);
		break;
	case 16:	/* chown <name> <mode> */
		printf("chown(\"%s\", %x)\n", fn, arg2);
		break;
	case 17:	/* sbrk <addr> */
		printf("sbrk(%x)\n", arg1);
		break;
	case 18:	/* stat fn buf */
		printf("stat(\"%s\", %x\n", fn, arg2);
		break;
	case 28:	/* fstat fd buf */
		printf("fstat(%d, %x\n", fd, arg2);
		break;
	case 19:	/* seek fd where mode */
		printf("seek(%d, %d, %d)\n", fd, arg1, arg2);
		break;
	case 20:	/* getpid */
		printf("getpid()\n");
		break;
	case 21:	/* mount */
		printf("mount(\"%s\", \"%s\")\n", fn, fn2);
		break;
	case 22:	/* umount */
		printf("umount(\"%s\")\n", fn);
		break;
	case 23:	/* setuid */
		printf("setuid(%x)\n", fd);
		break;
	case 24:	/* getuid */
		printf("getuid()\n");
		break;
	case 25:	/* stime */
		printf("stime()\n");
		break;
	case 31:	/* stty */
		printf("stty(%d, %x)\n", fd, arg1);
		break;
	case 32:	/* gtty */
		printf("gtty(%d, %x)\n", fd, arg1);
		break;
	case 33:	/* access <name> <mode> */
		printf("access(\"%s\", %x)\n", fn, arg2);
		break;
	case 34:	/* nice */
		printf("nice(%d)\n", fd);
		break;
	case 35:	/* sleep */
		printf("sleep(%x)\n", fd);
		break;
	case 36:	/* sync */
		printf("sync()\n");
		break;
	case 37:	/* kill <pid in hl> signal */
		printf("kill(%d, %d)\n", fd, arg1);
		break;
	case 41:
		printf("dup(%d)\n", fd);
		break;
	case 42:
		printf("pipe()\n");
		break;
	case 48:	/* set signal handler */
		printf("signal(%d, %x)\n", arg1, arg2);
		break;
	default:
		printf("unrecognized syscall %d %x\n", code, code);
		break;
	}
	dumpmem(&fubyte, sc, sp->argbytes + 1);
}
