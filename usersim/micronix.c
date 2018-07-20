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
#include <time.h>
#include "z80emu.h"
#include "z80user.h"

typedef unsigned char UCHAR;
typedef unsigned short UINT;

#include "../include/obj.h"

#define MAXIMUM_STRING_LENGTH   100

static void	do_exec(char *filename);
static void	emulate();

char *rootdir = "../filesystem";
char *initfile = "/etc/init";
int verbose;
MACHINE	context;

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
			case 'v':
				verbose++;
				break;
			default:
				printf("bad flag %c\n", (*argv)[1]);
				break; 
			}
		} else {
			initfile = *argv;
		}
	}

	printf("emulating %s with root %s\n", initfile, rootdir);
	if (verbose) { printf("verbose %d\n", verbose); }

        Z80Reset(&context.state);
        context.state.pc = 0x100;

	do_exec(initfile);
	emulate();
        return EXIT_SUCCESS;
}

/*
 * this is the actual micronix emulator that emulates all the system calls of micronix.
 * initially, it starts with exec of the named file, and then jumps to the emulation
 */
static void do_exec(char *filename)
{
        FILE   	*file;
        long   	l;
	struct obj header;

        printf("emulating \"%s\"...\n", filename);
        if ((file = fopen(filename, "rb")) == NULL) {
                fprintf(stderr, "Can't open file!\n");
                exit(EXIT_FAILURE);

        }
        fseek(file, 0, SEEK_SET);
	fread(&header, 1, sizeof(header), file);
	printf("exec header: magic: %x conf: %x symsize: %d text: %d data: %d bss: %d heap: %d textoff: %x dataoff: %x\n",
		header.ident, header.conf, 
		header.table, header.text, header.data, 
		header.bss, header.heap, 
		header.textoff, header.dataoff);

	fread(context.memory + header.textoff, 1, header.text, file);
	fread(context.memory + header.dataoff, 1, header.data, file);
        fclose(file);

        context.state.pc = header.textoff;
	context.is_done = 0;
}

/*
 * this is the actual micronix emulator that emulates all the system calls of micronix.
 */
static void emulate()
{
	unsigned char *ip;

	do {
		ip = &context.memory[context.state.pc & 0xffff];
		printf("pc = %04x: %02x %02x %02x %02x ", 
			context.state.pc, ip[0], ip[1], ip[2], ip[3]);
		printf("a: %02x bc: %04x de: %04x hl: %04x sp: %04x\n",
			context.state.registers.byte[Z80_A],
			context.state.registers.word[Z80_BC],
			context.state.registers.word[Z80_DE],
			context.state.registers.word[Z80_HL],
			context.state.registers.word[Z80_SP]);

		/* the second arg is the number of cycles we are allowing the emulator to run */
                Z80Emulate(&context.state, 1, &context);
	} while (!context.is_done);
}

/*
 * system call hook
 */
void SystemCall (MACHINE *cp)
{

        if (cp->state.registers.byte[Z80_C] == 2)

                printf("%c", cp->state.registers.byte[Z80_E]);

        else if (cp->state.registers.byte[Z80_C] == 9) {

                int     i, c;

                for (i = cp->state.registers.word[Z80_DE], c = 0; 
                        cp->memory[i] != '$';
                        i++) {

                        printf("%c", cp->memory[i & 0xffff]);
                        if (c++ > MAXIMUM_STRING_LENGTH) {

                                fprintf(stderr,
                                        "String to print is too long!\n");
                                exit(EXIT_FAILURE);

                        }

                }

        }
}
