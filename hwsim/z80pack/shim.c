/*
 * glue shim for z80pack cpu simulator and hwsim
 */

#include "../common/sim.h"
#include "sim.h"
#include "simglb.h"

void
z80_init()
{
}

void
z80_run()
{
	cpu_z80();
}

BYTE
memrdr(unsigned short addr)
{
	return get_byte(addr);
}

BYTE
memwrt(unsigned short addr, BYTE value)
{
	put_byte(addr, value);
}

BYTE
io_in(BYTE addr_low, BYTE addr_hi)
{
	return input(addr_low);
}

BYTE
io_out(BYTE addr_low, BYTE addr_hi, BYTE value)
{
	output(addr_low, value);
}

byte 
z80_get_reg8(enum reg8 r8)
{
    switch (r8) {
    default:
        printf("i don't know about an 8 bit register %d\"n, r16);
        return 0;
    }
}

word 
z80_get_reg16(enum reg16 r16)
{
    switch (r16) {
    default:
        printf("i don't know about a 16 bit register %d\"n, r16);
        return 0;
    }
}

void 
z80_set_reg8(enum reg8 r8, byte v)
{
    switch (r8) {
    default:
        printf("i don't know about an 8 bit register %d\"n, r16);
    }
}

void 
z80_set_reg16(enum reg16 r16, word v)
{
    switch (r16) {
    default:
        printf("i don't know about a 16 bit register %d\"n, r16);
    }
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
