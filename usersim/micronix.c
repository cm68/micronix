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

struct MACHINE *cp;

int 
main(int argc, char **argv)
{
	while (--argc) {
		argv++;
		if (**argv == '-') {
			switch ((*argv)[1]) {
			case 'd':
				--argc;
				rootdir = *++argv;
				break;
			case 't':
				trace++;
				break;
			case 'v':
				verbose++;
				trace++;
				break;
			default:
				printf("bad flag %c\n", (*argv)[1]);
				break; 
			}
		} else {
			initfile = *argv;
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
	
	chroot(rootdir);
	return 0;
}

char *
get_symname()
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

	format_instr(cp->state.pc, outbuf, &get_byte, &get_symname);
	printf("%04x: %-30s ", cp->state.pc, outbuf);
}

dumpcpu()
{
	printf("pc:%04x a:%02x bc:%04x de:%04x hl:%04x sp:%04x status:%d\n",
		cp->state.pc,
		cp->state.registers.byte[Z80_A],
		cp->state.registers.word[Z80_BC],
		cp->state.registers.word[Z80_DE],
		cp->state.registers.word[Z80_HL],
		cp->state.registers.word[Z80_SP],
		cp->state.status);
}

unsigned char pchars[16];
int pcol;

dp()
{
	int i;
	char c;

	for (i = 0; i < pcol; i++) {
		c = pchars[i];
		if ((c <= 0x20) || (c >= 0x7f)) c = '.';
		printf("%c", c);
	}
	printf("\n");
}

dumpmem(unsigned short addr, unsigned short len)
{
	int i;
	pcol = 0;

	while (len) {
		if (pcol == 0) printf("%04x: ", addr);
		printf("%02x ", pchars[pcol] = get_byte(addr++));
		len--;
		if (pcol++ == 15) {
			dp();
			pcol = 0;
		}
	}
	if (pcol != 0) {
		for (i = pcol; i < 16; i++) printf("   ");
		dp();
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

	do {
		if (verbose) {
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

	calladdr = get_byte(sp) + (get_byte(sp+1) << 8) - 1;
	// dumpmem(calladdr, 16);

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
		// dumpmem(calladdr, 16);
		if ((code = get_byte(calladdr)) != 0xcf) {
			printf("indir no syscall %d %x!\n", code, calladdr);
		}
		code = get_byte(calladdr+1);
	}

	/* let's build some common args */
	addr = get_byte(calladdr+2) + (get_byte(calladdr+3) << 8);
	addr2 = get_byte(calladdr+4) + (get_byte(calladdr+5) << 8);
	fd = cp->state.registers.word[Z80_HL];

	switch (code) {
	case 0:
		printf("double indirect! yarg!\n");
		break;		
	case 1:
		exit(0);
		break;
	case 13:	/* r_time */
		i = time(0);
		cp->state.registers.word[Z80_DE] = i & 0xffff;
		cp->state.registers.word[Z80_HL] = (i >> 16) & 0xffff;
		if (trace) printf("r_time %x %04x %04x\n", i, (i >> 16) & 0xffff, i & 0xffff);
		break;
	case 17:	/* sbrk */
		if (trace) printf("sbrk %04x\n", addr);
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
		dumpmem(addr2, 36);
		bytes += 4;
	case 32:
		if (trace) printf("gtty %04x\n", addr);
		bytes += 2;
		break;
	case 4:
		if (trace) printf("write fd:%d %04x %04x\n", fd, addr, addr2);
		if (verbose) dumpmem(addr, addr2);
		write(fd, &cp->memory[addr], addr2);
		bytes += 4;
		break;

	default:
		printf("%ssyscall %d\n", indirect ? "indirect " : "", code);
		// dumpmem(calladdr, 16);
		// dumpcpu();
	}
	
	/* this is a return */
	if (indirect) {
		bytes = 3;
	}
	cp->state.pc = get_byte(sp) + (get_byte(sp+1) << 8) + bytes;
	cp->state.registers.word[Z80_SP] += 2;
	cp->state.status = 0;
	// dumpcpu();
}
