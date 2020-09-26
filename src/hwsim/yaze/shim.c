/*
 * glue shim for yaze cpu simulator and hwsim
 */

#include <stdio.h>
#include "../common/sim.h"
#include "shim.h"
#include "yaze.h"

byte status;

void
z80_init()
{
}

void
z80_run()
{
	simz80(pc);
}

BYTE
GetOpCode(unsigned short addr)
{
	byte temp;

	status |= S_M1;
	temp = get_byte(addr);
	status &= ~S_M1;

	return (temp);
}

BYTE
io_in(BYTE addr_low, BYTE addr_hi)
{
	return input(addr_low | (addr_hi << 8));
}

BYTE
io_out(BYTE addr_low, BYTE addr_hi, BYTE value)
{
	output(addr_low | (addr_hi << 8), value);
}

byte
z80_get_reg8(enum reg8 r8)
{
    switch (r8) {
    case status_reg:
        return status;
    default:
        printf("unknown 8 bit register %d\n", r8);
        return 0;
    }
}

word
z80_get_reg16(enum reg16 r16)
{
    switch (r16) {
    case pc_reg:
        return pc;
    default:
        printf("unknown 16 bit register %d\n", r16);
        return 0;
    }
}

void
z80_set_reg8(enum reg8 r8, byte v)
{
    switch (r8) {
    default:
        printf("unknown 8 bit register %d\n", r8);
    }
}

void
z80_set_reg16(enum reg16 r16, word v)
{
    switch (r16) {
    case pc_reg:
        pc = v;
        break;
    default:
        printf("unknown 16 bit register %d\n", r16);
    }
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
