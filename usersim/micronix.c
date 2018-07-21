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
#include <time.h>
#include "z80emu.h"
#include "z80user.h"

typedef unsigned int ULONG;
typedef unsigned char UCHAR;
typedef unsigned short UINT;
#include "../kernel/inode.h"
#include "../include/obj.h"

#define MAXIMUM_STRING_LENGTH   100

static int	do_exec(char *filename);
static void	emulate();

char *rootdir = "../filesystem";
char *initfile = "/etc/init";
int verbose;
int trace;
MACHINE	context;

unsigned short brake;

struct MACHINE *cp;

int 
main(int argc, char **argv)
{
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
			initfile = *argv;
			break;
		}
	}

	if (verbose) { 
		printf("verbose %d\n", verbose);
		printf("emulating %s with root %s\n", initfile, rootdir);
	}

	cp = &context;
        Z80Reset(&cp->state);

        cp->state.pc = 0x100;

	if (do_exec(initfile)) {
		return (EXIT_FAILURE);
	}
	emulate();
        return EXIT_SUCCESS;
}

static int 
do_exec(char *filename)
{
        FILE   	*file;
	struct obj header;

        if ((file = fopen(filename, "rb")) == NULL) {
                fprintf(stderr, "Can't open file!\n");
                exit(EXIT_FAILURE);

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

	cp->memory[8] = 0x76;
        cp->state.pc = header.textoff;
        cp->state.registers.word[Z80_SP] = 0xefff;
	cp->is_done = 0;
	brake = header.dataoff + header.data + header.bss + header.heap;
	
	chroot(rootdir);
	return 0;
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

unsigned char
get_byte(unsigned short addr)
{
	return cp->memory[addr];
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

	if (verbose) {
		dumpcpu();
	}
	calladdr = get_byte(sp) + (get_byte(sp+1) << 8) - 1;

	if ((code = get_byte(calladdr)) != 0xcf) {
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
		printf("%ssyscall %d %04x %04x %04x\n", 
			indirect ? "indirect " : "", code,
			fd, addr, addr2);
	}

	switch (code) {
	case 0:
		printf("double indirect! yarg!\n");
		break;		
	case 1:
		if (trace) printf("exit %04x %04x\n", fd, addr);
		exit(0);
		break;
	case 3:
		if (trace) printf("read fd:%d %04x %04x\n", fd, addr, addr2);
		read(fd, &cp->memory[addr], addr2);
		if (verbose) dumpmem(&get_byte, addr, addr2);
		bytes += 4;
		carry_clear();
		break;
	case 4:
		if (trace) printf("write fd:%d %04x %04x\n", fd, addr, addr2);
		if (verbose) dumpmem(&get_byte, addr, addr2);
		write(fd, &cp->memory[addr], addr2);
		bytes += 4;
		carry_clear();
		break;
	case 5:
		if (trace) printf("open %s %04x %04x\n", &cp->memory[addr], addr, addr2);
		bytes += 4;
		fd = open(&cp->memory[addr], addr);
		cp->state.registers.word[Z80_HL] = fd;
		carry_clear();
		break;
	case 13:	/* r_time */
		i = time(0);
		cp->state.registers.word[Z80_DE] = i & 0xffff;
		cp->state.registers.word[Z80_HL] = (i >> 16) & 0xffff;
		carry_clear();
		if (trace) printf("r_time %x %04x %04x\n", i, (i >> 16) & 0xffff, i & 0xffff);
		break;
	case 17:	/* sbrk */
		if (trace) printf("sbrk %04x\n", addr);
		brake = addr;
		carry_clear();
		bytes += 2;
		break;	
	case 18:
		if (trace) printf("stat(\"%s\", %04x)\n", &cp->memory[addr], addr2);
		stat(&cp->memory[addr], &sbuf);
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
		dumpmem(&get_byte, addr2, 36);
		bytes += 2;
		break;
	case 19:
		if (trace) printf("seek fd:%d %04x %04x\n", fd, addr, addr2);
		bytes += 2;
		break;
	case 20:
		if (trace) printf("getpid fd:%d %04x %04x\n", fd, addr, addr2);
		cp->state.registers.word[Z80_HL] = getpid();
		bytes += 2;
		break;
	case 32:
		if (trace) printf("gtty %04x\n", addr);
		bytes += 2;
		break;

	default:
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
