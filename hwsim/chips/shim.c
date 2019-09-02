#define CHIPS_IMPL

#include <stdio.h>
#include "../common/sim.h"
#include "z80.h"

byte control;
byte status;

z80_desc_t context;
z80_t z80;

/*
 * this simulator is pretty magic in that it actually simulates what happens on the Z80's pins
 * and we get a callback to run machine cycles
 */
uint64_t 
z80_tick(int num_ticks, uint64_t pins, void* user_data) {
    if (pins & Z80_MREQ) {
        if (pins & Z80_RD) {
            if (pins & Z80_M1) {
                status |= S_M1;
            }
            Z80_SET_DATA(pins, get_byte(Z80_GET_ADDR(pins)));
            status &= ~S_M1;
        }
        else if (pins & Z80_WR) {
            put_byte(Z80_GET_ADDR(pins), Z80_GET_DATA(pins));
        }
    } else if (pins & Z80_IORQ) {
        if (pins & Z80_RD) {
            Z80_SET_DATA(pins, input(Z80_GET_ADDR(pins)));
        } else if (pins & Z80_WR) {
            output(Z80_GET_ADDR(pins), Z80_GET_DATA(pins));
        }
    }
    return pins;
}

void
z80_init()
{
    context.tick_cb = z80_tick;
	z80init(&z80, &context);
}

void
z80_run()
{
	do {
		z80_exec(&z80, 1);
	} while (!z80_opdone(&z80));
}

byte
z80_get_reg8(enum reg8 r8)
{
    switch (r8) {
    case a_reg:
        return z80_a(&z80);
    case f_reg:
        return z80_f(&z80);
    case status_reg:
        return status;
    default:
        printf("get unknown 8 bit register %d\n", r8);
        return 0;
    }
}

word
z80_get_reg16(enum reg16 r16)
{
    switch (r16) {
    case bc_reg:
        return z80_bc(&z80);
    case de_reg:
        return z80_de(&z80);
    case hl_reg:
        return z80_hl(&z80);
    case ix_reg:
        return z80_ix(&z80);
    case iy_reg:
        return z80_iy(&z80);
    case sp_reg:
        return z80_sp(&z80);
    case pc_reg:
        return z80_pc(&z80);
    default:
        printf("get unknown 16 bit register %d\n", r16);
        return 0;
    }
}

void
z80_set_reg8(enum reg8 r8, byte v)
{
    switch (r8) {
    default:
        printf("set unknown 8 bit register %d\n", r8);
    }
}

void
z80_set_reg16(enum reg16 r16, word v)
{
    switch (r16) {
    case pc_reg:
        z80_set_pc(&z80, v);
        break;
    default:
        printf("set unknown 16 bit register %d\n", r16);
    }
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */