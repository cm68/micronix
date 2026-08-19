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
#include "z80glue.h"
#include "hwsim.h"
#include "mnix.h"
#include "util.h"
#include "disz80.h"

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
 * Watching only the calls is half an observation, and it is the half
 * that cannot tell a request from an answer: a trace ending at
 * sbrk(4723) does not say whether the break moved, and a stat that is
 * the last line in the log might have failed, returned, or never come
 * back at all.
 *
 * So the call arms the address the caller resumes at, and the next M1
 * fetch there reports what came back.
 *
 * What to look at there is the carry flag and HL, not BC.  The kernel
 * answers with carry clear and its result in HL, or carry set and an
 * error number in HL; BC belongs to the library wrapper, which sets it
 * afterwards - "ld bc,0 / ret nc" leaving a call that worked, "dec bc"
 * with the errno stored leaving one that did not.  Reading BC at the
 * resume point gives you whatever the caller happened to be holding,
 * which reads convincingly as a result and is not one.
 *
 * usersim prints the call and its result on one line because it is the
 * kernel and knows both at once.  Here a real kernel runs in between, so
 * the two are separate lines with whatever the kernel did between them -
 * which is usually the interesting part anyway.
 */
/*
 * One slot, which is not enough: fork leaves two processes making calls
 * and whichever calls next takes the slot, so a return can go unreported
 * - stat("/dev/ttyB") loses its answer to a wait() from the child.  A
 * slot per task would fix it and has not been needed yet.
 */
#define NSLOTS 8
static word retpc[NSLOTS];
static char *retname[NSLOTS];
static int armed[NSLOTS];

static void
syscall_arm(word addr, char *name)
{
    int i;

    for (i = 0; i < NSLOTS; i++) {
        if (!armed[i]) {
            retpc[i] = addr;
            retname[i] = name;
            armed[i] = 1;
            return;
        }
    }
}

void
syscall_return(word addr)
{
    word hl;
    byte f;
    int i;

    for (i = 0; i < NSLOTS; i++) {
        if (!armed[i] || addr != retpc[i])
            continue;
        armed[i] = 0;
        hl = z80_get_reg16(hl_reg);
        f = z80_get_reg8(f_reg);
        if (f & 1) {
            printf("micronix %s = ERROR %d\n", retname[i] ? retname[i] : "?", hl);
        } else {
            printf("micronix %s = %d (0x%04x)\n", retname[i] ? retname[i] : "?",
                (short) hl, hl);
        }
        return;
    }
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
	word retaddr;	/* where the caller comes back to */

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
	retaddr = sc;           /* rst pushed the byte after itself */
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
		/*
		 * The descriptor is elsewhere, but the caller still resumes
		 * after its own four bytes - rst, zero, and the address.
		 */
		retaddr = sc + 4;
		sc = fuword(sc + 2);
		if ((code = fubyte(sc)) != 0xcf) {
			printf("indir no syscall %d %x!\n", code, sc);
			return;
		}
		code = fubyte(sc + 1);
	}

	/*
	 * A code the table does not describe is not a reason to read
	 * past the end of it.  The entry decides how many inline argument
	 * bytes follow the rst, and the dump below and the return address
	 * are both worked out from that, so a garbage argbytes dumps a
	 * garbage-sized region and resumes in the wrong place - which is
	 * a wedged machine, from tracing alone.  The standalone 1.67
	 * system makes such a call at boot, so this is on the path to
	 * every traced run, not a corner.  mnix_sys.c bounds its own
	 * lookup the same way.
	 */
	if (code < 0 || code >= nsyscalls) {
		printf("unrecognized syscall %d %x, not traced\n", code, code);
		return;
	}
	sp = &syscalls[code];
	{
		char sbuf[16];
		printf("%s ", dis_space(sc, sbuf, sizeof(sbuf)));
	}
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
	case 11: /* exec <name> <argv> */
		/*
		 * arg2 is argv: a null terminated vector of pointers to
		 * strings, all in the caller's address space.  Printing them
		 * is the only way to see what init is trying to run and with
		 * what, which is the question when a system boots to a banner
		 * and then goes quiet.
		 */
		printf("exec(\"%s\"", fn);
		if (arg2) {
			word p;
			int i;

			for (i = 0; i < 16; i++) {
				p = fuword(arg2 + i * 2);
				if (!p)
					break;
				printf(", \"%s\"", getname(p));
			}
			if (i == 16)
				printf(", ...");
		}
		printf(")\n");
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

	/*
	 * Arm the return.  Watching only the calls is half an observation,
	 * and it is the half that cannot tell a request from an answer: a
	 * trace that ends at sbrk(4723) does not say whether the break
	 * moved, and a stat that is the last thing in the log might have
	 * failed, or returned, or never come back at all.
	 *
	 * For a direct call the caller resumes past the code byte and the
	 * inline arguments, which is argbytes + 1 from the rst; an indirect
	 * one resumes after its own four bytes and that was worked out
	 * above.
	 */
	if (!indirect) {
		retaddr = sc + sp->argbytes + 1;
	}
	syscall_arm(retaddr, sp->name);
}

