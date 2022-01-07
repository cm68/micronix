/*
 * this file contains the emulator abstraction implementation
 * for Lin Ke-Fong's emulation
 * it is intended to be an interface equivalent to the shim.c in
 * the chips emulation
 */
#include <stdio.h>
#include "z80emu.h"
#include "z80user.h"
#include "../hwsim/common/sim.h"

MACHINE context;
int running;

void
z80_init()
{
	Z80Reset(&context.state);
}

void
z80_run()
{
    Z80Emulate(&context.state, 1, &context);
}

void
put_word(unsigned short addr, unsigned short value)
{
    context.memory[addr] = value & 0xff;
    context.memory[addr + 1] = (value >> 8) & 0xff;
}

void
put_byte(unsigned short addr, unsigned char value)
{
    context.memory[addr] = value;
}

unsigned short
get_word(unsigned short addr)
{
    return context.memory[addr] + (context.memory[addr + 1] << 8);
}

unsigned char
get_byte(unsigned short addr)
{
    addr &= 0xffff;
    return context.memory[addr];
}

unsigned short
z80_get_reg16(enum reg16 r16)
{
    switch (r16) {
    case pc_reg:
        return context.state.pc;
    case sp_reg:
        return context.state.registers.word[Z80_SP];
    case bc_reg:
        return context.state.registers.word[Z80_BC];
    case de_reg:
        return context.state.registers.word[Z80_DE];
    case hl_reg:
        return context.state.registers.word[Z80_HL];
    case ix_reg:
        return context.state.registers.word[Z80_IX];
    case iy_reg:
        return context.state.registers.word[Z80_IY];
    default:
        printf("get unknown 16 bit reg %d\n", r16);
        break;
    }
}

void
z80_set_reg16(enum reg16 r16, unsigned short v)
{
    switch (r16) {
    case pc_reg:
        context.state.pc = v;
        break;
    case sp_reg:
        context.state.registers.word[Z80_SP] = v;
        break;
    case bc_reg:
        context.state.registers.word[Z80_BC] = v;
        break;
    case de_reg:
        context.state.registers.word[Z80_DE] = v;
        break;
    case hl_reg:
        context.state.registers.word[Z80_HL] = v;
        break;
    case ix_reg:
        context.state.registers.word[Z80_IX] = v;
        break;
    case iy_reg:
        context.state.registers.word[Z80_IY] = v;
        break;
    default:
        printf("set unknown 16 bit reg %d\n", r16);
        break;
    }
}

unsigned char 
z80_get_reg8(enum reg8 r8)
{
    switch (r8) {
    case a_reg:
        return context.state.registers.byte[Z80_A];
    case f_reg:
        return context.state.registers.byte[Z80_F];
    case b_reg:
        return context.state.registers.byte[Z80_B];
    case c_reg:
        return context.state.registers.byte[Z80_C];
    case d_reg:
        return context.state.registers.byte[Z80_D];
    case e_reg:
        return context.state.registers.byte[Z80_E];
    case h_reg:
        return context.state.registers.byte[Z80_H];
    case l_reg:
        return context.state.registers.byte[Z80_L];
    case i_reg:
        return context.state.i;
    case r_reg:
        return context.state.r;
    case irr_reg:
        return context.state.iff1 | (context.state.iff2 << 1);
    case control_reg:
        return 0;
    case status_reg:
        if (context.state.status == Z80_STATUS_HALT)
            return S_HLTA;
        return 0;
    default:
        printf("get unknown 8 bit reg %d\n", r8);
        break;
    }
}

void
z80_set_reg8(enum reg8 r8, unsigned char v)
{
    switch (r8) {
    case a_reg:
        context.state.registers.byte[Z80_A] = v;
        break;
    case f_reg:
        context.state.registers.byte[Z80_F] = v;
        break;
    case b_reg:
        context.state.registers.byte[Z80_B] = v;
        break;
    case c_reg:
        context.state.registers.byte[Z80_C] = v;
        break;
    case d_reg:
        context.state.registers.byte[Z80_D] = v;
        break;
    case e_reg:
        context.state.registers.byte[Z80_E] = v;
        break;
    case h_reg:
        context.state.registers.byte[Z80_H] = v;
        break;
    case l_reg:
        context.state.registers.byte[Z80_L] = v;
        break;
    case i_reg:
        context.state.i = v;
        break;
    case r_reg:
        context.state.r = v;
        break;
    case irr_reg:
        break;
    case control_reg:
        break;
    case status_reg:
        context.state.status = v;
        break;
    default:
        printf("set unknown 8 bit reg %d\n", r8);
        break;
    }
}

void
copyout(byte *buf, paddr pa, int len)
{
    while (len--) {
        put_byte(pa++, *buf++);
    }
}

void
copyin(byte *buf, paddr pa, int len)
{
    while (len--) {
        *buf++ = get_byte(pa++);
    }
}


/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
