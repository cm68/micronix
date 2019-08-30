/*
 * glue shim for z80pack cpu simulator and hwsim
 */

#include "../common/sim.h"
#include "sim.h"
#include "simglb.h"

byte cpu_control;
#define	CPU_NMI		0x01
#define	CPU_INT		0x02
#define	CPU_RESET	0x04	

void
z80_init(struct cpuregs *cp)
{
	cp->f_ptr = (byte *)&F;	
	cp->a_ptr = (byte *)&A;	
	cp->b_ptr = (byte *)&B;	
	cp->c_ptr = (byte *)&C;	
	cp->d_ptr = (byte *)&D;	
	cp->e_ptr = (byte *)&E;	
	cp->h_ptr = (byte *)&H;	
	cp->l_ptr = (byte *)&L;	
	cp->ix_ptr = (word *)&IX;
	cp->iy_ptr = (word *)&IY;
	cp->sp_ptr = (word *)&SP;
	cp->pc_ptr = (word *)&PC;
	cp->r_ptr = (byte *)&R;
	cp->i_ptr = (byte *)&I;
	cp->status = &cpu_bus;
	cp->sbits[S_M1] = CPU_M1;
	cp->sbits[S_INTA] = CPU_INTA;
	cp->sbits[S_HLTA] = CPU_HLTA;
	cp->control = &cpu_control;
	cp->sbits[C_RESET] = CPU_RESET;
	cp->sbits[C_INT] = CPU_INT;
	cp->sbits[C_NMI] = CPU_NMI;
}

void
z80_run()
{
	if (cpu_control & C_RESET) {
		cpu_state |= RESET;
	}
	if (cpu_control & C_NMI) {
		int_nmi = 1;
	}
	if (cpu_control & C_INT) {
		int_int = 1;
	}

	cpu_z80();

	if (cpu_state & RESET) {
		cpu_control |= C_RESET;
	} else {
		cpu_control &= ~C_RESET;
	}
	if (int_int) {
		cpu_control |= C_INT;
	} else {
		cpu_control &= ~C_INT;
	}
	if (int_nmi) {
		cpu_control |= C_NMI;
	} else {
		cpu_control &= ~C_NMI;
	}
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


