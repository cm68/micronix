/*
 * Z80SIM  -  a Z80-CPU simulator
 *
 * This module is the minimal decision 1
 *
 * Copyright (C) 2014-2017 by Udo Munk
 *
 * History:
 * 15-DEC-14 first version
 * 20-DEC-14 added 4FDC emulation and machine boots CP/M 2.2
 * 28-DEC-14 second version with 16FDC, CP/M 2.2 boots
 * 01-JAN-15 fixed 16FDC, machine now also boots CDOS 2.58 from 8" and 5.25"
 * 01-JAN-15 fixed frontpanel switch settings, added boot flag to fp switch
 * 12-JAN-15 fdc and tu-art improvements, implemented banked memory
 * 24-APR-15 added Cromemco DAZZLER to the machine
 * 01-MAR-16 added sleep for threads before switching tty to raw mode
 * 09-MAY-16 frontpanel configuration with path support
 * 06-DEZ-16 implemented status display and stepping for all machine cycles
 * 13-MAR-17 can't examine/deposit if CPU running HALT instruction
 * 29-JUN-17 system reset overworked
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>
#include "sim.h"
#include "simglb.h"
#include "config.h"
#include "memory.h"
#include "../../iodevices/unix_terminal.h"

extern int load_file(char *);
extern int load_core(void);
extern void cpu_z80(void);

static int reset;
static int power;

static int load(void);
static void run_cpu(void);

/*
 *	This function initialises the front panel and terminal.
 *	Boot code gets loaded if provided and then the machine
 *	waits to be operated from the front panel, until power
 *	switched OFF again.
 */
void mon(void)
{
	static struct timespec timer;
	static struct sigaction newact;

	/* load memory from file */
	if (load())
		exit(1);

	/* give threads a bit time and then empty buffer */
	sleep(1);
	fflush(stdout);

	/* initialise terminal */
	set_unix_terminal();

	/* operate machine from front panel */
	while (cpu_error == NONE) {

		run_cpu();

		/* wait a bit, system is idling */
		timer.tv_sec = 0;
		timer.tv_nsec = 10000000L;
		nanosleep(&timer, NULL);
	}

	/* timer interrupts off */
	newact.sa_handler = SIG_IGN;
	memset((void *) &newact.sa_mask, 0, sizeof(newact.sa_mask));
	newact.sa_flags = 0;
	sigaction(SIGALRM, &newact, NULL);

	/* reset terminal */
	reset_unix_terminal();
	putchar('\n');

	cpu_bus = 0;
	bus_request = 0;
	IFF = 0;

	/* wait a bit before termination */
	sleep(1);
}

/*
 *	Load code into memory from file, if provided
 */
int load(void)
{
	if (l_flag) {
		return(load_core());
	}

	if (x_flag) {
		return(load_file(xfn));
	}

	return(0);
}

/*
 *	Report CPU error
 */
void report_error(void)
{
	switch (cpu_error) {
	case NONE:
		break;
	case OPHALT:
		printf("\r\nINT disabled and HALT Op-Code reached at %04x\r\n",
		       PC - 1);
		break;
	case IOTRAPIN:
		printf("\r\nI/O input Trap at %04x, port %02x\r\n", PC, io_port);
		break;
	case IOTRAPOUT:
		printf("\r\nI/O output Trap at %04x, port %02x\r\n", PC, io_port);
		break;
	case IOHALT:
		printf("\nSystem halted, bye.\n");
		break;
	case IOERROR:
		printf("\r\nFatal I/O Error at %04x\r\n", PC);
		break;
	case OPTRAP1:
		printf("\r\nOp-code trap at %04x %02x\r\n", PC - 1,
		       *(mem_base() + PC - 1));
		break;
	case OPTRAP2:
		printf("\r\nOp-code trap at %04x %02x %02x\r\n",
		       PC - 2, *(mem_base() + PC - 2), *(mem_base() + PC - 1));
		break;
	case OPTRAP4:
		printf("\r\nOp-code trap at %04x %02x %02x %02x %02x\r\n",
		       PC - 4, *(mem_base() + PC - 4), *(mem_base() + PC - 3),
		       *(mem_base() + PC - 2), *(mem_base() + PC - 1));
		break;
	case USERINT:
		printf("\r\nUser Interrupt at %04x\r\n", PC);
		break;
	case POWEROFF:
		printf("\r\nSystem powered off, bye.\r\n");
		break;
	default:
		printf("\r\nUnknown error %d\r\n", cpu_error);
		break;
	}
}

/*
 *	Run CPU
 */
void run_cpu(void)
{
	cpu_state = CONTIN_RUN;
	cpu_error = NONE;
	cpu_z80();
	report_error();
}
