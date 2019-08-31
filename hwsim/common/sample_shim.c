/*
 * sample glue shim for instruction simulator
 */

#include <stdio.h>
#include "../common/sim.h"

void
z80_init()
{
}

void
z80_run()
{
}

byte
z80_get_reg8(enum reg8 r8)
{
    switch (r8) {
    default:
        printf("unknown 8 bit register %d\n", r8);
        return 0;
    }
}

word
z80_get_reg16(enum reg16 r16)
{
    switch (r16) {
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
        break;
    default:
        printf("unknown 16 bit register %d\n", r16);
    }
}

/*
 * these are the calls by the instruction set into the simulator
 */
unsigned char
memrdr(unsigned short addr)
{
        return get_byte(addr);
}

void
memwrt(unsigned short addr, unsigned char value)
{
        put_byte(addr, value);
}

unsigned char
io_in(unsigned char addr_low, unsigned char addr_hi)
{
        return input(addr_low);
}

void
io_out(unsigned char addr_low, unsigned char addr_hi, unsigned char value)
{
        output(addr_low, value);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */

